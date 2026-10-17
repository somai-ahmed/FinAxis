# `.FinAxis` File Format (version 1)

A UTF-8 JSON file written by `Session_Sauvegarder` and read by `Session_Charger` (cJSON).

- All **amounts are integers in millimes** (1 DT = 1000).
- Dates are ISO strings `YYYY-MM-DD`.
- Account balances are **not stored**: they are recomputed on load by replaying the validated entries.

## Top-level structure

```json
{
  "format": "FinAxis",
  "version": 1,
  "config":   { "nom_entreprise": "...", "devise": "TND" },
  "comptes":  [ ... ],
  "periodes": [ ... ],
  "ecritures": [ ... ]
}
```

## `comptes[]`

| Key | Type | Notes |
|---|---|---|
| `id` | int | Unique, > 0 |
| `code` | string | Account code, e.g. `"101"` |
| `nom` | string | Label |
| `classe` | int | 1–7 |
| `type` | int | 1 actif, 2 passif, 3 capitaux, 4 produit, 5 charge |
| `solde_normal` | int | 0 debit, 1 credit |
| `parent_id` | int | 0 = root |
| `est_active` | int | 0 / 1 |

## `periodes[]`

| Key | Type | Notes |
|---|---|---|
| `id` | int | |
| `nom` | string | e.g. `"2026-01"` |
| `date_debut`, `date_fin` | string | ISO dates |
| `statut` | int | 1 open, 2 closed, 3 locked |

## `ecritures[]`

| Key | Type | Notes |
|---|---|---|
| `id` | int | |
| `periode_id` | int | Must reference a period |
| `date` | string | ISO |
| `reference` | string | ≤ 31 chars |
| `description` | string | ≤ 255 chars |
| `est_validee` | int | 0 draft, 1 validated |
| `lignes` | array | Lines, see below |

## `ecritures[].lignes[]`

| Key | Type | Notes |
|---|---|---|
| `id` | int | Line id |
| `compte_id` | int | References `comptes[].id` |
| `debit` | int | Millimes |
| `credit` | int | Millimes |
| `libelle` | string | ≤ 255 chars |

Total debit must equal total credit for an entry to be validated.

## Seed data: `bilan_tunisien.json`

Not a session file. A list of Tunisian accounts (`code`, `label`, `class`) extracted from the official chart-of-accounts text. Its own `note` field says it must be checked before production use as an official chart.
