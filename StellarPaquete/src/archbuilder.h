#ifndef ARCHBUILDER_H
#define ARCHBUILDER_H

#include "packagebuilder.h"

class ArchBuilder : public PackageBuilder
{
    Q_OBJECT

public:
    explicit ArchBuilder(QObject *parent = nullptr);
    ~ArchBuilder() override = default;

    bool build(const PackageMetadata &meta, const QString &outputPath) override;

private:
    QString archName(const QString &arch) const;
    bool writePKGBUILD(const QString &path, const PackageMetadata &meta,
                        const QStringList &fileEntries) const;
};

#endif
