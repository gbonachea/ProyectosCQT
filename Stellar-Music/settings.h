#pragma once
#include <QString>
#include <QRect>
#include <Qt>

struct WindowState {
    int x, y, width, height;
    int state; // Qt::WindowState
};

WindowState loadWindowState(const QString &windowName, int defX, int defY, int defW, int defH);
void        saveWindowState(const QString &windowName, const QRect &geometry, Qt::WindowStates state);
QString     loadStylesheet();
QString     getIconPath();
