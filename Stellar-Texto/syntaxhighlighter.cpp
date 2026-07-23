#include "syntaxhighlighter.h"

SyntaxHighlighter::SyntaxHighlighter(QTextDocument *parent)
    : QSyntaxHighlighter(parent)
{
    setupCppRules();
    setupPythonRules();
    setupOtherRules();

    m_multiLineCommentFormat.setForeground(QColor(0x7A, 0x7A, 0x7A));
    m_multiLineCommentFormat.setFontItalic(true);
}

void SyntaxHighlighter::setFileExtension(const QString &ext)
{
    m_fileExtension = ext.toLower();
    rehighlight();
}

void SyntaxHighlighter::setupCppRules()
{
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor(0x00, 0x56, 0x8E));
    keywordFormat.setFontWeight(QFont::Bold);

    QStringList keywords = {
        "\\bauto\\b", "\\bbool\\b", "\\bbreak\\b", "\\bcase\\b", "\\bcatch\\b",
        "\\bchar\\b", "\\bclass\\b", "\\bconst\\b", "\\bconstexpr\\b", "\\bcontinue\\b",
        "\\bdefault\\b", "\\bdelete\\b", "\\bdo\\b", "\\bdouble\\b", "belse\\b",
        "\\benum\\b", "\\bexplicit\\b", "\\bextern\\b", "\\bfloat\\b", "\\bfor\\b",
        "\\bfriend\\b", "\\bgoto\\b", "\\bif\\b", "\\binline\\b", "\\bint\\b",
        "\\blong\\b", "\\bmutable\\b", "\\bnamespace\\b", "\\bnew\\b", "\\bnoexcept\\b",
        "\\bnullptr\\b", "\\boperator\\b", "\\boverride\\b", "\\bprivate\\b",
        "\\bprotected\\b", "\\bpublic\\b", "\\breturn\\b", "\\bshort\\b",
        "\\bsigned\\b", "\\bsizeof\\b", "\\bstatic\\b", "\\bstruct\\b",
        "\\bswitch\\b", "\\btemplate\\b", "\\bthis\\b", "\\bthrow\\b",
        "\\btry\\b", "\\btypedef\\b", "\\btypename\\b", "\\bunion\\b",
        "\\bunsigned\\b", "\\busing\\b", "\\bvirtual\\b", "\\bvoid\\b",
        "\\bvolatile\\b", "\\bwhile\\b", "\\binclude\\b", "\\bdefine\\b",
        "\\bifdef\\b", "\\bendif\\b", "\\bpragma\\b"
    };

    for (const auto &kw : keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(kw);
        rule.format = keywordFormat;
        m_cppRules.append(rule);
    }

    QTextCharFormat singleLineComment;
    singleLineComment.setForeground(QColor(0x7A, 0x7A, 0x7A));
    singleLineComment.setFontItalic(true);

    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression("//[^\n]*");
    commentRule.format = singleLineComment;
    m_cppRules.append(commentRule);

    QTextCharFormat stringFormat;
    stringFormat.setForeground(QColor(0xBA, 0x21, 0x21));

    HighlightRule stringRule;
    stringRule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    stringRule.format = stringFormat;
    m_cppRules.append(stringRule);

    QTextCharFormat charFormat;
    charFormat.setForeground(QColor(0xBA, 0x21, 0x21));

    HighlightRule charRule;
    charRule.pattern = QRegularExpression("'.'");
    charRule.format = charFormat;
    m_cppRules.append(charRule);

    QTextCharFormat numberFormat;
    numberFormat.setForeground(QColor(0x09, 0x85, 0x91));

    HighlightRule numberRule;
    numberRule.pattern = QRegularExpression("\\b\\d+(\\.\\d+)?\\b");
    numberRule.format = numberFormat;
    m_cppRules.append(numberRule);

    QTextCharFormat preprocessorFormat;
    preprocessorFormat.setForeground(QColor(0x7A, 0x3E, 0x9D));

    HighlightRule preprocessorRule;
    preprocessorRule.pattern = QRegularExpression("^#[^\n]*");
    preprocessorRule.format = preprocessorFormat;
    m_cppRules.append(preprocessorRule);

    m_commentStartExpr = QRegularExpression("/\\*");
    m_commentEndExpr = QRegularExpression("\\*/");
}

