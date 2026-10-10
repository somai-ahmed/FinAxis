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

#incldue "Src/FinCore.Native/include/cJSON.h"
#include "Src/FinCore.Native/include/session.h"
#include "Src/FinCore.Native/include/comptes.h"
#include "Src/FinCore.Native/include/ecritures.h"


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

/* Remplit une config avec les valeurs par defaut (exp.nom vide, devise "TND") */
static void config_par_defaut(SessionConfig *config) {
    memset(config, 0, sizeof(SessionConfig));   /* initalisation a zero : chaines vides & dates a 0 */
    copier_texte(config->devise, FNC_SESSION_DEVISE_LEN, "TND"); /* TND par defaut */
}

/* Libere les lignes de chaque ecriture */
static void liberer_ecritures(Session *session) {
    size_t i;

    for (i = 0; i < session->nb_ecritures; i++) {
        ecritures_detruire(&session->ecritures[i]);
    }
    free(session->ecritures);
    session->ecritures = NULL;
    session->nb_ecritures = 0;
    session->capacite_ecritures = 0;
}

/* elargit le tableau de comptes si il est plein (realloc) */
static Etat reserver_comptes(Session *session) {
    Compte *nouveau;
    size_t nouvelle_capacite;

    if (session->nb_comptes < session->capacite_comptes) {
        return ETAT_OK;   /* il reste de la place */
    }

    nouvelle_capacite = (session->capacite_comptes == 0) ? SESSION_CAPACITE_INITIALE : session->capacite_comptes * 2;
    nouveau = realloc(session->comptes, nouvelle_capacite * sizeof(Compte));
    if (nouveau == NULL) {
        return ERR_SORTIE_DU_MEMOIRE;   /* l'ancien tableau reste valide */
    }
    session->comptes = nouveau;
    session->capacite_comptes = nouvelle_capacite;
    return ETAT_OK;
}

/* elargire le tableau des ecritures si il est plein (realloc) */
      /* meme logique que reserver_comptes */
static Etat reserver_ecritures(Session *session) {
    Ecriture *nouveau;
    size_t nouvelle_capacite;

    if (session->nb_ecritures < session->capacite_ecritures) {
        return ETAT_OK;
    }

    nouvelle_capacite = (session->capacite_ecritures == 0) ? SESSION_CAPACITE_INITIALE : session->capacite_ecritures * 2;
    nouveau = realloc(session->ecritures, nouvelle_capacite * sizeof(Ecriture));
    if (nouveau == NULL) {
        return ERR_SORTIE_DU_MEMOIRE;
    }
    session->ecritures = nouveau;
    session->capacite_ecritures = nouvelle_capacite;
    return ETAT_OK;
}

/* la recherche par id (autre version en cas du changement) */
static Compte *trouver_compte_modifiable(Session *session, id_compte id) {
    size_t i;

    for (i = 0; i < session->nb_comptes; i++) {
        if (session->comptes[i].id == id) {
            return &session->comptes[i];
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------
                   Fonctions du cycle du vie                   
 ------------------------------------------------------------------ */

Etat creer_session(const SessionConfig *config, Session **out_session) {
    Session *session;

    if (out_session == NULL) {
        return ERR_POINTEUR_NULLE;
    }
    *out_session = NULL;


    session = calloc(1, sizeof(Session));
    if (session == NULL) {
        return ERR_SORTIE_DU_MEMOIRE;
    }

    if (config == NULL) {
        config_par_defaut(&session->config);
    } else {
        session->config = *config;

        session->config.nom_entreprise[FNC_SESSION_NOM_LEN - 1] = '\0';
        session->config.devise[FNC_SESSION_DEVISE_LEN - 1] = '\0';
    }

      /* initialisation a 1 , car 0 est INVALID_ID */
    session->prochain_id_compte = 1;   
    session->prochain_id_ecriture = 1;
    session->derniere_erreur = ETAT_OK;

    *out_session = session; /* SUCCESS */
    return ETAT_OK;
}

void detruire_session(Session *session) {
    if (session == NULL) {
        return;
    }
    liberer_ecritures(session);
    free(session->comptes);
    free(session);
}

Etat reinitialiser_session(Session *session) {
    if (session == NULL) {
        return ERR_SESSION_INVALIDE;
    }

      /* liberation & re-initilialisation */

    liberer_ecritures(session);
    free(session->comptes);
    session->comptes = NULL;
    session->nb_comptes = 0;
    session->capacite_comptes = 0;

    session->prochain_id_compte = 1;
    session->prochain_id_ecriture = 1;
    session->derniere_erreur = ETAT_OK;

    return ETAT_OK;
}

/*------------------------------------------
                  CONFIG
--------------------------------------------*/

const SessionConfig *Session_avoir_Config(const Session *session) {
    if (session == NULL) {
        return NULL;
    }
    return &session->config;
}

Etat Session_SetConfig(Session *session, const SessionConfig *config) {
    if (session == NULL) {
        return ERR_SESSION_INVALIDE;
    }
    if (config == NULL) {
        return session_echec(session, ERR_POINTEUR_NULLE);
    }

    session->config = *config;
    session->config.nom_entreprise[FNC_SESSION_NOM_LEN - 1] = '\0';
    session->config.devise[FNC_SESSION_DEVISE_LEN - 1] = '\0';
    return ETAT_OK;
}

Etat Session_avoir_le_dernier_Erreur(const Session *session) {
    if (session == NULL) {
        return ERR_SESSION_INVALIDE;
    }
    return session->derniere_erreur;
}
