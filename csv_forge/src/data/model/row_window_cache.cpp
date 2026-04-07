#include "data/model/row_window_cache.h"
#include "utils/constants.h"

namespace csvforge {

RowWindowCache::RowWindowCache(QObject* parent)
    : QObject(parent)
    , m_pageSize(constants::DEFAULT_PAGE_SIZE)
    , m_bufferPages(constants::DEFAULT_CACHE_PAGES)
{
}

void RowWindowCache::setPageSize(int pageSize)
{
    if (pageSize > 0) {
        m_pageSize = pageSize;
        clear();
    }
}

void RowWindowCache::setBufferPages(int pages)
{
    if (pages > 0) {
        m_bufferPages = pages;
        clear();
    }
}

int RowWindowCache::pageSize() const
{
    return m_pageSize;
}

int RowWindowCache::bufferPages() const
{
    return m_bufferPages;
}

bool RowWindowCache::hasRow(qint64 row) const
{
    if (m_data.isEmpty()) {
        return false;
    }
    return row >= m_windowStart && row < m_windowStart + m_data.size();
}

QVector<QVariant> RowWindowCache::getRow(qint64 row) const
{
    if (!hasRow(row)) {
        // Const-correct: update stats via mutable-equivalent pattern
        const_cast<RowWindowCache*>(this)->m_misses++;
        return {};
    }
    const_cast<RowWindowCache*>(this)->m_hits++;
    return m_data[static_cast<int>(row - m_windowStart)];
}

void RowWindowCache::insertPage(qint64 startRow, const QVector<QVector<QVariant>>& rows)
{
    if (rows.isEmpty()) {
        return;
    }

    // Calculate the window we want to maintain around this page.
    // Keep up to m_bufferPages * m_pageSize rows centered around the new page.
    const qint64 maxCacheRows = static_cast<qint64>(m_bufferPages) * m_pageSize;

    // If the new data is contiguous or overlapping with current cache, merge
    if (!m_data.isEmpty()) {
        const qint64 currentEnd = m_windowStart + m_data.size();
        const qint64 newEnd = startRow + rows.size();

        // Check if new page is adjacent/overlapping
        if (startRow >= m_windowStart && startRow <= currentEnd) {
            // Append or overwrite overlapping portion
            const qint64 overlapStart = startRow - m_windowStart;
            const int newDataNeeded = static_cast<int>(newEnd - m_windowStart) - m_data.size();
            if (newDataNeeded > 0) {
                m_data.resize(m_data.size() + newDataNeeded);
            }
            for (int i = 0; i < rows.size(); ++i) {
                m_data[static_cast<int>(overlapStart) + i] = rows[i];
            }
        } else if (startRow < m_windowStart && newEnd >= m_windowStart) {
            // Prepend overlapping
            QVector<QVector<QVariant>> merged;
            merged.reserve(static_cast<int>(currentEnd - startRow));
            merged = rows;
            const qint64 skip = newEnd - m_windowStart;
            for (qint64 i = skip; i < m_data.size(); ++i) {
                merged.append(m_data[static_cast<int>(i)]);
            }
            m_data = std::move(merged);
            m_windowStart = startRow;
        } else {
            // Non-contiguous: replace entirely
            m_data = rows;
            m_windowStart = startRow;
        }
    } else {
        m_data = rows;
        m_windowStart = startRow;
    }

    // Trim cache to max size, keeping data centered around the newest page
    if (m_data.size() > maxCacheRows) {
        // Determine the center point of the new page within our data
        const qint64 newPageCenter = startRow + rows.size() / 2;
        const qint64 desiredStart = qMax(qint64(0), newPageCenter - maxCacheRows / 2);
        const qint64 trimFront = qMax(qint64(0), desiredStart - m_windowStart);
        if (trimFront > 0 && trimFront < m_data.size()) {
            m_data.remove(0, static_cast<int>(trimFront));
            m_windowStart += trimFront;
        }
        if (m_data.size() > maxCacheRows) {
            m_data.resize(static_cast<int>(maxCacheRows));
        }
    }

    emit pageLoaded(startRow, rows.size());
}

void RowWindowCache::clear()
{
    m_windowStart = 0;
    m_data.clear();
    m_data.squeeze();
    emit cacheInvalidated();
}

qint64 RowWindowCache::cachedRowStart() const
{
    return m_windowStart;
}

qint64 RowWindowCache::cachedRowEnd() const
{
    return m_windowStart + m_data.size();
}

int RowWindowCache::cachedRowCount() const
{
    return m_data.size();
}

bool RowWindowCache::isEmpty() const
{
    return m_data.isEmpty();
}

qint64 RowWindowCache::pageStartForRow(qint64 row) const
{
    if (m_pageSize <= 0) {
        return 0;
    }
    return (row / m_pageSize) * m_pageSize;
}

bool RowWindowCache::needsFetch(qint64 row) const
{
    return !hasRow(row);
}

int RowWindowCache::hitCount() const
{
    return m_hits;
}

int RowWindowCache::missCount() const
{
    return m_misses;
}

double RowWindowCache::hitRate() const
{
    const int total = m_hits + m_misses;
    if (total == 0) {
        return 0.0;
    }
    return static_cast<double>(m_hits) / total;
}

void RowWindowCache::resetStats()
{
    m_hits = 0;
    m_misses = 0;
}

} // namespace csvforge
