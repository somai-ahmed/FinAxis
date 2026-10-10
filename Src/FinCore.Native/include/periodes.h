#ifndef PERIODES_H
#define PERIODES_H

/* ============================================================
 * Moteur Comptable FinAxis - Périodes fiscales
 * ============================================================
 * Une période fiscale est un intervalle de dates fermé
 * [date_debut, date_fin] portant un statut :
 *
 *      OUVERTE  <-->  CLOTURE  -->  VEROUILLEE
 *
 *   OUVERTE      les écritures sont acceptées
 *   CLOTURE      les écritures sont refusées, réouverture possible
 *   VEROUILLEE   définitive, aucune réouverture
 *
 * Une collection de périodes est un tableau fourni par
 * l'appelant (tableau + nombre + capacité), comme pour les
 * comptes : pas d'allocation, simple à passer via ctypes.
 * Deux périodes d'une même collection ne se chevauchent jamais.
 * ============================================================ */

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "types.h"
#include "errors.h"
#include "dates.h"

#define PERIODES_PAR_ANNEE 12

/* Création et validation */
Etat periodes_creer(prop_periode_fiscale *periode, idperiodefiscale id, const char *nom, DATE date_debut, DATE date_fin);
bool periodes_plage_valide(DATE date_debut, DATE date_fin);
bool periodes_est_valide(const prop_periode_fiscale *periode);

/* Génère les 12 périodes mensuelles d'une année civile (ids consécutifs) */
Etat periodes_generer_mensuelles(int32_t annee, idperiodefiscale premier_id, prop_periode_fiscale *sortie, size_t capacite);

/* Consultation */
bool periodes_est_ouverte(const prop_periode_fiscale *periode);
bool periodes_est_cloturee(const prop_periode_fiscale *periode);
bool periodes_est_verrouillee(const prop_periode_fiscale *periode);
bool periodes_contient_date(const prop_periode_fiscale *periode, DATE date);
bool periodes_se_chevauchent(const prop_periode_fiscale *a, const prop_periode_fiscale *b);
int32_t periodes_duree_jours(const prop_periode_fiscale *periode);

/* Cycle de vie : OUVERTE <--> CLOTURE --> VEROUILLEE */
Etat periodes_cloturer(prop_periode_fiscale *periode);
Etat periodes_rouvrir(prop_periode_fiscale *periode);
Etat periodes_verrouiller(prop_periode_fiscale *periode);

/* Collection : tableau + nombre + capacité fournis par l'appelant */
Etat periodes_ajouter(prop_periode_fiscale *periodes, size_t *nombre, size_t capacite, const prop_periode_fiscale *nouvelle);
Etat periodes_trouver_par_id(const prop_periode_fiscale *periodes, size_t nombre, idperiodefiscale id, size_t *indice_out);
Etat periodes_trouver_par_date(const prop_periode_fiscale *periodes, size_t nombre, DATE date, size_t *indice_out);

/* Contrôle avant comptabilisation : la période de l'écriture existe,
 * est ouverte, et la date de l'écriture tombe bien dans cette période */
Etat periodes_verifier_ecriture(const prop_periode_fiscale *periodes, size_t nombre, const Ecriture *ecriture);

#endif
