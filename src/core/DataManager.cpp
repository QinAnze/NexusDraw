#include "DataManager.h"

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
#include <QDebug>

DataManager::DataManager(QObject* parent)
    : QObject(parent)
{
}

bool DataManager::isLoaded() const
{
    return m_loaded;
}

QString DataManager::filePath() const { return m_filePath; }
QString DataManager::fileName() const { return QFileInfo(m_filePath).fileName(); }
QStringList DataManager::columnNames() const { return m_headers; }
QStringList DataManager::columnTypes() const { return m_columnTypes; }
int DataManager::rowCount() const { return m_data.size(); }
int DataManager::columnCount() const { return m_headers.size(); }
const QVector<QVector<QVariant>>& DataManager::data() const { return m_data; }

QChar DataManager::detectDelimiter(const QString& firstLine) const
{
    int commas = firstLine.count(',');
    int tabs = firstLine.count('\t');
    int semicolons = firstLine.count(';');

    if (tabs > commas && tabs > semicolons) return '\t';
    if (semicolons > commas) return ';';
    return ',';
}

bool DataManager::loadFile(const QString& filePath)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        emit loadError(tr("Cannot open file: %1").arg(file.errorString()));
        return false;
    }

    m_filePath = filePath;
    m_headers.clear();
    m_columnTypes.clear();
    m_data.clear();

    QTextStream stream(&file);
    // Qt6 defaults to UTF-8, no setCodec needed

    // Detect BOM
    QString content = stream.readAll();
    if (content.startsWith(QChar(0xFEFF))) {
        content.remove(0, 1);
    }

    QStringList lines = content.split(QRegularExpression("[\r\n]"), Qt::SkipEmptyParts);
    if (lines.isEmpty()) {
        m_loaded = false;
        emit loadError(tr("File is empty."));
        return false;
    }

    // Detect delimiter
    QChar delimiter = detectDelimiter(lines.first());

    // Parse header
    // Handle quoted fields
    auto splitCSV = [delimiter](const QString& line) -> QStringList {
        QStringList fields;
        QString field;
        bool inQuotes = false;
        for (int i = 0; i < line.length(); ++i) {
            QChar c = line[i];
            if (c == '"') {
                inQuotes = !inQuotes;
            } else if (c == delimiter && !inQuotes) {
                fields.append(field.trimmed());
                field.clear();
            } else {
                field += c;
            }
        }
        fields.append(field.trimmed());
        return fields;
    };

    m_headers = splitCSV(lines.first());
    if (m_headers.isEmpty()) {
        m_loaded = false;
        emit loadError(tr("No columns found in header."));
        return false;
    }

    // Clean header names (remove quotes)
    for (auto& h : m_headers) {
        h.remove('"');
    }

    // Parse data rows (limit to 100000 rows for performance)
    int maxRows = qMin(lines.size() - 1, 100000);
    m_data.reserve(maxRows);

    for (int i = 1; i <= maxRows; ++i) {
        QStringList fields = splitCSV(lines[i]);
        QVector<QVariant> row;
        row.reserve(m_headers.size());
        for (int j = 0; j < m_headers.size(); ++j) {
            if (j < fields.size()) {
                QString val = fields[j];
                val.remove('"');
                row.append(QVariant(val));
            } else {
                row.append(QVariant(QString()));
            }
        }
        m_data.append(row);
    }

    // Infer column types
    m_columnTypes.clear();
    for (int col = 0; col < m_headers.size(); ++col) {
        m_columnTypes.append(inferColumnType(col));
    }

    m_loaded = true;
    emit dataLoaded(m_data.size(), m_headers.size());

    qDebug() << "Loaded" << m_data.size() << "rows," << m_headers.size() << "columns";
    return true;
}

QString DataManager::inferColumnType(int col) const
{
    if (m_data.isEmpty()) return "unknown";

    int numericCount = 0;
    int intCount = 0;
    int totalChecked = qMin(m_data.size(), 100);

    for (int row = 0; row < totalChecked; ++row) {
        QString val = m_data[row][col].toString().trimmed();
        if (val.isEmpty()) continue;

        bool ok = false;
        double d = val.toDouble(&ok);
        if (ok) {
            numericCount++;
            // Check if it's an integer
            if (d == qint64(d)) intCount++;
        }
    }

    double numericRatio = static_cast<double>(numericCount) / totalChecked;
    double intRatio = static_cast<double>(intCount) / numericCount;

    if (numericRatio > 0.8) {
        if (intRatio > 0.9) return "integer";
        return "numeric";
    }
    return "categorical";
}

QString DataManager::sampleDataString(int maxRows) const
{
    if (m_data.isEmpty() || m_headers.isEmpty()) return "";

    QString result;

    // Build a markdown table
    // Header
    result += "| " + m_headers.join(" | ") + " |\n";
    result += "|" + QString(" --- |").repeated(m_headers.size()) + "\n";

    // Rows
    int rowsToShow = qMin(m_data.size(), maxRows);
    for (int r = 0; r < rowsToShow; ++r) {
        QStringList rowVals;
        for (int c = 0; c < m_headers.size(); ++c) {
            rowVals.append(m_data[r][c].toString());
        }
        result += "| " + rowVals.join(" | ") + " |\n";
    }

    return result;
}

QJsonObject DataManager::buildDatasetInfo(const QString& outputImagePath) const
{
    QJsonObject info;
    info["filename"] = fileName();
    info["filePath"] = m_filePath;
    info["outputPath"] = outputImagePath;
    info["rowCount"] = m_data.size();
    info["columnCount"] = m_headers.size();

    QJsonArray columns;
    for (int i = 0; i < m_headers.size(); ++i) {
        QJsonObject col;
        col["name"] = m_headers[i];
        col["type"] = i < m_columnTypes.size() ? m_columnTypes[i] : "unknown";
        columns.append(col);
    }
    info["columns"] = columns;

    info["sampleData"] = sampleDataString(5);

    // Basic stats for numeric columns
    QJsonObject stats;
    for (int c = 0; c < m_headers.size(); ++c) {
        if (c < m_columnTypes.size() &&
            (m_columnTypes[c] == "numeric" || m_columnTypes[c] == "integer")) {

            QJsonObject colStats;
            double minVal = std::numeric_limits<double>::max();
            double maxVal = std::numeric_limits<double>::lowest();
            int nonNullCount = 0;
            int rowsToCheck = qMin(m_data.size(), 5000);

            for (int r = 0; r < rowsToCheck; ++r) {
                QString val = m_data[r][c].toString().trimmed();
                if (val.isEmpty()) continue;
                bool ok = false;
                double d = val.toDouble(&ok);
                if (ok) {
                    minVal = qMin(minVal, d);
                    maxVal = qMax(maxVal, d);
                    nonNullCount++;
                }
            }

            if (nonNullCount > 0) {
                colStats["min"] = minVal;
                colStats["max"] = maxVal;
                colStats["count"] = nonNullCount;
            }
            stats[m_headers[c]] = colStats;
        }
    }
    info["stats"] = stats;

    return info;
}
