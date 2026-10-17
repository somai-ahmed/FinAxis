# Fondations comptables pour programmeurs

Tout ce qu'il faut pour lire le code C de FinAxis sans être comptable. Chaque concept est relié à la structure ou à la fonction qui l'implémente.

**Ordre de lecture :** ce fichier → [02_bilan.fr.md](02_bilan.fr.md) → [03_grand_livre.fr.md](03_grand_livre.fr.md)

---

## 1. L'idée centrale : la partie double

Chaque événement est enregistré **deux fois** : une fois en *débit* sur un compte, une fois en *crédit* sur un autre, pour le **même montant**.

Voyez cela comme un **checksum** : si la somme des débits ≠ la somme des crédits, les données sont corrompues.

| Vue programmeur | Terme comptable |
|---|---|
| Transaction (atomique, tout ou rien) | *Écriture* |
| Ligne d'une transaction | *Ligne* (`ligne_journal`) |
| Invariant : `Σ débit == Σ crédit` | Écriture équilibrée (`ecritures_est_equilibree`) |
| Journal d'événements en ajout seul | *Journal* |
| Vue matérialisée par compte | *Grand livre* |
| Instantané de tous les soldes | *Balance* |
| Synthèse finale à une date | *Bilan* |

**Débit et crédit ne sont pas « plus » et « moins ».** Ce sont simplement *gauche* et *droite*. Qu'un débit augmente ou diminue un compte dépend du type du compte (section 3).

---

## 2. L'écriture comptable

```
Écriture  ref=ACH-001  date=2026-01-10  "Achat de marchandises à crédit"
  ligne 1   compte 31 (Stocks)         débit  20 000,000   crédit      0
  ligne 2   compte 40 (Fournisseurs)   débit       0       crédit 20 000,000
                                       ─────────────────────────────────────
                                       Σ débit = 20 000    Σ crédit = 20 000   ✔
```

En C (`types.h`) :

```c
typedef struct ligne_journal { id_ligne id; id_compte compte_id; Monnaie Debit; Monnaie credit; char libelle[256]; } ligne_journal;
typedef struct Ecriture {
    IdEcriture id; idperiodefiscale periode_id; DATE date;
    char reference[32]; char description[256];
    ligne_journal *lignes; size_t nombre_lignes;
    int est_validee;                       /* 0 = brouillon, 1 = comptabilisée */
} Ecriture;
```

**Cycle de vie d'une écriture**

```
 brouillon (est_validee = 0) ──ecritures_comptabiliser()──► comptabilisée (est_validee = 1)
                                  │
                                  ├─ 1. ecritures_valider() : lignes valides + Σ débit == Σ crédit
                                  ├─ 2. tous les comptes doivent exister (vérifié AVANT de toucher un solde)
                                  └─ 3. application de comptes_debiter / comptes_crediter sur chaque ligne
```

- Dans ce moteur, **« validée » et « comptabilisée » sont le même drapeau** : `est_validee`. `ecritures_est_comptabilisee()` ne fait que le lire.
- `ecritures_valider()` seule est un simple **contrôle** : elle ne modifie pas l'écriture.
- Deux passes (tout vérifier, puis appliquer) rendent la comptabilisation **tout ou rien**, comme une transaction de base de données.
- Une écriture comptabilisée ne peut plus être modifiée (`ecritures_ajouter_ligne` renvoie `ERR_JOURNAL_DEJA_COMPTABILISE`) ni comptabilisée deux fois.
- Seules les écritures **comptabilisées** apparaissent dans les rapports (`ecriture_retenue()` dans `balance.c` / `grand_livre.c`).
- Une ligne a un débit **ou** un crédit, jamais les deux, jamais zéro des deux (`ecritures_ligne_valide`). Les montants négatifs sont refusés.

---

## 3. Comptes, classes et « solde normal »

Le plan comptable est un arbre. Le premier chiffre est la **classe** :

| Classe | `ClasseCompte` | Contenu | Va dans | Solde normal |
|---|---|---|---|---|
| 1 | `CLASSE_1_CP_ET_PNC` | Capitaux propres et passifs non courants (capital, réserves, emprunts) | Bilan – Passif | Crédit |
| 2 | `CLASSE_2_ACTIFS_NON_COURANTS` | Immobilisations (bâtiments, machines) | Bilan – Actif | Débit |
| 3 | `CLASSE_3_STOCKS` | Stocks | Bilan – Actif | Débit |
| 4 | `CLASSE_4_TIERS` | Tiers (clients, fournisseurs, État, personnel) | Actif **ou** Passif | Selon le signe |
| 5 | `CLASSE_5_TRESORERIE` | Banque et caisse | Actif (ou Passif si à découvert) | Débit |
| 6 | `CLASSE_6_CHARGES` | Charges | Compte de résultat | Débit |
| 7 | `CLASSE_7_PRODUITS` | Produits | Compte de résultat | Crédit |

Les classes 1 à 5 sont des **comptes de bilan** (permanents). Les classes 6 et 7 sont des **comptes de gestion** (remis à zéro chaque exercice).

### Solde normal (`SoldeNormal`)

| | `SOLDE_DEBITEUR` | `SOLDE_CREDITEUR` |
|---|---|---|
| Quels comptes | Actifs, charges | Passifs, capitaux propres, produits |
| Un **débit** | augmente | diminue |
| Un **crédit** | diminue | augmente |

C'est exactement `comptes_debiter` / `comptes_crediter` dans `comptes.c` :

