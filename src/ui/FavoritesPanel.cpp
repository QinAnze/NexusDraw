#include "FavoritesPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QFileDialog>
#include <QDir>
#include <QMessageBox>
#include <QFile>
#include <QPixmap>
#include <QBuffer>
#include <QStandardPaths>
#include <QDialog>
#include <QFormLayout>
#include <QDialogButtonBox>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>

FavoritesPanel::FavoritesPanel(QWidget* parent) : QWidget(parent)
{
    setupUI();
}

void FavoritesPanel::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    auto* headerLayout = new QHBoxLayout;
    m_titleLabel = new QLabel(QString::fromUtf8("收藏夹"));
    m_titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; padding: 8px 10px 0 10px; color: #1a1a1a;");
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();
    mainLayout->addLayout(headerLayout);

    m_list = new QListWidget;
    m_list->setViewMode(QListView::IconMode);
    m_list->setFlow(QListView::LeftToRight);
    m_list->setWrapping(true);
    m_list->setResizeMode(QListView::Adjust);
    m_list->setSelectionMode(QAbstractItemView::MultiSelection);
    m_list->setDragEnabled(false);
    m_list->setDragDropMode(QAbstractItemView::NoDragDrop);
    m_list->setIconSize(QSize(140, 105));
    m_list->setSpacing(6);
    m_list->setStyleSheet("QListWidget { background: #fff; border: 2px solid #000; outline: none; }"
                          "QListWidget::item { padding: 4px; border: none; color: #000; }"
                          "QListWidget::item:selected { background: none; }"
                          "QListWidget::indicator { border: 2px solid #000; background: #fff; width: 14px; height: 14px; }"
                          "QListWidget::indicator:checked { background: #000; }");
    mainLayout->addWidget(m_list, 1);

    auto* btnLayout = new QHBoxLayout;
    btnLayout->setSpacing(6);

    m_exportBtn = new QPushButton(QString::fromUtf8("批量导出"));
    m_exportBtn->setFixedHeight(26);
    connect(m_exportBtn, &QPushButton::clicked, this, &FavoritesPanel::onExportSelected);

    m_removeBtn = new QPushButton(QString::fromUtf8("移除选中"));
    m_removeBtn->setFixedHeight(26);
    connect(m_removeBtn, &QPushButton::clicked, this, &FavoritesPanel::onRemoveSelected);

    m_clearBtn = new QPushButton(QString::fromUtf8("清空"));
    m_clearBtn->setFixedHeight(26);
    connect(m_clearBtn, &QPushButton::clicked, this, &FavoritesPanel::onClearAll);

    btnLayout->addWidget(m_exportBtn);
    btnLayout->addWidget(m_removeBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_clearBtn);
    mainLayout->addLayout(btnLayout);
}

void FavoritesPanel::addFavorite(const QString& imagePath)
{
    if (m_paths.contains(imagePath)) return;
    m_paths.append(imagePath);

    auto* item = new QListWidgetItem(QIcon(imagePath), QFileInfo(imagePath).fileName());
    item->setData(Qt::UserRole, imagePath);
    item->setSizeHint(QSize(160, 135));
    item->setTextAlignment(Qt::AlignCenter);
    item->setCheckState(Qt::Unchecked);
    item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
    m_list->addItem(item);
    m_titleLabel->setText(QString::fromUtf8("收藏夹 (%1)").arg(m_paths.size()));
}

QStringList FavoritesPanel::selectedPaths() const
{
    QStringList paths;
    for (int i = 0; i < m_list->count(); ++i) {
        if (m_list->item(i)->checkState() == Qt::Checked)
            paths.append(m_list->item(i)->data(Qt::UserRole).toString());
    }
    return paths;
}

int FavoritesPanel::count() const { return m_paths.size(); }

void FavoritesPanel::onRemoveSelected()
{
    for (int i = m_list->count() - 1; i >= 0; --i) {
        if (m_list->item(i)->checkState() == Qt::Checked) {
            QString path = m_list->item(i)->data(Qt::UserRole).toString();
            m_paths.removeAll(path);
            delete m_list->takeItem(i);
        }
    }
    m_titleLabel->setText(QString::fromUtf8("收藏夹 (%1)").arg(m_paths.size()));
}

void FavoritesPanel::onExportSelected()
{
    QStringList paths = selectedPaths();
    if (paths.isEmpty()) {
        for (int i = 0; i < m_list->count(); ++i)
            paths.append(m_list->item(i)->data(Qt::UserRole).toString());
    }
    if (paths.isEmpty()) return;

    // Build dialog
    QDialog dlg(this);
    dlg.setWindowTitle(QString::fromUtf8("批量导出 (%1 张)").arg(paths.size()));
    auto* layout = new QFormLayout(&dlg);

    auto* fmtCombo = new QComboBox;
    fmtCombo->addItem("PNG (*.png)", "png");
    fmtCombo->addItem("JPEG (*.jpg)", "jpg");
    fmtCombo->addItem("BMP (*.bmp)", "bmp");
    fmtCombo->addItem("SVG (*.svg)", "svg");
    layout->addRow(QString::fromUtf8("格式:"), fmtCombo);

    auto* pathRow = new QHBoxLayout;
    auto* pathEdit = new QLineEdit(QStandardPaths::writableLocation(QStandardPaths::PicturesLocation));
    pathRow->addWidget(pathEdit);
    auto* browseBtn = new QPushButton(QString::fromUtf8("浏览..."));
    pathRow->addWidget(browseBtn);
    layout->addRow(QString::fromUtf8("保存到:"), pathRow);

    auto* btns = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    connect(btns, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addRow(btns);

    connect(browseBtn, &QPushButton::clicked, this, [&]() {
        QString dir = QFileDialog::getExistingDirectory(&dlg, QString::fromUtf8("选择文件夹"));
        if (!dir.isEmpty()) pathEdit->setText(dir);
    });

    if (dlg.exec() != QDialog::Accepted) return;

    QString dir = pathEdit->text().trimmed();
    QString ext = fmtCombo->currentData().toString();
    QDir().mkpath(dir);

    int count = 0;
    for (const QString& src : paths) {
        QString base = QFileInfo(src).completeBaseName();
        QString dest = dir + "/" + base + "." + ext;
        if (ext == "svg") {
            QPixmap px(src);
            QByteArray pngData;
            QBuffer buf(&pngData);
            buf.open(QIODevice::WriteOnly);
            px.save(&buf, "PNG"); buf.close();
            QFile f(dest);
            if (f.open(QIODevice::WriteOnly)) {
                f.write(QString("<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"%1\" height=\"%2\">"
                    "<image width=\"%1\" height=\"%2\" href=\"data:image/png;base64,%3\"/></svg>")
                    .arg(px.width()).arg(px.height())
                    .arg(QString::fromLatin1(pngData.toBase64())).toUtf8());
                f.close(); ++count;
            }
        } else {
            QPixmap px(src);
            int q = (ext == "jpg" || ext == "jpeg") ? 95 : -1;
            if (px.save(dest, nullptr, q)) ++count;
        }
    }
    QMessageBox::information(this, QString::fromUtf8("导出完成"),
        QString::fromUtf8("已导出 %1/%2 张图片到:\n%3").arg(count).arg(paths.size()).arg(dir));
}

void FavoritesPanel::onClearAll()
{
    m_list->clear();
    m_paths.clear();
    m_titleLabel->setText(QString::fromUtf8("收藏夹"));
}
