#include "mainwindow.h"
#include "queryparser.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QDesktopServices>
#include <QUrl>
#include <QDir>
#include <QMenu>
#include <QAction>
#include <QApplication>
#include <QIcon>
#include <QFileInfo>
#include <QStyle>
#include <QTimer>

static const char *DARK_THEME = R"(
QMainWindow, QWidget {
    background-color: #1a1b2e;
    color: #e0e0e0;
}
QLineEdit {
    font-size: 14px; padding: 8px 12px;
    border: 2px solid #3a3b5e;
    border-radius: 6px;
    background: #252640;
    color: #e0e0e0;
}
QLineEdit:focus { border-color: #7c5cfc; }
QComboBox {
    font-size: 13px; padding: 8px;
    background: #252640; color: #e0e0e0;
    border: 1px solid #3a3b5e; border-radius: 4px;
    min-width: 120px;
}
QComboBox::drop-down { border: none; }
QComboBox QAbstractItemView {
    background: #252640; color: #e0e0e0;
    selection-background-color: #7c5cfc;
}
QPushButton {
    font-size: 14px; padding: 8px 20px;
    background: #7c5cfc; color: white;
    border: none; border-radius: 6px; font-weight: bold;
}
QPushButton:hover { background: #6a4de0; }
QPushButton:disabled { background: #3a3b5e; color: #666; }
QTreeWidget {
    background: #1e1f38;
    alternate-background-color: #222342;
    color: #e0e0e0;
    border: 1px solid #3a3b5e;
    border-radius: 4px; font-size: 13px;
}
QTreeWidget::item { padding: 4px; }
QTreeWidget::item:selected { background: #7c5cfc; color: white; }
QHeaderView::section {
    background: #252640; color: #a0a0c0; padding: 6px;
    border: none; border-bottom: 1px solid #3a3b5e; font-weight: bold;
}
QProgressBar {
    border: 1px solid #3a3b5e; border-radius: 4px;
    text-align: center; background: #1e1f38;
    color: #e0e0e0; height: 18px;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #7c5cfc, stop:1 #5c3cfc);
    border-radius: 3px;
}
QSplitter::handle { background: #3a3b5e; width: 2px; }
QLabel { color: #e0e0e0; background: transparent; }
QStatusBar { background: #12132a; color: #888; border-top: 1px solid #3a3b5e; }
QScrollBar:vertical { background: #1e1f38; width: 10px; border: none; }
QScrollBar::handle:vertical { background: #3a3b5e; border-radius: 5px; min-height: 20px; }
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical { height: 0; }
QScrollBar:horizontal { background: #1e1f38; height: 10px; border: none; }
QScrollBar::handle:horizontal { background: #3a3b5e; border-radius: 5px; min-width: 20px; }
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal { width: 0; }
QMenu { background: #252640; color: #e0e0e0; border: 1px solid #3a3b5e; }
QMenu::item:selected { background: #7c5cfc; }
QMessageBox { background: #1a1b2e; color: #e0e0e0; }
QMessageBox QLabel { color: #e0e0e0; }
QMessageBox QPushButton { min-width: 80px; }
)";

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), searcher(nullptr), searchThread(nullptr),
      isSearching(false), isMaximized(false), dragging(false) {
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground, false);
    qApp->setStyleSheet(DARK_THEME);
    setupUI();
    setupConnections();
    resize(1200, 750);
    loadSettings();
    setWindowTitle("Stellar Busqueda");
    setWindowIcon(QIcon(":/app-icon"));
}

MainWindow::~MainWindow() {
    cleanupSearch();
}

void MainWindow::setupUI() {
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *mainLayout = new QVBoxLayout(central);
    mainLayout->setSpacing(0);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // --- Custom title bar ---
    titleBar = new QWidget(this);
    titleBar->setFixedHeight(36);
    titleBar->setStyleSheet("background: #12132a; border-bottom: 1px solid #3a3b5e;");
    titleBar->setCursor(Qt::OpenHandCursor);

    QHBoxLayout *titleLayout = new QHBoxLayout(titleBar);
    titleLayout->setContentsMargins(10, 0, 6, 0);
    titleLayout->setSpacing(6);

    QLabel *iconLabel = new QLabel(titleBar);
    iconLabel->setPixmap(QPixmap(":/app-icon").scaled(20, 20, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    iconLabel->setFixedSize(20, 20);

    titleLabel = new QLabel("Stellar Busqueda", titleBar);
    titleLabel->setStyleSheet("font-size: 13px; font-weight: bold; color: #c0c0e0;");

    auto makeTitleBtn = [this](const QString &text, const QString &color, const QString &hover) -> QPushButton* {
        QPushButton *btn = new QPushButton(text, titleBar);
        btn->setFixedSize(28, 24);
        btn->setCursor(Qt::ArrowCursor);
        btn->setStyleSheet(QString(
            "QPushButton { font-size: 12px; background: transparent; color: %1; "
            "border: none; border-radius: 3px; font-weight: bold; }"
            "QPushButton:hover { background: %2; }").arg(color, hover));
        return btn;
    };

    minButton = makeTitleBtn("─", "#a0a0c0", "#3a3b5e");
    maxButton = makeTitleBtn("□", "#a0a0c0", "#3a3b5e");
    closeButton = makeTitleBtn("✕", "#e0a0a0", "#c0392b");

    titleLayout->addWidget(iconLabel);
    titleLayout->addWidget(titleLabel, 1);
    titleLayout->addWidget(minButton);
    titleLayout->addWidget(maxButton);
    titleLayout->addWidget(closeButton);

    mainLayout->addWidget(titleBar);

    // --- Content ---
    QWidget *content = new QWidget(this);
    QVBoxLayout *contentLayout = new QVBoxLayout(content);
    contentLayout->setSpacing(8);
    contentLayout->setContentsMargins(12, 12, 12, 12);

    QHBoxLayout *searchLayout = new QHBoxLayout();
    searchLayout->setSpacing(6);

    searchInput = new QLineEdit(content);
    searchInput->setPlaceholderText(
        "Describe lo que buscas... (ej: 'una foto de un barco', 'documento que dice gus', 'videos del 2023')");
    searchInput->setMinimumHeight(40);

    typeFilter = new QComboBox(content);
    typeFilter->addItem("Todos", "all");
    typeFilter->addItem("Imágenes", "image");
    typeFilter->addItem("Documentos", "document");
    typeFilter->addItem("Videos", "video");
    typeFilter->addItem("Audio", "audio");

    searchButton = new QPushButton("🔍 Buscar", content);
    searchButton->setMinimumHeight(40);

    stopButton = new QPushButton("⏹ Detener", content);
    stopButton->setStyleSheet(
        "QPushButton { font-size: 14px; padding: 8px 20px; background: #e74c3c; "
        "color: white; border: none; border-radius: 6px; font-weight: bold; }"
        "QPushButton:hover { background: #c0392b; }");
    stopButton->setMinimumHeight(40);
    stopButton->hide();

    searchLayout->addWidget(searchInput, 1);
    searchLayout->addWidget(typeFilter);
    searchLayout->addWidget(searchButton);
    searchLayout->addWidget(stopButton);

    splitter = new QSplitter(Qt::Horizontal, content);

    resultTree = new QTreeWidget(content);
    resultTree->setColumnCount(5);
    resultTree->setHeaderLabels({"Nombre", "Ruta", "Tamaño", "Modificado", "Tipo"});
    resultTree->setRootIsDecorated(false);
    resultTree->setAlternatingRowColors(true);
    resultTree->setSelectionMode(QAbstractItemView::SingleSelection);
    resultTree->setSortingEnabled(true);
    resultTree->sortByColumn(3, Qt::DescendingOrder);
    resultTree->header()->setStretchLastSection(false);
    resultTree->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    resultTree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    resultTree->header()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    resultTree->header()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    resultTree->header()->setSectionResizeMode(4, QHeaderView::ResizeToContents);
    resultTree->setColumnWidth(0, 220);

    previewWidget = new PreviewWidget(content);

    splitter->addWidget(resultTree);
    splitter->addWidget(previewWidget);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    progressBar = new QProgressBar(content);
    progressBar->setRange(0, 0);
    progressBar->hide();

    statusLabel = new QLabel("Listo para buscar", content);
    statusBar()->addWidget(statusLabel, 1);
    statusBar()->addPermanentWidget(progressBar, 0);

    contentLayout->addLayout(searchLayout);

    QHBoxLayout *bottomLayout = new QHBoxLayout();
    bottomLayout->setSpacing(6);

    aboutButton = new QPushButton("ⓘ", content);
    aboutButton->setFixedSize(28, 28);
    aboutButton->setToolTip("Acerca de Stellar Busqueda");
    aboutButton->setStyleSheet(
        "QPushButton { font-size: 14px; background: transparent; color: #7c5cfc; "
        "border: 1px solid #3a3b5e; border-radius: 14px; }"
        "QPushButton:hover { background: #252640; color: #a08cfc; }");

    bottomLayout->addStretch();
    bottomLayout->addWidget(aboutButton);
    contentLayout->addLayout(bottomLayout);

    contentLayout->addWidget(splitter, 1);

    mainLayout->addWidget(content, 1);

    resultTree->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MainWindow::setupConnections() {
    connect(searchButton, &QPushButton::clicked, this, &MainWindow::onSearch);
    connect(stopButton, &QPushButton::clicked, this, &MainWindow::onStop);
    connect(searchInput, &QLineEdit::returnPressed, this, &MainWindow::onSearch);
    connect(resultTree, &QTreeWidget::itemClicked, this, &MainWindow::onItemClicked);
    connect(aboutButton, &QPushButton::clicked, this, &MainWindow::onAbout);
    connect(minButton, &QPushButton::clicked, this, &MainWindow::onMinimize);
    connect(maxButton, &QPushButton::clicked, this, &MainWindow::onMaximize);
    connect(closeButton, &QPushButton::clicked, this, &MainWindow::onClose);

    connect(resultTree, &QTreeWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
        QTreeWidgetItem *item = resultTree->itemAt(pos);
        if (!item) return;

        QMenu menu(this);
        QAction *openAct = menu.addAction("Abrir archivo");
        QAction *folderAct = menu.addAction("Abrir carpeta contenedora");
        connect(openAct, &QAction::triggered, this, &MainWindow::onOpenFile);
        connect(folderAct, &QAction::triggered, this, &MainWindow::onOpenFolder);
        menu.exec(resultTree->viewport()->mapToGlobal(pos));
    });
}

// --- Window control ---
void MainWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        QPoint pos = event->pos();
        QWidget *child = childAt(pos);
        if (child && (child == titleBar || child == titleLabel ||
                      child->parentWidget() == titleBar)) {
            dragStart = event->globalPos() - frameGeometry().topLeft();
            dragging = true;
            titleBar->setCursor(Qt::ClosedHandCursor);
            event->accept();
            return;
        }
    }
    QMainWindow::mousePressEvent(event);
}

void MainWindow::mouseMoveEvent(QMouseEvent *event) {
    if (dragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPos() - dragStart);
        event->accept();
        return;
    }
    QMainWindow::mouseMoveEvent(event);
}

void MainWindow::mouseReleaseEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton && dragging) {
        dragging = false;
        titleBar->setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    QMainWindow::mouseReleaseEvent(event);
}

void MainWindow::onMinimize() {
    showMinimized();
}

void MainWindow::onMaximize() {
    if (isMaximized) {
        showNormal();
        isMaximized = false;
        maxButton->setText("□");
    } else {
        showMaximized();
        isMaximized = true;
        maxButton->setText("❐");
    }
}

void MainWindow::onClose() {
    close();
}

// --- Search ---
void MainWindow::onSearch() {
    if (isSearching) return;

    QString query = searchInput->text().trimmed();
    if (query.isEmpty()) {
        QMessageBox::information(this, "Stellar Busqueda",
                                 "Por favor describe lo que quieres buscar.\n"
                                 "Ejemplos:\n"
                                 "• 'una foto de un barco'\n"
                                 "• 'documento que dice gus'\n"
                                 "• 'videos de vacaciones'\n"
                                 "• 'archivo pdf con informe'");
        return;
    }

    ParsedQuery parsed = QueryParser::parse(query);

    QString manualType = typeFilter->currentData().toString();
    if (manualType != "all") {
        parsed.fileCategory = manualType;
    }

    resultTree->clear();
    results.clear();
    previewWidget->clear();

    statusLabel->setText("🔍 Analizando consulta y buscando archivos...");
    searchButton->hide();
    stopButton->show();
    progressBar->show();
    isSearching = true;

    cleanupSearch();

    searcher = new FileSearcher();
    searcher->setSearchDirs(getSearchDirectories());

    if (parsed.fileCategory != "all") {
        searcher->setExtensions(parsed.fileExtensions);
    }

    searcher->setKeywords(parsed.keywords);
    searcher->setExactPhrases(parsed.exactPhrases);
    searcher->setSearchContent(parsed.searchContent);
    searcher->setMaxResults(500);

    searchThread = new QThread(this);
    searcher->moveToThread(searchThread);

    connect(searcher, &FileSearcher::resultFound, this, &MainWindow::onResultFound, Qt::QueuedConnection);
    connect(searcher, &FileSearcher::searchProgress, this, &MainWindow::onSearchProgress, Qt::QueuedConnection);
    connect(searcher, &FileSearcher::searchFinished, this, &MainWindow::onSearchFinished, Qt::QueuedConnection);
    connect(searcher, &FileSearcher::searchError, this, &MainWindow::onSearchError, Qt::QueuedConnection);

    connect(searchThread, &QThread::started, searcher, &FileSearcher::startSearch);
    connect(searcher, &FileSearcher::searchFinished, searchThread, &QThread::quit);

    searchThread->start();
}

void MainWindow::onStop() {
    if (searcher) {
        searcher->stopSearch();
    }
    statusLabel->setText("⏹ Búsqueda detenida");
    searchButton->show();
    stopButton->hide();
    progressBar->hide();
    isSearching = false;
}

void MainWindow::cleanupSearch() {
    if (searchThread) {
        searchThread->quit();
        searchThread->wait(3000);
        delete searchThread;
        searchThread = nullptr;
    }
    if (searcher) {
        delete searcher;
        searcher = nullptr;
    }
}

void MainWindow::onResultFound(const SearchResult &result) {
    QTreeWidgetItem *item = new QTreeWidgetItem();
    item->setText(0, result.fileName);
    item->setText(1, result.dirPath);
    item->setText(2, result.sizeString());
    item->setText(3, result.lastModified.toString("dd/MM/yyyy hh:mm"));

    QString suffix = QFileInfo(result.filePath).suffix().toLower();
    QString typeStr;
    if (QStringList{"jpg","jpeg","png","gif","bmp","webp"}.contains(suffix))
        typeStr = "📷 Imagen";
    else if (QStringList{"pdf","doc","docx","xls","xlsx","ppt","pptx","txt","md"}.contains(suffix))
        typeStr = "📄 Documento";
    else if (QStringList{"mp4","avi","mkv","mov","wmv","flv","webm"}.contains(suffix))
        typeStr = "🎬 Video";
    else if (QStringList{"mp3","wav","flac","aac","ogg","m4a"}.contains(suffix))
        typeStr = "🎵 Audio";
    else
        typeStr = "📎 " + suffix.toUpper();
    item->setText(4, typeStr);

    item->setData(0, Qt::UserRole, results.size());
    item->setToolTip(0, result.filePath);

    resultTree->addTopLevelItem(item);
    results.append(result);
}

void MainWindow::onSearchProgress(int files, int dirs) {
    statusLabel->setText(QString("🔍 Escaneados: %1 archivos, %2 carpetas...")
                         .arg(files).arg(dirs));
}

void MainWindow::onSearchFinished(int total) {
    searchButton->show();
    stopButton->hide();
    progressBar->hide();
    isSearching = false;

    if (total == 0) {
        statusLabel->setText("😕 No se encontraron archivos. Intenta con otras palabras.");
    } else {
        statusLabel->setText(QString("✅ Búsqueda completada: %1 resultados encontrados").arg(total));
        for (int i = 0; i < resultTree->columnCount(); ++i)
            resultTree->resizeColumnToContents(i);
    }
}

void MainWindow::onSearchError(const QString &error) {
    statusLabel->setText("❌ Error: " + error);
    searchButton->show();
    stopButton->hide();
    progressBar->hide();
    isSearching = false;
}

void MainWindow::onItemClicked(QTreeWidgetItem *item, int) {
    int idx = item->data(0, Qt::UserRole).toInt();
    if (idx >= 0 && idx < results.size()) {
        previewWidget->showPreview(results[idx]);
    }
}

void MainWindow::onOpenFile() {
    QTreeWidgetItem *item = resultTree->currentItem();
    if (!item) return;
    int idx = item->data(0, Qt::UserRole).toInt();
    if (idx >= 0 && idx < results.size()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(results[idx].filePath));
    }
}

void MainWindow::onOpenFolder() {
    QTreeWidgetItem *item = resultTree->currentItem();
    if (!item) return;
    int idx = item->data(0, Qt::UserRole).toInt();
    if (idx >= 0 && idx < results.size()) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(results[idx].dirPath));
    }
}

void MainWindow::onAbout() {
    QMessageBox about(this);
    about.setWindowTitle("Acerca de Stellar Busqueda");
    about.setIconPixmap(QPixmap(":/app-icon").scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    about.setText(
        "<h2>Stellar Busqueda</h2>"
        "<p><b>Versión:</b> 1.0.0</p>"
        "<p><b>Descripción:</b></p>"
        "<p>Buscador inteligente de archivos que permite encontrar "
        "cualquier archivo en tu sistema mediante descripciones "
        "en lenguaje natural.</p>"
        "<p><b>Características:</b></p>"
        "<ul>"
        "<li>Búsqueda por palabras clave con parser inteligente</li>"
        "<li>Filtrado por tipo de archivo (imágenes, documentos, videos, audio)</li>"
        "<li>Búsqueda dentro del contenido de documentos</li>"
        "<li>Vista previa de imágenes, texto y video</li>"
        "<li>Tema oscuro fresco con ventana personalizada</li>"
        "</ul>"
        "<p><b>Tecnologías:</b> Qt5, C++17</p>"
    );
    about.setStandardButtons(QMessageBox::Ok);
    about.exec();
}

QStringList MainWindow::getSearchDirectories() {
    QStringList dirs;
    QString home = QDir::homePath();
    dirs << home;

    QDir media("/media");
    if (media.exists()) dirs << "/media";

    QDir mnt("/mnt");
    if (mnt.exists()) dirs << "/mnt";

    QDir opt("/opt");
    if (opt.exists()) dirs << "/opt";

    static const QStringList excludePrefixes = {
        "/proc", "/sys", "/dev", "/run", "/lost+found",
        "/boot", "/etc", "/usr", "/var", "/root", "/snap"
    };

    QStringList result;
    for (const QString &d : dirs) {
        bool excluded = false;
        for (const QString &pref : excludePrefixes) {
            if (d.startsWith(pref)) { excluded = true; break; }
        }
        if (!excluded) result << d;
    }

    return result;
}

void MainWindow::loadSettings() {
    QSettings settings("StellarBusqueda", "StellarBusqueda");
    QByteArray geo = settings.value("geometry").toByteArray();
    if (!geo.isEmpty()) {
        restoreGeometry(geo);
    }
    isMaximized = settings.value("maximized", false).toBool();
    if (isMaximized) {
        QTimer::singleShot(50, this, [this]() {
            showMaximized();
            maxButton->setText("❐");
        });
    }
    int typeIdx = settings.value("typeFilter", 0).toInt();
    typeFilter->setCurrentIndex(typeIdx);
}

void MainWindow::saveSettings() {
    QSettings settings("StellarBusqueda", "StellarBusqueda");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("maximized", isMaximized);
    settings.setValue("typeFilter", typeFilter->currentIndex());
}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveSettings();
    QMainWindow::closeEvent(event);
}
