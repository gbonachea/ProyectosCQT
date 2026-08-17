#ifndef DESKTOPBUILDER_H
#define DESKTOPBUILDER_H

#include "packagebuilder.h"

class DesktopBuilder : public PackageBuilder
{
    Q_OBJECT

public:
    explicit DesktopBuilder(QObject *parent = nullptr);
    ~DesktopBuilder() override = default;

    bool build(const PackageMetadata &meta, const QString &outputPath) override;
};

#endif
