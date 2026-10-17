# Known Issues

The project was partly renamed from **FinCore** to **FinAxis**. These leftovers stop a clean build/run. Each fix is small.

| # | Where | Problem | Fix |
|---|---|---|---|
| 1 | `Src/FinCore.Native/CMakeLists.txt` | Sources are stored in `FINAXIS_SOURCES` but `add_library` uses `${FINCORE_SOURCES}` (empty) | Use `${FINAXIS_SOURCES}` |
| 2 | Same file | Source paths do not match the repo: `string_fincore.c` (real: `string_finaxis.c`), `Detection/detection.c` (real: `detetction.c`), `Detection/valeurs_aberrantes.c` (real: `valeur_aberrantes.c`), `third_party/cJSON/cJSON.c` (real: `Src/cJSON/cJSON.c`), `Src/Core/...` listed relative to the CMake folder | Align the names (CMake or file names) |
| 3 | `bindings/finaxis_ffi.py` | Looks for `FinCore_ae.dll/.so`; CMake builds `FinAxis_ae` | Rename in `_NOMS_BIBLIOTHEQUE` |
| 4 | `main.py` | `from bindings.fincore_ffi import ...`; the file is `finaxis_ffi.py` | Fix the import |
| 5 | FFI vs C | FFI calls `FinCore_Version` / `FinCore_TailleStruct`; C exports `finAxis_Version` / `finAxis_TailleStruct` | Rename on one side |
| 6 | C sources | Includes use `"Src/FinCore.Native/include/..."` (repo-root paths); CMake only adds `include` | Add the repo root to `target_include_directories`, or switch to `"types.h"` style |
| 7 | `Documentation/project_architecture.md` | Describes lowercase folders (`src/`, `finaxis_ae.h`); real tree uses `Src/`, `FinAxis_AE.h`, `Detection/`, `Rapports/` | Update the doc (project rule: docs follow code) |
| 8 | Repo | `__pycache__` folders and `.pyc` files are committed; no `.gitignore` | Add `.gitignore` (`__pycache__/`, `*.pyc`, `build/`, `.venv/`) and `git rm -r --cached` them |
| 9 | Repo | `tests/` and `.github/workflows/` are planned but absent; CMake references `../../tests/native/*.c` | Add tests or keep `FINCORE_BUILD_TESTS=OFF` |
| 10 | Naming | Typos in file names: `detetction.c`, `detetction_service.py`, `valeur_aberrantes.c` vs `valeurs_` | Rename when fixing #2 |

Notes referenced by source comments but not present: `documentation/classes_comptabilite.md`, `bilan_exp.md`, `detection-engine.md`, `erreurs.md`, `Benford_strategie.ipynb`, `API_detection.ipynb`.
