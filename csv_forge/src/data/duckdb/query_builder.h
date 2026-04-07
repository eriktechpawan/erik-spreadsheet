#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>
#include <vector>
#include <utility>

#include "data/types/filter_rule.h"

namespace csvforge {

class QueryBuilder {
public:
    QueryBuilder();

    // SELECT
    QueryBuilder& select(const QStringList& columns);
    QueryBuilder& selectAll();
    QueryBuilder& selectCount();
    QueryBuilder& selectDistinct(const QString& column);

    // FROM
    QueryBuilder& from(const QString& table);
    QueryBuilder& fromCSV(const QString& filePath);

    // WHERE
    QueryBuilder& where(const FilterGroup& filters);
    QueryBuilder& whereRaw(const QString& clause);

    // ORDER BY
    QueryBuilder& orderBy(const QString& column, bool ascending = true);
    QueryBuilder& orderBy(const std::vector<std::pair<QString, bool>>& sorts);

    // LIMIT / OFFSET
    QueryBuilder& limit(qint64 count);
    QueryBuilder& offset(qint64 off);

    // Build the final SQL string
    QString build() const;

    // Static helpers for filter → SQL translation
    static QString filterGroupToSQL(const FilterGroup& group);
    static QString filterRuleToSQL(const FilterRule& rule);
    static QString filterOperatorToSQL(FilterOperator op, const QString& columnRef,
                                       const QVariant& value, const QVariant& secondValue,
                                       const QStringList& inList, bool caseSensitive);

    void reset();

private:
    QStringList m_selectCols;
    QString m_fromClause;
    QString m_whereClause;
    QStringList m_orderClauses;
    qint64 m_limit = -1;
    qint64 m_offset = -1;
    bool m_distinct = false;
    bool m_count = false;
};

} // namespace csvforge
