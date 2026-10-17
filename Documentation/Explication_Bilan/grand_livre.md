# Le Grand Livre expliqué aux programmeurs

Sources : `Src/FinCore.Native/Src/Rapports/grand_livre.c`, `include/rapports.h`, `services/report_service.py`.
Prérequis : [fondations_comptables](fondations_comptables.md). Lié : [bilan](bilan.md).

---

## 1. Ce que c'est

Le **journal** est organisé par *événement* (une écriture = un événement, plusieurs comptes).
Le **grand livre** contient les mêmes données organisées par *compte* (un compte = tous ses mouvements, avec un solde cumulé).

```
 JOURNAL (par écriture)                    GRAND LIVRE (par compte) — compte 53 Banques
 ACH-001 ─ 31 D 20 000                     date        réf       débit    crédit   solde cumulé
           40 C 20 000                     01/01  CAP-001   100 000                 100 000
 CAP-001 ─ 53 D 100 000   ─────────────►   05/01  IMM-001               30 000      70 000
           10 C 100 000                    25/01  LOY-001                2 000      68 000
 IMM-001 ─ 22 D 30 000                     28/01  ENC-001     5 000                 73 000
           53 C 30 000
 ...
```

Vue programmeur : le journal est un **log d'événements** ; le grand livre est une **vue matérialisée** `GROUP BY compte ORDER BY date` avec une **somme glissante** (`SUM() OVER (ORDER BY date)`).

```sql
-- équivalent SQL
SELECT date, ref, libelle, debit, credit,
       ouverture + SUM(CASE WHEN normal = 'CREDIT' THEN credit - debit ELSE debit - credit END)
                   OVER (ORDER BY date, ref) AS solde_cumule
FROM   lignes JOIN ecritures USING (ecriture_id)
WHERE  compte_id = :id AND comptabilisee AND date BETWEEN :debut AND :fin;
```

---

## 2. Structure de données

```c
typedef struct Entree_GrandLivre {
    DATE    date;
    char    reference[32];     /* référence de l'écriture */
    char    libelle[256];      /* libellé de la ligne, ou description de l'écriture si la ligne n'en a pas */
    Monnaie debit;
    Monnaie credit;
    Monnaie solde_cumule;      /* solde APRÈS ce mouvement, dans le sens propre du compte */
} Entree_GrandLivre;
```

**Une `Entree_GrandLivre` par ligne d'écriture touchant le compte.** Une écriture avec deux lignes sur le même compte donne deux lignes.

---

## 3. La formule centrale

```c
static Monnaie effet_sur_solde(const Compte *c, Monnaie debit, Monnaie credit) {
    if (c->solde_normal == SOLDE_CREDITEUR) return credit - debit;   /* passifs, capitaux propres, produits */
    return debit - credit;                                           /* actifs, charges */
}
```

`solde_cumule` est exprimé **dans le sens normal du compte** :

| Compte | Normal | Solde cumulé positif signifie | Négatif signifie |
|---|---|---|---|
| 53 Banques | Débit | de l'argent en banque | découvert |
| 41 Clients | Débit | les clients nous doivent | clients ayant trop payé |
| 40 Fournisseurs | Crédit | nous devons aux fournisseurs | nous avons trop payé |
| 70 Ventes | Crédit | produits cumulés | retours supérieurs aux ventes |

---

## 4. L'algorithme (`grand_livre_generer`)

Signature :

```c
Etat grand_livre_generer(id_compte compte_id,
                         const Compte *comptes, size_t nb_comptes,
                         const Ecriture *ecritures, size_t nb_ecritures,
                         DATE debut, DATE fin,
                         Entree_GrandLivre *sortie, size_t capacite, size_t *nb_sortie);
```

| Étape | Ce qui se passe | Erreur renvoyée |
|---|---|---|
| 0 | Valider les arguments : non NULL, `periodes_plage_valide(debut, fin)`, `compte_id != 0` | `ERR_POINTEUR_NULLE`, `ERR_PLAGE_DATES_INVALIDE`, `ERR_IDENTIFIANT_INVALIDE` |
| 1 | Trouver le compte dans `comptes[]` | `ERR_COMPTE_INTROUVABLE` |
| 2 | **Passe 1 – compter** les mouvements du compte dans les écritures comptabilisées de `[debut, fin]`. Écrire le nombre dans `*nb_sortie` | |
| 3 | Si `capacite < nombre` → renvoyer la taille nécessaire | `ERR_TRES_PETIT_BUFFER` |
| 4 | Si `nombre == 0` | `ERR_AUCUNE_DONNEE_PERIODE` |
| 5 | **Passe 2 – remplir** : une ligne par ligne d'écriture concernée. Libellé = libellé de la ligne, sinon description de l'écriture | |
| 6 | **Trier** avec `qsort` : par date, puis `strcmp(reference)` | |
| 7 | `cumul = grand_livre_solde_initial(...)`, puis pour chaque ligne `cumul += effet_sur_solde(...)` ; stocker dans `solde_cumule` | |

Écritures retenues (`ecriture_retenue`) : **comptabilisées** (`est_validee != 0`) **et** `debut <= date <= fin`.

### Le schéma deux passes / « demande de taille »

Comme l'API C n'alloue jamais de mémoire pour l'appelant, celui-ci appelle deux fois :

