#pragma once

#include <QString>
#include <QStringList>
#include <QVector>
#include <QPair>

struct ParsedQuery {
    QStringList keywords;
    QStringList fileExtensions;
    QString fileCategory; // "image", "document", "video", "audio", "all"
    QStringList exactPhrases;
    bool searchContent;
    QString originalQuery;

    bool isEmpty() const {
        return keywords.isEmpty() && fileCategory == "all" && exactPhrases.isEmpty();
    }
};

class QueryParser {
public:
    static ParsedQuery parse(const QString &query);

private:
    static QStringList extractKeywords(const QString &query);
    static QString detectFileCategory(const QString &query);
    static QStringList detectExtensions(const QString &category);
    static QStringList extractExactPhrases(const QString &query);
    static QString cleanWord(const QString &word);

    static const QStringList stopWords;
    static const QVector<QPair<QString, QStringList>> categoryMap;
};
