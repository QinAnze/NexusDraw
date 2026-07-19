#include "DataPreviewPanel.h"
#include "core/DataManager.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QFileInfo>

DataPreviewPanel::DataPreviewPanel(QWidget* parent) : QWidget(parent)
{
    setupUI();
    applyDarkTheme();
}

void DataPreviewPanel::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(6);

    auto* headerLayout = new QHBoxLayout;
    m_titleLabel = new QLabel(QString::fromUtf8("数据预览"));
    m_titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; padding: 10px; color: #1a1a1a;");
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();

    m_uploadBtn = new QPushButton(QString::fromUtf8("上传数据集"));
    m_uploadBtn->setFixedHeight(30);
    connect(m_uploadBtn, &QPushButton::clicked, this, &DataPreviewPanel::uploadRequested);
    headerLayout->addWidget(m_uploadBtn);
    mainLayout->addLayout(headerLayout);

    m_infoLabel = new QLabel(QString::fromUtf8("尚未加载数据集。点击 [上传数据集] 开始。"));
    m_infoLabel->setStyleSheet("color: #999; font-size: 12px; padding: 0 10px;");
    m_infoLabel->setWordWrap(true);
    mainLayout->addWidget(m_infoLabel);

    m_table = new QTableWidget;
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_table->setAlternatingRowColors(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->verticalHeader()->setVisible(true);
    m_table->setSortingEnabled(false);
    mainLayout->addWidget(m_table, 1);
}

void DataPreviewPanel::loadFromDataManager(const DataManager* manager)
{
    if (!manager || !manager->isLoaded()) { clear(); return; }

    m_infoLabel->setText(QString::fromUtf8("文件: %1 | 行数: %2 | 列数: %3")
        .arg(manager->fileName()).arg(manager->rowCount()).arg(manager->columnCount()));

    QStringList headers = manager->columnNames();
    QStringList types = manager->columnTypes();

    QStringList displayHeaders;
    for (int i = 0; i < headers.size(); ++i) {
        QString typeStr = i < types.size() ? types[i] : "unknown";
        displayHeaders << QString("%1 [%2]").arg(headers[i], typeStr);
    }

    int previewRows = qMin(manager->rowCount(), 100);
    const auto& data = manager->data();

    m_table->setColumnCount(headers.size());
    m_table->setRowCount(previewRows);
    m_table->setHorizontalHeaderLabels(displayHeaders);

    for (int r = 0; r < previewRows; ++r) {
        for (int c = 0; c < headers.size(); ++c) {
            auto* item = new QTableWidgetItem(data[r][c].toString());
            item->setToolTip(data[r][c].toString());
            m_table->setItem(r, c, item);
        }
    }
    m_table->resizeColumnsToContents();
    for (int c = 0; c < m_table->columnCount(); ++c) {
        m_table->setColumnWidth(c, qMin(m_table->columnWidth(c), 180));
    }
}

void DataPreviewPanel::clear()
{
    m_infoLabel->setText(QString::fromUtf8("尚未加载数据集。点击 [上传数据集] 开始。"));
    m_table->clear();
    m_table->setRowCount(0);
    m_table->setColumnCount(0);
}

void DataPreviewPanel::applyDarkTheme()
{
    // Global stylesheet handles everything
}
