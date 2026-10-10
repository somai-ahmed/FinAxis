/*
 * errors.c -- table des messages d'erreur du moteur FinCore
 *
 * GetErrorMessage transforme un code Etat en texte lisible (jamais NULL).
 * Utilise par Python (ctypes) pour construire les exceptions.
 * La table suit l'ordre de errors.h : ajouter ici tout nouveau code ERR_*.
 */

#include "Src/FinCore.Native/include/errors.h"

typedef struct {
    Etat code;
    const char *message;
} EntreeMessage;

static const EntreeMessage TABLE_MESSAGES[] = {
/* dataset sous forme du matrice (on peut l'implementer a traver un fichier JSON ) */
    { ERR_INCONNU, "Erreur inconnue" },
    { ERR_ARGUMENT_INVALIDE, "Argument invalide" },
    { ERR_POINTEUR_NULLE, "Pointeur nul" },
    { ERR_SORTIE_DU_MEMOIRE, "Memoire insuffisante" },
    { ERR_SESSION_INVALIDE, "Session invalide ou absente" },
    { ERR_NON_INITIALISE, "Element non initialise" },
    { ERR_TRES_PETIT_BUFFER, "Tampon trop petit" },
    { ERR_OPERATION_INTERDITE, "Operation interdite" },
    { ERR_IDENTIFIANT_INVALIDE, "Identifiant invalide" },
    { ERR_ELEMENT_DEJA_EXISTANT, "Element deja existant" },
    { ERR_ELEMENT_INTROUVABLE, "Element introuvable" },
    { ERR_COMPTE_INTROUVABLE, "Compte introuvable" },
    { ERR_COMPTE_EXISTE, "Compte existe" },
    { ERR_COMPTE_INACTIF, "Compte inactif" },
    { ERR_COMPTE_AVEC_ENFANTS, "Compte avec enfants" },
    { ERR_CODE_COMPTE_INVALIDE, "Code compte invalide" },
    { ERR_TYPE_COMPTE_INVALIDE, "Type compte invalide" },
    { ERR_CLASSE_COMPTE_INVALIDE, "Classe compte invalide" },
    { ERR_COMPTE_NON_AUTORISE, "Compte non autorise" },
    { ERR_COMPTE_UTILISE, "Compte utilise" },
    { ERR_COMPTE_COLLECTIF_INVALIDE, "Compte collectif invalide" },
    { ERR_COMPTE_AUXILIAIRE_INVALIDE, "Compte auxiliaire invalide" },
    { ERR_COMPTE_RESTREINT, "Compte restreint" },
    { ERR_JOURNAL_INTROUVABLE, "Journal introuvable" },
    { ERR_JOURNAL_NON_EQUILIBRE, "Ecriture non equilibree : total debit different du total credit" },
    { ERR_JOURNAL_VIDE, "Journal vide" },
    { ERR_JOURNAL_DEJA_COMPTABILISE, "Journal deja comptabilise" },
    { ERR_LIGNE_JOURNAL_INVALIDE, "Ligne journal invalide" },
    { ERR_JOURNAL_MONTANT_NEGATIF, "Journal montant negatif" },
    { ERR_JOURNAL_SANS_MONTANT, "Journal sans montant" },
    { ERR_JOURNAL_DEBIT_CREDIT_SIMULTANES, "Journal debit credit simultanes" },
    { ERR_DATE_JOURNAL_INVALIDE, "Date journal invalide" },
    { ERR_LIBELLE_JOURNAL_MANQUANT, "Libelle journal manquant" },
    { ERR_COMPTE_DEBIT_INVALIDE, "Compte debit invalide" },
    { ERR_COMPTE_CREDIT_INVALIDE, "Compte credit invalide" },
    { ERR_JOURNAL_DEJA_EXISTANT, "Journal deja existant" },
    { ERR_JOURNAL_PERIODE_INVALIDE, "Journal periode invalide" },
    { ERR_JOURNAL_PERIODE_FERMEE, "Journal periode fermee" },
    { ERR_JOURNAL_MONTANT_NUL, "Journal montant nul" },
    { ERR_BANQUE_SOLDE_NEGATIF, "Banque solde negatif" },
    { ERR_CAISSE_SOLDE_NEGATIF, "Caisse solde negatif" },
    { ERR_TRESORERIE_SOLDE_NEGATIF, "Tresorerie solde negatif" },
    { ERR_MONTANT_BANCAIRE_INVALIDE, "Montant bancaire invalide" },
    { ERR_COMPTE_BANCAIRE_INTROUVABLE, "Compte bancaire introuvable" },
    { ERR_COMPTE_CAISSE_INTROUVABLE, "Compte caisse introuvable" },
    { ERR_SOLDE_INSUFFISANT, "Solde insuffisant" },
    { ERR_OPERATION_BANCAIRE_INTERDITE, "Operation bancaire interdite" },
    { ERR_RAPPROCHEMENT_BANCAIRE_ECHEC, "Rapprochement bancaire echec" },
    { ERR_ECART_SOLDE_BANCAIRE, "Ecart solde bancaire" },
    { ERR_PERIODE_INTROUVABLE, "Periode introuvable" },
    { ERR_PERIODE_FERMEE, "Periode fermee : aucune ecriture acceptee" },
    { ERR_PERIODE_VERROUILLEE, "Periode verrouillee" },
    { ERR_PERIODE_CHEVAUCHEMENT, "Periode chevauchement" },
    { ERR_PLAGE_DATES_INVALIDE, "Plage dates invalide" },
    { ERR_EXERCICE_INTROUVABLE, "Exercice introuvable" },
    { ERR_EXERCICE_FERME, "Exercice ferme" },
    { ERR_DATE_HORS_EXERCICE, "Date hors exercice" },
    { ERR_MODIFICATION_PERIODE_FERMEE, "Modification periode fermee" },
    { ERR_PERIODE_EXISTE, "Periode existe" },
    { ERR_ECHEC_GENERATION_RAPPORT, "Echec generation rapport" },
    { ERR_AUCUNE_DONNEE_PERIODE, "Aucune donnee periode" },
    { ERR_BILAN_NON_EQUILIBRE, "Bilan non equilibre" },
    { ERR_ACTIF_PASSIF_INCOHERENT, "Actif passif incoherent" },
    { ERR_BALANCE_NON_EQUILIBREE, "Balance non equilibree" },
    { ERR_BALANCE_DEBIT_CREDIT_INCOHERENTS, "Balance debit credit incoherents" },
    { ERR_COMPTE_RESULTAT_INVALIDE, "Compte resultat invalide" },
    { ERR_RESULTAT_NET_INVALIDE, "Resultat net invalide" },
    { ERR_GRAND_LIVRE_INCOMPLET, "Grand livre incomplet" },
    { ERR_DONNEES_BILAN_INSUFFISANTES, "Donnees bilan insuffisantes" },
    { ERR_DONNEES_RESULTAT_INSUFFISANTES, "Donnees resultat insuffisantes" },
    { ERR_ETAT_FINANCIER_INCOHERENT, "Etat financier incoherent" },
    { ERR_GRAND_LIVRE_BALANCE_INCOHERENTS, "Grand livre balance incoherents" },
    { ERR_BALANCE_BILAN_INCOHERENTS, "Balance bilan incoherents" },
    { ERR_RESULTAT_BILAN_INCOHERENTS, "Resultat bilan incoherents" },
    { ERR_ECHEC_DETECTION, "Echec detection" },
    { ERR_DONNEES_INSUFFISANTES, "Donnees insuffisantes" },
    { ERR_ECHEC_ANALYSE_BENFORD, "Echec analyse benford" },
    { ERR_ECHANTILLON_BENFORD_INSUFFISANT, "Echantillon trop petit pour l'analyse de Benford" },
    { ERR_ECHEC_DETECTION_DOUBLONS, "Echec detection doublons" },
    { ERR_ECHEC_DETECTION_MONTANTS_RONDS, "Echec detection montants ronds" },
    { ERR_ECHEC_DETECTION_VALEURS_ABERRANTES, "Echec detection valeurs aberrantes" },
    { ERR_CONFIGURATION_DETECTION_INVALIDE, "Configuration detection invalide" },
    { ERR_RESULTAT_DETECTION_INVALIDE, "Resultat detection invalide" },
    { ERR_FICHIER_INTROUVABLE, "Fichier introuvable" },
    { ERR_ACCES_FICHIER_REFUSE, "Acces fichier refuse" },
    { ERR_FICHIER_CORROMPU, "Fichier corrompu" },
    { ERR_ECHEC_SERIALISATION, "Echec serialisation" },
    { ERR_ECHEC_DESERIALISATION, "Echec deserialisation" },
    { ERR_VERSION_NON_SUPPORTEE, "Version non supportee" },
    { ERR_ERREUR_ENTREE_SORTIE, "Erreur entree sortie" },
    { ERR_ECHEC_SAUVEGARDE, "Echec sauvegarde" },
    { ERR_ECHEC_CHARGEMENT, "Echec chargement" },
    { ERR_JSON_INVALIDE, "Json invalide" },
    { ERR_CSV_INVALIDE, "Csv invalide" },
    { ERR_DONNEES_COMPTABLES_INVALIDES, "Donnees comptables invalides" },
    { ERR_BALANCE_GENERALE_INCOHERENTE, "Balance generale incoherente" },
    { ERR_TOTAL_DEBIT_INCORRECT, "Total debit incorrect" },
    { ERR_TOTAL_CREDIT_INCORRECT, "Total credit incorrect" },
    { ERR_DEBIT_CREDIT_DIFFERENTS, "Debit credit differents" },
    { ERR_SOLDE_COMPTE_INCOHERENT, "Solde compte incoherent" },
    { ERR_MOUVEMENT_COMPTABLE_INVALIDE, "Mouvement comptable invalide" },
    { ERR_GRAND_LIVRE_BALANCE_DIFFERENTS, "Grand livre balance differents" },
    { ERR_BALANCE_BILAN_DIFFERENTS, "Balance bilan differents" },
    { ERR_RESULTAT_FINANCIER_INCOHERENT, "Resultat financier incoherent" },
    { ERR_TRANSACTION_INVALIDE, "Transaction invalide" },
    { ERR_TRANSACTION_DEJA_COMPTABILISEE, "Transaction deja comptabilisee" },
    { ERR_TRANSACTION_ANNULEE, "Transaction annulee" },
    { ERR_TRANSACTION_NON_ANNULABLE, "Transaction non annulable" },
    { ERR_TRANSACTION_HORS_PERIODE, "Transaction hors periode" },
    { ERR_TRANSACTION_NON_EQUILIBREE, "Transaction non equilibree" },
    { ERR_TRANSACTION_INTERDITE, "Transaction interdite" },
};

#define NB_MESSAGES ((int)(sizeof(TABLE_MESSAGES) / sizeof(TABLE_MESSAGES[0])))

const char *GetErrorMessage(Etat etat) {
    int i;

    if (etat == ETAT_OK) {
        return "Succes";
    }

    /* recherche lineaire : la table est petite et l'appel est rare */
    for (i = 0; i < NB_MESSAGES; i++) {
        if (TABLE_MESSAGES[i].code == etat) {
            return TABLE_MESSAGES[i].message;
        }
    }
    return "Erreur inconnue";
}
