#include <stdio.h>
#include <string.h>
#include "Src/FinCore.Native/include/periodes.h"


static bool statut_est_valide(status_periode_fiscale statut){
    return statut == PERIODE_FISCALE_OUVERTE  || statut == PERIODE_FISCALE_CLOTURE  || statut == PERIODE_FISCALE_VEROUILLEE;
}

bool periodes_plage_valide(DATE date_debut, DATE date_fin){
    return date_valide(date_debut) && date_valide(date_fin) && comparer_dates(date_debut, date_fin) <= 0;
}

bool periodes_est_valide(const prop_periode_fiscale *periode){
    if (periode == NULL) return false;
    if (periode->id == INVALID_ID) return false;
    if (periode->nom[0] == '\0') return false;
    if (memchr(periode->nom, '\0', sizeof(periode->nom)) == NULL) return false;
    if (!statut_est_valide(periode->statut)) return false;

    return periodes_plage_valide(periode->date_debut, periode->date_fin);
}

Etat periodes_creer(prop_periode_fiscale *periode, idperiodefiscale id,  const char *nom, DATE date_debut, DATE date_fin){
    size_t longueur;

    if (periode == NULL || nom == NULL)
        return ERR_POINTEUR_NULLE;
    if (id == INVALID_ID)
        return ERR_IDENTIFIANT_INVALIDE;

    longueur = strlen(nom);
    if (longueur == 0 || longueur >= sizeof(periode->nom))
        return ERR_ARGUMENT_INVALIDE;
    if (!periodes_plage_valide(date_debut, date_fin))
        return ERR_PLAGE_DATES_INVALIDE;

    memset(periode, 0, sizeof(*periode));
    periode->id = id;
    memcpy(periode->nom, nom, longueur + 1);
    periode->date_debut = date_debut;
    periode->date_fin = date_fin;
    periode->statut = PERIODE_FISCALE_OUVERTE;

    return ETAT_OK;
}

Etat periodes_generer_mensuelles(int32_t annee, idperiodefiscale premier_id,prop_periode_fiscale *sortie, size_t capacite){
    prop_periode_fiscale tampon[PERIODES_PAR_ANNEE];
    int32_t mois;
    Etat etat;

    if (sortie == NULL)
        return ERR_POINTEUR_NULLE;
    if (capacite < PERIODES_PAR_ANNEE)
        return ERR_TRES_PETIT_BUFFER;
    if (premier_id == INVALID_ID || premier_id > UINT32_MAX - (PERIODES_PAR_ANNEE - 1))
        return ERR_IDENTIFIANT_INVALIDE;
    if (!date_valide((DATE){ .annee = annee, .mois = 1, .jour = 1 }))
        return ERR_ARGUMENT_INVALIDE;

    /* Construction dans un tampon local : sortie n'est écrite qu'en cas de succès complet */
    for (mois = 1; mois <= PERIODES_PAR_ANNEE; mois++) {
        char nom[32];
        DATE debut = { .annee = annee, .mois = mois, .jour = 1 };
        DATE fin   = { .annee = annee, .mois = mois, .jour = jours_dans_mois(mois, annee) };

        snprintf(nom, sizeof(nom), "%04d-%02d", (int)annee, (int)mois);

        etat = periodes_creer(&tampon[mois - 1], premier_id + (idperiodefiscale)(mois - 1), nom, debut, fin);
        if (etat != ETAT_OK)
            return etat;
    }

    memcpy(sortie, tampon, sizeof(tampon));
    return ETAT_OK;
}

/* ------------------------------------------------------------
 * Consultation
 * ------------------------------------------------------------ */

bool periodes_est_ouverte(const prop_periode_fiscale *periode){
    return periode != NULL && periode->statut == PERIODE_FISCALE_OUVERTE;
}

bool periodes_est_cloturee(const prop_periode_fiscale *periode){
    return periode != NULL && periode->statut == PERIODE_FISCALE_CLOTURE;
}

bool periodes_est_verrouillee(const prop_periode_fiscale *periode){
    return periode != NULL && periode->statut == PERIODE_FISCALE_VEROUILLEE;
}

bool periodes_contient_date(const prop_periode_fiscale *periode, DATE date){
    if (periode == NULL || !date_valide(date))
        return false;

    return comparer_dates(date, periode->date_debut) >= 0
        && comparer_dates(date, periode->date_fin) <= 0;
}

/* Deux intervalles fermés se chevauchent ssi chacun commence avant la fin de l'autre */
bool periodes_se_chevauchent(const prop_periode_fiscale *a, const prop_periode_fiscale *b){
  
    if (a == NULL || b == NULL)
        return false;

    return comparer_dates(a->date_debut, b->date_fin) <= 0
        && comparer_dates(b->date_debut, a->date_fin) <= 0;
}

/* Nombre de jours, bornes incluses ; 0 si la période est invalide */
int32_t periodes_duree_jours(const prop_periode_fiscale *periode){
  
    if (periode == NULL || !periodes_plage_valide(periode->date_debut, periode->date_fin))
        return 0;

    return date_en_jdn(periode->date_fin) - date_en_jdn(periode->date_debut) + 1;
}

/* ------------------------------------------------------------
 * Cycle de vie : OUVERTE <--> CLOTURE --> VEROUILLEE
 * ------------------------------------------------------------ */

