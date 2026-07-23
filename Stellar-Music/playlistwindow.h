#pragma once
#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

class PlaylistWindow : public QWidget {
    Q_OBJECT
public:
    explicit PlaylistWindow(QWidget *parent = nullptr);
    QListWidget *playlist() { return m_playlist; }

signals:
    void playSignal(const QString &index);
    void addFileSignal(const QString &filepath);

protected:
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private slots:
    void addAudio();
    void moveUp();
    void moveDown();
    void removeAudio();
    void playItem(QListWidgetItem *item);

private:
    void setupUi();
    void loadSavedGeometry();

    QPushButton  *m_btnAdd;
    QPushButton  *m_btnUp;
    QPushButton  *m_btnDown;
    QPushButton  *m_btnRemove;
    QListWidget  *m_playlist;
};
