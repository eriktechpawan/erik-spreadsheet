#pragma once

#include <QString>
#include <QtTypes>

namespace csvforge {

/// Return the size of a file in bytes, or -1 on error.
qint64 fileSize(const QString& path);

/// Format a byte count into a human-readable string (e.g. "1.23 MB").
QString fileSizeFormatted(qint64 bytes);

/// True if the path has a .csv extension (case-insensitive).
bool isCSVFile(const QString& path);

/// True if the path has a .tsv extension (case-insensitive).
bool isTSVFile(const QString& path);

/// True if the file type is supported (.csv, .tsv, .txt).
bool isSupportedFile(const QString& path);

/// Generate a unique temporary file path with the given prefix inside the system temp dir.
QString generateTempPath(const QString& prefix);

/// Ensure the directory at path exists, creating it if necessary. Returns true on success.
bool ensureDirectoryExists(const QString& path);

} // namespace csvforge
