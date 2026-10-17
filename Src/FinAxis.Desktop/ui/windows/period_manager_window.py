"""Periodes fiscales : creation, cloture, reouverture, verrouillage."""
from __future__ import annotations

from datetime import date

from PySide6.QtWidgets import (QHBoxLayout, QLineEdit, QMessageBox, QPushButton, QSpinBox,
                               QVBoxLayout)

from models.period import StatutPeriode
from ui.context import afficher_erreur
from ui.widgets.report_viewer_widget import ReportViewerWidget

from .base_window import BaseWindow


class PeriodManagerWindow(BaseWindow):
    titre = "Périodes fiscales"

    def __init__(self, ctx, parent=None):
        super().__init__(ctx, parent)
        v = QVBoxLayout(self)
        self.table = ReportViewerWidget()
        v.addWidget(self.table)

        creer = QHBoxLayout()
        self.annee = QSpinBox(); self.annee.setRange(1990, 2100); self.annee.setValue(date.today().year)
        b12 = QPushButton("Générer les 12 mois")
        b12.clicked.connect(self.generer)
        self.nom = QLineEdit(); self.nom.setPlaceholderText("Nom d'une période personnalisée")
        self.debut, self.fin = self.champ_date(date(date.today().year, 1, 1)), self.champ_date(date(date.today().year, 12, 31))
        bp = QPushButton("Ajouter")
        bp.clicked.connect(self.ajouter)
        for w in (self.annee, b12, self.nom, self.debut, self.fin, bp):
            creer.addWidget(w)
        v.addLayout(creer)

        actions = QHBoxLayout()
        for texte, fn in (("Clôturer", self.cloturer), ("Rouvrir", self.rouvrir), ("Verrouiller (définitif)", self.verrouiller)):
            b = QPushButton(texte)
            b.clicked.connect(fn)
            actions.addWidget(b)
        v.addLayout(actions)
        self.rafraichir()

    def rafraichir(self) -> None:
        if not self.ctx.session.est_ouverte:
            return
        self._periodes = self.ctx.periodes.lister()
        self.table.afficher(["Id", "Nom", "Début", "Fin", "Statut"],
                            [[p.id, p.nom, f"{p.date_debut:%d/%m/%Y}", f"{p.date_fin:%d/%m/%Y}", p.statut.name.capitalize()] for p in self._periodes])

    def _courante(self) -> int | None:
        i = self.table.currentRow()
        return self._periodes[i].id if 0 <= i < len(self._periodes) else None

    def _agir(self, fn) -> None:
        pid = self._courante()
        if pid is None:
            return
        try:
            fn(pid)
        except Exception as e:
            return afficher_erreur(self, e)
        self.ctx.signaler_modification()

    def cloturer(self): self._agir(self.ctx.periodes.cloturer)
    def rouvrir(self): self._agir(self.ctx.periodes.rouvrir)

    def verrouiller(self) -> None:
        if QMessageBox.question(self, "Verrouiller", "Un verrouillage est définitif. Continuer ?") == QMessageBox.StandardButton.Yes:
            self._agir(self.ctx.periodes.verrouiller)

    def generer(self) -> None:
        try:
            self.ctx.periodes.generer_mensuelles(self.annee.value())
        except Exception as e:
            return afficher_erreur(self, e)
        self.ctx.signaler_modification()

    def ajouter(self) -> None:
        try:
            self.ctx.periodes.ajouter(self.nom.text().strip(), self.date_de(self.debut), self.date_de(self.fin))
        except Exception as e:
            return afficher_erreur(self, e)
        self.nom.clear()
        self.ctx.signaler_modification()
