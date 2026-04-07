#pragma once

#include <QObject>
#include <QString>

#include "data/duckdb/duckdb_connection.h"
#include "data/duckdb/duckdb_engine.h"
#include "data/types/lookup_config.h"

namespace csvforge {

class LookupService : public QObject {
    Q_OBJECT
public:
    explicit LookupService(DuckDBEngine* engine, QObject* parent = nullptr);

    struct LookupResult {
        bool success = false;
        QString error;
        QString resultTableName;
        qint64 matchedRows = 0;
        qint64 unmatchedRows = 0;
        qint64 elapsedMs = 0;
    };

    QString buildLookupQuery(const LookupConfig& config);

    LookupResult executeLookup(const LookupConfig& config);

    DuckDBConnection::QueryResult previewLookup(const LookupConfig& config,
                                                 int maxRows = 100);

private:
    DuckDBEngine* m_engine;

    QString ensureTargetLoaded(const LookupConfig& config);
};

} // namespace csvforge
