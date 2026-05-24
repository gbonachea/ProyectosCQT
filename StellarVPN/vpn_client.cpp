#include "vpn_client.h"

#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QCloseEvent>
#include <QPixmap>
#include <QIcon>
#include <QAction>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QScrollArea>
#include <QSizePolicy>
#include <QStandardPaths>

#include <cmath>
#include <cstdlib>
#include <ctime>

// ─────────────────────────────────────────────────────────────────────────────
//  Shared stylesheet (dark theme, identical to the Python version)
// ─────────────────────────────────────────────────────────────────────────────
static const QString kDarkStyle = R"(
    QWidget {
        background-color: #23272f;
        color: #e0e0e0;
        font-family: 'Segoe UI', sans-serif;
        font-size: 13px;
    }
    QPushButton {
        background-color: #2c313c;
        color: #e0e0e0;
        border: none;
        padding: 6px 16px;
        border-radius: 4px;
    }
    QPushButton:hover  { background-color: #3c424f; }
    QPushButton:pressed{ background-color: #4c525f; }
    QPushButton:disabled { color: #555; background-color: #1e2128; }
    QComboBox {
        background-color: #2c313c;
        color: #e0e0e0;
        border: 1px solid #3c424f;
        padding: 4px 8px;
        border-radius: 4px;
    }
    QComboBox QAbstractItemView {
        background-color: #2c313c;
        color: #e0e0e0;
        selection-background-color: #3c424f;
    }
    QTabWidget::pane  { border: 1px solid #3c424f; }
    QTabBar::tab {
        background-color: #2c313c;
        color: #e0e0e0;
        padding: 6px 14px;
    }
    QTabBar::tab:selected { background-color: #3c424f; }
    QListWidget {
        background-color: #2c313c;
        color: #e0e0e0;
        border: 1px solid #3c424f;
    }
    QListWidget::item:selected { background-color: #3c424f; }
    QLineEdit {
        background-color: #2c313c;
        color: #e0e0e0;
        border: 1px solid #3c424f;
        padding: 4px 8px;
        border-radius: 4px;
    }
    QCheckBox { color: #e0e0e0; spacing: 6px; }
    QCheckBox::indicator {
        width: 16px; height: 16px;
        border: 1px solid #555;
        background-color: #2c313c;
    }
    QCheckBox::indicator:checked { background-color: #5b9bd5; }
    QMessageBox {
        background-color: #23272f;
        color: #e0e0e0;
        min-width: 400px;
    }
    QMessageBox QLabel { color: #e0e0e0; }
    QDialog { background-color: #2c313c; }
    QLabel  { color: #e0e0e0; }
    QScrollArea { border: none; }
)";

// ─────────────────────────────────────────────────────────────────────────────
//  Helper: application base directory (same folder as the executable)
// ─────────────────────────────────────────────────────────────────────────────
static QString appDir()
{
    return QApplication::applicationDirPath();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Constructor
// ─────────────────────────────────────────────────────────────────────────────
VPNClient::VPNClient(QWidget *parent)
    : QMainWindow(parent)
    , combobox(nullptr)
    , connectBtn(nullptr)
    , disconnectBtn(nullptr)
    , speedBtn(nullptr)
    , ipBtn(nullptr)
    , autoStartVar(nullptr)
    , autoConnectVar(nullptr)
    , killSwitchVar(nullptr)
    , trayIcon(nullptr)
    , trayMenu(nullptr)
    , restoreAction(nullptr)
    , quitAction(nullptr)
    , vpnProcess(nullptr)
    , timer(nullptr)
{
    std::srand(static_cast<unsigned>(std::time(nullptr)));

    setWindowTitle("Cliente VPN - StellarVPN");
    setGeometry(100, 100, 420, 480);
    setStyleSheet(kDarkStyle);
    setWindowFlags(Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowCloseButtonHint);

    // Window icon
    const QString iconPath = appDir() + "/icons/icons.png";
    if (QFile::exists(iconPath))
        setWindowIcon(QIcon(iconPath));

    setupTrayIcon();
    initUI();
    updateInfo();

    trayIcon->show();
}

VPNClient::~VPNClient()
{
    if (vpnProcess && vpnProcess->state() != QProcess::NotRunning) {
        vpnProcess->terminate();
        vpnProcess->waitForFinished(3000);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tray icon setup
// ─────────────────────────────────────────────────────────────────────────────
void VPNClient::setupTrayIcon()
{
    trayIcon = new QSystemTrayIcon(this);

    const QString iconPath = appDir() + "/icons/icons.png";
    if (QFile::exists(iconPath))
        trayIcon->setIcon(QIcon(iconPath));

    trayMenu      = new QMenu(this);
    restoreAction = trayMenu->addAction("Restaurar");
    trayMenu->addSeparator();
    quitAction    = trayMenu->addAction("Salir");

    connect(restoreAction, &QAction::triggered, this, &QMainWindow::showNormal);
    connect(quitAction,    &QAction::triggered, this, &VPNClient::closeApplication);

    trayIcon->setContextMenu(trayMenu);
    connect(trayIcon, &QSystemTrayIcon::activated,
            this,     &VPNClient::trayIconActivated);
}

// ─────────────────────────────────────────────────────────────────────────────
//  closeEvent  – mirrors the Python closeEvent dialog
// ─────────────────────────────────────────────────────────────────────────────
void VPNClient::closeEvent(QCloseEvent *event)
{
    QMessageBox dialog(this);
    dialog.setWindowTitle("Cerrar StellarVPN");
    dialog.setText("¿Qué desea hacer?");
    dialog.setIcon(QMessageBox::Question);
    dialog.setStyleSheet(kDarkStyle);

    QPushButton *minimizeBtn = dialog.addButton("Minimizar a la bandeja", QMessageBox::ActionRole);
    QPushButton *closeBtn    = dialog.addButton("Cerrar aplicación",      QMessageBox::ActionRole);
    /* QPushButton *cancelBtn = */ dialog.addButton("Cancelar", QMessageBox::RejectRole);

    dialog.exec();

    QAbstractButton *clicked = dialog.clickedButton();
    if (clicked == minimizeBtn) {
        event->ignore();
        hide();
    } else if (clicked == closeBtn) {
        if (vpnProcess && vpnProcess->state() != QProcess::NotRunning)
            disconnectVPN();
        trayIcon->hide();
        event->accept();
        QApplication::quit();
    } else {
        event->ignore();
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tray icon double-click → restore window
// ─────────────────────────────────────────────────────────────────────────────
void VPNClient::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick) {
        show();
        activateWindow();
    }
}

void VPNClient::closeApplication()
{
    if (vpnProcess && vpnProcess->state() != QProcess::NotRunning)
        disconnectVPN();
    trayIcon->hide();
    QApplication::quit();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Build the main window UI
// ─────────────────────────────────────────────────────────────────────────────
void VPNClient::initUI()
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QVBoxLayout *vbox = new QVBoxLayout(central);
    vbox->setContentsMargins(16, 16, 16, 16);
    vbox->setSpacing(10);

    // ── App icon ──
    const QString iconPath = appDir() + "/icons/icon.png";
    if (QFile::exists(iconPath)) {
        QPixmap pixmap(iconPath);
        pixmap = pixmap.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QLabel *iconLabel = new QLabel;
        iconLabel->setPixmap(pixmap);
        iconLabel->setAlignment(Qt::AlignCenter);
        vbox->addWidget(iconLabel);
    }

    // ── Server label + combo ──
    QLabel *label = new QLabel("Selecciona el servidor para la conexion:");
    label->setStyleSheet("color: #e0e0e0; font-size: 14px;");
    vbox->addWidget(label);

    combobox = new QComboBox;
    combobox->addItems(getOvpnFiles());
    connect(combobox, &QComboBox::currentIndexChanged,
            this, &VPNClient::onServerSelected);
    vbox->addWidget(combobox);

    // ── Connect / Disconnect buttons ──
    QHBoxLayout *btnHBox = new QHBoxLayout;
    connectBtn = new QPushButton("Conectar VPN");
    connectBtn->setEnabled(false);
    connect(connectBtn, &QPushButton::clicked, this, &VPNClient::connectVPN);
    btnHBox->addWidget(connectBtn);

    disconnectBtn = new QPushButton("Desconectar VPN");
    disconnectBtn->setEnabled(false);
    connect(disconnectBtn, &QPushButton::clicked, this, &VPNClient::disconnectVPN);
    btnHBox->addWidget(disconnectBtn);
    vbox->addLayout(btnHBox);

    // ── Speed / IP info buttons (display only) ──
    speedBtn = new QPushButton("Velocidad: -- Mbps");
    speedBtn->setEnabled(false);
    vbox->addWidget(speedBtn);

    ipBtn = new QPushButton("IP: -- | Puerto: --");
    ipBtn->setEnabled(false);
    vbox->addWidget(ipBtn);

    // ── Settings button (gear icon, bottom-right) ──
    const QString configIconPath = appDir() + "/icons/menu.png";
    QPushButton *configBtn = new QPushButton;
    if (QFile::exists(configIconPath))
        configBtn->setIcon(QIcon(configIconPath));
    else
        configBtn->setText("⚙");
    configBtn->setFixedSize(30, 30);
    connect(configBtn, &QPushButton::clicked, this, &VPNClient::openSettingsWindow);

    QHBoxLayout *gearLayout = new QHBoxLayout;
    gearLayout->addStretch();
    gearLayout->addWidget(configBtn);
    vbox->addLayout(gearLayout);

    // ── Update timer ──
    timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, &VPNClient::updateInfo);
    timer->start(1000);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Return list of .ovpn files in <appDir>/ovpn/
// ─────────────────────────────────────────────────────────────────────────────
QStringList VPNClient::getOvpnFiles()
{
    const QString folder = appDir() + "/ovpn";
    QDir dir(folder);
    if (!dir.exists())
        return {};
    return dir.entryList(QStringList() << "*.ovpn", QDir::Files);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Combo selection changed
// ─────────────────────────────────────────────────────────────────────────────
void VPNClient::onServerSelected(int index)
{
    if (index < 0) return;
    const QString folder = appDir() + "/ovpn";
    ovpnFile = folder + "/" + combobox->currentText();
    connectBtn->setEnabled(true);
    QMessageBox::information(this, "Archivo seleccionado",
                             QString("Archivo: %1").arg(ovpnFile));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Connect VPN  – asks for sudo password then launches openvpn
// ─────────────────────────────────────────────────────────────────────────────
void VPNClient::connectVPN()
{
    if (ovpnFile.isEmpty()) {
        QMessageBox::critical(this, "Error", "Debes seleccionar un archivo .ovpn");
        return;
    }

    // ── Password dialog ──
    QDialog authDialog(this);
    authDialog.setWindowTitle("Autenticación requerida");
    authDialog.resize(300, 120);
    authDialog.setStyleSheet(kDarkStyle);

    QVBoxLayout *vbox = new QVBoxLayout(&authDialog);
    vbox->addWidget(new QLabel("Contraseña de sudo:"));

    QLineEdit *passwordEntry = new QLineEdit;
    passwordEntry->setEchoMode(QLineEdit::Password);
    vbox->addWidget(passwordEntry);

    QPushButton *submitBtn = new QPushButton("Conectar");
    vbox->addWidget(submitBtn);

    // Lambda triggered by the submit button
    connect(submitBtn, &QPushButton::clicked, [&]() {
        const QString password = passwordEntry->text();
        authDialog.accept();

        vpnProcess = new QProcess(this);
        vpnProcess->setProcessChannelMode(QProcess::MergedChannels);

        QStringList args;
        args << "-S" << "openvpn" << "--config" << ovpnFile;

        vpnProcess->start("sudo", args);
        if (!vpnProcess->waitForStarted(3000)) {
            QMessageBox::critical(this, "Error", "No se pudo iniciar openvpn.");
            delete vpnProcess;
            vpnProcess = nullptr;
            return;
        }

        // Feed password to sudo's stdin
        vpnProcess->write((password + "\n").toUtf8());
        vpnProcess->closeWriteChannel();

        connectBtn->setEnabled(false);
        disconnectBtn->setEnabled(true);
        QMessageBox::information(this, "VPN",
            QString("Conectando a la VPN con %1 ...").arg(ovpnFile));
    });

    authDialog.exec();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Disconnect VPN
// ─────────────────────────────────────────────────────────────────────────────
void VPNClient::disconnectVPN()
{
    if (vpnProcess) {
        vpnProcess->terminate();
        if (!vpnProcess->waitForFinished(3000))
            vpnProcess->kill();
        delete vpnProcess;
        vpnProcess = nullptr;

        connectBtn->setEnabled(true);
        disconnectBtn->setEnabled(false);
        QMessageBox::information(this, "VPN", "VPN desconectada");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Update status labels every second
// ─────────────────────────────────────────────────────────────────────────────
void VPNClient::updateInfo()
{
    const bool connected = vpnProcess &&
                           vpnProcess->state() == QProcess::Running;
    if (connected) {
        // Simulate a random speed between 10 and 100 Mbps (same as Python version)
        double speed = 10.0 + (std::rand() / static_cast<double>(RAND_MAX)) * 90.0;
        speed = std::round(speed * 100.0) / 100.0;
        speedBtn->setText(QString("Velocidad: %1 Mbps").arg(speed, 0, 'f', 2));
        ipBtn->setText("IP: 192.168.1.100 | Puerto: 1194");
    } else {
        speedBtn->setText("Velocidad: -- Mbps");
        ipBtn->setText("IP: -- | Puerto: --");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Load settings from settings.json
// ─────────────────────────────────────────────────────────────────────────────
void VPNClient::loadSettings()
{
    // autoStartVar / autoConnectVar / killSwitchVar must be set before calling this
    if (!autoStartVar) return;

    const QString path = appDir() + "/settings.json";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) return;

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    const QJsonObject   obj = doc.object();

    autoStartVar->setChecked(obj.value("auto_start").toBool(false));
    autoConnectVar->setChecked(obj.value("auto_connect").toBool(false));
    killSwitchVar->setChecked(obj.value("kill_switch").toBool(false));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Save settings to settings.json
// ─────────────────────────────────────────────────────────────────────────────
void VPNClient::saveSettings()
{
    QJsonObject obj;
    obj["auto_start"]   = autoStartVar->isChecked();
    obj["auto_connect"] = autoConnectVar->isChecked();
    obj["kill_switch"]  = killSwitchVar->isChecked();

    const QString path = appDir() + "/settings.json";
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, "Error",
                              QString("No se pudo guardar: %1").arg(file.errorString()));
        return;
    }
    file.write(QJsonDocument(obj).toJson());
    QMessageBox::information(this, "Ajustes", "Cambios guardados correctamente.");
}

// ─────────────────────────────────────────────────────────────────────────────
//  Settings window  (3 tabs: Generales · Servidores · Acerca de)
// ─────────────────────────────────────────────────────────────────────────────
void VPNClient::openSettingsWindow()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Ajustes del aplicativo");
    dialog.setStyleSheet(kDarkStyle);
    dialog.resize(400, 420);

    const QString appIconPath = appDir() + "/icons/icon.png";

    QVBoxLayout *mainLayout = new QVBoxLayout(&dialog);
    QTabWidget  *tabs       = new QTabWidget;
    mainLayout->addWidget(tabs);

    // ── Tab 1: Generales ─────────────────────────────────────────────────────
    QWidget     *generales  = new QWidget;
    QVBoxLayout *genLayout  = new QVBoxLayout(generales);

    QLabel *genLabel = new QLabel("Ajustes generales");
    genLabel->setStyleSheet("font-size: 14px;");
    genLayout->addWidget(genLabel);

    autoStartVar   = new QCheckBox("Iniciar automáticamente con el sistema");
    autoConnectVar = new QCheckBox("Conectar automáticamente al iniciar");
    killSwitchVar  = new QCheckBox("Kill Switch (bloquear internet si la VPN se desconecta)");
    genLayout->addWidget(autoStartVar);
    genLayout->addWidget(autoConnectVar);
    genLayout->addWidget(killSwitchVar);

    loadSettings();   // populate checkboxes from disk

    QPushButton *guardarBtn = new QPushButton("Guardar cambios");
    connect(guardarBtn, &QPushButton::clicked, this, &VPNClient::saveSettings);
    genLayout->addWidget(guardarBtn);
    genLayout->addStretch();

    tabs->addTab(generales, "Generales");

    // ── Tab 2: Servidores ────────────────────────────────────────────────────
    QWidget     *servidores = new QWidget;
    QVBoxLayout *servLayout = new QVBoxLayout(servidores);

    QLabel *servLabel = new QLabel("Lista de servidores disponibles");
    servLabel->setStyleSheet("font-size: 14px;");
    servLayout->addWidget(servLabel);

    QListWidget *serversList = new QListWidget;
    serversList->addItems(getOvpnFiles());
    servLayout->addWidget(serversList);

    QPushButton *eliminarBtn = new QPushButton("Eliminar servidor");
    connect(eliminarBtn, &QPushButton::clicked, [&]() {
        const int row = serversList->currentRow();
        if (row < 0) {
            QMessageBox::warning(&dialog, "Eliminar servidor",
                                 "Selecciona un servidor para eliminar.");
            return;
        }
        const QString nombre = serversList->item(row)->text();
        const QString ruta   = appDir() + "/ovpn/" + nombre;
        if (QFile::remove(ruta)) {
            delete serversList->takeItem(row);
            QMessageBox::information(&dialog, "Eliminar servidor",
                                     QString("Servidor '%1' eliminado.").arg(nombre));
        } else {
            QMessageBox::critical(&dialog, "Error",
                                  QString("No se pudo eliminar '%1'.").arg(nombre));
        }
    });
    servLayout->addWidget(eliminarBtn);

    QPushButton *agregarBtn = new QPushButton("Agregar servidor");
    connect(agregarBtn, &QPushButton::clicked, [&]() {
        // Start in home directory for better user experience
        const QString startDir = QStandardPaths::writableLocation(QStandardPaths::HomeLocation);
        const QString archivo = QFileDialog::getOpenFileName(
            &dialog, 
            "Selecciona archivo .ovpn", 
            startDir,
            "Archivos OVPN (*.ovpn);;Todos los archivos (*)");
        
        if (archivo.isEmpty()) return;

        const QString nombre  = QFileInfo(archivo).fileName();
        const QString destino = appDir() + "/ovpn/" + nombre;
        
        // Check if directory is writable
        QDir ovpnDir(appDir() + "/ovpn");
        if (!ovpnDir.exists()) {
            QMessageBox::critical(&dialog, "Error",
                                  "El directorio /ovpn no existe.");
            return;
        }
        
        // Check if file already exists
        if (QFile::exists(destino)) {
            QMessageBox::warning(&dialog, "Archivo existente",
                                 QString("El servidor '%1' ya existe.").arg(nombre));
            return;
        }
        
        if (QFile::copy(archivo, destino)) {
            serversList->addItem(nombre);
            // Refresh the combobox in main window
            combobox->clear();
            combobox->addItems(getOvpnFiles());
            QMessageBox::information(&dialog, "Agregar servidor",
                                     QString("Servidor '%1' agregado exitosamente.").arg(nombre));
        } else {
            QMessageBox::critical(&dialog, "Error",
                                  QString("No se pudo copiar '%1'.\nVerifica permisos en %2")
                                  .arg(nombre, appDir() + "/ovpn"));
        }
    });
    servLayout->addWidget(agregarBtn);

    tabs->addTab(servidores, "Servidores");

    // ── Tab 3: Acerca de ─────────────────────────────────────────────────────
    QWidget     *acerca      = new QWidget;
    QVBoxLayout *acercaLayout = new QVBoxLayout(acerca);
    acercaLayout->setAlignment(Qt::AlignCenter);

    if (QFile::exists(appIconPath)) {
        QPixmap pixmap(appIconPath);
        pixmap = pixmap.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QLabel *iconLabel = new QLabel;
        iconLabel->setPixmap(pixmap);
        iconLabel->setAlignment(Qt::AlignCenter);
        acercaLayout->addWidget(iconLabel);
    }

    QLabel *acercaLabel = new QLabel(
        "Cliente VPN StellarVPN\nVersión 1.0\nDesarrollado por B&R.Comp");
    acercaLabel->setStyleSheet("font-size: 14px;");
    acercaLabel->setAlignment(Qt::AlignCenter);
    acercaLayout->addWidget(acercaLabel);
    acercaLayout->addStretch();

    tabs->addTab(acerca, "Acerca de");

    dialog.exec();

    // Reset pointers after dialog closes (widgets are owned by the dialog)
    autoStartVar   = nullptr;
    autoConnectVar = nullptr;
    killSwitchVar  = nullptr;
}
