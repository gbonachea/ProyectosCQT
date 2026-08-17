#include "debbuilder.h"

#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QTextStream>
#include <QTemporaryDir>

DebBuilder::DebBuilder(QObject *parent)
    : PackageBuilder(parent)
{
}

QString DebBuilder::debPackageName(const QString &name) const
{
    QString n = name.toLower();
    n.replace(QLatin1Char('_'), QLatin1Char('-'));
    return n;
}

bool DebBuilder::build(const PackageMetadata &meta, const QString &outputPath)
{
    QTemporaryDir tempDir;
    tempDir.setAutoRemove(true);
    if (!tempDir.isValid()) {
        emit logMessage(QStringLiteral("Error: no se pudo crear un directorio temporal."));
        return false;
    }

    const QString pkgName = debPackageName(meta.name);
    if (pkgName != meta.name) {
        emit logMessage(QStringLiteral("Advertencia: el nombre \"%1\" se ajustó a \"%2\" "
                                       "para cumplir las reglas de nomenclatura de dpkg.")
                            .arg(meta.name, pkgName));
    }

    const QString rootDir = tempDir.path();
    const QString debianDir = rootDir + QStringLiteral("/DEBIAN");
    if (!createDirectory(debianDir)) {
        emit logMessage(QStringLiteral("Error: no se pudo crear el directorio DEBIAN."));
        return false;
    }

    quint64 installedSize = 0;
    for (const FileMapping &file : meta.files) {
        const QString rel = normalizeTargetPath(file.targetPath);
        if (rel.isEmpty()) {
            emit logMessage(QStringLiteral("Advertencia: ruta de destino inválida ignorada para \"%1\".")
                                .arg(file.sourcePath));
            continue;
        }

        const QString dest = rootDir + QLatin1Char('/') + rel;
        if (!createDirectory(QFileInfo(dest).absolutePath())) {
            emit logMessage(QStringLiteral("Error: no se pudo crear el directorio para \"%1\".").arg(dest));
            return false;
        }
        if (!copyFileTo(file.sourcePath, dest)) {
            emit logMessage(QStringLiteral("Error: no se pudo copiar \"%1\" a \"%2\".")
                                .arg(file.sourcePath, dest));
            return false;
        }
        installedSize += QFileInfo(file.sourcePath).size();
    }

    const QString controlPath = debianDir + QStringLiteral("/control");
    PackageMetadata debMeta = meta;
    debMeta.name = pkgName;
    if (!writeControlFile(controlPath, debMeta, installedSize)) {
        emit logMessage(QStringLiteral("Error: no se pudo escribir el archivo de control."));
        return false;
    }

    const QString debFileName = QStringLiteral("%1_%2_%3.deb")
                                    .arg(pkgName, meta.version, meta.architecture);
    const QString debOut = QDir(outputPath).filePath(debFileName);
    if (QFileInfo::exists(debOut))
        QFile::remove(debOut);

    if (!runProcess(QStringLiteral("dpkg-deb"),
                    { QStringLiteral("--build"), QStringLiteral("--root-owner-group"),
                      rootDir, debOut })) {
        emit logMessage(QStringLiteral("Error: dpkg-deb falló al construir el paquete .deb."));
        return false;
    }

    emit logMessage(QStringLiteral("Paquete .deb creado: %1").arg(debOut));
    return true;
}

bool DebBuilder::writeControlFile(const QString &controlPath, const PackageMetadata &meta,
                                  quint64 installedSize) const
{
    QFile file(controlPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << "Package: " << meta.name << '\n';
    out << "Version: " << meta.version << '\n';
    out << "Architecture: " << meta.architecture << '\n';
    out << "Maintainer: " << meta.maintainer << '\n';
    out << "Installed-Size: " << (installedSize / 1024 + 1) << '\n';
    if (!meta.dependencies.isEmpty())
        out << "Depends: " << meta.dependencies.join(QStringLiteral(", ")) << '\n';
    out << "Section: utils\n";
    out << "Priority: optional\n";
    out << "License: " << meta.license << '\n';
    out << "Description: " << meta.shortDescription << '\n';

    const QString longDesc = meta.longDescription.trimmed();
    if (!longDesc.isEmpty()) {
        const QStringList lines = longDesc.split(QLatin1Char('\n'));
        for (const QString &line : lines)
            out << ' ' << line << '\n';
    }

    out.flush();
    return file.error() == QFileDevice::NoError;
}
