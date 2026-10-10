#ifndef ECRITURES_H
#define ECRITURES_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "types.h"
#include "errors.h"
#include "comptes.h"

/* Création et libération */
Etat ecritures_creer(Ecriture *ecriture, idperiodefiscale periode_id, DATE date, const char *reference, const char *description);
void ecritures_detruire(Ecriture *ecriture);

/* Ajout et consultation des lignes */
Etat ecritures_ajouter_ligne(Ecriture *ecriture, id_compte compte_id, Monnaie debit, Monnaie credit, const char *libelle);
size_t ecritures_nombre_lignes(const Ecriture *ecriture);
bool ecritures_avoir_ligne(const Ecriture *ecriture, size_t index, ligne_journal *ligne_out);

/* Validation */
bool ecritures_ligne_valide(const ligne_journal *ligne);
bool ecritures_est_equilibree(const Ecriture *ecriture);
Etat ecritures_valider(const Ecriture *ecriture);

/* Totaux */
Monnaie ecritures_total_debit(const Ecriture *ecriture);
Monnaie ecritures_total_credit(const Ecriture *ecriture);

/* Comptabilisation : valide puis applique les lignes aux soldes des comptes */
Etat ecritures_comptabiliser(Ecriture *ecriture, Compte *comptes, size_t nombre_comptes);
bool ecritures_est_comptabilisee(const Ecriture *ecriture);

#endif
