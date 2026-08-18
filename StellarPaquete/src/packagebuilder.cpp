#include "packagebuilder.h"

#include <QDir>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>

#include "archbuilder.h"
#include "debbuilder.h"
#include "desktopbuilder.h"
#include "rpmbuilder.h"

PackageBuilder::PackageBuilder(QObject *parent)
    : QObject(parent)
{
}

bool PackageBuilder::toolAvailable(const QString &tool)
{
    return !QStandardPaths::findExecutable(tool).isEmpty();
}

PackageBuilder *PackageBuilder::createBuilder(const QString &format, QObject *parent)
{
    const QString f = format.toLower();
    if (f == QStringLiteral("deb"))
        return new DebBuilder(parent);
    if (f == QStringLiteral("rpm"))
        return new RpmBuilder(parent);
    if (f == QStringLiteral("arch"))
        return new ArchBuilder(parent);
    if (f == QStringLiteral("desktop"))
        return new DesktopBuilder(parent);
    return nullptr;
}

bool PackageBuilder::runProcess(const QString &program, const QStringList &arguments)
{
    QProcess process;
    process.setProcessChannelMode(QProcess::SeparateChannels);

    auto forwardOutput = [this](QProcess *p, bool isError) {
        const QByteArray data = isError ? p->readAllStandardError()
                                        : p->readAllStandardOutput();
        if (!data.isEmpty())
            emit logMessage(QString::fromUtf8(data));
    };

    QObject::connect(&process, &QProcess::readyReadStandardOutput, &process,
                     [&process, &forwardOutput]() { forwardOutput(&process, false); });
    QObject::connect(&process, &QProcess::readyReadStandardError, &process,
                     [&process, &forwardOutput]() { forwardOutput(&process, true); });

    emit logMessage(QStringLiteral("$ %1 %2").arg(program, arguments.join(QLatin1Char(' '))));

    process.start(program, arguments);
    if (!process.waitForStarted()) {
        emit logMessage(QStringLiteral("Error: no se pudo iniciar \"%1\".").arg(program));
        return false;
    }

    QEventLoop loop;
    QObject::connect(&process,
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     &loop, &QEventLoop::quit);
    loop.exec();

    forwardOutput(&process, false);
    forwardOutput(&process, true);

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

bool PackageBuilder::copyFileTo(const QString &source, const QString &dest)
{
    return QFile::copy(source, dest);
}

bool PackageBuilder::createDirectory(const QString &path)
{
    return QDir().mkpath(path);
}

QString PackageBuilder::normalizeTargetPath(const QString &path) const
{
    QString p = path.trimmed();
    while (p.startsWith(QLatin1Char('/')))
        p.remove(0, 1);
    p = QDir::cleanPath(p);
    if (p.isEmpty() || p == QLatin1String(".") || p == QLatin1String("..") || p.startsWith(QLatin1String("../")))
        return QString();
    return p;
}
