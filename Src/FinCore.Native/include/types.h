#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <time.h>

#include "dates.h"  


/* ============================================================
 * Moteur comptable FinAxis — definition des types fondamentaux
 * ============================================================ */

/* Representation monétaire en virgule fixe : unités entières mineures (ex. millimes)
 * pour eviter les erreurs d'arrondi en virgule flottante dans les calculs comptables */
typedef int64_t Monnaie;

#define Monnaie_Unite 1000  /* 3 decimal places (millimes) */

/* structure interne maqsquee aux consommateurs */
typedef struct Session Session;

/* Identifiants uniques */
typedef uint32_t id_compte;
typedef uint32_t IdEcriture;
typedef uint32_t JournalEntryId;
typedef uint32_t idperiodefiscale;
typedef uint32_t id_ligne;

#define INVALID_ID 0

/* ------------------------------------------------------------
 * Classification des comptes (plan comptable de type PCG tunisien / français)
 * ------------------------------------------------------------ */
/*voir documentation/classes_comptabilite.md*/
typedef enum ClasseCompte {
    CLASSE_1_CP_ET_PNC = 1,
    CLASSE_2_ACTIFS_NON_COURANTS = 2,
    CLASSE_3_STOCKS = 3,
    CLASSE_4_TIERS = 4,
    CLASSE_5_TRESORERIE = 5,
    CLASSE_6_CHARGES = 6,
    CLASSE_7_PRODUITS = 7
} ClasseCompte;

typedef enum TypeCompte {
    COMPTE_ACTIF = 1,
    COMPTE_PASSIF = 2,
    COMPTE_CAPITAUX = 3,
    COMPTE_PRODUIT = 4,
    COMPTE_CHARGE = 5
} TypeCompte;

typedef enum SoldeNormal {
    SOLDE_DEBITEUR = 0,        /* DEBIT */
    SOLDE_CREDITEUR = 1        /* CREDIT */
} SoldeNormal;

typedef struct Compte {
    id_compte    id;
    char         code[16];     
    char         nom[128];
    ClasseCompte classe;
    TypeCompte   type;
    SoldeNormal  solde_normal;
    Monnaie      solde;              /* solde courant, mis a jour par debiter/crediter */
    id_compte    parent_id;          /* 0 si racine */
    int          est_active;
}Compte;
/*--------------------------------------------------------------
    l implementation des structures pour la journal
---------------------------------------------------------------*/
typedef struct ligne_journal{
    id_ligne id;
    id_compte compte_id;
    Monnaie Debit;
    Monnaie credit ;
    char libelle[256] ;
}ligne_journal;

typedef struct Ecriture {
    IdEcriture        id;
    idperiodefiscale  periode_id;
    DATE              date;
    char              reference[32];
    char              description[256];
    ligne_journal*    lignes;
    size_t            nombre_lignes;
    int               est_validee;     /* 0 = brouillon, 1 = validée */
} Ecriture;

typedef enum status_periode_fiscale{
    PERIODE_FISCALE_OUVERTE = 1,
    PERIODE_FISCALE_CLOTURE = 2,
    PERIODE_FISCALE_VEROUILLEE = 3
}status_periode_fiscale;

typedef struct prop_periode_fiscale {
    idperiodefiscale       id;
    char                   nom[64];
    DATE                   date_debut;
    DATE                   date_fin;
    status_periode_fiscale statut;
}prop_periode_fiscale;

/*--------------------------------------------------------------
    l implementation des structures pour la balance
---------------------------------------------------------------*/

/*implementation d une structure pour appeler les champs du la structure compte*/
typedef struct ref_compte{
    id_compte id;
    char code[16];
    char nom[256];
}ref_compte;

typedef struct ligne_balance {
    ref_compte compte;
    Monnaie total_debit;
    Monnaie total_credit;
    Monnaie solde_debit;
    Monnaie solde_credit;
}LigneBalance;

typedef struct Entree_GrandLivre {
    DATE date;
    char reference[32];
    char libelle[256];
    Monnaie debit;
    Monnaie credit;
    Monnaie solde_cumule;
}Entree_GrandLivre;

typedef struct ligne_bilan {
    char libelle[128];
    Monnaie montant;
    int est_sous_total;   /* 0/1 */
    int profondeur;        /* voir documentation/bilan_exp.md */
} ligne_bilan;

/* ------------------------------------------------------------
 * Buffer générique pour passage de frontière FFI
 * ------------------------------------------------------------ */
typedef struct Buffer {
    uint8_t* donnees;
    size_t   taille;
} Buffer;

#endif
