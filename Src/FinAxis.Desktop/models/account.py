"""Compte du plan comptable (miroir de la structure Compte)."""
from __future__ import annotations

from dataclasses import dataclass
from enum import IntEnum


class ClasseCompte(IntEnum):
    CAPITAUX = 1
    ACTIFS_NON_COURANTS = 2
    STOCKS = 3
    TIERS = 4
    TRESORERIE = 5
    CHARGES = 6
    PRODUITS = 7


class TypeCompte(IntEnum):
    ACTIF = 1
    PASSIF = 2
    CAPITAUX = 3
    PRODUIT = 4
    CHARGE = 5


class SoldeNormal(IntEnum):
    DEBITEUR = 0
    CREDITEUR = 1


@dataclass
class Account:
    code: str
    nom: str
    classe: ClasseCompte
    type: TypeCompte
    solde_normal: SoldeNormal
    parent_id: int = 0
    id: int = 0                 # 0 = pas encore enregistre dans la session
    solde: int = 0              # millimes
    est_active: bool = True

    @property
    def libelle(self) -> str:
        return f"{self.code} - {self.nom}"
