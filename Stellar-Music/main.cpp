#include <QApplication>
#include "audioplayer.h"
#include "settings.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("HeroMusic");
    app.setOrganizationName("BRComp");

    // Apply dark theme from CSS file
    QString style = loadStylesheet();
    if (!style.isEmpty())
        app.setStyleSheet(style);

    AudioPlayer player;
    player.show();

    return app.exec();
}
