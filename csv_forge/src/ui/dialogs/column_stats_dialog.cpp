#include "ui/dialogs/column_stats_dialog.h"

#include "utils/string_utils.h"

#include <QApplication>
#include <QClipboard>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

namespace csvforge {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ColumnStatsDialog::ColumnStatsDialog(const ColumnStats& stats, QWidget* parent)
    : QDialog(parent)
    , m_stats(stats)
{
    setWindowTitle(QStringLiteral("Column Statistics"));
    resize(500, 600);
    buildUI();
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------

void ColumnStatsDialog::onCopyStats()
{
    QApplication::clipboard()->setText(statsAsText());
}

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

void ColumnStatsDialog::buildUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Column name (bold) + type
    m_columnNameLabel = new QLabel(this);
    QFont boldFont = m_columnNameLabel->font();
    boldFont.setBold(true);
    boldFont.setPointSize(boldFont.pointSize() + 2);
    m_columnNameLabel->setFont(boldFont);
    m_columnNameLabel->setText(m_stats.columnName);

    m_typeLabel = new QLabel(this);
    m_typeLabel->setText(
        QStringLiteral("Type: %1").arg(ColumnSchema::typeToString(m_stats.type)));

    mainLayout->addWidget(m_columnNameLabel);
    mainLayout->addWidget(m_typeLabel);

    mainLayout->addWidget(buildSummarySection());
    mainLayout->addWidget(buildTopValuesSection(), /*stretch=*/1);

    m_selectionGroup = buildSelectionSection();
    m_selectionGroup->setVisible(m_stats.selectionCount > 0);
    mainLayout->addWidget(m_selectionGroup);

    // Bottom buttons
    auto* bottomLayout = new QHBoxLayout;
    m_copyBtn = new QPushButton(QStringLiteral("Copy Stats"), this);
    connect(m_copyBtn, &QPushButton::clicked,
            this, &ColumnStatsDialog::onCopyStats);
    bottomLayout->addWidget(m_copyBtn);
    bottomLayout->addStretch();

    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    bottomLayout->addWidget(m_buttonBox);
    mainLayout->addLayout(bottomLayout);

    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QGroupBox* ColumnStatsDialog::buildSummarySection()
{
    auto* group  = new QGroupBox(QStringLiteral("Summary Statistics"), this);
    auto* layout = new QFormLayout(group);

    m_totalRowsLabel    = new QLabel(formatNumber(m_stats.totalRows), group);
    m_filteredRowsLabel = new QLabel(formatNumber(m_stats.filteredRows), group);
    m_nullCountLabel    = new QLabel(formatNumber(m_stats.nullCount), group);
    m_nonNullCountLabel = new QLabel(formatNumber(m_stats.nonNullCount), group);
    m_uniqueCountLabel  = new QLabel(formatNumber(m_stats.uniqueCount), group);
    m_minLabel          = new QLabel(m_stats.minValue.toString(), group);
    m_maxLabel          = new QLabel(m_stats.maxValue.toString(), group);
    m_sumLabel          = new QLabel(formatDouble(m_stats.sum), group);
    m_averageLabel      = new QLabel(formatDouble(m_stats.average), group);
    m_medianLabel       = new QLabel(formatDouble(m_stats.median), group);

    layout->addRow(QStringLiteral("Total Rows:"),    m_totalRowsLabel);
    layout->addRow(QStringLiteral("Filtered Rows:"), m_filteredRowsLabel);
    layout->addRow(QStringLiteral("Null Count:"),    m_nullCountLabel);
    layout->addRow(QStringLiteral("Non-Null Count:"), m_nonNullCountLabel);
    layout->addRow(QStringLiteral("Unique Count:"),  m_uniqueCountLabel);
    layout->addRow(QStringLiteral("Min:"),           m_minLabel);
    layout->addRow(QStringLiteral("Max:"),           m_maxLabel);
    layout->addRow(QStringLiteral("Sum:"),           m_sumLabel);
    layout->addRow(QStringLiteral("Average:"),       m_averageLabel);
    layout->addRow(QStringLiteral("Median:"),        m_medianLabel);

    return group;
}

QGroupBox* ColumnStatsDialog::buildTopValuesSection()
{
    auto* group  = new QGroupBox(QStringLiteral("Top Values"), this);
    auto* layout = new QVBoxLayout(group);

    m_topValuesTable = new QTableWidget(group);
    m_topValuesTable->setColumnCount(3);
    m_topValuesTable->setHorizontalHeaderLabels(
        {QStringLiteral("Value"), QStringLiteral("Count"),
         QStringLiteral("Percentage")});
    m_topValuesTable->horizontalHeader()->setStretchLastSection(true);
    m_topValuesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_topValuesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_topValuesTable->verticalHeader()->setVisible(false);

    const int rowCount = qMin(m_stats.topValues.size(), qsizetype(20));
    m_topValuesTable->setRowCount(rowCount);

    for (int i = 0; i < rowCount; ++i) {
        const auto& [value, count] = m_stats.topValues[i];
        double pct = (m_stats.nonNullCount > 0)
                         ? (static_cast<double>(count) / m_stats.nonNullCount * 100.0)
                         : 0.0;

        m_topValuesTable->setItem(
            i, 0, new QTableWidgetItem(value));
        m_topValuesTable->setItem(
            i, 1, new QTableWidgetItem(formatNumber(count)));
        m_topValuesTable->setItem(
            i, 2, new QTableWidgetItem(formatDouble(pct) + QStringLiteral("%")));
    }

    m_topValuesTable->resizeColumnsToContents();
    layout->addWidget(m_topValuesTable);
    return group;
}

QGroupBox* ColumnStatsDialog::buildSelectionSection()
{
    auto* group  = new QGroupBox(QStringLiteral("Selection Stats"), this);
    auto* layout = new QFormLayout(group);

    m_selCountLabel = new QLabel(formatNumber(m_stats.selectionCount), group);
    m_selSumLabel   = new QLabel(formatDouble(m_stats.selectionSum), group);
    m_selAvgLabel   = new QLabel(formatDouble(m_stats.selectionAvg), group);

    layout->addRow(QStringLiteral("Selection Count:"),   m_selCountLabel);
    layout->addRow(QStringLiteral("Selection Sum:"),     m_selSumLabel);
    layout->addRow(QStringLiteral("Selection Average:"), m_selAvgLabel);

    return group;
}

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

QString ColumnStatsDialog::statsAsText() const
{
    QStringList lines;
    lines << QStringLiteral("Column: %1").arg(m_stats.columnName);
    lines << QStringLiteral("Type: %1").arg(ColumnSchema::typeToString(m_stats.type));
    lines << QString();
    lines << QStringLiteral("Total Rows: %1").arg(formatNumber(m_stats.totalRows));
    lines << QStringLiteral("Filtered Rows: %1").arg(formatNumber(m_stats.filteredRows));
    lines << QStringLiteral("Null Count: %1").arg(formatNumber(m_stats.nullCount));
    lines << QStringLiteral("Non-Null Count: %1").arg(formatNumber(m_stats.nonNullCount));
    lines << QStringLiteral("Unique Count: %1").arg(formatNumber(m_stats.uniqueCount));
    lines << QStringLiteral("Min: %1").arg(m_stats.minValue.toString());
    lines << QStringLiteral("Max: %1").arg(m_stats.maxValue.toString());
    lines << QStringLiteral("Sum: %1").arg(formatDouble(m_stats.sum));
    lines << QStringLiteral("Average: %1").arg(formatDouble(m_stats.average));
    lines << QStringLiteral("Median: %1").arg(formatDouble(m_stats.median));

    if (m_stats.selectionCount > 0) {
        lines << QString();
        lines << QStringLiteral("Selection Count: %1").arg(formatNumber(m_stats.selectionCount));
        lines << QStringLiteral("Selection Sum: %1").arg(formatDouble(m_stats.selectionSum));
        lines << QStringLiteral("Selection Average: %1").arg(formatDouble(m_stats.selectionAvg));
    }

    if (!m_stats.topValues.isEmpty()) {
        lines << QString();
        lines << QStringLiteral("Top Values:");
        const int count = qMin(m_stats.topValues.size(), qsizetype(20));
        for (int i = 0; i < count; ++i) {
            const auto& [value, cnt] = m_stats.topValues[i];
            double pct = (m_stats.nonNullCount > 0)
                             ? (static_cast<double>(cnt) / m_stats.nonNullCount * 100.0)
                             : 0.0;
            lines << QStringLiteral("  %1: %2 (%3%)")
                         .arg(value)
                         .arg(formatNumber(cnt))
                         .arg(formatDouble(pct));
        }
    }

    return lines.join(QLatin1Char('\n'));
}

} // namespace csvforge
