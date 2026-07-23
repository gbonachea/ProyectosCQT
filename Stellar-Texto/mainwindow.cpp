#include "mainwindow.h"
#include "codeeditor.h"
#include "syntaxhighlighter.h"
#include "finddialog.h"

#include <QMenuBar>
#include <QMenu>
#include <QToolBar>
#include <QStatusBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QTextStream>
#include <QApplication>
#include <QShortcut>
#include <QFileInfo>
#include <QIcon>
#include <QtPrintSupport/QPrinter>
#include <QtPrintSupport/QPrintDialog>
#include <QFontDialog>
#include <QColorDialog>
#include <QPushButton>
#include <QPixmap>
#include <QPainter>
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QComboBox>
#include <QToolButton>
#include <QFontDatabase>
#include <QFrame>
#include <QMenu>
#include <QAction>
#include <QWidgetAction>
#include <QSizePolicy>
#include <QSettings>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_editorFont = QFont("Monospace", 11);
    m_editorFont.setStyleHint(QFont::Monospace);
    m_textColor = QColor("#2E3436");
    m_bgColor = QColor("#FFFFFF");
    m_underlineColor = QColor("#FF0000");
    m_alignment = Qt::AlignLeft;
    m_lastDir = QDir::homePath();
    setWindowIcon(QIcon(":/icons/stellartexto.png"));

    m_tabWidget = new QTabWidget(this);
    m_tabWidget->setTabsClosable(true);
    m_tabWidget->setMovable(true);
    m_tabWidget->setDocumentMode(false);
    setCentralWidget(m_tabWidget);

    setupMenuBar();
    setupToolBar();
    setupStatusBar();
    setupShortcuts();

    connect(m_tabWidget, &QTabWidget::currentChanged, this, &MainWindow::onTabChanged);
    connect(m_tabWidget, &QTabWidget::tabCloseRequested, this, &MainWindow::closeCurrentTab);

    QSettings settings;
    if (settings.contains("window/geometry"))
        restoreGeometry(settings.value("window/geometry").toByteArray());
    else
        resize(1000, 700);

    if (settings.contains("editor/textColor"))
        m_textColor = settings.value("editor/textColor").value<QColor>();
    if (settings.contains("editor/bgColor"))
        m_bgColor = settings.value("editor/bgColor").value<QColor>();
    if (settings.contains("editor/underlineColor"))
        m_underlineColor = settings.value("editor/underlineColor").value<QColor>();
    if (settings.contains("editor/alignment"))
        m_alignment = settings.value("editor/alignment").toInt();

    newFile();
}

CodeEditor *MainWindow::currentEditor() const
{
    return qobject_cast<CodeEditor *>(m_tabWidget->currentWidget());
}

int MainWindow::addEditorTab(const QString &title)
{
    auto *editor = new CodeEditor();
    editor->setFont(m_editorFont);
    editor->setStyleSheet(
        QString("QPlainTextEdit {"
                "  background-color: %1;"
                "  color: %2;"
                "  selection-background-color: #B4D7FF;"
                "}")
        .arg(m_bgColor.name())
        .arg(m_textColor.name())
    );

    auto *highlighter = new SyntaxHighlighter(editor->document());

    connect(editor, &CodeEditor::textChanged, this, [this]() {
        updateTitle();
        updateStatusBar();
    });
    connect(editor, &CodeEditor::cursorPositionChanged, this, &MainWindow::updateStatusBar);

    int index = m_tabWidget->addTab(editor, title);
    m_tabWidget->setCurrentIndex(index);

    auto *closeBtn = new QPushButton();
    closeBtn->setFixedSize(18, 18);
    closeBtn->setFlat(true);
    closeBtn->setCursor(Qt::ArrowCursor);
    closeBtn->setToolTip("Cerrar pestaña");
    QPixmap cx(14, 14);
    cx.fill(Qt::transparent);
    QPainter xp(&cx);
    xp.setRenderHint(QPainter::Antialiasing);
    xp.setPen(QPen(QColor("#888888"), 1.5));
    xp.drawLine(3, 3, 11, 11);
    xp.drawLine(11, 3, 3, 11);
    xp.end();
    closeBtn->setIcon(QIcon(cx));
    closeBtn->setIconSize(QSize(14, 14));
    closeBtn->setStyleSheet(
        "QPushButton { background: transparent; border: none; border-radius: 3px; }"
        "QPushButton:hover { background: #C84C4C; }"
    );
    connect(closeBtn, &QPushButton::clicked, this, [this, editor]() {
        int idx = m_tabWidget->indexOf(editor);
        if (idx >= 0) closeCurrentTab(idx);
    });
    m_tabWidget->tabBar()->setTabButton(index, QTabBar::RightSide, closeBtn);

    return index;
}

