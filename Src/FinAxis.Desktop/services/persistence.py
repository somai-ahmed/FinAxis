"""Sauvegarde / chargement des fichiers .FinAxis (JSON)."""
from __future__ import annotations

import ctypes
import os

from bindings.marshaling import encode

from .session_service import SessionService

EXTENSION = ".FinCore"


class PersistenceService:
    def __init__(self, session: SessionService):
        self.session = session

    def sauvegarder(self, chemin: str | None = None) -> str:
        """Ecrit la session ; sans chemin, reutilise le dernier fichier. Retourne le chemin utilise."""
        chemin = chemin or self.session.chemin
        if not chemin:
            raise ValueError("Aucun fichier associé à la session : indiquez un chemin.")
        if not chemin.lower().endswith(EXTENSION.lower()):
            chemin += EXTENSION
        ffi = self.session.ffi
        ffi.verifier(ffi.Session_Sauvegarder(self.session.exiger_session(), encode(os.fspath(chemin))))
        self.session.chemin = chemin
        return chemin

    def charger(self, chemin: str) -> None:
        """Remplace la session courante par le contenu du fichier."""
        ffi = self.session.ffi
        sortie = ctypes.c_void_p()
        ffi.verifier(ffi.Session_Charger(encode(os.fspath(chemin)), ctypes.byref(sortie)))
        self.session.remplacer_handle(sortie, os.fspath(chemin))
