#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QPixmap>
#include <QGraphicsView>
#include <QGraphicsScene>
/// Displays the generated plot with mouse pan, wheel zoom, double-click new window.
class PlotViewPanel : public QWidget
{
    Q_OBJECT

public:
    explicit PlotViewPanel(QWidget* parent = nullptr);

    void loadPlot(const QString& imagePath);
    void clear();
    bool hasPlot() const;
    QString currentImagePath() const;

signals:
    void exportRequested(const QString& imagePath);
    void plotLoaded(bool success);

private slots:
    void onExport();

private:
    void setupUI();

    QLabel* m_titleLabel;
    QGraphicsView* m_view;
    QGraphicsScene* m_scene;
    QPushButton* m_exportBtn;

    QString m_imagePath;
    QPixmap m_originalPixmap;
    bool m_hasPlot = false;
};

/// Custom QGraphicsView with wheel zoom + double-click signal
class ZoomGraphicsView : public QGraphicsView
{
    Q_OBJECT
public:
    explicit ZoomGraphicsView(QWidget* parent = nullptr);
signals:
    void doubleClicked();
protected:
    void wheelEvent(QWheelEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
};

/// Standalone window for viewing plot at full resolution
class PlotWindow : public QWidget
{
    Q_OBJECT
public:
    explicit PlotWindow(const QPixmap& pixmap, QWidget* parent = nullptr);
private:
    QGraphicsView* m_view;
    QGraphicsScene* m_scene;
};
