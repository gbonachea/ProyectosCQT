#pragma once

#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QScrollArea>
#include <QPixmap>
#include "searchresult.h"

class PreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit PreviewWidget(QWidget *parent = nullptr);

    void showPreview(const SearchResult &result);
    void clear();

private:
    void showImagePreview(const QString &path);
    void showTextPreview(const QString &path);
    void showVideoPreview(const QString &path);
    void showGenericPreview(const SearchResult &result);

    QLabel *iconLabel;
    QLabel *nameLabel;
    QLabel *pathLabel;
    QLabel *sizeLabel;
    QLabel *dateLabel;
    QLabel *previewLabel;
    QScrollArea *previewScroll;
    QWidget *previewContent;
    QVBoxLayout *mainLayout;
};
