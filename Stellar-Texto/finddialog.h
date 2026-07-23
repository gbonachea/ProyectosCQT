#ifndef FINDDIALOG_H
#define FINDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QPlainTextEdit>

class FindDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FindDialog(QPlainTextEdit *editor, QWidget *parent = nullptr);

private slots:
    void findNext();
    void replace();
    void replaceAll();

private:
    QPlainTextEdit *m_editor;
    QLineEdit *m_findInput;
    QLineEdit *m_replaceInput;
    QCheckBox *m_caseSensitive;
    QCheckBox *m_wholeWords;
    QLabel *m_statusLabel;
};

#endif
