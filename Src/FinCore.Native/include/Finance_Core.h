#ifndef FINAXIS_CORE_H
#define FINAXIS_CORE_H

#include "types.h"
#include "errors.h"
#include "detection.h"



/*=======definition des constants========*/
#define LONGUEUR_MAX_LIBELLE             256
#define LONGUEUR_MAX_CODE_COMPTE         16
#define LONGUEUR_MAX_DEVISE              4
#define LONGUEUR_MAX_ID_ECRITURE         32
#define LONGUEUR_MAX_ID_PERIODE          16
#define LONGUEUR_MAX_ID_UTILISATEUR      32
#define LONGUEUR_MAX_REFERENCE           64
#define LONGUEUR_MAX_SECTION             64
#define LONGUEUR_MAX_DATE                11
#define LONGUEUR_MAX_DATE_HEURE          20

#define MAX_SESSIONS                     16
#define MAX_COMPTES                      1000
#define MAX_PERIODES                     50
#define MAX_LIGNES_JOURNAL               100000
#define MAX_ECRITURES                    50000
#define MAX_DETECTIONS                   10000
#define MAX_LIGNES_RAPPORT               50000

#define BENFORD_ECHANTILLON_MIN          300

/*=======les enumerations========*/
typedef enum {
    OK = 0,
    ERREUR_ENTREE_INVALIDE = -1,
    ERREUR_MEMOIRE_INSUFFISANTE = -2,
    ERREUR_COMPTE_INTROUVABLE = -3,
    ERREUR_ECRITURE_NON_EQUILIBREE = -4,
    ERREUR_PERIODE_FERMEE = -5,
    ERREUR_CLE_DUPLIQUEE = -6,
    ERREUR_ENTREE_SORTIE = -7,
    ERREUR_DETECTION = -8,
    ERREUR_SESSION_INTROUVABLE = -9,
    ERREUR_PERIODE_INTROUVABLE = -10,
    ERREUR_ECRITURE_INTROUVABLE = -11,
    ERREUR_COMPTE_UTILISE = -12,
    ERREUR_MONTANT_NUL = -13,
    ERREUR_DATE_INVALIDE = -14,
    ERREUR_DONNEES_INSUFFISANTES = -15
} ETAT;

typedef enum {
    ACTIF = 1,
    PASSIF = 2,
    CHARGE = 3,
    PRODUIT = 4
} TypeCompte;

typedef enum {
    CAT_BALANCE = 0,
    CAT_BILAN = 1,
    CAT_RESULTAT = 2
} Categorie_Compte;

typedef enum {
    PERIODE_OUVERTE = 0,
    PERIODE_FERMEE = 1,
    PERIODE_VERROUILLEE = 2
} ETAT_PERIODE;


/*
*    INDICATEUR DU DETECTION
*    
* Chaque valeur correspond a un bit :
* 
*    0x01 -> Benford
*    0x02 -> Doublons
*    0x04 -> Nombres ronds
*    0x08 -> Valeurs aberrantes
*
*    0x0F -> Toutes les detections
*
*    VOIR :
* Documentation/Indicateur_Detection_explication_Binaire.ipynb
*/
    
typedef enum {
    DET_BENFORD = 0x01,
    DET_DOUBLON = 0x02,
    DET_NOMBRE_ROND = 0x04,
    DET_VALEUR_ABERRANTE = 0x08,
    DET_TOUTES = 0x0F
} IndicateurDetection;

/*=========STRUCTURE DES DONNEES=========*/

/*
 * Structure representant un compte comptable.
 */
typedef struct {
    char    code[ LONGUEUR_MAX_CODE_COMPTE];
    char    libelle[LONGUEUR_MAX_LIBELLE];
    int     type;
    int     categorie;
    char    parent[LONGUEUR_MAX_CODE_COMPTE];
    double  solde_initial;
    int     est_actif;
}Compte;


/*
 * Structure representant une ligne d'ecriture comptable.
 */
typedef struct {
    char    id_ecriture[LONGUEUR_MAX_ID_ECRITURE];
    DATE date;
    char    code_compte[LONGUEUR_MAX_CODE_COMPTE ];
    char    libelle[LONGUEUR_MAX_LIBELLE];
    char    reference[LONGUEUR_MAX_REFERENCE];
    double  debit;
    double  credit;
    char    devise[LONGUEUR_MAX_DEVISE];
    double  taux_change;
    char    id_periode[LONGUEUR_MAX_ID_PERIODE];
    char    id_utilisateur[LONGUEUR_MAX_ID_UTILISATEUR];
    DATE_H temps_creation;
}LigneJournal;

