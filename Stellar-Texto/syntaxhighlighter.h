#ifndef SYNTAXHIGHLIGHTER_H
#define SYNTAXHIGHLIGHTER_H

#include <QSyntaxHighlighter>
#include <QTextCharFormat>
#include <QRegularExpression>

class SyntaxHighlighter : public QSyntaxHighlighter
{
    Q_OBJECT

public:
    explicit SyntaxHighlighter(QTextDocument *parent = nullptr);

    void setFileExtension(const QString &ext);

protected:
    void highlightBlock(const QString &text) override;

private:
    struct HighlightRule {
        QRegularExpression pattern;
        QTextCharFormat format;
    };
    QVector<HighlightRule> m_cppRules;
    QVector<HighlightRule> m_pythonRules;
    QVector<HighlightRule> m_otherRules;

    QRegularExpression m_commentStartExpr;
    QRegularExpression m_commentEndExpr;
    QTextCharFormat m_multiLineCommentFormat;

    QString m_fileExtension;

    void setupCppRules();
    void setupPythonRules();
    void setupOtherRules();
};

#endif
