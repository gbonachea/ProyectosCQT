#ifndef APPIMAGEBUILDER_H
#define APPIMAGEBUILDER_H

#include "packagebuilder.h"

class AppImageBuilder : public PackageBuilder
{
    Q_OBJECT

public:
    explicit AppImageBuilder(QObject *parent = nullptr);
    ~AppImageBuilder() override = default;

    bool build(const PackageMetadata &meta, const QString &outputPath) override;

private:
    QString findExecutablePath(const PackageMetadata &meta) const;
    QString findIconPath(const PackageMetadata &meta) const;
    bool writeAppRun(const QString &appRunPath, const PackageMetadata &meta,
                     const QString &execRel) const;
    bool writeDesktopFile(const QString &desktopPath, const PackageMetadata &meta,
                          const QString &iconName) const;
    bool createDefaultIcon(const QString &iconPath, const QString &label) const;
};

#endif
