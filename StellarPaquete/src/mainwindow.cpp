#include "mainwindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QProgressBar>
#include <QPushButton>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QStatusBar>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QTextCursor>
#include <QTextEdit>
#include <QTextStream>
#include <QTimer>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QVBoxLayout>

#include "packagebuilder.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setupUi();
    checkDependencies();
}

void MainWindow::setupUi()
{
    setWindowTitle(QStringLiteral("Stellar Paquete — Creador de paquetes .deb, .rpm y Arch Linux"));
    resize(820, 580);

    QPushButton *aboutButton = new QPushButton(QStringLiteral("Acerca de..."), this);
    aboutButton->setFlat(true);
    connect(aboutButton, &QPushButton::clicked, this, &MainWindow::showAbout);
    statusBar()->addPermanentWidget(aboutButton);

    m_tabWidget = new QTabWidget(this);
    setCentralWidget(m_tabWidget);

    QWidget *generalTab = new QWidget(this);
    QWidget *filesTab = new QWidget(this);
    QWidget *dependenciesTab = new QWidget(this);
    QWidget *desktopTab = new QWidget(this);
    QWidget *formatTab = new QWidget(this);
    QWidget *logTab = new QWidget(this);

    setupGeneralTab(generalTab);
    setupFilesTab(filesTab);
    setupDependenciesTab(dependenciesTab);
    setupDesktopTab(desktopTab);
    setupFormatTab(formatTab);
    setupLogTab(logTab);

    m_tabWidget->addTab(generalTab, QStringLiteral("Información General"));
    m_tabWidget->addTab(filesTab, QStringLiteral("Archivos e Instalación"));
    m_tabWidget->addTab(dependenciesTab, QStringLiteral("Dependencias"));
    m_tabWidget->addTab(desktopTab, QStringLiteral("Escritorio (.desktop)"));
    m_tabWidget->addTab(formatTab, QStringLiteral("Formato de Salida"));
    m_tabWidget->addTab(logTab, QStringLiteral("Consola"));
}

void MainWindow::setupGeneralTab(QWidget *tab)
{
    QFormLayout *form = new QFormLayout(tab);

    m_nameEdit = new QLineEdit(tab);
    form->addRow(QStringLiteral("Nombre del paquete:"), m_nameEdit);

    m_versionEdit = new QLineEdit(tab);
    m_versionEdit->setText(QStringLiteral("1.0.0"));
    form->addRow(QStringLiteral("Versión:"), m_versionEdit);

    m_archCombo = new QComboBox(tab);
    m_archCombo->addItems({ QStringLiteral("amd64"), QStringLiteral("i386"),
                            QStringLiteral("arm64"), QStringLiteral("armhf"),
                            QStringLiteral("all"), QStringLiteral("x86_64"),
                            QStringLiteral("aarch64") });
    form->addRow(QStringLiteral("Arquitectura:"), m_archCombo);

    m_maintainerEdit = new QLineEdit(tab);
    m_maintainerEdit->setPlaceholderText(QStringLiteral("Nombre <correo@ejemplo.com>"));
    form->addRow(QStringLiteral("Mantenedor:"), m_maintainerEdit);

    m_licenseCombo = new QComboBox(tab);
    m_licenseCombo->setEditable(true);
    m_licenseCombo->addItems({ QStringLiteral("GPL-3.0-or-later"),
                               QStringLiteral("GPL-2.0-or-later"),
                               QStringLiteral("GPL-3.0-only"),
                               QStringLiteral("LGPL-3.0-or-later"),
                               QStringLiteral("LGPL-2.1-or-later"),
                               QStringLiteral("AGPL-3.0-or-later"),
                               QStringLiteral("Apache-2.0"),
                               QStringLiteral("MIT"),
                               QStringLiteral("BSD-2-Clause"),
                               QStringLiteral("BSD-3-Clause"),
                               QStringLiteral("MPL-2.0"),
                               QStringLiteral("ISC"),
                               QStringLiteral("Zlib"),
                               QStringLiteral("Unlicense"),
                               QStringLiteral("CC0-1.0") });
    m_licenseCombo->setCurrentText(QStringLiteral("GPL-3.0-or-later"));
    m_licenseCombo->lineEdit()->setPlaceholderText(QStringLiteral("Selecciona o escribe una licencia (SPDX)..."));
    form->addRow(QStringLiteral("Licencia:"), m_licenseCombo);

    m_shortDescEdit = new QLineEdit(tab);
    form->addRow(QStringLiteral("Descripción corta:"), m_shortDescEdit);

    m_longDescEdit = new QPlainTextEdit(tab);
    m_longDescEdit->setPlaceholderText(
        QStringLiteral("Descripción larga del paquete (cada línea debe ocupar menos de 80 caracteres)..."));
    form->addRow(QStringLiteral("Descripción larga:"), m_longDescEdit);
}

