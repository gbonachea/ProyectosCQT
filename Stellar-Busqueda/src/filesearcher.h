#pragma once

#include <QObject>
#include <QAtomicInt>
#include <QVector>
#include <QMutex>
#include <QStringList>
#include "searchresult.h"

class FileSearcher : public QObject {
    Q_OBJECT
public:
    explicit FileSearcher(QObject *parent = nullptr);

    void setSearchDirs(const QStringList &dirs);
    void setExtensions(const QStringList &exts);
    void setKeywords(const QStringList &keywords);
    void setExactPhrases(const QStringList &phrases);
    void setSearchContent(bool searchContent);
    void setMaxResults(int max);
    void setMinFileSize(qint64 minBytes);

public slots:
    void startSearch();
    void stopSearch();

signals:
    void resultFound(const SearchResult &result);
    void searchProgress(int filesScanned, int dirsScanned);
    void searchFinished(int totalResults);
    void searchError(const QString &error);

private:
    void searchDirectory(const QString &dirPath);
    bool matchesQuery(const QFileInfo &fi);
    bool matchesName(const QString &fileName);
    bool matchesContent(const QString &filePath, const QStringList &keywords);
    bool shouldExcludeDir(const QString &dirName);

    QStringList searchDirs;
    QStringList extensions;
    QStringList keywords;
    QStringList exactPhrases;
    bool searchContent;
    int maxResults;
    qint64 minFileSize;

    QAtomicInt stopFlag;
    QAtomicInt fileCount;
    QAtomicInt dirCount;
    int resultCount;
    QMutex resultMutex;
};
