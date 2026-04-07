#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QTextEdit>
#include <QTreeWidget>

#include "data/model/transformation_pipeline.h"

namespace csvforge {

class TabState;

/// Read-only dialog showing the lineage/provenance of a dataset tab.
class QueryLineageDialog : public QDialog {
    Q_OBJECT
public:
    explicit QueryLineageDialog(const TabState* tabState,
                                const TransformationPipeline& pipeline,
                                QWidget* parent = nullptr);
    ~QueryLineageDialog() override = default;

private:
    void buildUI();
    void populateTree();

    const TabState* m_tabState;
    TransformationPipeline m_pipeline;

    QLabel*       m_titleLabel   = nullptr;
    QLabel*       m_typeLabel    = nullptr;
    QLabel*       m_sourceLabel  = nullptr;
    QTreeWidget*  m_stepsTree    = nullptr;
    QTextEdit*    m_sqlPreview   = nullptr;
    QDialogButtonBox* m_buttonBox = nullptr;
};

} // namespace csvforge
