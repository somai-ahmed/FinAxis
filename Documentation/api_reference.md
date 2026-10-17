# C API Reference

Single include: `#include "FinAxis_AE.h"` (pulls in every public header).

## Conventions

- Return type `Etat`: `ETAT_OK == 0`, errors are negative. Message: `GetErrorMessage(etat)`.
- Money: `Monnaie` = `int64_t` in **millimes** (`Monnaie_Unite = 1000`, i.e. 3 decimals).
- Dates: `DATE { int32_t annee, mois, jour; }`; invalid date = `DATE_ERREUR`.
- Ids: `id_compte`, `IdEcriture`, `idperiodefiscale`, `id_ligne` are `uint32_t`; `INVALID_ID == 0`.
- `Session` is opaque. Strings are fixed-size char arrays inside structs.
- Functions returning arrays follow the *caller-allocates* pattern (`buffer, capacite, *nombre`), except detection (library-allocated, you free).

## Error codes (`errors.h`)

| Range | Module |
|---|---|
| `0` | `ETAT_OK` |
| `-1` … `-99` | General (`ERR_ARGUMENT_INVALIDE`, `ERR_POINTEUR_NULLE`, `ERR_SORTIE_DU_MEMOIRE`, `ERR_SESSION_INVALIDE`, …) |
| `-100` … `-199` | Accounting (`ERR_COMPTE_INTROUVABLE`, `ERR_COMPTE_EXISTE`, `ERR_CODE_COMPTE_INVALIDE`, …) |
| further blocks | Entries, periods, reports, detection, persistence — see `errors.h` |

## Core types (`types.h`)

| Type | Fields / values |
|---|---|
| `ClasseCompte` | `CLASSE_1_CP_ET_PNC` … `CLASSE_7_PRODUITS` (1–7) |
| `TypeCompte` | `COMPTE_ACTIF`, `PASSIF`, `CAPITAUX`, `PRODUIT`, `CHARGE` |
| `SoldeNormal` | `SOLDE_DEBITEUR` (0), `SOLDE_CREDITEUR` (1) |
| `Compte` | `id, code[16], nom[128], classe, type, solde_normal, solde, parent_id, est_active` |
| `ligne_journal` | `id, compte_id, Debit, credit, libelle[256]` |
| `Ecriture` | `id, periode_id, date, reference[32], description[256], lignes*, nombre_lignes, est_validee` |
| `prop_periode_fiscale` | `id, nom[64], date_debut, date_fin, statut` (`OUVERTE`=1, `CLOTURE`=2, `VEROUILLEE`=3) |
| `LigneBalance` | `compte (ref_compte), total_debit, total_credit, solde_debit, solde_credit` |
| `Entree_GrandLivre` | `date, reference, libelle, debit, credit, solde_cumule` |
| `ligne_bilan` | `libelle[128], montant, est_sous_total, profondeur` |

## Session

| Function | Purpose |
|---|---|
| `creer_session(config, &session)` / `detruire_session` / `reinitialiser_session` | Lifecycle |
| `Session_avoir_Config` / `Session_SetConfig` | Company name, currency |
| `Session_ajouterCompte`, `Session_avoir_nombre_comptes`, `Session_avoir_CompteAt` | Accounts |
| `Session_chercher_Compte_Par_ID` / `_Par_Code` | Account lookup |
| `Session_AddEcriture`, `Session_GetEcritureCount`, `Session_avoir_EcritureAt`, `Session_chercher_Ecriture_Par_ID` | Entries |
| `Session_comptabiliserEcriture(session, id)` | Post an entry (updates account balances) |
| `Session_ajouterPeriode`, `Session_avoir_PeriodeAt`, `Session_chercher_Periode_Par_ID` | Periods |
| `Session_cloturerPeriode` / `rouvrirPeriode` / `verrouillerPeriode` | Period state changes |
| `Session_nouvel_id_compte` / `Session_nouvel_id_ecriture` | Id generators |
| `Session_avoir_le_dernier_Erreur` | Last error of the session |

## Accounts (`comptes.h`)

`comptes_creer`, `comptes_valider_code`, `comptes_valider`, `comptes_debiter`, `comptes_crediter`, `comptes_avoir_solde`, `comptes_a_parent`, `comptes_est_enfant_de`, `comptes_activer`, `comptes_desactiver`.

## Journal entries (`ecritures.h`)

```c
Ecriture e;
ecritures_creer(&e, periode_id, date, "ACH-001", "Achat marchandises");
ecritures_ajouter_ligne(&e, id_achats, 500000, 0, "Achats");
ecritures_ajouter_ligne(&e, id_fournisseur, 0, 500000, "Fournisseur");
if (ecritures_est_equilibree(&e) && ecritures_valider(&e) == ETAT_OK)
    Session_AddEcriture(session, &e);
ecritures_detruire(&e);
```

Also: `ecritures_nombre_lignes`, `ecritures_avoir_ligne`, `ecritures_ligne_valide`, `ecritures_total_debit/credit`, `ecritures_comptabiliser`, `ecritures_est_comptabilisee`.

## Periods (`periodes.h`)

`periodes_creer`, `periodes_generer_mensuelles(annee, premier_id, sortie, capacite)`, `periodes_plage_valide`, `periodes_est_valide/ouverte/cloturee/verrouillee`, `periodes_contient_date`, `periodes_se_chevauchent`, `periodes_duree_jours`, `periodes_cloturer/rouvrir/verrouiller`, `periodes_ajouter`, `periodes_trouver_par_id/par_date`, `periodes_verifier_ecriture`.

## Reports (`rapports.h`)

| Function | Output |
|---|---|
| `Session_GenererBalance(session, debut, fin, lignes, capacite, &n)` | `LigneBalance[]` |
| `Session_GenererGrandLivre(session, compte_id, debut, fin, entrees, capacite, &n)` | `Entree_GrandLivre[]` |
| `Session_GenererBilan(session, date_arret, lignes, capacite, &n, &total_actif, &total_passif)` | `ligne_bilan[]` |
| `Session_GenererCompteResultat(session, debut, fin, &cr)` | `CompteResultat` |
| `balance_est_equilibree`, `balance_verifier`, `bilan_verifier`, `grand_livre_verifier` | Consistency checks |

## Detection (`detection.h`) — see [detection_engine.md](detection_engine.md)

```c
config_detection cfg;
detection_config_par_defaut(&cfg);

Resultat_Detection *res = NULL; size_t n = 0;
Etat st = execute_detection(session, periode_id, &cfg, &res, &n);
/* ... use res[0..n-1] ... */
liberer_resultats_detection(res, n);

Rapport_Benford rb;
Executer_Analyse_Benford(session, periode_id, &rb);
```

## Persistence (`session_io.h`)

```c
Session_Sauvegarder(session, "societe.FinAxis");
Session *s = NULL;
Session_Charger("societe.FinAxis", &s);     /* free with detruire_session */
```

## Version (`FinAxis_API.h`)

`finAxis_Version()` → `"1.0.0"`; `finAxis_TailleStruct("Compte")` → `sizeof` for FFI sanity checks (`0` if unknown).********
