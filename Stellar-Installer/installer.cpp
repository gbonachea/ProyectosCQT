#include <QApplication>
#include <QWizard>
#include <QWizardPage>
#include <QFile>
#include <QDir>
#include <QCoreApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QTextStream>
#include <QVBoxLayout>
#include <QProcess>
#include <QMessageBox>

class InstallerWizard : public QWizard {
    Q_OBJECT
public:
    InstallerWizard(QWidget *parent = nullptr) : QWizard(parent) {
        setWindowTitle("Stellar Instalador");
        setWizardStyle(QWizard::ModernStyle);
        setPixmap(QWizard::LogoPixmap, QPixmap("../main/icon.png"));

        introPage = createIntroPage();
        authPage = createAuthPage();
        progressPage = createProgressPage();
        conclusionPage = createConclusionPage();

        addPage(introPage);
        addPage(authPage);
        addPage(progressPage);
        addPage(conclusionPage);

        // when the wizard changes page, start install when we hit progress page
        connect(this, &QWizard::currentIdChanged, this, &InstallerWizard::onPageChanged);
    }

private:
    QWizardPage *introPage;
    QWizardPage *authPage;
    QWizardPage *progressPage;
    QWizardPage *conclusionPage;

private:
    QWizardPage *createIntroPage() {
        QWizardPage *page = new QWizardPage;
        page->setTitle("Bienvenido");
        QString info = loadInfo("../main/appinfo.txt");
        QLabel *label = new QLabel(info);
        label->setWordWrap(true);
        QLabel *iconLabel = new QLabel;
        iconLabel->setPixmap(QPixmap("../main/icon.png").scaled(64, 64, Qt::KeepAspectRatio));
        QVBoxLayout *lay = new QVBoxLayout;
        lay->addWidget(iconLabel, 0, Qt::AlignCenter);
        lay->addWidget(label);
        page->setLayout(lay);
        return page;
    }

    QWizardPage *createAuthPage() {
        QWizardPage *page = new QWizardPage;
        page->setTitle("Permisos de administrador");
        QLabel *label = new QLabel("Introduzca la contraseña de sudo para copiar archivos y ejecutar dependencias:");
        label->setWordWrap(true);
        pwdEdit = new QLineEdit;
        pwdEdit->setEchoMode(QLineEdit::Password);
        pwdEdit->setPlaceholderText("Contraseña sudo");
        QVBoxLayout *lay = new QVBoxLayout;
        lay->addWidget(label);
        lay->addWidget(pwdEdit);
        page->setLayout(lay);
        return page;
    }

    QWizardPage *createProgressPage() {
        QWizardPage *page = new QWizardPage;
        page->setTitle("Instalando");
        progress = new QProgressBar;
        progress->setRange(0, 100);
        progress->setValue(0);
        progress->setTextVisible(true);
        QVBoxLayout *lay = new QVBoxLayout;
        lay->addWidget(progress);
        page->setLayout(lay);
        return page;
    }

    QWizardPage *createConclusionPage() {
        QWizardPage *page = new QWizardPage;
        page->setTitle("Finalizado");
        QLabel *label = new QLabel("La instalación ha terminado. Pulse Finalizar para cerrar.");
        label->setWordWrap(true);
        QVBoxLayout *lay = new QVBoxLayout;
        lay->addWidget(label);
        page->setLayout(lay);
        return page;
    }


private:
    QString loadInfo(const QString &path) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return QString("Aplicación desconocida");
        QTextStream in(&f);
        return in.readAll();
    }

