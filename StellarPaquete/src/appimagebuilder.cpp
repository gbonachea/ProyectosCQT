#include "appimagebuilder.h"

#include <QColor>
#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QFont>
#include <QImage>
#include <QPainter>
#include <QRectF>
#include <QTextStream>
#include <QTemporaryDir>

AppImageBuilder::AppImageBuilder(QObject *parent)
    : PackageBuilder(parent)
{
}

QString AppImageBuilder::findExecutablePath(const PackageMetadata &meta) const
{
    for (const FileMapping &file : meta.files) {
        const QString rel = normalizeTargetPath(file.targetPath);
        if (rel.startsWith(QStringLiteral("usr/bin/")) || rel.startsWith(QStringLiteral("bin/")))
            return rel;
    }
    return QStringLiteral("usr/bin/%1").arg(meta.name);
}

QString AppImageBuilder::findIconPath(const PackageMetadata &meta) const
{
    static const QStringList suffixes = { QStringLiteral("png"), QStringLiteral("svg"),
                                          QStringLiteral("svgz"), QStringLiteral("ico") };
    for (const FileMapping &file : meta.files) {
        if (suffixes.contains(QFileInfo(file.sourcePath).suffix().toLower()))
            return normalizeTargetPath(file.targetPath);
    }
    return QString();
}

