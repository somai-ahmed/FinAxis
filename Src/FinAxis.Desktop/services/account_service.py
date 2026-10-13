"""Plan comptable : lecture, creation, recherche."""
from __future__ import annotations

import ctypes

from bindings import marshaling as m
from bindings.fincore_ffi import ErreurFinCore
from bindings.structs import CCompte
from models.account import Account, ClasseCompte, SoldeNormal, TypeCompte

from .session_service import SessionService


def compte_depuis_c(c: CCompte) -> Account:
    return Account(
        code=m.decode(c.code), nom=m.decode(c.nom), classe=ClasseCompte(c.classe),
        type=TypeCompte(c.type), solde_normal=SoldeNormal(c.solde_normal),
        parent_id=c.parent_id, id=c.id, solde=c.solde, est_active=bool(c.est_active))


class AccountService:
    def __init__(self, session: SessionService):
        self.session = session

    @property
    def _ffi(self):
        return self.session.ffi

    def lister(self) -> list[Account]:
        h = self.session.exiger_session()
        n = self._ffi.Session_avoir_nombre_comptes(h)
        return [compte_depuis_c(self._ffi.Session_avoir_CompteAt(h, i).contents) for i in range(n)]

    def chercher_par_code(self, code: str) -> Account | None:
        p = self._ffi.Session_chercher_Compte_Par_Code(self.session.exiger_session(), m.encode(code))
        return compte_depuis_c(p.contents) if p else None

    def chercher_par_id(self, id_compte: int) -> Account | None:
        p = self._ffi.Session_chercher_Compte_Par_ID(self.session.exiger_session(), id_compte)
        return compte_depuis_c(p.contents) if p else None

    def ajouter(self, compte: Account) -> Account:
        """Cree le compte dans la session et retourne sa version enregistree (avec son id)."""
        c = CCompte()
        self._ffi.verifier(self._ffi.comptes_creer(
            ctypes.byref(c), m.encode(compte.code), m.encode(compte.nom),
            int(compte.classe), int(compte.type), int(compte.solde_normal), compte.parent_id))
        c.est_active = 1 if compte.est_active else 0
        self._ffi.verifier(self._ffi.Session_ajouterCompte(self.session.exiger_session(), ctypes.byref(c)))
        cree = self.chercher_par_code(compte.code)
        if cree is None:
            raise ErreurFinCore(-11, self._ffi.message(-11))
        return cree

    def charger_plan_par_defaut(self, plan: list[tuple[str, str, int, int, int]] | None = None) -> int:
        """Ajoute un plan comptable de base (tuples code, nom, classe, type, solde_normal). Retourne le nombre ajoute."""
        ajoutes = 0
        for code, nom, classe, type_, sens in (plan or PLAN_PAR_DEFAUT):
            if self.chercher_par_code(code) is None:
                self.ajouter(Account(code, nom, ClasseCompte(classe), TypeCompte(type_), SoldeNormal(sens)))
                ajoutes += 1
        return ajoutes


# Petit plan inspire du SCE tunisien (classe, type, sens normal)
PLAN_PAR_DEFAUT = [
    ("101", "Capital social", 1, 3, 1),
    ("120", "Résultats reportés", 1, 3, 1),
    ("215", "Matériel et outillage", 2, 1, 0),
    ("311", "Marchandises", 3, 1, 0),
    ("401", "Fournisseurs", 4, 2, 1),
    ("411", "Clients", 4, 1, 0),
    ("436", "État, TVA récupérable", 4, 1, 0),
    ("437", "État, TVA collectée", 4, 2, 1),
    ("512", "Banque", 5, 1, 0),
    ("531", "Caisse", 5, 1, 0),
    ("601", "Achats de marchandises", 6, 5, 0),
    ("624", "Transports", 6, 5, 0),
    ("641", "Salaires", 6, 5, 0),
    ("661", "Charges d'intérêts", 6, 5, 0),
    ("701", "Ventes de marchandises", 7, 4, 1),
    ("706", "Prestations de services", 7, 4, 1),
    ("761", "Produits de placements", 7, 4, 1),
]
