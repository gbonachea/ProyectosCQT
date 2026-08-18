#include "archbuilder.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QTextStream>
#include <QTemporaryDir>

ArchBuilder::ArchBuilder(QObject *parent)
    : PackageBuilder(parent)
{
}

QString ArchBuilder::archName(const QString &arch) const
{
    const QString a = arch.trimmed().toLower();
    if (a == QStringLiteral("amd64") || a == QStringLiteral("x86_64"))
        return QStringLiteral("x86_64");
    if (a == QStringLiteral("arm64") || a == QStringLiteral("aarch64"))
        return QStringLiteral("aarch64");
    if (a == QStringLiteral("all") || a == QStringLiteral("noarch"))
        return QStringLiteral("any");
    if (a.isEmpty())
        return QStringLiteral("x86_64");
    return a;
}

bool ArchBuilder::build(const PackageMetadata &meta, const QString &outputPath)
{
    QTemporaryDir tempDir;
    tempDir.setAutoRemove(true);
    if (!tempDir.isValid()) {
        emit logMessage(QStringLiteral("Error: no se pudo crear un directorio temporal."));
        return false;
    }

    const QString workDir = tempDir.path();

    const QString sourceDir = workDir + QStringLiteral("/source");
    if (!createDirectory(sourceDir)) {
        emit logMessage(QStringLiteral("Error: no se pudo crear el directorio fuente."));
        return false;
    }

    for (const FileMapping &file : meta.files) {
        const QString rel = normalizeTargetPath(file.targetPath);
        if (rel.isEmpty()) {
            emit logMessage(QStringLiteral("Advertencia: ruta de destino inválida ignorada para \"%1\".")
                                .arg(file.sourcePath));
            continue;
        }

        const QString dest = sourceDir + QLatin1Char('/') + rel;
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

    const QString tarName = QStringLiteral("%1-%2-source.tar.gz").arg(meta.name, meta.version);
    const QString tarPath = workDir + QLatin1Char('/') + tarName;
    if (!runProcess(QStringLiteral("tar"),
                    { QStringLiteral("-czf"), tarPath, QStringLiteral("-C"), sourceDir,
                      QStringLiteral(".") })) {
        emit logMessage(QStringLiteral("Error: no se pudo crear el archivo fuente."));
        return false;
    }

    QStringList fileEntries;
    QDir sourceDirObj(sourceDir);
    sourceDirObj.setFilter(QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot);
    for (const QString &entry :
         sourceDirObj.entryList({ QStringLiteral("*") }, QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot)) {
        QFileInfo fi(sourceDir + QLatin1Char('/') + entry);
        if (fi.isFile()) {
            fileEntries.append(entry);
        }
    }

    const QString pkgbuildPath = workDir + QStringLiteral("/PKGBUILD");
    if (!writePKGBUILD(pkgbuildPath, meta, fileEntries)) {
        emit logMessage(QStringLiteral("Error: no se pudo escribir el PKGBUILD."));
        return false;
    }

    const QString arch = archName(meta.architecture);
    const QString pkgFileName = QStringLiteral("%1-%2-1-%3.pkg.tar.zst")
                                    .arg(meta.name, meta.version, arch);
    const QString pkgOut = QDir(outputPath).filePath(pkgFileName);

    if (!runProcess(QStringLiteral("makepkg"),
                    { QStringLiteral("--noconfirm"), QStringLiteral("--noprogressbar"),
                      QStringLiteral("--nocheck"), QStringLiteral("--skippgpcheck") })) {
        emit logMessage(QStringLiteral("Error: makepkg falló al construir el paquete Arch Linux."));
        return false;
    }

    const QString pkgFile = workDir + QLatin1Char('/') + pkgFileName;
    if (!QFileInfo::exists(pkgFile)) {
        const QStringList candidates = QDir(workDir).entryList({ QStringLiteral("*.pkg.tar.zst") }, QDir::Files);
        if (candidates.isEmpty()) {
            emit logMessage(QStringLiteral("Error: no se encontró el paquete generado (.pkg.tar.zst)."));
            return false;
        }
        if (QFileInfo::exists(pkgOut))
            QFile::remove(pkgOut);
        if (!copyFileTo(workDir + QLatin1Char('/') + candidates.first(), pkgOut)) {
            emit logMessage(QStringLiteral("Error: no se pudo copiar el paquete a %1.").arg(pkgOut));
            return false;
        }
    } else {
        if (QFileInfo::exists(pkgOut))
            QFile::remove(pkgOut);
        if (!copyFileTo(pkgFile, pkgOut)) {
            emit logMessage(QStringLiteral("Error: no se pudo copiar el paquete a %1.").arg(pkgOut));
            return false;
        }
    }

    emit logMessage(QStringLiteral("Paquete Arch Linux creado: %1").arg(pkgOut));
    return true;
}

bool ArchBuilder::writePKGBUILD(const QString &path, const PackageMetadata &meta,
                                const QStringList &fileEntries) const
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    const QString arch = archName(meta.architecture);
    const QStringList depList = meta.dependencies;

    QTextStream out(&file);
    out << "pkgname='" << meta.name << "'\n";
    out << "pkgver='" << meta.version << "'\n";
    out << "pkgrel='1'\n";
    QString shortDesc = meta.shortDescription;
    shortDesc.replace(QStringLiteral("'"), QStringLiteral("'\\''"));
    out << "pkgdesc='" << shortDesc << "'\n";

    if (arch == QStringLiteral("any"))
        out << "arch=('any')\n";
    else
        out << "arch=('" << arch << "')\n";

    if (!meta.license.isEmpty())
        out << "license=('" << meta.license << "')\n";

    out << "options=(!strip !debug !docs !libtool !staticlibs !emptydirs)\n";
    out << "install=" << meta.name << ".install\n";

    out << "source=('" << meta.name << "-" << meta.version << "-source.tar.gz')\n";
    out << "sha256sums=('SKIP')\n";

    out << '\n';
    out << "package() {\n";
    out << "  cd \"$srcdir\"\n";
    out << "  mkdir -p \"$pkgdir\"\n";

    QSet<QString> dirs;
    for (const QString &entry : fileEntries) {
        const QString dir = QFileInfo(entry).absolutePath();
        if (dir != QStringLiteral(".") && !dir.isEmpty())
            dirs.insert(dir);
    }
    for (const QString &dir : std::as_const(dirs))
        out << "  mkdir -p \"$pkgdir/" << dir << "\"\n";

    for (const QString &entry : fileEntries)
        out << "  cp -a \"$srcdir/" << entry << "\" \"$pkgdir/" << entry << "\"\n";

    out << "}\n";

    out.flush();
    return file.error() == QFileDevice::NoError;
}
