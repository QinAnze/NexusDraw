#pragma once

#include <QWidget>
#include <QTextEdit>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>

class ChatPanel : public QWidget
{
    Q_OBJECT

public:
    explicit ChatPanel(QWidget* parent = nullptr);
    ~ChatPanel() = default;

    void addMessage(const QString& text, bool isUser);
    void addSystemMessage(const QString& text);
    void clearChat();
    void setInputEnabled(bool enabled);

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

signals:
    void messageSent(const QString& message, bool isPlotMode, const QString& language, const QString& colorScheme);
    void languageChanged(const QString& language);

private slots:
    void onSendClicked();

private:
    void setupUI();
    void applyTheme();
    QWidget* createMessageBubble(const QString& text, bool isUser);

    QScrollArea* m_scrollArea;
    QWidget* m_messageContainer;
    QVBoxLayout* m_messageLayout;
    QTextEdit* m_inputEdit;
    QPushButton* m_sendBtn;
    QPushButton* m_clearBtn;
    QComboBox* m_modeCombo;
    QComboBox* m_langCombo;
    QComboBox* m_colorCombo;
};
