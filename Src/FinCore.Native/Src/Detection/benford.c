/*
 * benford.c
 * ---------
 * Implemente l'analyse de la loi de Benford pour le moteur de detection.
 *
 * Rappel du principe (voir Documentation/Benford_strategie.ipynb) : dans
 * beaucoup de jeux de donnees "naturels" (montants comptables inclus), le
 * premier chiffre significatif n'est pas uniformement reparti entre 1 et 9 :
 * le chiffre 1 apparait environ 30% du temps, le 9 seulement environ 4.6%.
 * Une fraude "fabriquee" (montants inventes) s'ecarte souvent de cette loi,
 * d'ou l'interet de la detecter automatiquement.
 *
 * Ce fichier fournit deux fonctions, toutes les deux forward-declarees
 * dans detection.c (elles ne font pas partie de l'API publique de la DLL) :
 *   - calculer_rapport_benford : le rapport statistique complet
 *   - detecter_benford_anomalies : l'adaptateur qui transforme ce rapport
 *     en un (eventuel) Resultat_Detection pour l'orchestrateur
 */

#include <stdio.h>   /* pour snprintf : construire le texte de description */
#include <stdlib.h>  /* pour malloc/realloc/free : allocation dynamique */
#include <string.h>  /* pour memset : remettre une structure a zero proprement */

#include "Src/FinCore.Native/include/types.h"
#include "Src/FinCore.Native/include/errors.h"
#include "Src/FinCore.Native/include/session.h"
#include "Src/FinCore.Native/include/detection.h"
#include "Src/FinCore.Native/include/math_utils.h"

