#pragma once

#include <QString>
#include <QVariant>
#include <vector>

#include "duckdb.h"

namespace csvforge {

class DuckDBConnection {
public:
    DuckDBConnection();
    ~DuckDBConnection();

    DuckDBConnection(const DuckDBConnection&) = delete;
    DuckDBConnection& operator=(const DuckDBConnection&) = delete;
    DuckDBConnection(DuckDBConnection&& other) noexcept;
    DuckDBConnection& operator=(DuckDBConnection&& other) noexcept;

    bool isOpen() const;
    QString lastError() const;

    struct QueryResult {
        bool success = false;
        QString error;
        std::vector<QString> columnNames;
        std::vector<QString> columnTypes;
        std::vector<std::vector<QVariant>> rows;
        qint64 rowsAffected = 0;
        qint64 totalRows = 0;
    };

    QueryResult execute(const QString& sql);
    QueryResult execute(const QString& sql, const std::vector<QVariant>& params);

    QVariant executeScalar(const QString& sql);
    qint64 executeCount(const QString& sql);

    duckdb_connection rawConnection() const;

private:
    duckdb_database m_database = nullptr;
    duckdb_connection m_connection = nullptr;
    QString m_lastError;

    void cleanup();
    static QVariant extractValue(duckdb_result* result, idx_t row, idx_t col);
};

} // namespace csvforge
