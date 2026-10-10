/*
 * rapports.h -- rapports financiers : balance, grand livre, bilan, compte de resultat
 *
 * Tous les rapports travaillent sur des tableaux fournis par l'appelant
 * (tableau + capacite + nombre ecrit), sans allocation cachee : c'est simple
 * a appeler depuis Python avec ctypes.
 *
 * Les montants sont de type Monnaie (int64, 3 decimales, voir types.h).
 * Les structures de ligne (LigneBalance, Entree_GrandLivre, ligne_bilan)
 * sont definies dans types.h.
 *
 * Convention de retour : Etat (ETAT_OK == 0, ERR_* negatif en cas d'echec).
 */
#ifndef RAPPORTS_H
#define RAPPORTS_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "types.h"
#include "errors.h"
#include "dates.h"
#include "session.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */
/* Balance                                                            */
/* ------------------------------------------------------------------ */

/* Une ligne par compte (dans l'ordre de comptes[]), sur la periode [debut, fin].
 * Seules les ecritures comptabilisees sont retenues.
 * capacite doit etre >= nb_comptes. */
Etat balance_generer(const Compte *comptes, size_t nb_comptes,
                     const Ecriture *ecritures, size_t nb_ecritures,
                     DATE debut, DATE fin,
                     LigneBalance *sortie, size_t capacite, size_t *nb_sortie);

Monnaie balance_total_debit(const LigneBalance *lignes, size_t nombre);
Monnaie balance_total_credit(const LigneBalance *lignes, size_t nombre);
bool    balance_est_equilibree(const LigneBalance *lignes, size_t nombre);
Etat    balance_verifier(const LigneBalance *lignes, size_t nombre);

/* ------------------------------------------------------------------ */
/* Grand livre (un seul compte)                                       */
/* ------------------------------------------------------------------ */

/* Solde du compte juste AVANT la date donnee (report a nouveau) */
Monnaie grand_livre_solde_initial(const Compte *compte,
                                  const Ecriture *ecritures, size_t nb_ecritures,
                                  DATE avant_date);

Etat grand_livre_generer(id_compte compte_id,
                         const Compte *comptes, size_t nb_comptes,
                         const Ecriture *ecritures, size_t nb_ecritures,
                         DATE debut, DATE fin,
                         Entree_GrandLivre *sortie, size_t capacite, size_t *nb_sortie);

/* Controle que le dernier solde cumule est coherent avec la ligne de balance */
Etat grand_livre_verifier(const Entree_GrandLivre *entrees, size_t nombre,
                          const LigneBalance *ligne);

/* ------------------------------------------------------------------ */
/* Bilan                                                              */
/* ------------------------------------------------------------------ */

/* Somme des soldes (debiteurs ou crediteurs selon sens) d'une classe de comptes */
Monnaie bilan_somme_soldes(const Compte *comptes, size_t nb_comptes,
                           const LigneBalance *balance, size_t nb_balance,
                           ClasseCompte classe, SoldeNormal sens);

/* Resultat de l'exercice = produits nets (classe 7) - charges nettes (classe 6) */
Monnaie bilan_resultat(const Compte *comptes, size_t nb_comptes,
                       const LigneBalance *balance, size_t nb_balance);

/* Le bilan a toujours 15 lignes : capacite >= 15 */
Etat bilan_generer(const Compte *comptes, size_t nb_comptes,
                   const LigneBalance *balance, size_t nb_balance,
                   ligne_bilan *sortie, size_t capacite, size_t *nb_sortie,
                   Monnaie *total_actif, Monnaie *total_passif);

Etat bilan_verifier(Monnaie total_actif, Monnaie total_passif);

/* ------------------------------------------------------------------ */
/* Compte de resultat                                                 */
/* ------------------------------------------------------------------ */

#define CR_MAX_LIGNES 512

typedef enum RubriqueCR {
    CR_PRODUITS_EXPLOITATION = 0,
    CR_CHARGES_EXPLOITATION,
    CR_PRODUITS_FINANCIERS,
    CR_CHARGES_FINANCIERES,
    CR_PRODUITS_EXCEPTIONNELS,
    CR_CHARGES_EXCEPTIONNELLES,
    CR_IMPOT_BENEFICES,
    CR_NB_RUBRIQUES
} RubriqueCR;

typedef struct LigneCompteResultat {
    char       code[16];
    char       nom[128];
    RubriqueCR rubrique;
    Monnaie    montant;      /* positif = situation normale (charge debitrice, produit crediteur) */
} LigneCompteResultat;

typedef struct CompteResultat {
    LigneCompteResultat lignes[CR_MAX_LIGNES];
    size_t  nb_lignes;
    Monnaie total_rubrique[CR_NB_RUBRIQUES];
    Monnaie total_produits;
    Monnaie total_charges;
    Monnaie resultat_exploitation;
    Monnaie resultat_financier;
    Monnaie resultat_exceptionnel;
    Monnaie resultat_avant_impot;
    Monnaie resultat_net;    /* positif = benefice, negatif = perte */
} CompteResultat;

/* comptes[], total_debit[], total_credit[] sont des tableaux paralleles (meme indice = meme compte) */
Etat compte_resultat_generer(const Compte *comptes,
                             const Monnaie *total_debit,
                             const Monnaie *total_credit,
                             int nb_comptes,
                             CompteResultat *resultat);

const char *compte_resultat_nom_rubrique(RubriqueCR rubrique);

/* ------------------------------------------------------------------ */
/* Interface "Session" : l'API que l'interface Python appelle          */
/* ------------------------------------------------------------------ */
/* Ces fonctions lisent directement le plan comptable et le journal de la
 * session : l'appelant n'a ni tableau de comptes ni tableau d'ecritures a
 * preparer. Les tableaux de sortie restent a sa charge (tableau + capacite). */

/* capacite >= Session_avoir_nombre_comptes(session).
 * Retourne ERR_AUCUNE_DONNEE_PERIODE (sortie remplie a zero) si aucune
 * ecriture comptabilisee ne tombe dans [debut, fin]. */
Etat Session_GenererBalance(const Session *session, DATE debut, DATE fin,
                            LigneBalance *sortie, size_t capacite, size_t *nb_sortie);

/* Appel avec capacite = 0 : *nb_sortie donne la taille necessaire
 * (ERR_TRES_PETIT_BUFFER), comme grand_livre_generer. */
Etat Session_GenererGrandLivre(const Session *session, id_compte compte_id, DATE debut, DATE fin,
                               Entree_GrandLivre *sortie, size_t capacite, size_t *nb_sortie);

/* Bilan arrete a date_arret (soldes cumules depuis le debut) : 15 lignes */
Etat Session_GenererBilan(const Session *session, DATE date_arret,
                          ligne_bilan *sortie, size_t capacite, size_t *nb_sortie,
                          Monnaie *total_actif, Monnaie *total_passif);

Etat Session_GenererCompteResultat(const Session *session, DATE debut, DATE fin,
                                   CompteResultat *resultat);

#ifdef __cplusplus
}
#endif

#endif
