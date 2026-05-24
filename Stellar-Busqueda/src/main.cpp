#include <QApplication>
#include <QIcon>
#include <QDir>
#include "searchresult.h"
#include "mainwindow.h"

int main(int argc, char *argv[]) {
    qRegisterMetaType<SearchResult>("SearchResult");

    QApplication app(argc, argv);

    QString appDir = app.applicationDirPath();
    app.addLibraryPath(appDir + "/plugins");
    qputenv("QT_PLUGIN_PATH", (appDir + "/plugins").toUtf8());

    app.setApplicationName("Stellar Busqueda");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("StellarBusqueda");
    app.setWindowIcon(QIcon(":/app-icon"));

    MainWindow window;
    window.show();

    return app.exec();
}
