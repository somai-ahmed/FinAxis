"""Grand livre d'un compte sur une plage de dates."""
from __future__ import annotations

from PySide6.QtWidgets import QHBoxLayout, QLabel, QPushButton, QVBoxLayout

from excel.exporter import exporter_grand_livre
from helpers.currency_helper import formater_montant
from ui.context import afficher_erreur
from ui.dialogs.export_dialog import demander_fichier_excel
from ui.widgets.account_combo_box import AccountComboBox
from ui.widgets.report_viewer_widget import ReportViewerWidget

from .base_window import BaseWindow


class GrandLivreWindow(BaseWindow):
    titre = "Grand livre"

    def __init__(self, ctx, parent=None):
        super().__init__(ctx, parent)
        v = QVBoxLayout(self)
        h = QHBoxLayout()
        self.compte = AccountComboBox()
        d, f = self.plage_exercice() if ctx.session.est_ouverte else (None, None)
        self.debut, self.fin = self.champ_date(d), self.champ_date(f)
        gen = QPushButton("Générer")
        gen.clicked.connect(self.generer)
        exp = QPushButton("Exporter Excel")
        exp.clicked.connect(self.exporter)
        for w in (QLabel("Compte"), self.compte, QLabel("Du"), self.debut, QLabel("au"), self.fin, gen, exp):
            h.addWidget(w)
        v.addLayout(h)
        self.vue = ReportViewerWidget()
        v.addWidget(self.vue)
        self._lignes = []
        self.rafraichir()

    def rafraichir(self) -> None:
        if self.ctx.session.est_ouverte:
            self.compte.definir_comptes(self.ctx.comptes.lister())

    def generer(self) -> None:
        id_c = self.compte.id_compte()
        if not id_c:
            return afficher_erreur(self, ValueError("Choisissez un compte."))
        try:
            self._lignes = self.ctx.rapports.grand_livre(id_c, self.date_de(self.debut), self.date_de(self.fin))
        except Exception as e:
            return afficher_erreur(self, e)
        self.vue.afficher(["Date", "Référence", "Libellé", "Débit", "Crédit", "Solde cumulé"],
                          [[f"{l.date:%d/%m/%Y}" if l.date else "", l.reference, l.libelle, formater_montant(l.debit, zero_vide=True),
                            formater_montant(l.credit, zero_vide=True), formater_montant(l.solde_cumule)] for l in self._lignes],
                          colonnes_numeriques={3, 4, 5})

    def exporter(self) -> None:
        if not self._lignes:
            return afficher_erreur(self, ValueError("Générez d'abord le grand livre."))
        chemin = demander_fichier_excel(self, "grand_livre.xlsx")
        if chemin:
            try:
                exporter_grand_livre(chemin, self._lignes, self.compte.currentText(), self.ctx.entreprise,
                                     self.date_de(self.debut), self.date_de(self.fin), self.ctx.devise)
            except Exception as e:
                afficher_erreur(self, e)
