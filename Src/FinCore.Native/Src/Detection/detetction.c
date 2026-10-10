/*
 * detection.c
 * -----------
 * Chef d'orchestre du moteur de détection.
 *
 * Ce fichier ne fait PAS les calculs statistiques lui-même : il appelle
 * les 4 méthodes de détection (chacune dans son propre fichier : benford.c,
 * doublons.c, montants_ronds.c, valeurs_aberrantes.c), recupere leurs
 * résultats, et les fusionne en un seul tableau qu'il renvoie à l'appelant.
 *
 * Chaque méthode alloue son propre petit tableau de résultats ; ce fichier
 * les recolle ensemble avec realloc() au fur et à mesure.
 */

#include <stdlib.h>  /* malloc, realloc, free pour l allocation dynamique */
#include <string.h> /* pour l usage du memcpy function pour le copiage du un bloc de memoire */


#include "Src/FinCore.Native/include/types.h"
#include "Src/FinCore.Native/include/errors.h"
#include "Src/FinCore.Native/include/session.h"
#include "Src/FinCore.Native/include/detection.h"

/* ---------------------------------------------------------------------------------------------------------------------
 Methodes de detection individuelles                            
Chacune est implemente dans son propre fichier et respecte le
meme contrat que "execute_detection" - fonction :: son idee : elle alloue son tableau en
interne (via malloc/realloc) et le laisse a NULL / 0 en cas d'erreur.

Elles ne font pas partie de l'API publique de la DLL -> donc elles ne sont pas declares dans detection.h, seulement ici 
----------------------------------------------------------------------------------------------------------------------- */

Etat detecter_benford_anomalies(Session *session, idperiodefiscale id_periode, const config_detection *cfg, Resultat_Detection **resultats, size_t *nombre_resultats);

Etat detecter_doublons(Session *session, idperiodefiscale id_periode, const config_detection *cfg, Resultat_Detection **resultats, size_t *nombre_resultats);

Etat detecter_montants_ronds(Session *session, idperiodefiscale id_periode, const config_detection *cfg, Resultat_Detection **resultats, size_t *nombre_resultats);

Etat detecter_valeurs_aberrantes(Session *session, idperiodefiscale id_periode, const config_detection *cfg, Resultat_Detection **resultats, size_t *nombre_resultats);

/* Calcule le rapport statistique complet de Benford (va etre codee dans benford.c) */
Etat calculer_rapport_benford(Session *session, idperiodefiscale id_periode, Rapport_Benford *rapport);

/* ------------------------------------------------------------------ 
                 Fonctions internes ( static functions)                                           
 ------------------------------------------------------------------ */

/*
 * Ajoute les résultats d'une méthode (source, nb_source) à la fin du
 * tableau final (dest, nb_dest), puis libère le petit tableau source
 * (on n'en a plus besoin une fois copié dans le tableau final).
 *
 * realloc() agrandit un bloc mémoire déjà alloué en gardant son contenu ;
 * s'il ne peut pas agrandir sur place, il déplace tout ailleurs et rend
 * la nouvelle adresse (c'est pour ça qu'on ne doit jamais écraser dest
 * avant d'avoir vérifié que realloc n'a pas renvoyé NULL).
 */

static Etat fusionner_resultats(Resultat_Detection **dest, size_t *nb_dest, Resultat_Detection *source, size_t nb_source) {
    if (nb_source == 0) {
        free(source); /* tableau vide éventuel : rien à copier, mais on le libère quand même */
        return ETAT_OK;
    }

    size_t nouvelle_taille = *nb_dest + nb_source;
    Resultat_Detection *nv_dest = realloc(*dest, nouvelle_taille * sizeof(Resultat_Detection));
    if (nv_dest == NULL) {
        free(source);
        return ERR_SORTIE_DU_MEMOIRE;
    }

    /* memcpy copie nb_source structures d'un coup, juste après ce qui existait déjà */
    memcpy(nv_dest + *nb_dest, source, nb_source * sizeof(Resultat_Detection));

    *dest = nv_dest;
    *nb_dest = nouvelle_taille;

    free(source); /* le petit tableau a été recopié, on n'en a plus besoin */
    return ETAT_OK;
}

