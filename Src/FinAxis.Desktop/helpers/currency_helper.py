"""Montants : le moteur compte en millimes (int), l'interface en dinars (Decimal)."""
from __future__ import annotations

from decimal import ROUND_HALF_UP, Decimal, InvalidOperation

MONNAIE_UNITE = 1000   # 1 DT = 1000 millimes


def millimes_vers_decimal(millimes: int) -> Decimal:
    return Decimal(millimes) / MONNAIE_UNITE


def decimal_vers_millimes(montant: Decimal | float | int | str) -> int:
    """Decimal (DT) -> millimes, arrondi a 3 decimales (0.5 arrondi vers le haut)."""
    d = Decimal(str(montant)) if not isinstance(montant, Decimal) else montant
    return int((d * MONNAIE_UNITE).quantize(Decimal(1), rounding=ROUND_HALF_UP))


def analyser_montant(texte: str) -> int | None:
    """'1 234,500' / '1234.5' / '-12' -> millimes ; None si le texte n'est pas un montant."""
    if texte is None:
        return None
    propre = texte.strip().replace("\u00a0", "").replace(" ", "").replace(",", ".")
    if not propre:
        return None
    try:
        return decimal_vers_millimes(Decimal(propre))
    except InvalidOperation:
        return None


def formater_montant(millimes: int, devise: str = "", zero_vide: bool = False) -> str:
    """12345678 -> '12 345,678' (separateur de milliers = espace, virgule decimale)."""
    if zero_vide and millimes == 0:
        return ""
    signe = "-" if millimes < 0 else ""
    absolu = abs(millimes)
    entier, frac = divmod(absolu, MONNAIE_UNITE)
    texte = f"{signe}{entier:,}".replace(",", " ") + f",{frac:03d}"
    return f"{texte} {devise}".rstrip()
