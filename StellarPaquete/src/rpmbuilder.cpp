#include "rpmbuilder.h"
#include "desktopbuilder.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QTextStream>
#include <QTemporaryDir>

RpmBuilder::RpmBuilder(QObject *parent)
    : PackageBuilder(parent)
{
}

QString RpmBuilder::rpmArch(const QString &arch) const
{
    const QString a = arch.trimmed().toLower();
    if (a == QStringLiteral("amd64"))
        return QStringLiteral("x86_64");
    if (a == QStringLiteral("noarch"))
        return QStringLiteral("noarch");
    if (a.isEmpty())
        return QStringLiteral("noarch");
    return a;
}

QString RpmBuilder::rpmPackageName(const PackageMetadata &meta, const QString &arch) const
{
    return QStringLiteral("%1-%2-1.%3.rpm").arg(meta.name, meta.version, arch);
}

bool RpmBuilder::build(const PackageMetadata &meta, const QString &outputPath)
{
    QTemporaryDir tempDir;
    tempDir.setAutoRemove(true);
    if (!tempDir.isValid()) {
        emit logMessage(QStringLiteral("Error: no se pudo crear un directorio temporal."));
        return false;
    }

    const QString topDir = tempDir.path();
    const QString stagingDir = topDir + QStringLiteral("/staging");
    const QString specDir = topDir + QStringLiteral("/SPECS");
    if (!createDirectory(stagingDir) || !createDirectory(specDir)) {
        emit logMessage(QStringLiteral("Error: no se pudieron crear los directorios de trabajo."));
        return false;
    }

    for (const FileMapping &file : meta.files) {
        const QString rel = normalizeTargetPath(file.targetPath);
        if (rel.isEmpty()) {
            emit logMessage(QStringLiteral("Advertencia: ruta de destino inválida ignorada para \"%1\".")
                                .arg(file.sourcePath));
            continue;
        }

        const QString dest = stagingDir + QLatin1Char('/') + rel;
        if (!createDirectory(QFileInfo(dest).absolutePath())) {
            emit logMessage(QStringLiteral("Error: no se pudo crear el directorio para \"%1\".").arg(dest));
            return false;
        }
        if (!copyFileTo(file.sourcePath, dest)) {
            emit logMessage(QStringLiteral("Error: no se pudo copiar \"%1\" a \"%2\".")
                                .arg(file.sourcePath, dest));
            return false;
        }
    }

    if (meta.desktop.generate) {
        const QString appsDir = stagingDir + QStringLiteral("/usr/share/applications");
        if (!createDirectory(appsDir)) {
            emit logMessage(QStringLiteral("Error: no se pudo crear el directorio usr/share/applications."));
            return false;
        }
        const QString desktopName = DesktopBuilder::desktopFileName(meta);
        const QString desktopPath = appsDir + QLatin1Char('/') + desktopName;
        if (!DesktopBuilder::generateDesktopFile(meta, desktopPath)) {
            emit logMessage(QStringLiteral("Error: no se pudo generar el archivo .desktop."));
            return false;
        }
        emit logMessage(QStringLiteral("Archivo .desktop incluido: usr/share/applications/%1").arg(desktopName));
    }

    const QString arch = rpmArch(meta.architecture);
    const QString specPath = specDir + QLatin1Char('/') + meta.name + QStringLiteral(".spec");
    if (!writeSpecFile(specPath, meta, stagingDir, arch)) {
        emit logMessage(QStringLiteral("Error: no se pudo escribir el archivo .spec."));
        return false;
    }

    QStringList args = { QStringLiteral("-bb"),
                         QStringLiteral("--define"), QStringLiteral("_topdir %1").arg(topDir),
                         QStringLiteral("--define"), QStringLiteral("debug_package %{nil}") };
    args << specPath;

    if (!runProcess(QStringLiteral("rpmbuild"), args)) {
        emit logMessage(QStringLiteral("Error: rpmbuild falló al construir el paquete .rpm."));
        return false;
    }

    const QString builtRpm = QDir(topDir).filePath(QStringLiteral("RPMS/") + arch + QLatin1Char('/') + rpmPackageName(meta, arch));
    const QString finalRpm = QDir(outputPath).filePath(rpmPackageName(meta, arch));
    if (!QFileInfo::exists(builtRpm)) {
        emit logMessage(QStringLiteral("Error: no se encontró el paquete generado: %1").arg(builtRpm));
        return false;
    }
    if (QFileInfo::exists(finalRpm))
        QFile::remove(finalRpm);
    if (!copyFileTo(builtRpm, finalRpm)) {
        emit logMessage(QStringLiteral("Error: no se pudo copiar el paquete a %1.").arg(finalRpm));
        return false;
    }

    emit logMessage(QStringLiteral("Paquete .rpm creado: %1").arg(finalRpm));
    return true;
}

bool RpmBuilder::writeSpecFile(const QString &specPath, const PackageMetadata &meta,
                               const QString &stagingDir, const QString &arch) const
{
    QFile file(specPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << "Name: " << meta.name << '\n';
    out << "Version: " << meta.version << '\n';
    out << "Release: 1\n";
    out << "Summary: " << meta.shortDescription << '\n';
    out << "License: " << meta.license << '\n';
    out << "Group: Applications/System\n";
    out << "AutoReqProv: no\n";
    for (const QString &dep : meta.dependencies)
        out << "Requires: " << dep << '\n';
    if (arch == QStringLiteral("noarch"))
        out << "BuildArch: noarch\n";
    else
        out << "BuildArch: " << arch << '\n';

    out << '\n' << "%description\n";
    out << meta.longDescription.trimmed() << '\n';

    out << '\n' << "%prep\n";
    out << "mkdir -p %{_builddir}\n";

    out << '\n' << "%build\n";

    out << '\n' << "%install\n";
    out << "rm -rf %{buildroot}\n";
    out << "mkdir -p %{buildroot}\n";
    out << "cp -a " << stagingDir << "/. %{buildroot}/\n";

    out << '\n' << "%files\n";
    for (const FileMapping &file : meta.files) {
        const QString rel = normalizeTargetPath(file.targetPath);
        if (rel.isEmpty())
            continue;
        out << '/' << rel << '\n';
    }
    if (meta.desktop.generate)
        out << "/usr/share/applications/" << DesktopBuilder::desktopFileName(meta) << '\n';

    out << '\n' << "%clean\n";
    out << "rm -rf %{buildroot}\n";

    out << '\n' << "%changelog\n";
    out << "* " << QDate::currentDate().toString(QStringLiteral("ddd MMM d yyyy")) << ' '
        << meta.maintainer << '\n';
    out << "- Versión " << meta.version << '\n';

    out.flush();
    return file.error() == QFileDevice::NoError;
}