void MainWindow::setupMenuBar()
{
    QMenu *fileMenu = menuBar()->addMenu("&Archivo");

    fileMenu->addAction("&Nuevo", QKeySequence::New, this, &MainWindow::newFile);
    fileMenu->addAction("&Abrir...", QKeySequence::Open, this, &MainWindow::openFile);
    fileMenu->addSeparator();
    fileMenu->addAction("&Guardar", QKeySequence::Save, this, [this]() { saveFile(); });
    fileMenu->addAction("Guardar &como...", QKeySequence("Ctrl+Shift+S"), this, [this]() { saveFileAs(); });
    fileMenu->addSeparator();
    fileMenu->addAction("Cerrar pestaña", QKeySequence::Close, this, [this]() { closeCurrentTab(m_tabWidget->currentIndex()); });
    fileMenu->addSeparator();
    fileMenu->addAction("&Salir", QKeySequence::Quit, this, &QWidget::close);

    QMenu *editMenu = menuBar()->addMenu("&Editar");

    editMenu->addAction("&Deshacer", QKeySequence::Undo, this, &MainWindow::undo);
    editMenu->addAction("&Rehacer", QKeySequence::Redo, this, &MainWindow::redo);
    editMenu->addSeparator();
    editMenu->addAction("&Cortar", QKeySequence::Cut, this, &MainWindow::cut);
    editMenu->addAction("C&opiar", QKeySequence::Copy, this, &MainWindow::copy);
    editMenu->addAction("&Pegar", QKeySequence::Paste, this, &MainWindow::paste);
    editMenu->addSeparator();
    editMenu->addAction("&Seleccionar todo", QKeySequence::SelectAll, this, &MainWindow::selectAll);
    editMenu->addSeparator();
    editMenu->addAction("&Buscar y reemplazar...", QKeySequence::Find, this, &MainWindow::showFindDialog);

    QMenu *viewMenu = menuBar()->addMenu("&Ver");

    viewMenu->addAction("Acercar", QKeySequence::ZoomIn, this, &MainWindow::zoomIn);
    viewMenu->addAction("Alejar", QKeySequence::ZoomOut, this, &MainWindow::zoomOut);
    viewMenu->addAction("Restablecer zoom", QKeySequence("Ctrl+0"), this, &MainWindow::resetZoom);

    QMenu *helpMenu = menuBar()->addMenu("&Ayuda");
    helpMenu->addAction("&Acerca de...", this, &MainWindow::showAbout);
}

