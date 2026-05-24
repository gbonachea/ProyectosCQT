#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>
#include <QLabel>
#include <QProgressBar>
#include <QComboBox>
#include <QSplitter>
#include <QStatusBar>
#include <QSettings>
#include <QThread>
#include <QMouseEvent>
#include "filesearcher.h"
#include "previewwidget.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private slots:
    void onSearch();
    void onStop();
    void onResultFound(const SearchResult &result);
    void onSearchProgress(int files, int dirs);
    void onSearchFinished(int total);
    void onSearchError(const QString &error);
    void onItemClicked(QTreeWidgetItem *item, int column);
    void onOpenFile();
    void onOpenFolder();
    void onAbout();
    void onMinimize();
    void onMaximize();
    void onClose();

private:
    void setupUI();
    void setupConnections();
    void loadSettings();
    void saveSettings();
    QStringList getSearchDirectories();
    void cleanupSearch();

    QLineEdit *searchInput;
    QPushButton *searchButton;
    QPushButton *stopButton;
    QPushButton *aboutButton;
    QPushButton *minButton;
    QPushButton *maxButton;
    QPushButton *closeButton;
    QWidget *titleBar;
    QLabel *titleLabel;
    QComboBox *typeFilter;
    QTreeWidget *resultTree;
    PreviewWidget *previewWidget;
    QProgressBar *progressBar;
    QLabel *statusLabel;
    QSplitter *splitter;

    FileSearcher *searcher;
    QThread *searchThread;
    QVector<SearchResult> results;
    bool isSearching;
    bool isMaximized;
    QPoint dragStart;
    bool dragging;
};
