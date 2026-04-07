cmake_minimum_required(VERSION 3.24)

project(CSVForge
    VERSION 0.1.0
    DESCRIPTION "A high-performance CSV editor powered by DuckDB and Qt6"
    LANGUAGES CXX
)

# -----------------------------------------------------------------------------
# C++ Standard
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Enable Qt's MOC/UIC/RCC automatically
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# -----------------------------------------------------------------------------
# Dependencies — Qt6
# -----------------------------------------------------------------------------
find_package(Qt6 REQUIRED COMPONENTS Widgets Concurrent)

# -----------------------------------------------------------------------------
# Dependencies — DuckDB (C library, downloaded from GitHub releases)
# -----------------------------------------------------------------------------
include(FetchContent)

set(DUCKDB_VERSION "1.2.1")
set(DUCKDB_URL
    "https://github.com/duckdb/duckdb/releases/download/v${DUCKDB_VERSION}/libduckdb-osx-universal.zip"
)

FetchContent_Declare(
    duckdb_lib
    URL      ${DUCKDB_URL}
    DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)
FetchContent_MakeAvailable(duckdb_lib)

# Create an imported target for DuckDB
add_library(duckdb SHARED IMPORTED)
set_target_properties(duckdb PROPERTIES
    IMPORTED_LOCATION "${duckdb_lib_SOURCE_DIR}/libduckdb.dylib"
    INTERFACE_INCLUDE_DIRECTORIES "${duckdb_lib_SOURCE_DIR}"
)

# -----------------------------------------------------------------------------
# Source files
# -----------------------------------------------------------------------------

# Entry point
set(MAIN_SOURCE
    src/main.cpp
)

# src/app/
set(APP_SOURCES
    src/app/Application.cpp
    src/app/AppConfig.cpp
)

# src/ui/
set(UI_SOURCES
    src/ui/MainWindow.cpp
    src/ui/CsvTableView.cpp
    src/ui/StatusBarManager.cpp
    src/ui/ToolBarManager.cpp
)

# src/ui/dialogs/
set(UI_DIALOG_SOURCES
    src/ui/dialogs/ImportDialog.cpp
    src/ui/dialogs/ExportDialog.cpp
    src/ui/dialogs/QueryDialog.cpp
    src/ui/dialogs/PreferencesDialog.cpp
    src/ui/dialogs/AboutDialog.cpp
)

# src/ui/widgets/
set(UI_WIDGET_SOURCES
    src/ui/widgets/CellEditor.cpp
    src/ui/widgets/FilterWidget.cpp
    src/ui/widgets/SearchWidget.cpp
    src/ui/widgets/ColumnHeaderWidget.cpp
)

# src/data/duckdb/
set(DATA_DUCKDB_SOURCES
    src/data/duckdb/DuckDBManager.cpp
    src/data/duckdb/QueryExecutor.cpp
    src/data/duckdb/DataImporter.cpp
    src/data/duckdb/DataExporter.cpp
)

# src/data/model/
set(DATA_MODEL_SOURCES
    src/data/model/CsvTableModel.cpp
    src/data/model/ColumnDefinition.cpp
    src/data/model/QueryResultModel.cpp
)

# src/data/types/
set(DATA_TYPES_SOURCES
    src/data/types/ColumnType.cpp
    src/data/types/TypeConverter.cpp
)

# src/workers/
set(WORKER_SOURCES
    src/workers/ImportWorker.cpp
    src/workers/ExportWorker.cpp
    src/workers/QueryWorker.cpp
)

# src/utils/
set(UTILS_SOURCES
    src/utils/FileUtils.cpp
    src/utils/StringUtils.cpp
    src/utils/CsvParser.cpp
    src/utils/logging.cpp
    src/utils/perf_timer.cpp
    src/utils/string_utils.cpp
    src/utils/file_utils.cpp
    src/utils/csv_sniffer.cpp
)

# Aggregate all library sources (everything except main.cpp)
set(LIB_SOURCES
    ${APP_SOURCES}
    ${UI_SOURCES}
    ${UI_DIALOG_SOURCES}
    ${UI_WIDGET_SOURCES}
    ${DATA_DUCKDB_SOURCES}
    ${DATA_MODEL_SOURCES}
    ${DATA_TYPES_SOURCES}
    ${WORKER_SOURCES}
    ${UTILS_SOURCES}
)

# -----------------------------------------------------------------------------
# Application target
# -----------------------------------------------------------------------------
add_executable(csv_forge MACOSX_BUNDLE
    ${MAIN_SOURCE}
    ${LIB_SOURCES}
)

target_include_directories(csv_forge PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(csv_forge PRIVATE
    Qt6::Widgets
    Qt6::Concurrent
    duckdb
)

# -----------------------------------------------------------------------------
# macOS bundle properties
# -----------------------------------------------------------------------------
set_target_properties(csv_forge PROPERTIES
    MACOSX_BUNDLE TRUE
    MACOSX_BUNDLE_BUNDLE_NAME "CSV Forge"
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.csvforge.app"
    MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/resources/Info.plist"
    OUTPUT_NAME "CSV Forge"
)

# Copy DuckDB dylib into the app bundle so the binary can find it at runtime
add_custom_command(TARGET csv_forge POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
        "$<TARGET_BUNDLE_DIR:csv_forge>/Contents/Frameworks"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${duckdb_lib_SOURCE_DIR}/libduckdb.dylib"
        "$<TARGET_BUNDLE_DIR:csv_forge>/Contents/Frameworks/libduckdb.dylib"
    COMMENT "Bundling libduckdb.dylib into app Frameworks"
)

# Fix up the RPATH so the executable finds the bundled dylib
set_target_properties(csv_forge PROPERTIES
    INSTALL_RPATH "@executable_path/../Frameworks"
    BUILD_RPATH   "@executable_path/../Frameworks"
)

# -----------------------------------------------------------------------------
# Testing
# -----------------------------------------------------------------------------
enable_testing()

# Shared object library for tests (avoids re-compiling all sources per test)
add_library(csv_forge_lib OBJECT ${LIB_SOURCES})

target_include_directories(csv_forge_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(csv_forge_lib PUBLIC
    Qt6::Widgets
    Qt6::Concurrent
    duckdb
)

# --- Unit tests ---
set(UNIT_TEST_SOURCES
    tests/unit/test_CsvTableModel.cpp
    tests/unit/test_DuckDBManager.cpp
    tests/unit/test_QueryExecutor.cpp
    tests/unit/test_CsvParser.cpp
    tests/unit/test_FileUtils.cpp
)

foreach(test_source IN LISTS UNIT_TEST_SOURCES)
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE csv_forge_lib)
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES LABELS "unit")
endforeach()

# --- Integration tests ---
set(INTEGRATION_TEST_SOURCES
    tests/integration/test_ImportExport.cpp
    tests/integration/test_QueryExecution.cpp
    tests/integration/test_EndToEnd.cpp
)

foreach(test_source IN LISTS INTEGRATION_TEST_SOURCES)
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE csv_forge_lib)
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES LABELS "integration")
endforeach()
