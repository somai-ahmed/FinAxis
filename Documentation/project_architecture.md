# FinAxis Accounting & Fraud Detection Engine — Architecture

_Supersedes GUI/Excel decisions in Project Explication Book v1.0. C core API and business rules (sections 3, 4, 8, 9, 10) are unchanged._

## 1. Stack Summary

| Layer | v1.0 (original) | Current |
|---|---|---|
| Core engine | C, Windows DLL | C, Windows DLL (unchanged) |
| GUI | VB.NET / Windows Forms | **Python + PySide6** |
| Interop | P/Invoke | **ctypes** |
| Excel I/O | ClosedXML | **openpyxl** (proposed — swap if you already have a preference) |
| Persistence | Flat JSON | Flat JSON, needs a C JSON lib — **cJSON** suggested |

## 2. Naming Conventions (resolved)

- Accounting-domain files/functions: **French** — `comptes`, `ecritures`, `periodes`, `monnaie`, `identifiants`, `balance`, `bilan`, `grand_livre`, `compte_resultat`, `doublons`, `montants_ronds`, `valeurs_aberrantes`, etc.
- Infra/generic files: **English** — `core`, `utils`, `session`, `errors`, `validation`, `dates`, `strings`, `math_utils`, `reports`, `detection`, `persistence`.
- Docs are updated to match code, not the reverse (code never renamed to fit an old doc).
- Public headers live flat in `include/`; internal-only headers live next to their `.c` in `src/`.

## 3. Directory Tree

```
FinAxis/
├── Src/
│   ├── FinAxis.Native/
│   │   ├── include/
│   │   │   ├── finaxis_ae.h        # umbrella header, includes all below
│   │   │   ├── types.h
│   │   │   ├── errors.h
│   │   │   ├── dates.h
│   │   │   ├── monnaie.h
│   │   │   ├── identifiants.h
│   │   │   ├── validation.h
│   │   │   ├── comptes.h
│   │   │   ├── ecritures.h
│   │   │   ├── periodes.h
│   │   │   ├── reports.h           # GL / Balance / Bilan / Compte de Resultat
│   │   │   ├── detection.h         # RunDetection + 4 Check* + Benford dist
│   │   │   └── session.h
│   │   ├── src/
│   │   │   ├── core/
│   │   │   │   ├── session.c       # [MVP] lifecycle, in-memory working set
│   │   │   │   └── errors.c        # [MVP] error table / GetLastError
│   │   │   ├── utils/
│   │   │   │   ├── dates.c         # [MVP]
│   │   │   │   ├── monnaie.c       # [MVP]
│   │   │   │   ├── monnaie_convert.c # [MVP]
│   │   │   │   ├── identifiants.c  # [MVP]
│   │   │   │   ├── validation.c    # [MVP]
│   │   │   │   ├── strings.c       # [MVP]
│   │   │   │   └── math_utils.c    # [MVP] + stats helpers (chi-sq, incomplete
│   │   │   │                       #        gamma, z-score) reused by detection/
│   │   │   ├── accounting/
│   │   │   │   ├── comptes.c       # [done]
│   │   │   │   ├── ecritures.c     # [done]
│   │   │   │   └── periodes.c      # [Stretch] period open/close
│   │   │   ├── reports/
│   │   │   │   ├── balance.c       # [MVP]
│   │   │   │   ├── grand_livre.c   # [MVP]
│   │   │   │   ├── bilan.c         # [MVP]
│   │   │   │   └── compte_resultat.c # [Stretch]
│   │   │   ├── detection/
│   │   │   │   ├── detection.c     # [MVP] dispatcher (RunDetection)
│   │   │   │   ├── benford.c       # [MVP]
│   │   │   │   ├── doublons.c      # [Stretch]
│   │   │   │   ├── montants_ronds.c # [Stretch]
│   │   │   │   └── valeurs_aberrantes.c # [Stretch]
│   │   │   └── persistence/
│   │   │       └── session_io.c    # [Stretch for demo] JSON save/load, needs cJSON
│   │   └── build/
│   │       └── FinAxis_ae.dll      # (.so if building for Linux dev/testing)
│   └── FinAxis.Desktop/            # Python + PySide6
│       ├── bindings/
│       │   ├── finaxis_ffi.py      # ctypes.CDLL load + prototypes
│       │   ├── structs.py          # ctypes.Structure mirrors of C structs
│       │   └── marshaling.py       # buffer/array helpers
│       ├── models/
│       │   ├── account.py
│       │   ├── journal_entry.py
│       │   ├── period.py
│       │   └── detection_result.py
│       ├── services/
│       │   ├── session_service.py
│       │   ├── account_service.py
│       │   ├── journal_service.py
│       │   ├── report_service.py
│       │   ├── detection_service.py
│       │   └── persistence_service.py
│       ├── helpers/
│       │   ├── date_helper.py
│       │   ├── currency_helper.py
│       │   └── validation_helper.py
│       ├── ui/
│       │   ├── windows/
│       │   │   ├── main_window.py       # QMainWindow
│       │   │   ├── login_dialog.py
│       │   │   ├── account_chart_window.py
│       │   │   ├── journal_entry_window.py
│       │   │   ├── grand_livre_window.py
│       │   │   ├── balance_window.py
│       │   │   ├── bilan_window.py
│       │   │   ├── detection_window.py
│       │   │   ├── benford_chart_window.py
│       │   │   ├── import_window.py     # Stretch
│       │   │   └── period_manager_window.py # Stretch
│       │   ├── widgets/
│       │   │   ├── amount_line_edit.py  # QLineEdit + QDoubleValidator
│       │   │   │   ├── account_combo_box.py # QComboBox + QCompleter
│       │   │   │   ├── journal_table_widget.py # QTableWidget
│       │   │   │   └── report_viewer_widget.py
│       │   └── dialogs/
│       │       ├── export_dialog.py
│       │       ├── detection_config_dialog.py
│       │       └── about_dialog.py
│       ├── excel/
│       │   ├── exporter.py          # openpyxl writer
│       │   ├── importer.py          # Stretch
│       │   ├── templates/
│       │   │   ├── grand_livre_template.py
│       │   │   ├── balance_template.py
│       │   │   ├── bilan_template.py
│       │   │   └── detection_template.py
│       │   └── styles/
│       │       ├── cell_styles.py
│       │       └── color_palette.py
│       ├── resources/
│       │   └── style.qss
│       └── main.py
├── tests/
│   ├── native/
│   │   ├── test_comptes.c
│   │   ├── test_ecritures.c
│   │   ├── test_balance.c
│   │   ├── test_detection.c
│   │   └── test_utils.c
│   └── desktop/
│       ├── test_bindings.py
│       ├── test_services.py
│       └── test_excel_export.py
├── docs/
│   ├── architecture.md          # this file
│   ├── api_reference.md
│   ├── file_format.md
│   └── demo_script.md
├── .github/workflows/
│   ├── build-native.yml
│   ├── build-desktop.yml
│   └── release.yml
└── demo_data.FinAxis
```