/* liberer *resultats en cas d'erreur en cours de route, pour ne rien laisser de partiel. */
static void liberer_apres_erreur(Resultat_Detection **resultats, size_t *nombre_resultats) {
    /* liberation */
    /* initialisation a 0 / NULL */
    free(*resultats);
    *resultats = NULL;
    *nombre_resultats = 0;
}

 /* ------------------------------------------------------------------ 
                  API publique (declare en detection.)                  
 ------------------------------------------------------------------ */

void detection_config_par_defaut(config_detection* cfg) {
    /* voir documentation/detection/config_par_defaut.ipynb*/
    if (cfg == NULL) {
        return;
    }

    cfg->benford_est_active = 1;
    cfg->doublons_est_active = 1;
    cfg->nombres_ronds_active = 1;
    cfg->valeurs_aberrantes_active = 1;

    /* Au-delà de 3 écarts-types, un montant est jugé statistiquement extrême
       par rapport aux autres écritures du même compte. */
    cfg->seuil_ecart_type_extreme = 3.0;

    /* Un montant est jugé "rond" s'il est divisible par ce seuil (ex : 1000 DT). */
    cfg->seuil_ecart_nombres_ronds = 1000.0;

    /* Fenêtre de recherche des doublons : on compare chaque écriture aux
       autres écritures des 7 jours autour d'elle. */
    cfg->fenetre_jours_doublon = 7;
}

Etat execute_detection(Session* session, idperiodefiscale id_periode, const config_detection* cfg, Resultat_Detection** resultats, size_t* nombre_resultats) {
 
    if (session == NULL || cfg == NULL || resultats == NULL || nombre_resultats == NULL) {
        return ERR_POINTEUR_NULLE;
    }

    *resultats = NULL;
    *nombre_resultats = 0;

    /*
     * Table {drapeau actif, fonction a appeler} : ça evite de repeter 4 fois
     * le même bloc "si actif -> appeler -> fusionner -> vérifier l'erreur".
     */
    struct {
        int est_active;
        Etat (*executer)(Session*, idperiodefiscale, const config_detection*, Resultat_Detection**, size_t*);
    }
  
   methodes[] = {
        { cfg->benford_est_active , detecter_benford_anomalies },
        { cfg->doublons_est_active ,detecter_doublons },
        { cfg->nombres_ronds_active, detecter_montants_ronds },
        { cfg->valeurs_aberrantes_active, detecter_valeurs_aberrantes }
    };
 
    int nb_methodes = (int)(sizeof(methodes) / sizeof(methodes[0]));

    for (int i = 0; i < nb_methodes; i++) {
        if (!methodes[i].est_active) {
            continue;
        }

        Resultat_Detection *partiels = NULL;
        size_t nb_partiels = 0;

        Etat etat = methodes[i].executer(session, id_periode, cfg, &partiels, &nb_partiels);
        if (etat != ETAT_OK) {
            liberer_apres_erreur(resultats, nombre_resultats);
            return etat;
        }

        etat = fusionner_resultats(resultats, nombre_resultats, partiels, nb_partiels);
        if (etat != ETAT_OK) {
            liberer_apres_erreur(resultats, nombre_resultats);
            return etat;
        }
    }

    return ETAT_OK;
}

Etat Executer_Analyse_Benford(Session* session, idperiodefiscale id_periode, Rapport_Benford* rapport) {
    if (session == NULL || rapport == NULL) {
        return ERR_POINTEUR_NULLE;
    }

    /* Le calcul réel (extraction des premiers chiffres, comparaison à la loi
       de Benford, chi-carré) est fait dans le fichier  "benford.c" -- checkout  */
    return calculer_rapport_benford(session, id_periode, rapport);
}

void liberer_resultats_detection(Resultat_Detection* resultat, size_t nombre_resultats) {
    /*
     * Le tableau est un seul bloc alloué par malloc/realloc (voir
     * fusionner_resultats), donc un seul free() qui libere tout d'un coup.
     * nombre_resultats n'est pas utilise pour l'instant, on le garde dans
     * la signature pour rester fidele à l'API et pouvoir évoluer plus tard
     * (ex : si un jour un champ de Resultat_Detection est alloue à part).
     */
    (void)nombre_resultats; /* évite un warning -Wunused-parameter */

    free(resultat);
}
