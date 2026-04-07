#pragma once

#include <QObject>
#include <QStringList>
#include <QVariant>
#include <vector>

#include "data/types/filter_rule.h"

namespace csvforge {

class FilterState : public QObject {
    Q_OBJECT
public:
    explicit FilterState(QObject* parent = nullptr);

    // Current filter
    const FilterGroup& currentFilter() const;
    void setFilter(const FilterGroup& group);
    void clearFilter();
    bool hasActiveFilter() const;

    // Add/remove rules
    void addRule(const FilterRule& rule);
    void removeRule(int index);
    void updateRule(int index, const FilterRule& rule);

    // Combination mode
    FilterCombination combination() const;
    void setCombination(FilterCombination combo);

    // Presets
    void savePreset(const QString& name);
    void loadPreset(const QString& name);
    void deletePreset(const QString& name);
    QStringList presetNames() const;
    const std::vector<FilterPreset>& presets() const;

    // Quick filters
    void applyQuickFilter(const QString& columnName, FilterOperator op,
                          const QVariant& value = {});

    // Generate SQL WHERE clause
    QString toWhereClause() const;

    // Recent filters
    void addToRecent(const FilterGroup& group);
    const std::vector<FilterGroup>& recentFilters() const;

signals:
    void filterChanged();
    void presetsChanged();

private:
    FilterGroup m_currentFilter;
    std::vector<FilterPreset> m_presets;
    std::vector<FilterGroup> m_recentFilters;
    static constexpr int kMaxRecentFilters = 10;
};

} // namespace csvforge
