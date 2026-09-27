/*
 * doublons.c
 * ----------
 * Detection des doublons : deux ecritures DIFFERENTES qui debitent (ou
 * creditent) le meme compte du meme montant, a quelques jours d'intervalle,
 * sont suspectes d'etre une saisie en double (erreur de manipulation, ou
 * fraude qui rejoue volontairement une meme depense).
 *
 * Methode : on met a plat toutes les lignes de la periode dans un tableau,
 * puis on compare chaque ligne a toutes celles qui la suivent (comparaison
 * en O(n^2) --> complexite :: sur les lignes de UNE periode : le volume reste raisonnable,
 * pas besoin d'une structure plus complexe pour l'instant).
 *
 * Deux lignes sont retenues comme "doublon" si :
 *   - elles appartiennent a deux ecritures differentes (deux lignes de la
 *     meme ecriture ne sont pas un doublon, juste une ecriture normale) ;
 *   - meme compte, meme montant, meme sens (debit contre debit, ou credit
 *     contre credit) ;
 *   - leurs dates sont a moins de cfg->fenetre_jours_doublon jours
 *     d'ecart 
 */

#include <stdio.h>   /* snprintf : formatage de texte dans un buffer de taille fixe */
#include <stdlib.h>  /* malloc/realloc/free : allocation dynamique des tableaux */
#include <string.h>  /* memset : remise a zero d'une structure */

#include "Src/FinCore.Native/include/types.h"
#include "Src/FinCore.Native/include/errors.h"
#include "Src/FinCore.Native/include/session.h"
#include "Src/FinCore.Native/include/detection.h"
#include "Src/FinCore.Native/include/dates.h"