```c
size_t n = 0;
Etat st = grand_livre_generer(id, comptes, nc, ecr, ne, d1, d2, NULL, 0, &n);   /* 1er appel : capacite = 0 */
/* st == ERR_TRES_PETIT_BUFFER, n = nombre de lignes nécessaires (ERR_AUCUNE_DONNEE_PERIODE si n == 0) */
Entree_GrandLivre *lignes = malloc(n * sizeof *lignes);
st = grand_livre_generer(id, comptes, nc, ecr, ne, d1, d2, lignes, n, &n);      /* 2e appel : remplit */
```

`services/report_service.py` fait exactement cela avec `ctypes`, et renvoie `[]` s'il n'y a aucun mouvement.

### Solde initial (*report à nouveau*)

```c
Monnaie grand_livre_solde_initial(const Compte *compte, const Ecriture *ecr, size_t n, DATE avant_date);
```

= la somme de `effet_sur_solde` sur toutes les lignes comptabilisées du compte datées **strictement avant** `avant_date`. C'est ce qui fait qu'un grand livre du 21 au 31 janvier démarre au bon nombre au lieu de 0.

Exemple, compte 53, plage `21/01 → 31/01` :

```
solde initial (avant 21/01) = 100 000 − 30 000 = 70 000
25/01  LOY-001  C  2 000   →  68 000
28/01  ENC-001  D  5 000   →  73 000
```

> Le solde initial **n'est pas** une ligne du résultat. Le premier `solde_cumule` l'inclut déjà. Une interface qui veut une ligne « solde initial » doit appeler elle-même `grand_livre_solde_initial`.

---

## 5. Exemple complet

Données : les six écritures de [bilan](bilan.md). Grand livre du compte **41 Clients** (compte débiteur), 01/01 → 31/01 :

| Date | Réf | Libellé | Débit | Crédit | Solde cumulé |
|---|---|---|---|---|---|
| 20/01 | VTE-001 | Vente à crédit | 15 000 | | 15 000 |
| 28/01 | ENC-001 | Encaissement client | | 5 000 | 10 000 |

Grand livre du compte **40 Fournisseurs** (compte créditeur) :

| Date | Réf | Libellé | Débit | Crédit | Solde cumulé |
|---|---|---|---|---|---|
| 10/01 | ACH-001 | Achat à crédit | | 20 000 | 20 000 |

Le dernier solde cumulé d'un compte égale le chiffre de la balance/du bilan pour ce compte (`10 000` pour le 41 → « Tiers débiteurs »).

---

## 6. Vérifier : `grand_livre_verifier`

```c
Etat grand_livre_verifier(const Entree_GrandLivre *entrees, size_t n, const LigneBalance *ligne);
```

`ligne` = la ligne de ce compte dans une balance générée sur **la même plage de dates**.

| Contrôle | Erreur |
|---|---|
| Un `debit < 0` ou `credit < 0` | `ERR_MOUVEMENT_COMPTABLE_INVALIDE` |
| `Σ debit != ligne->total_debit` ou `Σ credit != ligne->total_credit` | `ERR_GRAND_LIVRE_BALANCE_INCOHERENTS` |

C'est un **recoupement entre deux chemins de code indépendants** (grand livre vs balance) : mêmes données, deux calculs, ils doivent concorder. Il détecte les bugs de filtrage, de plage de dates ou de tri.

---

## 7. Grand livre ↔ balance ↔ bilan

```
 Grand livre du compte X ── Σ débit, Σ crédit ──►  Ligne de balance de X  ── solde (par classe) ──►  Bilan
        dernier solde_cumule  ==  solde_debit ou solde_credit de la ligne de balance (si debut = début des livres)
```

| Si `debut` est… | Alors le dernier `solde_cumule` égale… |
|---|---|
| le début des livres (ex. `1900-01-01`) | le solde du compte dans la balance et le bilan |
| une date plus tardive | la balance **plus le solde initial** (le grand livre part de `solde_initial`) |

---

## 8. Pièges

| Piège | Explication |
|---|---|
| Brouillons absents | Seules les écritures comptabilisées sont listées. Un brouillon est invisible dans tous les rapports. |
| Ordre le même jour | Tri par date puis `reference` (comparaison de chaînes). `qsort` n'est pas stable : deux lignes de même date **et** même référence peuvent s'échanger ; le solde final n'est pas affecté mais les soldes intermédiaires peuvent différer. |
| Lire `solde_cumule` comme « débit moins crédit » | Il est dans le **sens propre du compte** (comptes créditeurs : crédit − débit). |
| Débit/crédit inversés sur comptes créditeurs | Affichez `debit` et `credit` tels que stockés ; seul le solde cumulé change de convention de signe. |
| Gros volumes | Chaque appel parcourt deux fois toutes les écritures (O(écritures × lignes)). Correct pour un poste de bureau ; un index par compte serait l'optimisation suivante. |
| Tous les comptes d'un coup | Il n'y a pas de fonction « tous les comptes » : appelez `grand_livre_generer` compte par compte (la fenêtre de l'interface le fait pour le compte choisi). |

## 9. Python, de bout en bout

```python
lignes = report_service.grand_livre(id_compte=compte.id, debut=date(2026, 1, 1), fin=date(2026, 1, 31))
for r in lignes:                     # LedgerRow(date, reference, libelle, debit, credit, solde_cumule)
    print(r.date, r.reference, r.debit / 1000, r.credit / 1000, r.solde_cumule / 1000)
```

Le tableau de l'interface (`grand_livre_window.py`) affiche : Date · Référence · Libellé · Débit · Crédit · Solde cumulé.
Une implémentation Python pure du même algorithme est dans [reference_pipeline.py](reference_pipeline.py) (`grand_livre()`), utile pour tester le moteur C contre un oracle indépendant.
