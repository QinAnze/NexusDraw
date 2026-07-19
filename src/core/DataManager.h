#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <QVector>
#include <QVariant>

/// Parses and manages uploaded datasets (CSV, TSV, etc.).
/// Extracts metadata (columns, types, sample rows) for LLM context.
class DataManager : public QObject
{
    Q_OBJECT

public:
    explicit DataManager(QObject* parent = nullptr);
    ~DataManager() = default;

    /// Load a dataset from file path. Returns true on success.
    bool loadFile(const QString& filePath);

    /// Get the current dataset file path
    QString filePath() const;

    /// Get the filename only
    QString fileName() const;

    /// Get column names
    QStringList columnNames() const;

    /// Get column types (inferred from data)
    QStringList columnTypes() const;

    /// Get the number of rows
    int rowCount() const;

    /// Get the number of columns
    int columnCount() const;

    /// Get all data as a 2D table (for preview)
    const QVector<QVector<QVariant>>& data() const;

    /// Get first N rows as formatted string for LLM prompt
    QString sampleDataString(int maxRows = 5) const;

    /// Build a complete JSON info object for the LLM
    QJsonObject buildDatasetInfo(const QString& outputImagePath) const;

    /// Check if a dataset is currently loaded
    bool isLoaded() const;

signals:
    void dataLoaded(int rowCount, int columnCount);
    void loadError(const QString& error);

private:
    /// Infer the data type of a column from its values
    QString inferColumnType(int col) const;

    /// Detect delimiter from file content
    QChar detectDelimiter(const QString& firstLine) const;

    QString m_filePath;
    QStringList m_headers;
    QStringList m_columnTypes;
    QVector<QVector<QVariant>> m_data;
    bool m_loaded = false;
};
