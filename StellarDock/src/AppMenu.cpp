#include "AppMenu.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QScrollArea>
#include <QPainter>
#include <QPainterPath>
#include <QKeyEvent>
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <QProcess>
#include <algorithm>

// ─── AppIcon Widget ─────────────────────────────────────────────────────────
// Widget personalizado para mostrar cada aplicación

class AppIcon : public QWidget {
    Q_OBJECT
public:
    AppIcon(const InfoApp& app, QWidget* parent = nullptr) 
        : QWidget(parent), m_app(app), m_hover(false) {
        setFixedSize(110, 130);
        setContentsMargins(0, 0, 0, 0);
        setMouseTracking(true);
    }

    const InfoApp& getApp() const { return m_app; }

signals:
    void clicked();

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);

        QRect rc = rect();

        // Fondo redondeado al hover
        if (m_hover) {
            QPainterPath path;
            path.addRoundedRect(rc, 12, 12);
            p.fillPath(path, QColor(100, 100, 100, 30));
        }

        // Dibujar icono (64x64 centrado)
        QPixmap pixmap = m_app.icono.pixmap(QSize(64, 64));
        if (!pixmap.isNull()) {
            int iconX = (rc.width() - 64) / 2;
            p.drawPixmap(iconX, 12, pixmap);
        }

        // Nombre de la app (centrado en la parte inferior)
        QFont font = p.font();
        font.setPointSize(9);
        p.setFont(font);
        p.setPen(QColor(200, 200, 200));

        QRect textRect(5, 82, rc.width() - 10, 48);
        p.drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap, m_app.nombre);
    }

    void mousePressEvent(QMouseEvent*) override {
        emit clicked();
    }

    void enterEvent(QEnterEvent*) override {
        m_hover = true;
        update();
    }

    void leaveEvent(QEvent*) override {
        m_hover = false;
        update();
    }

private:
    InfoApp m_app;
    bool m_hover;
};

// ─── CategorySection ────────────────────────────────────────────────────────

class CategorySection : public QWidget {
    Q_OBJECT
public:
    CategorySection(const QString& nombre, const QList<InfoApp>& apps, QWidget* parent = nullptr)
        : QWidget(parent), m_categoryName(nombre), m_apps(apps) {

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setContentsMargins(24, 16, 24, 24);
        mainLayout->setSpacing(12);

        // Encabezado de categoría
        auto* headerLabel = new QLabel(nombre);
        QFont headerFont = headerLabel->font();
        headerFont.setPointSize(12);
        headerFont.setBold(true);
        headerLabel->setFont(headerFont);
        headerLabel->setStyleSheet("color: #ffffff; padding: 4px 0px;");
        mainLayout->addWidget(headerLabel);

        // Grid de aplicaciones (5 columnas para mejor visualización)
        auto* gridLayout = new QGridLayout();
        gridLayout->setSpacing(16);
        gridLayout->setContentsMargins(0, 0, 0, 0);

        int col = 0;
        int row = 0;
        
        for (const InfoApp& app : m_apps) {
            auto* appIcon = new AppIcon(app, this);
            
            connect(appIcon, &AppIcon::clicked, [app]() {
                QStringList args = app.comandoExec.split(' ');
                QString programa = args.takeFirst();
                QProcess::startDetached(programa, args);
            });

            gridLayout->addWidget(appIcon, row, col);

            col++;
            if (col >= 5) {
                col = 0;
                row++;
            }
        }

        mainLayout->addLayout(gridLayout, 1);
    }

private:
    QString m_categoryName;
    QList<InfoApp> m_apps;
};

// ─── AppMenu ────────────────────────────────────────────────────────────────

