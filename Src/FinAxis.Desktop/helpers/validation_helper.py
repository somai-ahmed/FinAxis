"""Validations de saisie cote interface (le moteur C revalide tout de toute facon)."""
from __future__ import annotations

import re

_CODE_COMPTE = re.compile(r"^[0-9]{3,15}$")


def code_compte_valide(code: str) -> bool:
    """Code numerique de 3 a 15 chiffres (meme regle que le moteur)."""
    return bool(_CODE_COMPTE.match(code or ""))


def classe_depuis_code(code: str) -> int | None:
    """Le premier chiffre du code donne la classe PCG (1 a 7)."""
    if code_compte_valide(code) and "1" <= code[0] <= "7":
        return int(code[0])
    return None


def texte_non_vide(texte: str) -> bool:
    return bool(texte and texte.strip())