void MainWindow::setupToolBar()
{
    QToolBar *toolbar = addToolBar("Principal");
    toolbar->setMovable(false);
    toolbar->setIconSize(QSize(20, 20));
    toolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);

    auto addBtn = [&](const QString &iconFile, const QString &text, auto &&slot) {
        QAction *act = toolbar->addAction(QIcon(iconFile), text, this, std::forward<decltype(slot)>(slot));
        return act;
    };

    addBtn(":/icons/nuevo.png",   "Nuevo",    &MainWindow::newFile);
    addBtn(":/icons/guardar.png", "Guardar",  [this]() { saveFile(); });
    addBtn(":/icons/copiar.png",  "Copiar",   &MainWindow::copy);
    addBtn(":/icons/cortar.png",  "Cortar",   &MainWindow::cut);
    addBtn(":/icons/pegar.png",   "Pegar",    &MainWindow::paste);
    addBtn(":/icons/buscar.png",  "Buscar",   &MainWindow::showFindDialog);
    addBtn(":/icons/imprimir.png","Imprimir", &MainWindow::printFile);

    toolbar->addSeparator();

    addBtn(":/icons/rehacer.png", "Rehacer",  &MainWindow::redo);
    addBtn(":/icons/deshacer.png","Deshacer", &MainWindow::undo);

    auto *spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    toolbar->addWidget(spacer);

    auto *ajustesAct = toolbar->addAction(QIcon(":/icons/ajustes.png"), "", this, &MainWindow::showSettings);
    ajustesAct->setToolTip("Ajustes");
}

void MainWindow::setupStatusBar()
{
    m_statusCursor = new QLabel("Línea: 1  Columna: 1");
    m_statusFile = new QLabel("");

    statusBar()->addWidget(m_statusCursor);
    statusBar()->addPermanentWidget(m_statusFile);
}

void MainWindow::setupShortcuts()
{
    auto *findReplaceShortcut = new QShortcut(QKeySequence("Ctrl+H"), this);
    connect(findReplaceShortcut, &QShortcut::activated, this, &MainWindow::showFindDialog);
}

void MainWindow::newFile()
{
    addEditorTab();
    updateTitle();
    updateStatusBar();
}

void MainWindow::openFile()
{
    QString fileName = QFileDialog::getOpenFileName(this, "Abrir archivo", m_lastDir);
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "No se pudo abrir el archivo:\n" + file.errorString());
        return;
    }

    m_lastDir = QFileInfo(fileName).absolutePath();

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();

    int index = addEditorTab(QFileInfo(fileName).fileName());
    auto *editor = qobject_cast<CodeEditor *>(m_tabWidget->widget(index));
    editor->setPlainText(content);

    auto prop = editor->document()->property("fileName");
    editor->document()->setProperty("fileName", fileName);

    auto *highlighter = editor->findChild<SyntaxHighlighter *>();
    if (highlighter)
        highlighter->setFileExtension(getFileExtension(fileName));

    m_tabWidget->setTabToolTip(index, fileName);
    updateTitle();
    updateStatusBar();
}

void MainWindow::openFileFromPath(const QString &fileName)
{
    if (fileName.isEmpty())
        return;

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    m_lastDir = QFileInfo(fileName).absolutePath();

    QTextStream in(&file);
    in.setEncoding(QStringConverter::Utf8);
    QString content = in.readAll();
    file.close();

    int index = addEditorTab(QFileInfo(fileName).fileName());
    auto *editor = qobject_cast<CodeEditor *>(m_tabWidget->widget(index));
    editor->setPlainText(content);

    editor->document()->setProperty("fileName", fileName);

    auto *highlighter = editor->findChild<SyntaxHighlighter *>();
    if (highlighter)
        highlighter->setFileExtension(getFileExtension(fileName));

    m_tabWidget->setTabToolTip(index, fileName);
    updateTitle();
    updateStatusBar();
}

bool MainWindow::saveFile()
{
    auto *editor = currentEditor();
    if (!editor)
        return false;

    QString fileName = editor->document()->property("fileName").toString();
    if (fileName.isEmpty())
        return saveFileAs();

    return saveFile(editor, fileName);
}

bool MainWindow::saveFileAs()
{
    auto *editor = currentEditor();
    if (!editor)
        return false;

    QString fileName = QFileDialog::getSaveFileName(this, "Guardar como", m_lastDir);
    if (fileName.isEmpty())
        return false;

    m_lastDir = QFileInfo(fileName).absolutePath();
    return saveFile(editor, fileName);
}

