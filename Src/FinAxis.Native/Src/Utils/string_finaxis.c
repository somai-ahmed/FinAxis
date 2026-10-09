#include "Src/FinAxis.Native/include/string_finaxis.h"
#include <ctype.h>
#include <stddef.h>
#include <stdbool.h>

size_t longeurchaine(const char *chaine){
  size_t len = 0;
  if (chaine) {
    for (; chaine[len] != '\0'; len++) {
    }
  }
  return len ;
}

bool chaine_non_vide(const char *chaine){ 
  return (chaine != NULL && longeurchaine(chaine) > 0);
}

bool chaine_est_numerique(const char *chaine) {
    if (chaine == NULL || !chaine_non_vide(chaine)) return false;

    for (const char *p = chaine; *p != '\0'; p++) {
        if (!isdigit((unsigned char)*p))
            return false;
    }
    return true;
}
   
bool ch_copy_securise(char *dest, const char *src, size_t taille_dest){
    if (dest == NULL || src == NULL || taille_dest == 0)
        return false;

    size_t i ;

    while (i < taille_dest - 1 && src[i] != '\0') {
        dest[i] = src[i];
        i++;
    }

    dest[i] = '\0';
    return true;
}

void ch_rogner(char *chaine){
    if (chaine == NULL) return;

    size_t debut = 0;
    size_t fin = longeurchaine(chaine);

    /* efface les espace du debut de la chaine */
    while (chaine[debut] != '\0' && isspace((unsigned char)chaine[debut])) {
        debut++;
    }
    /* efface les espace du fin de la chaine */
    while (fin > debut && isspace((unsigned char)chaine[fin - 1])) {
        fin--;
    }

    size_t i = 0;

    while (debut < fin) {
        chaine[i] = chaine[debut];
        i++;
        debut++;
    }

    chaine[i] = '\0';
}

int strcmp(const char *a, const char *b){
    if (a == NULL || b == NULL)
        return -1;

    while (*a != '\0' && *b != '\0') {

        unsigned char ca = (unsigned char)*a;
        unsigned char cb = (unsigned char)*b;

        ca = (unsigned char)tolower(ca);
        cb = (unsigned char)tolower(cb);

        if (ca != cb)
            return ca - cb;

        a++;
        b++;
    }

    return (unsigned char)*a - (unsigned char)*b;
}
