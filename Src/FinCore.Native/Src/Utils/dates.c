#include "Src/FinCore.Native/include/dates.h"

bool est_annee_bissextile(int32_t annee){
    return (annee % 4 == 0 && annee % 100 != 0) || (annee % 400 == 0);
}

int32_t jours_dans_mois(int32_t mois,int32_t annee){
    switch(mois){
        case 1:
        case 3:
        case 5:
        case 7:
        case 8:
        case 10:
        case 12:
            return 31; 
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            return 30;
            break;
        case 2 : 
            return est_annee_bissextile(annee) ? FEVRIER_JOURS_BISSEXTILE : FEVRIER_JOURS_NORMAL;
            break;
        default :
            return 0 ;
            break;
    }
}        

bool date_valide(DATE d){
    if (d.jour < 1 || d.mois < 1 || d.mois > 12) return false;

    /* mois à 31 jours */ 
    if (d.jour > 31 && (d.mois == 1 || d.mois == 3 || d.mois == 5 || d.mois == 7 || d.mois == 8 || d.mois == 10 || d.mois == 12))
        return false;

    /* mois à 30 jours */
    if (d.jour > 30 && (d.mois == 4 || d.mois == 6 || d.mois == 9 || d.mois == 11))
        return false;

    /*fevrier : le cas particulier*/
    /*l'apppel du la fonction du l'annee bissextile*/
    if (d.mois == 2) {
        int max_jour_en_fevrier = est_annee_bissextile(d.annee)? FEVRIER_JOURS_BISSEXTILE : FEVRIER_JOURS_NORMAL;
        if (d.jour > max_jour_en_fevrier)
            return false;
    }
    return true ; /*date valide et propre*/
}

int comparer_dates(DATE d1, DATE d2){
    /* le type d'output du la fonction comparer_dates est 'integer' : 
        c'est le cas ou ::    la fonction retourne -2 , si la date est invalide 
                              la fonction retourne -1 si la 2eme date est superieure que la premiere et 1 en inverse
                              sinon la foction retourne 0 :: si les deux dates sont identiques*/
    
    if (!date_valide(d1) || !date_valide(d2)) return -2;

    if (d1.annee != d2.annee) return (d1.annee < d2.annee) ? -1 : 1;

    if (d1.mois != d2.mois) return (d1.mois < d2.mois) ? -1 : 1;

    if (d1.jour != d2.jour) return (d1.jour < d2.jour) ? -1 : 1;

    return 0 ; /*d1 et d2 sont identiques*/
}

bool date_inclus_dans_periode(DATE d, DATE debut_periode, DATE fin_periode) {
    /* vrai si debut_periode <= d <= fin_periode (bornes incluses) */
    if (!date_valide(d) || !date_valide(debut_periode) || !date_valide(fin_periode)) return false;

    /* une periode a l'envers (debut apres fin) n'a aucune date */
    if (comparer_dates(debut_periode, fin_periode) > 0) return false;

    return comparer_dates(d, debut_periode) >= 0 && comparer_dates(d, fin_periode) <= 0;
}

DATE ajouter_jours_au_date(DATE d, int32_t jours) {
    /* on passe par le numero de jour julien : l'addition devient simple
     * et les changements de mois / d'annee sont geres automatiquement */
    if (!date_valide(d)) return d;

    return jdn_en_date(date_en_jdn(d) + jours);
}

