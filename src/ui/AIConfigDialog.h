#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

/// Dialog for configuring the OpenAI-format LLM connection settings.
/// Provides fields for Base URL, API Key, Model name, and language preference.
class AIConfigDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AIConfigDialog(QWidget* parent = nullptr);
    ~AIConfigDialog() = default;

private slots:
    void onSave();
    void onTestConnection();
    void onBrowsePython();
    void onBrowseR();

private:
    void setupUI();
    void loadSettings();
    void applyDarkTheme();

    QLineEdit* m_baseURLEdit;
    QLineEdit* m_apiKeyEdit;
    QLineEdit* m_modelEdit;
    QLineEdit* m_pythonPathEdit;
    QLineEdit* m_rPathEdit;
    QPushButton* m_testBtn;
    QPushButton* m_saveBtn;
    QPushButton* m_cancelBtn;
    QLabel* m_statusLabel;
};
