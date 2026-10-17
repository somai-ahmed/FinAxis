"""Fenetre de base : branchee le rafraichissement sur le signal 'donnees modifiees'."""
from __future__ import annotations

from datetime import date

from PySide6.QtCore import QDate
from PySide6.QtWidgets import QComboBox, QDateEdit, QWidget

from ui.context import AppContext


class BaseWindow(QWidget):
    titre = "FinCore"

    def __init__(self, ctx: AppContext, parent=None):
        super().__init__(parent)
        self.ctx = ctx
        self.setWindowTitle(self.titre)
        ctx.modifie.connect(self.rafraichir)

    def rafraichir(self) -> None:
        """A redefinir : recharge les donnees depuis les services."""

    # --- petits outils partages ---
    @staticmethod
    def champ_date(valeur: date | None = None) -> QDateEdit:
        e = QDateEdit()
        e.setCalendarPopup(True)
        e.setDisplayFormat("dd/MM/yyyy")
        v = valeur or date.today()
        e.setDate(QDate(v.year, v.month, v.day))
        return e

    @staticmethod
    def date_de(e: QDateEdit) -> date:
        return e.date().toPython()

    def remplir_periodes(self, combo: QComboBox, avec_toutes: bool = False) -> None:
        courant = combo.currentData()
        combo.clear()
        if avec_toutes:
            combo.addItem("Sans période", 0)
        for p in self.ctx.periodes.lister():
            combo.addItem(f"{p.nom}  ({p.date_debut:%d/%m/%Y} → {p.date_fin:%d/%m/%Y})", p.id)
        i = combo.findData(courant)
        if i >= 0:
            combo.setCurrentIndex(i)

    def plage_exercice(self) -> tuple[date, date]:
        """Debut / fin de l'exercice configure (annee en cours par defaut)."""
        c = self.ctx.session.lire_config()
        an = date.today().year
        return c.debut_exercice or date(an, 1, 1), c.fin_exercice or date(an, 12, 31)