DATE difference_date(DATE d1, DATE d2) {
    if (!date_valide(d1) || !date_valide(d2)) return DATE_ERREUR;

    int32_t cmp = comparer_dates(d1, d2);
    if (cmp == 0) return (DATE){ .annee = 0, .mois = 0, .jour = 0 };

    DATE date_recente  = (cmp == 1) ? d1 : d2;
    DATE date_ancienne = (cmp == 1) ? d2 : d1;

    int32_t jours  = date_recente.jour  - date_ancienne.jour;
    int32_t mois   = date_recente.mois  - date_ancienne.mois;
    int32_t annees = date_recente.annee - date_ancienne.annee;

    /* emprunt : si les jours sont negatifs on prend les jours du mois precedent */
    if (jours < 0) {
        int32_t mois_prec = date_recente.mois - 1;
        int32_t annee_du_mois_prec = date_recente.annee;
        if (mois_prec == 0) {
            mois_prec = 12;
            annee_du_mois_prec--;
        }
        jours += jours_dans_mois(mois_prec, annee_du_mois_prec);
        mois--;
    }

    /* meme principe pour les mois negatifs : on emprunte une annee */
    if (mois < 0) {
        mois += 12;
        annees--;
    }

    return (DATE){ .annee = annees, .mois = mois, .jour = jours };
}

int32_t date_en_jdn(DATE d) {
    /* formule classique du calendrier gregorien -> numero de jour julien */
    int32_t a = (14 - d.mois) / 12;
    int32_t y = d.annee + 4800 - a;
    int32_t m = d.mois + 12 * a - 3;

    return d.jour
           + (153 * m + 2) / 5
           + 365 * y
           + y / 4
           - y / 100
           + y / 400
           - 32045;
}

DATE jdn_en_date(int32_t jdn) {
    /* operation inverse de date_en_jdn */
    DATE d;
    int32_t a = jdn + 32044;
    int32_t b = (4 * a + 3) / 146097;
    int32_t c = a - (146097 * b) / 4;
    int32_t e = (4 * c + 3) / 1461;
    int32_t f = c - (1461 * e) / 4;
    int32_t m = (5 * f + 2) / 153;

    d.jour  = f - (153 * m + 2) / 5 + 1;
    d.mois  = m + 3 - 12 * (m / 10);
    d.annee = 100 * b + e - 4800 + (m / 10);

    return d;
}

bool analyser_date(const char *date_en_texte, DATE_FORMAT format, DATE *resultat) {
    int jj = 0, mm = 0, aaaa = 0;
    int nb_lus;   /* nombre de nombres lus par sscanf */
    DATE candidate;

    if (date_en_texte == NULL || resultat == NULL) return false;

    switch (format) {
        case DATE_FORMAT_JJMMAAAA:
            nb_lus = sscanf(date_en_texte, "%d/%d/%d", &jj, &mm, &aaaa);
            break;
        case DATE_FORMAT_MMJJAAAA:
            nb_lus = sscanf(date_en_texte, "%d/%d/%d", &mm, &jj, &aaaa);
            break;
        case DATE_FORMAT_ISO:
            nb_lus = sscanf(date_en_texte, "%d-%d-%d", &aaaa, &mm, &jj);
            break;
        default:
            return false;
    }

    if (nb_lus != 3) return false;

    candidate.jour  = jj;
    candidate.mois  = mm;
    candidate.annee = aaaa;

    /* date_valide fait deja toutes les verifications (mois, jours, bissextile) */
    if (!date_valide(candidate)) return false;

    *resultat = candidate;
    return true;
}

bool formater_date(DATE date, DATE_FORMAT format, char *buffer, int32_t taille_buffer) {
    int ecrit;

    if (buffer == NULL || taille_buffer <= 0) return false;
    if (!date_valide(date)) return false;

    switch (format) {
        case DATE_FORMAT_JJMMAAAA:
            ecrit = snprintf(buffer, (size_t)taille_buffer, "%02d/%02d/%04d", date.jour, date.mois, date.annee);
            break;
        case DATE_FORMAT_MMJJAAAA:
            ecrit = snprintf(buffer, (size_t)taille_buffer, "%02d/%02d/%04d", date.mois, date.jour, date.annee);
            break;
        case DATE_FORMAT_ISO:
            ecrit = snprintf(buffer, (size_t)taille_buffer, "%04d-%02d-%02d", date.annee, date.mois, date.jour);
            break;
        default:
            return false;
    }

    /* snprintf renvoie la taille voulue : si elle depasse le buffer, le texte est tronque */
    if (ecrit < 0 || ecrit >= taille_buffer) return false;

    return true;
}
