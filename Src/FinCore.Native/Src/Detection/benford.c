/*
 * benford.c
 * ---------
 * Implemente l'analyse de la loi de Benford pour le moteur de detection.
 *
 * Rappel du principe (voir Documentation/Benford_strategie.ipynb) : dans
 * beaucoup de jeux de donnees "naturels" (montants comptables inclus), le
 * premier chiffre significatif n'est pas uniformement reparti entre 1 et 9 :
 * le chiffre 1 apparait environ 30% du temps, le 9 seulement environ 4.6%.
 * Une fraude "fabriquee" (montants inventes) s'ecarte souvent de cette loi,
 * d'ou l'interet de la detecter automatiquement.
 *
 * Ce fichier fournit deux fonctions, toutes les deux forward-declarees
 * dans detection.c (elles ne font pas partie de l'API publique de la DLL) :
 *   - calculer_rapport_benford : le rapport statistique complet
 *   - detecter_benford_anomalies : l'adaptateur qui transforme ce rapport
 *     en un (eventuel) Resultat_Detection pour l'orchestrateur
 */

#include <stdio.h>   /* pour snprintf : construire le texte de description */
#include <stdlib.h>  /* pour malloc/realloc/free : allocation dynamique */
#include <string.h>  /* pour memset : remettre une structure a zero proprement */

#include "Src/FinCore.Native/include/types.h"
#include "Src/FinCore.Native/include/errors.h"
#include "Src/FinCore.Native/include/session.h"
#include "Src/FinCore.Native/include/detection.h"
#include "Src/FinCore.Native/include/math_utils.h"


/* En dessous de ce nombre de montants, le test du Khi-deux n'est plus
 * fiable statistiquement (regle empirique generalement admise pour
 * Benford : un echantillon trop petit donne des frequences observees
 * trop instables pour juger quoi que ce soit). */
#define BENFORD_TAILLE_ECHANTILLON_MIN 30

/* ------------------------------------------------------------------
              Fonction interne (static functions)
 -------------------------------------------------------------------*/

/* Une ligne_journal valide n'a jamais Debit ET credit non nuls en meme
    temps (voir ecritures_ligne_valide dans ecritures.c) : un seul cote
    porte le montant, c'est celui-la qui nous interesse ici. */

static Monnaie montant_significatif_ligne(const ligne_journal *ligne) {
    return (ligne->Debit != 0) ? ligne->Debit : ligne->credit;
}

/* Ajoute une valeur au tableau dynamique "valeurs", en l'agrandissant
 * (doublement de capacite) si besoin. Meme logique de realloc function (malloc.h library)*/

static Etat ajouter_montant(double **valeurs, size_t *nombre, size_t *capacite, double valeur) {
    if (*nombre == *capacite) {
        size_t nouvelle_capacite = (*capacite == 0) ? 64 : (*capacite * 2);
        double *nv = realloc(*valeurs, nouvelle_capacite * sizeof(double));

        if (nv == NULL) {
            return ERR_SORTIE_DU_MEMOIRE;
        }
        *valeurs = nv;
        *capacite = nouvelle_capacite;
    }

    (*valeurs)[*nombre] = valeur;
    (*nombre)++;
    return ETAT_OK;
}
