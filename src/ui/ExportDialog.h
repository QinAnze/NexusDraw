#pragma once

#include <QDialog>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QSpinBox>

/// Dialog for exporting the generated plot in various formats.
/// Supports PNG, JPG, SVG, PDF, and configurable DPI/size.
class ExportDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ExportDialog(const QString& sourceImagePath, QWidget* parent = nullptr);

private slots:
    void onBrowse();
    void onExport();
    void onFormatChanged(int index);

private:
    void setupUI();
    void applyDarkTheme();

    QString m_sourcePath;
    QComboBox* m_formatCombo;
    QLineEdit* m_pathEdit;
    QPushButton* m_browseBtn;
    QSpinBox* m_dpiSpinBox;
    QSpinBox* m_widthSpinBox;
    QSpinBox* m_heightSpinBox;
    QPushButton* m_exportBtn;
    QPushButton* m_cancelBtn;
};
