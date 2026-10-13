"""Periode fiscale (miroir de prop_periode_fiscale)."""
from __future__ import annotations

from dataclasses import dataclass
from datetime import date
from enum import IntEnum


class StatutPeriode(IntEnum):
    OUVERTE = 1
    CLOTUREE = 2
    VERROUILLEE = 3


@dataclass
class Period:
    id: int
    nom: str
    date_debut: date
    date_fin: date
    statut: StatutPeriode = StatutPeriode.OUVERTE

    def contient(self, d: date) -> bool:
        return self.date_debut <= d <= self.date_fin
