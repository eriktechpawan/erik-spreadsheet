#include "file_utils.h"

#include <QDir>
#include <QFileInfo>
#include <QUuid>

namespace csvforge {

qint64 fileSize(const QString& path)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isFile()) {
        return -1;
    }
    return info.size();
}

QString fileSizeFormatted(qint64 bytes)
{
    if (bytes < 0) {
        return QStringLiteral("Unknown");
    }

    constexpr qint64 KB = 1024;
    constexpr qint64 MB = KB * 1024;
    constexpr qint64 GB = MB * 1024;

    if (bytes >= GB) {
        return QStringLiteral("%1 GB").arg(static_cast<double>(bytes) / GB, 0, 'f', 2);
    }
    if (bytes >= MB) {
        return QStringLiteral("%1 MB").arg(static_cast<double>(bytes) / MB, 0, 'f', 2);
    }
    if (bytes >= KB) {
        return QStringLiteral("%1 KB").arg(static_cast<double>(bytes) / KB, 0, 'f', 2);
    }
    return QStringLiteral("%1 bytes").arg(bytes);
}

bool isCSVFile(const QString& path)
{
    return QFileInfo(path).suffix().compare(QLatin1String("csv"), Qt::CaseInsensitive) == 0;
}

bool isTSVFile(const QString& path)
{
    return QFileInfo(path).suffix().compare(QLatin1String("tsv"), Qt::CaseInsensitive) == 0;
}

bool isSupportedFile(const QString& path)
{
    const QString ext = QFileInfo(path).suffix().toLower();
    return ext == QLatin1String("csv")
        || ext == QLatin1String("tsv")
        || ext == QLatin1String("txt");
}

QString generateTempPath(const QString& prefix)
{
    const QString fileName = prefix + QLatin1Char('_')
                           + QUuid::createUuid().toString(QUuid::WithoutBraces)
                           + QLatin1String(".tmp");
    return QDir::temp().filePath(fileName);
}

bool ensureDirectoryExists(const QString& path)
{
    QDir dir(path);
    if (dir.exists()) {
        return true;
    }
    return dir.mkpath(QStringLiteral("."));
}

} // namespace csvforge
