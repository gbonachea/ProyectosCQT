#include "DockItem.h"
#include "DockWindow.h"
#include <QPainter>
#include <QPainterPath>
#include <QMouseEvent>
#include <QProcess>
#include <QFontMetrics>
#include <cmath>

// ─── Constructor ────────────────────────────────────────────────────────────

DockItem::DockItem(const InfoApp& app, DockWindow* dock, QWidget* parent)
    : QWidget(parent), m_app(app), m_tipo(Tipo::Aplicacion), m_dock(dock)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    m_animEscala = new QPropertyAnimation(this, "escala", this);
    m_animEscala->setDuration(140);
    m_animEscala->setEasingCurve(QEasingCurve::OutCubic);

    m_animRebote = new QPropertyAnimation(this, "rebote", this);
    m_animRebote->setDuration(180);
    m_animRebote->setEasingCurve(QEasingCurve::OutQuad);

    m_timerEtiqueta = new QTimer(this);
    m_timerEtiqueta->setSingleShot(true);
    m_timerEtiqueta->setInterval(600);
    connect(m_timerEtiqueta, &QTimer::timeout, [this](){ m_mostrarEtiqueta = true; update(); });

    m_timerRebote = new QTimer(this);
    connect(m_timerRebote, &QTimer::timeout, this, &DockItem::animarRebote);
}

DockItem::DockItem(Tipo tipo, DockWindow* dock, QWidget* parent)
    : QWidget(parent), m_tipo(tipo), m_dock(dock)
{
    setAttribute(Qt::WA_TranslucentBackground);
    m_animEscala = new QPropertyAnimation(this, "escala", this);
    m_animEscala->setDuration(140);
    m_animRebote = new QPropertyAnimation(this, "rebote", this);
    m_timerEtiqueta = new QTimer(this);
    m_timerRebote   = new QTimer(this);
}

// ─── Size ───────────────────────────────────────────────────────────────────

int DockItem::tamanoActual() const {
    return m_tamMag > 0 ? m_tamMag : (m_dock ? m_dock->config().tamanoIcono : 48);
}

QSize DockItem::tamano() const {
    if (m_tipo == Tipo::Separador) {
        bool h = m_dock && (m_dock->config().posicion == DockPosition::Abajo ||
                             m_dock->config().posicion == DockPosition::Arriba);
        int base = m_dock ? m_dock->config().tamanoIcono : 48;
        return h ? QSize(14, base + 20) : QSize(base + 20, 14);
    }
    int sz = tamanoActual() + 20;
    return QSize(sz, sz);
}

// ─── Windows ────────────────────────────────────────────────────────────────

void DockItem::agregarVentana(quint64 id) {
    if (!m_app.ventanas.contains(id)) m_app.ventanas.append(id);
    m_app.estaEjecutando = true;
    update();
}

void DockItem::quitarVentana(quint64 id) {
    m_app.ventanas.removeAll(id);
    m_app.estaEjecutando = !m_app.ventanas.isEmpty();
    update();
}

// ─── Launch ─────────────────────────────────────────────────────────────────

void DockItem::lanzar() {
    if (m_app.comandoExec.isEmpty()) return;
    QProcess::startDetached("/bin/sh", {"-c", m_app.comandoExec});
    if (m_dock && m_dock->config().rebotarAlAbrir) {
        m_contRebote = 0;
        m_timerRebote->start(90);
    }
}

void DockItem::animarRebote() {
    if (m_contRebote >= 6) { m_timerRebote->stop(); setRebote(0); return; }
    qreal amp = (m_contRebote % 2 == 0) ? -10.0 : 0.0;

    // Invert for top dock
    if (m_dock && m_dock->config().posicion == DockPosition::Arriba) amp = -amp;

    m_animRebote->stop();
    m_animRebote->setStartValue(m_rebote);
    m_animRebote->setEndValue(amp);
    m_animRebote->start();
    m_contRebote++;
}

// ─── Paint ──────────────────────────────────────────────────────────────────

void DockItem::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    // Separador
    if (m_tipo == Tipo::Separador) {
        bool h = m_dock && (m_dock->config().posicion == DockPosition::Abajo ||
                             m_dock->config().posicion == DockPosition::Arriba);
        p.setPen(QPen(QColor(255,255,255,60), 1.5));
        if (h) p.drawLine(width()/2, height()*0.2, width()/2, height()*0.8);
        else   p.drawLine(width()*0.2, height()/2, width()*0.8, height()/2);
        return;
    }

    bool esHorizontal = m_dock && (m_dock->config().posicion == DockPosition::Abajo ||
                                    m_dock->config().posicion == DockPosition::Arriba);

    int sz     = (int)(tamanoActual() * m_escala);
    int cx     = width()  / 2;
    int cy     = height() / 2;

    // Aplica rebote
    if (esHorizontal) cy += (int)m_rebote;
    else              cx += (int)m_rebote;

    QRect rc(cx - sz/2, cy - sz/2, sz, sz);

    // Glow si hover o activa
    if (m_hover || m_activa) {
        QPainterPath glow;
        glow.addRoundedRect(rc.adjusted(-3,-3,3,3), sz*0.25, sz*0.25);
        p.fillPath(glow, QColor(255,255,255, m_activa ? 45 : 25));
    }

    // Sombra suave
    if (sz > 20) {
        p.save();
        p.setOpacity(0.25);
        QPixmap shadow = m_app.icono.pixmap(sz, sz);
        p.drawPixmap(rc.translated(0, 3), shadow);
        p.setOpacity(1.0);
        p.restore();
    }

    // Ícono con esquinas redondeadas
    if (!m_app.icono.isNull()) {
        QPixmap px = m_app.icono.pixmap(sz, sz);
        if (!px.isNull()) {
            QPainterPath clip;
            clip.addRoundedRect(rc, sz * 0.22, sz * 0.22);
            p.save();
            p.setClipPath(clip);
            p.drawPixmap(rc, px);
            p.restore();
        }
    } else {
        // Placeholder gris
        p.setBrush(QColor(80,80,80,200));
        p.setPen(Qt::NoPen);
        p.drawRoundedRect(rc, sz*0.22, sz*0.22);
        p.setPen(QColor(200,200,200));
        p.setFont(QFont("Sans", sz/4));
        p.drawText(rc, Qt::AlignCenter, m_app.nombre.left(1).toUpper());
    }

    // Indicadores de ejecución
    if (m_dock && m_dock->config().mostrarIndicadores && m_app.estaEjecutando)
        dibujarIndicadores(p, rc);

    // Etiqueta
    if (m_mostrarEtiqueta && m_hover && m_dock && m_dock->config().mostrarEtiquetas)
        dibujarEtiqueta(p);
}

