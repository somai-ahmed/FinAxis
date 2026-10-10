#ifndef ERRORS_H
#define ERRORS_H

/* ============================================================
 * Moteur Comptable FinCore - Codes d'erreur
 * ============================================================
 * Toutes les fonctions publiques de l'API retournent un
 * Status. 0 signifie toujours le succès ; les valeurs
 * négatives sont des erreurs, regroupées par module en blocs
 * de -100 afin que PY6 puisse mapper les plages à des
 * types d'exceptions.
 * ============================================================ */

/* --------------------------------------------------------------
    FinCore peut envisager pluseieurs erreurs : ces erreurs sont
    clasé sur plusieures modules , dont chaque erreur appartient
    a un grand classe des erreur : 
    les codes des erreurs sont presentes comme des intervalles
    c'est le cas ou : 
    on definit l'erreur par : 
        |    INTERVALLE     | MODULE    |  ERREUR PREVUE     |

        NB : les erreurs sont ppresente en nombres NEGATIFS (le 
        correcte codage des codes des erreurs)

    voir documentation/erreurs.md
    -------------------------------------------------------------*/
        

typedef enum Etat {
    ETAT_OK = 0,

    /* Erreurs générales : -1 à -99 */

    ERR_INCONNU                  = -1,
    ERR_ARGUMENT_INVALIDE        = -2,
    ERR_POINTEUR_NULLE           = -3,
    ERR_SORTIE_DU_MEMOIRE        = -4,
    ERR_SESSION_INVALIDE         = -5,
    ERR_NON_INITIALISE           = -6,
    ERR_TRES_PETIT_BUFFER        = -7,
    ERR_OPERATION_INTERDITE      = -8,
    ERR_IDENTIFIANT_INVALIDE     = -9,
    ERR_ELEMENT_DEJA_EXISTANT    = -10,
    ERR_ELEMENT_INTROUVABLE      = -11,

    /* Erreurs comptables : -100 à -199 */
    ERR_COMPTE_INTROUVABLE          = -100,
    ERR_COMPTE_EXISTE               = -101,
    ERR_COMPTE_INACTIF              = -102,
    ERR_COMPTE_AVEC_ENFANTS         = -103,
    ERR_CODE_COMPTE_INVALIDE        = -104,
    ERR_TYPE_COMPTE_INVALIDE        = -105,
    ERR_CLASSE_COMPTE_INVALIDE      = -106,
    ERR_COMPTE_NON_AUTORISE         = -107,
    ERR_COMPTE_UTILISE              = -108,
    ERR_COMPTE_COLLECTIF_INVALIDE   = -109,
    ERR_COMPTE_AUXILIAIRE_INVALIDE  = -110,
    ERR_COMPTE_RESTREINT            = -111,

    /* Erreurs de journal : -200 à -249 */
    ERR_JOURNAL_INTROUVABLE             = -200,
    ERR_JOURNAL_NON_EQUILIBRE           = -201,
    ERR_JOURNAL_VIDE                    = -202,
    ERR_JOURNAL_DEJA_COMPTABILISE       = -203,
    ERR_LIGNE_JOURNAL_INVALIDE          = -204,
    ERR_JOURNAL_MONTANT_NEGATIF         = -205,
    ERR_JOURNAL_SANS_MONTANT            = -206,
    ERR_JOURNAL_DEBIT_CREDIT_SIMULTANES = -207,
    ERR_DATE_JOURNAL_INVALIDE           = -208,
    ERR_LIBELLE_JOURNAL_MANQUANT        = -209,
    ERR_COMPTE_DEBIT_INVALIDE           = -210,
    ERR_COMPTE_CREDIT_INVALIDE          = -211,
    ERR_JOURNAL_DEJA_EXISTANT           = -212,
    ERR_JOURNAL_PERIODE_INVALIDE        = -213,
    ERR_JOURNAL_PERIODE_FERMEE          = -214,
    ERR_JOURNAL_MONTANT_NUL             = -215,

    /* Erreurs de BANQUE : -250 à -299 */
    ERR_BANQUE_SOLDE_NEGATIF             = -250,
    ERR_CAISSE_SOLDE_NEGATIF             = -251,
    ERR_TRESORERIE_SOLDE_NEGATIF         = -252,
    ERR_MONTANT_BANCAIRE_INVALIDE        = -253,
    ERR_COMPTE_BANCAIRE_INTROUVABLE      = -254,
    ERR_COMPTE_CAISSE_INTROUVABLE        = -255,
    ERR_SOLDE_INSUFFISANT                = -256,
    ERR_OPERATION_BANCAIRE_INTERDITE     = -257,
    ERR_RAPPROCHEMENT_BANCAIRE_ECHEC     = -258,
    ERR_ECART_SOLDE_BANCAIRE             = -259,

    /* Erreurs de période : -300 à -399 */
    ERR_PERIODE_INTROUVABLE              = -300,
    ERR_PERIODE_FERMEE                   = -301,
    ERR_PERIODE_VERROUILLEE              = -302,
    ERR_PERIODE_CHEVAUCHEMENT            = -303,
    ERR_PLAGE_DATES_INVALIDE             = -304,
    ERR_EXERCICE_INTROUVABLE             = -305,
    ERR_EXERCICE_FERME                   = -306,
    ERR_DATE_HORS_EXERCICE               = -307,
    ERR_MODIFICATION_PERIODE_FERMEE      = -308,
    ERR_PERIODE_EXISTE                   = -309,

    /* Erreurs de rapport : -400 à -499 */
    ERR_ECHEC_GENERATION_RAPPORT         = -400,
    ERR_AUCUNE_DONNEE_PERIODE            = -401,
    ERR_BILAN_NON_EQUILIBRE              = -402,
    ERR_ACTIF_PASSIF_INCOHERENT          = -403,
    ERR_BALANCE_NON_EQUILIBREE           = -404,
    ERR_BALANCE_DEBIT_CREDIT_INCOHERENTS = -405,
    ERR_COMPTE_RESULTAT_INVALIDE         = -406,
    ERR_RESULTAT_NET_INVALIDE            = -407,
    ERR_GRAND_LIVRE_INCOMPLET            = -408,
    ERR_DONNEES_BILAN_INSUFFISANTES      = -409,
    ERR_DONNEES_RESULTAT_INSUFFISANTES   = -410,
    ERR_ETAT_FINANCIER_INCOHERENT        = -411,
    ERR_GRAND_LIVRE_BALANCE_INCOHERENTS  = -412,
    ERR_BALANCE_BILAN_INCOHERENTS        = -413,
    ERR_RESULTAT_BILAN_INCOHERENTS       = -414,

    /* Erreurs du moteur de détection : -500 à -599 */
    ERR_ECHEC_DETECTION                    = -500,
    ERR_DONNEES_INSUFFISANTES              = -501,
    ERR_ECHEC_ANALYSE_BENFORD              = -502,
    ERR_ECHANTILLON_BENFORD_INSUFFISANT    = -503,
    ERR_ECHEC_DETECTION_DOUBLONS            = -504,
    ERR_ECHEC_DETECTION_MONTANTS_RONDS      = -505,
    ERR_ECHEC_DETECTION_VALEURS_ABERRANTES  = -506,
    ERR_CONFIGURATION_DETECTION_INVALIDE    = -507,
    ERR_RESULTAT_DETECTION_INVALIDE         = -508,

    /* Erreurs de persistance : -600 à -699 */
    ERR_FICHIER_INTROUVABLE          = -600,
    ERR_ACCES_FICHIER_REFUSE         = -601,
    ERR_FICHIER_CORROMPU             = -602,
    ERR_ECHEC_SERIALISATION          = -603,
    ERR_ECHEC_DESERIALISATION        = -604,
    ERR_VERSION_NON_SUPPORTEE        = -605,
    ERR_ERREUR_ENTREE_SORTIE         = -606,
    ERR_ECHEC_SAUVEGARDE             = -607,
    ERR_ECHEC_CHARGEMENT             = -608,
    ERR_JSON_INVALIDE                 = -609,
    ERR_CSV_INVALIDE                  = -610,

    /* Erreurs de validation comptable : -800 à -899 */
    ERR_DONNEES_COMPTABLES_INVALIDES       = -800,
    ERR_BALANCE_GENERALE_INCOHERENTE       = -801,
    ERR_TOTAL_DEBIT_INCORRECT              = -802,
    ERR_TOTAL_CREDIT_INCORRECT             = -803,
    ERR_DEBIT_CREDIT_DIFFERENTS            = -804,
    ERR_SOLDE_COMPTE_INCOHERENT            = -805,
    ERR_MOUVEMENT_COMPTABLE_INVALIDE       = -806,
    ERR_GRAND_LIVRE_BALANCE_DIFFERENTS     = -807,
    ERR_BALANCE_BILAN_DIFFERENTS           = -808,
    ERR_RESULTAT_FINANCIER_INCOHERENT      = -809,

    /* Erreurs de transaction : -900 à -999 */

    ERR_TRANSACTION_INVALIDE               = -900,
    ERR_TRANSACTION_DEJA_COMPTABILISEE     = -901,
    ERR_TRANSACTION_ANNULEE                = -902,
    ERR_TRANSACTION_NON_ANNULABLE          = -903,
    ERR_TRANSACTION_HORS_PERIODE           = -904,
    ERR_TRANSACTION_NON_EQUILIBREE         = -905,
    ERR_TRANSACTION_INTERDITE              = -906

}Etat;

/* Retourne un texte lisible pour un code Etat (jamais NULL) */
const char* GetErrorMessage(Etat etat);

#endif
