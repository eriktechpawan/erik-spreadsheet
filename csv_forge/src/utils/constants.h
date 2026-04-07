#pragma once

#include <QString>
#include <QChar>

namespace csvforge {
namespace constants {

// Application info
inline constexpr const char* APP_NAME    = "CSV Forge";
inline constexpr const char* APP_VERSION = "1.0.0";

// Row-window cache
inline constexpr int DEFAULT_PAGE_SIZE    = 10000;
inline constexpr int DEFAULT_CACHE_PAGES  = 3;

// Recent-files list
inline constexpr int MAX_RECENT_FILES = 20;

// CSV defaults
inline constexpr char DEFAULT_DELIMITER = ',';
inline constexpr char DEFAULT_QUOTE_CHAR = '"';

// Session persistence
inline constexpr const char* SESSION_FILE_EXTENSION = ".csvforge";

// DuckDB
inline constexpr const char* DUCKDB_TEMP_TABLE_PREFIX = "csvforge_";

// UI debounce intervals (milliseconds)
inline constexpr int FILTER_DEBOUNCE_MS = 300;
inline constexpr int SEARCH_DEBOUNCE_MS = 200;

// Filter dropdown cap
inline constexpr int MAX_DISTINCT_FILTER_VALUES = 5000;

// Progress reporting
inline constexpr int PROGRESS_UPDATE_INTERVAL_MS = 100;

} // namespace constants
} // namespace csvforge
