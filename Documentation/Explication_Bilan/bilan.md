# Le Bilan expliqué aux programmeurs

Sources : `Src/FinCore.Native/Src/Rapports/bilan.c`, `include/rapports.h`.
Prérequis : [01_fondations_comptables.fr.md](01_fondations_comptables.fr.md).
Version Python exécutable de tout ce qui suit : [reference_pipeline.py](reference_pipeline.py).

---

## 1. Ce que c'est

Le bilan est un **instantané à une date** : *ce que l'entreprise possède* (ACTIF) face à *la façon dont c'est financé* (PASSIF).

```
         ACTIF                              PASSIF
  ce que l'entreprise possède        qui a financé
 ┌──────────────────────┐          ┌──────────────────────────┐
 │ Actifs non courants  │          │ Capitaux propres         │
 │ Actifs courants      │          │ (+ passifs non courants) │
 │                      │          │ Passifs courants         │
 ├──────────────────────┤          ├──────────────────────────┤
 │ TOTAL ACTIF          │    ==    │ TOTAL PASSIF             │
 └──────────────────────┘          └──────────────────────────┘
```

`TOTAL ACTIF == TOTAL PASSIF` toujours, si le journal est équilibré. `bilan_verifier(total_actif, total_passif)` le contrôle.

> L'ancienne note française de ce dossier écrit `ACTIF = PASSIF + CAPITAUX PROPRES`. Dans la présentation tunisienne, **le PASSIF contient déjà les capitaux propres** : le contrôle est simplement `ACTIF == PASSIF`.

**Instantané, pas flux.** Le bilan n'est *pas* « janvier seulement ». Il lui faut des soldes **cumulés depuis l'origine**. C'est pourquoi `Session_GenererBilan` prend une seule `date_arret` et que la balance sous-jacente est calculée d'une date de début très ancienne jusqu'à `date_arret`.

---

## 2. Structure de données : `ligne_bilan` et `profondeur`

```c
typedef struct ligne_bilan {
    char    libelle[128];
    Monnaie montant;
    int     est_sous_total;   /* 1 = ligne de total (NE PAS l'additionner à ses enfants) */
    int     profondeur;       /* niveau d'indentation dans l'arbre */
} ligne_bilan;
```

Le bilan est un **arbre aplati en tableau**. `profondeur` est le niveau (0 = racine). Une interface indente avec ; un export s'en sert pour le style.

`bilan_generer` produit toujours exactement **15 lignes** (`NB_LIGNES_BILAN`) :

| # | `libelle` | `profondeur` | `est_sous_total` | Valeur |
|---|---|---|---|---|
| 1 | ACTIF | 0 | 0 | 0 (titre) |
| 2 | Actifs non courants | 1 | 0 | `net(classe 2)` |
| 3 | Actifs courants | 1 | **1** | `stocks + tiers_debiteurs + tresorerie` |
| 4 | Stocks | 2 | 0 | `net(classe 3)` |
| 5 | Tiers debiteurs | 2 | 0 | Σ `solde_debit` de la classe 4 |
| 6 | Tresorerie | 2 | 0 | Σ `solde_debit` de la classe 5 |
| 7 | **TOTAL ACTIF** | 0 | **1** | `actifs_non_courants + actifs_courants` |
| 8 | PASSIF | 0 | 0 | 0 (titre) |
| 9 | Capitaux propres et passifs non courants | 1 | **1** | `capitaux_classe_1 + resultat` |
| 10 | Capital, reserves et dettes non courantes | 2 | 0 | `-net(classe 1)` |
| 11 | Resultat de l exercice | 2 | 0 | `produits (7) - charges (6)` |
| 12 | Passifs courants | 1 | **1** | `tiers_crediteurs + concours_bancaires` |
| 13 | Tiers crediteurs | 2 | 0 | Σ `solde_credit` de la classe 4 |
| 14 | Concours bancaires | 2 | 0 | Σ `solde_credit` de la classe 5 |
| 15 | **TOTAL PASSIF** | 0 | **1** | `fonds_propres + passifs_courants` |

Les libellés n'ont volontairement pas d'accents (évite les problèmes d'encodage entre le compilateur C et l'interface).

**Règle pour les consommateurs :** pour recalculer un total, additionnez seulement les lignes `est_sous_total == 0` *et* `profondeur == 2` (feuilles), plus la ligne « Actifs non courants ». Ne jamais additionner un sous-total avec ses enfants (double comptage).