void MainWindow::setupFilesTab(QWidget *tab)
{
    QVBoxLayout *layout = new QVBoxLayout(tab);

    m_filesTree = new QTreeWidget(tab);
    m_filesTree->setColumnCount(2);
    m_filesTree->setHeaderLabels({ QStringLiteral("Archivo de origen"),
                                   QStringLiteral("Ruta de destino (relativa)") });
    m_filesTree->header()->setSectionResizeMode(QHeaderView::Stretch);
    m_filesTree->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_filesTree->setRootIsDecorated(false);
    layout->addWidget(m_filesTree);

    QHBoxLayout *buttons = new QHBoxLayout;
    QPushButton *addFileButton = new QPushButton(QStringLiteral("Añadir archivo..."), tab);
    QPushButton *addFolderButton = new QPushButton(QStringLiteral("Añadir carpeta..."), tab);
    QPushButton *removeButton = new QPushButton(QStringLiteral("Eliminar selección"), tab);

    connect(addFileButton, &QPushButton::clicked, this, &MainWindow::addFile);
    connect(addFolderButton, &QPushButton::clicked, this, &MainWindow::addFolder);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeSelected);

    buttons->addWidget(addFileButton);
    buttons->addWidget(addFolderButton);
    buttons->addStretch(1);
    buttons->addWidget(removeButton);
    layout->addLayout(buttons);
}

void MainWindow::setupDependenciesTab(QWidget *tab)
{
    QVBoxLayout *layout = new QVBoxLayout(tab);

    QLabel *hint = new QLabel(
        QStringLiteral("Paquetes del repositorio del sistema que deben instalarse junto con este paquete "
                       "(se escriben como \"Depends:\" en .deb y como \"Requires:\" en .rpm).\n"
                       "Los nombres deben corresponder al repositorio de cada formato. Los paquetes Arch Linux "
                       "usan las dependencias en el PKGBUILD como \"depends=()\"."),
        tab);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    QGroupBox *addBox = new QGroupBox(QStringLiteral("Añadir dependencia"), tab);
    QHBoxLayout *addLayout = new QHBoxLayout(addBox);
    m_depPresetCombo = new QComboBox(addBox);
    m_depPresetCombo->setEditable(true);
    m_depPresetCombo->setInsertPolicy(QComboBox::NoInsert);
    m_depPresetCombo->setCurrentText(QString());
    m_depPresetCombo->addItems({
        QStringLiteral("libc6"), QStringLiteral("libstdc++6"), QStringLiteral("libgcc-s1"),
        QStringLiteral("zlib1g"), QStringLiteral("libglib2.0-0"), QStringLiteral("libcairo2"),
        QStringLiteral("libx11-6"), QStringLiteral("libxcb1"), QStringLiteral("libxext6"),
        QStringLiteral("libpng16-16"), QStringLiteral("libjpeg8"), QStringLiteral("libfreetype6"),
        QStringLiteral("libfontconfig1"), QStringLiteral("libssl3"), QStringLiteral("libcurl4"),
        QStringLiteral("libsqlite3-0"), QStringLiteral("libxml2"), QStringLiteral("libgtk-3-0"),
        QStringLiteral("libqt6core6"), QStringLiteral("libqt6gui6"), QStringLiteral("libqt6widgets6"),
        QStringLiteral("libqt6network6"), QStringLiteral("libqt6sql6"), QStringLiteral("libqt6svg6"),
        QStringLiteral("libqt6printsupport6"), QStringLiteral("libqt6dbus6"), QStringLiteral("libqt6x11extras6")
    });
    QPushButton *addButton = new QPushButton(QStringLiteral("Añadir"), addBox);
    connect(addButton, &QPushButton::clicked, this, &MainWindow::addDependency);
    addLayout->addWidget(m_depPresetCombo, 1);
    addLayout->addWidget(addButton);
    layout->addWidget(addBox);

    m_dependenciesList = new QListWidget(tab);
    m_dependenciesList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    layout->addWidget(m_dependenciesList, 1);

    QHBoxLayout *buttons = new QHBoxLayout;
    QPushButton *importButton = new QPushButton(QStringLiteral("Importar..."), tab);
    QPushButton *exportButton = new QPushButton(QStringLiteral("Exportar..."), tab);
    QPushButton *removeButton = new QPushButton(QStringLiteral("Eliminar selección"), tab);
    connect(importButton, &QPushButton::clicked, this, &MainWindow::importDependencies);
    connect(exportButton, &QPushButton::clicked, this, &MainWindow::exportDependencies);
    connect(removeButton, &QPushButton::clicked, this, &MainWindow::removeSelectedDependency);
    buttons->addWidget(importButton);
    buttons->addWidget(exportButton);
    buttons->addStretch(1);
    buttons->addWidget(removeButton);
    layout->addLayout(buttons);
}

