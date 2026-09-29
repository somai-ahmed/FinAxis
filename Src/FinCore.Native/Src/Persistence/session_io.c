/*
 * session_io.c -- implementation de la Session du moteur FinCore
 *
 * La Session est la "memoire" du moteur pour UNE entreprise :
 * elle garde le plan comptable, le journal et la configuration.
 * La structure est definie ici seulement (opaque) : le reste du code,
 * et Python via ctypes, ne voit qu'un pointeur Session*.
 */


#include <stdlib.h>  /* pour calloc/realloc/free : allocation dynamique */
#include <string.h>  /* pour memcpy/strncpy/strcmp : copier et comparer */

#include "Src/FinCore.Native/include/session.h"
#include "Src/FinCore.Native/include/comptes.h"
#include "Src/FinCore.Native/include/ecritures.h"

/* Capacite de depart des tableaux -- definition*/
#define SESSION_CAPACITE_INITIALE 16

/* ------------------------------------------------------------------
      les structures internes ( cache du l'utilisateur du DLL)       
---------------------------------------------------------------------- */


struct Session {
    SessionConfig config;          /* nom, devise, exercice */

    Compte *comptes;               /* plan comptable (tableau dynamique ( calloc, realloc & session capacite) */
    size_t  nb_comptes;
    size_t  capacite_comptes;

    Ecriture *ecritures;           /* journal (tableau dynamique) */
    size_t nb_ecritures;
    size_t capacite_ecritures;

    id_compte prochain_id_compte;    /* identifiants uniques */
    IdEcriture prochain_id_ecriture;

    Etat derniere_erreur;          /* dernier echec sur cette session */
};

/* ------------------------------------------------------------------
                   Fonctions internes (static)                      
 ------------------------------------------------------------------ */

/* Retient l'erreur dans la session puis la renvoie telle quelle.
 * un exemple vivant du code reel et exuctable pour la
 clarite d'usage      "return session_echec(session, ERR_...)" en une ligne. */

static Etat session_echec(Session *session, Etat etat) {
    if (session != NULL && etat != ETAT_OK) {
        session->derniere_erreur = etat;
    }
    return etat;
}

/* Copie un texte dans un buffer de taille fixe */
static void copier_texte(char *destination, size_t taille, const char *source) {
    if (source == NULL) {
        destination[0] = '\0';
        return;
    }
    strncpy(destination, source, taille - 1);
    destination[taille - 1] = '\0';
}
