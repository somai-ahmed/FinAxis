#ifndef VALIDATION_H
#define VALIDATION_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "monnaie.h"

/* Fonctions de validation generiques reutilisees par les couches
 * metier (comptes, ecritures, import Excel, etc.).
 *
 * Convention : chaque fonction retourne un bool (true = valide).
 * Aucune de ces fonctions n'alloue ni ne modifie la chaine
 * d'entree. */

/* ---- chaines de caracteres ---- */
bool valider_chaine_non_vide(const char *chaine);
bool valider_longueur_chaine(const char *chaine, size_t min, size_t max);
bool valider_caracteres_autorises(const char *chaine, const char *caracteres_extra);

/* ---- codes et references metier ---- */
bool valider_code_compte(const char *code, size_t longueur_min, size_t longueur_max);
bool valider_format_reference(const char *reference, const char *prefixe_attendu);

/* ---- valeurs numeriques ---- */
bool valider_plage_i64(int64_t valeur, int64_t min, int64_t max);
bool valider_montant(monnaie m, bool autoriser_negatif);

#endif