void MainWindow::setupDesktopTab(QWidget *tab)
{
    QVBoxLayout *layout = new QVBoxLayout(tab);

    m_desktopCheck = new QCheckBox(
        QStringLiteral("Generar archivo .desktop (lanzador para /usr/share/applications)"), tab);
    layout->addWidget(m_desktopCheck);

    QGroupBox *desktopBox = new QGroupBox(QStringLiteral("Propiedades del lanzador"), tab);
    QFormLayout *form = new QFormLayout(desktopBox);

    m_desktopNameEdit = new QLineEdit(desktopBox);
    m_desktopNameEdit->setPlaceholderText(QStringLiteral("Por defecto: nombre del paquete"));
    form->addRow(QStringLiteral("Name:"), m_desktopNameEdit);

    m_desktopCommentEdit = new QLineEdit(desktopBox);
    m_desktopCommentEdit->setPlaceholderText(QStringLiteral("Por defecto: descripción corta"));
    form->addRow(QStringLiteral("Comment:"), m_desktopCommentEdit);

    m_desktopExecEdit = new QLineEdit(desktopBox);
    m_desktopExecEdit->setPlaceholderText(QStringLiteral("Por defecto: nombre del paquete"));
    form->addRow(QStringLiteral("Exec:"), m_desktopExecEdit);

    m_desktopIconEdit = new QLineEdit(desktopBox);
    m_desktopIconEdit->setPlaceholderText(QStringLiteral("Por defecto: nombre del paquete"));
    form->addRow(QStringLiteral("Icon:"), m_desktopIconEdit);

    m_desktopCategoriesList = new QListWidget(desktopBox);
    m_desktopCategoriesList->setMaximumHeight(120);
    const QStringList xdgCategories = {
        QStringLiteral("AudioVideo"), QStringLiteral("Audio"), QStringLiteral("Video"),
        QStringLiteral("Development"), QStringLiteral("Education"), QStringLiteral("Game"),
        QStringLiteral("Graphics"), QStringLiteral("Network"), QStringLiteral("Office"),
        QStringLiteral("Settings"), QStringLiteral("System"), QStringLiteral("Utility")
    };
    for (const QString &cat : xdgCategories) {
        QListWidgetItem *item = new QListWidgetItem(cat, m_desktopCategoriesList);
        item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
        item->setCheckState(cat == QStringLiteral("Utility") ? Qt::Checked : Qt::Unchecked);
    }
    form->addRow(QStringLiteral("Categories:"), m_desktopCategoriesList);

    m_desktopMimeEdit = new QLineEdit(desktopBox);
    m_desktopMimeEdit->setPlaceholderText(QStringLiteral("Opcional, ej. text/html;text/plain;"));
    form->addRow(QStringLiteral("MimeType:"), m_desktopMimeEdit);

    m_desktopTerminalCheck = new QCheckBox(QStringLiteral("Ejecutar en una terminal"), desktopBox);
    form->addRow(QStringLiteral("Terminal:"), m_desktopTerminalCheck);

    m_desktopStartupCheck = new QCheckBox(QStringLiteral("Notificar al iniciar"), desktopBox);
    m_desktopStartupCheck->setChecked(true);
    form->addRow(QStringLiteral("StartupNotify:"), m_desktopStartupCheck);

    layout->addWidget(desktopBox);

    QLabel *hint = new QLabel(
        QStringLiteral("El archivo generado se instala copiándolo a la carpeta de aplicaciones:\n\n"
                       "    cp <paquete>.desktop ~/.local/share/applications/   (usuario)\n"
                       "    sudo cp <paquete>.desktop /usr/share/applications/   (sistema)\n"
                       "    update-desktop-database ~/.local/share/applications\n\n"
                       "También puedes incluirlo dentro del .deb/.rpm añadiéndolo en \"Archivos e "
                       "Instalación\" con destino usr/share/applications/."),
        tab);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    layout->addStretch(1);

    desktopBox->setEnabled(false);
    connect(m_desktopCheck, &QCheckBox::toggled, desktopBox, &QGroupBox::setEnabled);
}

