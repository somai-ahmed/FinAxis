"""Bilan (a une date d'arret) et compte de resultat (sur une plage)."""
from __future__ import annotations

from PySide6.QtWidgets import QHBoxLayout, QLabel, QPushButton, QTabWidget, QVBoxLayout, QWidget

from excel.exporter import exporter_bilan
from helpers.currency_helper import formater_montant
from models.report import RUBRIQUES_CR
from ui.context import afficher_erreur
from ui.dialogs.export_dialog import demander_fichier_excel
from ui.widgets.report_viewer_widget import ReportViewerWidget

from .base_window import BaseWindow


class BilanWindow(BaseWindow):
    titre = "Bilan et compte de résultat"

    def __init__(self, ctx, parent=None):
        super().__init__(ctx, parent)
        v = QVBoxLayout(self)
        h = QHBoxLayout()
        d, f = self.plage_exercice() if ctx.session.est_ouverte else (None, None)
        self.debut, self.fin = self.champ_date(d), self.champ_date(f)
        gen = QPushButton("Générer")
        gen.clicked.connect(self.generer)
        exp = QPushButton("Exporter le bilan (Excel)")
        exp.clicked.connect(self.exporter)
        for w in (QLabel("Du"), self.debut, QLabel("au / arrêté au"), self.fin, gen, exp):
            h.addWidget(w)
        v.addLayout(h)
        onglets = QTabWidget()
        self.vue_bilan, self.vue_cr = ReportViewerWidget(), ReportViewerWidget()
        onglets.addTab(self.vue_bilan, "Bilan")
        onglets.addTab(self.vue_cr, "Compte de résultat")
        v.addWidget(onglets)
        self.etat = QLabel()
        v.addWidget(self.etat)
        self._bilan = None

    def generer(self) -> None:
        debut, fin = self.date_de(self.debut), self.date_de(self.fin)
        try:
            self._bilan = self.ctx.rapports.bilan(fin)
            cr = self.ctx.rapports.compte_resultat(debut, fin)
        except Exception as e:
            return afficher_erreur(self, e)
        b = self._bilan
        self.vue_bilan.afficher(["Poste", "Montant"], [["    " * l.profondeur + l.libelle, formater_montant(l.montant)] for l in b.lignes],
                                colonnes_numeriques={1}, lignes_en_gras={i for i, l in enumerate(b.lignes) if l.est_sous_total})
        self.etat.setText("Bilan équilibré ✓ (actif = passif)" if b.est_equilibre
                          else f"⚠ Bilan déséquilibré : actif {formater_montant(b.total_actif)} ≠ passif {formater_montant(b.total_passif)}")
        rows, gras = [], set()
        for r, nom in enumerate(RUBRIQUES_CR):
            rows.append([nom, formater_montant(cr.total_rubrique[r])])
            gras.add(len(rows) - 1)
            rows += [[f"    {l.code} {l.nom}", formater_montant(l.montant)] for l in cr.lignes if l.rubrique == r]
        for texte, val in (("Résultat d'exploitation", cr.resultat_exploitation), ("Résultat financier", cr.resultat_financier),
                           ("Résultat exceptionnel", cr.resultat_exceptionnel), ("Résultat avant impôt", cr.resultat_avant_impot),
                           ("RÉSULTAT NET", cr.resultat_net)):
            rows.append([texte, formater_montant(val)])
            gras.add(len(rows) - 1)
        self.vue_cr.afficher(["Rubrique", "Montant"], rows, colonnes_numeriques={1}, lignes_en_gras=gras)

    def exporter(self) -> None:
        if self._bilan is None:
            return afficher_erreur(self, ValueError("Générez d'abord le bilan."))
        chemin = demander_fichier_excel(self, "bilan.xlsx")
        if chemin:
            try:
                exporter_bilan(chemin, self._bilan, self.ctx.entreprise, self.date_de(self.fin), self.ctx.devise)
            except Exception as e:
                afficher_erreur(self, e)