bool MainWindow::saveFile(CodeEditor *editor, const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Error", "No se pudo guardar el archivo:\n" + file.errorString());
        return false;
    }

    QTextStream out(&file);
    out.setEncoding(QStringConverter::Utf8);
    out << editor->toPlainText();
    file.close();

    editor->document()->setProperty("fileName", fileName);
    editor->document()->setModified(false);

    int idx = m_tabWidget->currentIndex();
    m_tabWidget->setTabText(idx, QFileInfo(fileName).fileName());
    m_tabWidget->setTabToolTip(idx, fileName);

    auto *highlighter = editor->findChild<SyntaxHighlighter *>();
    if (highlighter)
        highlighter->setFileExtension(getFileExtension(fileName));

    updateTitle();
    return true;
}

void MainWindow::closeCurrentTab(int index)
{
    if (index < 0)
        return;

    auto *editor = qobject_cast<CodeEditor *>(m_tabWidget->widget(index));
    if (!editor)
        return;

    if (editor->document()->isModified()) {
        QString fileName = editor->document()->property("fileName").toString();
        if (fileName.isEmpty())
            fileName = "Sin título";

        auto ret = QMessageBox::question(
            this, "Archivo modificado",
            QString("¿Guardar los cambios en \"%1\"?").arg(fileName),
            QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
        );

        if (ret == QMessageBox::Save) {
            m_tabWidget->setCurrentIndex(index);
            if (!saveFile(editor, editor->document()->property("fileName").toString()))
                return;
        } else if (ret == QMessageBox::Cancel) {
            return;
        }
    }

    m_tabWidget->removeTab(index);
    if (m_tabWidget->count() == 0)
        newFile();
}

void MainWindow::undo()
{
    if (auto *ed = currentEditor()) ed->undo();
}

void MainWindow::redo()
{
    if (auto *ed = currentEditor()) ed->redo();
}

void MainWindow::cut()
{
    if (auto *ed = currentEditor()) ed->cut();
}

void MainWindow::copy()
{
    if (auto *ed = currentEditor()) ed->copy();
}

void MainWindow::paste()
{
    if (auto *ed = currentEditor()) ed->paste();
}

void MainWindow::selectAll()
{
    if (auto *ed = currentEditor()) ed->selectAll();
}

