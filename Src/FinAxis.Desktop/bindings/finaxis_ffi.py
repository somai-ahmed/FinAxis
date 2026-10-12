"""Chargement de FinAxis_ae.dll (ou .so / .dylib) et prototypes ctypes.

C'est le SEUL module qui touche directement la DLL : le reste de
l'application passe par les services (services/).
"""
from __future__ import annotations

import ctypes
import os
import sys
from ctypes import (POINTER, c_char_p, c_int, c_int32, c_int64, c_size_t,
                    c_uint32, c_void_p)
from pathlib import Path

from .structs import (STRUCTS_C, CCompte, CCompteResultat, CConfigDetection,
                      CDate, CEcriture, CEntreeGrandLivre, CLigneBalance,
                      CLigneBilan, CPeriode, CRapportBenford,
                      CResultatDetection, CSessionConfig)

VERSION_ATTENDUE = "1.0"   # majeure.mineure du moteur acceptee par ce client

_NOMS_BIBLIOTHEQUE = {
    "win32": ["FinCore_ae.dll"],
    "darwin": ["FinCore_ae.dylib", "FinCore_ae.so"],
}.get(sys.platform, ["FinCore_ae.so", "libFinCore_ae.so"])


class ErreurFinCore(Exception):
    """Erreur renvoyee par le moteur C (code Etat negatif)."""

    def __init__(self, code: int, message: str):
        super().__init__(f"[{code}] {message}")
        self.code = code
        self.message = message


class BibliothequeIntrouvable(RuntimeError):
    """La DLL du moteur est absente ou incompatible."""


def _candidats() -> list[Path]:
    chemins: list[Path] = []
    env = os.environ.get("FINCORE_DLL")
    if env:
        chemins.append(Path(env))
    if getattr(sys, "_MEIPASS", None):               # application empaquetee (PyInstaller)
        chemins += [Path(sys._MEIPASS) / n for n in _NOMS_BIBLIOTHEQUE]
    racine_native = Path(__file__).resolve().parents[2] / "FinCore.Native"
    for sous in ("build", "build/Release", "build/Debug", "."):
        chemins += [racine_native / sous / n for n in _NOMS_BIBLIOTHEQUE]
    return chemins


def trouver_bibliotheque() -> Path:
    for chemin in _candidats():
        if chemin.is_file():
            return chemin
    raise BibliothequeIntrouvable(
        "FinCore_ae introuvable. Compilez le moteur (cmake) ou definissez FINCORE_DLL. "
        "Chemins essayes : " + ", ".join(str(c) for c in _candidats()))


