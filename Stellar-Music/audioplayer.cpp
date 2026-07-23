#include "audioplayer.h"
#include "playlistwindow.h"
#include "configwindow.h"
#include "settings.h"

#include <QApplication>
#include <QFileDialog>
#include <QMessageBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QCloseEvent>
#include <QEvent>
#include <QWidgetAction>
#include <QFileInfo>
#include <QSizePolicy>
#include <QIcon>
#include <QDebug>

AudioPlayer::AudioPlayer(QWidget *parent)
    : QWidget(parent)
    , m_isPaused(false)
    , m_audioLength(0)
{
    setWindowTitle("Hero Music Player");

    // Media backend
    m_player      = new QMediaPlayer(this);
    m_audioOutput = new QAudioOutput(this);
    m_player->setAudioOutput(m_audioOutput);
    m_audioOutput->setVolume(1.0f);

    connect(m_player, &QMediaPlayer::mediaStatusChanged,
            this, &AudioPlayer::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::playbackStateChanged,
            this, &AudioPlayer::onPlaybackStateChanged);

    // Icon
    QString iconPath = getIconPath();
    QIcon appIcon = iconPath.isEmpty()
                    ? QIcon::fromTheme("audio-x-generic")
                    : QIcon(iconPath);
    setWindowIcon(appIcon);

    // Tray icon
    m_trayIcon = new QSystemTrayIcon(appIcon, this);
    m_trayMenu = new QMenu(this);
    auto *showAction = m_trayMenu->addAction("Mostrar");
    auto *quitAction = m_trayMenu->addAction("Salir");
    connect(showAction, &QAction::triggered, this, &QWidget::show);
    connect(quitAction, &QAction::triggered, this, &AudioPlayer::quitApplication);
    m_trayIcon->setContextMenu(m_trayMenu);
    connect(m_trayIcon, &QSystemTrayIcon::activated,
            this, &AudioPlayer::activateFromTray);
    m_trayIcon->show();

    // Label
    m_label = new QLabel("No hay archivo cargado");
    m_label->setTextFormat(Qt::PlainText);
    m_label->setWordWrap(false);
    m_label->setAlignment(Qt::AlignCenter);
    m_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_label->setMinimumWidth(200);
    m_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    // Buttons
    const int btnSize  = 24;
    const int icoSize  = 16;

    auto makeBtn = [&](const QString &theme, const QString &tip = {}) {
        auto *b = new QPushButton();
        b->setIcon(QIcon::fromTheme(theme));
        if (!tip.isEmpty()) b->setToolTip(tip);
        b->setFixedSize(btnSize, btnSize);
        b->setIconSize(QSize(icoSize, icoSize));
        return b;
    };

    m_btnLoad     = makeBtn("folder-open",        "Cargar archivo");
    m_btnPlay     = makeBtn("media-playback-start");
    m_btnPause    = makeBtn("media-playback-pause");
    m_btnStop     = makeBtn("media-playback-stop");
    m_btnPlaylist = makeBtn("view-list");
    m_volumeButton= makeBtn("audio-volume-high",  "Volumen");
    m_btnConfig   = makeBtn("preferences-system", "Configuración");

    m_btnPlay->setEnabled(false);
    m_btnPause->setEnabled(false);
    m_btnStop->setEnabled(false);

    connect(m_btnLoad,     &QPushButton::clicked, this, &AudioPlayer::loadFile);
    connect(m_btnPlay,     &QPushButton::clicked, this, &AudioPlayer::playAudio);
    connect(m_btnPause,    &QPushButton::clicked, this, &AudioPlayer::pauseAudio);
    connect(m_btnStop,     &QPushButton::clicked, this, &AudioPlayer::stopAudio);
    connect(m_btnPlaylist, &QPushButton::clicked, this, &AudioPlayer::showPlaylist);
    connect(m_btnConfig,   &QPushButton::clicked, this, &AudioPlayer::showConfig);

    // Volume slider inside popup menu
    m_volumeMenu   = new QMenu(this);
    auto *volWidget= new QWidget();
    auto *volLayout= new QVBoxLayout(volWidget);
    volLayout->setContentsMargins(4, 4, 4, 4);

    m_volumeSlider = new QSlider(Qt::Vertical);
    m_volumeSlider->setRange(0, 100);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setFixedSize(20, 100);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &AudioPlayer::changeVolume);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &AudioPlayer::updateVolumeIcon);

    volLayout->addWidget(m_volumeSlider, 0, Qt::AlignCenter);
    auto *wa = new QWidgetAction(this);
    wa->setDefaultWidget(volWidget);
    m_volumeMenu->addAction(wa);
    m_volumeMenu->setFixedWidth(28);

    connect(m_volumeButton, &QPushButton::clicked, this, &AudioPlayer::showVolumeMenu);

    // Seekbar
    m_seekbar = new QSlider(Qt::Horizontal);
    m_seekbar->setRange(0, 100);
    m_seekbar->setValue(0);
    m_seekbar->setEnabled(false);
    connect(m_seekbar, &QSlider::sliderReleased, this, &AudioPlayer::seekAudio);

    // Timer
    m_timer = new QTimer(this);
    m_timer->setInterval(500);
    connect(m_timer, &QTimer::timeout, this, &AudioPlayer::updateSeekbar);

    // Sub-windows
    m_playlistWindow = new PlaylistWindow();
    m_configWindow   = new ConfigWindow(this);

    connect(m_playlistWindow, &PlaylistWindow::playSignal,
            this, &AudioPlayer::playFromPlaylist);
    connect(m_playlistWindow, &PlaylistWindow::addFileSignal,
            this, &AudioPlayer::addFileToPlaylist);

    // Layouts
    auto *leftLayout = new QHBoxLayout();
    leftLayout->addWidget(m_btnLoad);
    leftLayout->addStretch();

    auto *centerLayout = new QHBoxLayout();
    centerLayout->addWidget(m_btnPlay);
    centerLayout->addWidget(m_btnPause);
    centerLayout->addWidget(m_btnStop);
    centerLayout->addWidget(m_btnPlaylist);

    auto *rightLayout = new QHBoxLayout();
    rightLayout->addStretch();
    rightLayout->addWidget(m_volumeButton);
    rightLayout->addWidget(m_btnConfig);

    auto *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addLayout(leftLayout);
    buttonsLayout->addLayout(centerLayout);
    buttonsLayout->addLayout(rightLayout);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_label);
    layout->addWidget(m_seekbar);
    layout->addLayout(buttonsLayout);

    setMinimumSize(400, 150);
    setAcceptDrops(true);
    loadSavedGeometry();
}