private slots:
    void onPageChanged(int id) {
        // progress page has id 2 (zero-based order of addPage calls)
        if (id == 2) {
            startInstallation();
        }
    }

    void startInstallation() {
        sudoPassword = pwdEdit->text();  // save password for later use

        // disable navigation while install is running
        if (auto *b = button(QWizard::NextButton)) b->setEnabled(false);
        if (auto *b = button(QWizard::BackButton)) b->setEnabled(false);
        if (auto *b = button(QWizard::CancelButton)) b->setEnabled(false);

        progress->setRange(0, 0);
        progress->setValue(0);

        proc = new QProcess(this);
        connect(proc, &QProcess::readyReadStandardOutput,
                this, &InstallerWizard::readStdout);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &InstallerWizard::copyFinished);

        // determine base directory where the binary is located
        QString base = QCoreApplication::applicationDirPath();
        // assume data/ and main/ are sibling directories of build/ or the binary
        QString dataPath = QDir(base).absoluteFilePath("../data");
        QString runScript = QDir(base).absoluteFilePath("../main/run.sh");

        QStringList args;
        args << "-S" << "bash" << "-c"
             << QString("echo PROGRESS:10; cp -r %1/* / && echo PROGRESS:50").arg(dataPath);
        proc->start("sudo", args);
        if (!proc->waitForStarted()) {
            QMessageBox::critical(this, "Error", "No se pudo arrancar sudo");
            reject();
            return;
        }
        proc->write(sudoPassword.toUtf8() + "\n");
    }

    void readStdout() {
        QByteArray out = proc->readAllStandardOutput();
        for (const QByteArray &line : out.split('\n')) {
            if (line.startsWith("PROGRESS:")) {
                bool ok=false;
                int val = line.mid(9).toInt(&ok);
                if (ok) {
                    // si el valor se encuentra en el rango 0-50 lo dejamos, si es mayor lo mapeamos
                    if (val <= 50) {
                        progress->setRange(0, 50);
                        progress->setValue(val);
                    } else {
                        // valores emitidos por el script pueden ir 0..100
                        progress->setRange(0, 100);
                        progress->setValue(val);
                    }
                }
            }
        }
    }

    void copyFinished(int code, QProcess::ExitStatus status) {
        if (status != QProcess::NormalExit || code != 0) {
            QMessageBox::warning(this, "Instalador", "Error al copiar archivos");
            reject();
            return;
        }
        // Ahora ejecutar el script run.sh (el propio contenido del script pedirá sudo de nuevo)
        disconnect(proc, nullptr, this, nullptr);
        connect(proc, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                this, &InstallerWizard::installFinished);

        progress->setRange(0, 0); // de nuevo indeterminado

        QString base = QCoreApplication::applicationDirPath();
        QString runScript = QDir(base).absoluteFilePath("../main/run.sh");

        QStringList args;
        args << "-S" << "bash" << runScript; // execute script with sudo
        proc->start("sudo", args);
        if (!proc->waitForStarted()) {
            QMessageBox::critical(this, "Error", "No se pudo ejecutar el script de instalación");
            reject();
            return;
        }
        proc->write(sudoPassword.toUtf8() + "\n");
    }

    void installFinished(int code, QProcess::ExitStatus status) {
        progress->setRange(0, 100);
        progress->setValue(100);

        // check for failed packages
        QString failedMsg;
        QFile failedFile("/tmp/failed_packages");
        if (failedFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QTextStream in(&failedFile);
            QStringList failedPackages;
            while (!in.atEnd()) {
                QString line = in.readLine().trimmed();
                if (!line.isEmpty()) {
                    failedPackages.append(line);
                }
            }
            if (!failedPackages.isEmpty()) {
                failedMsg = "\n\nPaquetes que no se pudieron instalar:\n" + failedPackages.join("\n");
            }
        }

        if (status != QProcess::NormalExit || code != 0) {
            QMessageBox::warning(this, "Instalador",
                                 QString("La instalación terminó con errores.\nCódigo: %1, Estado: %2%3")
                                 .arg(code).arg(status == QProcess::NormalExit ? "Normal" : "Crash").arg(failedMsg));
        } else {
            QMessageBox::information(this, "Instalador", QString("Instalación completada.%1").arg(failedMsg));
        }
        // move to conclusion page automatically and re-enable buttons
        next();
        if (auto *b = button(QWizard::NextButton)) b->setEnabled(true);
        if (auto *b = button(QWizard::BackButton)) b->setEnabled(true);
        if (auto *b = button(QWizard::CancelButton)) b->setEnabled(true);
    }

private:
    QLineEdit *pwdEdit;
    QProgressBar *progress;
    QProcess *proc;
    QString sudoPassword;
};

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon("../installer.png"));
    InstallerWizard wiz;
    wiz.show();
    return app.exec();
}

#include "installer.moc"
