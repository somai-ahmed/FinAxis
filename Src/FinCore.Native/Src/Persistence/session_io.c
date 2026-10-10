/*
 * session_io.c -- implementation de la Session du moteur FinCore
 *
 * La Session est la "memoire" du moteur pour UNE entreprise :
 * elle garde le plan comptable, le journal et la configuration.
 * La structure est definie ici seulement (opaque) : le reste du code,
 * et Python via ctypes, ne voit qu'un pointeur Session*.
 */


#include <stdlib.h>  /* pour calloc/realloc/free : allocation dynamique */
#include <string.h>  /* pour memcpy/strncpy/strcmp : copier et comparer */

#include "Src/FinCore.Native/include/cJSON.h"
#include "Src/FinCore.Native/include/session.h"
#include "Src/FinCore.Native/include/comptes.h"
#include "Src/FinCore.Native/include/ecritures.h"


/* ------------------------------------------------------------------
                      OUTILS D'ECRITURE JSON
 ------------------------------------------------------------------ */

/* ajoute "cle": "AAAA-MM-JJ" (ou "" si la date n'est pas valide, ex. config vide) */
static void json_ajouter_date(cJSON *objet, const char *cle, DATE date) {
    char texte[LONGEUR_CHAINE_DATE + 1];

    if (formater_date(date, DATE_FORMAT_ISO, texte, (int32_t)sizeof(texte))) {
        cJSON_AddStringToObject(objet, cle, texte);
    } else {
        cJSON_AddStringToObject(objet, cle, "");
    }
}

static cJSON *json_construire(const Session *session) {
    cJSON *racine = cJSON_CreateObject();
    cJSON *config, *comptes, *periodes, *ecritures;
    const SessionConfig *cfg = Session_avoir_Config(session);
    size_t i, j;

    if (racine == NULL) return NULL;

    cJSON_AddStringToObject(racine, "format", FNC_FORMAT_NOM);
    cJSON_AddNumberToObject(racine, "version", FNC_FORMAT_VERSION);

    config = cJSON_AddObjectToObject(racine, "config");
    cJSON_AddStringToObject(config, "nom_entreprise", cfg->nom_entreprise);
    cJSON_AddStringToObject(config, "devise", cfg->devise);
    json_ajouter_date(config, "debut_exercice", cfg->debut_exercice);
    json_ajouter_date(config, "fin_exercice", cfg->fin_exercice);

    comptes = cJSON_AddArrayToObject(racine, "comptes");
    for (i = 0; i < Session_avoir_nombre_comptes(session); i++) {
        const Compte *c = Session_avoir_CompteAt(session, i);
        cJSON *o = cJSON_CreateObject();

        cJSON_AddNumberToObject(o, "id", c->id);
        cJSON_AddStringToObject(o, "code", c->code);
        cJSON_AddStringToObject(o, "nom", c->nom);
        cJSON_AddNumberToObject(o, "classe", c->classe);
        cJSON_AddNumberToObject(o, "type", c->type);
        cJSON_AddNumberToObject(o, "solde_normal", c->solde_normal);
        cJSON_AddNumberToObject(o, "parent_id", c->parent_id);
        cJSON_AddNumberToObject(o, "est_active", c->est_active);
        cJSON_AddItemToArray(comptes, o);
    }

    periodes = cJSON_AddArrayToObject(racine, "periodes");
    for (i = 0; i < Session_avoir_nombre_periodes(session); i++) {
        const prop_periode_fiscale *p = Session_avoir_PeriodeAt(session, i);
        cJSON *o = cJSON_CreateObject();

        cJSON_AddNumberToObject(o, "id", p->id);
        cJSON_AddStringToObject(o, "nom", p->nom);
        json_ajouter_date(o, "date_debut", p->date_debut);
        json_ajouter_date(o, "date_fin", p->date_fin);
        cJSON_AddNumberToObject(o, "statut", p->statut);
        cJSON_AddItemToArray(periodes, o);
    }

    ecritures = cJSON_AddArrayToObject(racine, "ecritures");
    for (i = 0; i < Session_GetEcritureCount(session); i++) {
        const Ecriture *e = Session_avoir_EcritureAt(session, i);
        cJSON *o = cJSON_CreateObject();
        cJSON *lignes;

        cJSON_AddNumberToObject(o, "id", e->id);
        cJSON_AddNumberToObject(o, "periode_id", e->periode_id);
        json_ajouter_date(o, "date", e->date);
        cJSON_AddStringToObject(o, "reference", e->reference);
        cJSON_AddStringToObject(o, "description", e->description);
        cJSON_AddNumberToObject(o, "est_validee", e->est_validee);

        lignes = cJSON_AddArrayToObject(o, "lignes");
        for (j = 0; j < e->nombre_lignes; j++) {
            const ligne_journal *l = &e->lignes[j];
            cJSON *lo = cJSON_CreateObject();

            cJSON_AddNumberToObject(lo, "id", l->id);
            cJSON_AddNumberToObject(lo, "compte_id", l->compte_id);
            cJSON_AddNumberToObject(lo, "debit", (double)l->Debit);
            cJSON_AddNumberToObject(lo, "credit", (double)l->credit);
            cJSON_AddStringToObject(lo, "libelle", l->libelle);
            cJSON_AddItemToArray(lignes, lo);
        }
        cJSON_AddItemToArray(ecritures, o);
    }

    return racine;
}

