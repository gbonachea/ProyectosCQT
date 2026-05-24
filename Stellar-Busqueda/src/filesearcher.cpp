#include "filesearcher.h"
#include <QDir>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>
#include <QFile>
#include <QThread>
#include <QDebug>

FileSearcher::FileSearcher(QObject *parent)
    : QObject(parent), maxResults(1000), minFileSize(0),
      searchContent(false), resultCount(0) {
    searchDirs = QStringList(QDir::homePath());
}

void FileSearcher::setSearchDirs(const QStringList &dirs) { searchDirs = dirs; }
void FileSearcher::setExtensions(const QStringList &exts) { extensions = exts; }
void FileSearcher::setKeywords(const QStringList &kws) { keywords = kws; }
void FileSearcher::setExactPhrases(const QStringList &phrases) { exactPhrases = phrases; }
void FileSearcher::setSearchContent(bool sc) { searchContent = sc; }
void FileSearcher::setMaxResults(int max) { maxResults = max; }
void FileSearcher::setMinFileSize(qint64 bytes) { minFileSize = bytes; }

void FileSearcher::startSearch() {
    stopFlag.storeRelaxed(0);
    fileCount.storeRelaxed(0);
    dirCount.storeRelaxed(0);
    resultCount = 0;

    for (const QString &dir : searchDirs) {
        if (stopFlag.loadRelaxed()) break;
        QDir d(dir);
        if (d.exists()) {
            searchDirectory(dir);
        }
    }

    emit searchFinished(resultCount);
}

void FileSearcher::stopSearch() {
    stopFlag.storeRelaxed(1);
}

void FileSearcher::searchDirectory(const QString &dirPath) {
    if (stopFlag.loadRelaxed()) return;

    QDir dir(dirPath);
    if (!dir.exists()) return;

    dirCount.fetchAndAddRelaxed(1);

    QFileInfoList entries = dir.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot,
                                              QDir::Name);

    for (const QFileInfo &entry : entries) {
        if (stopFlag.loadRelaxed()) return;

        if (entry.isDir()) {
            QString dirName = entry.fileName();
            if (shouldExcludeDir(dirName)) continue;
            searchDirectory(entry.absoluteFilePath());
        } else if (entry.isFile()) {
            fileCount.fetchAndAddRelaxed(1);

            if (fileCount.loadRelaxed() % 100 == 0) {
                emit searchProgress(fileCount.loadRelaxed(), dirCount.loadRelaxed());
            }

            if (matchesQuery(entry)) {
                QMutexLocker locker(&resultMutex);
                if (resultCount >= maxResults) return;
                resultCount++;

                SearchResult sr = SearchResult::fromFileInfo(entry);
                emit resultFound(sr);
            }
        }
    }
}

bool FileSearcher::matchesQuery(const QFileInfo &fi) {
    if (fi.isDir()) return false;
    if (fi.size() < minFileSize) return false;

    QString fileName = fi.fileName().toLower();

    if (!extensions.isEmpty()) {
        QString suffix = fi.suffix().toLower();
        if (!extensions.contains(suffix)) return false;
    }

    if (keywords.isEmpty() && exactPhrases.isEmpty()) return true;

    bool nameMatch = matchesName(fileName);

    if (nameMatch) return true;

    if (searchContent && !keywords.isEmpty()) {
        return matchesContent(fi.absoluteFilePath(), keywords);
    }

    return false;
}

bool FileSearcher::matchesName(const QString &fileName) {
    for (const QString &phrase : exactPhrases) {
        if (fileName.contains(phrase)) return true;
    }

    int matchCount = 0;
    for (const QString &kw : keywords) {
        if (fileName.contains(kw)) {
            matchCount++;
        }
    }

    if (keywords.isEmpty()) return !exactPhrases.isEmpty();
    return matchCount > 0;
}

bool FileSearcher::matchesContent(const QString &filePath, const QStringList &kws) {
    QString suffix = QFileInfo(filePath).suffix().toLower();

    QStringList textExtensions = {"txt", "md", "csv", "log", "xml", "json", "yaml",
                                  "yml", "ini", "cfg", "conf", "html", "htm", "css",
                                  "js", "py", "cpp", "c", "h", "hpp", "java", "rb",
                                  "php", "pl", "sh", "bat", "ps1", "tex", "rtf"};
    if (!textExtensions.contains(suffix)) return false;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;

    QTextStream in(&file);
    in.setCodec("UTF-8");

    const int maxLines = 1000;
    for (int i = 0; i < maxLines && !in.atEnd(); i++) {
        QString line = in.readLine().toLower();
        int matchCount = 0;
        for (const QString &kw : kws) {
            if (line.contains(kw)) matchCount++;
        }
        if (matchCount > 0) return true;
    }

    return false;
}

bool FileSearcher::shouldExcludeDir(const QString &dirName) {
    static const QStringList excluded = {
        "proc", "sys", "dev", "run", "lost+found",
        "proc", "sys", "etc", "usr", "boot",
        "var/lib", "var/log", "var/cache",
        "snap", "flatpak", "steam", ".steam",
        ".cache", ".local/share/Trash",
        "node_modules", ".git", "__pycache__",
        ".opencode", ".var"
    };
    return excluded.contains(dirName);
}
