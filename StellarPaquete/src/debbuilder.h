#ifndef DEBBUILDER_H
#define DEBBUILDER_H

#include "packagebuilder.h"

class DebBuilder : public PackageBuilder
{
    Q_OBJECT

public:
    explicit DebBuilder(QObject *parent = nullptr);
    ~DebBuilder() override = default;

    bool build(const PackageMetadata &meta, const QString &outputPath) override;

private:
    QString debPackageName(const QString &name) const;
    bool writeControlFile(const QString &controlPath, const PackageMetadata &meta,
                          quint64 installedSize) const;
};

#endif
