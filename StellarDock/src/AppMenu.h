#pragma once
#include <QWidget>
#include <QMap>
#include <QList>
#include <QScrollArea>
#include <QVBoxLayout>
#include "ApplicationScanner.h"

// Ventana principal del menú de aplicaciones
class AppMenu : public QWidget {
    Q_OBJECT
public:
    explicit AppMenu(ApplicationScanner* scanner, QWidget* parent = nullptr);

    void mostrarEnPosicion(const QPoint& pos, const QRect& dockGeometry);
    void ocultarMenu();

signals:
    void appLaunchRequested(const InfoApp& app);

protected:
    void keyPressEvent(QKeyEvent*) override;
    void focusOutEvent(QFocusEvent*) override;
    void paintEvent(QPaintEvent*) override;

private:
    void construirMenu();
    void categorizarApps();

    ApplicationScanner* m_scanner;
    QMap<QString, QList<InfoApp>> m_appPorCategoria;
    QScrollArea* m_scrollArea;
    QWidget* m_contenedor;
    QVBoxLayout* m_layoutContenedor;
};
