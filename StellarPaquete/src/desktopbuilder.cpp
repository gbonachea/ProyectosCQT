#include "desktopbuilder.h"

#include <QDir>
#include <QFile>
#include <QFileDevice>
#include <QFileInfo>
#include <QRegularExpression>
#include <QTextStream>

DesktopBuilder::DesktopBuilder(QObject *parent)
    : PackageBuilder(parent)
{
}

bool DesktopBuilder::build(const PackageMetadata &meta, const QString &outputPath)
{
    const QString name = meta.desktop.name.trimmed().isEmpty()
                             ? meta.name.trimmed()
                             : meta.desktop.name.trimmed();
    const QString comment = meta.desktop.comment.trimmed().isEmpty()
                                ? meta.shortDescription.trimmed()
                                : meta.desktop.comment.trimmed();
    const QString exec = meta.desktop.exec.trimmed().isEmpty()
                             ? meta.name.trimmed()
                             : meta.desktop.exec.trimmed();
    const QString icon = meta.desktop.icon.trimmed().isEmpty()
                             ? meta.name.trimmed()
                             : meta.desktop.icon.trimmed();
    const QString categories = meta.desktop.categories.trimmed().isEmpty()
                                   ? QStringLiteral("Utility;")
                                   : meta.desktop.categories.trimmed();

    QString fileName = name;
    fileName.replace(QRegularExpression(QStringLiteral("[^a-zA-Z0-9.+-]")), QStringLiteral("-"));
    if (!fileName.endsWith(QStringLiteral(".desktop")))
        fileName += QStringLiteral(".desktop");

    const QString out = QDir(outputPath).filePath(fileName);
    if (QFileInfo::exists(out))
        QFile::remove(out);

    QFile file(out);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit logMessage(QStringLiteral("Error: no se pudo escribir \"%1\".").arg(out));
        return false;
    }

    QTextStream stream(&file);
    stream << "[Desktop Entry]\n";
    stream << "Type=Application\n";
    stream << "Name=" << name << '\n';
    if (!comment.isEmpty())
        stream << "Comment=" << comment << '\n';
    stream << "Exec=" << exec << '\n';
    stream << "Icon=" << icon << '\n';
    stream << "Terminal=" << (meta.desktop.terminal ? QStringLiteral("true")
                                                    : QStringLiteral("false")) << '\n';
    stream << "StartupNotify=" << (meta.desktop.startupNotify ? QStringLiteral("true")
                                                              : QStringLiteral("false")) << '\n';
    if (!meta.desktop.mimeTypes.trimmed().isEmpty())
        stream << "MimeType=" << meta.desktop.mimeTypes.trimmed() << '\n';
    stream << "Categories=" << categories << '\n';
    stream.flush();

    if (file.error() != QFileDevice::NoError) {
        emit logMessage(QStringLiteral("Error: no se pudo escribir el archivo .desktop."));
        return false;
    }

    emit logMessage(QStringLiteral("Archivo .desktop creado: %1").arg(out));
    return true;
}