---

## 3. L'algorithme, pas à pas

Entrées : le plan comptable (`Compte[]`) et la **balance** (`LigneBalance[]`). Le bilan ne lit jamais directement les écritures.

### Étape 0 — Gardes

```c
if (capacite < 15)      return ERR_TRES_PETIT_BUFFER;          /* *nb_sortie vaut quand même 15 */
if (nb_balance == 0)    return ERR_DONNEES_BILAN_INSUFFISANTES;
/* chaque ligne de balance doit pointer vers un compte connu de classe 1..7 */
```

Appeler avec `capacite = 0` est une façon légale de demander « quelle taille de tampon me faut-il ? » (schéma courant avec `ctypes`).

### Étape 1 — Deux sommes utilitaires

```c
bilan_somme_soldes(comptes, balance, classe, sens)
   // Σ solde_debit   (sens = SOLDE_DEBITEUR)   ou   Σ solde_credit (sens = SOLDE_CREDITEUR)
   // sur toutes les lignes de balance dont le compte appartient à `classe`

net_classe(classe) = Σ solde_debit(classe) − Σ solde_credit(classe)
   // > 0 : la classe est globalement débitrice, < 0 : globalement créditrice
```

Une ligne de balance n'a **pas de classe** ; on la retrouve en cherchant le compte par son id (`trouver_indice_compte`).

### Étape 2 — ACTIF (ce que l'entreprise possède)

| Poste | Formule | Pourquoi |
|---|---|---|
| Actifs non courants | `net(2)` | La classe 2 contient les **amortissements (28x)**, de solde créditeur, qui diminuent la classe. Le *net* s'en occupe. |
| Stocks | `net(3)` | Même idée avec les provisions de stocks (39x). |
| Tiers débiteurs | `Σ solde_debit(4)` | **Compte par compte** : un client qui nous doit de l'argent est un actif ; un client au solde créditeur n'est *pas* soustrait ici, il va au PASSIF. |
| Trésorerie | `Σ solde_debit(5)` | Banque/caisse au solde positif. |
| Actifs courants | stocks + tiers débiteurs + trésorerie | |

### Étape 3 — PASSIF (comment c'est financé)

| Poste | Formule | Pourquoi |
|---|---|---|
| Capital, réserves, dettes non courantes | `-net(1)` | La classe 1 a des soldes créditeurs, donc `net` est négatif ; on inverse le signe. |
| Résultat de l'exercice | `bilan_resultat()` = produits(7) − charges(6) | Bénéfice (+) ou perte (−) pas encore reversé en classe 1. |
| Capitaux propres et passifs non courants | classe 1 + résultat | |
| Tiers créditeurs | `Σ solde_credit(4)` | Fournisseurs, État, clients ayant trop payé. |
| Concours bancaires | `Σ solde_credit(5)` | Compte bancaire à découvert. |
| Passifs courants | tiers créditeurs + concours | |

### Étape 4 — Pourquoi le résultat est ajouté à la main

Pendant l'année, produits (classe 7) et charges (classe 6) s'accumulent dans leurs propres comptes. Ils n'ont **pas encore** été fusionnés dans les capitaux propres. Le bilan doit pourtant s'équilibrer : il calcule donc le bénéfice et l'ajoute au PASSIF :

```
Actif  =  Capitaux propres + Dettes
Actif  =  (Capital + [Produits − Charges]) + Dettes
```

Après l'**écriture de clôture** de fin d'exercice, les classes 6 et 7 sont remises à zéro et le bénéfice vit en classe 1 (compte 13). `bilan_resultat()` renvoie alors 0 et rien n'est compté deux fois (commentaire dans `bilan.c`).

### Étape 5 — Vérifier

```c
bilan_verifier(total_actif, total_passif);   // ETAT_OK ssi égaux
```

En cas d'échec, contrôlez dans cet ordre : la balance (`balance_verifier`) → le grand livre (`grand_livre_verifier`) → les écritures (`ecritures_est_equilibree`).

---

## 4. Exemple complet (mêmes données que le fichier Python)

Devise : DT. Dans le moteur, les montants sont ×1000 (millimes).

