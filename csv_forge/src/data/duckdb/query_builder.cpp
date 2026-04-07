#include "data/duckdb/query_builder.h"
#include "utils/string_utils.h"

namespace csvforge {

QueryBuilder::QueryBuilder() = default;

// ---------------------------------------------------------------------------
// SELECT
// ---------------------------------------------------------------------------

QueryBuilder& QueryBuilder::select(const QStringList& columns)
{
    m_selectCols = columns;
    m_count = false;
    m_distinct = false;
    return *this;
}

QueryBuilder& QueryBuilder::selectAll()
{
    m_selectCols.clear();
    m_count = false;
    m_distinct = false;
    return *this;
}

QueryBuilder& QueryBuilder::selectCount()
{
    m_count = true;
    m_selectCols.clear();
    m_distinct = false;
    return *this;
}

QueryBuilder& QueryBuilder::selectDistinct(const QString& column)
{
    m_distinct = true;
    m_selectCols = { column };
    m_count = false;
    return *this;
}

// ---------------------------------------------------------------------------
// FROM
// ---------------------------------------------------------------------------

QueryBuilder& QueryBuilder::from(const QString& table)
{
    m_fromClause = quoteName(table);
    return *this;
}

QueryBuilder& QueryBuilder::fromCSV(const QString& filePath)
{
    m_fromClause = QStringLiteral("read_csv_auto('%1')").arg(escapeSQL(filePath));
    return *this;
}

// ---------------------------------------------------------------------------
// WHERE
// ---------------------------------------------------------------------------

QueryBuilder& QueryBuilder::where(const FilterGroup& filters)
{
    if (!filters.isEmpty()) {
        m_whereClause = filterGroupToSQL(filters);
    }
    return *this;
}

QueryBuilder& QueryBuilder::whereRaw(const QString& clause)
{
    m_whereClause = clause;
    return *this;
}

// ---------------------------------------------------------------------------
// ORDER BY
// ---------------------------------------------------------------------------

QueryBuilder& QueryBuilder::orderBy(const QString& column, bool ascending)
{
    m_orderClauses << QStringLiteral("%1 %2")
                          .arg(quoteName(column),
                               ascending ? QStringLiteral("ASC") : QStringLiteral("DESC"));
    return *this;
}

QueryBuilder& QueryBuilder::orderBy(const std::vector<std::pair<QString, bool>>& sorts)
{
    for (const auto& [col, asc] : sorts) {
        orderBy(col, asc);
    }
    return *this;
}

// ---------------------------------------------------------------------------
// LIMIT / OFFSET
// ---------------------------------------------------------------------------

QueryBuilder& QueryBuilder::limit(qint64 count)
{
    m_limit = count;
    return *this;
}

QueryBuilder& QueryBuilder::offset(qint64 off)
{
    m_offset = off;
    return *this;
}

// ---------------------------------------------------------------------------
// Build
// ---------------------------------------------------------------------------

QString QueryBuilder::build() const
{
    QStringList parts;

    // SELECT clause
    if (m_count) {
        parts << QStringLiteral("SELECT COUNT(*)");
    } else if (m_selectCols.isEmpty()) {
        if (m_distinct) {
            parts << QStringLiteral("SELECT DISTINCT *");
        } else {
            parts << QStringLiteral("SELECT *");
        }
    } else {
        QStringList quoted;
        quoted.reserve(m_selectCols.size());
        for (const QString& c : m_selectCols) {
            if (c == QLatin1String("*")) {
                quoted << c;
            } else {
                quoted << quoteName(c);
            }
        }
        if (m_distinct) {
            parts << QStringLiteral("SELECT DISTINCT %1").arg(quoted.join(QStringLiteral(", ")));
        } else {
            parts << QStringLiteral("SELECT %1").arg(quoted.join(QStringLiteral(", ")));
        }
    }

    // FROM clause
    if (!m_fromClause.isEmpty()) {
        parts << QStringLiteral("FROM %1").arg(m_fromClause);
    }

    // WHERE clause
    if (!m_whereClause.isEmpty()) {
        parts << QStringLiteral("WHERE %1").arg(m_whereClause);
    }

    // ORDER BY
    if (!m_orderClauses.isEmpty()) {
        parts << QStringLiteral("ORDER BY %1").arg(m_orderClauses.join(QStringLiteral(", ")));
    }

    // LIMIT
    if (m_limit >= 0) {
        parts << QStringLiteral("LIMIT %1").arg(m_limit);
    }

    // OFFSET
    if (m_offset >= 0) {
        parts << QStringLiteral("OFFSET %1").arg(m_offset);
    }

    return parts.join(QLatin1Char(' '));
}

// ---------------------------------------------------------------------------
// Reset
// ---------------------------------------------------------------------------

void QueryBuilder::reset()
{
    m_selectCols.clear();
    m_fromClause.clear();
    m_whereClause.clear();
    m_orderClauses.clear();
    m_limit = -1;
    m_offset = -1;
    m_distinct = false;
    m_count = false;
}

// ---------------------------------------------------------------------------
// Filter → SQL translation
// ---------------------------------------------------------------------------

QString QueryBuilder::filterGroupToSQL(const FilterGroup& group)
{
    if (group.isEmpty()) {
        return {};
    }

    QStringList clauses;

    for (const auto& rule : group.rules) {
        if (!rule.isValid()) continue;
        clauses << filterRuleToSQL(rule);
    }

    for (const auto& sub : group.subGroups) {
        QString subSQL = filterGroupToSQL(sub);
        if (!subSQL.isEmpty()) {
            clauses << QStringLiteral("(%1)").arg(subSQL);
        }
    }

    if (clauses.isEmpty()) {
        return {};
    }

    const QString joiner = (group.combination == FilterCombination::And)
                               ? QStringLiteral(" AND ")
                               : QStringLiteral(" OR ");

    return clauses.join(joiner);
}

QString QueryBuilder::filterRuleToSQL(const FilterRule& rule)
{
    const QString colRef = quoteName(rule.columnName);
    return filterOperatorToSQL(rule.op, colRef, rule.value, rule.secondValue,
                               rule.inListValues, rule.caseSensitive);
}

QString QueryBuilder::filterOperatorToSQL(FilterOperator op,
                                          const QString& columnRef,
                                          const QVariant& value,
                                          const QVariant& secondValue,
                                          const QStringList& inList,
                                          bool caseSensitive)
{
    const QString strVal = escapeSQL(value.toString());

    auto sqlLiteral = [&]() -> QString {
        if (isNumericString(value.toString())) {
            return value.toString();
        }
        return QStringLiteral("'%1'").arg(strVal);
    };

    auto colOrLower = [&](const QString& col) -> QString {
        return caseSensitive ? col : QStringLiteral("LOWER(%1)").arg(col);
    };

    auto valOrLower = [&](const QString& v) -> QString {
        return caseSensitive ? v : v.toLower();
    };

    switch (op) {
    case FilterOperator::Equals:
        return QStringLiteral("%1 = %2").arg(columnRef, sqlLiteral());

    case FilterOperator::NotEquals:
        return QStringLiteral("%1 != %2").arg(columnRef, sqlLiteral());

    case FilterOperator::CaseSensitiveEquals:
        return QStringLiteral("%1 = '%2'").arg(columnRef, strVal);

    case FilterOperator::ExactMatch:
        return QStringLiteral("CAST(%1 AS VARCHAR) = '%2'").arg(columnRef, strVal);

    case FilterOperator::Contains:
        return QStringLiteral("%1 LIKE '%%%2%%'")
            .arg(colOrLower(columnRef), valOrLower(strVal));

    case FilterOperator::NotContains:
        return QStringLiteral("%1 NOT LIKE '%%%2%%'")
            .arg(colOrLower(columnRef), valOrLower(strVal));

    case FilterOperator::StartsWith:
        return QStringLiteral("%1 LIKE '%2%%'")
            .arg(colOrLower(columnRef), valOrLower(strVal));

    case FilterOperator::EndsWith:
        return QStringLiteral("%1 LIKE '%%%2'")
            .arg(colOrLower(columnRef), valOrLower(strVal));

    case FilterOperator::GreaterThan:
        return QStringLiteral("%1 > %2").arg(columnRef, sqlLiteral());

    case FilterOperator::GreaterEqual:
        return QStringLiteral("%1 >= %2").arg(columnRef, sqlLiteral());

    case FilterOperator::LessThan:
        return QStringLiteral("%1 < %2").arg(columnRef, sqlLiteral());

    case FilterOperator::LessEqual:
        return QStringLiteral("%1 <= %2").arg(columnRef, sqlLiteral());

    case FilterOperator::Between: {
        const QString v1 = escapeSQL(value.toString());
        const QString v2 = escapeSQL(secondValue.toString());
        const bool numeric = isNumericString(value.toString())
                             && isNumericString(secondValue.toString());
        if (numeric) {
            return QStringLiteral("%1 BETWEEN %2 AND %3")
                .arg(columnRef, v1, v2);
        }
        return QStringLiteral("%1 BETWEEN '%2' AND '%3'")
            .arg(columnRef, v1, v2);
    }

    case FilterOperator::IsZero:
        return QStringLiteral("%1 = 0").arg(columnRef);

    case FilterOperator::NonZero:
        return QStringLiteral("%1 != 0").arg(columnRef);

    case FilterOperator::IsPositive:
        return QStringLiteral("%1 > 0").arg(columnRef);

    case FilterOperator::IsNegative:
        return QStringLiteral("%1 < 0").arg(columnRef);

    case FilterOperator::IsEmpty:
        return QStringLiteral("(%1 IS NULL OR CAST(%1 AS VARCHAR) = '')")
            .arg(columnRef);

    case FilterOperator::IsNotEmpty:
        return QStringLiteral("(%1 IS NOT NULL AND CAST(%1 AS VARCHAR) != '')")
            .arg(columnRef);

    case FilterOperator::IsNull:
        return QStringLiteral("%1 IS NULL").arg(columnRef);

    case FilterOperator::IsNotNull:
        return QStringLiteral("%1 IS NOT NULL").arg(columnRef);

    case FilterOperator::InList: {
        QStringList escaped;
        escaped.reserve(inList.size());
        for (const QString& item : inList) {
            escaped << QStringLiteral("'%1'").arg(escapeSQL(item));
        }
        return QStringLiteral("%1 IN (%2)")
            .arg(columnRef, escaped.join(QStringLiteral(", ")));
    }

    case FilterOperator::NotInList: {
        QStringList escaped;
        escaped.reserve(inList.size());
        for (const QString& item : inList) {
            escaped << QStringLiteral("'%1'").arg(escapeSQL(item));
        }
        return QStringLiteral("%1 NOT IN (%2)")
            .arg(columnRef, escaped.join(QStringLiteral(", ")));
    }

    case FilterOperator::Regex:
        return QStringLiteral("regexp_matches(CAST(%1 AS VARCHAR), '%2')")
            .arg(columnRef, strVal);

    case FilterOperator::AboveAverage:
        // Uses a subquery placeholder: the caller must set the FROM table context.
        // We generate a self-contained correlated expression.
        return QStringLiteral("%1 > (SELECT AVG(%1) FROM __SOURCE_TABLE__)")
            .arg(columnRef);

    case FilterOperator::BelowAverage:
        return QStringLiteral("%1 < (SELECT AVG(%1) FROM __SOURCE_TABLE__)")
            .arg(columnRef);
    }

    // Fallback – treat as equals
    return QStringLiteral("%1 = %2").arg(columnRef, sqlLiteral());
}

} // namespace csvforge