class FinCoreLib:
    """Enveloppe la DLL : prototypes declares, tailles de structures verifiees."""

    def __init__(self, chemin: str | os.PathLike | None = None):
        self.chemin = Path(chemin) if chemin else trouver_bibliotheque()
        if sys.platform == "win32" and hasattr(os, "add_dll_directory"):
            os.add_dll_directory(str(self.chemin.parent))   # dependances voisines (MinGW)
        self.lib = ctypes.CDLL(str(self.chemin))
        self._declarer()
        self._verifier_compatibilite()

    def _declarer(self) -> None:
        L, P, S = self.lib, POINTER, c_void_p   # S = Session*

        def p(nom, res, args):
            f = getattr(L, nom)
            f.restype, f.argtypes = res, args

        p("FinCore_Version", c_char_p, [])
        p("FinCore_TailleStruct", c_size_t, [c_char_p])
        p("GetErrorMessage", c_char_p, [c_int])

        # session
        p("creer_session", c_int, [P(CSessionConfig), P(c_void_p)])
        p("detruire_session", None, [S])
        p("reinitialiser_session", c_int, [S])
        p("Session_avoir_Config", P(CSessionConfig), [S])
        p("Session_SetConfig", c_int, [S, P(CSessionConfig)])
        p("Session_avoir_le_dernier_Erreur", c_int, [S])
        p("Session_nouvel_id_compte", c_uint32, [S])
        p("Session_nouvel_id_ecriture", c_uint32, [S])

        # plan comptable
        p("comptes_creer", c_int, [P(CCompte), c_char_p, c_char_p, c_int, c_int, c_int, c_uint32])
        p("Session_ajouterCompte", c_int, [S, P(CCompte)])
        p("Session_avoir_nombre_comptes", c_size_t, [S])
        p("Session_avoir_CompteAt", P(CCompte), [S, c_size_t])
        p("Session_chercher_Compte_Par_ID", P(CCompte), [S, c_int32])
        p("Session_chercher_Compte_Par_Code", P(CCompte), [S, c_char_p])

        # journal
        p("ecritures_creer", c_int, [P(CEcriture), c_uint32, CDate, c_char_p, c_char_p])
        p("ecritures_ajouter_ligne", c_int, [P(CEcriture), c_uint32, c_int64, c_int64, c_char_p])
        p("ecritures_detruire", None, [P(CEcriture)])
        p("ecritures_valider", c_int, [P(CEcriture)])
        p("Session_AddEcriture", c_int, [S, P(CEcriture)])
        p("Session_GetEcritureCount", c_size_t, [S])
        p("Session_avoir_EcritureAt", P(CEcriture), [S, c_size_t])
        p("Session_chercher_Ecriture_Par_ID", P(CEcriture), [S, c_uint32])
        p("Session_comptabiliserEcriture", c_int, [S, c_uint32])

        # periodes
        p("periodes_creer", c_int, [P(CPeriode), c_uint32, c_char_p, CDate, CDate])
        p("periodes_generer_mensuelles", c_int, [c_int32, c_uint32, P(CPeriode), c_size_t])
        p("Session_ajouterPeriode", c_int, [S, P(CPeriode)])
        p("Session_avoir_nombre_periodes", c_size_t, [S])
        p("Session_avoir_PeriodeAt", P(CPeriode), [S, c_size_t])
        p("Session_chercher_Periode_Par_ID", P(CPeriode), [S, c_uint32])
        p("Session_cloturerPeriode", c_int, [S, c_uint32])
        p("Session_rouvrirPeriode", c_int, [S, c_uint32])
        p("Session_verrouillerPeriode", c_int, [S, c_uint32])

        # rapports
        p("Session_GenererBalance", c_int, [S, CDate, CDate, P(CLigneBalance), c_size_t, P(c_size_t)])
        p("Session_GenererGrandLivre", c_int, [S, c_uint32, CDate, CDate, P(CEntreeGrandLivre), c_size_t, P(c_size_t)])
        p("Session_GenererBilan", c_int, [S, CDate, P(CLigneBilan), c_size_t, P(c_size_t), P(c_int64), P(c_int64)])
        p("Session_GenererCompteResultat", c_int, [S, CDate, CDate, P(CCompteResultat)])
        p("bilan_verifier", c_int, [c_int64, c_int64])

        # detection
        p("detection_config_par_defaut", None, [P(CConfigDetection)])
        p("execute_detection", c_int, [S, c_uint32, P(CConfigDetection), P(P(CResultatDetection)), P(c_size_t)])
        p("Executer_Analyse_Benford", c_int, [S, c_uint32, P(CRapportBenford)])
        p("liberer_resultats_detection", None, [P(CResultatDetection), c_size_t])

        # persistance
        p("Session_Sauvegarder", c_int, [S, c_char_p])
        p("Session_Charger", c_int, [c_char_p, P(c_void_p)])

    def _verifier_compatibilite(self) -> None:
        version = self.version()
        if version.split(".")[:2] != VERSION_ATTENDUE.split("."):
            raise BibliothequeIntrouvable(f"Version du moteur incompatible : {version} (attendue {VERSION_ATTENDUE}.x)")
        for nom_c, classe in STRUCTS_C.items():
            taille_c = self.lib.FinCore_TailleStruct(nom_c.encode())
            if taille_c != ctypes.sizeof(classe):
                raise BibliothequeIntrouvable(
                    f"Structure {nom_c} : taille C = {taille_c} octets, miroir Python = {ctypes.sizeof(classe)} octets")

    def version(self) -> str:
        return self.lib.FinCore_Version().decode()

    def message(self, code: int) -> str:
        return self.lib.GetErrorMessage(code).decode("utf-8", errors="replace")

    def verifier(self, code: int) -> None:
        """Leve ErreurFinCore si le code est une erreur (negatif)."""
        if code != 0:
            raise ErreurFinCore(code, self.message(code))

    def __getattr__(self, nom: str):
        # acces direct aux fonctions C : ffi.Session_AddEcriture(...)
        return getattr(self.__dict__["lib"], nom)


_instance: FinCoreLib | None = None


def charger(chemin: str | os.PathLike | None = None) -> FinCoreLib:
    """Instance partagee de la DLL (chargee au premier appel)."""
    global _instance
    if _instance is None or chemin is not None:
        _instance = FinCoreLib(chemin)
    return _instance
