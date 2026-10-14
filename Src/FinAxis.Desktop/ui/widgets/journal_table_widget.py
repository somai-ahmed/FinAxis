"""Tableau de saisie des lignes d'une ecriture (compte, debit, credit, libelle)."""
from __future__ import annotations

from PySide6.QtCore import Signal
from PySide6.QtWidgets import QHeaderView, QTableWidget

from helpers.currency_helper import formater_montant
from models.account import Account
from models.journal_entry import JournalLine

from .account_combo_box import AccountComboBox
from .amount_line_edit import AmountLineEdit

COL_COMPTE, COL_DEBIT, COL_CREDIT, COL_LIBELLE = range(4)


class JournalTableWidget(QTableWidget):
    totaux_changes = Signal(int, int)       # total debit, total credit (millimes)

    def __init__(self, parent=None):
        super().__init__(0, 4, parent)
        self.setHorizontalHeaderLabels(["Compte", "Débit", "Crédit", "Libellé"])
        self.horizontalHeader().setSectionResizeMode(COL_COMPTE, QHeaderView.ResizeMode.Stretch)
        self.horizontalHeader().setSectionResizeMode(COL_LIBELLE, QHeaderView.ResizeMode.Stretch)
        self._comptes: list[Account] = []

    def definir_comptes(self, comptes: list[Account]) -> None:
        self._comptes = [c for c in comptes if c.est_active]
        for i in range(self.rowCount()):
            self.cellWidget(i, COL_COMPTE).definir_comptes(self._comptes)

    def ajouter_ligne(self, ligne: JournalLine | None = None) -> None:
        i = self.rowCount()
        self.insertRow(i)
        combo = AccountComboBox()
        combo.definir_comptes(self._comptes)
        if ligne is None:
            combo.setCurrentIndex(-1)
        debit, credit = AmountLineEdit(), AmountLineEdit()
        self.setCellWidget(i, COL_COMPTE, combo)
        self.setCellWidget(i, COL_DEBIT, debit)
        self.setCellWidget(i, COL_CREDIT, credit)
        self.setItem(i, COL_LIBELLE, self._item(ligne.libelle if ligne else ""))
        if ligne:
            combo.definir_id(ligne.compte_id)
            debit.definir_millimes(ligne.debit)
            credit.definir_millimes(ligne.credit)
        debit.textChanged.connect(self._totaux)
        credit.textChanged.connect(self._totaux)
        self._totaux()

    @staticmethod
    def _item(texte: str):
        from PySide6.QtWidgets import QTableWidgetItem
        return QTableWidgetItem(texte)

    def supprimer_ligne_courante(self) -> None:
        if self.currentRow() >= 0:
            self.removeRow(self.currentRow())
            self._totaux()

    def vider(self) -> None:
        self.setRowCount(0)
        self._totaux()

    def lignes(self, *, strict: bool = True) -> list[JournalLine]:
        """Lignes saisies (les lignes sans compte ni montant sont ignorees)."""
        resultat = []
        for i in range(self.rowCount()):
            d, c = self.cellWidget(i, COL_DEBIT).millimes(), self.cellWidget(i, COL_CREDIT).millimes()
            compte = self.cellWidget(i, COL_COMPTE).id_compte()
            if compte == 0 and d == 0 and c == 0:
                continue
            if compte == 0 and strict:
                raise ValueError(f"Ligne {i + 1} : choisissez un compte du plan comptable.")
            item = self.item(i, COL_LIBELLE)
            resultat.append(JournalLine(compte, d, c, item.text() if item else ""))
        return resultat

    def totaux(self) -> tuple[int, int]:
        lignes = self.lignes(strict=False)
        return sum(l.debit for l in lignes), sum(l.credit for l in lignes)

    def _totaux(self) -> None:
        d, c = self.totaux()
        self.totaux_changes.emit(d, c)

    @staticmethod
    def texte_totaux(d: int, c: int) -> str:
        ecart = d - c
        etat = "équilibrée ✓" if ecart == 0 and d > 0 else f"écart {formater_montant(ecart)}"
        return f"Débit {formater_montant(d)}   Crédit {formater_montant(c)}   ({etat})"