AppMenu::AppMenu(ApplicationScanner* scanner, QWidget* parent)
    : QWidget(parent), m_scanner(scanner) {

    setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setMouseTracking(true);

    // Scroll area con estilo mejorado
    m_scrollArea = new QScrollArea();
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setStyleSheet(R"(
        QScrollArea {
            background: transparent;
            border: none;
        }
        QScrollBar:vertical {
            background-color: transparent;
            width: 10px;
            margin: 0px;
        }
        QScrollBar::handle:vertical {
            background-color: #555555;
            border-radius: 5px;
            margin: 2px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #666666;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
    )");

    // Contenedor principal
    m_contenedor = new QWidget();
    m_contenedor->setStyleSheet("background-color: transparent;");
    m_layoutContenedor = new QVBoxLayout(m_contenedor);
    m_layoutContenedor->setContentsMargins(0, 0, 0, 0);
    m_layoutContenedor->setSpacing(0);

    m_scrollArea->setWidget(m_contenedor);
    m_scrollArea->setWidgetResizable(true);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(m_scrollArea);
    setLayout(layout);

    construirMenu();
}

void AppMenu::construirMenu() {
    categorizarApps();

    // Limpiar layout anterior
    while (QLayoutItem* item = m_layoutContenedor->takeAt(0)) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }

    // Agregar categorías
    QStringList categorysOrdenadas;
    
    // Orden preferido
    QStringList ordenPreferido = {
        "Accesorios", "Accesories",
        "Desarrollo", "Development",
        "Educación", "Education",
        "Gráficos", "Graphics",
        "Multimedia",
        "Oficina", "Office",
        "Internet",
        "Red", "Network",
        "Utilidades", "Utilities",
        "Sistema", "System",
        "Herramientas", "Tools",
    };

    for (const QString& cat : ordenPreferido) {
        if (m_appPorCategoria.contains(cat) && !m_appPorCategoria[cat].isEmpty()) {
            categorysOrdenadas << cat;
        }
    }

    // Agregar categorías restantes
    for (auto it = m_appPorCategoria.begin(); it != m_appPorCategoria.end(); ++it) {
        if (!categorysOrdenadas.contains(it.key()) && !it.value().isEmpty()) {
            categorysOrdenadas << it.key();
        }
    }

    // Crear widgets de categoría
    for (const QString& categoria : categorysOrdenadas) {
        auto* catWidget = new CategorySection(categoria, m_appPorCategoria[categoria], m_contenedor);
        m_layoutContenedor->addWidget(catWidget);

        // Separador visual entre categorías (opcional)
        auto* separator = new QWidget();
        separator->setFixedHeight(1);
        separator->setStyleSheet("background-color: #333333;");
        m_layoutContenedor->addWidget(separator);
    }

    m_layoutContenedor->addStretch();
}