AudioPlayer::~AudioPlayer() {
    delete m_playlistWindow;
}

void AudioPlayer::loadSavedGeometry() {
    auto ws = loadWindowState("main_player", 100, 100, 600, 200);
    setGeometry(ws.x, ws.y, ws.width, ws.height);
    if (ws.state)
        setWindowState(Qt::WindowStates(ws.state));
}

void AudioPlayer::closeEvent(QCloseEvent *event) {
    saveWindowState("main_player", geometry(), windowState());
    m_playlistWindow->close();
    event->accept();
}

void AudioPlayer::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange) {
        if (windowState() & Qt::WindowMinimized) {
            bool toTray = m_configWindow
                          ? m_configWindow->property("minimize_to_tray").toBool()
                          : false;
            // read from QSettings directly
            QSettings s("Player", "AudioPlayer");
            if (s.value("minimize_to_tray", false).toBool()) {
                hide();
                event->ignore();
                return;
            }
        }
    }
    QWidget::changeEvent(event);
}

void AudioPlayer::activateFromTray(QSystemTrayIcon::ActivationReason reason) {
    if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
        show();
        setWindowState(Qt::WindowActive);
        activateWindow();
        raise();
    }
}

// ── Drag & Drop ────────────────────────────────────────────────────────────────
void AudioPlayer::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasUrls()) {
        for (const QUrl &u : event->mimeData()->urls()) {
            QString p = u.toLocalFile().toLower();
            if (p.endsWith(".mp3") || p.endsWith(".wav")) {
                event->acceptProposedAction();
                return;
            }
        }
    }
    event->ignore();
}

