#pragma once

#include <QString>
#include <QDateTime>
#include <QFileInfo>
#include <QIcon>
#include <QMetaType>

struct SearchResult {
    QString filePath;
    QString fileName;
    QString dirPath;
    qint64 fileSize;
    QDateTime lastModified;
    QDateTime created;
    bool isDir;
    QString mimeType;
    QString snippet;

    static SearchResult fromFileInfo(const QFileInfo &fi) {
        SearchResult r;
        r.filePath = fi.absoluteFilePath();
        r.fileName = fi.fileName();
        r.dirPath = fi.absolutePath();
        r.fileSize = fi.size();
        r.lastModified = fi.lastModified();
        r.created = fi.birthTime();
        r.isDir = fi.isDir();
        return r;
    }

    QString sizeString() const {
        if (fileSize < 1024) return QString::number(fileSize) + " B";
        if (fileSize < 1024 * 1024) return QString::number(fileSize / 1024.0, 'f', 1) + " KB";
        if (fileSize < 1024LL * 1024 * 1024) return QString::number(fileSize / (1024.0 * 1024.0), 'f', 1) + " MB";
        return QString::number(fileSize / (1024.0 * 1024.0 * 1024.0), 'f', 2) + " GB";
    }
};

Q_DECLARE_METATYPE(SearchResult)
