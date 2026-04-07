#include "ui/dialogs/filter_builder_dialog.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QMessageBox>
#include <QVBoxLayout>

namespace csvforge {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

FilterBuilderDialog::FilterBuilderDialog(const ColumnSchemaList& schema,
                                         const FilterGroup& currentFilter,
                                         QWidget* parent)
    : QDialog(parent)
    , m_schema(schema)
{
    setWindowTitle(QStringLiteral("Filter Builder"));
    resize(700, 480);
    buildUI();

    if (!currentFilter.isEmpty()) {
        loadFilterGroup(currentFilter);
    } else {
        // Start with a default root AND-group
        createGroupItem(FilterCombination::And);
    }
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

FilterGroup FilterBuilderDialog::filterGroup() const
{
    if (m_tree->topLevelItemCount() == 0) {
        return {};
    }
    return buildGroupFromItem(m_tree->topLevelItem(0));
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void FilterBuilderDialog::onAddRule()
{
    QTreeWidgetItem* parent = m_tree->currentItem();
    if (!parent) {
        if (m_tree->topLevelItemCount() > 0) {
            parent = m_tree->topLevelItem(0);
        } else {
            parent = createGroupItem(FilterCombination::And);
        }
    }
    // Walk up to nearest group item
    while (parent && parent->data(0, RoleType).toString() != QStringLiteral("group")) {
        parent = parent->parent();
    }
    if (!parent) {
        parent = m_tree->topLevelItem(0);
    }

    FilterRule rule;
    if (!m_schema.empty()) {
        rule.columnName = m_schema.front().name;
    }
    createRuleItem(rule, parent);
    parent->setExpanded(true);
}

void FilterBuilderDialog::onAddGroup()
{
    QTreeWidgetItem* parent = m_tree->currentItem();
    if (!parent) {
        if (m_tree->topLevelItemCount() > 0) {
            parent = m_tree->topLevelItem(0);
        } else {
            createGroupItem(FilterCombination::And);
            return;
        }
    }
    while (parent && parent->data(0, RoleType).toString() != QStringLiteral("group")) {
        parent = parent->parent();
    }
    if (!parent) {
        parent = m_tree->topLevelItem(0);
    }

    auto* child = createGroupItem(FilterCombination::And, parent);
    parent->setExpanded(true);
    Q_UNUSED(child)
}

void FilterBuilderDialog::onRemoveSelected()
{
    auto* item = m_tree->currentItem();
    if (!item) {
        return;
    }
    // Don't remove root
    if (!item->parent() && m_tree->indexOfTopLevelItem(item) >= 0) {
        QMessageBox::information(this, QStringLiteral("Filter Builder"),
            QStringLiteral("Cannot remove the root group. Use Clear instead."));
        return;
    }
    delete item;
}

void FilterBuilderDialog::onClear()
{
    m_tree->clear();
    createGroupItem(FilterCombination::And);
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

void FilterBuilderDialog::buildUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // ---- Tree widget ------------------------------------------------------
    m_tree = new QTreeWidget(this);
    m_tree->setHeaderLabels({QStringLiteral("Filter Logic")});
    m_tree->header()->setStretchLastSection(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    m_tree->setAlternatingRowColors(true);
    mainLayout->addWidget(m_tree, /*stretch=*/1);

    // ---- Action buttons ---------------------------------------------------
    auto* btnRow = new QHBoxLayout;

    m_addRuleBtn  = new QPushButton(QStringLiteral("Add Rule"), this);
    m_addGroupBtn = new QPushButton(QStringLiteral("Add Group"), this);
    m_removeBtn   = new QPushButton(QStringLiteral("Remove"), this);
    m_clearBtn    = new QPushButton(QStringLiteral("Clear"), this);

    btnRow->addWidget(m_addRuleBtn);
    btnRow->addWidget(m_addGroupBtn);
    btnRow->addWidget(m_removeBtn);
    btnRow->addStretch();
    btnRow->addWidget(m_clearBtn);
    mainLayout->addLayout(btnRow);

    connect(m_addRuleBtn,  &QPushButton::clicked, this, &FilterBuilderDialog::onAddRule);
    connect(m_addGroupBtn, &QPushButton::clicked, this, &FilterBuilderDialog::onAddGroup);
    connect(m_removeBtn,   &QPushButton::clicked, this, &FilterBuilderDialog::onRemoveSelected);
    connect(m_clearBtn,    &QPushButton::clicked, this, &FilterBuilderDialog::onClear);

    // ---- Dialog buttons ---------------------------------------------------
    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    mainLayout->addWidget(m_buttonBox);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

// ---------------------------------------------------------------------------
// Tree item helpers
// ---------------------------------------------------------------------------

QTreeWidgetItem* FilterBuilderDialog::createGroupItem(FilterCombination combo,
                                                       QTreeWidgetItem* parent)
{
    const QString label = (combo == FilterCombination::And)
                              ? QStringLiteral("AND Group")
                              : QStringLiteral("OR Group");

    QTreeWidgetItem* item = parent
        ? new QTreeWidgetItem(parent, {label})
        : new QTreeWidgetItem(m_tree, {label});

    item->setData(0, RoleType, QStringLiteral("group"));
    item->setData(0, RoleCombo, static_cast<int>(combo));
    item->setFlags(item->flags() | Qt::ItemIsEditable);
    item->setExpanded(true);

    // Embed a combo for AND/OR switching
    auto* comboWidget = new QComboBox(m_tree);
    comboWidget->addItem(QStringLiteral("AND"));
    comboWidget->addItem(QStringLiteral("OR"));
    comboWidget->setCurrentIndex(combo == FilterCombination::And ? 0 : 1);
    m_tree->setItemWidget(item, 0, comboWidget);

    connect(comboWidget, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [item](int idx) {
        const auto c = (idx == 0) ? FilterCombination::And : FilterCombination::Or;
        item->setData(0, Qt::UserRole + 1, static_cast<int>(c));
    });

    return item;
}

QTreeWidgetItem* FilterBuilderDialog::createRuleItem(const FilterRule& rule,
                                                      QTreeWidgetItem* parent)
{
    const QString display = QStringLiteral("%1  %2  %3")
                                .arg(rule.columnName,
                                     operatorToString(rule.op),
                                     rule.value.toString());

    QTreeWidgetItem* item = parent
        ? new QTreeWidgetItem(parent, {display})
        : new QTreeWidgetItem(m_tree, {display});

    item->setData(0, RoleType, QStringLiteral("rule"));
    item->setData(0, RoleCol, rule.columnName);
    item->setData(0, RoleOp, static_cast<int>(rule.op));
    item->setData(0, RoleVal, rule.value);

    // Build an inline widget row: [column combo] [operator combo] [value edit]
    auto* rowWidget = new QWidget(m_tree);
    auto* rowLayout = new QHBoxLayout(rowWidget);
    rowLayout->setContentsMargins(2, 1, 2, 1);

    auto* colCombo = new QComboBox(rowWidget);
    for (const auto& col : m_schema) {
        colCombo->addItem(col.name);
    }
    if (!rule.columnName.isEmpty()) {
        colCombo->setCurrentText(rule.columnName);
    }

    auto* opCombo = new QComboBox(rowWidget);
    for (const QString& name : operatorNames()) {
        opCombo->addItem(name);
    }
    opCombo->setCurrentText(operatorToString(rule.op));

    auto* valEdit = new QLineEdit(rule.value.toString(), rowWidget);
    valEdit->setPlaceholderText(QStringLiteral("value"));

    rowLayout->addWidget(colCombo, /*stretch=*/2);
    rowLayout->addWidget(opCombo,  /*stretch=*/2);
    rowLayout->addWidget(valEdit,  /*stretch=*/3);

    m_tree->setItemWidget(item, 0, rowWidget);

    // Keep tree item data in sync with widgets
    auto syncData = [item, colCombo, opCombo, valEdit, this]() {
        item->setData(0, RoleCol, colCombo->currentText());
        item->setData(0, RoleOp, static_cast<int>(operatorFromString(opCombo->currentText())));
        item->setData(0, RoleVal, valEdit->text());
    };
    connect(colCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, syncData);
    connect(opCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, syncData);
    connect(valEdit, &QLineEdit::textChanged, this, syncData);

    return item;
}

// ---------------------------------------------------------------------------
// Load / build filter groups
// ---------------------------------------------------------------------------

void FilterBuilderDialog::loadFilterGroup(const FilterGroup& group)
{
    m_tree->clear();
    auto* root = createGroupItem(group.combination);
    for (const auto& rule : group.rules) {
        createRuleItem(rule, root);
    }
    for (const auto& sub : group.subGroups) {
        populateTree(root, sub);
    }
    root->setExpanded(true);
}

void FilterBuilderDialog::populateTree(QTreeWidgetItem* parent,
                                        const FilterGroup& group)
{
    auto* gItem = createGroupItem(group.combination, parent);
    for (const auto& rule : group.rules) {
        createRuleItem(rule, gItem);
    }
    for (const auto& sub : group.subGroups) {
        populateTree(gItem, sub);
    }
}

FilterGroup FilterBuilderDialog::buildGroupFromItem(QTreeWidgetItem* item) const
{
    FilterGroup group;
    group.combination = static_cast<FilterCombination>(
        item->data(0, RoleCombo).toInt());

    for (int i = 0; i < item->childCount(); ++i) {
        auto* child = item->child(i);
        if (child->data(0, RoleType).toString() == QStringLiteral("group")) {
            group.subGroups.push_back(buildGroupFromItem(child));
        } else {
            group.rules.push_back(buildRuleFromItem(child));
        }
    }
    return group;
}

FilterRule FilterBuilderDialog::buildRuleFromItem(QTreeWidgetItem* item) const
{
    FilterRule rule;
    rule.columnName = item->data(0, RoleCol).toString();
    rule.op         = static_cast<FilterOperator>(item->data(0, RoleOp).toInt());
    rule.value      = item->data(0, RoleVal);
    return rule;
}

// ---------------------------------------------------------------------------
// Operator name mapping
// ---------------------------------------------------------------------------

QStringList FilterBuilderDialog::operatorNames() const
{
    return {
        QStringLiteral("Equals"),
        QStringLiteral("Not Equals"),
        QStringLiteral("Contains"),
        QStringLiteral("Not Contains"),
        QStringLiteral("Starts With"),
        QStringLiteral("Ends With"),
        QStringLiteral("Greater Than"),
        QStringLiteral("Greater or Equal"),
        QStringLiteral("Less Than"),
        QStringLiteral("Less or Equal"),
        QStringLiteral("Between"),
        QStringLiteral("Is Empty"),
        QStringLiteral("Is Not Empty"),
        QStringLiteral("Is Null"),
        QStringLiteral("Is Not Null"),
        QStringLiteral("In List"),
        QStringLiteral("Regex"),
    };
}

FilterOperator FilterBuilderDialog::operatorFromString(const QString& name) const
{
    static const QHash<QString, FilterOperator> map = {
        {QStringLiteral("Equals"),           FilterOperator::Equals},
        {QStringLiteral("Not Equals"),       FilterOperator::NotEquals},
        {QStringLiteral("Contains"),         FilterOperator::Contains},
        {QStringLiteral("Not Contains"),     FilterOperator::NotContains},
        {QStringLiteral("Starts With"),      FilterOperator::StartsWith},
        {QStringLiteral("Ends With"),        FilterOperator::EndsWith},
        {QStringLiteral("Greater Than"),     FilterOperator::GreaterThan},
        {QStringLiteral("Greater or Equal"), FilterOperator::GreaterEqual},
        {QStringLiteral("Less Than"),        FilterOperator::LessThan},
        {QStringLiteral("Less or Equal"),    FilterOperator::LessEqual},
        {QStringLiteral("Between"),          FilterOperator::Between},
        {QStringLiteral("Is Empty"),         FilterOperator::IsEmpty},
        {QStringLiteral("Is Not Empty"),     FilterOperator::IsNotEmpty},
        {QStringLiteral("Is Null"),          FilterOperator::IsNull},
        {QStringLiteral("Is Not Null"),      FilterOperator::IsNotNull},
        {QStringLiteral("In List"),          FilterOperator::InList},
        {QStringLiteral("Regex"),            FilterOperator::Regex},
    };
    return map.value(name, FilterOperator::Equals);
}

QString FilterBuilderDialog::operatorToString(FilterOperator op) const
{
    switch (op) {
    case FilterOperator::Equals:        return QStringLiteral("Equals");
    case FilterOperator::NotEquals:     return QStringLiteral("Not Equals");
    case FilterOperator::Contains:      return QStringLiteral("Contains");
    case FilterOperator::NotContains:   return QStringLiteral("Not Contains");
    case FilterOperator::StartsWith:    return QStringLiteral("Starts With");
    case FilterOperator::EndsWith:      return QStringLiteral("Ends With");
    case FilterOperator::GreaterThan:   return QStringLiteral("Greater Than");
    case FilterOperator::GreaterEqual:  return QStringLiteral("Greater or Equal");
    case FilterOperator::LessThan:      return QStringLiteral("Less Than");
    case FilterOperator::LessEqual:     return QStringLiteral("Less or Equal");
    case FilterOperator::Between:       return QStringLiteral("Between");
    case FilterOperator::IsEmpty:       return QStringLiteral("Is Empty");
    case FilterOperator::IsNotEmpty:    return QStringLiteral("Is Not Empty");
    case FilterOperator::IsNull:        return QStringLiteral("Is Null");
    case FilterOperator::IsNotNull:     return QStringLiteral("Is Not Null");
    case FilterOperator::InList:        return QStringLiteral("In List");
    case FilterOperator::Regex:         return QStringLiteral("Regex");
    default:                            return QStringLiteral("Equals");
    }
}

} // namespace csvforge
