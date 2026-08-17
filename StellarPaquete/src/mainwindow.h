#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "packagemetadata.h"

class QCheckBox;
class QComboBox;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QSystemTrayIcon;
class QTabWidget;
class QTextEdit;
class QTreeWidget;
class QTreeWidgetItem;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private slots:
    void addFile();
    void addFolder();
    void removeSelected();
    void addDependency();
    void removeSelectedDependency();
    void importDependencies();
    void exportDependencies();
    void installMissingTools();
    void showAbout();
    void browseOutputDirectory();
    void generatePackages();
    void showBuildNotification(bool success);
    void appendLog(const QString &message);

private:
    void setupUi();
    void setupGeneralTab(QWidget *tab);
    void setupFilesTab(QWidget *tab);
    void setupDependenciesTab(QWidget *tab);
    void setupDesktopTab(QWidget *tab);
    void setupFormatTab(QWidget *tab);
    void setupLogTab(QWidget *tab);
    void checkDependencies();
    void addFileToTree(const QString &source, const QString &target);
    QString installDependenciesScript() const;
    PackageMetadata collectMetadata() const;
    bool validateMetadata(const PackageMetadata &meta, QString *error) const;

    QTabWidget *m_tabWidget = nullptr;

    QLineEdit *m_nameEdit = nullptr;
    QLineEdit *m_versionEdit = nullptr;
    QComboBox *m_archCombo = nullptr;
    QLineEdit *m_maintainerEdit = nullptr;
    QComboBox *m_licenseCombo = nullptr;
    QLineEdit *m_shortDescEdit = nullptr;
    QPlainTextEdit *m_longDescEdit = nullptr;

    QTreeWidget *m_filesTree = nullptr;

    QComboBox *m_depPresetCombo = nullptr;
    QListWidget *m_dependenciesList = nullptr;

    QCheckBox *m_desktopCheck = nullptr;
    QLineEdit *m_desktopNameEdit = nullptr;
    QLineEdit *m_desktopCommentEdit = nullptr;
    QLineEdit *m_desktopExecEdit = nullptr;
    QLineEdit *m_desktopIconEdit = nullptr;
    QLineEdit *m_desktopCategoriesEdit = nullptr;
    QLineEdit *m_desktopMimeEdit = nullptr;
    QCheckBox *m_desktopTerminalCheck = nullptr;
    QCheckBox *m_desktopStartupCheck = nullptr;

    QCheckBox *m_debCheck = nullptr;
    QCheckBox *m_rpmCheck = nullptr;
    QCheckBox *m_appImageCheck = nullptr;
    QLineEdit *m_outputDirEdit = nullptr;

    QTextEdit *m_logEdit = nullptr;
    QProgressBar *m_progressBar = nullptr;
    QSystemTrayIcon *m_trayIcon = nullptr;
    QPushButton *m_generateButton = nullptr;
};

#endif
