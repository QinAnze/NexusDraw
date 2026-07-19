#include "CodeViewPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QFont>
#include <QTimer>
#include <QSyntaxHighlighter>
#include <QRegularExpression>

// ---- Simple Python/R Syntax Highlighter ----
class CodeHighlighter : public QSyntaxHighlighter {
public:
    CodeHighlighter(QTextDocument* parent) : QSyntaxHighlighter(parent) {
        // Keywords
        QTextCharFormat kwFmt;
        kwFmt.setForeground(QColor("#0000cc"));
        kwFmt.setFontWeight(QFont::Bold);
        QStringList keywords = {"def","import","from","as","class","return","if","elif","else",
            "for","while","try","except","with","as","in","not","and","or","True","False","None",
            "library","require","function","ggplot","aes","ggsave","print","quit","cat"};
        for (const QString& kw : keywords) {
            m_rules.append({QRegularExpression("\\b" + kw + "\\b"), kwFmt});
        }
        // Strings
        QTextCharFormat strFmt;
        strFmt.setForeground(QColor("#008800"));
        m_rules.append({QRegularExpression("\"[^\"]*\""), strFmt});
        m_rules.append({QRegularExpression("'[^']*'"), strFmt});
        // Comments
        QTextCharFormat cmtFmt;
        cmtFmt.setForeground(QColor("#888888"));
        cmtFmt.setFontItalic(true);
        m_rules.append({QRegularExpression("#[^\n]*"), cmtFmt});
        // Numbers
        QTextCharFormat numFmt;
        numFmt.setForeground(QColor("#cc0000"));
        m_rules.append({QRegularExpression("\\b[0-9]+\\.?[0-9]*\\b"), numFmt});
        // Function calls
        QTextCharFormat funcFmt;
        funcFmt.setForeground(QColor("#0000cc"));
        m_rules.append({QRegularExpression("\\b[a-zA-Z_][a-zA-Z0-9_]*(?=\\()"), funcFmt});
    }
protected:
    void highlightBlock(const QString& text) override {
        for (const auto& rule : m_rules) {
            QRegularExpressionMatchIterator it = rule.pattern.globalMatch(text);
            while (it.hasNext()) {
                QRegularExpressionMatch m = it.next();
                setFormat(m.capturedStart(), m.capturedLength(), rule.format);
            }
        }
    }
private:
    struct Rule { QRegularExpression pattern; QTextCharFormat format; };
    QVector<Rule> m_rules;
};

CodeViewPanel::CodeViewPanel(QWidget* parent) : QWidget(parent)
{
    setupUI();
}

void CodeViewPanel::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    auto* headerLayout = new QHBoxLayout;
    m_titleLabel = new QLabel(QString::fromUtf8("生成的代码"));
    m_titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; padding: 8px 10px 0 10px; color: #1a1a1a;");
    headerLayout->addWidget(m_titleLabel);

    headerLayout->addStretch();

    m_copyBtn = new QPushButton(QString::fromUtf8("复制"));
    m_copyBtn->setFixedHeight(26);
    connect(m_copyBtn, &QPushButton::clicked, this, [this]() {
        QApplication::clipboard()->setText(m_codeEdit->toPlainText());
        m_copyBtn->setText(QString::fromUtf8("已复制"));
        QTimer::singleShot(2000, this, [this]() { m_copyBtn->setText(QString::fromUtf8("复制")); });
    });
    headerLayout->addWidget(m_copyBtn);

    m_runBtn = new QPushButton(QString::fromUtf8("运行代码"));
    m_runBtn->setFixedHeight(26);
    m_runBtn->setEnabled(false);
    connect(m_runBtn, &QPushButton::clicked, this, &CodeViewPanel::runRequested);
    headerLayout->addWidget(m_runBtn);

    m_stopBtn = new QPushButton(QString::fromUtf8("停止"));
    m_stopBtn->setFixedHeight(26);
    m_stopBtn->setEnabled(false);
    connect(m_stopBtn, &QPushButton::clicked, this, &CodeViewPanel::stopRequested);
    headerLayout->addWidget(m_stopBtn);
    mainLayout->addLayout(headerLayout);

    m_codeEdit = new QPlainTextEdit;
    m_codeEdit->setReadOnly(false);
    m_codeEdit->setTabStopDistance(32);
    m_codeEdit->setLineWrapMode(QPlainTextEdit::NoWrap);
    QFont monoFont("Consolas", 12);
    monoFont.setStyleHint(QFont::Monospace);
    m_codeEdit->setFont(monoFont);
    new CodeHighlighter(m_codeEdit->document());

    mainLayout->addWidget(m_codeEdit, 1);
}

void CodeViewPanel::setCode(const QString& code, const QString& /*language*/)
{
    m_codeEdit->setPlainText(code);
    m_runBtn->setEnabled(!code.isEmpty());
    m_titleLabel->setText(QString::fromUtf8("生成的代码"));
}

QString CodeViewPanel::code() const { return m_codeEdit->toPlainText(); }

void CodeViewPanel::clear()
{
    m_codeEdit->clear();
    m_runBtn->setEnabled(false);
    m_titleLabel->setText(QString::fromUtf8("生成的代码"));
}

void CodeViewPanel::setRunEnabled(bool enabled)
{
    m_runBtn->setEnabled(enabled && !m_codeEdit->toPlainText().isEmpty());
}

void CodeViewPanel::setStopEnabled(bool enabled)
{
    m_stopBtn->setEnabled(enabled);
}

