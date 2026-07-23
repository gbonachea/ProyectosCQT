#pragma once
#include <QDialog>
#include <QTabWidget>
#include <QCheckBox>
#include <QSlider>
#include <QPushButton>
#include <QLabel>
#include <QMap>
#include <QSettings>

class ConfigWindow : public QDialog {
    Q_OBJECT
public:
    explicit ConfigWindow(QWidget *parent = nullptr);

signals:
    void equalizerChanged(const QMap<QString, int> &values);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void saveSettings();
    void saveEqSettings();
    void applyPreset(const QString &presetName);

private:
    void setupUi();
    void loadSavedGeometry();
    void loadEqSettings();

    QCheckBox  *m_startupCheck;
    QCheckBox  *m_minimizeCheck;
    QMap<QString, QSlider*> m_eqSliders;
    QSettings   m_settings;
};
