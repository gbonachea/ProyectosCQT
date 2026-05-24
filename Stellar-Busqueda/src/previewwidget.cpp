#include "previewwidget.h"
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QStyle>
#include <QApplication>

PreviewWidget::PreviewWidget(QWidget *parent) : QWidget(parent) {
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(12, 12, 12, 12);

    iconLabel = new QLabel(this);
    iconLabel->setAlignment(Qt::AlignCenter);
    iconLabel->setMinimumHeight(128);

    nameLabel = new QLabel(this);
    nameLabel->setWordWrap(true);
    nameLabel->setStyleSheet("font-size: 14px; font-weight: bold;");

    pathLabel = new QLabel(this);
    pathLabel->setWordWrap(true);
    pathLabel->setStyleSheet("color: #2a7de1; font-size: 11px;");

    sizeLabel = new QLabel(this);
    sizeLabel->setStyleSheet("color: #666; font-size: 11px;");

    dateLabel = new QLabel(this);
    dateLabel->setStyleSheet("color: #666; font-size: 11px;");

    previewLabel = new QLabel(this);
    previewLabel->setWordWrap(true);
    previewLabel->setTextFormat(Qt::PlainText);
    previewLabel->setStyleSheet("font-size: 12px; color: #333; background: #f5f5f5; padding: 8px; border-radius: 4px;");

    previewScroll = new QScrollArea(this);
    previewScroll->setWidgetResizable(true);
    previewScroll->setWidget(previewLabel);
    previewScroll->setMinimumHeight(200);

    mainLayout->addWidget(iconLabel);
    mainLayout->addWidget(nameLabel);
    mainLayout->addWidget(pathLabel);
    mainLayout->addWidget(sizeLabel);
    mainLayout->addWidget(dateLabel);
    mainLayout->addWidget(previewScroll);
    mainLayout->addStretch();

    clear();
}

void PreviewWidget::clear() {
    iconLabel->clear();
    nameLabel->clear();
    pathLabel->clear();
    sizeLabel->clear();
    dateLabel->clear();
    previewLabel->clear();
    previewScroll->hide();
}

void PreviewWidget::showPreview(const SearchResult &result) {
    nameLabel->setText(result.fileName);
    pathLabel->setText(result.dirPath);
    sizeLabel->setText("Tamaño: " + result.sizeString());
    dateLabel->setText("Modificado: " + result.lastModified.toString("dd/MM/yyyy hh:mm"));

    QString suffix = QFileInfo(result.filePath).suffix().toLower();
    QStringList imgExts = {"jpg", "jpeg", "png", "gif", "bmp", "webp", "svg", "ico"};
    QStringList txtExts = {"txt", "md", "csv", "log", "xml", "json", "html",
                           "cpp", "c", "h", "hpp", "py", "js", "css", "ini", "cfg"};
    QStringList vidExts = {"mp4", "avi", "mkv", "mov", "wmv", "flv", "webm"};

    if (imgExts.contains(suffix)) {
        showImagePreview(result.filePath);
    } else if (txtExts.contains(suffix)) {
        showTextPreview(result.filePath);
    } else if (vidExts.contains(suffix)) {
        showVideoPreview(result.filePath);
    } else {
        showGenericPreview(result);
    }
}

void PreviewWidget::showImagePreview(const QString &path) {
    QPixmap pix(path);
    if (!pix.isNull()) {
        QPixmap scaled = pix.scaled(400, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        iconLabel->setPixmap(scaled);
    } else {
        iconLabel->setText("🖼️ [Imagen]");
    }
    previewScroll->hide();
}

void PreviewWidget::showTextPreview(const QString &path) {
    iconLabel->setText("📄");
    QFile file(path);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&file);
        in.setCodec("UTF-8");
        QString content;
        for (int i = 0; i < 50 && !in.atEnd(); i++) {
            content += in.readLine() + "\n";
        }
        previewLabel->setText(content);
        previewScroll->show();
    }
}

void PreviewWidget::showVideoPreview(const QString &path) {
    iconLabel->setText("🎬");
    QFileInfo fi(path);
    previewLabel->setText(QString("Video: %1\nTamaño: %2")
                          .arg(fi.fileName())
                          .arg(sizeLabel->text()));
    previewScroll->show();
}

void PreviewWidget::showGenericPreview(const SearchResult &result) {
    iconLabel->setText("📎");
    previewScroll->hide();
}
