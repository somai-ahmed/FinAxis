/*
 * session_io.c -- implementation de la Session du moteur FinAxis
 *
 * La Session est la "memoire" du moteur pour UNE entreprise :
 * elle garde le plan comptable, le journal et la configuration.
 * La structure est definie ici seulement (opaque) : le reste du code,
 * et Python via ctypes, ne voit qu'un pointeur Session*.
 */


#include <stdlib.h>
#include <string.h>  

#include "session.h"
#include "comptes.h"
#include "ecritures.h"
#include "periodes.h"
#include "validation.h"

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

    prop_periode_fiscale *periodes; /* periodes fiscales (tableau dynamique) */
    size_t nb_periodes;
    size_t capacite_periodes;

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
    size_t n = strlen(source);
    if (n >= taille) {
        n = taille - 1;
    }
    memcpy(destination, source, n);
    destination[n] = '\0';
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
    free(session->periodes);
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

    free(session->periodes);
    session->periodes = NULL;
    session->nb_periodes = 0;
    session->capacite_periodes = 0;

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

/* ------------------------------------------------------------------
                         IDENTIFIANTS
 ------------------------------------------------------------------ */

id_compte Session_nouvel_id_compte(Session *session) {
    if (session == NULL) {
        return INVALID_ID;
    }
    return session->prochain_id_compte++;
}

IdEcriture Session_nouvel_id_ecriture(Session *session) {
    if (session == NULL) {
        return INVALID_ID;
    }
    return session->prochain_id_ecriture++;
}

/* ------------------------------------------------------------------
                         PLAN COMPTABLE
 ------------------------------------------------------------------ */

Etat Session_ajouterCompte(Session *session, const Compte *compte) {
    size_t i;
    Etat etat;

    if (session == NULL) {
        return ERR_SESSION_INVALIDE;
    }
    if (compte == NULL) {
        return session_echec(session, ERR_POINTEUR_NULLE);
    }
    if (!comptes_valider(compte)) {
        return session_echec(session, ERR_DONNEES_COMPTABLES_INVALIDES);
    }

    /* le code est unique dans tout le plan comptable */
    for (i = 0; i < session->nb_comptes; i++) {
        if (strcmp(session->comptes[i].code, compte->code) == 0) {
            return session_echec(session, ERR_COMPTE_EXISTE);
        }
    }
    /* l'identifiant aussi (sauf id == 0 : la session en attribue un) */
    if (compte->id != INVALID_ID && trouver_compte_modifiable(session, compte->id) != NULL) {
        return session_echec(session, ERR_COMPTE_EXISTE);
    }
    /* le parent, s'il y en a un, doit deja exister */
    if (compte->parent_id != INVALID_ID && trouver_compte_modifiable(session, compte->parent_id) == NULL) {
        return session_echec(session, ERR_COMPTE_INTROUVABLE);
    }

    etat = reserver_comptes(session);
    if (etat != ETAT_OK) {
        return session_echec(session, etat);
    }

    session->comptes[session->nb_comptes] = *compte;   /* copie : la session garde sa propre version */
    if (compte->id == INVALID_ID) {
        session->comptes[session->nb_comptes].id = session->prochain_id_compte++;
    } else if (compte->id >= session->prochain_id_compte) {
        session->prochain_id_compte = compte->id + 1;   /* on ne reutilise jamais un id */
    }
    session->nb_comptes++;
    return ETAT_OK;
}

size_t Session_avoir_nombre_comptes(const Session *session) {
    return (session == NULL) ? 0 : session->nb_comptes;
}

const Compte *Session_avoir_CompteAt(const Session *session, size_t index) {
    if (session == NULL || index >= session->nb_comptes) {
        return NULL;
    }
    return &session->comptes[index];
}

const Compte *Session_chercher_Compte_Par_ID(const Session *session, int32_t id) {
    size_t i;

    if (session == NULL || id <= 0) {
        return NULL;
    }
    for (i = 0; i < session->nb_comptes; i++) {
        if (session->comptes[i].id == (id_compte)id) {
            return &session->comptes[i];
        }
    }
    return NULL;
}

const Compte *Session_chercher_Compte_Par_Code(const Session *session, const char *code) {
    size_t i;

    if (session == NULL || code == NULL) {
        return NULL;
    }
    for (i = 0; i < session->nb_comptes; i++) {
        if (strcmp(session->comptes[i].code, code) == 0) {
            return &session->comptes[i];
        }
    }
    return NULL;
}

/* ------------------------------------------------------------------
                            PERIODES
 ------------------------------------------------------------------ */

