/*
 * session_io.h -- sauvegarde / chargement d'une Session au format .FinAxis (JSON)
 *
 * Format (version 1) : un objet JSON avec les cles
 *   "format", "version", "config", "comptes", "periodes", "ecritures".
 * Les montants sont ecrits en unites mineures (entiers, millimes).
 * Les soldes des comptes ne sont PAS sauvegardes : ils sont recalcules
 * au chargement en rejouant les ecritures validees.
 *
 * Convention de retour : Etat (ETAT_OK == 0, ERR_* negatif en cas d'echec).
 */
#ifndef SESSION_IO_H
#define SESSION_IO_H

#include "types.h"
#include "errors.h"
#include "session.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FNC_FORMAT_NOM "FinAxis"
#define FNC_FORMAT_VERSION 1

/* Ecrit la session dans le fichier (ecrase le fichier existant) */
Etat Session_Sauvegarder(const Session *session, const char *chemin);

/* Lit le fichier et cree une NOUVELLE session dans *out_session
 * (a detruire avec detruire_session). En cas d'echec, *out_session reste NULL. */
Etat Session_Charger(const char *chemin, Session **out_session);

#ifdef __cplusplus
}
#endif

#endif
