# Detection Engine

Runs on **validated** journal entries of one fiscal period. Entry point: `execute_detection(session, periode_id, &cfg, &results, &n)`.

## Configuration (`config_detection`)

| Field | Default | Meaning |
|---|---|---|
| `benford_est_active` | 1 | Enable Benford analysis |
| `doublons_est_active` | 1 | Enable duplicate detection |
| `nombres_ronds_active` | 1 | Enable round-amount detection |
| `valeurs_aberrantes_active` | 1 | Enable outlier detection |
| `seuil_ecart_type_extreme` | 3.0 | Outlier z-score threshold (σ) |
| `seuil_ecart_nombres_ronds` | 1000.0 | An amount is "round" if divisible by this (DT) |
| `fenetre_jours_doublon` | 7 | Days window to look for duplicates |

Fill with defaults: `detection_config_par_defaut(&cfg)`.

## Result (`Resultat_Detection`)

| Field | Meaning |
|---|---|
| `methode` | `DETECT_BENFORD`, `DETECT_DOUBLONS`, `DETECT_NOMBRES_RONDS`, `DETECT_VALS_ABERRANTES` |
| `gravite` | `INFO`(0) < `FAIBLE`(1) < `MOYENNE`(2) < `ELEVEE`(3) < `CRITIQUE`(4) |
| `id_ligne`, `id_compte` | Where the anomaly is |
| `description` | Human-readable text (256 chars) |
| `score` | Method-specific number (see below) |

The result array is allocated by the engine: free it with `liberer_resultats_detection(res, n)`.

## Methods

### 1. Benford's Law (`benford.c`)

Compares the observed first-digit distribution with the expected one (`log10(1 + 1/d)`), using a chi-square test.

- Threshold: `BENFORD_SEUIL_CHI_CARRE = 15.507` (8 degrees of freedom, 95 % confidence).
- Severity: `MOYENNE`; `ELEVEE` when chi-square > 2 × threshold.
- Score = chi-square value.
- Full report: `Executer_Analyse_Benford(session, periode_id, &rapport)` → `Rapport_Benford`:
  per-digit `StatChiffreBenford` (expected / observed frequency, count), chi-square, mean absolute deviation, sample size, `estnormal` flag. This feeds the Benford chart window in the GUI.

### 2. Duplicates (`doublons.c`)

Compares each entry with others inside ±`fenetre_jours_doublon` days.

- Severity: `ELEVEE` if same day, otherwise `MOYENNE`.
- Score = `1 − ecart_jours / fenetre_jours` (0–1, closer dates = more suspicious).

### 3. Round amounts (`montants_ronds.c`)

Flags lines whose amount is an exact multiple of `seuil_ecart_nombres_ronds`. A round amount is a signal, not proof, so the base case is `FAIBLE`; the cleaner the multiple (×10, ×100), the higher the level.

- Severity: `FAIBLE` → `MOYENNE` → `ELEVEE`.
- Score: 0.5 to 1.0.

### 4. Outliers (`valeur_aberrantes.c`)

Z-score of an amount against the other entries of the same account.

- Flagged when z ≥ `seuil_ecart_type_extreme`.
- Severity: `ELEVEE`, or `CRITIQUE` when z ≥ 1.5 × threshold.
- Score = `min(1, z / (2 × threshold))`.

## GUI

`Analyse` menu → **Detection** window (configurable via the detection config dialog) and **Benford chart** window. Results can be exported to Excel with `Excel/templates/detection_template.py`.
