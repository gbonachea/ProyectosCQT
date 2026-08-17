#ifndef PACKAGEBUILDER_H
#define PACKAGEBUILDER_H

#include <QObject>
#include <QString>
#include <QStringList>

#include "packagemetadata.h"

class PackageBuilder : public QObject
{
    Q_OBJECT

public:
    explicit PackageBuilder(QObject *parent = nullptr);
    ~PackageBuilder() override = default;

    virtual bool build(const PackageMetadata &meta, const QString &outputPath) = 0;

    static PackageBuilder *createBuilder(const QString &format, QObject *parent = nullptr);
    static bool toolAvailable(const QString &tool);

signals:
    void logMessage(const QString &message);

protected:
    bool runProcess(const QString &program, const QStringList &arguments);
    bool copyFileTo(const QString &source, const QString &dest);
    bool createDirectory(const QString &path);
    QString normalizeTargetPath(const QString &path) const;
};

#endif
