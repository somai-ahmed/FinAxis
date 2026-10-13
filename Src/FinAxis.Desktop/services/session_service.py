"""Cycle de vie de la session du moteur (une session = une entreprise)."""
from __future__ import annotations

import ctypes
from dataclasses import dataclass
from datetime import date

from bindings import marshaling as m
from bindings.fincore_ffi import ErreurFinCore, FinCoreLib, charger
from bindings.structs import CSessionConfig


@dataclass
class CompanyConfig:
    nom_entreprise: str = ""
    devise: str = "TND"
    debut_exercice: date | None = None
    fin_exercice: date | None = None


class SessionService:
    """Possede le pointeur Session* et le libere a la fermeture."""

    def __init__(self, ffi: FinCoreLib | None = None):
        self.ffi = ffi or charger()
        self.handle: ctypes.c_void_p | None = None
        self.chemin: str | None = None       # fichier .FinCore associe (None = jamais sauvegardee)

    # ------------------------------------------------------------------
    @property
    def est_ouverte(self) -> bool:
        return bool(self.handle)

    def exiger_session(self) -> ctypes.c_void_p:
        if not self.handle:
            raise ErreurFinCore(-5, self.ffi.message(-5))   # ERR_SESSION_INVALIDE
        return self.handle

    def creer(self, config: CompanyConfig | None = None) -> None:
        """Nouvelle session vide (ferme l'ancienne)."""
        self.fermer()
        sortie = ctypes.c_void_p()
        if config is None:
            self.ffi.verifier(self.ffi.creer_session(None, ctypes.byref(sortie)))
        else:
            self.ffi.verifier(self.ffi.creer_session(ctypes.byref(self._config_vers_c(config)), ctypes.byref(sortie)))
        self.handle = sortie
        self.chemin = None

    def fermer(self) -> None:
        if self.handle:
            self.ffi.detruire_session(self.handle)
        self.handle = None
        self.chemin = None

    def remplacer_handle(self, nouveau: ctypes.c_void_p, chemin: str | None) -> None:
        """Utilise par PersistenceService apres un chargement reussi."""
        self.fermer()
        self.handle = nouveau
        self.chemin = chemin

    # ------------------------------------------------------------------
    @staticmethod
    def _config_vers_c(config: CompanyConfig) -> CSessionConfig:
        c = CSessionConfig()
        m.ecrire_texte(c, "nom_entreprise", config.nom_entreprise)
        m.ecrire_texte(c, "devise", config.devise or "TND")
        c.debut_exercice = m.date_vers_c(config.debut_exercice)
        c.fin_exercice = m.date_vers_c(config.fin_exercice)
        return c

    def lire_config(self) -> CompanyConfig:
        c = self.ffi.Session_avoir_Config(self.exiger_session()).contents
        return CompanyConfig(m.decode(c.nom_entreprise), m.decode(c.devise),
                             m.date_depuis_c(c.debut_exercice), m.date_depuis_c(c.fin_exercice))

    def ecrire_config(self, config: CompanyConfig) -> None:
        self.ffi.verifier(self.ffi.Session_SetConfig(self.exiger_session(), ctypes.byref(self._config_vers_c(config))))

    def reinitialiser(self) -> None:
        """Vide comptes, periodes et journal (la configuration est conservee)."""
        self.ffi.verifier(self.ffi.reinitialiser_session(self.exiger_session()))

    def __del__(self):
        try:
            self.fermer()
        except Exception:
            pass
