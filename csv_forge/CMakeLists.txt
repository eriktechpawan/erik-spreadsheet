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
    src/app/tab_manager.cpp
)

# src/ui/
set(UI_SOURCES
    src/ui/main_window.cpp
    src/ui/menu_manager.cpp
    src/ui/toolbar_manager.cpp
    src/ui/status_bar_manager.cpp
)

# src/ui/dialogs/
set(UI_DIALOG_SOURCES
    src/ui/dialogs/import_dialog.cpp
    src/ui/dialogs/export_dialog.cpp
    src/ui/dialogs/filter_builder_dialog.cpp
    src/ui/dialogs/column_stats_dialog.cpp
    src/ui/dialogs/pivot_table_dialog.cpp
    src/ui/dialogs/lookup_wizard_dialog.cpp
    src/ui/dialogs/column_manager_dialog.cpp
    src/ui/dialogs/preferences_dialog.cpp
    src/ui/dialogs/recent_files_dialog.cpp
    src/ui/dialogs/calculated_column_dialog.cpp
    src/ui/dialogs/query_lineage_dialog.cpp
)

# src/ui/widgets/
set(UI_WIDGET_SOURCES
    src/ui/widgets/data_table_view.cpp
    src/ui/widgets/filter_panel.cpp
    src/ui/widgets/search_bar.cpp
    src/ui/widgets/active_filter_chip_bar.cpp
    src/ui/widgets/column_profile_panel.cpp
    src/ui/widgets/progress_overlay.cpp
    src/ui/widgets/result_tab_widget.cpp
)

# src/data/duckdb/
set(DATA_DUCKDB_SOURCES
    src/data/duckdb/duckdb_connection.cpp
    src/data/duckdb/duckdb_engine.cpp
    src/data/duckdb/query_builder.cpp
    src/data/duckdb/csv_import_service.cpp
    src/data/duckdb/stats_service.cpp
    src/data/duckdb/export_service.cpp
    src/data/duckdb/pivot_service.cpp
    src/data/duckdb/lookup_service.cpp
)

# src/data/model/
set(DATA_MODEL_SOURCES
    src/data/model/table_model.cpp
    src/data/model/row_window_cache.cpp
    src/data/model/filter_state.cpp
    src/data/model/sort_state.cpp
    src/data/model/edit_delta_store.cpp
    src/data/model/session_state.cpp
    src/data/model/dataset_descriptor.cpp
    src/data/model/tab_state.cpp
    src/data/model/transformation_step.cpp
    src/data/model/transformation_pipeline.cpp
)

# src/data/types/
set(DATA_TYPES_SOURCES
    src/data/types/pivot_config.cpp
    src/data/types/lookup_config.cpp
    src/data/types/formula_config.cpp
)

# src/data/formula/
set(DATA_FORMULA_SOURCES
    src/data/formula/formula_parser.cpp
    src/data/formula/formula_engine.cpp
    src/data/formula/formula_sql_translator.cpp
)

# src/workers/
set(WORKER_SOURCES
    src/workers/background_task.cpp
    src/workers/task_dispatcher.cpp
    src/workers/query_worker.cpp
    src/workers/import_worker.cpp
    src/workers/stats_worker.cpp
    src/workers/export_worker.cpp
)

# src/utils/
set(UTILS_SOURCES
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
    ${DATA_FORMULA_SOURCES}
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
    tests/unit/test_query_builder.cpp
    tests/unit/test_filter_state.cpp
    tests/unit/test_sort_state.cpp
    tests/unit/test_row_window_cache.cpp
    tests/unit/test_edit_delta_store.cpp
    tests/unit/formula/test_formula_parser.cpp
    tests/unit/formula/test_formula_sql_translator.cpp
    tests/unit/pivot/test_pivot_config.cpp
    tests/unit/lookup/test_lookup_config.cpp
    tests/unit/pipeline/test_transformation_pipeline.cpp
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
    tests/integration/test_import_export.cpp
    tests/integration/test_stats_service.cpp
    tests/integration/test_session_state.cpp
)

foreach(test_source IN LISTS INTEGRATION_TEST_SOURCES)
    get_filename_component(test_name ${test_source} NAME_WE)
    add_executable(${test_name} ${test_source})
    target_link_libraries(${test_name} PRIVATE csv_forge_lib)
    add_test(NAME ${test_name} COMMAND ${test_name})
    set_tests_properties(${test_name} PROPERTIES LABELS "integration")
endforeach()
