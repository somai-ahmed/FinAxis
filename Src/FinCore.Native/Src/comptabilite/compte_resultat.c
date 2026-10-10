/*
 * Génère le compte de résultat (charges et produits) d'un exercice.
 *
 * idee : ici on ne relit pas le journal :: On reçoit, pour chaque compte
 * le total de ses débits et le total de ses crédits sur la période
 * (c'est exactement ce que calcule la balance). 
 * On garde seulement les comptes de classe 6 (charges) et de classe 7 (produits)
 * on les range  par rubrique, puis on calcule les résultats intermédiaires :
 * exploitation, financier, exceptionnel, avant impot etc...
 */

#include <stdio.h>   /* snprintf : écrire du texte dans un tableau sans déborder */
#include <string.h>  /* memset, strlen, strncmp : outils pour la mémoire et les chaînes */

#include "Src/FinCore.Native/include/types.h"
#include "Src/FinCore.Native/include/errors.h" 

#include "Src/FinCore.Native/include/comptes.h"
#include "Src/FinCore.Native/include/monnaie.h"
#include "Src/FinCore.Native/include/reports.h"

/* ------------------------------------------------------------------
   Table de correspondance : début du code du compte -> rubrique   
  ------------------------------------------------------------------ */

/* Une règle = "si le code commence par ce préfixe, alors c'est cette rubrique" */
typedef struct {
    const char *prefixe;   /* les premiers chiffres du code du compte */
    RubriqueCR rubrique;   /* la rubrique du compte de résultat visée */
} RegleRubrique;

/*
              MATRICE DES VALEURS : (DATASET)
 * Quand plusieurs règles correspondent, on garde la plus précise (le préfixe
 * le plus long). Exemple : le compte 686 tombe dans "686" (financier)
 * et non dans "68" (exploitation).
 * Si tes comptes suivent le SCE tunisien plutôt que le PCG, c'est ICI
 * (et seulement ici) qu'il faut ajuster les préfixes
 */
static const RegleRubrique REGLES_RUBRIQUES[] = {
    /* Charges d'exploitation */
    { "60", CR_CHARGES_EXPLOITATION },
    { "61", CR_CHARGES_EXPLOITATION },
    { "62", CR_CHARGES_EXPLOITATION },
    { "63", CR_CHARGES_EXPLOITATION },
    { "64", CR_CHARGES_EXPLOITATION },
    { "65", CR_CHARGES_EXPLOITATION },
    { "68", CR_CHARGES_EXPLOITATION },

    /* Charges financières et exceptionnelles */
    { "66",  CR_CHARGES_FINANCIERES },
    { "686", CR_CHARGES_FINANCIERES },
    { "67",  CR_CHARGES_EXCEPTIONNELLES },
    { "687", CR_CHARGES_EXCEPTIONNELLES },

    /* Impôt sur les bénéfices */
    { "69", CR_IMPOT_BENEFICES },

    /* Produits d'exploitation */
    { "70", CR_PRODUITS_EXPLOITATION },
    { "71", CR_PRODUITS_EXPLOITATION },
    { "72", CR_PRODUITS_EXPLOITATION },
    { "73", CR_PRODUITS_EXPLOITATION },
    { "74", CR_PRODUITS_EXPLOITATION },
    { "75", CR_PRODUITS_EXPLOITATION },
    { "78", CR_PRODUITS_EXPLOITATION },
    { "79", CR_PRODUITS_EXPLOITATION },

    /* Produits financiers et exceptionnels */
    { "76",  CR_PRODUITS_FINANCIERS },
    { "786", CR_PRODUITS_FINANCIERS },
    { "77",  CR_PRODUITS_EXCEPTIONNELS },
    { "787", CR_PRODUITS_EXCEPTIONNELS }
};


/* Nombre de règles : taille totale du tableau divisée par la taille d'une case */
#define NB_REGLES_RUBRIQUES ((int)(sizeof(REGLES_RUBRIQUES) / sizeof(REGLES_RUBRIQUES[0])))

/* ------------------------------------------------------------------ */
/*  Fonctions internes (static = visibles seulement dans ce fichier)  */
/* ------------------------------------------------------------------ */

/*
 * Cherche la rubrique d'un compte à partir de son code
 * Retourne 1 si c'est un compte de charge ou de produit, 0 sinon
 * (les comptes des classes 1 à 5 n'ont rien à faire dans le compte de résultat)
 */
static int trouver_rubrique(const char *code, RubriqueCR *rubrique) {
    /* Un code vide, ou qui ne commence pas par 6 ou 7, on l'ignore */
    if (code == NULL || (code[0] != '6' && code[0] != '7')) {
        return 0;
    }

    /* Valeur de secours : si aucune règle ne correspond, on ne perd pas le compte,
       on le met dans l'exploitation de son côté (charge ou produit) */
    *rubrique = (code[0] == '6') ? CR_CHARGES_EXPLOITATION : CR_PRODUITS_EXPLOITATION;

    size_t meilleure_longueur = 0;

    for (int i = 0; i < NB_REGLES_RUBRIQUES; i++) {
        /* strlen donne le nombre de caractères du préfixe (sans le '\0' final) */
        size_t longueur = strlen(REGLES_RUBRIQUES[i].prefixe);

        /* strncmp compare seulement les "longueur" premiers caractères des deux
           textes et renvoie 0 s'ils sont identiques. Donc 0 = le code commence
           bien par ce préfixe */
        if (longueur > meilleure_longueur &&
            strncmp(code, REGLES_RUBRIQUES[i].prefixe, longueur) == 0) {
            meilleure_longueur = longueur;
            *rubrique = REGLES_RUBRIQUES[i].rubrique;
        }
    }

    return 1;
}

