#include "finddialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QTextCursor>

FindDialog::FindDialog(QPlainTextEdit *editor, QWidget *parent)
    : QDialog(parent), m_editor(editor)
{
    setWindowTitle("Buscar y reemplazar");
    setFixedSize(450, 250);

    auto *mainLayout = new QVBoxLayout(this);

    auto *findLayout = new QHBoxLayout();
    findLayout->addWidget(new QLabel("Buscar:"));
    m_findInput = new QLineEdit();
    findLayout->addWidget(m_findInput);
    mainLayout->addLayout(findLayout);

    auto *replaceLayout = new QHBoxLayout();
    replaceLayout->addWidget(new QLabel("Reemplazar:"));
    m_replaceInput = new QLineEdit();
    replaceLayout->addWidget(m_replaceInput);
    mainLayout->addLayout(replaceLayout);

    auto *optionsLayout = new QHBoxLayout();
    m_caseSensitive = new QCheckBox("Mayúsculas/minúsculas");
    m_wholeWords = new QCheckBox("Palabras completas");
    optionsLayout->addWidget(m_caseSensitive);
    optionsLayout->addWidget(m_wholeWords);
    mainLayout->addLayout(optionsLayout);

    auto *buttonLayout = new QHBoxLayout();
    auto *findBtn = new QPushButton("Buscar siguiente");
    auto *replaceBtn = new QPushButton("Reemplazar");
    auto *replaceAllBtn = new QPushButton("Reemplazar todo");
    auto *closeBtn = new QPushButton("Cerrar");

    buttonLayout->addWidget(findBtn);
    buttonLayout->addWidget(replaceBtn);
    buttonLayout->addWidget(replaceAllBtn);
    buttonLayout->addWidget(closeBtn);
    mainLayout->addLayout(buttonLayout);

    m_statusLabel = new QLabel();
    m_statusLabel->setStyleSheet("color: #7A3E9D;");
    mainLayout->addWidget(m_statusLabel);

    connect(findBtn, &QPushButton::clicked, this, &FindDialog::findNext);
    connect(replaceBtn, &QPushButton::clicked, this, &FindDialog::replace);
    connect(replaceAllBtn, &QPushButton::clicked, this, &FindDialog::replaceAll);
    connect(closeBtn, &QPushButton::clicked, this, &QDialog::close);
    connect(m_findInput, &QLineEdit::returnPressed, this, &FindDialog::findNext);
}

void FindDialog::findNext()
{
    QTextDocument::FindFlags flags;
    if (m_caseSensitive->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (m_wholeWords->isChecked())
        flags |= QTextDocument::FindWholeWords;

    bool found = m_editor->find(m_findInput->text(), flags);
    if (found) {
        m_statusLabel->setText("Encontrado.");
    } else {
        QTextCursor cursor = m_editor->textCursor();
        cursor.movePosition(QTextCursor::Start);
        m_editor->setTextCursor(cursor);
        found = m_editor->find(m_findInput->text(), flags);
        if (found)
            m_statusLabel->setText("Encontrado (desde el inicio).");
        else
            m_statusLabel->setText("No se encontró.");
    }
}

void FindDialog::replace()
{
    QTextCursor cursor = m_editor->textCursor();
    if (cursor.hasSelection() && cursor.selectedText() == m_findInput->text()) {
        cursor.insertText(m_replaceInput->text());
        findNext();
    } else {
        findNext();
    }
}

void FindDialog::replaceAll()
{
    QTextCursor cursor = m_editor->textCursor();
    cursor.movePosition(QTextCursor::Start);
    m_editor->setTextCursor(cursor);

    int count = 0;
    QTextDocument::FindFlags flags;
    if (m_caseSensitive->isChecked())
        flags |= QTextDocument::FindCaseSensitively;
    if (m_wholeWords->isChecked())
        flags |= QTextDocument::FindWholeWords;

    while (m_editor->find(m_findInput->text(), flags)) {
        QTextCursor c = m_editor->textCursor();
        c.insertText(m_replaceInput->text());
        ++count;
    }
    m_statusLabel->setText(QString("Reemplazadas %1 ocurrencia(s).").arg(count));
}
