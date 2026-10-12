"""Petits outils de conversion entre Python et les structures C.

Aucune logique metier ici : uniquement texte <-> octets, dates et tableaux.
"""
from __future__ import annotations

import ctypes
from datetime import date

from .structs import CDate

# Le moteur compte en millimes : 1 DT = 1000 unites (Monnaie_Unite dans types.h)
MONNAIE_UNITE = 1000


def encode(texte: str | None) -> bytes:
    """str -> octets UTF-8 (None devient une chaine vide)."""
    return (texte or "").encode("utf-8")


def decode(tampon) -> str:
    """Tampon char[] du C -> str (ctypes s'arrete deja au premier '\\0')."""
    if isinstance(tampon, bytes):
        return tampon.decode("utf-8", errors="replace")
    return bytes(tampon).split(b"\0", 1)[0].decode("utf-8", errors="replace")


def ecrire_texte(structure: ctypes.Structure, champ: str, texte: str) -> None:
    """Copie un texte dans un champ char[N], tronque sans couper un caractere UTF-8."""
    taille = getattr(type(structure), champ).size   # taille du champ char[N]
    brut = encode(texte)[: taille - 1]
    while brut:
        try:
            brut.decode("utf-8")
            break
        except UnicodeDecodeError:
            brut = brut[:-1]
    setattr(structure, champ, brut)


def date_vers_c(d: date | None) -> CDate:
    """datetime.date -> DATE (None donne une date a zero)."""
    c = CDate()
    if d is not None:
        c.annee, c.mois, c.jour = d.year, d.month, d.day
    return c


def date_depuis_c(c: CDate) -> date | None:
    """DATE -> datetime.date (None si la date est vide ou invalide)."""
    try:
        return date(c.annee, c.mois, c.jour)
    except ValueError:
        return None


def tableau(type_c, nombre: int):
    """Alloue un tableau C de `nombre` elements (au moins 1 : pointeur toujours valide)."""
    return (type_c * max(nombre, 1))()
