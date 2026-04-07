#include "data/model/filter_state.h"
#include "data/duckdb/query_builder.h"

#include <QDateTime>
#include <algorithm>

namespace csvforge {

FilterState::FilterState(QObject* parent)
    : QObject(parent)
{
}

const FilterGroup& FilterState::currentFilter() const
{
    return m_currentFilter;
}

void FilterState::setFilter(const FilterGroup& group)
{
    m_currentFilter = group;
    emit filterChanged();
}

void FilterState::clearFilter()
{
    m_currentFilter = FilterGroup{};
    emit filterChanged();
}

bool FilterState::hasActiveFilter() const
{
    return !m_currentFilter.isEmpty();
}

void FilterState::addRule(const FilterRule& rule)
{
    m_currentFilter.rules.push_back(rule);
    emit filterChanged();
}

void FilterState::removeRule(int index)
{
    if (index >= 0 && index < static_cast<int>(m_currentFilter.rules.size())) {
        m_currentFilter.rules.erase(m_currentFilter.rules.begin() + index);
        emit filterChanged();
    }
}

void FilterState::updateRule(int index, const FilterRule& rule)
{
    if (index >= 0 && index < static_cast<int>(m_currentFilter.rules.size())) {
        m_currentFilter.rules[static_cast<size_t>(index)] = rule;
        emit filterChanged();
    }
}

FilterCombination FilterState::combination() const
{
    return m_currentFilter.combination;
}

void FilterState::setCombination(FilterCombination combo)
{
    m_currentFilter.combination = combo;
    emit filterChanged();
}

void FilterState::savePreset(const QString& name)
{
    // Update existing preset with same name, or add new
    auto it = std::find_if(m_presets.begin(), m_presets.end(),
                           [&name](const FilterPreset& p) { return p.name == name; });
    if (it != m_presets.end()) {
        it->group = m_currentFilter;
        it->createdAt = QDateTime::currentDateTime();
    } else {
        FilterPreset preset;
        preset.name = name;
        preset.group = m_currentFilter;
        preset.createdAt = QDateTime::currentDateTime();
        m_presets.push_back(std::move(preset));
    }
    emit presetsChanged();
}

void FilterState::loadPreset(const QString& name)
{
    auto it = std::find_if(m_presets.begin(), m_presets.end(),
                           [&name](const FilterPreset& p) { return p.name == name; });
    if (it != m_presets.end()) {
        m_currentFilter = it->group;
        emit filterChanged();
    }
}

void FilterState::deletePreset(const QString& name)
{
    auto it = std::find_if(m_presets.begin(), m_presets.end(),
                           [&name](const FilterPreset& p) { return p.name == name; });
    if (it != m_presets.end()) {
        m_presets.erase(it);
        emit presetsChanged();
    }
}

QStringList FilterState::presetNames() const
{
    QStringList names;
    names.reserve(static_cast<int>(m_presets.size()));
    for (const auto& p : m_presets) {
        names.append(p.name);
    }
    return names;
}

const std::vector<FilterPreset>& FilterState::presets() const
{
    return m_presets;
}

void FilterState::applyQuickFilter(const QString& columnName, FilterOperator op,
                                   const QVariant& value)
{
    FilterRule rule;
    rule.columnName = columnName;
    rule.op = op;
    rule.value = value;

    // Quick filter replaces the current filter with a single rule
    m_currentFilter = FilterGroup{};
    m_currentFilter.combination = FilterCombination::And;
    m_currentFilter.rules.push_back(std::move(rule));
    emit filterChanged();
}

QString FilterState::toWhereClause() const
{
    if (m_currentFilter.isEmpty()) {
        return {};
    }
    return QueryBuilder::filterGroupToSQL(m_currentFilter);
}

void FilterState::addToRecent(const FilterGroup& group)
{
    if (group.isEmpty()) {
        return;
    }

    // Avoid duplicates: remove any existing identical entry
    // (Simple heuristic: compare rule count and first rule column)
    m_recentFilters.insert(m_recentFilters.begin(), group);

    if (static_cast<int>(m_recentFilters.size()) > kMaxRecentFilters) {
        m_recentFilters.resize(static_cast<size_t>(kMaxRecentFilters));
    }
}

const std::vector<FilterGroup>& FilterState::recentFilters() const
{
    return m_recentFilters;
}

} // namespace csvforge