bool AppImageBuilder::build(const PackageMetadata &meta, const QString &outputPath)
{
    QTemporaryDir tempDir;
    tempDir.setAutoRemove(true);
    if (!tempDir.isValid()) {
        emit logMessage(QStringLiteral("Error: no se pudo crear un directorio temporal."));
        return false;
    }

    const QString appDir = tempDir.path() + QStringLiteral("/AppDir");
    if (!createDirectory(appDir + QStringLiteral("/usr/share/applications"))) {
        emit logMessage(QStringLiteral("Error: no se pudo crear la estructura AppDir."));
        return false;
    }

    if (!meta.dependencies.isEmpty()) {
        emit logMessage(QStringLiteral("Nota: las dependencias declaradas no aplican a los AppImage, "
                                       "que se empaquetan de forma autocontenida (solo se usan en .deb y .rpm)."));
    }

    for (const FileMapping &file : meta.files) {
        const QString rel = normalizeTargetPath(file.targetPath);
        if (rel.isEmpty()) {
            emit logMessage(QStringLiteral("Advertencia: ruta de destino inválida ignorada para \"%1\".")
                                .arg(file.sourcePath));
            continue;
        }

        const QString dest = appDir + QLatin1Char('/') + rel;
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

    const QString execRel = findExecutablePath(meta);

    QString iconName;
    const QString iconRel = findIconPath(meta);
    if (!iconRel.isEmpty()) {
        iconName = QFileInfo(iconRel).completeBaseName();
    } else {
        iconName = meta.name;
        const QString fallbackIcon = appDir + QLatin1Char('/') + meta.name + QStringLiteral(".png");
        if (!createDefaultIcon(fallbackIcon, meta.name))
            emit logMessage(QStringLiteral("Advertencia: no se pudo generar un icono por defecto."));
    }

    const QString appRunPath = appDir + QStringLiteral("/AppRun");
    if (!writeAppRun(appRunPath, meta, execRel)) {
        emit logMessage(QStringLiteral("Error: no se pudo escribir AppRun."));
        return false;
    }
    QFile::setPermissions(appRunPath,
                          QFile::permissions(appRunPath)
                              | QFileDevice::ExeOwner | QFileDevice::ExeGroup | QFileDevice::ExeOther);

    const QString desktopPath = appDir + QLatin1Char('/') + meta.name + QStringLiteral(".desktop");
    if (!writeDesktopFile(desktopPath, meta, iconName)) {
        emit logMessage(QStringLiteral("Error: no se pudo escribir el archivo .desktop."));
        return false;
    }

    const QString appImageName = QStringLiteral("%1-%2-%3.AppImage")
                                     .arg(meta.name, meta.version, meta.architecture);
    const QString appImageOut = QDir(outputPath).filePath(appImageName);
    if (QFileInfo::exists(appImageOut))
        QFile::remove(appImageOut);

    if (toolAvailable(QStringLiteral("appimagetool"))) {
        if (!runProcess(QStringLiteral("appimagetool"), { appDir, appImageOut })) {
            emit logMessage(QStringLiteral("appimagetool falló sin --appimage-extract-and-run; reintentando..."));
            if (!runProcess(QStringLiteral("appimagetool"),
                            { QStringLiteral("--appimage-extract-and-run"), appDir, appImageOut })) {
                emit logMessage(QStringLiteral("Error: appimagetool falló al construir el AppImage."));
                return false;
            }
        }
    } else if (toolAvailable(QStringLiteral("linuxdeploy"))) {
        if (!runProcess(QStringLiteral("linuxdeploy"),
                        { QStringLiteral("--appdir"), appDir, QStringLiteral("--output"),
                          QStringLiteral("appimage") })) {
            emit logMessage(QStringLiteral("Error: linuxdeploy falló al construir el AppImage."));
            return false;
        }

        const QStringList produced = QDir(tempDir.path()).entryList({ QStringLiteral("*.AppImage") },
                                                                    QDir::Files);
        if (produced.isEmpty()) {
            emit logMessage(QStringLiteral("Error: linuxdeploy no generó ningún .AppImage."));
            return false;
        }
        if (QFileInfo::exists(appImageOut))
            QFile::remove(appImageOut);
        if (!copyFileTo(QDir(tempDir.path()).filePath(produced.first()), appImageOut)) {
            emit logMessage(QStringLiteral("Error: no se pudo copiar el AppImage a %1.").arg(appImageOut));
            return false;
        }
    } else {
        emit logMessage(QStringLiteral("Error: ni appimagetool ni linuxdeploy están disponibles."));
        return false;
    }

    emit logMessage(QStringLiteral("AppImage creado: %1").arg(appImageOut));
    return true;
}

bool AppImageBuilder::writeAppRun(const QString &appRunPath, const PackageMetadata &meta,
                                  const QString &execRel) const
{
    QFile file(appRunPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << "#!/bin/sh\n";
    out << "SELF=$(readlink -f \"$0\")\n";
    out << "HERE=${SELF%/*}\n";
    out << "export PATH=\"${HERE}/usr/bin:${HERE}/bin:${PATH}\"\n";
    out << "export LD_LIBRARY_PATH=\"${HERE}/usr/lib:${HERE}/usr/lib/x86_64-linux-gnu:${HERE}/lib:${HERE}/lib64:${LD_LIBRARY_PATH}\"\n";
    out << "export QT_XKB_CONFIG_ROOT=${QT_XKB_CONFIG_ROOT:-/usr/share/X11/xkb}\n";
    out << "exec \"${HERE}/" << execRel << "\" \"$@\"\n";
    out.flush();
    return file.error() == QFileDevice::NoError;
}

bool AppImageBuilder::writeDesktopFile(const QString &desktopPath, const PackageMetadata &meta,
                                       const QString &iconName) const
{
    QFile file(desktopPath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;

    QTextStream out(&file);
    out << "[Desktop Entry]\n";
    out << "Type=Application\n";
    out << "Name=" << meta.name << '\n';
    out << "Comment=" << meta.shortDescription << '\n';
    out << "Exec=" << meta.name << '\n';
    if (!iconName.isEmpty())
        out << "Icon=" << iconName << '\n';
    out << "Terminal=false\n";
    out << "Categories=Utility;\n";
    out.flush();
    return file.error() == QFileDevice::NoError;
}

bool AppImageBuilder::createDefaultIcon(const QString &iconPath, const QString &label) const
{
    QImage image(128, 128, QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    QPainter painter(&image);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setBrush(QColor(QStringLiteral("#3d6fb4")));
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(QRectF(0, 0, 128, 128), 20, 20);

    QFont font = painter.font();
    font.setPixelSize(72);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(Qt::white);
    painter.drawText(QRectF(0, 0, 128, 128), Qt::AlignCenter, label.left(1).toUpper());
    painter.end();

    return image.save(iconPath, "PNG");
}