void AudioPlayer::dropEvent(QDropEvent *event) {
    for (const QUrl &url : event->mimeData()->urls()) {
        QString path = url.toLocalFile();
        if (path.endsWith(".mp3", Qt::CaseInsensitive) ||
            path.endsWith(".wav", Qt::CaseInsensitive)) {
            addFileToPlaylist(path);
        }
    }
}

// ── File / Playlist management ─────────────────────────────────────────────────
void AudioPlayer::loadFile() {
    QString filename = QFileDialog::getOpenFileName(
        this, "Selecciona un archivo de audio", {},
        "Audio Files (*.mp3 *.wav)");
    if (filename.isEmpty()) return;

    addFileToPlaylist(filename);
    m_currentFile = filename;

    QString name = QFileInfo(filename).fileName();
    if (name.length() > 50) name = name.left(47) + "...";
    m_label->setText(name);
    m_label->setToolTip(filename);

    setPlayerReady(true);
    stopAudio();

    m_player->setSource(QUrl::fromLocalFile(filename));
    m_audioLength = 0; // will be updated on mediaLoaded
}

void AudioPlayer::addFileToPlaylist(const QString &filename) {
    if (!QFileInfo::exists(filename)) return;

    // Check if already in visual list
    for (int i = 0; i < m_playlistWindow->playlist()->count(); ++i) {
        if (m_playlistWindow->playlist()->item(i)->toolTip() == filename)
            return;
    }

    if (!m_playlist.contains(filename))
        m_playlist.append(filename);

    auto *item = new QListWidgetItem(QFileInfo(filename).fileName());
    item->setToolTip(filename);
    m_playlistWindow->playlist()->addItem(item);

    if (m_playlist.size() == 1 || m_currentFile.isEmpty()) {
        m_currentFile = filename;
        QString name  = QFileInfo(filename).fileName();
        if (name.length() > 50) name = name.left(47) + "...";
        m_label->setText(name);
        m_label->setToolTip(filename);
        setPlayerReady(true);

        m_player->setSource(QUrl::fromLocalFile(filename));
    }
}

void AudioPlayer::playFromPlaylist(const QString &indexStr) {
    bool ok;
    int index = indexStr.toInt(&ok);
    if (!ok) return;

    if (index < 0 || index >= m_playlistWindow->playlist()->count()) return;

    QString filename = m_playlistWindow->playlist()->item(index)->toolTip();
    if (!QFileInfo::exists(filename)) return;

    m_currentFile = filename;
    m_label->setText(QFileInfo(filename).fileName());

    m_player->setSource(QUrl::fromLocalFile(filename));
    m_player->play();
    m_isPaused = false;

    m_btnPlay->setEnabled(false);
    m_btnPause->setEnabled(true);
    m_btnStop->setEnabled(true);
    m_seekbar->setEnabled(true);
    m_timer->start();
}

void AudioPlayer::syncPlaylistWithWidget() {
    m_playlist.clear();
    for (int i = 0; i < m_playlistWindow->playlist()->count(); ++i)
        m_playlist.append(m_playlistWindow->playlist()->item(i)->toolTip());
}

// ── Playback controls ──────────────────────────────────────────────────────────
void AudioPlayer::playAudio() {
    if (m_isPaused) {
        m_player->play();
        m_isPaused = false;
    } else {
        if (m_currentFile.isEmpty() && m_playlistWindow->playlist()->count() > 0) {
            m_currentFile = m_playlistWindow->playlist()->item(0)->toolTip();
            m_label->setText(QFileInfo(m_currentFile).fileName());
            m_player->setSource(QUrl::fromLocalFile(m_currentFile));
        }
        if (!m_currentFile.isEmpty()) {
            m_player->play();
            updateAudioLength();
        }
    }
    m_btnPlay->setEnabled(false);
    m_btnPause->setEnabled(true);
    m_btnStop->setEnabled(true);
    m_seekbar->setEnabled(true);
    m_timer->start();
}