void MainWindow::showFindDialog()
{
    auto *editor = currentEditor();
    if (!editor)
        return;

    auto *dialog = new FindDialog(editor, this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();
}

void MainWindow::showAbout()
{
    QMessageBox::about(this, "Acerca de Stellar Texto",
        "<h2>Stellar Texto 1.0</h2>"
        "<p>Editor de texto minimalista y moderno.</p>"
        "<p>Desarrollado con Qt " QT_VERSION_STR " y C++.</p>"
        "<hr>"
        "<p><b>Características:</b></p>"
        "<ul>"
        "<li>Edición con pestañas</li>"
        "<li>Resaltado de sintaxis (C/C++, Python)</li>"
        "<li>Números de línea</li>"
        "<li>Buscar y reemplazar</li>"
        "<li>Impresión</li>"
        "</ul>"
    );
}

void MainWindow::printFile()
{
    auto *editor = currentEditor();
    if (!editor) return;

    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog dlg(&printer, this);
    if (dlg.exec() == QDialog::Accepted)
        editor->print(&printer);
}

void MainWindow::showSettings()
{
    auto *editor = currentEditor();
    if (!editor) return;

    QDialog dlg(this);
    dlg.setWindowTitle("Ajustes");
    dlg.setFixedSize(480, 320);

    auto *mainLayout = new QVBoxLayout(&dlg);

    auto applyStyle = [this]() {
        QString ss = QString("QPlainTextEdit {"
                             "  background-color: %1;"
                             "  color: %2;"
                             "  selection-background-color: #B4D7FF;"
                             "}")
                      .arg(m_bgColor.name())
                      .arg(m_textColor.name());
        for (int i = 0; i < m_tabWidget->count(); ++i) {
            auto *ed = qobject_cast<CodeEditor *>(m_tabWidget->widget(i));
            if (ed) {
                ed->setStyleSheet(ss);
                ed->setFont(m_editorFont);
            }
        }
    };

    auto *topRow = new QHBoxLayout();

    auto *fontCombo = new QComboBox();
    fontCombo->setEditable(true);
    fontCombo->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    QStringList families = QFontDatabase::families();
    families.sort();
    fontCombo->addItems(families);
    fontCombo->setCurrentText(m_editorFont.family());

    auto *sizeCombo = new QComboBox();
    QStringList sizes;
    for (int s = 6; s <= 72; ++s) sizes << QString::number(s);
    sizeCombo->addItems(sizes);
    sizeCombo->setCurrentText(QString::number(m_editorFont.pointSize()));

    topRow->addWidget(new QLabel("Fuente:"));
    topRow->addWidget(fontCombo, 1);
    topRow->addSpacing(10);
    topRow->addWidget(new QLabel("Tamaño:"));
    topRow->addWidget(sizeCombo);

    auto *boldBtn = new QToolButton();
    boldBtn->setText("N");
    boldBtn->setCheckable(true);
    boldBtn->setChecked(m_editorFont.bold());
    boldBtn->setToolTip("Negrita");
    boldBtn->setFont(QFont(boldBtn->font().family(), -1, QFont::Bold));
    boldBtn->setFixedSize(36, 30);

    auto *italicBtn = new QToolButton();
    italicBtn->setText("K");
    italicBtn->setCheckable(true);
    italicBtn->setChecked(m_editorFont.italic());
    italicBtn->setToolTip("Cursiva");
    QFont italicFont = italicBtn->font();
    italicFont.setItalic(true);
    italicBtn->setFont(italicFont);
    italicBtn->setFixedSize(36, 30);

    auto *underlineBtn = new QToolButton();
    underlineBtn->setText("S");
    underlineBtn->setCheckable(true);
    underlineBtn->setChecked(m_editorFont.underline());
    underlineBtn->setToolTip("Subrayado");
    QFont underFont = underlineBtn->font();
    underFont.setUnderline(true);
    underlineBtn->setFont(underFont);
    underlineBtn->setFixedSize(36, 30);

    auto *styleGroup = new QHBoxLayout();
    styleGroup->addWidget(boldBtn);
    styleGroup->addWidget(italicBtn);
    styleGroup->addWidget(underlineBtn);

    auto *alignLeftBtn = new QToolButton();
    alignLeftBtn->setText("≡");
    alignLeftBtn->setCheckable(true);
    alignLeftBtn->setChecked(m_alignment == Qt::AlignLeft);
    alignLeftBtn->setToolTip("Izquierda");
    alignLeftBtn->setFixedSize(36, 30);

    auto *alignCenterBtn = new QToolButton();
    alignCenterBtn->setText("≡");
    alignCenterBtn->setCheckable(true);
    alignCenterBtn->setChecked(m_alignment == Qt::AlignCenter);
    alignCenterBtn->setToolTip("Centro");
    alignCenterBtn->setFixedSize(36, 30);

    auto *alignRightBtn = new QToolButton();
    alignRightBtn->setText("≡");
    alignRightBtn->setCheckable(true);
    alignRightBtn->setChecked(m_alignment == Qt::AlignRight);
    alignRightBtn->setToolTip("Derecha");
    alignRightBtn->setFixedSize(36, 30);

    auto *alignGroup = new QHBoxLayout();
    alignGroup->addWidget(alignLeftBtn);
    alignGroup->addWidget(alignCenterBtn);
    alignGroup->addWidget(alignRightBtn);

    QFrame *separator1 = new QFrame();
    separator1->setFrameShape(QFrame::HLine);
    QFrame *separator2 = new QFrame();
    separator2->setFrameShape(QFrame::HLine);

    auto *colorRow = new QHBoxLayout();

    auto createColorToolBtn = [](const QString &label, const QColor &color) -> QToolButton * {
        auto *btn = new QToolButton();
        btn->setPopupMode(QToolButton::InstantPopup);
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        QPixmap px(32, 24);
        px.fill(color);
        QPainter pt(&px);
        pt.setPen(QColor("#999"));
        pt.drawRect(QRect(0, 0, 31, 23));
        pt.end();
        btn->setIcon(QIcon(px));
        btn->setIconSize(QSize(32, 24));
        btn->setFixedSize(44, 30);
        btn->setToolTip(QString("%1 (%2)").arg(label, color.name()));
        return btn;
    };

    auto *textColorBtn = createColorToolBtn("Color del texto", m_textColor);
    auto *underlineColorBtn = createColorToolBtn("Color de subrayado", m_underlineColor);

    auto setupColorMenu = [this, &dlg, applyStyle](QToolButton *btn, QColor &targetColor, const QString &title) {
        auto *menu = new QMenu(btn);
        QList<QColor> presetColors = {
            QColor("#000000"), QColor("#434343"), QColor("#666666"), QColor("#999999"),
            QColor("#B7B7B7"), QColor("#CCCCCC"), QColor("#D9D9D9"), QColor("#FFFFFF"),
            QColor("#980000"), QColor("#FF0000"), QColor("#FF9900"), QColor("#FFFF00"),
            QColor("#00FF00"), QColor("#00FFFF"), QColor("#4A86E8"), QColor("#0000FF"),
            QColor("#9900FF"), QColor("#FF00FF")
        };

        auto colorGrid = [&](QMenu *menu, const QList<QColor> &colors, int cols) {
            auto *w = new QWidget();
            auto *grid = new QGridLayout(w);
            grid->setSpacing(1);
            grid->setContentsMargins(4, 4, 4, 4);
            for (int i = 0; i < colors.size(); ++i) {
                auto *cb = new QPushButton();
                cb->setFixedSize(18, 18);
                QPixmap px(18, 18);
                px.fill(colors[i]);
                cb->setIcon(QIcon(px));
                cb->setIconSize(QSize(18, 18));
                cb->setStyleSheet("border: 1px solid #999; padding: 0;");
                connect(cb, &QPushButton::clicked, this, [this, &targetColor, colors, i, btn, applyStyle, &dlg, title]() {
                    targetColor = colors[i];
                    QPixmap px(32, 24);
                    px.fill(targetColor);
                    QPainter pt(&px);
                    pt.setPen(QColor("#999"));
                    pt.drawRect(QRect(0, 0, 31, 23));
                    pt.end();
                    btn->setIcon(QIcon(px));
                    btn->setToolTip(QString("%1 (%2)").arg(title, targetColor.name()));
                    applyStyle();
                    dlg.activateWindow();
                });
                grid->addWidget(cb, i / cols, i % cols);
            }
            auto *wa = new QWidgetAction(menu);
            wa->setDefaultWidget(w);
            menu->addAction(wa);
        };

        colorGrid(menu, presetColors, 6);

        menu->addSeparator();
        auto *customAct = menu->addAction("Personalizar...");
        QObject::connect(customAct, &QAction::triggered, this, [this, &targetColor, btn, applyStyle, &dlg, title]() {
            QColor c = QColorDialog::getColor(targetColor, &dlg, title);
            if (c.isValid()) {
                targetColor = c;
                QPixmap px(32, 24);
                px.fill(targetColor);
                QPainter pt(&px);
                pt.setPen(QColor("#999"));
                pt.drawRect(QRect(0, 0, 31, 23));
                pt.end();
                btn->setIcon(QIcon(px));
                btn->setToolTip(QString("%1 (%2)").arg(title, targetColor.name()));
                applyStyle();
            }
        });

        btn->setMenu(menu);
    };

    setupColorMenu(textColorBtn, m_textColor, "Color del texto");
    setupColorMenu(underlineColorBtn, m_underlineColor, "Color de subrayado");

    auto *textLabel = new QLabel("Texto:");
    auto *underlineLabel = new QLabel("Subrayado:");
    colorRow->addWidget(textLabel);
    colorRow->addWidget(textColorBtn);
    colorRow->addSpacing(20);
    colorRow->addWidget(underlineLabel);
    colorRow->addWidget(underlineColorBtn);
    colorRow->addStretch();

    auto *bottomRow = new QHBoxLayout();
    auto *closeBtn = new QPushButton("Cerrar");
    closeBtn->setFixedWidth(100);
    bottomRow->addStretch();
    bottomRow->addWidget(closeBtn);

    mainLayout->addLayout(topRow);

    auto *midContainer = new QHBoxLayout();
    midContainer->addLayout(styleGroup);
    midContainer->addStretch();
    midContainer->addLayout(alignGroup);
    mainLayout->addLayout(midContainer);

    auto *preview = new QLabel("AaBbCc 123");
    preview->setAlignment(Qt::AlignCenter);
    preview->setFixedHeight(44);
    preview->setAutoFillBackground(true);
    preview->setStyleSheet(QString("QLabel { background: #FFFFFF; color: %1; border: 1px solid #D0D4D9; border-radius: 6px; }").arg(m_textColor.name()));
    QFont prevFont = m_editorFont;
    prevFont.setPointSize(qMax(10, m_editorFont.pointSize() - 2));
    preview->setFont(prevFont);
    mainLayout->addWidget(preview);

    mainLayout->addWidget(separator1);
    mainLayout->addLayout(colorRow);
    mainLayout->addWidget(separator2);
    mainLayout->addStretch();
    mainLayout->addLayout(bottomRow);

    auto updateFont = [this, fontCombo, sizeCombo, boldBtn, italicBtn, underlineBtn, preview, applyStyle]() {
        m_editorFont.setFamily(fontCombo->currentText());
        m_editorFont.setPointSize(sizeCombo->currentText().toInt());
        m_editorFont.setBold(boldBtn->isChecked());
        m_editorFont.setItalic(italicBtn->isChecked());
        m_editorFont.setUnderline(underlineBtn->isChecked());
        QFont pFont = m_editorFont;
        pFont.setPointSize(qMax(10, m_editorFont.pointSize()));
        preview->setFont(pFont);
        applyStyle();
    };

    connect(fontCombo, &QComboBox::currentTextChanged, this, [updateFont](const QString &) { updateFont(); });
    connect(sizeCombo, &QComboBox::currentTextChanged, this, [updateFont](const QString &) { updateFont(); });
    connect(boldBtn, &QToolButton::toggled, this, [updateFont](bool) { updateFont(); });
    connect(italicBtn, &QToolButton::toggled, this, [updateFont](bool) { updateFont(); });
    connect(underlineBtn, &QToolButton::toggled, this, [updateFont](bool) { updateFont(); });

    connect(alignLeftBtn, &QToolButton::clicked, this, [this, alignLeftBtn, alignCenterBtn, alignRightBtn]() {
        m_alignment = Qt::AlignLeft;
        alignLeftBtn->setChecked(true);
        alignCenterBtn->setChecked(false);
        alignRightBtn->setChecked(false);
    });
    connect(alignCenterBtn, &QToolButton::clicked, this, [this, alignLeftBtn, alignCenterBtn, alignRightBtn]() {
        m_alignment = Qt::AlignCenter;
        alignLeftBtn->setChecked(false);
        alignCenterBtn->setChecked(true);
        alignRightBtn->setChecked(false);
    });
    connect(alignRightBtn, &QToolButton::clicked, this, [this, alignLeftBtn, alignCenterBtn, alignRightBtn]() {
        m_alignment = Qt::AlignRight;
        alignLeftBtn->setChecked(false);
        alignCenterBtn->setChecked(false);
        alignRightBtn->setChecked(true);
    });

    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);

    dlg.exec();
}

