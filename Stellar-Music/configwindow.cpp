#include "configwindow.h"
#include "settings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QLabel>
#include <QPixmap>
#include <QDir>
#include <QCoreApplication>
#include <QCloseEvent>
#include <QIcon>
#include <QProcess>
#include <QFile>
#include <QStandardPaths>

ConfigWindow::ConfigWindow(QWidget *parent)
    : QDialog(parent)
    , m_settings("Player", "AudioPlayer")
{
    setWindowTitle("Configuración");

    QString iconPath = getIconPath();
    if (!iconPath.isEmpty())
        setWindowIcon(QIcon(iconPath));
    else
        setWindowIcon(QIcon::fromTheme("audio-x-generic"));

    setupUi();
    loadSavedGeometry();
    loadEqSettings();
}

void ConfigWindow::setupUi() {
    auto *tabWidget = new QTabWidget(this);

    // ── General tab ──────────────────────────────────────────────────────────
    auto *generalTab    = new QWidget();
    auto *generalLayout = new QVBoxLayout(generalTab);

    m_startupCheck  = new QCheckBox("Iniciar con el sistema");
    m_minimizeCheck = new QCheckBox("Minimizar a la bandeja del sistema al minimizar");

    m_startupCheck->setChecked(m_settings.value("startup", false).toBool());
    m_minimizeCheck->setChecked(m_settings.value("minimize_to_tray", false).toBool());

    connect(m_startupCheck,  &QCheckBox::stateChanged, this, &ConfigWindow::saveSettings);
    connect(m_minimizeCheck, &QCheckBox::stateChanged, this, &ConfigWindow::saveSettings);

    generalLayout->addWidget(m_startupCheck);
    generalLayout->addWidget(m_minimizeCheck);
    generalLayout->addStretch();

    // ── Equalizer tab ─────────────────────────────────────────────────────────
    auto *eqTab    = new QWidget();
    auto *eqLayout = new QVBoxLayout(eqTab);

    const QStringList frequencies = {
        "60Hz","170Hz","310Hz","600Hz","1kHz","3kHz","6kHz","12kHz","14kHz","16kHz"
    };

    auto *sliderLayout = new QHBoxLayout();
    for (const QString &freq : frequencies) {
        auto *container   = new QVBoxLayout();
        auto *slider      = new QSlider(Qt::Vertical);
        auto *freqLabel   = new QLabel(freq);
        auto *valueLabel  = new QLabel("0 dB");

        slider->setRange(-12, 12);
        slider->setValue(0);
        slider->setTickPosition(QSlider::TicksBothSides);
        slider->setTickInterval(3);

        valueLabel->setAlignment(Qt::AlignCenter);
        freqLabel->setAlignment(Qt::AlignCenter);

        connect(slider, &QSlider::valueChanged, this, [valueLabel](int v){
            valueLabel->setText(QString("%1 dB").arg(v));
        });
        connect(slider, &QSlider::valueChanged, this, &ConfigWindow::saveEqSettings);

        m_eqSliders[freq] = slider;

        container->addWidget(valueLabel);
        container->addWidget(slider, 1, Qt::AlignHCenter);
        container->addWidget(freqLabel);
        sliderLayout->addLayout(container);
    }

    // Preset buttons
    auto *presetLayout = new QHBoxLayout();
    const QStringList presets = {"Plano","Rock","Pop","Jazz","Clásica"};
    for (const QString &p : presets) {
        auto *btn = new QPushButton(p);
        connect(btn, &QPushButton::clicked, this, [this, p]{ applyPreset(p); });
        presetLayout->addWidget(btn);
    }

    eqLayout->addLayout(sliderLayout);
    eqLayout->addLayout(presetLayout);

    // ── About tab ─────────────────────────────────────────────────────────────
    auto *aboutTab    = new QWidget();
    auto *aboutLayout = new QVBoxLayout(aboutTab);

    QString iconPath = getIconPath();
    if (!iconPath.isEmpty()) {
        auto *iconLabel = new QLabel();
        QPixmap pix(iconPath);
        if (!pix.isNull()) {
            iconLabel->setPixmap(pix.scaled(64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));
            iconLabel->setAlignment(Qt::AlignCenter);
            aboutLayout->addWidget(iconLabel);
        }
    }

    auto *appName = new QLabel("Hero Music");
    appName->setStyleSheet("font-size: 16px; font-weight: bold;");
    auto *version     = new QLabel("Versión 1.0");
    auto *author      = new QLabel("Desarrollado por: B&R.Comp");
    auto *description = new QLabel("Un reproductor de audio simple y elegante\ncon soporte para archivos MP3 y WAV.");

    for (QLabel *lbl : {appName, version, author, description}) {
        lbl->setAlignment(Qt::AlignCenter);
        aboutLayout->addWidget(lbl);
    }
    aboutLayout->addStretch();

    // ── Assemble tabs ─────────────────────────────────────────────────────────
    tabWidget->addTab(generalTab, "General");
    tabWidget->addTab(eqTab,      "Ecualización");
    tabWidget->addTab(aboutTab,   "Acerca de");

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(tabWidget);
}

