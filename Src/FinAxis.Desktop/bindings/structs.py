"""Miroirs ctypes des structures C du moteur FinAxis.

Chaque classe reproduit EXACTEMENT l'ordre et le type des champs de la
structure C correspondante (types.h, dates.h, session.h, rapports.h,
detection.h). Au chargement de la DLL, finaxis_ffi.py compare sizeof() de
chaque classe avec celui du C : un decalage est donc detecte tout de suite.
"""

import ctypes
from ctypes import c_char, c_double, c_int, c_int32, c_int64, c_size_t, c_uint32

CODE_LEN = 16
NOM_COMPTE_LEN = 128
REF_NOM_LEN = 256
REFERENCE_LEN = 32
DESCRIPTION_LEN = 256
LIBELLE_LEN = 256
PERIODE_NOM_LEN = 64
SESSION_NOM_LEN = 128
SESSION_DEVISE_LEN = 4
BILAN_LIBELLE_LEN = 128
CR_MAX_LIGNES = 512
CR_NB_RUBRIQUES = 7
BILAN_NB_LIGNES = 15


class CDate(ctypes.Structure):
    """DATE (dates.h) : annee, mois, jour."""
    _fields_ = [("annee", c_int32), ("mois", c_int32), ("jour", c_int32)]


class CCompte(ctypes.Structure):
    _fields_ = [
        ("id", c_uint32),
        ("code", c_char * CODE_LEN),
        ("nom", c_char * NOM_COMPTE_LEN),
        ("classe", c_int),
        ("type", c_int),
        ("solde_normal", c_int),
        ("solde", c_int64),
        ("parent_id", c_uint32),
        ("est_active", c_int),
    ]


class CLigneJournal(ctypes.Structure):
    _fields_ = [
        ("id", c_uint32),
        ("compte_id", c_uint32),
        ("debit", c_int64),     # champ "Debit" en C
        ("credit", c_int64),
        ("libelle", c_char * LIBELLE_LEN),
    ]


class CEcriture(ctypes.Structure):
    _fields_ = [
        ("id", c_uint32),
        ("periode_id", c_uint32),
        ("date", CDate),
        ("reference", c_char * REFERENCE_LEN),
        ("description", c_char * DESCRIPTION_LEN),
        ("lignes", ctypes.POINTER(CLigneJournal)),
        ("nombre_lignes", c_size_t),
        ("est_validee", c_int),
    ]


class CPeriode(ctypes.Structure):
    """prop_periode_fiscale."""
    _fields_ = [
        ("id", c_uint32),
        ("nom", c_char * PERIODE_NOM_LEN),
        ("date_debut", CDate),
        ("date_fin", CDate),
        ("statut", c_int),
    ]


class CSessionConfig(ctypes.Structure):
    _fields_ = [
        ("nom_entreprise", c_char * SESSION_NOM_LEN),
        ("devise", c_char * SESSION_DEVISE_LEN),
        ("debut_exercice", CDate),
        ("fin_exercice", CDate),
    ]


class CRefCompte(ctypes.Structure):
    _fields_ = [("id", c_uint32), ("code", c_char * CODE_LEN), ("nom", c_char * REF_NOM_LEN)]


class CLigneBalance(ctypes.Structure):
    _fields_ = [
        ("compte", CRefCompte),
        ("total_debit", c_int64),
        ("total_credit", c_int64),
        ("solde_debit", c_int64),
        ("solde_credit", c_int64),
    ]


class CEntreeGrandLivre(ctypes.Structure):
    _fields_ = [
        ("date", CDate),
        ("reference", c_char * REFERENCE_LEN),
        ("libelle", c_char * LIBELLE_LEN),
        ("debit", c_int64),
        ("credit", c_int64),
        ("solde_cumule", c_int64),
    ]


class CLigneBilan(ctypes.Structure):
    _fields_ = [
        ("libelle", c_char * BILAN_LIBELLE_LEN),
        ("montant", c_int64),
        ("est_sous_total", c_int),
        ("profondeur", c_int),
    ]


class CLigneCompteResultat(ctypes.Structure):
    _fields_ = [
        ("code", c_char * CODE_LEN),
        ("nom", c_char * NOM_COMPTE_LEN),
        ("rubrique", c_int),
        ("montant", c_int64),
    ]


class CCompteResultat(ctypes.Structure):
    _fields_ = [
        ("lignes", CLigneCompteResultat * CR_MAX_LIGNES),
        ("nb_lignes", c_size_t),
        ("total_rubrique", c_int64 * CR_NB_RUBRIQUES),
        ("total_produits", c_int64),
        ("total_charges", c_int64),
        ("resultat_exploitation", c_int64),
        ("resultat_financier", c_int64),
        ("resultat_exceptionnel", c_int64),
        ("resultat_avant_impot", c_int64),
        ("resultat_net", c_int64),
    ]


class CResultatDetection(ctypes.Structure):
    _fields_ = [
        ("methode", c_int),
        ("gravite", c_int),
        ("id_ligne", c_uint32),
        ("id_compte", c_uint32),
        ("description", c_char * DESCRIPTION_LEN),
        ("score", c_double),
    ]


class CStatChiffreBenford(ctypes.Structure):
    _fields_ = [
        ("numero", c_int),
        ("freq_attendu", c_double),
        ("freq_observe", c_double),
        ("nombre_observe", c_int),
    ]


class CRapportBenford(ctypes.Structure):
    _fields_ = [
        ("stat", CStatChiffreBenford * 9),
        ("chiffre_carree", c_double),
        ("ecart_absolu_moyen", c_double),
        ("taille_echantillon", c_int),
        ("estnormal", c_int),
    ]


class CConfigDetection(ctypes.Structure):
    _fields_ = [
        ("benford_est_active", c_int),
        ("doublons_est_active", c_int),
        ("nombres_ronds_active", c_int),
        ("valeurs_aberrantes_active", c_int),
        ("seuil_ecart_type_extreme", c_double),
        ("seuil_ecart_nombres_ronds", c_double),
        ("fenetre_jours_doublon", c_int),
    ]


# Nom de la structure C -> classe ctypes (utilise pour verifier les tailles)
STRUCTS_C = {
    "DATE": CDate,
    "Compte": CCompte,
    "ligne_journal": CLigneJournal,
    "Ecriture": CEcriture,
    "prop_periode_fiscale": CPeriode,
    "SessionConfig": CSessionConfig,
    "LigneBalance": CLigneBalance,
    "Entree_GrandLivre": CEntreeGrandLivre,
    "ligne_bilan": CLigneBilan,
    "LigneCompteResultat": CLigneCompteResultat,
    "CompteResultat": CCompteResultat,
    "Resultat_Detection": CResultatDetection,
    "StatChiffreBenford": CStatChiffreBenford,
    "Rapport_Benford": CRapportBenford,
    "config_detection": CConfigDetection,
}
