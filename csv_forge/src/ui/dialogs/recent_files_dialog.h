#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QStringList>
#include <QTableWidget>

namespace csvforge {

// ---------------------------------------------------------------------------
// RecentFilesDialog – browse and open recently used files
// ---------------------------------------------------------------------------

class RecentFilesDialog : public QDialog {
    Q_OBJECT

public:
    explicit RecentFilesDialog(const QStringList& recentFiles,
                               QWidget* parent = nullptr);
    ~RecentFilesDialog() override = default;

    QString selectedFile() const;

signals:
    void filesChanged(const QStringList& remaining);

private slots:
    void onOpen();
    void onRemove();
    void onClearAll();
    void onRowDoubleClicked(int row, int column);

private:
    void buildUI();
    void populateTable();

    QTableWidget* m_table = nullptr;

    QPushButton* m_openBtn     = nullptr;
    QPushButton* m_removeBtn   = nullptr;
    QPushButton* m_clearAllBtn = nullptr;

    QDialogButtonBox* m_buttonBox = nullptr;

    QStringList m_recentFiles;
    QString     m_selectedFile;
};

} // namespace csvforge
