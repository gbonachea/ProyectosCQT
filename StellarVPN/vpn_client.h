#ifndef VPN_CLIENT_H
#define VPN_CLIENT_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QMenu>
#include <QAction>
#include <QTimer>
#include <QProcess>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
#include <QListWidget>
#include <QLineEdit>
#include <QTabWidget>
#include <QDialog>

class VPNClient : public QMainWindow
{
    Q_OBJECT

public:
    explicit VPNClient(QWidget *parent = nullptr);
    ~VPNClient();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void trayIconActivated(QSystemTrayIcon::ActivationReason reason);
    void closeApplication();
    void onServerSelected(int index);
    void connectVPN();
    void disconnectVPN();
    void updateInfo();
    void openSettingsWindow();
    void saveSettings();
    void loadSettings();

private:
    void initUI();
    void setupTrayIcon();
    QStringList getOvpnFiles();

    // UI Elements
    QComboBox       *combobox;
    QPushButton     *connectBtn;
    QPushButton     *disconnectBtn;
    QPushButton     *speedBtn;
    QPushButton     *ipBtn;

    // Settings checkboxes (kept as members for load/save)
    QCheckBox       *autoStartVar;
    QCheckBox       *autoConnectVar;
    QCheckBox       *killSwitchVar;

    // System tray
    QSystemTrayIcon *trayIcon;
    QMenu           *trayMenu;
    QAction         *restoreAction;
    QAction         *quitAction;

    // VPN process
    QProcess        *vpnProcess;
    QString          ovpnFile;

    // Timer for status updates
    QTimer          *timer;
};

#endif // VPN_CLIENT_H
