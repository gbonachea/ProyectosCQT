#ifndef PACKAGEMETADATA_H
#define PACKAGEMETADATA_H

#include <QString>
#include <QStringList>
#include <QVector>

struct FileMapping {
    QString sourcePath;
    QString targetPath;
};

struct DesktopOptions {
    bool generate = false;
    QString name;
    QString comment;
    QString exec;
    QString icon;
    QString categories;
    QString mimeTypes;
    bool terminal = false;
    bool startupNotify = true;
};

struct PackageMetadata {
    QString name;
    QString version;
    QString architecture;
    QString maintainer;
    QString license;
    QString shortDescription;
    QString longDescription;
    QStringList formats;
    QStringList dependencies;
    QVector<FileMapping> files;
    DesktopOptions desktop;
    QString outputDirectory;
};

#endif
