#include "playlistwindow.h"
#include "settings.h"
#include <QFileDialog>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QCloseEvent>
#include <QIcon>

PlaylistWindow::PlaylistWindow(QWidget *parent) : QWidget(parent) {
    setWindowTitle("Lista de reproducción");

    QString iconPath = getIconPath();
    if (!iconPath.isEmpty())
        setWindowIcon(QIcon(iconPath));

    m_btnAdd    = new QPushButton();
    m_btnUp     = new QPushButton();
    m_btnDown   = new QPushButton();
    m_btnRemove = new QPushButton();
    m_playlist  = new QListWidget();

    setupUi();
    loadSavedGeometry();
}

void PlaylistWindow::setupUi() {
    const int btnSize  = 24;
    const int iconSize = 16;

    m_btnAdd->setIcon(QIcon::fromTheme("list-add"));
    m_btnAdd->setToolTip("Agregar audio");
    connect(m_btnAdd, &QPushButton::clicked, this, &PlaylistWindow::addAudio);

    m_btnUp->setIcon(QIcon::fromTheme("go-up"));
    m_btnUp->setToolTip("Mover arriba");
    connect(m_btnUp, &QPushButton::clicked, this, &PlaylistWindow::moveUp);

    m_btnDown->setIcon(QIcon::fromTheme("go-down"));
    m_btnDown->setToolTip("Mover abajo");
    connect(m_btnDown, &QPushButton::clicked, this, &PlaylistWindow::moveDown);

    m_btnRemove->setIcon(QIcon::fromTheme("list-remove"));
    m_btnRemove->setToolTip("Eliminar audio");
    connect(m_btnRemove, &QPushButton::clicked, this, &PlaylistWindow::removeAudio);

    for (QPushButton *btn : {m_btnAdd, m_btnUp, m_btnDown, m_btnRemove}) {
        btn->setFixedSize(btnSize, btnSize);
        btn->setIconSize(QSize(iconSize, iconSize));
    }

    m_playlist->setDragDropMode(QListWidget::InternalMove);
    m_playlist->setDefaultDropAction(Qt::MoveAction);
    connect(m_playlist, &QListWidget::itemDoubleClicked, this, &PlaylistWindow::playItem);

    auto *btnLayout = new QHBoxLayout();
    for (QPushButton *btn : {m_btnAdd, m_btnUp, m_btnDown, m_btnRemove})
        btnLayout->addWidget(btn);
    btnLayout->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(btnLayout);
    layout->addWidget(m_playlist);

    setAcceptDrops(true);
}

void PlaylistWindow::loadSavedGeometry() {
    auto ws = loadWindowState("playlist", 200, 200, 300, 400);
    setGeometry(ws.x, ws.y, ws.width, ws.height);
    if (ws.state)
        setWindowState(Qt::WindowStates(ws.state));
}

void PlaylistWindow::closeEvent(QCloseEvent *event) {
    saveWindowState("playlist", geometry(), windowState());
    event->accept();
}

void PlaylistWindow::dragEnterEvent(QDragEnterEvent *event) {
    // Only accept drops from outside (source == nullptr)
    if (!event->source() && event->mimeData()->hasUrls())
        event->acceptProposedAction();
    else
        event->ignore();
}

void PlaylistWindow::dropEvent(QDropEvent *event) {
    for (const QUrl &url : event->mimeData()->urls()) {
        QString path = url.toLocalFile();
        if (path.endsWith(".mp3", Qt::CaseInsensitive) ||
            path.endsWith(".wav", Qt::CaseInsensitive)) {
            emit addFileSignal(path);
        }
    }
}

void PlaylistWindow::playItem(QListWidgetItem *item) {
    int index = m_playlist->row(item);
    emit playSignal(QString::number(index));
}

void PlaylistWindow::addAudio() {
    QString filename = QFileDialog::getOpenFileName(
        this, "Selecciona un archivo de audio", {},
        "Audio Files (*.mp3 *.wav)");
    if (!filename.isEmpty())
        emit addFileSignal(filename);
}

void PlaylistWindow::moveUp() {
    int current = m_playlist->currentRow();
    if (current > 0) {
        QListWidgetItem *item = m_playlist->takeItem(current);
        m_playlist->insertItem(current - 1, item);
        m_playlist->setCurrentRow(current - 1);
    }
}

void PlaylistWindow::moveDown() {
    int current = m_playlist->currentRow();
    if (current < m_playlist->count() - 1) {
        QListWidgetItem *item = m_playlist->takeItem(current);
        m_playlist->insertItem(current + 1, item);
        m_playlist->setCurrentRow(current + 1);
    }
}

void PlaylistWindow::removeAudio() {
    int current = m_playlist->currentRow();
    if (current >= 0)
        delete m_playlist->takeItem(current);
}
