"""Fenetre de demarrage : nouvelle entreprise, ouvrir un fichier, ou charger la demo."""
from __future__ import annotations

from datetime import date

from PySide6.QtCore import QDate
from PySide6.QtWidgets import (QDateEdit, QDialog, QFormLayout, QHBoxLayout, QLabel,
                               QLineEdit, QPushButton, QVBoxLayout)

from services.session_service import CompanyConfig


class LoginDialog(QDialog):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setWindowTitle("FinCore — Démarrage")
        self.choix: str | None = None       # "nouveau" | "ouvrir" | "demo"
        v = QVBoxLayout(self)
        v.addWidget(QLabel("<h2>FinCore</h2>Comptabilité et détection d'anomalies"))
        f = QFormLayout()
        self.nom = QLineEdit()
        self.nom.setPlaceholderText("Nom de l'entreprise")
        self.devise = QLineEdit("TND")
        self.devise.setMaxLength(3)
        an = date.today().year
        self.debut, self.fin = QDateEdit(QDate(an, 1, 1)), QDateEdit(QDate(an, 12, 31))
        for e in (self.debut, self.fin):
            e.setCalendarPopup(True)
            e.setDisplayFormat("dd/MM/yyyy")
        f.addRow("Entreprise", self.nom)
        f.addRow("Devise", self.devise)
        f.addRow("Début d'exercice", self.debut)
        f.addRow("Fin d'exercice", self.fin)
        v.addLayout(f)
        h = QHBoxLayout()
        for texte, choix in (("Créer", "nouveau"), ("Ouvrir un fichier…", "ouvrir"), ("Données de démonstration", "demo")):
            b = QPushButton(texte)
            b.clicked.connect(lambda _=False, c=choix: self._choisir(c))
            h.addWidget(b)
        v.addLayout(h)

    def _choisir(self, choix: str) -> None:
        if choix == "nouveau" and not self.nom.text().strip():
            self.nom.setFocus()
            return
        self.choix = choix
        self.accept()

    def config(self) -> CompanyConfig:
        return CompanyConfig(self.nom.text().strip(), (self.devise.text().strip() or "TND").upper(),
                             self.debut.date().toPython(), self.fin.date().toPython())