void MainWindow::zoomIn()
{
    if (m_editorFont.pointSize() < 48) {
        m_editorFont.setPointSize(m_editorFont.pointSize() + 1);
        if (auto *ed = currentEditor()) ed->setFont(m_editorFont);
    }
}

void MainWindow::zoomOut()
{
    if (m_editorFont.pointSize() > 6) {
        m_editorFont.setPointSize(m_editorFont.pointSize() - 1);
        if (auto *ed = currentEditor()) ed->setFont(m_editorFont);
    }
}

void MainWindow::resetZoom()
{
    m_editorFont = QFont("Monospace", 11);
    m_editorFont.setStyleHint(QFont::Monospace);
    if (auto *ed = currentEditor()) ed->setFont(m_editorFont);
}

void MainWindow::onTabChanged(int index)
{
    updateTitle();
    updateStatusBar();

    auto *editor = qobject_cast<CodeEditor *>(m_tabWidget->widget(index));
    if (editor) {
        QString fileName = editor->document()->property("fileName").toString();
        auto *highlighter = editor->findChild<SyntaxHighlighter *>();
        if (highlighter)
            highlighter->setFileExtension(getFileExtension(fileName));
    }
}

void MainWindow::updateStatusBar()
{
    auto *editor = currentEditor();
    if (!editor) {
        m_statusCursor->setText("Línea: 1  Columna: 1");
        m_statusFile->setText("");
        return;
    }

    QTextCursor cursor = editor->textCursor();
    int line = cursor.blockNumber() + 1;
    int col = cursor.columnNumber() + 1;
    m_statusCursor->setText(QString("Línea: %1  Columna: %2").arg(line).arg(col));

    int charCount = editor->toPlainText().length();
    int wordCount = editor->toPlainText().split(QRegularExpression("\\s+"), Qt::SkipEmptyParts).count();
    m_statusFile->setText(QString("Palabras: %1  Caracteres: %2").arg(wordCount).arg(charCount));
}