Etat periodes_cloturer(prop_periode_fiscale *periode){
    if (periode == NULL)
        return ERR_POINTEUR_NULLE;

    switch (periode->statut) {
    case PERIODE_FISCALE_OUVERTE:
        periode->statut = PERIODE_FISCALE_CLOTURE;
        return ETAT_OK;
    case PERIODE_FISCALE_CLOTURE:
        return ERR_PERIODE_FERMEE;
    case PERIODE_FISCALE_VEROUILLEE:
        return ERR_PERIODE_VERROUILLEE;
    default:
        return ERR_ARGUMENT_INVALIDE;
    }
}

Etat periodes_rouvrir(prop_periode_fiscale *periode){
    if (periode == NULL)
        return ERR_POINTEUR_NULLE;

    switch (periode->statut) {
    case PERIODE_FISCALE_CLOTURE:
        periode->statut = PERIODE_FISCALE_OUVERTE;
        return ETAT_OK;
    case PERIODE_FISCALE_OUVERTE:
        return ERR_OPERATION_INTERDITE;     /* déjà ouverte */
    case PERIODE_FISCALE_VEROUILLEE:
        return ERR_PERIODE_VERROUILLEE;
    default:
        return ERR_ARGUMENT_INVALIDE;
    }
}

Etat periodes_verrouiller(prop_periode_fiscale *periode){
    if (periode == NULL)
        return ERR_POINTEUR_NULLE;

    switch (periode->statut) {
    case PERIODE_FISCALE_CLOTURE:
        periode->statut = PERIODE_FISCALE_VEROUILLEE;
        return ETAT_OK;
    case PERIODE_FISCALE_OUVERTE:
        return ERR_OPERATION_INTERDITE;     /* il faut d'abord clôturer */
    case PERIODE_FISCALE_VEROUILLEE:
        return ERR_PERIODE_VERROUILLEE;
    default:
        return ERR_ARGUMENT_INVALIDE;
    }
}


Etat periodes_ajouter(prop_periode_fiscale *periodes, size_t *nombre, size_t capacite, const prop_periode_fiscale *nouvelle){
    size_t i;

    if (periodes == NULL || nombre == NULL || nouvelle == NULL)
        return ERR_POINTEUR_NULLE;
    if (!periodes_plage_valide(nouvelle->date_debut, nouvelle->date_fin))
        return ERR_PLAGE_DATES_INVALIDE;
    if (!periodes_est_valide(nouvelle))
        return ERR_ARGUMENT_INVALIDE;

    for (i = 0; i < *nombre; i++) {
        if (periodes[i].id == nouvelle->id)
            return ERR_PERIODE_EXISTE;
        if (periodes_se_chevauchent(&periodes[i], nouvelle))
            return ERR_PERIODE_CHEVAUCHEMENT;
    }

    if (*nombre >= capacite)
        return ERR_TRES_PETIT_BUFFER;

    periodes[*nombre] = *nouvelle;
    (*nombre)++;

    return ETAT_OK;
}

Etat periodes_trouver_par_id(const prop_periode_fiscale *periodes, size_t nombre, idperiodefiscale id, size_t *indice_out){
    size_t i;

    if (periodes == NULL || indice_out == NULL)
        return ERR_POINTEUR_NULLE;
    if (id == INVALID_ID)
        return ERR_IDENTIFIANT_INVALIDE;

    for (i = 0; i < nombre; i++) {
        if (periodes[i].id == id) {
            *indice_out = i;
            return ETAT_OK;
        }
    }

    return ERR_PERIODE_INTROUVABLE;
}

Etat periodes_trouver_par_date(const prop_periode_fiscale *periodes, size_t nombre, DATE date, size_t *indice_out){
    size_t i;

    if (periodes == NULL || indice_out == NULL)
        return ERR_POINTEUR_NULLE;
    if (!date_valide(date))
        return ERR_ARGUMENT_INVALIDE;

    for (i = 0; i < nombre; i++) {
        if (periodes_contient_date(&periodes[i], date)) {
            *indice_out = i;
            return ETAT_OK;
        }
    }

    return ERR_PERIODE_INTROUVABLE;
}

/* ------------------------------------------------------------
 * Contrôle avant comptabilisation
 * ------------------------------------------------------------ */

Etat periodes_verifier_ecriture(const prop_periode_fiscale *periodes, size_t nombre, const Ecriture *ecriture){
    const prop_periode_fiscale *periode;
    size_t indice;
    Etat etat;

    if (periodes == NULL || ecriture == NULL)
        return ERR_POINTEUR_NULLE;

    etat = periodes_trouver_par_id(periodes, nombre, ecriture->periode_id, &indice);
    if (etat != ETAT_OK)
        return etat;

    periode = &periodes[indice];

    switch (periode->statut) {
    case PERIODE_FISCALE_OUVERTE:
        break;
    case PERIODE_FISCALE_CLOTURE:
        return ERR_PERIODE_FERMEE;
    case PERIODE_FISCALE_VEROUILLEE:
        return ERR_PERIODE_VERROUILLEE;
    default:
        return ERR_ARGUMENT_INVALIDE;
    }

    if (!periodes_contient_date(periode, ecriture->date))
        return ERR_TRANSACTION_HORS_PERIODE;

    return ETAT_OK;
}

