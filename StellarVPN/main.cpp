#include <QApplication>
#include <QIcon>
#include <QFile>
#include <QDir>

#include "vpn_client.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("StellarVPN");
    app.setApplicationVersion("1.0");

    // Global window icon (mirrors Python: app.setWindowIcon(...))
    const QString iconPath = QApplication::applicationDirPath() + "/icons/icons.png";
    if (QFile::exists(iconPath))
        app.setWindowIcon(QIcon(iconPath));

    VPNClient window;
    window.show();

    return app.exec();
}