void ConfigWindow::loadSavedGeometry() {
    auto ws = loadWindowState("config", 150, 150, 439, 267);
    setGeometry(ws.x, ws.y, ws.width, ws.height);
    if (ws.state)
        setWindowState(Qt::WindowStates(ws.state));
}

void ConfigWindow::closeEvent(QCloseEvent *event) {
    saveWindowState("config", geometry(), windowState());
    event->accept();
}

void ConfigWindow::saveSettings() {
    m_settings.setValue("startup",         m_startupCheck->isChecked());
    m_settings.setValue("minimize_to_tray", m_minimizeCheck->isChecked());

    // Linux autostart via .desktop file
    QString autostartDir  = QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                            + "/autostart/";
    QString desktopFile   = autostartDir + "heromusic.desktop";

    if (m_startupCheck->isChecked()) {
        QDir().mkpath(autostartDir);
        QFile f(desktopFile);
        if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&f);
            out << "[Desktop Entry]\n"
                << "Type=Application\n"
                << "Name=Hero Music\n"
                << "Exec=" << QCoreApplication::applicationFilePath() << "\n"
                << "Hidden=false\n"
                << "NoDisplay=false\n"
                << "X-GNOME-Autostart-enabled=true\n";
        }
    } else {
        QFile::remove(desktopFile);
    }
}

void ConfigWindow::saveEqSettings() {
    QMap<QString, QVariant> eqValues;
    for (auto it = m_eqSliders.constBegin(); it != m_eqSliders.constEnd(); ++it)
        eqValues[it.key()] = it.value()->value();

    m_settings.setValue("equalizer", eqValues);

    // Notify parent AudioPlayer
    QMap<QString, int> intValues;
    for (auto it = m_eqSliders.constBegin(); it != m_eqSliders.constEnd(); ++it)
        intValues[it.key()] = it.value()->value();

    emit equalizerChanged(intValues);
}

void ConfigWindow::loadEqSettings() {
    QMap<QString, QVariant> eqValues = m_settings.value("equalizer").toMap();
    for (auto it = eqValues.constBegin(); it != eqValues.constEnd(); ++it) {
        if (m_eqSliders.contains(it.key()))
            m_eqSliders[it.key()]->setValue(it.value().toInt());
    }
}

void ConfigWindow::applyPreset(const QString &presetName) {
    using Map = QMap<QString, int>;
    static const QMap<QString, Map> presets = {
        {"Plano",   {{"60Hz",0},{"170Hz",0},{"310Hz",0},{"600Hz",0},{"1kHz",0},{"3kHz",0},{"6kHz",0},{"12kHz",0},{"14kHz",0},{"16kHz",0}}},
        {"Rock",    {{"60Hz",4},{"170Hz",3},{"310Hz",-2},{"600Hz",-3},{"1kHz",2},{"3kHz",4},{"6kHz",3},{"12kHz",4},{"14kHz",4},{"16kHz",4}}},
        {"Pop",     {{"60Hz",-1},{"170Hz",-1},{"310Hz",0},{"600Hz",2},{"1kHz",3},{"3kHz",2},{"6kHz",1},{"12kHz",1},{"14kHz",2},{"16kHz",2}}},
        {"Jazz",    {{"60Hz",2},{"170Hz",1},{"310Hz",1},{"600Hz",2},{"1kHz",-1},{"3kHz",-1},{"6kHz",0},{"12kHz",1},{"14kHz",2},{"16kHz",3}}},
        {"Clásica", {{"60Hz",3},{"170Hz",2},{"310Hz",1},{"600Hz",0},{"1kHz",0},{"3kHz",0},{"6kHz",-1},{"12kHz",-1},{"14kHz",-2},{"16kHz",-2}}}
    };

    if (presets.contains(presetName)) {
        const Map &vals = presets[presetName];
        for (auto it = vals.constBegin(); it != vals.constEnd(); ++it) {
            if (m_eqSliders.contains(it.key()))
                m_eqSliders[it.key()]->setValue(it.value());
        }
        saveEqSettings();
    }
}