void MainWindow::updateTitle()
{
    auto *editor = currentEditor();
    if (!editor) {
        setWindowTitle("Stellar Texto");
        return;
    }

    QString fileName = editor->document()->property("fileName").toString();
    if (fileName.isEmpty())
        setWindowTitle("Stellar Texto - Sin título");
    else
        setWindowTitle("Stellar Texto - " + QFileInfo(fileName).fileName());

    if (editor->document()->isModified())
        setWindowTitle(windowTitle() + " *");
}

bool MainWindow::maybeSave()
{
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        auto *editor = qobject_cast<CodeEditor *>(m_tabWidget->widget(i));
        if (editor && editor->document()->isModified()) {
            m_tabWidget->setCurrentIndex(i);
            QString fileName = editor->document()->property("fileName").toString();
            if (fileName.isEmpty())
                fileName = "Sin título";

            auto ret = QMessageBox::question(
                this, "Archivo modificado",
                QString("¿Guardar los cambios en \"%1\"?").arg(fileName),
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel
            );

            if (ret == QMessageBox::Save) {
                if (!saveFile())
                    return false;
            } else if (ret == QMessageBox::Cancel) {
                return false;
            }
        }
    }
    return true;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        QSettings settings;
        settings.setValue("window/geometry", saveGeometry());
        settings.setValue("editor/textColor", m_textColor);
        settings.setValue("editor/bgColor", m_bgColor);
        settings.setValue("editor/underlineColor", m_underlineColor);
        settings.setValue("editor/alignment", m_alignment);
        event->accept();
    } else {
        event->ignore();
    }
}

QString MainWindow::getFileExtension(const QString &fileName) const
{
    return QFileInfo(fileName).suffix();
}
