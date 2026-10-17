"""Saisie des ecritures du journal + liste des ecritures existantes."""
from __future__ import annotations

from PySide6.QtWidgets import (QCheckBox, QComboBox, QFormLayout, QGroupBox, QHBoxLayout,
                               QLabel, QLineEdit, QPushButton, QVBoxLayout)

from helpers.currency_helper import formater_montant
from models.journal_entry import JournalEntry
from ui.context import afficher_erreur
from ui.widgets.journal_table_widget import JournalTableWidget
from ui.widgets.report_viewer_widget import ReportViewerWidget

from .base_window import BaseWindow


class JournalEntryWindow(BaseWindow):
    titre = "Journal"

    def __init__(self, ctx, parent=None):
        super().__init__(ctx, parent)
        v = QVBoxLayout(self)
        boite = QGroupBox("Nouvelle écriture")
        f = QFormLayout(boite)
        self.date = self.champ_date()
        self.reference, self.description = QLineEdit(), QLineEdit()
        self.periode = QComboBox()
        self.comptabiliser = QCheckBox("Comptabiliser tout de suite (sinon : brouillon)")
        self.comptabiliser.setChecked(True)
        f.addRow("Date", self.date)
        f.addRow("Référence", self.reference)
        f.addRow("Description", self.description)
        f.addRow("Période", self.periode)
        self.lignes = JournalTableWidget()
        f.addRow(self.lignes)
        h = QHBoxLayout()
        for texte, fn in (("+ Ligne", lambda: self.lignes.ajouter_ligne()), ("− Ligne", self.lignes.supprimer_ligne_courante)):
            b = QPushButton(texte)
            b.clicked.connect(fn)
            h.addWidget(b)
        self.totaux = QLabel()
        h.addWidget(self.totaux, 1)
        self.lignes.totaux_changes.connect(lambda d, c: self.totaux.setText(JournalTableWidget.texte_totaux(d, c)))
        enreg = QPushButton("Enregistrer l'écriture")
        enreg.clicked.connect(self.enregistrer)
        h.addWidget(enreg)
        f.addRow(h)
        f.addRow(self.comptabiliser)
        v.addWidget(boite, 3)

        self.liste = ReportViewerWidget()
        v.addWidget(self.liste, 2)
        post = QPushButton("Comptabiliser le brouillon sélectionné")
        post.clicked.connect(self.comptabiliser_selection)
        v.addWidget(post)
        self.lignes.ajouter_ligne()
        self.lignes.ajouter_ligne()
        self.rafraichir()

    def rafraichir(self) -> None:
        if not self.ctx.session.est_ouverte:
            return
        comptes = self.ctx.comptes.lister()
        self._noms = {c.id: c.code for c in comptes}
        self.lignes.definir_comptes(comptes)
        self.remplir_periodes(self.periode, avec_toutes=True)
        self._ecritures = self.ctx.journal.lister()
        self.liste.afficher(["Id", "Date", "Référence", "Description", "Total", "État"],
                            [[e.id, f"{e.date:%d/%m/%Y}", e.reference, e.description, formater_montant(e.total_debit),
                              "Comptabilisée" if e.est_validee else "Brouillon"] for e in self._ecritures],
                            colonnes_numeriques={4})

    def enregistrer(self) -> None:
        e = JournalEntry(self.date_de(self.date), self.reference.text().strip(), self.description.text().strip(),
                         self.lignes.lignes(), self.periode.currentData() or 0, self.comptabiliser.isChecked())
        try:
            self.ctx.journal.ajouter(e)
        except Exception as ex:
            return afficher_erreur(self, ex)
        self.reference.clear()
        self.description.clear()
        self.lignes.vider()
        self.lignes.ajouter_ligne()
        self.lignes.ajouter_ligne()
        self.ctx.signaler_modification()

    def comptabiliser_selection(self) -> None:
        i = self.liste.currentRow()
        if not 0 <= i < len(self._ecritures):
            return
        try:
            self.ctx.journal.comptabiliser(self._ecritures[i].id)
        except Exception as ex:
            return afficher_erreur(self, ex)
        self.ctx.signaler_modification()
