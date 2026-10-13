"""Periodes fiscales."""
from __future__ import annotations

import ctypes
from datetime import date

from bindings import marshaling as m
from bindings.structs import CPeriode
from models.period import Period, StatutPeriode

from .session_service import SessionService


def periode_depuis_c(c: CPeriode) -> Period:
    return Period(c.id, m.decode(c.nom), m.date_depuis_c(c.date_debut), m.date_depuis_c(c.date_fin), StatutPeriode(c.statut))


class PeriodService:
    def __init__(self, session: SessionService):
        self.session = session

    @property
    def _ffi(self):
        return self.session.ffi

    def lister(self) -> list[Period]:
        h = self.session.exiger_session()
        n = self._ffi.Session_avoir_nombre_periodes(h)
        return [periode_depuis_c(self._ffi.Session_avoir_PeriodeAt(h, i).contents) for i in range(n)]

    def chercher(self, id_periode: int) -> Period | None:
        p = self._ffi.Session_chercher_Periode_Par_ID(self.session.exiger_session(), id_periode)
        return periode_depuis_c(p.contents) if p else None

    def periode_pour_date(self, d: date) -> Period | None:
        return next((p for p in self.lister() if p.contient(d)), None)

    def ajouter(self, nom: str, debut: date, fin: date, id_periode: int = 0) -> Period:
        """Ajoute une periode (id automatique si 0)."""
        if id_periode == 0:
            id_periode = max((p.id for p in self.lister()), default=0) + 1
        c = CPeriode()
        self._ffi.verifier(self._ffi.periodes_creer(ctypes.byref(c), id_periode, m.encode(nom), m.date_vers_c(debut), m.date_vers_c(fin)))
        self._ffi.verifier(self._ffi.Session_ajouterPeriode(self.session.exiger_session(), ctypes.byref(c)))
        return self.chercher(id_periode)

    def generer_mensuelles(self, annee: int) -> list[Period]:
        """Cree les 12 periodes mensuelles de l'annee."""
        tampon = (CPeriode * 12)()
        premier = max((p.id for p in self.lister()), default=0) + 1
        self._ffi.verifier(self._ffi.periodes_generer_mensuelles(annee, premier, tampon, 12))
        for c in tampon:
            self._ffi.verifier(self._ffi.Session_ajouterPeriode(self.session.exiger_session(), ctypes.byref(c)))
        return [periode_depuis_c(c) for c in tampon]

    def cloturer(self, id_periode: int) -> None:
        self._ffi.verifier(self._ffi.Session_cloturerPeriode(self.session.exiger_session(), id_periode))

    def rouvrir(self, id_periode: int) -> None:
        self._ffi.verifier(self._ffi.Session_rouvrirPeriode(self.session.exiger_session(), id_periode))

    def verrouiller(self, id_periode: int) -> None:
        self._ffi.verifier(self._ffi.Session_verrouillerPeriode(self.session.exiger_session(), id_periode))
