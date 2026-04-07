#include "ui/dialogs/query_lineage_dialog.h"
#include "data/model/tab_state.h"

#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QVBoxLayout>

namespace csvforge {

QueryLineageDialog::QueryLineageDialog(const TabState* tabState,
                                       const TransformationPipeline& pipeline,
                                       QWidget* parent)
    : QDialog(parent)
    , m_tabState(tabState)
    , m_pipeline(pipeline)
{
    setWindowTitle(tr("Dataset Lineage"));
    setMinimumSize(600, 500);
    buildUI();
    populateTree();
}

void QueryLineageDialog::buildUI()
{
    auto* mainLayout = new QVBoxLayout(this);

    // Header info
    auto* headerGroup = new QGroupBox(tr("Dataset Information"), this);
    auto* headerLayout = new QVBoxLayout(headerGroup);

    m_titleLabel = new QLabel(this);
    m_titleLabel->setStyleSheet(QStringLiteral("font-weight: bold; font-size: 14px;"));
    m_typeLabel = new QLabel(this);
    m_sourceLabel = new QLabel(this);

    if (m_tabState) {
        const auto& desc = m_tabState->descriptor();
        m_titleLabel->setText(desc.displayName);
        m_typeLabel->setText(QStringLiteral("Type: %1").arg(desc.typeBadge()));
        m_sourceLabel->setText(
            desc.sourceFilePath.isEmpty()
                ? QStringLiteral("Source: (derived)")
                : QStringLiteral("Source: %1").arg(desc.sourceFilePath));
    }

    headerLayout->addWidget(m_titleLabel);
    headerLayout->addWidget(m_typeLabel);
    headerLayout->addWidget(m_sourceLabel);
    mainLayout->addWidget(headerGroup);

    // Transformation steps tree
    auto* stepsGroup = new QGroupBox(tr("Transformation Steps"), this);
    auto* stepsLayout = new QVBoxLayout(stepsGroup);
    m_stepsTree = new QTreeWidget(this);
    m_stepsTree->setHeaderLabels({tr("Step"), tr("Type"), tr("Description")});
    m_stepsTree->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    m_stepsTree->setAlternatingRowColors(true);
    stepsLayout->addWidget(m_stepsTree);
    mainLayout->addWidget(stepsGroup);

    // SQL preview
    auto* sqlGroup = new QGroupBox(tr("Generated SQL Preview"), this);
    auto* sqlLayout = new QVBoxLayout(sqlGroup);
    m_sqlPreview = new QTextEdit(this);
    m_sqlPreview->setReadOnly(true);
    m_sqlPreview->setFont(QFont(QStringLiteral("Menlo"), 12));
    m_sqlPreview->setPlainText(m_pipeline.toSQLPreview());
    sqlLayout->addWidget(m_sqlPreview);
    mainLayout->addWidget(sqlGroup);

    // Buttons
    m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, this);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(m_buttonBox);
}

void QueryLineageDialog::populateTree()
{
    m_stepsTree->clear();
    int stepNum = 1;
    for (const auto& step : m_pipeline.steps) {
        auto* item = new QTreeWidgetItem(m_stepsTree);
        item->setText(0, QStringLiteral("#%1").arg(stepNum++));
        item->setText(1, transformationTypeToString(step.type));
        item->setText(2, step.description);

        // Add config details as children
        const auto keys = step.config.keys();
        for (const auto& key : keys) {
            auto* child = new QTreeWidgetItem(item);
            child->setText(0, QString());
            child->setText(1, key);
            const auto val = step.config.value(key);
            if (val.isArray()) {
                QStringList items;
                for (const auto& v : val.toArray())
                    items << v.toString();
                child->setText(2, items.join(QStringLiteral(", ")));
            } else {
                child->setText(2, val.toVariant().toString());
            }
        }
    }
    m_stepsTree->expandAll();
}

} // namespace csvforge
