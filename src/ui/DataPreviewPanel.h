#pragma once

#include <QWidget>
#include <QTableWidget>
#include <QTextBrowser>
#include <QLabel>
#include <QPushButton>

class DataManager;

/// Panel displaying a preview of the uploaded dataset in a table view.
/// Shows column names, data types, and first N rows.
class DataPreviewPanel : public QWidget
{
    Q_OBJECT

public:
    explicit DataPreviewPanel(QWidget* parent = nullptr);
    ~DataPreviewPanel() = default;

    /// Populate the table with data from the DataManager
    void loadFromDataManager(const DataManager* manager);

    /// Clear the preview
    void clear();

signals:
    void uploadRequested();

private:
    void setupUI();
    void applyDarkTheme();

    QLabel* m_titleLabel;
    QLabel* m_infoLabel;
    QTableWidget* m_table;
    QTextBrowser* m_markdownView;
    QPushButton* m_uploadBtn;
};
