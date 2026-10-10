/*
 * api.c -- introspection de la DLL 
 */

#include <string.h>

#include "Src/FinCore.Native/include/fincore_ae.h"
#include "Src/FinCore.Native/incldue/api.h"

typedef struct {
    const char *nom;
    size_t      taille;
} EntreeTaille;

/* une ligne par structure echangee avec Python */
#define STRUCT_TAILLE(type) { #type, sizeof(type) }

static const EntreeTaille TABLE_TAILLES[] = {
    STRUCT_TAILLE(DATE),
    STRUCT_TAILLE(Compte),
    STRUCT_TAILLE(ligne_journal),
    STRUCT_TAILLE(Ecriture),
    STRUCT_TAILLE(prop_periode_fiscale),
    STRUCT_TAILLE(SessionConfig),
    STRUCT_TAILLE(LigneBalance),
    STRUCT_TAILLE(Entree_GrandLivre),
    STRUCT_TAILLE(ligne_bilan),
    STRUCT_TAILLE(LigneCompteResultat),
    STRUCT_TAILLE(CompteResultat),
    STRUCT_TAILLE(Resultat_Detection),
    STRUCT_TAILLE(StatChiffreBenford),
    STRUCT_TAILLE(Rapport_Benford),
    STRUCT_TAILLE(config_detection)
};

#define NB_TAILLES ((int)(sizeof(TABLE_TAILLES) / sizeof(TABLE_TAILLES[0])))

const char *FinCore_Version(void) {
    return FINCORE_VERSION_TEXTE;
}

size_t FinCore_TailleStruct(const char *nom) {
    int i;

    if (nom == NULL) {
        return 0;
    }
    for (i = 0; i < NB_TAILLES; i++) {
        if (strcmp(TABLE_TAILLES[i].nom, nom) == 0) {
            return TABLE_TAILLES[i].taille;
        }
    }
    return 0;
}
