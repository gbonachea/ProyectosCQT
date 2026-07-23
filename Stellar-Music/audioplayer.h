#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QTimer>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMediaPlayer>
#include <QAudioOutput>
#include <QListWidget>
#include <QUrl>

class PlaylistWindow;
class ConfigWindow;

class AudioPlayer : public QWidget {
    Q_OBJECT
public:
    explicit AudioPlayer(QWidget *parent = nullptr);
    ~AudioPlayer();

protected:
    void closeEvent(QCloseEvent *event) override;
    void changeEvent(QEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void loadFile();
    void playAudio();
    void pauseAudio();
    void stopAudio();
    void seekAudio();
    void updateSeekbar();
    void changeVolume(int value);
    void updateVolumeIcon(int value);
    void showVolumeMenu();
    void showPlaylist();
    void showConfig();
    void quitApplication();
    void activateFromTray(QSystemTrayIcon::ActivationReason reason);
    void addFileToPlaylist(const QString &filename);
    void playFromPlaylist(const QString &indexStr);
    void syncPlaylistWithWidget();
    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void onPlaybackStateChanged(QMediaPlayer::PlaybackState state);

private:
    void setupUi();
    void loadSavedGeometry();
    void updateAudioLength();
    void setPlayerReady(bool ready);

    QLabel         *m_label;
    QPushButton    *m_btnLoad;
    QPushButton    *m_btnPlay;
    QPushButton    *m_btnPause;
    QPushButton    *m_btnStop;
    QPushButton    *m_btnPlaylist;
    QPushButton    *m_volumeButton;
    QPushButton    *m_btnConfig;
    QSlider        *m_seekbar;
    QSlider        *m_volumeSlider;
    QMenu          *m_volumeMenu;
    QTimer         *m_timer;
    QSystemTrayIcon *m_trayIcon;
    QMenu          *m_trayMenu;

    QMediaPlayer   *m_player;
    QAudioOutput   *m_audioOutput;

    PlaylistWindow *m_playlistWindow;
    ConfigWindow   *m_configWindow;

    QString         m_currentFile;
    QStringList     m_playlist;
    bool            m_isPaused;
    qint64          m_audioLength;
};
