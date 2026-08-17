#ifndef RPMBUILDER_H
#define RPMBUILDER_H

#include "packagebuilder.h"

class RpmBuilder : public PackageBuilder
{
    Q_OBJECT

public:
    explicit RpmBuilder(QObject *parent = nullptr);
    ~RpmBuilder() override = default;

    bool build(const PackageMetadata &meta, const QString &outputPath) override;

private:
    bool writeSpecFile(const QString &specPath, const PackageMetadata &meta,
                       const QString &stagingDir, const QString &arch) const;
    QString rpmArch(const QString &arch) const;
    QString rpmPackageName(const PackageMetadata &meta, const QString &arch) const;
};

#endif
