"""Balance generale sur une plage de dates."""
from __future__ import annotations

from PySide6.QtWidgets import QCheckBox, QHBoxLayout, QLabel, QPushButton, QVBoxLayout

from excel.exporter import exporter_balance
from helpers.currency_helper import formater_montant
from ui.context import afficher_erreur
from ui.dialogs.export_dialog import demander_fichier_excel
from ui.widgets.report_viewer_widget import ReportViewerWidget

from .base_window import BaseWindow


class BalanceWindow(BaseWindow):
    titre = "Balance générale"

    def __init__(self, ctx, parent=None):
        super().__init__(ctx, parent)
        v = QVBoxLayout(self)
        h = QHBoxLayout()
        d, f = self.plage_exercice() if ctx.session.est_ouverte else (None, None)
        self.debut, self.fin = self.champ_date(d), self.champ_date(f)
        self.masquer_vides = QCheckBox("Masquer les comptes sans mouvement")
        self.masquer_vides.setChecked(True)
        gen = QPushButton("Générer")
        gen.clicked.connect(self.generer)
        exp = QPushButton("Exporter Excel")
        exp.clicked.connect(self.exporter)
        for w in (QLabel("Du"), self.debut, QLabel("au"), self.fin, self.masquer_vides, gen, exp):
            h.addWidget(w)
        v.addLayout(h)
        self.vue = ReportViewerWidget()
        v.addWidget(self.vue)
        self.etat = QLabel()
        v.addWidget(self.etat)
        self._lignes = []

    def generer(self) -> None:
        try:
            lignes = self.ctx.rapports.balance(self.date_de(self.debut), self.date_de(self.fin))
        except Exception as e:
            return afficher_erreur(self, e)
        if self.masquer_vides.isChecked():
            lignes = [l for l in lignes if l.total_debit or l.total_credit]
        self._lignes = lignes
        td, tc = sum(l.total_debit for l in lignes), sum(l.total_credit for l in lignes)
        rows = [[l.code, l.nom] + [formater_montant(x, zero_vide=True) for x in (l.total_debit, l.total_credit, l.solde_debit, l.solde_credit)] for l in lignes]
        rows.append(["", "TOTAUX", formater_montant(td), formater_montant(tc), formater_montant(sum(l.solde_debit for l in lignes)),
                     formater_montant(sum(l.solde_credit for l in lignes))])
        self.vue.afficher(["Code", "Compte", "Total débit", "Total crédit", "Solde débiteur", "Solde créditeur"], rows,
                          colonnes_numeriques={2, 3, 4, 5}, lignes_en_gras={len(rows) - 1})
        self.etat.setText("Balance équilibrée ✓" if td == tc else f"⚠ Balance déséquilibrée : écart {formater_montant(td - tc)}")

    def exporter(self) -> None:
        if not self._lignes:
            return afficher_erreur(self, ValueError("Générez d'abord la balance."))
        chemin = demander_fichier_excel(self, "balance.xlsx")
        if chemin:
            try:
                exporter_balance(chemin, self._lignes, self.ctx.entreprise, self.date_de(self.debut), self.date_de(self.fin), self.ctx.devise)
            except Exception as e:
                afficher_erreur(self, e)
