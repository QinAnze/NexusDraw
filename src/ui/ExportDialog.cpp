#include "ExportDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPixmap>
#include <QStandardPaths>
#include <QDir>

ExportDialog::ExportDialog(const QString& sourceImagePath, QWidget* parent)
    : QDialog(parent), m_sourcePath(sourceImagePath)
{
    setupUI();
    applyDarkTheme();
    QFileInfo info(sourceImagePath);
    QString picturesDir = QStandardPaths::writableLocation(QStandardPaths::PicturesLocation);
    QDir().mkpath(picturesDir);
    m_pathEdit->setText(picturesDir + "/" + info.completeBaseName() + "_exported.png");
    setWindowTitle(QString::fromUtf8("导出图表"));
    setMinimumWidth(460);
    setModal(true);
}

void ExportDialog::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 16, 20, 16);

    auto* fmtGroup = new QGroupBox(QString::fromUtf8("导出设置"));
    auto* formLayout = new QFormLayout(fmtGroup);
    formLayout->setSpacing(10);

    m_formatCombo = new QComboBox;
    m_formatCombo->addItem("PNG (*.png)", "png");
    m_formatCombo->addItem("JPEG (*.jpg)", "jpg");
    m_formatCombo->addItem("BMP (*.bmp)", "bmp");
    m_formatCombo->setMinimumHeight(30);
    connect(m_formatCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ExportDialog::onFormatChanged);
    formLayout->addRow(QString::fromUtf8("格式:"), m_formatCombo);

    auto* pathRow = new QHBoxLayout;
    m_pathEdit = new QLineEdit;
    m_pathEdit->setMinimumHeight(30);
    m_pathEdit->setReadOnly(true);
    pathRow->addWidget(m_pathEdit);
    m_browseBtn = new QPushButton(QString::fromUtf8("浏览..."));
    m_browseBtn->setMinimumHeight(30);
    connect(m_browseBtn, &QPushButton::clicked, this, &ExportDialog::onBrowse);
    pathRow->addWidget(m_browseBtn);
    formLayout->addRow(QString::fromUtf8("保存到:"), pathRow);

    m_dpiSpinBox = new QSpinBox;
    m_dpiSpinBox->setRange(72, 600);
    m_dpiSpinBox->setValue(150);
    m_dpiSpinBox->setMinimumHeight(30);
    m_dpiSpinBox->setSuffix(" DPI");
    formLayout->addRow(QString::fromUtf8("质量:"), m_dpiSpinBox);

    m_widthSpinBox = new QSpinBox;
    m_widthSpinBox->setRange(0, 10000);
    m_widthSpinBox->setValue(0);
    m_widthSpinBox->setMinimumHeight(30);
    m_widthSpinBox->setSpecialValueText(QString::fromUtf8("原始"));
    m_widthSpinBox->setSuffix(" px");
    formLayout->addRow(QString::fromUtf8("宽度:"), m_widthSpinBox);

    m_heightSpinBox = new QSpinBox;
    m_heightSpinBox->setRange(0, 10000);
    m_heightSpinBox->setValue(0);
    m_heightSpinBox->setMinimumHeight(30);
    m_heightSpinBox->setSpecialValueText(QString::fromUtf8("原始"));
    m_heightSpinBox->setSuffix(" px");
    formLayout->addRow(QString::fromUtf8("高度:"), m_heightSpinBox);

    mainLayout->addWidget(fmtGroup);

    auto* btnLayout = new QHBoxLayout;
    m_exportBtn = new QPushButton(QString::fromUtf8("导出"));
    m_exportBtn->setMinimumHeight(32);
    m_exportBtn->setDefault(true);
    // Uses global QSS
    m_cancelBtn = new QPushButton(QString::fromUtf8("取消"));
    m_cancelBtn->setMinimumHeight(32);
    btnLayout->addStretch();
    btnLayout->addWidget(m_cancelBtn);
    btnLayout->addWidget(m_exportBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_exportBtn, &QPushButton::clicked, this, &ExportDialog::onExport);
    connect(m_cancelBtn, &QPushButton::clicked, this, &QDialog::reject);
}

void ExportDialog::onBrowse()
{
    QString fmt = m_formatCombo->currentData().toString();
    QString filter = QString("%1 (*.%2)").arg(fmt.toUpper(), fmt);
    QString path = QFileDialog::getSaveFileName(this, QString::fromUtf8("保存图表"),
                                                 m_pathEdit->text(), filter);
    if (!path.isEmpty()) m_pathEdit->setText(path);
}

void ExportDialog::onFormatChanged(int)
{
    QString fmt = m_formatCombo->currentData().toString();
    QFileInfo info(m_pathEdit->text());
    m_pathEdit->setText(info.absolutePath() + "/" + info.completeBaseName() + "." + fmt);
}

void ExportDialog::onExport()
{
    QString dest = m_pathEdit->text().trimmed();
    if (dest.isEmpty()) {
        QMessageBox::warning(this, QString::fromUtf8("错误"),
                             QString::fromUtf8("请指定输出路径。"));
        return;
    }
    QString fmt = m_formatCombo->currentData().toString();
    QPixmap src(m_sourcePath);
    if (src.isNull()) {
        QMessageBox::warning(this, QString::fromUtf8("错误"),
                             QString::fromUtf8("无法加载源图像。"));
        return;
    }
    QPixmap toSave = src;
    int tw = m_widthSpinBox->value();
    int th = m_heightSpinBox->value();
    if (tw > 0 && th > 0)
        toSave = src.scaled(tw, th, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    else if (tw > 0)
        toSave = src.scaledToWidth(tw, Qt::SmoothTransformation);
    else if (th > 0)
        toSave = src.scaledToHeight(th, Qt::SmoothTransformation);

    int quality = (fmt == "jpg" || fmt == "jpeg") ? 95 : -1;
    if (toSave.save(dest, nullptr, quality)) {
        QMessageBox::information(this, QString::fromUtf8("成功"),
            QString::fromUtf8("图表已成功导出到:\n%1").arg(dest));
        accept();
    } else {
        QMessageBox::critical(this, QString::fromUtf8("导出失败"),
            QString::fromUtf8("保存图像失败，请检查文件权限和路径。"));
    }
}

void ExportDialog::applyDarkTheme()
{
    // Global stylesheet handles everything
}