/* ------------------------------------------------------------------
                      OUTILS DE LECTURE JSON
 ------------------------------------------------------------------ */

/* lit un nombre entier ; false si la cle manque ou n'est pas un nombre */
static bool json_lire_entier(const cJSON *objet, const char *cle, double *valeur) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(objet, cle);

    if (!cJSON_IsNumber(item)) return false;
    *valeur = item->valuedouble;
    return true;
}

/* copie un texte JSON dans un tampon de taille fixe ; false si la cle manque */
static bool json_lire_texte(const cJSON *objet, const char *cle, char *destination, size_t taille) {
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(objet, cle);

    if (!cJSON_IsString(item) || item->valuestring == NULL) return false;
    snprintf(destination, taille, "%s", item->valuestring);
    return true;
}

/* lit une date ISO ; une chaine vide donne une date a zero (config non renseignee) */
static bool json_lire_date(const cJSON *objet, const char *cle, DATE *date) {
    char texte[32];

    if (!json_lire_texte(objet, cle, texte, sizeof(texte))) return false;
    if (texte[0] == '\0') {
        memset(date, 0, sizeof(*date));
        return true;
    }
    return analyser_date(texte, DATE_FORMAT_ISO, date);
}

/* ------------------------------------------------------------------
                  RECONSTRUCTION D'UNE SESSION
 ------------------------------------------------------------------ */

