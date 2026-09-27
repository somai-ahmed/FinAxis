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

/* Meme principe, mais pour le tableau de Resultat_Detection final : un
 * doublon trouve = un Resultat_Detection ajoute a la fin du tableau. */
static Etat ajouter_resultat_doublon(Resultat_Detection **resultats, size_t *nombre, size_t *capacite, const ligne_contexte *a, const ligne_contexte *b, int32_t ecart_jours, int fenetre_jours) {
    Resultat_Detection *r;

    if (*nombre == *capacite) {
        size_t nouvelle_capacite = (*capacite == 0) ? 16 : (*capacite * 2);
        Resultat_Detection *nv = realloc(*resultats, nouvelle_capacite * sizeof(Resultat_Detection));

        if (nv == NULL) {
            return ERR_SORTIE_DU_MEMOIRE;
        }
        *resultats = nv;
        *capacite = nouvelle_capacite;
    }

    r = &(*resultats)[*nombre];
    memset(r, 0, sizeof(*r));
    r->methode = DETECT_DOUBLONS;
    /* meme jour = plus suspect qu'un ecart de plusieurs jours dans la fenetre */
    r->gravite = (ecart_jours == 0) ? GRAVITE_ELEVEE : GRAVITE_MOYENNE;
    r->id_compte = a->compte_id;
    r->id_ligne = b->id_ligne; /* "b" est la ligne la plus tardive : celle qu'on suspecte d'etre le doublon de "a" */
    snprintf(r->description, sizeof(r->description),
        "Doublon possible : compte %u, meme montant %s sur les ecritures %s et %s, %d jour(s) d'ecart",
        (unsigned)a->compte_id, a->est_debit ? "au debit" : "au credit", a->reference, b->reference, ecart_jours);

    /* score normalise entre 0 et 1 : plus les deux dates sont proches,
     * plus le score (donc la suspicion) est eleve */
    r->score = 1.0 - ((double)ecart_jours / (double)fenetre_jours);

    (*nombre)++;
    return ETAT_OK;
}

/* ------------------------------------------------------------------
                      API interne DE Detection
 * ------------------------------------------------------------------ */
Etat detecter_doublons(Session *session, idperiodefiscale id_periode, const config_detection *cfg, Resultat_Detection **resultats, size_t *nombre_resultats) {
    /* declaration */
    size_t nombre_ecritures;
    size_t i, j;
    ligne_contexte *lignes = NULL;
    size_t nb_lignes = 0;
    size_t capacite_lignes = 0;
    size_t capacite_resultats = 0;
    Etat etat;

    /* verification */
    if (session == NULL || cfg == NULL || resultats == NULL || nombre_resultats == NULL) {
        return ERR_POINTEUR_NULLE;
    }

    /* initialisation */
    *resultats = NULL;
    *nombre_resultats = 0;

    /* une fenetre a 0 (ou negative) ne veut rien dire : pas de config valide,
     * pas de detection possible */
    if (cfg->fenetre_jours_doublon <= 0) {
        return ERR_CONFIGURATION_DETECTION_INVALIDE;
    }

    nombre_ecritures = Session_GetEcritureCount(session);

    /* parcours & verification des lignes du journal */
    for (i = 0; i < nombre_ecritures; i++) {
        const Ecriture *ecriture = Session_avoir_EcritureAt(session, i);

        if (ecriture == NULL || ecriture->periode_id != id_periode || !ecriture->est_validee) {
            continue;
        }

        for (j = 0; j < ecriture->nombre_lignes; j++) {
            const ligne_journal *ligne = &ecriture->lignes[j];
            ligne_contexte ctx;

            if (!ligne_avoir_montant(ligne, &ctx.montant, &ctx.est_debit)) {
                continue; /* ligne a zero des deux cotes : rien a comparer */
            }

            ctx.id_ecriture = ecriture->id;
            ctx.id_ligne = ligne->id;
            ctx.compte_id = ligne->compte_id;
            ctx.date = ecriture->date;
            /* snprintf tronque proprement si la reference depasse 31 caracteres + le \0 final */
            snprintf(ctx.reference, sizeof(ctx.reference), "%s", ecriture->reference);

            etat = ajouter_ligne_contexte(&lignes, &nb_lignes, &capacite_lignes, &ctx);
            if (etat != ETAT_OK) {
                free(lignes);
                return etat;
            }
        }
    }
