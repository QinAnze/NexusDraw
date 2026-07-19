#include "ChatPanel.h"
#include "core/AppConfig.h"
#include <QScrollBar>
#include <QTimer>
#include <QKeyEvent>

ChatPanel::ChatPanel(QWidget* parent) : QWidget(parent)
{
    setupUI();
    applyTheme();
}

void ChatPanel::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    auto* headerLayout = new QHBoxLayout;
    auto* headerLabel = new QLabel(QString::fromUtf8("对话"));
    headerLabel->setStyleSheet("font-size: 14px; font-weight: bold; padding: 10px; color: #1a1a1a;");
    headerLayout->addWidget(headerLabel);
    headerLayout->addStretch();

    // Language selector
    m_langCombo = new QComboBox;
    m_langCombo->addItem("Python", "python");
    m_langCombo->addItem("R", "r");
    m_langCombo->setMinimumWidth(72);
    m_langCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_langCombo->setCurrentIndex(0);
    int langIdx = m_langCombo->findData(AppConfig::instance().preferredLanguage());
    if (langIdx >= 0) m_langCombo->setCurrentIndex(langIdx);
    headerLayout->addWidget(m_langCombo);

    // Color scheme selector
    m_colorCombo = new QComboBox;
    m_colorCombo->addItem(QString::fromUtf8("默认配色"), "");
    m_colorCombo->addItem("Viridis", "viridis");
    m_colorCombo->addItem("Plasma", "plasma");
    m_colorCombo->addItem("Inferno", "inferno");
    m_colorCombo->addItem("Cividis", "cividis");
    m_colorCombo->addItem("Okabe-Ito", "okabe-ito");
    m_colorCombo->addItem("Nature", "nature");
    m_colorCombo->addItem("Seaborn Deep", "seaborn-deep");
    m_colorCombo->addItem("Seaborn Muted", "seaborn-muted");
    m_colorCombo->addItem("Bio", "bio");
    m_colorCombo->addItem("Finance", "finance");
    m_colorCombo->addItem("BWR", "BWR");
    m_colorCombo->addItem("BWR2", "BWR2");
    m_colorCombo->setMinimumWidth(95);
    m_colorCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    headerLayout->addWidget(m_colorCombo);

    // Mode toggle
    m_modeCombo = new QComboBox;
    m_modeCombo->addItem(QString::fromUtf8("绘图模式"), true);
    m_modeCombo->addItem(QString::fromUtf8("对话模式"), false);
    m_modeCombo->setMinimumWidth(85);
    m_modeCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    headerLayout->addWidget(m_modeCombo);
    mainLayout->addLayout(headerLayout);

    m_scrollArea = new QScrollArea;
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setFrameShape(QFrame::NoFrame);
    m_scrollArea->setStyleSheet("QScrollArea { background-color: #ffffff; }");

    m_messageContainer = new QWidget;
    m_messageContainer->setStyleSheet("background-color: #ffffff;");
    m_messageLayout = new QVBoxLayout(m_messageContainer);
    m_messageLayout->setAlignment(Qt::AlignTop);
    m_messageLayout->setSpacing(8);
    m_messageLayout->setContentsMargins(10, 10, 10, 10);

    m_scrollArea->setWidget(m_messageContainer);
    mainLayout->addWidget(m_scrollArea, 1);

    auto* inputContainer = new QWidget;
    inputContainer->setObjectName("inputContainer");
    auto* inputLayout = new QVBoxLayout(inputContainer);
    inputLayout->setContentsMargins(10, 8, 10, 10);
    inputLayout->setSpacing(6);

    m_inputEdit = new QTextEdit;
    m_inputEdit->setPlaceholderText(
        QString::fromUtf8("描述您想要的图表...\n"
                          "例如: '用年龄做横轴、收入做纵轴画散点图，按性别着色'\n"
                          "Enter 发送, Shift+Enter 换行"));
    m_inputEdit->setMaximumHeight(90);
    m_inputEdit->setMinimumHeight(56);
    m_inputEdit->setTabChangesFocus(true);
    m_inputEdit->installEventFilter(this);
    inputLayout->addWidget(m_inputEdit);

    auto* btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(8);

    m_clearBtn = new QPushButton(QString::fromUtf8("清空对话"));
    m_clearBtn->setFixedHeight(30);
    connect(m_clearBtn, &QPushButton::clicked, this, &ChatPanel::clearChat);

    m_sendBtn = new QPushButton(QString::fromUtf8("发送"));
    m_sendBtn->setFixedHeight(30);
    m_sendBtn->setDefault(true);
    connect(m_sendBtn, &QPushButton::clicked, this, &ChatPanel::onSendClicked);

    btnLayout->addWidget(m_clearBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_sendBtn);
    inputLayout->addLayout(btnLayout);

    mainLayout->addWidget(inputContainer);

    addSystemMessage(QString::fromUtf8("欢迎使用 NexusDraw！请先上传数据集，然后描述您想要的图表。"));
}