/*
 * Calcule le montant d'un compte pour le compte de resultat
 * Une charge est normalement débitrice : montant = débit - crédit
 * Un produit est normalement créditeur : montant = crédit - débit
 * Comme ça, un montant positif veut dire "situation normale" dans les deux cas
 */
static Monnaie calculer_montant(int est_produit, Monnaie debit, Monnaie credit) {
    if (est_produit) {
        return credit - debit;
    }
    return debit - credit;
}

/* Calcule les totaux et les résultats intermédiaires à partir des totaux par rubrique */
static void calculer_resultats(CompteResultat *r) {
    r->total_produits = r->total_rubrique[CR_PRODUITS_EXPLOITATION]
                      + r->total_rubrique[CR_PRODUITS_FINANCIERS]
                      + r->total_rubrique[CR_PRODUITS_EXCEPTIONNELS];

    r->total_charges = r->total_rubrique[CR_CHARGES_EXPLOITATION]
                     + r->total_rubrique[CR_CHARGES_FINANCIERES]
                     + r->total_rubrique[CR_CHARGES_EXCEPTIONNELLES]
                     + r->total_rubrique[CR_IMPOT_BENEFICES];

    r->resultat_exploitation = r->total_rubrique[CR_PRODUITS_EXPLOITATION]
                             - r->total_rubrique[CR_CHARGES_EXPLOITATION];

    r->resultat_financier = r->total_rubrique[CR_PRODUITS_FINANCIERS]
                          - r->total_rubrique[CR_CHARGES_FINANCIERES];

    r->resultat_exceptionnel = r->total_rubrique[CR_PRODUITS_EXCEPTIONNELS]
                             - r->total_rubrique[CR_CHARGES_EXCEPTIONNELLES];

    /* resultat avant impôt = les trois resultats precedants additionnees */
    r->resultat_avant_impot = r->resultat_exploitation
                            + r->resultat_financier
                            + r->resultat_exceptionnel;

    /* resultat net = ce qu'il reste une fois l'impot sur les bénéfices payé
       Positif = benefice || negatif = perte */
    r->resultat_net = r->resultat_avant_impot - r->total_rubrique[CR_IMPOT_BENEFICES];
}

/* ------------------------------------------------------------------
                     Fonctions publiques                                            
------------------------------------------------------------------- */

/*
 * Génère le compte de résultat.
 * comptes / total_debit / total_credit sont trois tableaux "parallèles" :
 * la case i de chaque tableau parle du même compte.
 */
Etat compte_resultat_generer(const Compte *comptes,
                             const Monnaie *total_debit,
                             const Monnaie *total_credit,
                             int nb_comptes,
                             CompteResultat *resultat) {
    /* Première protection : aucun pointeur ne doit être vide. */
    if (comptes == NULL || total_debit == NULL || total_credit == NULL || resultat == NULL) {
        return ERR_POINTEUR_NULLE;
    }
    if (nb_comptes < 0) {
        return ERR_ARGUMENT_INVALIDE;
    }

    /* memset remplit toute la structure avec des 0 : lignes vides, totaux a zero.
       sizeof(*resultat) = la taille exacte de la structure à effacer. */
    memset(resultat, 0, sizeof(*resultat));

    for (int i = 0; i < nb_comptes; i++) {
        const char *code = comptes[i].code;
        RubriqueCR rubrique;

        /* On saute les comptes qui ne sont pas des charges ou des produits.
           On ne regarde pas est_active : un compte désactivé peut quand même
           avoir eu des mouvements pendant la periode. */
        if (!trouver_rubrique(code, &rubrique)) {
            continue;
        }

        /* On saute aussi les comptes sans aucun mouvement, ils n'apportent rien. */
        if (total_debit[i] == 0 && total_credit[i] == 0) {
            continue;
        }

        /* Le tableau de lignes est plein : on le signale au lieu de deborder . */
        if (resultat->nb_lignes >= CR_MAX_LIGNES) {
            return ERR_TRES_PETIT_BUFFER;
        }

        int est_produit = (code[0] == '7');
        Monnaie montant = calculer_montant(est_produit, total_debit[i], total_credit[i]);

        /* On prend un pointeur vers la prochaine case libre pour ecrire plus court. */
        LigneCompteResultat *ligne = &resultat->lignes[resultat->nb_lignes];

        /* snprintf copie le texte dans le tableau de la ligne, s'arrete avant
           la fin si c est trop long, et ajoute toujours le '\0' final. */
        snprintf(ligne->code, sizeof(ligne->code), "%s", code);
        snprintf(ligne->nom, sizeof(ligne->nom), "%s", comptes[i].nom);

        ligne->rubrique = rubrique;
        ligne->montant = montant;

        resultat->nb_lignes++;
        resultat->total_rubrique[rubrique] += montant;
    }

    calculer_resultats(resultat);

    return ETAT_OK;
}

/* Donne le nom lisible d'une rubrique (pour l'affichage ou l'export). */
const char *compte_resultat_nom_rubrique(RubriqueCR rubrique) {
    switch (rubrique) {
        case CR_PRODUITS_EXPLOITATION:   return "Produits d'exploitation";
        case CR_CHARGES_EXPLOITATION:    return "Charges d'exploitation";
        case CR_PRODUITS_FINANCIERS:     return "Produits financiers";
        case CR_CHARGES_FINANCIERES:     return "Charges financières";
        case CR_PRODUITS_EXCEPTIONNELS:  return "Produits exceptionnels";
        case CR_CHARGES_EXCEPTIONNELLES: return "Charges exceptionnelles";
        case CR_IMPOT_BENEFICES:         return "Impôt sur les bénéfices";
        default:                         return "Rubrique inconnue";
    }
}