void AudioPlayer::pauseAudio() {
    if (!m_currentFile.isEmpty() && m_player->playbackState() == QMediaPlayer::PlayingState) {
        m_player->pause();
        m_isPaused = true;
        m_timer->stop();
        m_btnPlay->setEnabled(true);
        m_btnPause->setEnabled(false);
    }
}

void AudioPlayer::stopAudio() {
    m_player->stop();
    m_label->setText("No hay archivo cargado");
    m_isPaused = false;
    m_seekbar->setValue(0);
    m_timer->stop();

    bool hasFiles = m_playlistWindow->playlist()->count() > 0;
    m_btnPlay->setEnabled(hasFiles);
    m_btnPause->setEnabled(false);
    m_btnStop->setEnabled(false);
    m_seekbar->setEnabled(false);
    m_currentFile.clear();
}

void AudioPlayer::seekAudio() {
    if (!m_currentFile.isEmpty() && m_audioLength > 0) {
        qint64 pos = static_cast<qint64>(m_seekbar->value()) * 1000;
        m_player->setPosition(pos);
        m_isPaused = false;
        m_timer->start();
    }
}

void AudioPlayer::updateSeekbar() {
    if (m_player->playbackState() == QMediaPlayer::PlayingState && !m_seekbar->isSliderDown()) {
        qint64 pos = m_player->position() / 1000;
        m_seekbar->setValue(static_cast<int>(pos));
    } else if (m_player->playbackState() != QMediaPlayer::PlayingState && !m_seekbar->isSliderDown()) {
        m_seekbar->setValue(0);
        m_timer->stop();
    }
}

void AudioPlayer::updateAudioLength() {
    qint64 dur = m_player->duration();
    if (dur > 0) {
        m_audioLength = dur / 1000;
        m_seekbar->setMaximum(static_cast<int>(m_audioLength));
    }
}

// ── Volume ─────────────────────────────────────────────────────────────────────
void AudioPlayer::changeVolume(int value) {
    m_audioOutput->setVolume(value / 100.0f);
}

void AudioPlayer::updateVolumeIcon(int value) {
    if (value == 0)
        m_volumeButton->setIcon(QIcon::fromTheme("audio-volume-muted"));
    else if (value < 33)
        m_volumeButton->setIcon(QIcon::fromTheme("audio-volume-low"));
    else if (value < 66)
        m_volumeButton->setIcon(QIcon::fromTheme("audio-volume-medium"));
    else
        m_volumeButton->setIcon(QIcon::fromTheme("audio-volume-high"));
}

void AudioPlayer::showVolumeMenu() {
    QPoint pos = m_volumeButton->mapToGlobal(m_volumeButton->rect().bottomLeft());
    m_volumeMenu->exec(pos);
}

// ── Windows ────────────────────────────────────────────────────────────────────
void AudioPlayer::showPlaylist() { m_playlistWindow->show(); }
void AudioPlayer::showConfig()   { m_configWindow->exec(); }

void AudioPlayer::quitApplication() { QApplication::quit(); }

// ── Player state helpers ───────────────────────────────────────────────────────
void AudioPlayer::setPlayerReady(bool ready) {
    m_btnPlay->setEnabled(ready);
    m_btnPause->setEnabled(ready);
    m_btnStop->setEnabled(ready);
    m_seekbar->setEnabled(ready);
}

void AudioPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status) {
    if (status == QMediaPlayer::LoadedMedia) {
        updateAudioLength();
    }
}

void AudioPlayer::onPlaybackStateChanged(QMediaPlayer::PlaybackState state) {
    if (state == QMediaPlayer::StoppedState) {
        m_timer->stop();
    }
}

// ── Drag & drop ───────────────────────────────────────────────────────────────
// (already defined above)
