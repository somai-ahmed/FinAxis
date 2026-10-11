#include <Src/FinCore.Native/Src/Utils/include/identifiants.h>
#include <string.h>

generateur_id initiation_generateur_id(int64_t valeur_depart){
    generateur_id gen;
    gen.compteur = valeur_depart;
    return gen;
}

int64_t generateur_id_suivant(generateur_id *gen){
    if (!gen) return -1;
    gen->compteur++;
    return gen->compteur;
}

bool generateur_id_reference(generateur_id *gen, const char *prefixe, int largeur_min, char *tampon, size_t taille_tampon){
    if (!gen || !prefixe || largeur_min <= 0 || taille_tampon == 0) return false ;

    int64_t id = generateur_id_suivant(gen);
    if (id<0) return false;

    size_t pos = 0;
    size_t len_prefixe = strlen(prefixe);

    /* PREFIXE */
    if (pos + len_prefixe + 1 >= taille_tampon) return false;
    memcpy(tampon + pos, prefixe, len_prefixe);
    pos += len_prefixe;
    tampon[pos++] = '-';

    /* chiffres de l'id, construits a l'envers */
    char chiffres[24];
    int n = 0;
    uint64_t valeur = (uint64_t)id;

    if (valeur == 0) {
        chiffres[n++] = '0';
    } else {
        while (valeur > 0 && n < (int)sizeof(chiffres)) {
            chiffres[n++] = (char)('0' + (valeur % 10));
            valeur /= 10;
        }
    }

    while (n < largeur_min && n < (int)sizeof(chiffres)) {
        chiffres[n++] = '0';
    }

    if (pos + (size_t)n + 1 > taille_tampon) return false;

    for (int i = 0; i < n; i++) {
        tampon[pos++] = chiffres[n - 1 - i];
    }

    tampon[pos] = '\0';
    return true;
}
