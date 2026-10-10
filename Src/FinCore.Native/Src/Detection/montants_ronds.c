/*
 * montants_ronds.c
 * ----------------
 * Detection des montants ronds : une ligne dont le montant est un multiple
 * exact d'un seuil "rond" (par defaut 1000 DT) est un signal classique de
 * montant invente ou estime, plutot que d'un montant reel issu d'une facture
 * (les vraies factures finissent rarement en 000,000).
 *
 * Methode : on parcourt toutes les lignes des ecritures validees de la
 * periode. Une ligne est retenue si son montant est un multiple du seuil.
 * Plus le montant est un multiple "propre" (x10, x100 du seuil), plus la
 * gravite et le score montent.
 *
 * Un montant rond n'est pas une preuve de fraude : la gravite reste donc
 * FAIBLE pour le cas de base.
 */

#include <stdio.h>   /* snprintf : construire le texte de description */
#include <stdlib.h>  /* realloc : agrandir le tableau de resultats */
#include <string.h>  /* memset : remise a zero d'une structure */

#include "Src/FinCore.Native/include/types.h"
#include "Src/FinCore.Native/include/errors.h"
#include "Src/FinCore.Native/include/session.h"
#include "Src/FinCore.Native/include/detection.h"

/* Ajoute un resultat a la fin du tableau dynamique (capacite doublee quand elle est atteinte) */
static Etat ajouter_resultat_rond(Resultat_Detection **resultats, size_t *nombre, size_t *capacite, const Ecriture *ecriture, const ligne_journal *ligne, Monnaie montant, int niveau) {
    Resultat_Detection *r;

    if (*nombre == *capacite) {
        size_t nouvelle_capacite = (*capacite == 0) ? 16 : (*capacite * 2);
        Resultat_Detection *nv = realloc(*resultats, nouvelle_capacite * sizeof(Resultat_Detection));

        if (nv == NULL) {
            return ERR_SORTIE_DU_MEMOIRE;   /* l'ancien tableau reste valide, l'appelant le libere */
        }
        *resultats = nv;
        *capacite = nouvelle_capacite;
    }

    r = &(*resultats)[*nombre];
    memset(r, 0, sizeof(*r));
    r->methode = DETECT_NOMBRES_RONDS;

    /* niveau 0 = multiple du seuil, 1 = multiple de 10 x seuil, 2+ = multiple de 100 x seuil */
    r->gravite = (niveau >= 2) ? GRAVITE_ELEVEE : (niveau == 1) ? GRAVITE_MOYENNE : GRAVITE_FAIBLE;
    r->score = 0.5 + 0.25 * (double)(niveau > 2 ? 2 : niveau);   /* entre 0.5 et 1.0 */
    r->id_compte = ligne->compte_id;
    r->id_ligne = ligne->id;

    snprintf(r->description, sizeof(r->description),
        "Montant rond : %lld.%03lld sur le compte %u (ecriture %s)",
        (long long)(montant / Monnaie_Unite), (long long)(montant % Monnaie_Unite),
        (unsigned)ligne->compte_id, ecriture->reference);

    (*nombre)++;
    return ETAT_OK;
}

Etat detecter_montants_ronds(Session *session, idperiodefiscale id_periode, const config_detection *cfg, Resultat_Detection **resultats, size_t *nombre_resultats) {
    size_t nombre_ecritures;
    size_t i, j;
    size_t capacite = 0;
    Monnaie seuil;   /* seuil converti en unites mineures (millimes) */
    Etat etat;

    if (session == NULL || cfg == NULL || resultats == NULL || nombre_resultats == NULL) {
        return ERR_POINTEUR_NULLE;
    }

    *resultats = NULL;
    *nombre_resultats = 0;

    /* le seuil est exprime en DT dans la config : on le passe en millimes pour rester en entiers */
    seuil = (Monnaie)(cfg->seuil_ecart_nombres_ronds * (double)Monnaie_Unite);
    if (seuil <= 0) {
        return ERR_CONFIGURATION_DETECTION_INVALIDE;
    }

    nombre_ecritures = Session_GetEcritureCount(session);

    for (i = 0; i < nombre_ecritures; i++) {
        const Ecriture *ecriture = Session_avoir_EcritureAt(session, i);

        if (ecriture == NULL || ecriture->periode_id != id_periode || !ecriture->est_validee) {
            continue;
        }

        for (j = 0; j < ecriture->nombre_lignes; j++) {
            const ligne_journal *ligne = &ecriture->lignes[j];
            Monnaie montant = (ligne->Debit != 0) ? ligne->Debit : ligne->credit;
            int niveau = 0;
            Monnaie palier;

            if (montant <= 0 || montant % seuil != 0) {
                continue;   /* pas un multiple du seuil : montant "normal" */
            }

            /* on monte de niveau tant que le montant est aussi multiple de 10 x seuil, 100 x seuil... */
            palier = seuil;
            while (niveau < 3 && palier <= montant / 10 && montant % (palier * 10) == 0) {
                palier *= 10;
                niveau++;
            }

            etat = ajouter_resultat_rond(resultats, nombre_resultats, &capacite, ecriture, ligne, montant, niveau);
            if (etat != ETAT_OK) {
                free(*resultats);
                *resultats = NULL;
                *nombre_resultats = 0;
                return etat;
            }
        }
    }

    return ETAT_OK;
}
