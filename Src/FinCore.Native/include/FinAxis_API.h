#ifndef API_H
#define API_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FINAXIS_VERSION_TEXTE "1.0.0"

/* Version du moteur, ex. "1.0.0" */
const char *finAxis_Version(void);

/* sizeof d'une structure publique donnee par son nom C ("Compte", "Ecriture"...).
 * Retourne 0 si le nom est inconnu. */
size_t finAxis_TailleStruct(const char *nom);

#ifdef __cplusplus
}
#endif

#endif
