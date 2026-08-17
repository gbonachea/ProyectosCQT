#include <QApplication>
#include <QColor>
#include <QIcon>
#include <QPalette>
#include <QStyleFactory>

#include "mainwindow.h"

static void applyDarkTheme(QApplication &app)
{
    app.setStyle(QStyleFactory::create(QStringLiteral("Fusion")));

    const QColor window(53, 53, 53);
    const QColor base(25, 25, 25);
    const QColor accent(42, 130, 218);

    QPalette palette;
    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, Qt::white);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, window);
    palette.setColor(QPalette::ToolTipBase, Qt::white);
    palette.setColor(QPalette::ToolTipText, Qt::white);
    palette.setColor(QPalette::Text, Qt::white);
    palette.setColor(QPalette::Button, window);
    palette.setColor(QPalette::ButtonText, Qt::white);
    palette.setColor(QPalette::BrightText, Qt::red);
    palette.setColor(QPalette::Link, accent);
    palette.setColor(QPalette::Highlight, accent);
    palette.setColor(QPalette::HighlightedText, Qt::black);
    palette.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
    palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
    palette.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
    app.setPalette(palette);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("StellarPaquete"));
    QApplication::setApplicationDisplayName(QStringLiteral("Stellar Paquete"));
    QApplication::setApplicationVersion(QStringLiteral("1.0.0"));
    QApplication::setOrganizationName(QStringLiteral("local"));

    applyDarkTheme(app);

    QIcon icon;
    icon.addFile(QStringLiteral(":/icons/stellarpaquete-16.png"), QSize(16, 16));
    icon.addFile(QStringLiteral(":/icons/stellarpaquete-22.png"), QSize(22, 22));
    icon.addFile(QStringLiteral(":/icons/stellarpaquete-24.png"), QSize(24, 24));
    icon.addFile(QStringLiteral(":/icons/stellarpaquete-32.png"), QSize(32, 32));
    icon.addFile(QStringLiteral(":/icons/stellarpaquete-48.png"), QSize(48, 48));
    icon.addFile(QStringLiteral(":/icons/stellarpaquete-64.png"), QSize(64, 64));
    icon.addFile(QStringLiteral(":/icons/stellarpaquete-128.png"), QSize(128, 128));
    icon.addFile(QStringLiteral(":/icons/stellarpaquete-256.png"), QSize(256, 256));
    app.setWindowIcon(icon);

    MainWindow window;
    window.show();

    return app.exec();
}