```c
Etat comptes_debiter(Compte *c, Monnaie m) {
    if (c->solde_normal == SOLDE_DEBITEUR) c->solde += m;   /* débit sur compte débiteur → augmente */
    else                                   c->solde -= m;   /* débit sur compte créditeur → diminue */
    ...
}
```

`Compte.solde` est donc toujours stocké **dans le sens propre du compte**. Un `solde` négatif signifie que le compte est « à l'envers » (ex. un client qui a trop payé).

### Aide-mémoire

```
                DÉBIT                          CRÉDIT
Actif        ↑ (vous recevez quelque chose)  ↓
Charge       ↑ (vous dépensez)               ↓
Passif       ↓ (vous remboursez une dette)   ↑ (vous devez plus)
Cap. propres ↓                               ↑
Produit      ↓                               ↑ (vous gagnez)
```

---

## 4. L'argent : pourquoi `int64_t` en millimes

```c
typedef int64_t Monnaie;
#define Monnaie_Unite 1000        /* 1 DT = 1000 millimes (le dinar tunisien a 3 décimales) */
```

- `0.1 + 0.2 != 0.3` en `double`. En comptabilité, les livres **doivent** s'équilibrer au millime près : les montants sont donc des **entiers**.
- `15 000,500 DT` est stocké `15000500`.
- Ne jamais convertir en `double` pour calculer ; seulement pour l'affichage (`monnaie.c`).

---

## 5. L'équation fondamentale

```
ACTIFS  =  CAPITAUX PROPRES  +  DETTES
(ce que vous possédez)   (qui a financé : propriétaires + créanciers)
```

Chaque écriture en partie double préserve cette équation. C'est *pourquoi* le bilan s'équilibre toujours si le journal est équilibré.

> Dans la présentation tunisienne, le **PASSIF** *contient déjà* les capitaux propres. Le contrôle est donc `TOTAL ACTIF == TOTAL PASSIF` (c'est `bilan_verifier`). Écrire `ACTIF = PASSIF + CAPITAUX PROPRES` compterait les capitaux propres deux fois.

---

## 6. Les périodes fiscales

Une *période fiscale* (`prop_periode_fiscale`) est une plage de dates avec un état :

| État | `status_periode_fiscale` | Comptabilisation possible ? |
|---|---|---|
| Ouverte | `PERIODE_FISCALE_OUVERTE` | Oui |
| Clôturée | `PERIODE_FISCALE_CLOTURE` | Non (peut être rouverte) |
| Verrouillée | `PERIODE_FISCALE_VEROUILLEE` | Non (définitif) |

La date d'une écriture doit tomber dans sa période (`periodes_verifier_ecriture`).

---

## 7. La chaîne de rapports

```
 Journal (écritures comptabilisées)
        │
        ├──► Grand livre   un compte, tous ses mouvements, solde cumulé
        │
        └──► Balance       une ligne par compte : Σ débit, Σ crédit, solde
                │
                └──► Bilan  comptes regroupés par classe → Actif / Passif   (+ compte de résultat)
```

| Rapport | Fonction | Entrée | Sens de la période |
|---|---|---|---|
| Grand livre | `grand_livre_generer` | écritures | Plage `[debut, fin]` + solde initial avant `debut` |
| Balance | `balance_generer` | comptes + écritures | Plage `[debut, fin]` |
| Bilan | `bilan_generer` | comptes + **balance** | Instantané à une date : balance depuis l'origine |
| Compte de résultat | `compte_resultat_generer` | comptes + totaux | Plage (flux) |

Règle simple : **bilan = instantané (état)**, **compte de résultat = flux (variation)**.

---

## 8. Auto-contrôles du moteur

| Contrôle | Fonction | Ce qu'il prouve |
|---|---|---|
| Écriture équilibrée | `ecritures_est_equilibree` | `Σ débit == Σ crédit` pour une écriture |
| Balance équilibrée | `balance_verifier` | Σ débits == Σ crédits ; chaque ligne d'un seul côté |
| Grand livre vs balance | `grand_livre_verifier` | Les totaux du grand livre correspondent à la ligne de balance |
| Bilan équilibré | `bilan_verifier` | `TOTAL ACTIF == TOTAL PASSIF` |

Si l'un échoue, le code d'erreur indique la couche fautive (voir `errors.h`). Déboguez de haut en bas : bilan → balance → grand livre → écriture.

---

## 9. Glossaire (FR → EN)

| Français | English | Dans le code |
|---|---|---|
| Écriture | Journal entry | `Ecriture` |
| Compte | Account | `Compte` |
| Plan comptable | Chart of accounts | `bilan_tunisien.json` |
| Débit / Crédit | Debit / Credit | `Debit`, `credit` |
| Solde | Balance (d'un compte) | `solde`, `solde_debit`, `solde_credit` |
| Balance | Trial balance | `LigneBalance` |
| Grand livre | General ledger | `Entree_GrandLivre` |
| Bilan | Balance sheet | `ligne_bilan` |
| Compte de résultat | Income statement | `CompteResultat` |
| Actif / Passif | Assets / Liabilities + equity | |
| Capitaux propres | Equity | classe 1 |
| Charges / Produits | Expenses / Revenue | classes 6 / 7 |
| Tiers | Third parties | classe 4 |
| Trésorerie | Cash & bank | classe 5 |
| Comptabiliser | To post | `Session_comptabiliserEcriture` |
| Clôturer | To close (a period) | `periodes_cloturer` |
| Exercice | Fiscal year | |
