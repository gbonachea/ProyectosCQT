#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTabWidget>
#include <QLabel>
#include <QFont>
#include <QColor>

class CodeEditor;
class SyntaxHighlighter;
class FindDialog;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;
    void openFileFromPath(const QString &fileName);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newFile();
    void openFile();
    bool saveFile();
    bool saveFileAs();
    void closeCurrentTab(int index);
    void undo();
    void redo();
    void cut();
    void copy();
    void paste();
    void selectAll();
    void showFindDialog();
    void printFile();
    void showSettings();
    void showAbout();
    void zoomIn();
    void zoomOut();
    void resetZoom();

    void onTabChanged(int index);
    void updateStatusBar();
    void updateTitle();

private:
    void setupMenuBar();
    void setupToolBar();
    void setupStatusBar();
    void setupShortcuts();
    CodeEditor *currentEditor() const;
    int addEditorTab(const QString &title = "Sin título");
    bool maybeSave();
    bool saveFile(CodeEditor *editor, const QString &fileName);
    QString getFileExtension(const QString &fileName) const;

    QTabWidget *m_tabWidget;
    QLabel *m_statusCursor;
    QLabel *m_statusFile;
    QFont m_editorFont;
    QColor m_textColor;
    QColor m_bgColor;
    QColor m_underlineColor;
    int m_alignment;
    QString m_lastDir;
};

#endif
