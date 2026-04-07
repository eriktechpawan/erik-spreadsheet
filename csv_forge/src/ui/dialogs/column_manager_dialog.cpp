#include "ui/dialogs/column_manager_dialog.h"

#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

namespace csvforge {

namespace {
constexpr int ColVisible = 0;
constexpr int ColName    = 1;
constexpr int ColType    = 2;
constexpr int ColWidth   = 3;
constexpr int ColFrozen  = 4;
} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ColumnManagerDialog::ColumnManagerDialog(const ColumnSchemaList& schema,
                                         QWidget* parent)
    : QDialog(parent)
    , m_schema(schema)
    , m_originalSchema(schema)
{
    setWindowTitle(QStringLiteral("Column Manager"));
    resize(600, 500);
    buildUI();
    populateTable();
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

ColumnSchemaList ColumnManagerDialog::getSchema() const
{
    ColumnSchemaList result;
    result.reserve(m_table->rowCount());

    for (int r = 0; r < m_table->rowCount(); ++r) {
        ColumnSchema col;
        col.displayOrder = r;

        auto* visItem = m_table->item(r, ColVisible);
        col.visible = (visItem->checkState() == Qt::Checked);

        col.name = m_table->item(r, ColName)->text();
        col.type = ColumnSchema::typeFromString(m_table->item(r, ColType)->text());

        auto* widthSpin = qobject_cast<QSpinBox*>(
            m_table->cellWidget(r, ColWidth));
        col.width = widthSpin ? widthSpin->value() : 120;

        auto* frozenItem = m_table->item(r, ColFrozen);
        col.frozen = (frozenItem->checkState() == Qt::Checked);

        // Preserve original index
        col.index = m_table->item(r, ColName)->data(Qt::UserRole).toInt();

        result.push_back(col);
    }
    return result;
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void ColumnManagerDialog::onMoveUp()
{
    const int row = m_table->currentRow();
    if (row > 0)
        swapRows(row, row - 1);
}

void ColumnManagerDialog::onMoveDown()
{
    const int row = m_table->currentRow();
    if (row >= 0 && row < m_table->rowCount() - 1)
        swapRows(row, row + 1);
}

void ColumnManagerDialog::onShowAll()
{
    for (int r = 0; r < m_table->rowCount(); ++r)
        m_table->item(r, ColVisible)->setCheckState(Qt::Checked);
}

void ColumnManagerDialog::onHideAll()
{
    for (int r = 0; r < m_table->rowCount(); ++r)
        m_table->item(r, ColVisible)->setCheckState(Qt::Unchecked);
}

void ColumnManagerDialog::onResetDefaults()
{
    m_schema = m_originalSchema;
    populateTable();
}

void ColumnManagerDialog::onApply()
{
    emit applied(getSchema());
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

void ColumnManagerDialog::buildUI()
{
    auto* mainLayout = new QHBoxLayout(this);

    // Table
    m_table = new QTableWidget(this);
    m_table->setColumnCount(5);
    m_table->setHorizontalHeaderLabels(
        {QStringLiteral("Visible"), QStringLiteral("Name"),
         QStringLiteral("Type"), QStringLiteral("Width"),
         QStringLiteral("Frozen")});
    m_table->horizontalHeader()->setStretchLastSection(true);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setSelectionMode(QAbstractItemView::SingleSelection);
    m_table->verticalHeader()->setVisible(false);
    mainLayout->addWidget(m_table, /*stretch=*/1);

    // Right side buttons
    auto* btnLayout = new QVBoxLayout;

    m_moveUpBtn = new QPushButton(QStringLiteral("Move Up"), this);
    m_moveDownBtn = new QPushButton(QStringLiteral("Move Down"), this);
    m_showAllBtn = new QPushButton(QStringLiteral("Show All"), this);
    m_hideAllBtn = new QPushButton(QStringLiteral("Hide All"), this);
    m_resetDefaultBtn = new QPushButton(QStringLiteral("Reset Defaults"), this);

    connect(m_moveUpBtn,       &QPushButton::clicked, this, &ColumnManagerDialog::onMoveUp);
    connect(m_moveDownBtn,     &QPushButton::clicked, this, &ColumnManagerDialog::onMoveDown);
    connect(m_showAllBtn,      &QPushButton::clicked, this, &ColumnManagerDialog::onShowAll);
    connect(m_hideAllBtn,      &QPushButton::clicked, this, &ColumnManagerDialog::onHideAll);
    connect(m_resetDefaultBtn, &QPushButton::clicked, this, &ColumnManagerDialog::onResetDefaults);

    btnLayout->addWidget(m_moveUpBtn);
    btnLayout->addWidget(m_moveDownBtn);
    btnLayout->addSpacing(16);
    btnLayout->addWidget(m_showAllBtn);
    btnLayout->addWidget(m_hideAllBtn);
    btnLayout->addSpacing(16);
    btnLayout->addWidget(m_resetDefaultBtn);
    btnLayout->addStretch();

    m_buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply,
        this);
    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_buttonBox->button(QDialogButtonBox::Apply), &QPushButton::clicked,
            this, &ColumnManagerDialog::onApply);

    btnLayout->addWidget(m_buttonBox);
    mainLayout->addLayout(btnLayout);
}

void ColumnManagerDialog::populateTable()
{
    m_table->setRowCount(0);
    m_table->setRowCount(static_cast<int>(m_schema.size()));

    for (int r = 0; r < static_cast<int>(m_schema.size()); ++r) {
        const auto& col = m_schema[static_cast<size_t>(r)];

        // Visible checkbox
        auto* visItem = new QTableWidgetItem();
        visItem->setFlags(visItem->flags() | Qt::ItemIsUserCheckable);
        visItem->setCheckState(col.visible ? Qt::Checked : Qt::Unchecked);
        m_table->setItem(r, ColVisible, visItem);

        // Name (read-only, stores original index)
        auto* nameItem = new QTableWidgetItem(col.name);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable);
        nameItem->setData(Qt::UserRole, col.index);
        m_table->setItem(r, ColName, nameItem);

        // Type (read-only)
        auto* typeItem = new QTableWidgetItem(ColumnSchema::typeToString(col.type));
        typeItem->setFlags(typeItem->flags() & ~Qt::ItemIsEditable);
        m_table->setItem(r, ColType, typeItem);

        // Width spin box
        auto* widthSpin = new QSpinBox(m_table);
        widthSpin->setRange(50, 500);
        widthSpin->setValue(col.width);
        m_table->setCellWidget(r, ColWidth, widthSpin);

        // Frozen checkbox
        auto* frozenItem = new QTableWidgetItem();
        frozenItem->setFlags(frozenItem->flags() | Qt::ItemIsUserCheckable);
        frozenItem->setCheckState(col.frozen ? Qt::Checked : Qt::Unchecked);
        m_table->setItem(r, ColFrozen, frozenItem);
    }

    m_table->resizeColumnsToContents();
}

void ColumnManagerDialog::swapRows(int a, int b)
{
    // Swap schema entries
    std::swap(m_schema[static_cast<size_t>(a)],
              m_schema[static_cast<size_t>(b)]);
    populateTable();
    m_table->selectRow(b);
}

} // namespace csvforge
