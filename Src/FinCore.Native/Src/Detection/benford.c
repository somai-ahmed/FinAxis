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

/* ------------------------------------------------------------------
                      API interne du module Detection
------------------------------------------------------------------ */

Etat calculer_rapport_benford(Session *session, idperiodefiscale id_periode, Rapport_Benford *rapport) {
  /* declaration des variables necessaires */
    size_t nombre_ecritures;
    size_t i, j;
    double *montants = NULL;
    size_t nb_montants = 0;
    size_t capacite = 0;
    int comptes_observes[9];
    double ecarts[9];
    double observes_d[9];
    double attendus_d[9];

    /*             verif               */
    if (session == NULL || rapport == NULL) {
        return ERR_POINTEUR_NULLE;
    }

    /*         initialisation          */
    memset(rapport, 0, sizeof(*rapport));
    memset(comptes_observes, 0, sizeof(comptes_observes));

    nombre_ecritures = Session_GetEcritureCount(session);


  /* parcours du tablau avec les verification 
      double parcours inclus des conditions d'arret*/
    for (i = 0; i < nombre_ecritures; i++) {
        const Ecriture *ecriture = Session_avoir_EcritureAt(session, i);

        if (ecriture == NULL || ecriture->periode_id != id_periode || !ecriture->est_validee) {
            continue;
        }

        for (j = 0; j < ecriture->nombre_lignes; j++) {
            Monnaie montant = montant_significatif_ligne(&ecriture->lignes[j]);
            Etat etat;

            if (montant == 0) {
                continue; /* une ligne a 0 n'a pas de premier chiffre significatif */
            }

            /* Monnaie est un entier en millimes (echelle fixe, voir "Src/FinCore.Native/include/types.h") :
             * multiplier ou diviser un nombre par une constante ne change
             * jamais son premier chiffre significatif, donc pas besoin de
             * reconvertir vers une unite "reelle" avant l'analyse */
            etat = ajouter_montant(&montants, &nb_montants, &capacite, (double)montant);
            if (etat != ETAT_OK) {
                free(montants);
                return etat;
            }
        }
    }

    if (nb_montants < BENFORD_TAILLE_ECHANTILLON_MIN) {
        free(montants);
        return ERR_ECHANTILLON_BENFORD_INSUFFISANT;
    }

    /*  pour chaque montant retenu, on extrait son premier
     * chiffre significatif (math_premier_chiffre, deja code dans
     * math_utils.c) et on incremente le compteur correspondant. */
    for (i = 0; i < nb_montants; i++) {
        int chiffre = math_premier_chiffre(montants[i]);

        if (chiffre >= 1 && chiffre <= 9) {
            comptes_observes[chiffre - 1]++;
        }
    }

    /* les montants bruts ont fait leur travail, plus besoin de les garder */
    free(montants);
    montants = NULL;

    /* on remplit stat[] : une entree StatChiffreBenford par chiffre 1-9,
     * avec la frequence attendue (loi de Benford), la frequence reellement
     * observee, et le nombre brut de montants qui commencent par ce chiffre */
    for (i = 0; i < 9; i++) {
        int chiffre = (int)i + 1;
        double freq_attendu = math_frequence_benford(chiffre);
        double freq_observe = (double)comptes_observes[i] / (double)nb_montants;

        rapport->stat[i].numero = chiffre;
        rapport->stat[i].freq_attendu = freq_attendu;
        rapport->stat[i].freq_observe = freq_observe;
        rapport->stat[i].nombre_observe = comptes_observes[i];

        /* ecart entre observe et attendu, utilise juste apres pour
         * calculer l'ecart absolu moyen sur les 9 chiffres */
        ecarts[i] = freq_observe - freq_attendu;

        /* effectifs (pas frequences) pour le test du Khi-deux standard :
         * attendu_i = frequence_attendue_i * taille de l'echantillon */
        observes_d[i] = (double)comptes_observes[i];
        attendus_d[i] = freq_attendu * (double)nb_montants;
    }

    rapport->chiffre_carree = math_chi_carre(observes_d, attendus_d, 9);
    rapport->ecart_absolu_moyen = math_ecart_absolu_moyen(ecarts, 9);
    rapport->taille_echantillon = (int)nb_montants;

    /* estnormal = 1 si le Khi-deux reste sous le seuil critique (8 degres
     * de liberte, seuil de confiance 95%) -> distribution jugee conforme
     * a la loi de Benford, 0 sinon */
    rapport->estnormal = (rapport->chiffre_carree <= BENFORD_SEUIL_CHI_CARRE) ? 1 : 0;

    return ETAT_OK;
}