void MainWindow::setupFormatTab(QWidget *tab)
{
    QVBoxLayout *layout = new QVBoxLayout(tab);

    QGroupBox *formatBox = new QGroupBox(QStringLiteral("Formatos de salida"), tab);
    QVBoxLayout *formatLayout = new QVBoxLayout(formatBox);
    m_debCheck = new QCheckBox(QStringLiteral("Paquete Debian (.deb) — requiere dpkg-deb"), formatBox);
    m_rpmCheck = new QCheckBox(QStringLiteral("Paquete Red Hat (.rpm) — requiere rpmbuild"), formatBox);
    m_archLinuxCheck = new QCheckBox(QStringLiteral("Paquete Arch Linux (.pkg.tar.zst) — requiere makepkg"), formatBox);
    m_debCheck->setChecked(true);
    formatLayout->addWidget(m_debCheck);
    formatLayout->addWidget(m_rpmCheck);
    formatLayout->addWidget(m_archLinuxCheck);
    layout->addWidget(formatBox);

    QGroupBox *outputBox = new QGroupBox(QStringLiteral("Directorio de salida"), tab);
    QHBoxLayout *outputLayout = new QHBoxLayout(outputBox);
    m_outputDirEdit = new QLineEdit(QDir::homePath(), outputBox);
    QPushButton *browseButton = new QPushButton(QStringLiteral("Examinar..."), outputBox);
    connect(browseButton, &QPushButton::clicked, this, &MainWindow::browseOutputDirectory);
    outputLayout->addWidget(m_outputDirEdit, 1);
    outputLayout->addWidget(browseButton);
    layout->addWidget(outputBox);

    QGroupBox *toolsBox = new QGroupBox(QStringLiteral("Herramientas del sistema"), tab);
    QVBoxLayout *toolsLayout = new QVBoxLayout(toolsBox);
    QLabel *toolsHint = new QLabel(
        QStringLiteral("La aplicación detecta automáticamente las herramientas instaladas y deshabilita "
                       "los formatos no disponibles. Puedes instalarlas con el botón de abajo "
                       "(pedirá permisos de administrador) o ejecutando el script\n\n"
                       "    sudo bash install-dependencies.sh"),
        toolsBox);
    toolsHint->setWordWrap(true);
    QPushButton *installToolsButton = new QPushButton(QStringLiteral("Instalar dependencias faltantes..."), toolsBox);
    connect(installToolsButton, &QPushButton::clicked, this, &MainWindow::installMissingTools);
    toolsLayout->addWidget(toolsHint);
    toolsLayout->addWidget(installToolsButton);
    layout->addWidget(toolsBox);

    layout->addStretch(1);
}

void MainWindow::setupLogTab(QWidget *tab)
{
    QVBoxLayout *layout = new QVBoxLayout(tab);

    m_logEdit = new QTextEdit(tab);
    m_logEdit->setReadOnly(true);
    m_logEdit->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    layout->addWidget(m_logEdit, 1);

    m_progressBar = new QProgressBar(tab);
    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(0);
    m_progressBar->setTextVisible(true);
    m_progressBar->hide();
    layout->addWidget(m_progressBar);

    QHBoxLayout *buttons = new QHBoxLayout;
    QPushButton *clearButton = new QPushButton(QStringLiteral("Limpiar log"), tab);
    connect(clearButton, &QPushButton::clicked, this, [this]() { m_logEdit->clear(); });

    m_generateButton = new QPushButton(QStringLiteral("Generar Paquetes"), tab);
    m_generateButton->setDefault(true);
    connect(m_generateButton, &QPushButton::clicked, this, &MainWindow::generatePackages);

    buttons->addWidget(clearButton);
    buttons->addStretch(1);
    buttons->addWidget(m_generateButton);
    layout->addLayout(buttons);
}

void MainWindow::checkDependencies()
{
    appendLog(QStringLiteral("=== Verificando herramientas del sistema ==="));

    const bool hasDpkg = PackageBuilder::toolAvailable(QStringLiteral("dpkg-deb"));
    m_debCheck->setEnabled(hasDpkg);
    if (!hasDpkg) {
        m_debCheck->setChecked(false);
        appendLog(QStringLiteral("Advertencia: \"dpkg-deb\" no encontrado. La opción .deb se ha deshabilitado."));
    }

    const bool hasRpmbuild = PackageBuilder::toolAvailable(QStringLiteral("rpmbuild"));
    m_rpmCheck->setEnabled(hasRpmbuild);
    if (!hasRpmbuild) {
        m_rpmCheck->setChecked(false);
        appendLog(QStringLiteral("Advertencia: \"rpmbuild\" no encontrado. La opción .rpm se ha deshabilitado."));
    }

    const bool hasMakepkg = PackageBuilder::toolAvailable(QStringLiteral("makepkg"));
    m_archLinuxCheck->setEnabled(hasMakepkg);
    if (!hasMakepkg) {
        m_archLinuxCheck->setChecked(false);
        appendLog(QStringLiteral("Advertencia: \"makepkg\" no encontrado. La opción Arch Linux se ha deshabilitado."));
    }

    appendLog(QStringLiteral("dpkg-deb: %1 | rpmbuild: %2 | makepkg: %3")
                  .arg(hasDpkg ? QStringLiteral("sí") : QStringLiteral("no"),
                       hasRpmbuild ? QStringLiteral("sí") : QStringLiteral("no"),
                       hasMakepkg ? QStringLiteral("sí") : QStringLiteral("no")));
}

void MainWindow::addFileToTree(const QString &source, const QString &target)
{
    QTreeWidgetItem *item = new QTreeWidgetItem(m_filesTree);
    item->setText(0, source);
    item->setData(0, Qt::UserRole, source);
    item->setText(1, target);
    item->setToolTip(0, source);
    item->setToolTip(1, target);
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    m_filesTree->addTopLevelItem(item);
}

