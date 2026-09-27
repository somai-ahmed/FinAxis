/*
 * doublons.c
 * ----------
 * Detection des doublons : deux ecritures DIFFERENTES qui debitent (ou
 * creditent) le meme compte du meme montant, a quelques jours d'intervalle,
 * sont suspectes d'etre une saisie en double (erreur de manipulation, ou
 * fraude qui rejoue volontairement une meme depense).
 *
 * Methode : on met a plat toutes les lignes de la periode dans un tableau,
 * puis on compare chaque ligne a toutes celles qui la suivent (comparaison
 * en O(n^2) --> complexite :: sur les lignes de UNE periode : le volume reste raisonnable,
 * pas besoin d'une structure plus complexe pour l'instant).
 *
 * Deux lignes sont retenues comme "doublon" si :
 *   - elles appartiennent a deux ecritures differentes (deux lignes de la
 *     meme ecriture ne sont pas un doublon, juste une ecriture normale) ;
 *   - meme compte, meme montant, meme sens (debit contre debit, ou credit
 *     contre credit) ;
 *   - leurs dates sont a moins de cfg->fenetre_jours_doublon jours
 *     d'ecart 
 */

#include <stdio.h>   /* snprintf : formatage de texte dans un buffer de taille fixe */
#include <stdlib.h>  /* malloc/realloc/free : allocation dynamique des tableaux */
#include <string.h>  /* memset : remise a zero d'une structure */

#include "Src/FinCore.Native/include/types.h"
#include "Src/FinCore.Native/include/errors.h"
#include "Src/FinCore.Native/include/session.h"
#include "Src/FinCore.Native/include/detection.h"
#include "Src/FinCore.Native/include/dates.h"



/* ------------------------------------------------------------------
 * Structure interne : une ligne de journal avec juste
 * ce qu'il faut de son ecriture parente pour comparer deux lignes entre
 * elles sans avoir a re-parcourir la session a chaque fois.
 * ------------------------------------------------------------------ */
typedef struct {
    IdEcriture id_ecriture;      /* variable unique */
    id_ligne   id_ligne;
    id_compte compte_id;
    Monnaie montant;          /* le cote non nul de la ligne (Debit ou credit) */
    int est_debit;        /* 1 si "montant" vient du Debit, 0 si du credit */
    DATE date;             /* date de l'ecriture parente */
    char reference[32];    /* reference de l'ecriture parente, pour les messages */
} ligne_contexte;

/* ------------------------------------------------------------------
                 Fonctions internes (static functions)
 * ------------------------------------------------------------------ */

/* Une ligne_journal valide n'a jamais Debit ET credit non nuls en meme temps
 (voir ecritures_ligne_valide) : cette fonction renvoie lequel des deux
 est renseigne. Renvoie false si la ligne est a zero des deux cotes
 (rien a comparer). */
static bool ligne_avoir_montant(const ligne_journal *ligne, Monnaie *montant_out, int *est_debit_out) {
    if (ligne->Debit != 0) {
        *montant_out = ligne->Debit;
        *est_debit_out = 1;
        return true;
    }
    if (ligne->credit != 0) {
        *montant_out = ligne->credit;
        *est_debit_out = 0;
        return true;
    }
    return false;
}

/* Ajoute un element a un tableau dynamique de ligne_contexte, en doublant
 la capacite quand elle est atteinte -- meme logique de la fonction "REALLOC"*/
static Etat ajouter_ligne_contexte(ligne_contexte **lignes, size_t *nombre, size_t *capacite, const ligne_contexte *nouvelle) {
    if (*nombre == *capacite) {
        size_t nouvelle_capacite = (*capacite == 0) ? 64 : (*capacite * 2);
        ligne_contexte *nv = realloc(*lignes, nouvelle_capacite * sizeof(ligne_contexte));

        if (nv == NULL) {
            return ERR_SORTIE_DU_MEMOIRE;
        }
        *lignes = nv;
        *capacite = nouvelle_capacite;
    }

    (*lignes)[*nombre] = *nouvelle;
    (*nombre)++;
    return ETAT_OK;
}
