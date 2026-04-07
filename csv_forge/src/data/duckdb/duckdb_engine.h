#pragma once

#include <QObject>
#include <QStringList>
#include <memory>

#include "data/duckdb/duckdb_connection.h"
#include "data/types/column_schema.h"

namespace csvforge {

struct CSVImportOptions {
    QChar delimiter = QLatin1Char(',');
    QChar quoteChar = QLatin1Char('"');
    bool hasHeader = true;
    QString encoding = QStringLiteral("utf-8");
    int skipRows = 0;
    bool autoDetect = true;
    int sampleSize = -1;   // -1 = auto
    bool ignoreErrors = false;
};

class DuckDBEngine : public QObject {
    Q_OBJECT
public:
    explicit DuckDBEngine(QObject* parent = nullptr);
    ~DuckDBEngine() override;

    bool initialize();
    bool isInitialized() const;

    // Table management
    QString loadCSVAsTable(const QString& filePath, const QString& tableName,
                           const CSVImportOptions& options);
    bool dropTable(const QString& tableName);
    bool tableExists(const QString& tableName) const;
    QStringList listTables() const;

    // Schema
    ColumnSchemaList getTableSchema(const QString& tableName) const;
    qint64 getRowCount(const QString& tableName) const;
    qint64 getFilteredRowCount(const QString& tableName,
                               const QString& whereClause) const;

    // Query execution
    DuckDBConnection::QueryResult executeQuery(const QString& sql);
    QVariant executeScalar(const QString& sql);

    // CSV direct scan (read-only, no import)
    DuckDBConnection::QueryResult scanCSV(const QString& filePath,
                                           const QString& sql);

    // Connection access
    DuckDBConnection& connection();
    const DuckDBConnection& connection() const;

signals:
    void errorOccurred(const QString& error);
    void tableLoaded(const QString& tableName, qint64 rowCount);

private:
    std::unique_ptr<DuckDBConnection> m_connection;
    bool m_initialized = false;
};

} // namespace csvforge
