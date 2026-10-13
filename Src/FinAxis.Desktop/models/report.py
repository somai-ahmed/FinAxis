"""Lignes des rapports financiers (montants en millimes)."""
from __future__ import annotations

from dataclasses import dataclass, field
from datetime import date


@dataclass
class BalanceRow:
    compte_id: int
    code: str
    nom: str
    total_debit: int
    total_credit: int
    solde_debit: int
    solde_credit: int


@dataclass
class LedgerRow:
    date: date | None
    reference: str
    libelle: str
    debit: int
    credit: int
    solde_cumule: int


@dataclass
class BilanRow:
    libelle: str
    montant: int
    est_sous_total: bool
    profondeur: int


@dataclass
class Bilan:
    lignes: list[BilanRow] = field(default_factory=list)
    total_actif: int = 0
    total_passif: int = 0

    @property
    def est_equilibre(self) -> bool:
        return self.total_actif == self.total_passif


@dataclass
class IncomeLine:
    code: str
    nom: str
    rubrique: int
    montant: int


@dataclass
class IncomeStatement:
    lignes: list[IncomeLine] = field(default_factory=list)
    total_rubrique: list[int] = field(default_factory=lambda: [0] * 7)
    total_produits: int = 0
    total_charges: int = 0
    resultat_exploitation: int = 0
    resultat_financier: int = 0
    resultat_exceptionnel: int = 0
    resultat_avant_impot: int = 0
    resultat_net: int = 0


RUBRIQUES_CR = [
    "Produits d'exploitation", "Charges d'exploitation",
    "Produits financiers", "Charges financières",
    "Produits exceptionnels", "Charges exceptionnelles",
    "Impôt sur les bénéfices",
]
