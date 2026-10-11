/*
 * rapports_session.c -- rapports calcules directement depuis une Session
 *
 * Les fonctions de balance.c / grand_livre.c / bilan.c / compte_resultat.c
 * travaillent sur des tableaux. Ici on les branche sur la Session pour que
 * l'interface Python n'ait qu'un seul appel a faire par rapport.
 */

#include <stdlib.h> 
#include <string.h> 

#include "Src/FinCore.Native/include/rapports.h"

/* Debut du calendrier utilise pour les soldes cumules du bilan */
#define DATE_TRES_ANCIENNE ((DATE){ .annee = 1900, .mois = 1, .jour = 1 })

/* Les comptes et les ecritures de la session sont stockes dans des tableaux
 * contigus : le pointeur du 1er element suffit pour parcourir tout le tableau. */
#define SESSION_COMPTES(s)    Session_avoir_CompteAt((s), 0)
#define SESSION_ECRITURES(s)  Session_avoir_EcritureAt((s), 0)

Etat Session_GenererBalance(const Session *session, DATE debut, DATE fin, LigneBalance *sortie, size_t capacite, size_t *nb_sortie) {
    if (session == NULL) return ERR_SESSION_INVALIDE;
    if (nb_sortie == NULL) return ERR_POINTEUR_NULLE;

    if (Session_avoir_nombre_comptes(session) == 0) {
        *nb_sortie = 0;
        return ERR_AUCUNE_DONNEE_PERIODE;
    }

    return balance_generer(SESSION_COMPTES(session), Session_avoir_nombre_comptes(session),
                           SESSION_ECRITURES(session), Session_GetEcritureCount(session),
                           debut, fin, sortie, capacite, nb_sortie);
}

Etat Session_GenererGrandLivre(const Session *session, id_compte compte_id, DATE debut, DATE fin, Entree_GrandLivre *sortie, size_t capacite, size_t *nb_sortie) {
    if (session == NULL) return ERR_SESSION_INVALIDE;
    if (nb_sortie == NULL) return ERR_POINTEUR_NULLE;
    if (Session_avoir_nombre_comptes(session) == 0) return ERR_COMPTE_INTROUVABLE;

    return grand_livre_generer(compte_id, SESSION_COMPTES(session), Session_avoir_nombre_comptes(session),
                               SESSION_ECRITURES(session), Session_GetEcritureCount(session),
                               debut, fin, sortie, capacite, nb_sortie);
}

Etat Session_GenererBilan(const Session *session, DATE date_arret, ligne_bilan *sortie, size_t capacite, size_t *nb_sortie, Monnaie *total_actif, Monnaie *total_passif) {
    size_t nb_comptes, nb_balance = 0;
    LigneBalance *balance;
    Etat etat;

    if (session == NULL) return ERR_SESSION_INVALIDE;
    if (nb_sortie == NULL || total_actif == NULL || total_passif == NULL) return ERR_POINTEUR_NULLE;

    nb_comptes = Session_avoir_nombre_comptes(session);
    if (nb_comptes == 0) return ERR_DONNEES_BILAN_INSUFFISANTES;

    /* le bilan se construit a partir d'une balance cumulee depuis le debut */
    balance = malloc(nb_comptes * sizeof(LigneBalance));
    if (balance == NULL) return ERR_SORTIE_DU_MEMOIRE;

    etat = balance_generer(SESSION_COMPTES(session), nb_comptes,
                           SESSION_ECRITURES(session), Session_GetEcritureCount(session),
                           DATE_TRES_ANCIENNE, date_arret, balance, nb_comptes, &nb_balance);
    /* aucune ecriture : la balance est a zero, le bilan sera vide mais valide */
    if (etat != ETAT_OK && etat != ERR_AUCUNE_DONNEE_PERIODE) {
        free(balance);
        return etat;
    }

    etat = bilan_generer(SESSION_COMPTES(session), nb_comptes, balance, nb_balance,
                         sortie, capacite, nb_sortie, total_actif, total_passif);
    free(balance);
    return etat;
}

Etat Session_GenererCompteResultat(const Session *session, DATE debut, DATE fin, CompteResultat *resultat) {
    size_t nb_comptes, nb_balance = 0, i;
    LigneBalance *balance;
    Monnaie *debits, *credits;
    Etat etat;

    if (session == NULL) return ERR_SESSION_INVALIDE;
    if (resultat == NULL) return ERR_POINTEUR_NULLE;

    nb_comptes = Session_avoir_nombre_comptes(session);
    if (nb_comptes == 0) return ERR_DONNEES_RESULTAT_INSUFFISANTES;

    balance = malloc(nb_comptes * sizeof(LigneBalance));
    debits  = malloc(nb_comptes * sizeof(Monnaie));
    credits = malloc(nb_comptes * sizeof(Monnaie));
    if (balance == NULL || debits == NULL || credits == NULL) {
        free(balance); free(debits); free(credits);
        return ERR_SORTIE_DU_MEMOIRE;
    }

    etat = balance_generer(SESSION_COMPTES(session), nb_comptes,
                           SESSION_ECRITURES(session), Session_GetEcritureCount(session),
                           debut, fin, balance, nb_comptes, &nb_balance);
    if (etat != ETAT_OK && etat != ERR_AUCUNE_DONNEE_PERIODE) {
        free(balance); free(debits); free(credits);
        return etat;
    }

    /* balance[i] correspond au compte i : on en tire les deux tableaux paralleles attendus */
    for (i = 0; i < nb_comptes; i++) {
        debits[i]  = balance[i].total_debit;
        credits[i] = balance[i].total_credit;
    }

    etat = compte_resultat_generer(SESSION_COMPTES(session), debits, credits, (int)nb_comptes, resultat);
    free(balance); free(debits); free(credits);
    return etat;
}