| Date | Réf | Débit | Crédit | Montant |
|---|---|---|---|---|
| 01/01 | CAP-001 | 53 Banques | 10 Capital | 100 000 |
| 05/01 | IMM-001 | 22 Immobilisations corporelles | 53 Banques | 30 000 |
| 10/01 | ACH-001 | 31 Stocks | 40 Fournisseurs | 20 000 |
| 20/01 | VTE-001 | 41 Clients | 70 Ventes | 15 000 |
| 25/01 | LOY-001 | 61 Services extérieurs | 53 Banques | 2 000 |
| 28/01 | ENC-001 | 53 Banques | 41 Clients | 5 000 |

**Balance au 31/01** (Σ débit = Σ crédit = 172 000 ✔)

| Compte | Σ débit | Σ crédit | Solde débiteur | Solde créditeur |
|---|---|---|---|---|
| 10 Capital | 0 | 100 000 | | 100 000 |
| 22 Immobilisations | 30 000 | 0 | 30 000 | |
| 31 Stocks | 20 000 | 0 | 20 000 | |
| 40 Fournisseurs | 0 | 20 000 | | 20 000 |
| 41 Clients | 15 000 | 5 000 | 10 000 | |
| 53 Banques | 105 000 | 32 000 | 73 000 | |
| 61 Services ext. | 2 000 | 0 | 2 000 | |
| 70 Ventes | 0 | 15 000 | | 15 000 |

**Bilan au 31/01**

```
ACTIF                                            PASSIF
  Actifs non courants        30 000                Capitaux propres et passifs non courants   113 000
  Actifs courants           103 000                  Capital, réserves, dettes non courantes  100 000
    Stocks                   20 000                  Résultat de l'exercice                    13 000   (15 000 − 2 000)
    Tiers débiteurs          10 000                Passifs courants                            20 000
    Trésorerie               73 000                  Tiers créditeurs                          20 000
                                                     Concours bancaires                             0
TOTAL ACTIF                 133 000              TOTAL PASSIF                                 133 000   ✔
```

---

## 5. L'appeler

### En C (bas niveau)

```c
Monnaie ta, tp; size_t n;
ligne_bilan lignes[15];
Etat st = bilan_generer(comptes, nb_comptes, balance, nb_balance, lignes, 15, &n, &ta, &tp);
if (st == ETAT_OK) st = bilan_verifier(ta, tp);
```

### Via l'API Session (celle qu'utilise Python)

```c
Etat Session_GenererBilan(const Session *s, DATE date_arret,
                          ligne_bilan *sortie, size_t capacite, size_t *nb,
                          Monnaie *total_actif, Monnaie *total_passif);
```

### En Python (`ctypes`)

`services/report_service.py` appelle `Session_GenererBilan` avec un tableau `CLigneBilan` de 15 éléments ; l'interface (`bilan_window.py`) indente avec `profondeur` et met en gras les lignes `est_sous_total`. Convertissez les millimes en DT uniquement pour l'affichage (`currency_helper.py`).

---

## 6. Pièges

| Piège | Explication |
|---|---|
| Utiliser une balance mensuelle | Bilan faux. Elle doit être cumulée depuis l'origine. |
| Additionner sous-totaux et feuilles | Double comptage. Utilisez `est_sous_total`. |
| Netter la classe 4 en bloc | Faux : un client débiteur et un fournisseur créditeur ne doivent **pas** se compenser. Le code sépare par signe, compte par compte. |
| Oublier les classes 6/7 | Résultat absent → bilan déséquilibré exactement du montant du bénéfice. |
| Compte inconnu dans la balance | Renvoie `ERR_COMPTE_INTROUVABLE` plutôt qu'un bilan faux sans le dire (voulu). |
| 15 lignes fixes | Ajouter une ligne sans mettre à jour `NB_LIGNES_BILAN` renvoie `ERR_ECHEC_GENERATION_RAPPORT`. |
| Détail par poste | Le bilan actuel est **par classe** (compact). Un bilan détaillé (par compte 21, 22, 41…) serait une nouvelle fonctionnalité. |

## 7. Lié : `compte_resultat_generer`

Le compte de résultat répartit les classes 6/7 en 7 rubriques (`RubriqueCR`) : exploitation, financier, exceptionnel (produits et charges pour chacun) et impôt sur les bénéfices. Il donne `resultat_exploitation`, `resultat_financier`, `resultat_exceptionnel`, `resultat_avant_impot` et `resultat_net` (positif = bénéfice). Le `resultat_net` est le même nombre que celui de la ligne 11 du bilan.
