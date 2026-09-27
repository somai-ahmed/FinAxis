/*
 * session.h -- session du moteur : tout ce qui est conserve en memoire
 *              pour une entreprise
 *
 * Une Session possede le plan comptable, les ecritures du journal et
 * la configuration.
 * La structure est opaque (definie dans core/session.c) : le code C et
 * Python (ctypes) ne manipulent qu'un Session* et passent uniquement
 * par les fonctions ci-dessous.
 *
 * Regles de propriete
 *   - Les fonctions Add* COPIENT les donnees fournies ; l'appelant
 *     conserve ses propres buffers.
 *   - Get* / Find* retournent des pointeurs en lecture seule appartenant
 *     a la session. Ils restent valides jusqu'au prochain appel a Add*,
 *     Session_Reset ou Session_Destroy (les tableaux internes peuvent
 *     etre realloues). Ne jamais les liberer.
 *
 * Convention de retour : Etat (ETAT_OK == 0, ERR_* negatif en cas d'echec).
 */
#ifndef SESSION_H
#define SESSION_H

#include <stddef.h>
#include <stdint.h>

#include "types.h"
#include "errors.h"
#include "dates.h"
#include "ecritures.h"


#define FNC_SESSION_NOM_LEN 128
#define FNC_SESSION_DEVISE_LEN 4 /* code ISO 4217 + NUL, ex. "TND" */

/* ------------------------------------------------------------------ */
/* configuration                                                      */
/* ------------------------------------------------------------------ */

typedef struct Session Session;

/* Structure simple (uniquement des champs de taille fixe) afin que
 * ctypes puisse la mapper directement. */
typedef struct {
    char nom_entreprise[FNC_SESSION_NOM_LEN];
    char devise[FNC_SESSION_DEVISE_LEN];
    DATE debut_exercice;
    DATE fin_exercice;
} SessionConfig;

/* ------------------------------------------------------------------ */
/* Cycle de vie                                                       */
/* ------------------------------------------------------------------ */

/* config == NULL -> valeurs par defaut (nom vide, "TND").
 * En cas de succes, *out_session est defini ; en cas d'echec,
 * il reste NULL. */
Etat creer_session(const SessionConfig *config, Session **out_session);

/* Libere tout ce que la session possede. Peut etre appelee avec NULL. */
void detruire_session(Session *session);

/* Supprime tous les comptes et toutes les ecritures,
 * mais conserve la configuration. */
Etat reinitialiser_session(Session *session);

/* ------------------------------------------------------------------ */
/* Configuration + derniere erreur                                   */
/* ------------------------------------------------------------------ */

const SessionConfig *Session_avoir_Config(const Session *session);
Etat Session_SetConfig(Session *session, const SessionConfig *config);

/* Etat de l'appel ayant echoue le plus recemment sur cette session
 * (ETAT_OK si aucune erreur). */
Etat Session_avoir_le_dernier_Erreur(const Session *session);

/* ------------------------------------------------------------------ */
/* Plan comptable                                                     */
/* ------------------------------------------------------------------ */

/* Copie *compte dans la session. Echec en cas d'identifiant ou de code
 * deja existant. */
Etat Session_ajouterCompte(Session *session, const Compte *compte);

size_t Session_avoir_nombre_comptes(const Session *session);
const Compte *Session_avoir_CompteAt(const Session *session, size_t index);
const Compte *Session_chercher_Compte_Par_ID(const Session *session, int32_t id);
const Compte *Session_chercher_Compte_Par_Code(const Session *session, const char *code);

/* ------------------------------------------------------------------ */
/* Journal                                                            */
/* ------------------------------------------------------------------ */

/* Valide l'ecriture (equilibree, comptes connus) avant de stocker
 * une copie profonde.
 * Rien n'est stocke si la validation echoue. */
Etat Session_AddEcriture(Session *session, const Ecriture *ecriture);

size_t Session_GetEcritureCount(const Session *session);
const Ecriture *Session_avoir_EcritureAt(const Session *session, size_t index);

#ifdef __cplusplus
}
#endif

#endif

