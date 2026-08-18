#ifndef DESKTOPBUILDER_H
#define DESKTOPBUILDER_H

#include "packagebuilder.h"

class DesktopBuilder : public PackageBuilder
{
    Q_OBJECT

public:
    explicit DesktopBuilder(QObject *parent = nullptr);
    ~DesktopBuilder() override = default;

    static QString desktopFileName(const PackageMetadata &meta);
    static bool generateDesktopFile(const PackageMetadata &meta, const QString &filePath);
    bool build(const PackageMetadata &meta, const QString &outputPath) override;
};

#endif
