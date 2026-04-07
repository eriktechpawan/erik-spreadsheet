#pragma once

#include <QObject>
#include <QVariant>
#include <QVector>

namespace csvforge {

class RowWindowCache : public QObject {
    Q_OBJECT
public:
    explicit RowWindowCache(QObject* parent = nullptr);

    // Configuration
    void setPageSize(int pageSize);
    void setBufferPages(int pages);
    int pageSize() const;
    int bufferPages() const;

    // Cache operations
    bool hasRow(qint64 row) const;
    QVector<QVariant> getRow(qint64 row) const;
    void insertPage(qint64 startRow, const QVector<QVector<QVariant>>& rows);
    void clear();

    // Cache state
    qint64 cachedRowStart() const;
    qint64 cachedRowEnd() const;
    int cachedRowCount() const;
    bool isEmpty() const;

    // Determine what page to fetch for a given row
    qint64 pageStartForRow(qint64 row) const;
    bool needsFetch(qint64 row) const;

    // Statistics
    int hitCount() const;
    int missCount() const;
    double hitRate() const;
    void resetStats();

signals:
    void cacheInvalidated();
    void pageLoaded(qint64 startRow, int count);

private:
    int m_pageSize;
    int m_bufferPages;
    qint64 m_windowStart = 0;
    QVector<QVector<QVariant>> m_data;
    int m_hits = 0;
    int m_misses = 0;
};

} // namespace csvforge