void MainWindow::addFile()
{
    const QStringList files = QFileDialog::getOpenFileNames(
        this, QStringLiteral("Seleccionar archivos"), QDir::homePath());
    if (files.isEmpty())
        return;

    const QString base = QInputDialog::getText(
        this, QStringLiteral("Ruta de destino"),
        QStringLiteral("Ruta relativa de destino (por ejemplo, \"usr/bin\"):"),
        QLineEdit::Normal, QStringLiteral("usr/bin"));
    if (base.isEmpty())
        return;

    for (const QString &file : files)
        addFileToTree(file, QDir::cleanPath(base + QLatin1Char('/') + QFileInfo(file).fileName()));
}

void MainWindow::addFolder()
{
    const QString folder = QFileDialog::getExistingDirectory(
        this, QStringLiteral("Seleccionar carpeta"), QDir::homePath());
    if (folder.isEmpty())
        return;

    const QString base = QInputDialog::getText(
        this, QStringLiteral("Ruta de destino"),
        QStringLiteral("Ruta relativa base de destino (por ejemplo, \"usr/share/myapp\"):"),
        QLineEdit::Normal, QStringLiteral("usr/share/") + QFileInfo(folder).fileName());
    if (base.isEmpty())
        return;

    QDirIterator it(folder, QDir::Files | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        const QString file = it.next();
        const QString rel = QDir(folder).relativeFilePath(file);
        addFileToTree(file, QDir::cleanPath(base + QLatin1Char('/') + rel));
    }
}

void MainWindow::removeSelected()
{
    const QList<QTreeWidgetItem *> items = m_filesTree->selectedItems();
    for (QTreeWidgetItem *item : items)
        delete item;
}

void MainWindow::addDependency()
{
    const QString dep = m_depPresetCombo->currentText().trimmed();
    if (dep.isEmpty())
        return;

    const QList<QListWidgetItem *> matches = m_dependenciesList->findItems(dep, Qt::MatchExactly);
    if (!matches.isEmpty()) {
        QMessageBox::information(this, QStringLiteral("Dependencia duplicada"),
                                 QStringLiteral("La dependencia \"%1\" ya está en la lista.").arg(dep));
        return;
    }

    m_dependenciesList->addItem(dep);
    m_depPresetCombo->setCurrentText(QString());
}

void MainWindow::removeSelectedDependency()
{
    const QList<QListWidgetItem *> items = m_dependenciesList->selectedItems();
    for (QListWidgetItem *item : items)
        delete item;
}

void MainWindow::importDependencies()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, QStringLiteral("Importar dependencias"), QDir::homePath(),
        QStringLiteral("Archivos de texto (*.txt);;Todos los archivos (*)"));
    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("Importar dependencias"),
                             QStringLiteral("No se pudo abrir el archivo:\n%1").arg(filePath));
        return;
    }

    int added = 0;
    int skipped = 0;
    while (!file.atEnd()) {
        const QString line = QString::fromUtf8(file.readLine()).trimmed();
        if (line.isEmpty())
            continue;
        if (m_dependenciesList->findItems(line, Qt::MatchExactly).isEmpty()) {
            m_dependenciesList->addItem(line);
            ++added;
        } else {
            ++skipped;
        }
    }

    appendLog(QStringLiteral("Importadas %1 dependencias desde %2%3")
                  .arg(added)
                  .arg(filePath)
                  .arg(skipped > 0 ? QStringLiteral(" (%1 duplicadas omitidas)").arg(skipped)
                                   : QString()));
}

void MainWindow::exportDependencies()
{
    if (m_dependenciesList->count() == 0) {
        QMessageBox::information(this, QStringLiteral("Exportar dependencias"),
                                 QStringLiteral("La lista de dependencias está vacía."));
        return;
    }

    const QString filePath = QFileDialog::getSaveFileName(
        this, QStringLiteral("Exportar dependencias"),
        QDir::homePath() + QStringLiteral("/dependencias.txt"),
        QStringLiteral("Archivos de texto (*.txt);;Todos los archivos (*)"));
    if (filePath.isEmpty())
        return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, QStringLiteral("Exportar dependencias"),
                             QStringLiteral("No se pudo escribir el archivo:\n%1").arg(filePath));
        return;
    }

    QTextStream out(&file);
    for (int i = 0; i < m_dependenciesList->count(); ++i)
        out << m_dependenciesList->item(i)->text() << '\n';
    out.flush();

    appendLog(QStringLiteral("Exportadas %1 dependencias a %2")
                  .arg(m_dependenciesList->count())
                  .arg(filePath));
}

