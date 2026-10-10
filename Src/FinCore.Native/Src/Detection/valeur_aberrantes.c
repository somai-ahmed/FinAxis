/*
 * valeurs_aberrantes.c
 * --------------------
 * Detection des valeurs aberrantes : pour chaque compte, on regarde si une
 * ligne s'ecarte "trop" de la moyenne des autres lignes du MEME compte.
 *voir documentation/exp_valeur_aberrantes.ipynb
 */

#include <math.h> 
#include <stdio.h>  
#include <stdlib.h>
#include <string.h>  

#include "types.h"
#include "errors.h"
#include "session.h"
#include "detection.h"
#include "math_utils.h"

#define ABERRANTES_TAILLE_GROUPE_MIN 8   /* en dessous, la moyenne et l'ecart-type ne veulent rien dire */

typedef struct {
    id_compte compte_id;
    id_ligne  ligne_id;
    double montant;        /* en DT (Monnaie / Monnaie_Unite) pour les calculs statistiques */
    char reference[32];  /* reference de l'ecriture parente, pour le message */
} ligne_stat;

/* comparaison pour qsort : regroupe les lignes par compte (puis par id de ligne pour un ordre stable) */
static int comparer_par_compte(const void *a, const void *b) {
    const ligne_stat *la = (const ligne_stat *)a;
    const ligne_stat *lb = (const ligne_stat *)b;

    if (la->compte_id != lb->compte_id) return (la->compte_id < lb->compte_id) ? -1 : 1;
    if (la->ligne_id != lb->ligne_id)   return (la->ligne_id < lb->ligne_id) ? -1 : 1;
    return 0;
}

/* ajoute une ligne_stat au tableau dynamique (capacite doublee quand elle est atteinte) */
static Etat ajouter_ligne_stat(ligne_stat **lignes, size_t *nombre, size_t *capacite, const ligne_stat *nouvelle) {
    if (*nombre == *capacite) {
        size_t nouvelle_capacite = (*capacite == 0) ? 64 : (*capacite * 2);
        ligne_stat *nv = realloc(*lignes, nouvelle_capacite * sizeof(ligne_stat));

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

static Etat ajouter_resultat_aberrant(Resultat_Detection **resultats, size_t *nombre, size_t *capacite, const ligne_stat *ligne, double z, double moyenne, double seuil) {
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
    r->methode = DETECT_VALS_ABERRANTES;
    /* au-dela de 1.5 x le seuil : tres suspect */
    r->gravite = (z >= 1.5 * seuil) ? GRAVITE_CRITIQUE : GRAVITE_ELEVEE;
    r->id_compte = ligne->compte_id;
    r->id_ligne = ligne->ligne_id;
    /* score entre 0 et 1 : 1 des que z atteint 2 x le seuil */
    r->score = z / (2.0 * seuil);
    if (r->score > 1.0) r->score = 1.0;

    snprintf(r->description, sizeof(r->description),
        "Valeur aberrante : %.3f sur le compte %u (ecriture %s), moyenne du compte %.3f, ecart %.1f sigma",
        ligne->montant, (unsigned)ligne->compte_id, ligne->reference, moyenne, z);

    (*nombre)++;
    return ETAT_OK;
}

Etat detecter_valeurs_aberrantes(Session *session, idperiodefiscale id_periode, const config_detection *cfg, Resultat_Detection **resultats, size_t *nombre_resultats) {
    size_t nombre_ecritures;
    size_t i, j, debut, fin, k;
    ligne_stat *lignes = NULL;
    size_t nb_lignes = 0, capacite_lignes = 0, capacite_resultats = 0;
    double *valeurs = NULL;   /* montants d'UN groupe (un compte), pour moyenne / ecart-type */
    Etat etat;

    if (session == NULL || cfg == NULL || resultats == NULL || nombre_resultats == NULL) {
        return ERR_POINTEUR_NULLE;
    }

    *resultats = NULL;
    *nombre_resultats = 0;

    if (cfg->seuil_ecart_type_extreme <= 0.0) {
        return ERR_CONFIGURATION_DETECTION_INVALIDE;
    }

    /* 1. mise a plat des lignes de la periode */
    nombre_ecritures = Session_GetEcritureCount(session);
    for (i = 0; i < nombre_ecritures; i++) {
        const Ecriture *ecriture = Session_avoir_EcritureAt(session, i);

        if (ecriture == NULL || ecriture->periode_id != id_periode || !ecriture->est_validee) {
            continue;
        }

        for (j = 0; j < ecriture->nombre_lignes; j++) {
            const ligne_journal *ligne = &ecriture->lignes[j];
            Monnaie montant = (ligne->Debit != 0) ? ligne->Debit : ligne->credit;
            ligne_stat st;

            if (montant == 0) continue;

            memset(&st, 0, sizeof(st));
            st.compte_id = ligne->compte_id;
            st.ligne_id = ligne->id;
            st.montant = (double)montant / (double)Monnaie_Unite;
            snprintf(st.reference, sizeof(st.reference), "%s", ecriture->reference);

            etat = ajouter_ligne_stat(&lignes, &nb_lignes, &capacite_lignes, &st);
            if (etat != ETAT_OK) {
                free(lignes);
                return etat;
            }
        }
    }

    if (nb_lignes < ABERRANTES_TAILLE_GROUPE_MIN) {
        free(lignes);
        return ETAT_OK;   /* pas assez de donnees : ce n'est pas une erreur */
    }

    /* 2. tri par compte */
    qsort(lignes, nb_lignes, sizeof(ligne_stat), comparer_par_compte);

    valeurs = malloc(nb_lignes * sizeof(double));
    if (valeurs == NULL) {
        free(lignes);
        return ERR_SORTIE_DU_MEMOIRE;
    }

    /* 3. un groupe = toutes les lignes consecutives du meme compte */
    debut = 0;
    while (debut < nb_lignes) {
        size_t taille;
        double moyenne, sigma;

        fin = debut;
        while (fin < nb_lignes && lignes[fin].compte_id == lignes[debut].compte_id) {
            fin++;
        }
        taille = fin - debut;

        if (taille >= ABERRANTES_TAILLE_GROUPE_MIN) {
            for (k = 0; k < taille; k++) {
                valeurs[k] = lignes[debut + k].montant;
            }
            moyenne = math_moyenne(valeurs, taille);
            sigma = math_ecart_type(valeurs, taille);

            /* sigma == 0 : tous les montants sont identiques, aucune valeur ne sort du lot */
            if (sigma > 0.0) {
                for (k = 0; k < taille; k++) {
                    double z = fabs(lignes[debut + k].montant - moyenne) / sigma;

                    if (z >= cfg->seuil_ecart_type_extreme) {
                        etat = ajouter_resultat_aberrant(resultats, nombre_resultats, &capacite_resultats, &lignes[debut + k], z, moyenne, cfg->seuil_ecart_type_extreme);
                        if (etat != ETAT_OK) {
                            free(valeurs);
                            free(lignes);
                            free(*resultats);
                            *resultats = NULL;
                            *nombre_resultats = 0;
                            return etat;
                        }
                    }
                }
            }
        }
        debut = fin;
    }

    free(valeurs);
    free(lignes);
    return ETAT_OK;
}
