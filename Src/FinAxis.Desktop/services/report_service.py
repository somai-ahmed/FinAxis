"""Rapports financiers : balance, grand livre, bilan, compte de resultat."""
from __future__ import annotations

import ctypes
from datetime import date

from bindings import marshaling as m
from bindings.fincore_ffi import ErreurFinCore
from bindings.structs import (BILAN_NB_LIGNES, CCompteResultat, CEntreeGrandLivre,
                              CLigneBalance, CLigneBilan)
from models.report import (Bilan, BilanRow, BalanceRow, IncomeLine,
                           IncomeStatement, LedgerRow)

from .session_service import SessionService

ERR_AUCUNE_DONNEE_PERIODE = -401
ERR_TRES_PETIT_BUFFER = -7


class ReportService:
    def __init__(self, session: SessionService):
        self.session = session

    @property
    def _ffi(self):
        return self.session.ffi

    def balance(self, debut: date, fin: date) -> list[BalanceRow]:
        """Balance generale. Liste vide si aucune ecriture comptabilisee sur la periode."""
        h = self.session.exiger_session()
        n = self._ffi.Session_avoir_nombre_comptes(h)
        tampon = m.tableau(CLigneBalance, n)
        nb = ctypes.c_size_t()
        etat = self._ffi.Session_GenererBalance(h, m.date_vers_c(debut), m.date_vers_c(fin), tampon, n, ctypes.byref(nb))
        if etat == ERR_AUCUNE_DONNEE_PERIODE:
            return []
        self._ffi.verifier(etat)
        return [BalanceRow(l.compte.id, m.decode(l.compte.code), m.decode(l.compte.nom),
                           l.total_debit, l.total_credit, l.solde_debit, l.solde_credit)
                for l in tampon[: nb.value]]

    def grand_livre(self, id_compte: int, debut: date, fin: date) -> list[LedgerRow]:
        h = self.session.exiger_session()
        nb = ctypes.c_size_t()
        # 1er appel avec capacite 0 : le moteur annonce la taille necessaire
        etat = self._ffi.Session_GenererGrandLivre(h, id_compte, m.date_vers_c(debut), m.date_vers_c(fin), None, 0, ctypes.byref(nb))
        if etat == ERR_AUCUNE_DONNEE_PERIODE or (etat == 0 and nb.value == 0):
            return []                     # aucun mouvement sur la plage : grand livre vide
        if etat not in (0, ERR_TRES_PETIT_BUFFER):
            self._ffi.verifier(etat)
        n = nb.value
        tampon = m.tableau(CEntreeGrandLivre, n)
        self._ffi.verifier(self._ffi.Session_GenererGrandLivre(h, id_compte, m.date_vers_c(debut), m.date_vers_c(fin), tampon, n, ctypes.byref(nb)))
        return [LedgerRow(m.date_depuis_c(e.date), m.decode(e.reference), m.decode(e.libelle), e.debit, e.credit, e.solde_cumule)
                for e in tampon[: nb.value]]

    def bilan(self, date_arret: date) -> Bilan:
        h = self.session.exiger_session()
        tampon = (CLigneBilan * BILAN_NB_LIGNES)()
        nb = ctypes.c_size_t()
        actif, passif = ctypes.c_int64(), ctypes.c_int64()
        self._ffi.verifier(self._ffi.Session_GenererBilan(h, m.date_vers_c(date_arret), tampon, BILAN_NB_LIGNES,
                                                          ctypes.byref(nb), ctypes.byref(actif), ctypes.byref(passif)))
        lignes = [BilanRow(m.decode(l.libelle), l.montant, bool(l.est_sous_total), l.profondeur) for l in tampon[: nb.value]]
        return Bilan(lignes, actif.value, passif.value)

    def compte_resultat(self, debut: date, fin: date) -> IncomeStatement:
        h = self.session.exiger_session()
        c = CCompteResultat()
        self._ffi.verifier(self._ffi.Session_GenererCompteResultat(h, m.date_vers_c(debut), m.date_vers_c(fin), ctypes.byref(c)))
        lignes = [IncomeLine(m.decode(l.code), m.decode(l.nom), l.rubrique, l.montant) for l in c.lignes[: c.nb_lignes]]
        return IncomeStatement(lignes, list(c.total_rubrique), c.total_produits, c.total_charges,
                               c.resultat_exploitation, c.resultat_financier, c.resultat_exceptionnel,
                               c.resultat_avant_impot, c.resultat_net)