QString MainWindow::installDependenciesScript() const
{
    return QStringLiteral(
        R"SCRIPT(#!/usr/bin/env bash
set -e

echo "=== Instalando herramientas para generar paquetes (.deb, .rpm, Arch Linux) ==="

if [ "$(id -u)" -ne 0 ]; then
    echo "Este script debe ejecutarse como root (usa: sudo bash install-dependencies.sh)"
    exit 1
fi

DISTRO_ID=$(grep -E '^ID=' /etc/os-release | cut -d= -f2 | tr -d '"')
DISTRO_LIKE=$(grep -E '^ID_LIKE=' /etc/os-release | cut -d= -f2 | tr -d '"' || true)

is() {
    case "$DISTRO_ID $DISTRO_LIKE" in
        *"$1"*) return 0 ;;
    esac
    return 1
}

has() {
    command -v "$1" >/dev/null 2>&1
}

install_pkg() {
    if has apt-get; then
        export DEBIAN_FRONTEND=noninteractive
        apt-get update -y
        apt-get install -y "$@"
    elif has dnf; then
        dnf install -y "$@"
    elif has yum; then
        yum install -y "$@"
    elif has pacman; then
        pacman -Sy --noconfirm --needed "$@"
    elif has zypper; then
        zypper --non-interactive install "$@"
    else
        echo "AVISO: no se reconoce el gestor de paquetes. Instala manualmente: $*"
        return 1
    fi
}

download() {
    local url="$1"
    local out="$2"
    if has curl; then
        curl -fL -o "$out" "$url"
    elif has wget; then
        wget -q -O "$out" "$url"
    else
        echo "Se necesita curl o wget para las descargas."
        return 1
    fi
}

echo "--- dpkg-deb (.deb) ---"
if ! has dpkg-deb; then
    install_pkg dpkg-dev || install_pkg dpkg
else
    echo "dpkg-deb ya está instalado."
fi

echo "--- rpmbuild (.rpm) ---"
if ! has rpmbuild; then
    install_pkg rpm-build || install_pkg rpm || install_pkg rpm-tools
else
    echo "rpmbuild ya está instalado."
fi

echo "--- makepkg (Arch Linux) ---"
if ! has makepkg; then
    if is arch manjaro EndeavourOS garuda; then
        echo "makepkg debería venir con el sistema base. Instalando base-devel..."
        install_pkg base-devel
    elif is fedora; then
        echo "makepkg no está disponible directamente en Fedora. Instalando pacman desde AUR o usa un contenedor Arch."
        echo "AVISO: makepkg requiere un sistema Arch Linux o derivado."
    elif is debian ubuntu; then
        echo "makepkg no está disponible directamente en Debian/Ubuntu."
        echo "Puedes usar un contenedor Arch: docker run -it archlinux base-devel"
    else
        echo "makepkg requiere un sistema Arch Linux o derivado."
    fi
else
    echo "makepkg ya está instalado."
fi

echo
echo "=== Verificación final ==="
for t in dpkg-deb rpmbuild makepkg; do
    if has "$t"; then
        echo "OK    $t"
    else
        echo "FALTA $t"
    fi
done
)SCRIPT");
}

void MainWindow::installMissingTools()
{
    const QString scriptPath = QDir::temp().filePath(QStringLiteral("paquete-install-dependencies.sh"));
    {
        QFile file(scriptPath);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QMessageBox::warning(this, QStringLiteral("Instalación de dependencias"),
                                 QStringLiteral("No se pudo crear el script temporal de instalación."));
            return;
        }
        file.write(installDependenciesScript().toUtf8());
        file.close();
    }
    QFile::setPermissions(scriptPath, QFile::permissions(scriptPath) | QFileDevice::ExeOwner);

    const QString pkexec = QStandardPaths::findExecutable(QStringLiteral("pkexec"));
    if (!pkexec.isEmpty()) {
        m_generateButton->setEnabled(false);
        appendLog(QStringLiteral("=== Instalando herramientas faltantes (se pedirá autenticación) ==="));

        QProcess process;
        process.setProcessChannelMode(QProcess::SeparateChannels);
        auto forward = [this](QProcess *p, bool isError) {
            const QByteArray data = isError ? p->readAllStandardError() : p->readAllStandardOutput();
            if (!data.isEmpty())
                appendLog(QString::fromUtf8(data));
        };
        QObject::connect(&process, &QProcess::readyReadStandardOutput, &process,
                         [&process, &forward]() { forward(&process, false); });
        QObject::connect(&process, &QProcess::readyReadStandardError, &process,
                         [&process, &forward]() { forward(&process, true); });

        appendLog(QStringLiteral("$ %1 bash %2").arg(pkexec, scriptPath));
        process.start(pkexec, { QStringLiteral("bash"), scriptPath });
        if (!process.waitForStarted()) {
            appendLog(QStringLiteral("Error: no se pudo iniciar pkexec."));
            m_generateButton->setEnabled(true);
            return;
        }
        process.waitForFinished(-1);
        forward(&process, false);
        forward(&process, true);

        const bool ok = process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
        appendLog(ok ? QStringLiteral("=== Instalación finalizada ===")
                     : QStringLiteral("=== La instalación terminó con errores ==="));
        m_generateButton->setEnabled(true);
        checkDependencies();
        QFile::remove(scriptPath);
        return;
    }

    const QString terminal = QStandardPaths::findExecutable(QStringLiteral("x-terminal-emulator"));
    if (!terminal.isEmpty()) {
        QProcess::startDetached(terminal, { QStringLiteral("-e"), QStringLiteral("sudo"),
                                            QStringLiteral("bash"), scriptPath });
        QMessageBox::information(this, QStringLiteral("Instalación de dependencias"),
                                 QStringLiteral("Se abrió una terminal. Escribe la contraseña cuando se pida.\n"
                                                "Cuando termine, la aplicación detectará las herramientas "
                                                "automáticamente."));
        return;
    }

    QMessageBox::information(this, QStringLiteral("Instalación de dependencias"),
                             QStringLiteral("No se pudo lanzar la instalación automática.\n"
                                            "Ejecuta manualmente como administrador:\n\n"
                                            "sudo bash %1").arg(scriptPath));
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, QStringLiteral("Acerca de Stellar Paquete"),
                       QStringLiteral("<h3>Stellar Paquete %1</h3>"
                                      "<p>Herramienta de escritorio para crear paquetes de software "
                                       "<b>.deb</b>, <b>.rpm</b> y <b>Arch Linux (.pkg.tar.zst)</b>.</p>"
                                      "<p>Permite definir los metadatos del paquete, mapear los archivos "
                                      "de instalación, declarar las dependencias que debe resolver el "
                                      "sistema y generar los paquetes directamente desde la interfaz.</p>"
                                      "<p>Tecnología: C++17, Qt %2 y Qt Widgets.</p>")
                           .arg(QApplication::applicationVersion(),
                                QString::fromLatin1(qVersion())));
}

