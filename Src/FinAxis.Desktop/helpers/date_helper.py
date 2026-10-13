"""Dates : conversions texte <-> datetime.date (formats utilises dans l'interface)."""
from __future__ import annotations

from datetime import date, datetime

FORMAT_AFFICHAGE = "%d/%m/%Y"
FORMAT_ISO = "%Y-%m-%d"


def analyser_date(texte: str) -> date | None:
    """Accepte JJ/MM/AAAA ou AAAA-MM-JJ ; None si invalide."""
    texte = (texte or "").strip()
    for fmt in (FORMAT_AFFICHAGE, FORMAT_ISO):
        try:
            return datetime.strptime(texte, fmt).date()
        except ValueError:
            continue
    return None


def formater_date(d: date | None) -> str:
    return d.strftime(FORMAT_AFFICHAGE) if d else ""


def debut_exercice(annee: int) -> date:
    return date(annee, 1, 1)


def fin_exercice(annee: int) -> date:
    return date(annee, 12, 31)