static Etat charger_depuis_json(const cJSON *racine, Session **out_session) {
    const cJSON *item, *tableau, *o;
    Session *session = NULL;
    SessionConfig cfg;
    double v;
    Etat etat;
    char texte[64];
    size_t nb_periodes_json = 0;
    /* statuts a appliquer APRES les ecritures (une periode cloturee refuserait leur chargement) */
    status_periode_fiscale *statuts = NULL;
    idperiodefiscale *ids_periodes = NULL;
    size_t k;

    if (!json_lire_texte(racine, "format", texte, sizeof(texte)) || strcmp(texte, FNC_FORMAT_NOM) != 0) {
        return ERR_FICHIER_CORROMPU;
    }
    if (!json_lire_entier(racine, "version", &v)) return ERR_FICHIER_CORROMPU;
    if ((int)v != FNC_FORMAT_VERSION) return ERR_VERSION_NON_SUPPORTEE;

    /* config */
    item = cJSON_GetObjectItemCaseSensitive(racine, "config");
    if (!cJSON_IsObject(item)) return ERR_FICHIER_CORROMPU;
    memset(&cfg, 0, sizeof(cfg));
    if (!json_lire_texte(item, "nom_entreprise", cfg.nom_entreprise, sizeof(cfg.nom_entreprise)) ||
        !json_lire_texte(item, "devise", cfg.devise, sizeof(cfg.devise)) ||
        !json_lire_date(item, "debut_exercice", &cfg.debut_exercice) ||
        !json_lire_date(item, "fin_exercice", &cfg.fin_exercice)) {
        return ERR_FICHIER_CORROMPU;
    }

    etat = creer_session(&cfg, &session);
    if (etat != ETAT_OK) return etat;

    /* comptes : parents avant enfants dans le fichier (c'est l'ordre dans lequel on les sauvegarde) */
    tableau = cJSON_GetObjectItemCaseSensitive(racine, "comptes");
    if (!cJSON_IsArray(tableau)) { etat = ERR_FICHIER_CORROMPU; goto echec; }
    cJSON_ArrayForEach(o, tableau) {
        Compte c;
        double classe, type, sens, parent, actif, id;

        memset(&c, 0, sizeof(c));
        if (!json_lire_entier(o, "id", &id) || !json_lire_texte(o, "code", c.code, sizeof(c.code)) ||
            !json_lire_texte(o, "nom", c.nom, sizeof(c.nom)) || !json_lire_entier(o, "classe", &classe) ||
            !json_lire_entier(o, "type", &type) || !json_lire_entier(o, "solde_normal", &sens) ||
            !json_lire_entier(o, "parent_id", &parent) || !json_lire_entier(o, "est_active", &actif)) {
            etat = ERR_FICHIER_CORROMPU;
            goto echec;
        }
        c.id = (id_compte)id;
        c.classe = (ClasseCompte)(int)classe;
        c.type = (TypeCompte)(int)type;
        c.solde_normal = (SoldeNormal)(int)sens;
        c.parent_id = (id_compte)parent;
        c.est_active = (int)actif;
        c.solde = 0;   /* recalcule en rejouant les ecritures */

        etat = Session_ajouterCompte(session, &c);
        if (etat != ETAT_OK) goto echec;
    }

    /* periodes : ajoutees OUVERTES, le vrai statut est applique a la fin */
    tableau = cJSON_GetObjectItemCaseSensitive(racine, "periodes");
    if (!cJSON_IsArray(tableau)) { etat = ERR_FICHIER_CORROMPU; goto echec; }
    nb_periodes_json = (size_t)cJSON_GetArraySize(tableau);
    if (nb_periodes_json > 0) {
        statuts = malloc(nb_periodes_json * sizeof(*statuts));
        ids_periodes = malloc(nb_periodes_json * sizeof(*ids_periodes));
        if (statuts == NULL || ids_periodes == NULL) { etat = ERR_SORTIE_DU_MEMOIRE; goto echec; }
    }
    k = 0;
    cJSON_ArrayForEach(o, tableau) {
        prop_periode_fiscale p;
        double id, statut;

        memset(&p, 0, sizeof(p));
        if (!json_lire_entier(o, "id", &id) || !json_lire_texte(o, "nom", p.nom, sizeof(p.nom)) ||
            !json_lire_date(o, "date_debut", &p.date_debut) || !json_lire_date(o, "date_fin", &p.date_fin) ||
            !json_lire_entier(o, "statut", &statut)) {
            etat = ERR_FICHIER_CORROMPU;
            goto echec;
        }
        p.id = (idperiodefiscale)id;
        p.statut = PERIODE_FISCALE_OUVERTE;
        statuts[k] = (status_periode_fiscale)(int)statut;
        ids_periodes[k] = p.id;
        k++;

        etat = Session_ajouterPeriode(session, &p);
        if (etat != ETAT_OK) goto echec;
    }

    /* ecritures : stockees en brouillon puis comptabilisees si elles l'etaient (soldes recalcules) */
    tableau = cJSON_GetObjectItemCaseSensitive(racine, "ecritures");
    if (!cJSON_IsArray(tableau)) { etat = ERR_FICHIER_CORROMPU; goto echec; }
    cJSON_ArrayForEach(o, tableau) {
        Ecriture e;
        const cJSON *lignes, *lo;
        double id, periode, validee;
        bool ligne_ok = true;

        memset(&e, 0, sizeof(e));
        if (!json_lire_entier(o, "id", &id) || !json_lire_entier(o, "periode_id", &periode) ||
            !json_lire_date(o, "date", &e.date) || !json_lire_texte(o, "reference", e.reference, sizeof(e.reference)) ||
            !json_lire_texte(o, "description", e.description, sizeof(e.description)) ||
            !json_lire_entier(o, "est_validee", &validee)) {
            etat = ERR_FICHIER_CORROMPU;
            goto echec;
        }
        e.id = (IdEcriture)id;
        e.periode_id = (idperiodefiscale)periode;

        lignes = cJSON_GetObjectItemCaseSensitive(o, "lignes");
        if (!cJSON_IsArray(lignes)) { etat = ERR_FICHIER_CORROMPU; goto echec; }
        cJSON_ArrayForEach(lo, lignes) {
            double cid, debit, credit;
            char libelle[256];

            if (!json_lire_entier(lo, "compte_id", &cid) || !json_lire_entier(lo, "debit", &debit) ||
                !json_lire_entier(lo, "credit", &credit) || !json_lire_texte(lo, "libelle", libelle, sizeof(libelle))) {
                ligne_ok = false;
                break;
            }
            etat = ecritures_ajouter_ligne(&e, (id_compte)cid, (Monnaie)debit, (Monnaie)credit, libelle);
            if (etat != ETAT_OK) { ligne_ok = false; break; }
        }
        if (!ligne_ok) {
            ecritures_detruire(&e);
            if (etat == ETAT_OK) etat = ERR_FICHIER_CORROMPU;
            goto echec;
        }

        e.est_validee = (int)validee;
        etat = Session_AddEcriture(session, &e);
        ecritures_detruire(&e);   /* la session a sa propre copie */
        if (etat != ETAT_OK) goto echec;
    }

    /* statuts des periodes : OUVERTE <-> CLOTURE --> VEROUILLEE */
    for (k = 0; k < nb_periodes_json; k++) {
        if (statuts[k] == PERIODE_FISCALE_CLOTURE || statuts[k] == PERIODE_FISCALE_VEROUILLEE) {
            etat = Session_cloturerPeriode(session, ids_periodes[k]);
            if (etat != ETAT_OK) goto echec;
        }
        if (statuts[k] == PERIODE_FISCALE_VEROUILLEE) {
            etat = Session_verrouillerPeriode(session, ids_periodes[k]);
            if (etat != ETAT_OK) goto echec;
        }
    }

    free(statuts);
    free(ids_periodes);
    *out_session = session;
    return ETAT_OK;

echec:
    free(statuts);
    free(ids_periodes);
    detruire_session(session);
    return etat;
}

