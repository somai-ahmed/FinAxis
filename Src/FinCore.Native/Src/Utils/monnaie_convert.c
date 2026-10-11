#include <Src/FinCore.Native/include/monnaie_convert.h>

#include <stdint.h>

/* ajouter cette fonction sous le caractere static car elle est dedie juste a faciliter le fonctionnement du cette fichier */
static int64_t puissance_de_dix(int n) {
    int64_t resultat = 1;
    int i;
    for (i = 0; i < n; i++) {
        resultat *= 10;
    }
    return resultat;
}

monnaie monnaie_depuis_fixe(Monnaie m){
    return monnaie_depuis_unites_mineures(m, MONNAIE_FIXE_ECHELLE); /* l"appelle du la fonction du le moteur de la monnaie du base <monnaie.h> */
}

Monnaie monnaie_vers_fixe(monnaie m, bool *success){
    int64_t facteur;
    int64_t resultat;

    if (success != NULL) {
        *success = true;
    }

    if (m.echelle == MONNAIE_FIXE_ECHELLE) {
        return m.unites_min;
    }

    if (m.echelle < MONNAIE_FIXE_ECHELLE) {
        /* echelle source moins precise -> on complete avec des zeros */
        facteur = puissance_de_dix(MONNAIE_FIXE_ECHELLE - m.echelle);

        if (m.unites_min != 0 &&
            (m.unites_min > INT64_MAX / facteur || m.unites_min < INT64_MIN / facteur)) {
            if (success != NULL) {
                *success = false;
            }
            return 0;
        }

        resultat = m.unites_min * facteur;
    } 
    else {
        /* echelle source plus precise -> arrondi (perte de precision) */
        facteur = puissance_de_dix(m.echelle - MONNAIE_FIXE_ECHELLE);

        if (m.unites_min >= 0) {
            resultat = (m.unites_min + facteur / 2) / facteur;
        } 
        else {
            resultat = (m.unites_min - facteur / 2) / facteur;
        }
    }

    return resultat;
}
