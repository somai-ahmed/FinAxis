"""Champ de saisie d'un montant en dinars (3 decimales, virgule ou point)."""
from __future__ import annotations

from PySide6.QtCore import QRegularExpression, Qt
from PySide6.QtGui import QRegularExpressionValidator
from PySide6.QtWidgets import QLineEdit

from helpers.currency_helper import analyser_montant, formater_montant


class AmountLineEdit(QLineEdit):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setAlignment(Qt.AlignmentFlag.AlignRight)
        self.setPlaceholderText("0,000")
        # chiffres, espaces, signe optionnel, separateur decimal . ou , (3 decimales max)
        self.setValidator(QRegularExpressionValidator(QRegularExpression(r"^-?[0-9 ]*[.,]?[0-9]{0,3}$"), self))
        self.editingFinished.connect(self._reformater)

    def millimes(self) -> int:
        """Montant saisi en millimes (0 si vide ou invalide)."""
        return analyser_montant(self.text()) or 0

    def definir_millimes(self, valeur: int) -> None:
        self.setText(formater_montant(valeur, zero_vide=True))

    def _reformater(self) -> None:
        v = analyser_montant(self.text())
        self.setText("" if v in (None, 0) else formater_montant(v))
