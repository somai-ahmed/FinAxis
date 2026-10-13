"""Resultats du moteur de detection d'anomalies."""
from __future__ import annotations

from dataclasses import dataclass, field
from enum import IntEnum


class MethodeDetection(IntEnum):
    BENFORD = 0
    DOUBLONS = 1
    NOMBRES_RONDS = 2
    VALEURS_ABERRANTES = 3


class Gravite(IntEnum):
    INFO = 0
    FAIBLE = 1
    MOYENNE = 2
    ELEVEE = 3
    CRITIQUE = 4


@dataclass
class DetectionResult:
    methode: MethodeDetection
    gravite: Gravite
    id_compte: int
    id_ligne: int
    description: str
    score: float                # entre 0 et 1


@dataclass
class BenfordDigit:
    chiffre: int
    attendu: float              # frequence theorique
    observe: float              # frequence mesuree
    nombre: int


@dataclass
class BenfordReport:
    chiffres: list[BenfordDigit] = field(default_factory=list)
    chi_carre: float = 0.0
    ecart_absolu_moyen: float = 0.0
    taille_echantillon: int = 0
    est_normal: bool = True


@dataclass
class DetectionConfig:
    benford: bool = True
    doublons: bool = True
    nombres_ronds: bool = True
    valeurs_aberrantes: bool = True
    seuil_ecart_type: float = 3.0
    seuil_nombre_rond: float = 1000.0     # en DT
    fenetre_jours_doublon: int = 7
