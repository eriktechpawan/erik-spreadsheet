#include "data/types/filter_rule.h"

namespace csvforge {

// ---------------------------------------------------------------------------
// FilterRule
// ---------------------------------------------------------------------------

bool FilterRule::isValid() const
{
    if (columnName.isEmpty())
        return false;

    // Operators that don't require a value
    switch (op) {
    case FilterOperator::IsEmpty:
    case FilterOperator::IsNotEmpty:
    case FilterOperator::IsNull:
    case FilterOperator::IsNotNull:
    case FilterOperator::IsZero:
    case FilterOperator::NonZero:
    case FilterOperator::IsPositive:
    case FilterOperator::IsNegative:
    case FilterOperator::AboveAverage:
    case FilterOperator::BelowAverage:
        return true;
    default:
        break;
    }

    // InList / NotInList need at least one item
    if (op == FilterOperator::InList || op == FilterOperator::NotInList)
        return !inListValues.isEmpty();

    // Between needs both values
    if (op == FilterOperator::Between)
        return value.isValid() && secondValue.isValid();

    // All others need a primary value
    return value.isValid();
}

static QString operatorToString(FilterOperator op)
{
    switch (op) {
    case FilterOperator::Equals:              return QStringLiteral("=");
    case FilterOperator::NotEquals:           return QStringLiteral("≠");
    case FilterOperator::Contains:            return QStringLiteral("contains");
    case FilterOperator::NotContains:         return QStringLiteral("not contains");
    case FilterOperator::StartsWith:          return QStringLiteral("starts with");
    case FilterOperator::EndsWith:            return QStringLiteral("ends with");
    case FilterOperator::ExactMatch:          return QStringLiteral("exact match");
    case FilterOperator::GreaterThan:         return QStringLiteral(">");
    case FilterOperator::GreaterEqual:        return QStringLiteral("≥");
    case FilterOperator::LessThan:            return QStringLiteral("<");
    case FilterOperator::LessEqual:           return QStringLiteral("≤");
    case FilterOperator::Between:             return QStringLiteral("between");
    case FilterOperator::IsZero:              return QStringLiteral("is zero");
    case FilterOperator::NonZero:             return QStringLiteral("non-zero");
    case FilterOperator::IsPositive:          return QStringLiteral("is positive");
    case FilterOperator::IsNegative:          return QStringLiteral("is negative");
    case FilterOperator::IsEmpty:             return QStringLiteral("is empty");
    case FilterOperator::IsNotEmpty:          return QStringLiteral("is not empty");
    case FilterOperator::IsNull:              return QStringLiteral("is null");
    case FilterOperator::IsNotNull:           return QStringLiteral("is not null");
    case FilterOperator::InList:              return QStringLiteral("in list");
    case FilterOperator::NotInList:           return QStringLiteral("not in list");
    case FilterOperator::Regex:               return QStringLiteral("matches regex");
    case FilterOperator::AboveAverage:        return QStringLiteral("above average");
    case FilterOperator::BelowAverage:        return QStringLiteral("below average");
    case FilterOperator::CaseSensitiveEquals: return QStringLiteral("= (case)");
    }
    return QStringLiteral("?");
}

QString FilterRule::toDisplayString() const
{
    const QString opStr = operatorToString(op);

    switch (op) {
    case FilterOperator::IsEmpty:
    case FilterOperator::IsNotEmpty:
    case FilterOperator::IsNull:
    case FilterOperator::IsNotNull:
    case FilterOperator::IsZero:
    case FilterOperator::NonZero:
    case FilterOperator::IsPositive:
    case FilterOperator::IsNegative:
    case FilterOperator::AboveAverage:
    case FilterOperator::BelowAverage:
        return QStringLiteral("%1 %2").arg(columnName, opStr);
    case FilterOperator::Between:
        return QStringLiteral("%1 %2 %3 and %4")
            .arg(columnName, opStr, value.toString(), secondValue.toString());
    case FilterOperator::InList:
    case FilterOperator::NotInList:
        return QStringLiteral("%1 %2 (%3)")
            .arg(columnName, opStr, inListValues.join(QStringLiteral(", ")));
    default:
        return QStringLiteral("%1 %2 %3").arg(columnName, opStr, value.toString());
    }
}

// ---------------------------------------------------------------------------
// FilterGroup
// ---------------------------------------------------------------------------

bool FilterGroup::isEmpty() const
{
    return rules.empty() && subGroups.empty();
}

int FilterGroup::ruleCount() const
{
    int count = static_cast<int>(rules.size());
    for (const auto& sg : subGroups) {
        count += sg.ruleCount();
    }
    return count;
}

} // namespace csvforge
