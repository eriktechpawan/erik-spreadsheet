#pragma once
#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QGroupBox>
#include "data/types/column_schema.h"
#include "data/types/filter_rule.h"

namespace csvforge {

class FilterState;
class DuckDBEngine;

class FilterPanel : public QWidget {
    Q_OBJECT
public:
    explicit FilterPanel(QWidget* parent = nullptr);

    void setFilterState(FilterState* state);
    void setEngine(DuckDBEngine* engine);
    void setTableName(const QString& tableName);
    void setSchema(const ColumnSchemaList& schema);
    void showQuickFilterForColumn(const QString& columnName, const QVariant& value = {});

signals:
    void filterChanged(const FilterGroup& group);
    void filterCleared();

private:
    FilterState* m_filterState = nullptr;
    DuckDBEngine* m_engine = nullptr;
    QString m_tableName;
    ColumnSchemaList m_schema;

    QComboBox* m_columnSelector = nullptr;
    QComboBox* m_operatorSelector = nullptr;
    QLineEdit* m_valueEdit = nullptr;
    QLineEdit* m_secondValueEdit = nullptr;
    QPushButton* m_addFilterBtn = nullptr;
    QPushButton* m_clearBtn = nullptr;
    QComboBox* m_combinationSelector = nullptr;
    QListWidget* m_activeFiltersList = nullptr;
    QComboBox* m_presetSelector = nullptr;
    QPushButton* m_savePresetBtn = nullptr;
    QGroupBox* m_quickFilterGroup = nullptr;

    void setupUI();
    void populateOperators(ColumnType type);
    void onAddFilter();
    void onRemoveFilter();
    void onClearFilters();
    void onSavePreset();
    void onLoadPreset(const QString& name);
    void updateActiveFiltersList();
    FilterOperator selectedOperator() const;
};

} // namespace csvforge