/* recherche modifiable d'une periode par id (interne) */
static prop_periode_fiscale *trouver_periode_modifiable(Session *session, idperiodefiscale id) {
    size_t i;

    for (i = 0; i < session->nb_periodes; i++) {
        if (session->periodes[i].id == id) {
            return &session->periodes[i];
        }
    }
    return NULL;
}

Etat Session_ajouterPeriode(Session *session, const prop_periode_fiscale *periode) {
    size_t i;

    if (session == NULL) {
        return ERR_SESSION_INVALIDE;
    }
    if (periode == NULL) {
        return session_echec(session, ERR_POINTEUR_NULLE);
    }
    if (!periodes_est_valide(periode)) {
        return session_echec(session, ERR_ARGUMENT_INVALIDE);
    }
    if (trouver_periode_modifiable(session, periode->id) != NULL) {
        return session_echec(session, ERR_PERIODE_EXISTE);
    }
    for (i = 0; i < session->nb_periodes; i++) {
        if (periodes_se_chevauchent(&session->periodes[i], periode)) {
            return session_echec(session, ERR_PERIODE_CHEVAUCHEMENT);
        }
    }

    if (session->nb_periodes == session->capacite_periodes) {
        size_t nouvelle_capacite = (session->capacite_periodes == 0) ? 12 : session->capacite_periodes * 2;
        prop_periode_fiscale *nouveau = realloc(session->periodes, nouvelle_capacite * sizeof(prop_periode_fiscale));

        if (nouveau == NULL) {
            return session_echec(session, ERR_SORTIE_DU_MEMOIRE);
        }
        session->periodes = nouveau;
        session->capacite_periodes = nouvelle_capacite;
    }

    session->periodes[session->nb_periodes++] = *periode;
    return ETAT_OK;
}

size_t Session_avoir_nombre_periodes(const Session *session) {
    return (session == NULL) ? 0 : session->nb_periodes;
}

const prop_periode_fiscale *Session_avoir_PeriodeAt(const Session *session, size_t index) {
    if (session == NULL || index >= session->nb_periodes) {
        return NULL;
    }
    return &session->periodes[index];
}

const prop_periode_fiscale *Session_chercher_Periode_Par_ID(const Session *session, idperiodefiscale id) {
    size_t i;

    if (session == NULL) {
        return NULL;
    }
    for (i = 0; i < session->nb_periodes; i++) {
        if (session->periodes[i].id == id) {
            return &session->periodes[i];
        }
    }
    return NULL;
}

Etat Session_cloturerPeriode(Session *session, idperiodefiscale id) {
    prop_periode_fiscale *p;

    if (session == NULL) return ERR_SESSION_INVALIDE;
    p = trouver_periode_modifiable(session, id);
    if (p == NULL) return session_echec(session, ERR_PERIODE_INTROUVABLE);
    return session_echec(session, periodes_cloturer(p));
}

Etat Session_rouvrirPeriode(Session *session, idperiodefiscale id) {
    prop_periode_fiscale *p;

    if (session == NULL) return ERR_SESSION_INVALIDE;
    p = trouver_periode_modifiable(session, id);
    if (p == NULL) return session_echec(session, ERR_PERIODE_INTROUVABLE);
    return session_echec(session, periodes_rouvrir(p));
}

Etat Session_verrouillerPeriode(Session *session, idperiodefiscale id) {
    prop_periode_fiscale *p;

    if (session == NULL) return ERR_SESSION_INVALIDE;
    p = trouver_periode_modifiable(session, id);
    if (p == NULL) return session_echec(session, ERR_PERIODE_INTROUVABLE);
    return session_echec(session, periodes_verrouiller(p));
}

/* ------------------------------------------------------------------
                             JOURNAL
 ------------------------------------------------------------------ */

/* copie profonde : meme contenu, mais ses propres lignes (malloc) */
static Etat copier_ecriture(Ecriture *destination, const Ecriture *source) {
    *destination = *source;   /* copie des champs simples, y compris le pointeur lignes (corrige juste apres) */
    destination->lignes = NULL;

    if (source->nombre_lignes > 0) {
        destination->lignes = malloc(source->nombre_lignes * sizeof(ligne_journal));
        if (destination->lignes == NULL) {
            destination->nombre_lignes = 0;
            return ERR_SORTIE_DU_MEMOIRE;
        }
        memcpy(destination->lignes, source->lignes, source->nombre_lignes * sizeof(ligne_journal));
    }
    return ETAT_OK;
}

