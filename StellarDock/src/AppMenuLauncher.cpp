#include "AppMenuLauncher.h"
#include "DockWindow.h"
#include <QPainter>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QApplication>
#include <QScreen>
#include <QProcess>
#include <QDebug>

AppMenuLauncher::AppMenuLauncher(ApplicationScanner* scanner, DockWindow* dock, QWidget* parent)
    : QWidget(parent), m_scanner(scanner), m_dock(dock) {

    m_menu = new AppMenu(scanner, nullptr);

    setMouseTracking(true);
}

int AppMenuLauncher::tamanoActual() const {
    return m_dock->config().tamanoIcono;
}

void AppMenuLauncher::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    int size = tamanoActual();
    int x = (width() - size) / 2;
    int y = (height() - size) / 2;

    // Aplicar escala y magnificación
    QTransform transform;
    transform.translate(width() / 2, height() / 2);
    qreal scale = m_escala;
    if (m_tamMag > size) {
        scale *= qreal(m_tamMag) / qreal(size);
    }
    transform.scale(scale, scale);
    transform.translate(-width() / 2, -height() / 2);
    p.setTransform(transform);

    // Dibujar icono de menú (símbolo de grid/hamburguesa)
    QRect rc(x, y, size, size);

    // Fondo circular
    if (m_hover) {
        p.fillRect(rc, QColor(100, 100, 100, 50));
        p.drawEllipse(rc);
    }

    // Icono de grid/menú (9 puntos)
    int dotSize = 5;
    int spacing = (size - dotSize * 3) / 4;
    QColor dotColor = m_hover ? QColor(200, 200, 200) : QColor(150, 150, 150);
    p.setRenderHint(QPainter::Antialiasing, true);
    p.setPen(Qt::NoPen);
    p.setBrush(dotColor);

    for (int row = 0; row < 3; ++row) {
        for (int col = 0; col < 3; ++col) {
            int px = x + spacing + col * (dotSize + spacing) + dotSize / 2;
            int py = y + spacing + row * (dotSize + spacing) + dotSize / 2;
            p.drawEllipse(QPointF(px, py), dotSize / 2, dotSize / 2);
        }
    }

    // Tooltip text si está sobre el elemento
    if (m_hover) {
        p.save();
        p.resetTransform();
        QFont font = p.font();
        font.setPointSize(9);
        p.setFont(font);
        p.setPen(QColor(200, 200, 200));
        QRect textRect = rect().adjusted(0, size + 5, 0, 20);
        p.drawText(textRect, Qt::AlignCenter, "Aplicaciones");
        p.restore();
    }
}

void AppMenuLauncher::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        mostrarMenu();
    }
}

void AppMenuLauncher::enterEvent(QEnterEvent*) {
    m_hover = true;
    update();
}

void AppMenuLauncher::leaveEvent(QEvent*) {
    m_hover = false;
    update();
}

void AppMenuLauncher::mostrarMenu() {
    // Calcular posición respecto al dock
    QPoint globalPos = mapToGlobal(QPoint(0, 0));
    QRect dockGeometry = m_dock->geometry();
    
    // Mostrar el menú cerca del dock
    m_menu->mostrarEnPosicion(globalPos, dockGeometry);
}