void DockItem::dibujarIndicadores(QPainter& p, const QRect& rc) {
    auto pos = m_dock->config().posicion;
    int n = qMin((int)m_app.ventanas.size(), 3);
    if (n == 0) n = 1;

    QColor col = m_activa ? QColor(255,255,255,230) : QColor(160,200,255,180);
    p.setBrush(col);
    p.setPen(Qt::NoPen);

    int dot = 4, gap = 7;
    int total = n * dot + (n-1) * (gap - dot);

    for (int i = 0; i < n; i++) {
        int x, y;
        if (pos == DockPosition::Abajo) {
            x = rc.center().x() - total/2 + i*gap;
            y = rc.bottom() + 5;
        } else if (pos == DockPosition::Arriba) {
            x = rc.center().x() - total/2 + i*gap;
            y = rc.top() - 9;
        } else if (pos == DockPosition::Izquierda) {
            x = rc.left() - 9;
            y = rc.center().y() - total/2 + i*gap;
        } else {
            x = rc.right() + 5;
            y = rc.center().y() - total/2 + i*gap;
        }
        p.drawEllipse(x, y, dot, dot);
    }
}

void DockItem::dibujarEtiqueta(QPainter& p) {
    QString nombre = m_app.nombre;
    if (nombre.length() > 24) nombre = nombre.left(22) + "…";

    QFont f("Sans", 11, QFont::Medium);
    p.setFont(f);
    QFontMetrics fm(f);
    int tw = fm.horizontalAdvance(nombre) + 18;
    int th = fm.height() + 10;

    auto pos = m_dock->config().posicion;
    QRect lr;
    if      (pos == DockPosition::Abajo)     lr = QRect(width()/2-tw/2, -th-8, tw, th);
    else if (pos == DockPosition::Arriba)    lr = QRect(width()/2-tw/2, height()+8, tw, th);
    else if (pos == DockPosition::Izquierda) lr = QRect(width()+8, height()/2-th/2, tw, th);
    else                                     lr = QRect(-tw-8, height()/2-th/2, tw, th);

    p.save();
    QPainterPath bg;
    bg.addRoundedRect(lr, 7, 7);
    p.fillPath(bg, QColor(20,20,20,215));
    p.setPen(QColor(255,255,255,40));
    p.drawPath(bg);
    p.setPen(Qt::white);
    p.drawText(lr, Qt::AlignCenter, nombre);
    p.restore();
}

// ─── Events ─────────────────────────────────────────────────────────────────

void DockItem::mousePressEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton) {
        m_presion = true;
        m_animEscala->stop();
        m_animEscala->setStartValue(m_escala);
        m_animEscala->setEndValue(0.88);
        m_animEscala->start();
    } else if (e->button() == Qt::MiddleButton) {
        emit clicMedio();
    }
    QWidget::mousePressEvent(e);
}

void DockItem::mouseReleaseEvent(QMouseEvent* e) {
    if (e->button() == Qt::LeftButton && m_presion) {
        m_presion = false;
        m_animEscala->stop();
        m_animEscala->setStartValue(m_escala);
        m_animEscala->setEndValue(m_hover ? 1.08 : 1.0);
        m_animEscala->start();
        if (rect().contains(e->pos())) emit clicIzquierdo();
    } else if (e->button() == Qt::RightButton) {
        emit clicDerecho(e->globalPosition().toPoint());
    }
    QWidget::mouseReleaseEvent(e);
}

void DockItem::enterEvent(QEnterEvent* e) {
    m_hover = true;
    m_timerEtiqueta->start();
    m_animEscala->stop();
    m_animEscala->setStartValue(m_escala);
    m_animEscala->setEndValue(1.08);
    m_animEscala->start();
    update();
    QWidget::enterEvent(e);
}

void DockItem::leaveEvent(QEvent* e) {
    m_hover = false;
    m_mostrarEtiqueta = false;
    m_timerEtiqueta->stop();
    m_animEscala->stop();
    m_animEscala->setStartValue(m_escala);
    m_animEscala->setEndValue(1.0);
    m_animEscala->start();
    update();
    QWidget::leaveEvent(e);
}
