#include <QApplication>
#include <QIcon>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Stellar Texto");
    app.setOrganizationName("StellarText");
    app.setApplicationDisplayName("Stellar Texto");

    app.setWindowIcon(QIcon(":/icons/stellartexto.png"));

    app.setStyleSheet(R"(
        QMainWindow, QDialog {
            background: #1E1E1E;
        }
        QMenuBar {
            background: #2D2D2D;
            border-bottom: 1px solid #3C3C3C;
            padding: 2px 0;
            font-size: 13px;
            color: #CCCCCC;
        }
        QMenuBar::item {
            padding: 4px 10px;
            background: transparent;
            border-radius: 4px;
        }
        QMenuBar::item:selected {
            background: #3C3C3C;
        }
        QMenu {
            background: #2D2D2D;
            border: 1px solid #3C3C3C;
            border-radius: 6px;
            padding: 4px;
            color: #CCCCCC;
        }
        QMenu::item {
            padding: 6px 24px 6px 12px;
            border-radius: 4px;
        }
        QMenu::item:selected {
            background: #4A86E8;
            color: #FFFFFF;
        }
        QMenu::separator {
            height: 1px;
            background: #3C3C3C;
            margin: 4px 8px;
        }
        QToolBar {
            background: #2D2D2D;
            border-bottom: 1px solid #3C3C3C;
            spacing: 4px;
            padding: 2px 4px;
        }
        QToolButton {
            background: transparent;
            border: none;
            border-radius: 4px;
            padding: 4px 8px;
            font-size: 12px;
            color: #CCCCCC;
        }
        QToolButton:hover {
            background: #3C3C3C;
        }
        QToolButton:checked {
            background: #4A86E8;
            color: #FFFFFF;
        }
        QTabWidget::pane {
            border: none;
            background: #1E1E1E;
            margin-top: -1px;
        }
        QTabBar {
            background: #2D2D2D;
        }
        QTabBar::tab {
            background: #2D2D2D;
            border: none;
            border-bottom: 2px solid transparent;
            padding: 6px 16px 6px 10px;
            margin-right: 2px;
            font-size: 12px;
            color: #999999;
            min-width: 60px;
        }
        QTabBar::tab:selected {
            background: #1E1E1E;
            border-bottom: 2px solid #4A86E8;
            color: #FFFFFF;
            font-weight: bold;
        }
        QTabBar::tab:hover:!selected {
            background: #3C3C3C;
        }
        QTabBar::close-button {
            background: transparent;
            border: none;
            padding: 2px;
            subcontrol-position: right;
        }
        QTabBar::close-button:hover {
            background: #C84C4C;
            border-radius: 2px;
        }
        QStatusBar {
            background: #2D2D2D;
            border-top: 1px solid #3C3C3C;
            font-size: 12px;
            color: #999999;
        }
        QPushButton {
            background: #3C3C3C;
            border: 1px solid #555555;
            border-radius: 6px;
            padding: 6px 16px;
            font-size: 13px;
            color: #CCCCCC;
        }
        QPushButton:hover {
            background: #4A4A4A;
            border-color: #666666;
        }
        QPushButton:pressed {
            background: #4A86E8;
            color: #FFFFFF;
        }
        QComboBox {
            background: #3C3C3C;
            border: 1px solid #555555;
            border-radius: 6px;
            padding: 4px 8px;
            font-size: 12px;
            color: #CCCCCC;
        }
        QComboBox:hover {
            border-color: #4A86E8;
        }
        QComboBox::drop-down {
            border: none;
            width: 20px;
        }
        QComboBox QAbstractItemView {
            background: #2D2D2D;
            color: #CCCCCC;
            selection-background-color: #4A86E8;
        }
        QLineEdit {
            background: #3C3C3C;
            border: 1px solid #555555;
            border-radius: 6px;
            padding: 4px 8px;
            font-size: 13px;
            color: #CCCCCC;
        }
        QLineEdit:focus {
            border-color: #4A86E8;
        }
        QCheckBox {
            font-size: 13px;
            spacing: 6px;
            color: #CCCCCC;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
        }
        QLabel {
            font-size: 13px;
            color: #CCCCCC;
        }
        QScrollBar:vertical {
            width: 10px;
            background: #1E1E1E;
        }
        QScrollBar::handle:vertical {
            background: #555555;
            border-radius: 5px;
            min-height: 20px;
        }
        QScrollBar::handle:vertical:hover {
            background: #777777;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0;
        }
        QScrollBar:horizontal {
            height: 10px;
            background: #1E1E1E;
        }
        QScrollBar::handle:horizontal {
            background: #555555;
            border-radius: 5px;
            min-width: 20px;
        }
        QScrollBar::handle:horizontal:hover {
            background: #777777;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0;
        }
        QFrame[frameShape="4"] {
            color: #3C3C3C;
        }
    )");

    MainWindow window;
    window.show();

    QStringList args = app.arguments();
    for (int i = 1; i < args.size(); ++i)
        window.openFileFromPath(args[i]);

    return app.exec();
}
