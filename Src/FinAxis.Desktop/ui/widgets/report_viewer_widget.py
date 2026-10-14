"""Tableau generique en lecture seule pour afficher un rapport."""
from __future__ import annotations

from PySide6.QtCore import Qt
from PySide6.QtGui import QBrush, QColor, QFont
from PySide6.QtWidgets import QAbstractItemView, QHeaderView, QTableWidget, QTableWidgetItem


class ReportViewerWidget(QTableWidget):
    def __init__(self, parent=None):
        super().__init__(parent)
        self.setEditTriggers(QAbstractItemView.EditTrigger.NoEditTriggers)
        self.setAlternatingRowColors(True)
        self.setSelectionBehavior(QAbstractItemView.SelectionBehavior.SelectRows)
        self.verticalHeader().setVisible(False)

    def afficher(self, entetes: list[str], lignes: list[list], colonnes_numeriques: set[int] = frozenset(),
                 lignes_en_gras: set[int] = frozenset(), couleurs: dict[int, str] | None = None) -> None:
        """lignes : liste de listes de textes. colonnes_numeriques : alignees a droite."""
        self.clear()
        self.setColumnCount(len(entetes))
        self.setHorizontalHeaderLabels(entetes)
        self.setRowCount(len(lignes))
        gras = QFont()
        gras.setBold(True)
        for i, ligne in enumerate(lignes):
            for j, texte in enumerate(ligne):
                item = QTableWidgetItem(str(texte))
                if j in colonnes_numeriques:
                    item.setTextAlignment(Qt.AlignmentFlag.AlignRight | Qt.AlignmentFlag.AlignVCenter)
                if i in lignes_en_gras:
                    item.setFont(gras)
                if couleurs and i in couleurs:
                    item.setBackground(QBrush(QColor(couleurs[i])))
                self.setItem(i, j, item)
        self.horizontalHeader().setSectionResizeMode(QHeaderView.ResizeMode.ResizeToContents)
        if entetes:
            self.horizontalHeader().setStretchLastSection(True)
