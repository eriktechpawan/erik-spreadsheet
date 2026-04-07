#include "column_profile_panel.h"

#include <QFormLayout>
#include <QHeaderView>
#include <QVBoxLayout>

namespace csvforge {

ColumnProfilePanel::ColumnProfilePanel(QWidget* parent)
    : QWidget(parent)
{
    setupUI();
}

void ColumnProfilePanel::setupUI()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(8, 8, 8, 8);
    mainLayout->setSpacing(6);

    // Loading indicator
    m_loadingBar = new QProgressBar(this);
    m_loadingBar->setRange(0, 0); // indeterminate
    m_loadingBar->setMaximumHeight(4);
    m_loadingBar->setTextVisible(false);
    m_loadingBar->setVisible(false);
    mainLayout->addWidget(m_loadingBar);

    // Column name
    m_columnNameLabel = new QLabel(QStringLiteral("-"), this);
    QFont nameFont = m_columnNameLabel->font();
    nameFont.setBold(true);
    nameFont.setPointSize(nameFont.pointSize() + 2);
    m_columnNameLabel->setFont(nameFont);
    mainLayout->addWidget(m_columnNameLabel);

    // Type label
    m_typeLabel = new QLabel(QStringLiteral("-"), this);
    m_typeLabel->setStyleSheet(QStringLiteral("color: #666;"));
    mainLayout->addWidget(m_typeLabel);

    // Statistics form
    auto* statsForm = new QFormLayout();
    statsForm->setSpacing(4);

    m_totalRowsLabel = new QLabel(QStringLiteral("-"), this);
    statsForm->addRow(tr("Total Rows:"), m_totalRowsLabel);

    m_filteredRowsLabel = new QLabel(QStringLiteral("-"), this);
    statsForm->addRow(tr("Filtered:"), m_filteredRowsLabel);

    m_nullCountLabel = new QLabel(QStringLiteral("-"), this);
    statsForm->addRow(tr("Null:"), m_nullCountLabel);

    m_uniqueCountLabel = new QLabel(QStringLiteral("-"), this);
    statsForm->addRow(tr("Unique:"), m_uniqueCountLabel);

    m_minLabel = new QLabel(QStringLiteral("-"), this);
    statsForm->addRow(tr("Min:"), m_minLabel);

    m_maxLabel = new QLabel(QStringLiteral("-"), this);
    statsForm->addRow(tr("Max:"), m_maxLabel);

    m_sumLabel = new QLabel(QStringLiteral("-"), this);
    statsForm->addRow(tr("Sum:"), m_sumLabel);

    m_avgLabel = new QLabel(QStringLiteral("-"), this);
    statsForm->addRow(tr("Avg:"), m_avgLabel);

    m_medianLabel = new QLabel(QStringLiteral("-"), this);
    statsForm->addRow(tr("Median:"), m_medianLabel);

    mainLayout->addLayout(statsForm);

    // Top values table
    auto* topLabel = new QLabel(tr("Top Values"), this);
    QFont topFont = topLabel->font();
    topFont.setBold(true);
    topLabel->setFont(topFont);
    mainLayout->addWidget(topLabel);

    m_topValuesTable = new QTableWidget(0, 2, this);
    m_topValuesTable->setHorizontalHeaderLabels({tr("Value"), tr("Count")});
    m_topValuesTable->horizontalHeader()->setStretchLastSection(true);
    m_topValuesTable->verticalHeader()->setVisible(false);
    m_topValuesTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_topValuesTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_topValuesTable->setMaximumHeight(200);
    mainLayout->addWidget(m_topValuesTable);

    // Selection stats
    auto* selLabel = new QLabel(tr("Selection"), this);
    QFont selFont = selLabel->font();
    selFont.setBold(true);
    selLabel->setFont(selFont);
    mainLayout->addWidget(selLabel);

    auto* selForm = new QFormLayout();
    m_selectionSumLabel = new QLabel(QStringLiteral("-"), this);
    selForm->addRow(tr("Sum:"), m_selectionSumLabel);
    m_selectionAvgLabel = new QLabel(QStringLiteral("-"), this);
    selForm->addRow(tr("Avg:"), m_selectionAvgLabel);
    mainLayout->addLayout(selForm);

    mainLayout->addStretch();
}

void ColumnProfilePanel::showStats(const ColumnStats& stats)
{
    m_columnNameLabel->setText(stats.columnName);
    m_typeLabel->setText(stats.typeName);
    m_totalRowsLabel->setText(QString::number(stats.totalCount));
    m_filteredRowsLabel->setText(QString::number(stats.filteredCount));
    m_nullCountLabel->setText(QString::number(stats.nullCount));
    m_uniqueCountLabel->setText(QString::number(stats.uniqueCount));
    m_minLabel->setText(stats.min.toString());
    m_maxLabel->setText(stats.max.toString());
    m_sumLabel->setText(stats.sum.toString());
    m_avgLabel->setText(stats.avg.isValid()
                            ? QString::number(stats.avg.toDouble(), 'f', 2)
                            : QStringLiteral("-"));
    m_medianLabel->setText(stats.median.isValid()
                               ? stats.median.toString()
                               : QStringLiteral("-"));

    // Top values
    m_topValuesTable->setRowCount(0);
    for (int i = 0; i < stats.topValues.size(); ++i) {
        m_topValuesTable->insertRow(i);
        const auto& entry = stats.topValues[i];
        m_topValuesTable->setItem(i, 0,
            new QTableWidgetItem(entry.first.toString()));
        m_topValuesTable->setItem(i, 1,
            new QTableWidgetItem(QString::number(entry.second)));
    }

    setLoading(false);
}

void ColumnProfilePanel::clear()
{
    m_columnNameLabel->setText(QStringLiteral("-"));
    m_typeLabel->setText(QStringLiteral("-"));
    m_totalRowsLabel->setText(QStringLiteral("-"));
    m_filteredRowsLabel->setText(QStringLiteral("-"));
    m_nullCountLabel->setText(QStringLiteral("-"));
    m_uniqueCountLabel->setText(QStringLiteral("-"));
    m_minLabel->setText(QStringLiteral("-"));
    m_maxLabel->setText(QStringLiteral("-"));
    m_sumLabel->setText(QStringLiteral("-"));
    m_avgLabel->setText(QStringLiteral("-"));
    m_medianLabel->setText(QStringLiteral("-"));
    m_topValuesTable->setRowCount(0);
    m_selectionSumLabel->setText(QStringLiteral("-"));
    m_selectionAvgLabel->setText(QStringLiteral("-"));
}

void ColumnProfilePanel::setLoading(bool loading)
{
    m_loadingBar->setVisible(loading);
}

} // namespace csvforge
