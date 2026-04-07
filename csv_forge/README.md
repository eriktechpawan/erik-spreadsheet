# CSV Forge

A high-performance desktop CSV editor for macOS, built with **Qt6** and powered by **DuckDB**. CSV Forge lets you open, explore, query, and transform large CSV files with the speed of an embedded analytical database — all from a native, responsive GUI.

---

## Features

- **Blazing-fast CSV loading** — DuckDB's columnar engine ingests even multi-GB files in seconds.
- **SQL query editor** — run arbitrary SQL against your data and view results instantly.
- **In-place cell editing** — modify individual cells through a familiar spreadsheet interface.
- **Column filtering & search** — narrow down rows with per-column filters or full-text search.
- **Import / Export dialogs** — fine-tune delimiters, encodings, and quoting when reading or writing files.
- **Background processing** — long-running imports, exports, and queries execute on worker threads so the UI stays responsive.
- **Type-aware columns** — automatic type detection with manual override (integer, real, text, date, boolean, …).
- **Preferences** — configurable defaults for delimiters, locale, theme, and more.
- **Native macOS experience** — ships as a `.app` bundle with proper Retina support on Apple Silicon.

---

## Build Prerequisites

| Requirement | Minimum Version | Install |
|---|---|---|
| macOS | 13 Ventura (Apple Silicon) | — |
| Xcode Command Line Tools | 15+ | `xcode-select --install` |
| [Homebrew](https://brew.sh) | latest | `/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"` |
| CMake | 3.24+ | `brew install cmake` |
| Qt 6 | 6.5+ | `brew install qt@6` |

> **Note:** DuckDB is downloaded automatically during the CMake configure step — no manual installation required.

---

## Build Instructions (Apple Silicon Mac)

```bash
# 1. Clone the repository
git clone https://github.com/eriktechpawan/erik-spreadsheet.git
cd erik-spreadsheet/csv_forge

# 2. Create an out-of-source build directory
cmake -B build -S . \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)"

# 3. Build the application
cmake --build build --parallel

# 4. Run the app bundle
open build/CSV\ Forge.app
# — or —
./build/CSV\ Forge.app/Contents/MacOS/CSV\ Forge
```

### Running Tests

```bash
# Run all tests
cd build && ctest --output-on-failure

# Run only unit tests
ctest --output-on-failure -L unit

# Run only integration tests
ctest --output-on-failure -L integration
```

---

## Usage Guide

1. **Open a CSV file** — use *File → Open* or drag-and-drop a `.csv` onto the window.
2. **Browse data** — scroll through the table view; columns are auto-typed on import.
3. **Edit cells** — double-click any cell to edit its value in place.
4. **Filter & search** — use the filter bar above each column or the global search widget.
5. **Run SQL** — open the query dialog (*Tools → SQL Query*), type a query such as `SELECT * FROM data WHERE amount > 100`, and press *Execute*.
6. **Export** — *File → Export* lets you save the current view (or query result) to CSV, TSV, or Parquet.

---

## Architecture Overview

```
csv_forge/
├── CMakeLists.txt              # Build system
├── resources/                  # App icons, Info.plist, etc.
├── src/
│   ├── main.cpp                # Application entry point
│   ├── app/                    # Application & configuration classes
│   │   ├── Application.cpp
│   │   └── AppConfig.cpp
│   ├── ui/                     # Top-level UI components
│   │   ├── MainWindow.cpp
│   │   ├── CsvTableView.cpp
│   │   ├── StatusBarManager.cpp
│   │   ├── ToolBarManager.cpp
│   │   ├── dialogs/            # Modal dialogs
│   │   │   ├── ImportDialog.cpp
│   │   │   ├── ExportDialog.cpp
│   │   │   ├── QueryDialog.cpp
│   │   │   ├── PreferencesDialog.cpp
│   │   │   └── AboutDialog.cpp
│   │   └── widgets/            # Reusable custom widgets
│   │       ├── CellEditor.cpp
│   │       ├── FilterWidget.cpp
│   │       ├── SearchWidget.cpp
│   │       └── ColumnHeaderWidget.cpp
│   ├── data/
│   │   ├── duckdb/             # DuckDB integration layer (C API)
│   │   │   ├── DuckDBManager.cpp
│   │   │   ├── QueryExecutor.cpp
│   │   │   ├── DataImporter.cpp
│   │   │   └── DataExporter.cpp
│   │   ├── model/              # Qt data models
│   │   │   ├── CsvTableModel.cpp
│   │   │   ├── ColumnDefinition.cpp
│   │   │   └── QueryResultModel.cpp
│   │   └── types/              # Type system helpers
│   │       ├── ColumnType.cpp
│   │       └── TypeConverter.cpp
│   ├── workers/                # QRunnable / QtConcurrent workers
│   │   ├── ImportWorker.cpp
│   │   ├── ExportWorker.cpp
│   │   └── QueryWorker.cpp
│   └── utils/                  # Shared utilities
│       ├── FileUtils.cpp
│       ├── StringUtils.cpp
│       └── CsvParser.cpp
├── tests/
│   ├── unit/                   # Fast, isolated unit tests
│   │   ├── test_CsvTableModel.cpp
│   │   ├── test_DuckDBManager.cpp
│   │   ├── test_QueryExecutor.cpp
│   │   ├── test_CsvParser.cpp
│   │   └── test_FileUtils.cpp
│   └── integration/            # End-to-end & integration tests
│       ├── test_ImportExport.cpp
│       ├── test_QueryExecution.cpp
│       └── test_EndToEnd.cpp
└── third_party/                # Reserved for vendored dependencies
```

### Key design decisions

| Decision | Rationale |
|---|---|
| **DuckDB via the C API** | Keeps the build simple (single header + dylib) and avoids C++ ABI issues across compiler versions. |
| **Qt6 Widgets over QML** | Provides a mature, table-centric widget set ideal for spreadsheet UIs. |
| **Worker threads via QtConcurrent** | Offloads heavy I/O and query work to background threads while keeping signal/slot integration with the UI. |
| **Object library for tests** | All source files are compiled once into a CMake OBJECT library that both the app executable and every test binary link against — fast incremental rebuilds. |

---

## License

This project is released under the [MIT License](../LICENSE).
