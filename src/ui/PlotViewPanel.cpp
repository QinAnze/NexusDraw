#include "PlotViewPanel.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFileInfo>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QApplication>

// ---- ZoomGraphicsView ----
ZoomGraphicsView::ZoomGraphicsView(QWidget* parent) : QGraphicsView(parent) {
    setDragMode(QGraphicsView::ScrollHandDrag);
    setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    setRenderHint(QPainter::SmoothPixmapTransform);
    setFrameShape(QFrame::NoFrame);
    setStyleSheet("QGraphicsView { background-color: #fafafa; border: 2px solid #000; }");
}

void ZoomGraphicsView::wheelEvent(QWheelEvent* event) {
    double factor = (event->angleDelta().y() > 0) ? 1.15 : 1.0 / 1.15;
    scale(factor, factor);
}

void ZoomGraphicsView::mouseDoubleClickEvent(QMouseEvent*) {
    emit doubleClicked();
}

// ---- PlotViewPanel ----
PlotViewPanel::PlotViewPanel(QWidget* parent) : QWidget(parent) {
    setupUI();
}

void PlotViewPanel::setupUI() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(4);

    auto* headerLayout = new QHBoxLayout;
    m_titleLabel = new QLabel(QString::fromUtf8("图表输出"));
    m_titleLabel->setStyleSheet("font-size: 14px; font-weight: bold; padding: 8px 10px 0 10px; color: #1a1a1a;");
    headerLayout->addWidget(m_titleLabel);
    headerLayout->addStretch();

    m_exportBtn = new QPushButton(QString::fromUtf8("导出"));
    m_exportBtn->setFixedHeight(26);
    connect(m_exportBtn, &QPushButton::clicked, this, &PlotViewPanel::onExport);
    headerLayout->addWidget(m_exportBtn);

    auto* favBtn = new QPushButton(QString::fromUtf8("收藏"));
    favBtn->setFixedHeight(26);
    connect(favBtn, &QPushButton::clicked, this, [this]() {
        if (m_hasPlot && !m_imagePath.isEmpty())
            emit favoriteRequested(m_imagePath);
    });
    headerLayout->addWidget(favBtn);
    mainLayout->addLayout(headerLayout);

    m_scene = new QGraphicsScene(this);
    m_view = new ZoomGraphicsView(this);
    m_view->setScene(m_scene);

    connect(static_cast<ZoomGraphicsView*>(m_view), &ZoomGraphicsView::doubleClicked, this, [this]() {
        if (m_hasPlot) {
            auto* pw = new PlotWindow(m_originalPixmap);
            pw->setWindowTitle(QString::fromUtf8("图表 - %1").arg(QFileInfo(m_imagePath).fileName()));
            pw->resize(1000, 750);
            pw->show();
        }
    });

    mainLayout->addWidget(m_view, 1);
    clear();
}

void PlotViewPanel::loadPlot(const QString& imagePath) {
    if (!QFileInfo::exists(imagePath)) { clear(); emit plotLoaded(false); return; }
    m_imagePath = imagePath;
    m_originalPixmap = QPixmap(imagePath);
    if (m_originalPixmap.isNull()) { clear(); emit plotLoaded(false); return; }
    m_hasPlot = true;
    m_scene->clear();
    m_scene->addPixmap(m_originalPixmap);
    m_scene->setSceneRect(m_originalPixmap.rect());
    m_view->fitInView(m_scene->sceneRect(), Qt::KeepAspectRatio);
    m_titleLabel->setText(QString::fromUtf8("图表输出 - %1").arg(QFileInfo(imagePath).fileName()));
    emit plotLoaded(true);
}

void PlotViewPanel::clear() {
    m_scene->clear();
    m_imagePath.clear();
    m_originalPixmap = QPixmap();
    m_hasPlot = false;
    m_titleLabel->setText(QString::fromUtf8("图表输出"));
    // Show placeholder text
    m_scene->addText(QString::fromUtf8("尚未生成图表\n\n鼠标滚轮缩放 | 拖动平移 | 双击新窗口打开"),
                      QFont("Georgia", 12));
}

bool PlotViewPanel::hasPlot() const { return m_hasPlot; }
QString PlotViewPanel::currentImagePath() const { return m_imagePath; }

void PlotViewPanel::onExport() {
    if (m_hasPlot && !m_imagePath.isEmpty())
        emit exportRequested(m_imagePath);
}

// ---- PlotWindow (double-click standalone) ----
PlotWindow::PlotWindow(const QPixmap& pixmap, QWidget* parent) : QWidget(parent, Qt::Window) {
    setAttribute(Qt::WA_DeleteOnClose);
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    auto* view = new ZoomGraphicsView(this);
    auto* scene = new QGraphicsScene(this);
    scene->addPixmap(pixmap);
    scene->setSceneRect(pixmap.rect());
    view->setScene(scene);
    view->fitInView(scene->sceneRect(), Qt::KeepAspectRatio);

    layout->addWidget(view);
}
