#include "settings.h"
#include <QCoreApplication>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDir>
#include <QStandardPaths>
#include <QDebug>

static QString configFilePath() {
    return QDir(QCoreApplication::applicationDirPath()).filePath("setting.json");
}

WindowState loadWindowState(const QString &windowName, int defX, int defY, int defW, int defH) {
    WindowState ws { defX, defY, defW, defH, 0 };
    QString path = configFilePath();
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return ws;

    QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
    f.close();
    if (!doc.isObject()) return ws;

    QJsonObject root = doc.object();
    if (!root.contains(windowName)) return ws;

    QJsonObject obj = root[windowName].toObject();
    ws.x      = obj.value("x").toInt(defX);
    ws.y      = obj.value("y").toInt(defY);
    ws.width  = obj.value("width").toInt(defW);
    ws.height = obj.value("height").toInt(defH);
    ws.state  = obj.value("state").toInt(0);
    qDebug() << "Config loaded for:" << windowName;
    return ws;
}

void saveWindowState(const QString &windowName, const QRect &geometry, Qt::WindowStates state) {
    QString path = configFilePath();
    QJsonObject root;

    QFile f(path);
    if (f.open(QIODevice::ReadOnly)) {
        QJsonDocument doc = QJsonDocument::fromJson(f.readAll());
        f.close();
        if (doc.isObject()) root = doc.object();
    }

    QJsonObject obj;
    obj["x"]      = geometry.x();
    obj["y"]      = geometry.y();
    obj["width"]  = geometry.width();
    obj["height"] = geometry.height();
    obj["state"]  = static_cast<int>(state);
    root[windowName] = obj;

    if (f.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        f.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
        f.close();
        qDebug() << "Config saved for:" << windowName;
    } else {
        qWarning() << "Could not save config:" << path;
    }
}

QString loadStylesheet() {
    QString path = QDir(QCoreApplication::applicationDirPath()).filePath("dark_theme.css");
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qWarning() << "Could not load stylesheet:" << path;
        return {};
    }
    return QString::fromUtf8(f.readAll());
}

QString getIconPath() {
    QStringList candidates = {
        QDir(QCoreApplication::applicationDirPath()).filePath("icons/icon.png"),
        QDir(QCoreApplication::applicationDirPath()).filePath("../icons/icon.png"),
        "icons/icon.png"
    };
    for (const QString &p : candidates) {
        if (QFile::exists(p)) {
            qDebug() << "Icon found at:" << p;
            return p;
        }
    }
    qDebug() << "Icon not found";
    return {};
}
