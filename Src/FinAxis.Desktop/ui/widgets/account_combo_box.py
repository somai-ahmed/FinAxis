"""Liste deroulante stricte de comptes."""
from __future__ import annotations

from PySide6.QtWidgets import QComboBox

from models.account import Account


class AccountComboBox(QComboBox):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setEditable(False)
        self.setInsertPolicy(QComboBox.InsertPolicy.NoInsert)
        self.setMaxVisibleItems(18)

    def definir_comptes(self, comptes: list[Account], inclure_vide: bool = False) -> None:
        courant = self.id_compte()
        self.clear()
        if inclure_vide:
            self.addItem("", 0)
        for c in sorted(comptes, key=lambda c: c.code):
            self.addItem(c.libelle, c.id)
        if courant:
            self.definir_id(courant)
        elif inclure_vide:
            self.setCurrentIndex(0)
        else:
            self.setCurrentIndex(-1)

    def id_compte(self) -> int:
        """Id du compte choisi (0 si aucun compte n'est selectionne)."""
        return int(self.currentData() or 0)

    def definir_id(self, id_compte: int) -> None:
        i = self.findData(id_compte)
        if i >= 0:
            self.setCurrentIndex(i)
