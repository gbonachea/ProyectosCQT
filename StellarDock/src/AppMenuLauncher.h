#pragma once
#include <QWidget>
#include <QPropertyAnimation>
#include "AppMenu.h"

class DockWindow;

class AppMenuLauncher : public QWidget {
    Q_OBJECT
    Q_PROPERTY(qreal escala READ escala WRITE setEscala)

public:
    explicit AppMenuLauncher(ApplicationScanner* scanner, DockWindow* dock, QWidget* parent = nullptr);

    int tamanoActual() const;
    void setTamanoMagnificado(int sz) { m_tamMag = sz; updateGeometry(); update(); }

    // Animation props
    qreal escala() const { return m_escala; }
    void setEscala(qreal v) { m_escala = v; update(); }

public slots:
    void mostrarMenu();

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void enterEvent(QEnterEvent*) override;
    void leaveEvent(QEvent*) override;

private:
    ApplicationScanner* m_scanner;
    DockWindow* m_dock;
    AppMenu* m_menu;

    qreal m_escala = 1.0;
    bool m_hover = false;
    int m_tamMag = 0;

    QPropertyAnimation* m_animEscala = nullptr;
};
