"""Journal : creation, lecture et comptabilisation des ecritures."""
from __future__ import annotations

import ctypes

from bindings import marshaling as m
from bindings.structs import CEcriture
from models.journal_entry import JournalEntry, JournalLine

from .session_service import SessionService


def ecriture_depuis_c(c: CEcriture) -> JournalEntry:
    lignes = [JournalLine(l.compte_id, l.debit, l.credit, m.decode(l.libelle), l.id)
              for l in (c.lignes[i] for i in range(c.nombre_lignes))]
    return JournalEntry(m.date_depuis_c(c.date), m.decode(c.reference), m.decode(c.description),
                        lignes, c.periode_id, bool(c.est_validee), c.id)


class JournalService:
    def __init__(self, session: SessionService):
        self.session = session

    @property
    def _ffi(self):
        return self.session.ffi

    def lister(self) -> list[JournalEntry]:
        h = self.session.exiger_session()
        n = self._ffi.Session_GetEcritureCount(h)
        return [ecriture_depuis_c(self._ffi.Session_avoir_EcritureAt(h, i).contents) for i in range(n)]

    def chercher(self, id_ecriture: int) -> JournalEntry | None:
        p = self._ffi.Session_chercher_Ecriture_Par_ID(self.session.exiger_session(), id_ecriture)
        return ecriture_depuis_c(p.contents) if p else None

    def ajouter(self, ecriture: JournalEntry) -> JournalEntry:
        """Valide puis enregistre l'ecriture (brouillon, ou comptabilisee si est_validee).
        Leve ErreurFinCore avec le message du moteur en cas de refus."""
        c = CEcriture()
        ffi = self._ffi
        ffi.verifier(ffi.ecritures_creer(ctypes.byref(c), ecriture.periode_id, m.date_vers_c(ecriture.date),
                                         m.encode(ecriture.reference), m.encode(ecriture.description)))
        try:
            for l in ecriture.lignes:
                ffi.verifier(ffi.ecritures_ajouter_ligne(ctypes.byref(c), l.compte_id, l.debit, l.credit, m.encode(l.libelle)))
            c.est_validee = 1 if ecriture.est_validee else 0
            c.id = ecriture.id
            ffi.verifier(ffi.Session_AddEcriture(self.session.exiger_session(), ctypes.byref(c)))
            # la session a attribue l'id : c'est la derniere ecriture du tableau
            h = self.session.handle
            dernier = ffi.Session_avoir_EcritureAt(h, ffi.Session_GetEcritureCount(h) - 1).contents
            return ecriture_depuis_c(dernier)
        finally:
            ffi.ecritures_detruire(ctypes.byref(c))     # la session a sa propre copie

    def comptabiliser(self, id_ecriture: int) -> None:
        self._ffi.verifier(self._ffi.Session_comptabiliserEcriture(self.session.exiger_session(), id_ecriture))
