"""Ecriture du journal (miroir de Ecriture / ligne_journal)."""
from __future__ import annotations

from dataclasses import dataclass, field
from datetime import date


@dataclass
class JournalLine:
    compte_id: int
    debit: int = 0  # millimes
    credit: int = 0 # millimes
    libelle: str = ""
    id: int = 0


@dataclass
class JournalEntry:
    date: date
    reference: str
    description: str
    lignes: list[JournalLine] = field(default_factory=list)
    periode_id: int = 0         # 0 = sans periode
    est_validee: bool = False   # False = brouillon
    id: int = 0

    @property
    def total_debit(self) -> int:
        return sum(l.debit for l in self.lignes)

    @property
    def total_credit(self) -> int:
        return sum(l.credit for l in self.lignes)

    @property
    def est_equilibree(self) -> bool:
        return bool(self.lignes) and self.total_debit == self.total_credit
