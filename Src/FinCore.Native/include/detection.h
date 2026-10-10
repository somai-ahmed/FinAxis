#ifndef DETECTION_H
#define DETECTION_H 

#include "types.h"
#include "errors.h"
#include "periodes.h"



/* ============================================================
* Moteur Comptable FinCore - Moteur de détection
* ============================================================
* Le module Detection permet à FinCore d'identifier les anomalies
* pouvant apparaître dans les écritures et données comptables.
*
* Les anomalies sont classées selon plusieurs méthodes et niveaux
* de sévérité afin de faciliter leur analyse et leur traitement.
*
* Le moteur peut être configuré selon les méthodes de détection
* utilisées et les seuils appliqués lors de l'analyse.
*
* Les résultats retournés permettent d'identifier précisément
* l'écriture, la ligne ou le compte concerné par une anomalie.
*
* Voir documentation/detection-engine.md
* ============================================================ */


/* Seuil du chi-carre de Benford : 8 degres de liberte, confiance 95 % */
#define BENFORD_SEUIL_CHI_CARRE 15.507

typedef enum Meth_Detection {
    DETECT_BENFORD = 0,
    DETECT_DOUBLONS = 1,
    DETECT_NOMBRES_RONDS = 2,
    DETECT_VALS_ABERRANTES = 3
} Meth_Detection;

/*Les niveaux de gravite sont numerotes du niveau le plus sain 
    au niveau le plus dangereux*/
typedef enum Gravite {
   GRAVITE_INFO     = 0,
   GRAVITE_FAIBLE   = 1,
   GRAVITE_MOYENNE  = 2,
   GRAVITE_ELEVEE   = 3,
   GRAVITE_CRITIQUE = 4
} Gravite;

typedef struct Resultat_Detection{
    Meth_Detection methode;
    Gravite gravite;
    id_ligne id_ligne; /*extrait de la fichier types.h -- << typedef uint32_t id_ligne >> */
    id_compte id_compte; /*extrait de la fichier types.h -- << typedef uint32_t id_compte >> */
    char description[256];
    double score ; /*la detection du resultat va etre analyse sur des bases mathematiques et des formules precis sous forme d'un score
    puis on va classer ceci selon le score obtenu*/
}Resultat_Detection;

/*voir l'explication du theoreme Benford en Documentation/Benford_strategie.ipynb*/
typedef struct StatChiffreBenford {
    int numero; /* intervalle [0,9] */
    double freq_attendu; /* Proportion attendue selon Benford */
    double freq_observe; /* Proportion réelle dans le jeu de données */
    int nombre_observe;
} StatChiffreBenford;

/*pour le rapport on a besoin des statistiques*/
/*l'appelle du structure "StatChiffreBenford" pour la designiation du rapport*/
typedef struct Rapport_Benford{
    StatChiffreBenford stat[9] ;
    double chiffre_carree;
    double ecart_absolu_moyen;
    int taille_echantillon;
    int estnormal ; /*variable booleenne peut prendre seulemnt 0 ou 1 comme un valeur*/
}Rapport_Benford;

typedef struct config_detection{
    int benford_est_active ;
    int doublons_est_active;
    int nombres_ronds_active;
    int valeurs_aberrantes_active; 
    double seuil_ecart_type_extreme;    /* ex :3,0 sigma */
    double seuil_ecart_nombres_ronds; /*ex: montants divisibles par 1000*/
    int fenetre_jours_doublon; /*période de recherche des doublons*/
}config_detection;

/*pour le fonctionnement d'une maniere propre du programme , on necessite l'integration du l'API du detection*/
/*cad l API aka "APPLICATION PROGRAMMING INTERFACE" : l'interface entre le programme et le moteur du detection*/
/*dans ce cas , la session comptable va etre analyse et passe par l'API du detection pour on conclure a la fin
    le RESULTAT puis le renvoi au programme principal*/

/* voir Documentation/API_detection.ipynb*/

/*----------------------------------------------------------------------
                        API DU DETECTION
    ------------------------------------------------------------------*/

/* Remplit config avec des valeurs par défaut 
    --> Cette fonction ne peut jamais etre FRAUDE*/
void detection_config_par_defaut(config_detection* cfg);

/* Exécute toutes les méthodes de détection activées sur les écritures
    comptables validées pendant la période donnée
    Les résultats sont alloués en interne
    l'appelant doit appeler LibererResultatsDetection lorsqu'il a terminé */

Etat execute_detection(
    Session* session,
    idperiodefiscale id_periode,
    const config_detection* cfg,
    Resultat_Detection** resultats,
    size_t* nombre_resultats
);/*output est du forme Etat*/ /*struct en fichier src/FinCore.Native/include/errors.h*/


/* Exécute l'analyse de Benford sur les écritures comptables
    validées pendant la période donnée
    Le rapport d'analyse est rempli en interne
    l'appelant doit fournir un pointeur vers le rapport */
Etat Executer_Analyse_Benford(
    Session* session,
    idperiodefiscale id_periode,
    Rapport_Benford* rapport
);

/* Libère la mémoire allouée pour les résultats de détection
    Le tableau de résultats et les données associées sont libérés
    l'appelant doit fournir le nombre de résultats à libérer */
void liberer_resultats_detection(Resultat_Detection* resultat, size_t nombre_resultats);

#endif
