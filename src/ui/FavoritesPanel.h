#pragma once

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QStringList>

class FavoritesPanel : public QWidget
{
    Q_OBJECT
public:
    explicit FavoritesPanel(QWidget* parent = nullptr);

    void addFavorite(const QString& imagePath);
    QStringList selectedPaths() const;
    int count() const;

signals:
    void batchExportRequested(const QStringList& paths);

private slots:
    void onRemoveSelected();
    void onExportSelected();
    void onClearAll();

private:
    void setupUI();

    QLabel* m_titleLabel;
    QListWidget* m_list;
    QPushButton* m_exportBtn;
    QPushButton* m_removeBtn;
    QPushButton* m_clearBtn;
    QStringList m_paths;
};
