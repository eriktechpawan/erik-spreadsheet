#pragma once

#include "data/types/column_schema.h"
#include "data/types/filter_rule.h"

#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QPushButton>
#include <QTreeWidget>

namespace csvforge {

// ---------------------------------------------------------------------------
// FilterBuilderDialog – advanced filter rule/group builder
// ---------------------------------------------------------------------------

class FilterBuilderDialog : public QDialog {
    Q_OBJECT

public:
    explicit FilterBuilderDialog(const ColumnSchemaList& schema,
                                 const FilterGroup& currentFilter = {},
                                 QWidget* parent = nullptr);
    ~FilterBuilderDialog() override = default;

    FilterGroup filterGroup() const;

private slots:
    void onAddRule();
    void onAddGroup();
    void onRemoveSelected();
    void onClear();

private:
    void buildUI();
    void populateTree(QTreeWidgetItem* parent, const FilterGroup& group);
    void loadFilterGroup(const FilterGroup& group);
    FilterGroup buildGroupFromItem(QTreeWidgetItem* item) const;
    FilterRule  buildRuleFromItem(QTreeWidgetItem* item) const;
    QStringList operatorNames() const;
    FilterOperator operatorFromString(const QString& name) const;
    QString operatorToString(FilterOperator op) const;

    QTreeWidgetItem* createGroupItem(FilterCombination combo,
                                     QTreeWidgetItem* parent = nullptr);
    QTreeWidgetItem* createRuleItem(const FilterRule& rule,
                                    QTreeWidgetItem* parent = nullptr);

    // ---- Widgets ----------------------------------------------------------
    QTreeWidget*      m_tree      = nullptr;
    QPushButton*      m_addRuleBtn   = nullptr;
    QPushButton*      m_addGroupBtn  = nullptr;
    QPushButton*      m_removeBtn    = nullptr;
    QPushButton*      m_clearBtn     = nullptr;
    QDialogButtonBox* m_buttonBox    = nullptr;

    // ---- Data -------------------------------------------------------------
    ColumnSchemaList m_schema;

    static constexpr int RoleType  = Qt::UserRole;     // "group" or "rule"
    static constexpr int RoleCombo = Qt::UserRole + 1;  // FilterCombination
    static constexpr int RoleCol   = Qt::UserRole + 2;
    static constexpr int RoleOp    = Qt::UserRole + 3;
    static constexpr int RoleVal   = Qt::UserRole + 4;
};

} // namespace csvforge