/* ------------------------------------------------------------------
                          API PUBLIQUE
 ------------------------------------------------------------------ */

Etat Session_Sauvegarder(const Session *session, const char *chemin) {
    cJSON *racine;
    char *texte;
    char chemin_tmp[1024];
    FILE *fichier;
    size_t longueur, ecrit;

    if (session == NULL) return ERR_SESSION_INVALIDE;
    if (chemin == NULL || chemin[0] == '\0') return ERR_POINTEUR_NULLE;
    if (strlen(chemin) + 5 >= sizeof(chemin_tmp)) return ERR_ARGUMENT_INVALIDE;

    racine = json_construire(session);
    if (racine == NULL) return ERR_SORTIE_DU_MEMOIRE;
    texte = cJSON_Print(racine);
    cJSON_Delete(racine);
    if (texte == NULL) return ERR_ECHEC_SERIALISATION;

    /* on ecrit d'abord dans un fichier temporaire : si le disque est plein
     * ou si le programme plante, l'ancien fichier reste intact */
    snprintf(chemin_tmp, sizeof(chemin_tmp), "%s.tmp", chemin);
    fichier = fopen(chemin_tmp, "wb");
    if (fichier == NULL) {
        free(texte);
        return ERR_ACCES_FICHIER_REFUSE;
    }
    longueur = strlen(texte);
    ecrit = fwrite(texte, 1, longueur, fichier);
    free(texte);
    if (fclose(fichier) != 0 || ecrit != longueur) {
        remove(chemin_tmp);
        return ERR_ECHEC_SAUVEGARDE;
    }

    remove(chemin);   /* sous Windows, rename echoue si la cible existe deja */
    if (rename(chemin_tmp, chemin) != 0) {
        remove(chemin_tmp);
        return ERR_ECHEC_SAUVEGARDE;
    }
    return ETAT_OK;
}

Etat Session_Charger(const char *chemin, Session **out_session) {
    FILE *fichier;
    long taille;
    char *tampon;
    cJSON *racine;
    Etat etat;

    if (out_session == NULL) return ERR_POINTEUR_NULLE;
    *out_session = NULL;
    if (chemin == NULL || chemin[0] == '\0') return ERR_POINTEUR_NULLE;

    fichier = fopen(chemin, "rb");
    if (fichier == NULL) return ERR_FICHIER_INTROUVABLE;

    /* on lit tout le fichier en memoire : fseek/ftell donnent sa taille */
    if (fseek(fichier, 0, SEEK_END) != 0 || (taille = ftell(fichier)) < 0 || fseek(fichier, 0, SEEK_SET) != 0) {
        fclose(fichier);
        return ERR_ERREUR_ENTREE_SORTIE;
    }
    tampon = malloc((size_t)taille + 1);
    if (tampon == NULL) {
        fclose(fichier);
        return ERR_SORTIE_DU_MEMOIRE;
    }
    if (fread(tampon, 1, (size_t)taille, fichier) != (size_t)taille) {
        free(tampon);
        fclose(fichier);
        return ERR_ERREUR_ENTREE_SORTIE;
    }
    fclose(fichier);
    tampon[taille] = '\0';

    racine = cJSON_Parse(tampon);
    free(tampon);
    if (racine == NULL) return ERR_JSON_INVALIDE;

    etat = charger_depuis_json(racine, out_session);
    cJSON_Delete(racine);
    return etat;
}