void AppMenu::categorizarApps() {
    m_appPorCategoria.clear();

    const QList<InfoApp>& apps = m_scanner->todasLasApps();

    for (const InfoApp& app : apps) {
        QString categoria = "Otras";
        QString nombre = app.nombre.toLower();
        QString exec = app.comandoExec.toLower();

        if (nombre.contains("firefox") || nombre.contains("chrome") || nombre.contains("navegador") ||
            nombre.contains("browser") || nombre.contains("vivaldi") || nombre.contains("edge") ||
            nombre.contains("chromium")) {
            categoria = "Internet";
        } else if (nombre.contains("blender") || nombre.contains("gimp") || nombre.contains("inkscape") ||
                   nombre.contains("gráfico") || nombre.contains("graphics") || nombre.contains("fotograf") ||
                   nombre.contains("krita") || nombre.contains("darktable")) {
            categoria = "Gráficos";
        } else if (nombre.contains("terminal") || nombre.contains("konsole") || nombre.contains("bash") ||
                   nombre.contains("xterm")) {
            categoria = "Sistema";
        } else if (nombre.contains("nautilus") || nombre.contains("nemo") || nombre.contains("file") ||
                   nombre.contains("gestor") || nombre.contains("archivos") || nombre.contains("thunar") ||
                   nombre.contains("dolphin")) {
            categoria = "Sistema";
        } else if (nombre.contains("calc") || nombre.contains("writer") || nombre.contains("office") ||
                   nombre.contains("libreoffice") || nombre.contains("word") || nombre.contains("excel")) {
            categoria = "Oficina";
        } else if (nombre.contains("développement") || nombre.contains("development") ||
                   nombre.contains("ide") || nombre.contains("vscode") || nombre.contains("visual") ||
                   nombre.contains("atom") || nombre.contains("sublime") || nombre.contains("gedit") ||
                   nombre.contains("pluma") || nombre.contains("code")) {
            categoria = "Desarrollo";
        } else if (nombre.contains("musica") || nombre.contains("música") || nombre.contains("audio") ||
                   nombre.contains("vlc") || nombre.contains("reproductor") || nombre.contains("video") ||
                   nombre.contains("audacity") || nombre.contains("shotcut")) {
            categoria = "Multimedia";
        } else if (nombre.contains("mail") || nombre.contains("correo") || nombre.contains("thunderbird")) {
            categoria = "Internet";
        } else if (nombre.contains("torrent") || nombre.contains("descarga") || nombre.contains("qbittorrent")) {
            categoria = "Internet";
        } else if (nombre.contains("settings") || nombre.contains("preferencias") || nombre.contains("sistema")) {
            categoria = "Sistema";
        }

        if (!m_appPorCategoria.contains(categoria)) {
            m_appPorCategoria[categoria] = QList<InfoApp>();
        }
        m_appPorCategoria[categoria].append(app);
    }

    // Ordenar dentro de cada categoría
    for (auto it = m_appPorCategoria.begin(); it != m_appPorCategoria.end(); ++it) {
        std::sort(it.value().begin(), it.value().end(),
                  [](const InfoApp& a, const InfoApp& b) {
                      return a.nombre.toLower() < b.nombre.toLower();
                  });
    }
}

void AppMenu::mostrarEnPosicion(const QPoint& pos, const QRect& dockGeometry) {
    show();
    activateWindow();
    raise();

    // Tamaño más grande y apropiado
    resize(1000, 700);

    // Posicionar respecto al dock
    QRect screenGeom = QApplication::primaryScreen()->geometry();
    QPoint newPos = pos;

    // Centrar en pantalla
    newPos.setX((screenGeom.width() - width()) / 2);
    newPos.setY((screenGeom.height() - height()) / 2);

    // Evitar que se salga de pantalla
    if (newPos.x() + width() > screenGeom.right()) {
        newPos.setX(screenGeom.right() - width() - 10);
    }
    if (newPos.x() < screenGeom.left()) {
        newPos.setX(screenGeom.left() + 10);
    }
    if (newPos.y() + height() > screenGeom.bottom()) {
        newPos.setY(screenGeom.bottom() - height() - 10);
    }
    if (newPos.y() < screenGeom.top()) {
        newPos.setY(screenGeom.top() + 10);
    }

    move(newPos);
    setFocus();
}

void AppMenu::ocultarMenu() {
    hide();
}

void AppMenu::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Escape) {
        ocultarMenu();
    }
    QWidget::keyPressEvent(event);
}

void AppMenu::focusOutEvent(QFocusEvent* event) {
    ocultarMenu();
    QWidget::focusOutEvent(event);
}

void AppMenu::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    
    // Fondo redondeado con efecto glassmorphism
    QRect rc = rect();
    
    // Fondo principal semi-transparente
    QColor bgColor(30, 30, 35);
    bgColor.setAlpha(240);
    
    QPainterPath path;
    path.addRoundedRect(rc, 24, 24);
    p.fillPath(path, bgColor);
    
    // Borde sutil
    QPen pen(QColor(100, 100, 100, 100));
    pen.setWidth(1);
    p.setPen(pen);
    p.drawPath(path);
}

#include "AppMenu.moc"
