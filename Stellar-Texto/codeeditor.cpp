#include "codeeditor.h"
#include <QPainter>
#include <QTextBlock>
#include <QKeyEvent>
#include <QMenu>
#include <QContextMenuEvent>
#include <QClipboard>

CodeEditor::CodeEditor(QWidget *parent)
    : QPlainTextEdit(parent)
{
    m_lineNumberArea = new LineNumberArea(this);

    connect(this, &CodeEditor::blockCountChanged, this, &CodeEditor::updateLineNumberAreaWidth);
    connect(this, &CodeEditor::updateRequest, this, &CodeEditor::updateLineNumberArea);
    connect(this, &CodeEditor::cursorPositionChanged, this, &CodeEditor::highlightCurrentLine);

    updateLineNumberAreaWidth(0);
    highlightCurrentLine();

    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
    setMouseTracking(true);
}

int CodeEditor::lineNumberAreaWidth() const
{
    int digits = 1;
    int max = qMax(1, blockCount());
    while (max >= 10) {
        max /= 10;
        ++digits;
    }
    int space = 10 + fontMetrics().horizontalAdvance(QLatin1Char('9')) * digits;
    return space;
}

void CodeEditor::updateLineNumberAreaWidth(int)
{
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void CodeEditor::updateLineNumberArea(const QRect &rect, int dy)
{
    if (dy)
        m_lineNumberArea->scroll(0, dy);
    else
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());

    if (rect.contains(viewport()->rect()))
        updateLineNumberAreaWidth(0);
}

void CodeEditor::resizeEvent(QResizeEvent *event)
{
    QPlainTextEdit::resizeEvent(event);
    QRect cr = contentsRect();
    m_lineNumberArea->setGeometry(QRect(cr.left(), cr.top(), lineNumberAreaWidth(), cr.height()));
}

void CodeEditor::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Tab) {
        insertPlainText(QString(4, ' '));
        return;
    }
    QPlainTextEdit::keyPressEvent(event);
}

void CodeEditor::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu menu(this);

    menu.addAction("Deshacer", QKeySequence::Undo, this, &CodeEditor::undo);
    menu.addAction("Rehacer", QKeySequence::Redo, this, &CodeEditor::redo);
    menu.addSeparator();
    menu.addAction("Cortar", QKeySequence::Cut, this, &CodeEditor::cut);
    menu.addAction("Copiar", QKeySequence::Copy, this, &CodeEditor::copy);
    menu.addAction("Pegar", QKeySequence::Paste, this, &CodeEditor::paste);
    menu.addAction("Eliminar", this, [this]() { insertPlainText(QString()); });
    menu.addSeparator();
    menu.addAction("Seleccionar todo", QKeySequence::SelectAll, this, &CodeEditor::selectAll);

    menu.exec(event->globalPos());
}

void CodeEditor::highlightCurrentLine()
{
    QList<QTextEdit::ExtraSelection> extraSelections;
    if (!isReadOnly()) {
        QTextEdit::ExtraSelection selection;
        QColor lineColor = QColor(Qt::yellow).lighter(180);
        selection.format.setBackground(lineColor);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.cursor = textCursor();
        selection.cursor.clearSelection();
        extraSelections.append(selection);
    }
    setExtraSelections(extraSelections);
}

void CodeEditor::lineNumberAreaPaintEvent(QPaintEvent *event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), QColor(0xF5, 0xF5, 0xF5));

    QTextBlock block = firstVisibleBlock();
    int blockNumber = block.blockNumber();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    int bottom = top + qRound(blockBoundingRect(block).height());

    while (block.isValid() && top <= event->rect().bottom()) {
        if (block.isVisible() && bottom >= event->rect().top()) {
            QString number = QString::number(blockNumber + 1);
            painter.setPen(QColor(0x7A, 0x7A, 0x7A));
            painter.drawText(0, top, m_lineNumberArea->width() - 8,
                             fontMetrics().height(), Qt::AlignRight, number);
        }
        block = block.next();
        top = bottom;
        bottom = top + qRound(blockBoundingRect(block).height());
        ++blockNumber;
    }
}
