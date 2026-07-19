#pragma once

#include <QWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QLabel>

/// Displays the generated code from the LLM with syntax-highlighting-like formatting.
/// Allows the user to review, edit, and execute the code.
class CodeViewPanel : public QWidget
{
    Q_OBJECT

public:
    explicit CodeViewPanel(QWidget* parent = nullptr);
    ~CodeViewPanel() = default;

    /// Set the code to display
    void setCode(const QString& code, const QString& language);

    /// Get the current code (possibly edited by user)
    QString code() const;

    /// Clear the code view
    void clear();

    /// Set whether run/stop buttons are enabled
    void setRunEnabled(bool enabled);
    void setStopEnabled(bool enabled);

signals:
    void runRequested();
    void stopRequested();

private:
    void setupUI();
    void applyDarkTheme();

    QLabel* m_titleLabel;
    QPlainTextEdit* m_codeEdit;
    QPushButton* m_runBtn;
    QPushButton* m_stopBtn;
    QPushButton* m_copyBtn;
};
