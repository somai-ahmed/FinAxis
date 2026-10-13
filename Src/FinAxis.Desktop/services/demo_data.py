"""Jeu de donnees de demonstration : une PME fictive, 2025, avec des anomalies cachees.

Les montants "normaux" suivent la loi de Benford (10^u, u uniforme) pour que
l'analyse de Benford reste conforme ; on ajoute ensuite a la main :
  - un doublon de facture fournisseur,
  - trois montants tres ronds,
  - une depense aberrante.
"""

from __future__ import annotations

import random
from datetime import date

from models.journal_entry import JournalEntry, JournalLine
from services.session_service import CompanyConfig

ANNEE = 2025


def _montant(rng: random.Random, bas: float = 2.0, haut: float = 4.3) -> int:
    """Montant en millimes, premier chiffre selon Benford, 3 decimales non rondes."""
    dt = 10 ** rng.uniform(bas, haut)
    return int(round(dt * 1000)) + rng.randint(1, 9)


def remplir_demo(ctx) -> int:
    """Cree une nouvelle session de demo dans le contexte. Retourne le nombre d'ecritures creees."""
    rng = random.Random(2025)
    ctx.session.creer(CompanyConfig("Société Démo SARL", "TND", date(ANNEE, 1, 1), date(ANNEE, 12, 31)))
    ctx.comptes.charger_plan_par_defaut()
    ctx.periodes.generer_mensuelles(ANNEE)
    c = {x.code: x.id for x in ctx.comptes.lister()}
    nb = 0

    def ecr(mois, jour, ref, desc, debit_code, credit_code, montant):
        nonlocal nb
        d = date(ANNEE, mois, jour)
        p = ctx.periodes.periode_pour_date(d)
        ctx.journal.ajouter(JournalEntry(d, ref, desc, [JournalLine(c[debit_code], montant, 0, desc), JournalLine(c[credit_code], 0, montant, desc)],
                                         p.id if p else 0, True))
        nb += 1

    ecr(1, 2, "CAP-1", "Libération du capital", "512", "101", 50_000_000)
    for mois in range(1, 13):
        for k in range(4):
            ecr(mois, 3 + k * 6, f"FV-{mois:02d}{k}", "Facture client", "411", "701", _montant(rng))
        for k in range(3):
            ecr(mois, 4 + k * 7, f"FA-{mois:02d}{k}", "Facture fournisseur", "601", "401", _montant(rng))
        ecr(mois, 25, f"SAL-{mois:02d}", "Salaires du mois", "641", "512", 2_400_000 + rng.randint(1, 999))
        ecr(mois, 27, f"ENC-{mois:02d}", "Encaissement client", "512", "411", _montant(rng, 2.5, 3.8))

    # --- anomalies plantees (en mars) ---
    ecr(3, 12, "FA-DUP1", "Facture fournisseur Alpha", "601", "401", 4_573_210)
    ecr(3, 14, "FA-DUP2", "Facture fournisseur Alpha", "601", "401", 4_573_210)        # doublon a 2 jours
    ecr(3, 20, "DIV-001", "Frais divers", "624", "531", 5_000_000)                     # 5 000 DT pile
    ecr(3, 21, "DIV-002", "Frais divers", "624", "531", 10_000_000)                    # 10 000 DT pile
    ecr(3, 22, "DIV-003", "Frais divers", "624", "531", 20_000_000)                    # 20 000 DT pile
    ecr(9, 18, "FA-GROS", "Prestation exceptionnelle", "624", "401", 98_765_432)       # tres au-dessus des autres transports
    # historique du compte 624 dans le meme mois que l'anomalie : la detection compare les lignes d'un compte DANS la periode
    for k in range(14):
        ecr(9, 1 + k, f"TR-{k}", "Transport", "624", "531", _montant(rng, 1.8, 2.6))
    ctx.signaler_modification()
    return nb
