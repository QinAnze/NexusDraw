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
    m_langCombo->addItem("SVG", "xml");
    m_langCombo->setMinimumWidth(72);
    m_langCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    m_langCombo->setCurrentIndex(0);
    int langIdx = m_langCombo->findData(AppConfig::instance().preferredLanguage());
    if (langIdx >= 0 && langIdx < 2) m_langCombo->setCurrentIndex(langIdx);
    headerLayout->addWidget(m_langCombo);

    // Color scheme selector
    m_colorCombo = new QComboBox;
    m_colorCombo->addItem(QString::fromUtf8("AI 自动选择"), "auto");
    m_colorCombo->addItem("Matplotlib: Viridis", "viridis");
    m_colorCombo->addItem("Matplotlib: Plasma", "plasma");
    m_colorCombo->addItem("Matplotlib: Inferno", "inferno");
    m_colorCombo->addItem("Matplotlib: Magma", "magma");
    m_colorCombo->addItem("Matplotlib: Cividis", "cividis");
    m_colorCombo->addItem("Seaborn: Deep", "deep");
    m_colorCombo->addItem("Seaborn: Muted", "muted");
    m_colorCombo->addItem("Seaborn: Colorblind", "colorblind");
    m_colorCombo->addItem("Seaborn: Bright", "bright");
    m_colorCombo->addItem("Seaborn: Dark", "dark");
    m_colorCombo->addItem("ggplot2: Default", "ggplot2");
    m_colorCombo->addItem("ggplot2: Set1", "Set1");
    m_colorCombo->addItem("ggplot2: Set2", "Set2");
    m_colorCombo->addItem("ggplot2: Set3", "Set3");
    m_colorCombo->addItem(QString::fromUtf8("Okabe-Ito (色弱友好)"), "okabe-ito");
    m_colorCombo->addItem(QString::fromUtf8("Nature 自然色"), "nature");
    m_colorCombo->addItem(QString::fromUtf8("Bio 生物期刊"), "bio");
    m_colorCombo->addItem(QString::fromUtf8("Finance 金融"), "finance");
    m_colorCombo->addItem("BWR 红白蓝", "BWR");
    m_colorCombo->addItem("BWR2 红白蓝2", "BWR2");
    m_colorCombo->setMinimumWidth(95);
    m_colorCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    headerLayout->addWidget(m_colorCombo);

    // Mode toggle
    m_modeCombo = new QComboBox;
    m_modeCombo->addItem(QString::fromUtf8("科学绘图"), "plot");
    m_modeCombo->addItem(QString::fromUtf8("流程图"), "flowchart");
    m_modeCombo->addItem(QString::fromUtf8("对话模式"), "chat");
    m_modeCombo->setMinimumWidth(85);
    m_modeCombo->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    // Auto-switch language when mode changes
    QString prevLang = m_langCombo->currentData().toString();
    connect(m_modeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, prevLang]() mutable {
        QString mode = m_modeCombo->currentData().toString();
        if (mode == "flowchart") {
            if (m_langCombo->currentData().toString() != "xml") {
                prevLang = m_langCombo->currentData().toString();
            }
            m_langCombo->setCurrentIndex(m_langCombo->findData("xml"));
            m_langCombo->setEnabled(false);
        } else if (mode == "plot") {
            m_langCombo->setEnabled(true);
            if (m_langCombo->currentData().toString() == "xml") {
                int idx = m_langCombo->findData(prevLang);
                if (idx >= 0 && idx < 2) m_langCombo->setCurrentIndex(idx);
                else m_langCombo->setCurrentIndex(0);
            }
        } else {
            m_langCombo->setEnabled(false);
        }
        emit modeChanged(mode);
    });
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
    QString mode = m_modeCombo->currentData().toString();
    QString lang = (mode == "flowchart") ? "xml" : m_langCombo->currentData().toString();
    QString color = m_colorCombo->currentData().toString();
    emit messageSent(text, mode, lang, color);
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