## 4. Module Responsibilities — Native

| Module | Owns |
|---|---|
| `core/` | Session lifecycle, error reporting |
| `utils/` | Dates, money (two representations + bridge), IDs, validation, strings, stats math |
| `accounting/` | Chart of accounts, journal entries, periods |
| `reports/` | GL, trial balance, balance sheet, income statement |
| `detection/` | Benford, duplicates, round numbers, outliers |
| `persistence/` | `.FinAxis` JSON read/write |

## 5. Module Responsibilities — Desktop

| Module | Owns |
|---|---|
| `bindings/` | All ctypes plumbing — only module that touches the DLL directly |
| `models/` | Plain Python dataclasses mirroring C structs |
| `services/` | Business orchestration, calls `bindings/` |
| `ui/` | PySide6 windows/widgets, call `services/` only — never `bindings/` directly |
| `excel/` | openpyxl export/import, isolated from UI |

## 6. Changes vs. Project Explication Book v1.0

1. GUI: VB.NET/WinForms/P-Invoke → Python/PySide6/ctypes.
2. Excel: ClosedXML → openpyxl (open decision, flagged above).
3. Persistence needs a C JSON library — cJSON suggested, not yet confirmed.
4. `types.h`/`errors.h` renamed from `FinAxis_types.h`/`FinAxis_errors.h`.
5. French names kept for domain files (`comptes.c`, `ecritures.c`, etc.) — doc updated to match, not the reverse.
6. `monnaie.h`, `identifiants.h`, `monnaie_convert.c` placed in `utils/`; `validation.c` stays in `utils/` (not `accounting/`); `errors.c` placed in `core/`.

