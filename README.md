# FinAxis

**FinAxis** is a double-entry accounting and fraud-detection engine for the Tunisian chart of accounts (PCG-style classes 1–7). A native C core does the accounting and statistics; a Python/PySide6 desktop app drives it through `ctypes`.

> Status: v1.0.0 — work in progress. See [Known issues](Documentation/known_issues.md) before building.

## Features

- **Accounting core (C11)** — chart of accounts, journal entries (balanced-entry validation), fiscal periods (open / closed / locked), fixed-point money in millimes (no floating-point rounding errors).
- **Reports** — trial balance (*balance*), general ledger (*grand livre*), balance sheet (*bilan*) with hierarchical depth, income statement (*compte de résultat*).
- **Anomaly detection** — four methods, each with a severity level (`INFO` → `CRITIQUE`) and a numeric score:
  - Benford's Law (chi-square test, 8 degrees of freedom, 95 % threshold = 15.507)
  - Duplicate entries within a configurable day window
  - Suspiciously round amounts
  - Statistical outliers (z-score threshold, default 3σ)
- **Persistence** — sessions saved as `.FinAxis` JSON files (cJSON).
- **Desktop GUI (PySide6)** — login / new company / demo data, chart of accounts, journal entry, ledger, balance, bilan, detection, Benford chart, period manager, Excel import / export (openpyxl).
- **Tunisian chart of accounts** — `bilan_tunisien.json` ships the account classes and labels as seed data.

## Architecture at a glance

```
 PySide6 UI  ──►  services/  ──►  bindings/ (ctypes)  ──►  FinAxis_ae.dll / .so  (C core)
                     │
                     └──►  Excel/ (openpyxl import / export)
```

| Layer | Path | Role |
|---|---|---|
| C engine | `Src/FinCore.Native/` | Accounting, reports, detection, persistence |
| FFI | `Src/FinAxis.Desktop/bindings/` | Only module that touches the shared library |
| Services | `Src/FinAxis.Desktop/services/` | Orchestration between UI and engine |
| UI | `Src/FinAxis.Desktop/ui/` | Windows, dialogs, widgets (never call `bindings/` directly) |
| Excel | `Src/FinAxis.Desktop/Excel/` | Export templates and import |

Full details: [Documentation/project_architecture.md](Documentation/project_architecture.md).

## Quick start

Requirements: Python ≥ 3.10, CMake ≥ 3.16, a C11 compiler (GCC / Clang / MSVC).

```bash
git clone https://github.com/somai-ahmed/FinAxis
cd FinAxis

# 1. Python dependencies
pip install -r requirements.txt

# 2. Build the native engine  (see Documentation/getting_started.md)
cd Src/FinCore.Native
cmake -S . -B build
cmake --build build

# 3. Run the desktop app
cd ../FinAxis.Desktop
python main.py
```

If the library is not found, point to it explicitly with the `FINCORE_DLL` environment variable.

## Documentation

| File | Content |
|---|---|
| [getting_started.md](Documentation/getting_started.md) | Install, build, run, troubleshoot |
| [project_architecture.md](Documentation/project_architecture.md) | Stack, naming conventions, module map |
| [api_reference.md](Documentation/api_reference.md) | Public C API (sessions, accounts, entries, periods, reports, detection) |
| [detection_engine.md](Documentation/detection_engine.md) | Detection methods, config, scoring, Benford |
| [file_format.md](Documentation/file_format.md) | `.FinAxis` JSON format, version 1 |
| [desktop_guide.md](Documentation/desktop_guide.md) | Python layers, FFI, UI windows |
| [known_issues.md](Documentation/known_issues.md) | Current inconsistencies and how to fix them |
| [Explication_Bilan/](Documentation/Explication_Bilan/explication_ecriture_bilan.txt) | How balance-sheet depth (`profondeur`) works (FR) |


## License

Apache License 2.0 — see [LICENSE](LICENSE). Bundled: [cJSON](Src/FinCore.Native/Src/cJSON/LICENSE) (MIT).

## Author

Ahmed Somai — [@somai-ahmed](https://github.com/somai-ahmed)