Etat Session_AddEcriture(Session *session, const Ecriture *ecriture) {
    size_t i;
    Etat etat;
    Ecriture *stockee;
    const prop_periode_fiscale *periode;

    if (session == NULL) {
        return ERR_SESSION_INVALIDE;
    }
    if (ecriture == NULL) {
        return session_echec(session, ERR_POINTEUR_NULLE);
    }

    /* 1. validation : equilibre, lignes correctes, date, libelle */
    etat = ecritures_valider(ecriture);
    if (etat != ETAT_OK) {
        return session_echec(session, etat);
    }

    /* 2. tous les comptes doivent exister et etre actifs */
    for (i = 0; i < ecriture->nombre_lignes; i++) {
        const Compte *c = Session_chercher_Compte_Par_ID(session, (int32_t)ecriture->lignes[i].compte_id);

        if (c == NULL) {
            return session_echec(session, ERR_COMPTE_INTROUVABLE);
        }
        if (!c->est_active) {
            return session_echec(session, ERR_COMPTE_INACTIF);
        }
    }

    /* 3. la periode (si l'ecriture en reference une) doit exister, etre ouverte et contenir la date.
     *    periode_id == 0 : ecriture sans periode, acceptee (utile pour les imports) */
    if (ecriture->periode_id != INVALID_ID) {
        periode = Session_chercher_Periode_Par_ID(session, ecriture->periode_id);
        if (periode == NULL) {
            return session_echec(session, ERR_PERIODE_INTROUVABLE);
        }
        if (!periodes_est_ouverte(periode)) {
            return session_echec(session, ERR_PERIODE_FERMEE);
        }
        if (!periodes_contient_date(periode, ecriture->date)) {
            return session_echec(session, ERR_DATE_HORS_EXERCICE);
        }
    }

    /* 4. l'identifiant doit etre libre (0 = la session en attribue un) */
    if (ecriture->id != INVALID_ID && Session_chercher_Ecriture_Par_ID(session, ecriture->id) != NULL) {
        return session_echec(session, ERR_JOURNAL_DEJA_EXISTANT);
    }

    etat = reserver_ecritures(session);
    if (etat != ETAT_OK) {
        return session_echec(session, etat);
    }

    /* 5. stockage : copie profonde, rien n'est garde si la copie echoue */
    stockee = &session->ecritures[session->nb_ecritures];
    etat = copier_ecriture(stockee, ecriture);
    if (etat != ETAT_OK) {
        return session_echec(session, etat);
    }
    stockee->est_validee = 0;   /* on stocke d'abord un brouillon */
    if (stockee->id == INVALID_ID) {
        stockee->id = session->prochain_id_ecriture++;
    } else if (stockee->id >= session->prochain_id_ecriture) {
        session->prochain_id_ecriture = stockee->id + 1;
    }
    session->nb_ecritures++;

    /* 6. si l'appelant l'a donnee comme validee : comptabilisation tout de suite
     *    (met a jour les soldes des comptes) */
    if (ecriture->est_validee) {
        etat = ecritures_comptabiliser(stockee, session->comptes, session->nb_comptes);
        if (etat != ETAT_OK) {
            /* on annule le stockage pour ne rien garder de partiel */
            ecritures_detruire(stockee);
            session->nb_ecritures--;
            return session_echec(session, etat);
        }
    }
    return ETAT_OK;
}

size_t Session_GetEcritureCount(const Session *session) {
    return (session == NULL) ? 0 : session->nb_ecritures;
}

const Ecriture *Session_avoir_EcritureAt(const Session *session, size_t index) {
    if (session == NULL || index >= session->nb_ecritures) {
        return NULL;
    }
    return &session->ecritures[index];
}

const Ecriture *Session_chercher_Ecriture_Par_ID(const Session *session, IdEcriture id) {
    size_t i;

    if (session == NULL || id == INVALID_ID) {
        return NULL;
    }
    for (i = 0; i < session->nb_ecritures; i++) {
        if (session->ecritures[i].id == id) {
            return &session->ecritures[i];
        }
    }
    return NULL;
}

Etat Session_comptabiliserEcriture(Session *session, IdEcriture id) {
    size_t i;

    if (session == NULL) {
        return ERR_SESSION_INVALIDE;
    }
    for (i = 0; i < session->nb_ecritures; i++) {
        if (session->ecritures[i].id == id) {
            return session_echec(session, ecritures_comptabiliser(&session->ecritures[i], session->comptes, session->nb_comptes));
        }
    }
    return session_echec(session, ERR_JOURNAL_INTROUVABLE);
}
