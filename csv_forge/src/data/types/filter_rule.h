#pragma once

#include <QString>
#include <QStringList>
#include <QVariant>
#include <QDateTime>
#include <vector>
#include <memory>

namespace csvforge {

enum class FilterOperator {
    Equals, NotEquals,
    Contains, NotContains,
    StartsWith, EndsWith,
    ExactMatch,
    GreaterThan, GreaterEqual, LessThan, LessEqual,
    Between,
    IsZero, NonZero,
    IsPositive, IsNegative,
    IsEmpty, IsNotEmpty,
    IsNull, IsNotNull,
    InList, NotInList,
    Regex,
    AboveAverage, BelowAverage,
    CaseSensitiveEquals
};

enum class FilterCombination {
    And,
    Or
};

struct FilterRule {
    QString columnName;
    FilterOperator op = FilterOperator::Equals;
    QVariant value;           // primary value
    QVariant secondValue;     // for Between
    QStringList inListValues; // for InList / NotInList
    bool caseSensitive = false;

    bool isValid() const;
    QString toDisplayString() const;
};

struct FilterGroup {
    FilterCombination combination = FilterCombination::And;
    std::vector<FilterRule> rules;
    std::vector<FilterGroup> subGroups; // nested groups

    bool isEmpty() const;
    int ruleCount() const;
};

struct FilterPreset {
    QString name;
    FilterGroup group;
    QDateTime createdAt;
};

} // namespace csvforge
