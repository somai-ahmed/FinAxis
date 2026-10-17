# Desktop Application Guide

Python 3 + PySide6, folder `Src/FinAxis.Desktop/`. Start with `python main.py`.

## Layering rules

```
ui/  ──►  services/  ──►  bindings/  ──►  C library
```

- `ui/` never imports `bindings/`.
- `bindings/` is the only code that touches the shared library.
- `Excel/` is isolated from the UI.

## Folders

| Folder | Content |
|---|---|
| `bindings/finaxis_ffi.py` | Library loader, `ctypes` prototypes (46 functions), `ErreurFinCore` exception |
| `bindings/structs.py` | `ctypes.Structure` mirrors of the C structs |
| `bindings/marshaling.py` | Buffer and array helpers |
| `models/` | `account`, `journal_entry`, `period`, `report`, `resultat_detection` |
| `services/` | `session_service`, `account_service`, `journal_service`, `period_service`, `report_service`, `detetction_service`, `persistence`, `demo_data` |
| `helpers/` | `date_helper`, `currency_helper`, `validation_helper` |
| `ui/context.py` | `AppContext`: shared state (library handle, current session) |
| `ui/windows/` | Main window, login, chart of accounts, journal entry, grand livre, balance, bilan, detection, Benford chart, import, period manager |
| `ui/widgets/` | `account_combo_box`, `amount_combo_box`, `journal_table_widget`, `report_viewer_widget` |
| `ui/dialogs/` | Export, detection config, about |
| `Excel/` | `exporter.py`, `importer.py`, `templates/` (balance, bilan, grand livre, detection), `styles/` |
| `resources/style.qss` | Qt stylesheet |

## Main menu

`Fichier` · `Données` · `Rapports` · `Analyse` · `Aide`

## Error handling

Any negative `Etat` returned by the C engine is raised as `ErreurFinCore(code, message)`; the message comes from `GetErrorMessage`. If the library is missing, `BibliothequeIntrouvable` is shown in a dialog at startup.

## Packaging

`main.py` and the loader already support PyInstaller (`sys._MEIPASS`). Bundle the shared library and `resources/style.qss` as data files.

## Tests

`pytest` is in `requirements.txt`. The architecture plans `tests/desktop/` (bindings, services, Excel export) and `tests/native/`; these folders are not in the repo yet.
