#include <QApplication>
#include <QIcon>
#include <QDebug>
#include "DockWindow.h"

int main(int argc, char* argv[]) {
    // Forzar plataforma XCB para soporte de transparencia
    qputenv("QT_QPA_PLATFORM", "xcb");

    QApplication app(argc, argv);
    app.setApplicationName("StellarDock");
    app.setApplicationVersion("2.0.0");
    app.setOrganizationName("StellarDock");
    app.setWindowIcon(QIcon::fromTheme("start-here"));

    DockWindow dock;
    dock.show();

    return app.exec();
}
