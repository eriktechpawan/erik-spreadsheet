#pragma once

#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include <QMap>
#include <QString>
#include <QStringList>
#include <optional>
#include <vector>

#include "data/model/sort_state.h"
#include "data/types/filter_rule.h"
#include "data/types/formula_config.h"
#include "data/types/lookup_config.h"
#include "data/types/pivot_config.h"

namespace csvforge {

struct SessionState {
    // File info
    QString sourceFilePath;
    QString sessionFilePath;
    bool editableMode = false;

    // View state
    FilterGroup filters;
    std::vector<SortColumn> sorts;
    QStringList hiddenColumns;
    QStringList columnOrder;
    QMap<QString, int> columnWidths;

    // Recent
    QStringList recentFiles;

    // Filter presets
    std::vector<FilterPreset> filterPresets;

    // Pivot configs
    std::vector<PivotConfig> pivotConfigs;

    // Lookup configs
    std::vector<LookupConfig> lookupConfigs;

    // Formula / calculated column configs
    std::vector<FormulaConfig> formulaConfigs;

    // Tabs (serialized tab state array)
    QJsonArray tabStates;
    int activeTabIndex = 0;

    // Window geometry
    QByteArray windowGeometry;
    QByteArray windowState;

    // Serialize
    QJsonObject toJson() const;
    static SessionState fromJson(const QJsonObject& obj);

    // Save/load file
    bool saveToFile(const QString& path) const;
    static std::optional<SessionState> loadFromFile(const QString& path);
};

} // namespace csvforge