void MainWindow::browseOutputDirectory()
{
    const QString dir = QFileDialog::getExistingDirectory(
        this, QStringLiteral("Seleccionar directorio de salida"), m_outputDirEdit->text());
    if (!dir.isEmpty())
        m_outputDirEdit->setText(dir);
}

PackageMetadata MainWindow::collectMetadata() const
{
    PackageMetadata meta;
    meta.name = m_nameEdit->text().trimmed();
    meta.version = m_versionEdit->text().trimmed();
    meta.architecture = m_archCombo->currentText();
    meta.maintainer = m_maintainerEdit->text().trimmed();
    meta.license = m_licenseCombo->currentText().trimmed();
    meta.shortDescription = m_shortDescEdit->text().trimmed();
    meta.longDescription = m_longDescEdit->toPlainText().trimmed();
    meta.outputDirectory = m_outputDirEdit->text().trimmed();

    if (m_debCheck->isChecked())
        meta.formats << QStringLiteral("deb");
    if (m_rpmCheck->isChecked())
        meta.formats << QStringLiteral("rpm");
    if (m_archLinuxCheck->isChecked())
        meta.formats << QStringLiteral("arch");

    meta.desktop.generate = m_desktopCheck->isChecked();
    meta.desktop.name = m_desktopNameEdit->text().trimmed();
    meta.desktop.comment = m_desktopCommentEdit->text().trimmed();
    meta.desktop.exec = m_desktopExecEdit->text().trimmed();
    meta.desktop.icon = m_desktopIconEdit->text().trimmed();
    QStringList checkedCategories;
    for (int i = 0; i < m_desktopCategoriesList->count(); ++i) {
        QListWidgetItem *item = m_desktopCategoriesList->item(i);
        if (item->checkState() == Qt::Checked)
            checkedCategories.append(item->text());
    }
    meta.desktop.categories = checkedCategories.join(QLatin1Char(';'));
    if (!meta.desktop.categories.isEmpty())
        meta.desktop.categories += QLatin1Char(';');
    meta.desktop.mimeTypes = m_desktopMimeEdit->text().trimmed();
    meta.desktop.terminal = m_desktopTerminalCheck->isChecked();
    meta.desktop.startupNotify = m_desktopStartupCheck->isChecked();
    if (meta.desktop.generate)
        meta.formats << QStringLiteral("desktop");

    for (int i = 0; i < m_dependenciesList->count(); ++i) {
        const QString dep = m_dependenciesList->item(i)->text().trimmed();
        if (!dep.isEmpty())
            meta.dependencies << dep;
    }

    for (int i = 0; i < m_filesTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem *item = m_filesTree->topLevelItem(i);
        const QString source = item->data(0, Qt::UserRole).toString();
        const QString target = item->text(1).trimmed();
        if (!source.isEmpty() && !target.isEmpty())
            meta.files.append({ source, target });
    }
    return meta;
}

