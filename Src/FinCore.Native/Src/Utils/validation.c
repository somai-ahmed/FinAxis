#include <Src/FinCore.Native/include/validation.h>

#include <string.h>
#include <ctype.h>


bool valider_chaine_non_vide(const char *chaine){
  if(chaine==NULL) return false;
  if(*chaine == '\0') return false;  
  return true;
}

bool valider_longueur_chaine(const char *chaine, size_t min, size_t max){
    if(chaine==NULL) return false;
    if(min > max) return false;

    size_t longueur = strlen(chaine);

    return longueur >= min && longueur <= max;
}


bool valider_caracteres_autorises(const char *chaine, const char *caracteres_extra){
    if (chaine == NULL) return false;

    const char *p = chaine;
    while (*p != '\0') {
        if (isalnum((unsigned char)*p)) {
            p++;
            continue;
        }
        if (*p == ' ') {
            p++;
            continue;
        }
        if (caracteres_extra != NULL && strchr(caracteres_extra, *p) != NULL) {
            p++;
            continue;
        }
        return false;
    }
    return true;
}

bool valider_code_compte(const char *code, size_t longueur_min, size_t longueur_max){
    if (code == NULL) return false;
    if (longueur_min > longueur_max) return false;

    size_t longueur = strlen(code);
    if (longueur < longueur_min || longueur > longueur_max) {
        return false;
    }

    for (const char *p = code; *p != '\0'; p++) {
        if (!isdigit((unsigned char)*p)) {
            return false;
        }
    }
    return true;
}

bool valider_format_reference(const char *reference, const char *prefixe_attendu){
    if (reference == NULL || prefixe_attendu == NULL) return false;

    size_t longueur_prefixe = strlen(prefixe_attendu);

    if (strncmp(reference, prefixe_attendu, longueur_prefixe) != 0) {
        return false;
    }

    const char *p = reference + longueur_prefixe;

    if (*p != '-') return false;
    p++;

    if (!isdigit((unsigned char)*p)) return false;
    while (isdigit((unsigned char)*p)) {
        p++;
    }

    return *p == '\0';
}

bool valider_plage_i64(int64_t valeur, int64_t min, int64_t max){
    if (min > max) return false;
    return valeur >= min && valeur <= max;
}

bool valider_montant(monnaie m, bool autoriser_negatif){
    if (m.echelle < 0) return false;
    if (!autoriser_negatif && m.unites_min < 0) return false;
    return true;
}
