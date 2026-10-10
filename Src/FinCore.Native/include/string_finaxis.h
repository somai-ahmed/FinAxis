#ifndef STRING_FINAXIS_H
#define STRING_FINAXIS_H

#include <stddef.h>
#include <stdbool.h>


/* Copie src dans dest en garantissant la terminaison nulle et sans
 * depassement (taille_dest = capacite totale de dest, buffer inclus) */
bool ch_copy_securise(char *dest, const char *src, size_t taille_dest);

/* Retire les espaces (et tabulations) en debut/fin de chaine, en place */
void ch_rogner(char *chaine);

/* Compare deux chaines sans tenir compte de la casse */
int ch_comparer_insensible(const char *a, const char *b);

/* Verifie que la chaine ne contient que des chiffres [0-9] */
bool chaine_est_numerique(const char *chaine);

/* Verifie qu'une chaine n'est ni NULL ni vide */
bool chaine_non_vide(const char *chaine);


#endif