QWidget* ChatPanel::createMessageBubble(const QString& text, bool isUser)
{
    auto* bubble = new QWidget;
    auto* layout = new QHBoxLayout(bubble);
    layout->setContentsMargins(0, 2, 0, 2);

    auto* label = new QLabel(text);
    label->setWordWrap(true);
    label->setTextFormat(Qt::PlainText);
    label->setMaximumWidth(380);

    if (isUser) {
        label->setStyleSheet(
            "background-color: #ffffff; color: #1a1a1a; "
            "border: 2px solid #000000; "
            "border-radius: 12px 12px 4px 12px; padding: 10px 14px; font-size: 13px;");
        layout->addStretch();
        layout->addWidget(label);
    } else {
        label->setStyleSheet(
            "background-color: #f5f5f5; color: #1a1a1a; "
            "border: 2px solid #000000; "
            "border-radius: 12px 12px 12px 4px; padding: 10px 14px; font-size: 13px;");
        layout->addWidget(label);
        layout->addStretch();
    }

    return bubble;
}

void ChatPanel::addMessage(const QString& text, bool isUser)
{
    m_messageLayout->addWidget(createMessageBubble(text, isUser));
    QTimer::singleShot(50, this, [this]() {
        m_scrollArea->verticalScrollBar()->setValue(
            m_scrollArea->verticalScrollBar()->maximum());
    });
}

void ChatPanel::addSystemMessage(const QString& text)
{
    auto* label = new QLabel(text);
    label->setWordWrap(true);
    label->setAlignment(Qt::AlignCenter);
    label->setStyleSheet("color: #999; font-size: 12px; padding: 6px 12px;");
    m_messageLayout->addWidget(label);
    QTimer::singleShot(50, this, [this]() {
        m_scrollArea->verticalScrollBar()->setValue(
            m_scrollArea->verticalScrollBar()->maximum());
    });
}

void ChatPanel::clearChat()
{
    QLayoutItem* item;
    while ((item = m_messageLayout->takeAt(0)) != nullptr) {
        if (item->widget()) item->widget()->deleteLater();
        delete item;
    }
    addSystemMessage(QString::fromUtf8("对话已清空。请描述您的图表需求..."));
}

void ChatPanel::setInputEnabled(bool enabled)
{
    m_inputEdit->setEnabled(enabled);
    m_sendBtn->setEnabled(enabled);
}

void ChatPanel::onSendClicked()
{
    QString text = m_inputEdit->toPlainText().trimmed();
    if (text.isEmpty()) return;
    bool isPlot = m_modeCombo->currentData().toBool();
    QString lang = m_langCombo->currentData().toString();
    QString color = m_colorCombo->currentData().toString();
    // Save language preference
    AppConfig::instance().setPreferredLanguage(lang);
    emit messageSent(text, isPlot, lang, color);
    addMessage(text, true);
    m_inputEdit->clear();
    m_inputEdit->setFocus();
}

void ChatPanel::applyTheme()
{
    // Global stylesheet handles everything
}

bool ChatPanel::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == m_inputEdit && event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Return || keyEvent->key() == Qt::Key_Enter) {
            if (keyEvent->modifiers() & Qt::ShiftModifier) {
                // Shift+Enter: insert newline (default behavior)
                return false;
            } else {
                // Enter: send message
                onSendClicked();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}