bool MainWindow::validateMetadata(const PackageMetadata &meta, QString *error) const
{
    static const QRegularExpression nameRe(QStringLiteral("^[a-zA-Z0-9][a-zA-Z0-9+.-]*$"));
    static const QRegularExpression versionRe(QStringLiteral("^[0-9][0-9A-Za-z.+~:-]*$"));

    if (meta.name.isEmpty() || !nameRe.match(meta.name).hasMatch()) {
        *error = QStringLiteral("El nombre del paquete es inválido (solo letras, números, '+', '.', '-').");
        return false;
    }
    if (meta.version.isEmpty() || !versionRe.match(meta.version).hasMatch()) {
        *error = QStringLiteral("La versión es inválida (debe empezar por un número).");
        return false;
    }
    if (meta.maintainer.isEmpty()) {
        *error = QStringLiteral("Debe indicar el mantenedor en el formato \"Nombre <email>\".");
        return false;
    }
    if (meta.license.isEmpty()) {
        *error = QStringLiteral("Debe indicar la licencia.");
        return false;
    }
    if (meta.shortDescription.isEmpty()) {
        *error = QStringLiteral("Debe indicar una descripción corta.");
        return false;
    }
    if (meta.longDescription.isEmpty()) {
        *error = QStringLiteral("Debe indicar una descripción larga.");
        return false;
    }
    if (meta.formats.isEmpty()) {
        *error = QStringLiteral("Seleccione al menos un formato de salida.");
        return false;
    }
    if (meta.formats.contains(QStringLiteral("deb"))) {
        static const QRegularExpression debNameRe(QStringLiteral("^[a-z0-9][a-z0-9+.-]*$"));
        if (!debNameRe.match(meta.name).hasMatch()) {
            *error = QStringLiteral("El nombre del paquete no es válido para .deb: debe estar en "
                                    "minúsculas y solo puede contener letras, números, '+', '.', '-' "
                                    "(sin guiones bajos).");
            return false;
        }
    }
    if (meta.files.isEmpty()) {
        *error = QStringLiteral("Añada al menos un archivo a la lista de instalación.");
        return false;
    }
    if (meta.outputDirectory.isEmpty()) {
        *error = QStringLiteral("Indique el directorio de salida.");
        return false;
    }
    if (!QDir(meta.outputDirectory).exists()) {
        *error = QStringLiteral("El directorio de salida no existe: %1").arg(meta.outputDirectory);
        return false;
    }
    return true;
}

void MainWindow::generatePackages()
{
    QString error;
    const PackageMetadata meta = collectMetadata();
    if (!validateMetadata(meta, &error)) {
        QMessageBox::warning(this, QStringLiteral("Datos incompletos"), error);
        return;
    }

    m_generateButton->setEnabled(false);
    appendLog(QStringLiteral("=== Generando paquetes para %1 %2 ===").arg(meta.name, meta.version));

    m_progressBar->setRange(0, 0);
    m_progressBar->setFormat(QStringLiteral("Generando paquetes..."));
    m_progressBar->show();

    bool anySucceeded = false;
    for (const QString &format : meta.formats) {
        PackageBuilder *builder = PackageBuilder::createBuilder(format, this);
        if (!builder)
            continue;

        m_progressBar->setFormat(QStringLiteral("Generando %1...").arg(format.toUpper()));
        connect(builder, &PackageBuilder::logMessage, this, &MainWindow::appendLog);
        const bool ok = builder->build(meta, meta.outputDirectory);
        if (ok)
            anySucceeded = true;
        disconnect(builder, &PackageBuilder::logMessage, this, &MainWindow::appendLog);
    }

    appendLog(anySucceeded ? QStringLiteral("=== Proceso finalizado ===")
                           : QStringLiteral("=== Proceso finalizado con errores ==="));

    m_progressBar->setRange(0, 100);
    m_progressBar->setValue(100);
    m_progressBar->setFormat(QStringLiteral("Proceso finalizado"));
    showBuildNotification(anySucceeded);
    QTimer::singleShot(2000, m_progressBar, [this]() {
        m_progressBar->hide();
        m_progressBar->reset();
    });

    m_generateButton->setEnabled(true);
}

void MainWindow::showBuildNotification(bool success)
{
    QApplication::alert(this);

    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;

    if (!m_trayIcon) {
        m_trayIcon = new QSystemTrayIcon(this);
        m_trayIcon->setIcon(windowIcon());
        m_trayIcon->setToolTip(QStringLiteral("Stellar Paquete"));
        m_trayIcon->show();
    }

    m_trayIcon->showMessage(
        QStringLiteral("Stellar Paquete"),
        success ? QStringLiteral("El paquete se generó correctamente.")
                : QStringLiteral("El proceso terminó con errores."),
        QSystemTrayIcon::Information, 5000);
}

void MainWindow::appendLog(const QString &message)
{
    m_logEdit->moveCursor(QTextCursor::End);
    m_logEdit->insertPlainText(message);
    if (!message.endsWith(QLatin1Char('\n')))
        m_logEdit->insertPlainText(QStringLiteral("\n"));
    m_logEdit->moveCursor(QTextCursor::End);
    m_logEdit->ensureCursorVisible();
}