void SyntaxHighlighter::setupPythonRules()
{
    QTextCharFormat keywordFormat;
    keywordFormat.setForeground(QColor(0x00, 0x56, 0x8E));
    keywordFormat.setFontWeight(QFont::Bold);

    QStringList keywords = {
        "\\band\\b", "\\bas\\b", "\\bassert\\b", "\\basync\\b", "\\bawait\\b",
        "\\bbreak\\b", "\\bclass\\b", "\\bcontinue\\b", "\\bdef\\b", "\\bdel\\b",
        "\\belif\\b", "\\belse\\b", "\\bexcept\\b", "\\bfalse\\b", "\\bfinally\\b",
        "\\bfor\\b", "\\bfrom\\b", "\\bglobal\\b", "\\bif\\b", "\\bimport\\b",
        "\\bin\\b", "\\bis\\b", "\\blambda\\b", "\\bNone\\b", "\\bnonlocal\\b",
        "\\bnot\\b", "\\bor\\b", "\\bpass\\b", "\\braise\\b", "\\breturn\\b",
        "\\bTrue\\b", "\\btry\\b", "\\bwhile\\b", "\\bwith\\b", "\\byield\\b",
        "\\bself\\b", "\\bprint\\b"
    };

    for (const auto &kw : keywords) {
        HighlightRule rule;
        rule.pattern = QRegularExpression(kw);
        rule.format = keywordFormat;
        m_pythonRules.append(rule);
    }

    QTextCharFormat singleLineComment;
    singleLineComment.setForeground(QColor(0x7A, 0x7A, 0x7A));
    singleLineComment.setFontItalic(true);

    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression("#[^\n]*");
    commentRule.format = singleLineComment;
    m_pythonRules.append(commentRule);

    QTextCharFormat stringFormat;
    stringFormat.setForeground(QColor(0xBA, 0x21, 0x21));

    HighlightRule stringRule;
    stringRule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    stringRule.format = stringFormat;
    m_pythonRules.append(stringRule);

    HighlightRule stringRule2;
    stringRule2.pattern = QRegularExpression("'[^'\\\\]*(\\\\.[^'\\\\]*)*'");
    stringRule2.format = stringFormat;
    m_pythonRules.append(stringRule2);

    QTextCharFormat functionFormat;
    functionFormat.setForeground(QColor(0x00, 0x56, 0x8E));
    functionFormat.setFontItalic(true);

    HighlightRule functionRule;
    functionRule.pattern = QRegularExpression("\\b[A-Za-z_][A-Za-z0-9_]+(?=\\()");
    functionRule.format = functionFormat;
    m_pythonRules.append(functionRule);

    QTextCharFormat numberFormat;
    numberFormat.setForeground(QColor(0x09, 0x85, 0x91));

    HighlightRule numberRule;
    numberRule.pattern = QRegularExpression("\\b\\d+(\\.\\d+)?\\b");
    numberRule.format = numberFormat;
    m_pythonRules.append(numberRule);

    QTextCharFormat decoratorFormat;
    decoratorFormat.setForeground(QColor(0x7A, 0x3E, 0x9D));

    HighlightRule decoratorRule;
    decoratorRule.pattern = QRegularExpression("@[A-Za-z_][A-Za-z0-9_.]*");
    decoratorRule.format = decoratorFormat;
    m_pythonRules.append(decoratorRule);

    m_commentStartExpr = QRegularExpression(R"(""")");
    m_commentEndExpr = QRegularExpression(R"(""")");
}

void SyntaxHighlighter::setupOtherRules()
{
    QTextCharFormat commentFormat;
    commentFormat.setForeground(QColor(0x7A, 0x7A, 0x7A));
    commentFormat.setFontItalic(true);

    HighlightRule commentRule;
    commentRule.pattern = QRegularExpression("#[^\n]*");
    commentRule.format = commentFormat;
    m_otherRules.append(commentRule);

    QTextCharFormat stringFormat;
    stringFormat.setForeground(QColor(0xBA, 0x21, 0x21));

    HighlightRule stringRule;
    stringRule.pattern = QRegularExpression("\"[^\"\\\\]*(\\\\.[^\"\\\\]*)*\"");
    stringRule.format = stringFormat;
    m_otherRules.append(stringRule);
}

void SyntaxHighlighter::highlightBlock(const QString &text)
{
    QVector<HighlightRule> *rules = &m_otherRules;
    if (m_fileExtension == "cpp" || m_fileExtension == "c" || m_fileExtension == "h"
        || m_fileExtension == "hpp" || m_fileExtension == "cc" || m_fileExtension == "cxx") {
        rules = &m_cppRules;
    } else if (m_fileExtension == "py") {
        rules = &m_pythonRules;
    }

    for (const auto &rule : *rules) {
        QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
        while (it.hasNext()) {
            QRegularExpressionMatch match = it.next();
            setFormat(match.capturedStart(), match.capturedLength(), rule.format);
        }
    }

    setCurrentBlockState(0);
    int startIndex = 0;
    if (previousBlockState() != 1)
        startIndex = text.indexOf(m_commentStartExpr);

    while (startIndex >= 0) {
        QRegularExpressionMatch endMatch = m_commentEndExpr.match(text, startIndex + 1);
        int endIndex = endMatch.capturedStart();
        int commentLength;
        if (endIndex == -1) {
            setCurrentBlockState(1);
            commentLength = text.length() - startIndex;
        } else {
            commentLength = endIndex - startIndex + endMatch.capturedLength();
        }
        setFormat(startIndex, commentLength, m_multiLineCommentFormat);
        startIndex = text.indexOf(m_commentStartExpr, startIndex + commentLength);
    }
}
