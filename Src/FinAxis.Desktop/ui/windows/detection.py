"""Detection d'anomalies sur une periode."""
from __future__ import annotations

from PySide6.QtWidgets import QComboBox, QHBoxLayout, QLabel, QPushButton, QVBoxLayout

from excel.exporter import exporter_detection
from models.detection_result import DetectionConfig, Gravite, MethodeDetection
from ui.context import afficher_erreur
from ui.dialogs.detection_config_dialog import DetectionConfigDialog
from ui.dialogs.export_dialog import demander_fichier_excel
from ui.widgets.report_viewer_widget import ReportViewerWidget

from .base_window import BaseWindow
from .benford_chart_window import BenfordChartWindow

COULEURS = {Gravite.INFO: "#F2F2F2", Gravite.FAIBLE: "#E2EFDA", Gravite.MOYENNE: "#FFF2CC",
            Gravite.ELEVEE: "#FCE4D6", Gravite.CRITIQUE: "#F8CBAD"}
NOMS_METHODES = {MethodeDetection.BENFORD: "Benford", MethodeDetection.DOUBLONS: "Doublons",
                 MethodeDetection.NOMBRES_RONDS: "Montants ronds", MethodeDetection.VALEURS_ABERRANTES: "Valeurs aberrantes"}


class DetectionWindow(BaseWindow):
    titre = "Détection d'anomalies"

    def __init__(self, ctx, parent=None):
        super().__init__(ctx, parent)
        v = QVBoxLayout(self)
        h = QHBoxLayout()
        self.periode = QComboBox()
        self.config = DetectionConfig()
        bp = QPushButton("Paramètres…")
        bp.clicked.connect(self.parametres)
        lancer = QPushButton("Lancer l'analyse")
        lancer.clicked.connect(self.lancer)
        bb = QPushButton("Graphique de Benford")
        bb.clicked.connect(self.voir_benford)
        exp = QPushButton("Exporter Excel")
        exp.clicked.connect(self.exporter)
        for w in (QLabel("Période"), self.periode, bp, lancer, bb, exp):
            h.addWidget(w)
        v.addLayout(h)
        self.vue = ReportViewerWidget()
        v.addWidget(self.vue)
        self.etat = QLabel("Aucune analyse lancée.")
        v.addWidget(self.etat)
        self._resultats = []
        self._fenetre_benford = None
        self.rafraichir()

    def rafraichir(self) -> None:
        if self.ctx.session.est_ouverte:
            self.remplir_periodes(self.periode)

    def parametres(self) -> None:
        d = DetectionConfigDialog(self.config, self)
        if d.exec():
            self.config = d.config()

    def lancer(self) -> None:
        pid = self.periode.currentData()
        if pid is None:
            return afficher_erreur(self, ValueError("Créez d'abord une période fiscale."))
        try:
            self._resultats = self.ctx.detection.executer(pid, self.config)
        except Exception as e:
            return afficher_erreur(self, e)
        noms = {c.id: c.libelle for c in self.ctx.comptes.lister()}
        self.vue.afficher(["Méthode", "Gravité", "Score", "Compte", "Description"],
                          [[NOMS_METHODES[r.methode], r.gravite.name.capitalize(), f"{r.score:.2f}", noms.get(r.id_compte, r.id_compte), r.description]
                           for r in self._resultats],
                          colonnes_numeriques={2}, couleurs={i: COULEURS[r.gravite] for i, r in enumerate(self._resultats)})
        graves = sum(1 for r in self._resultats if r.gravite >= Gravite.ELEVEE)
        self.etat.setText(f"{len(self._resultats)} anomalie(s) dont {graves} grave(s).")

    def voir_benford(self) -> None:
        pid = self.periode.currentData()
        if pid is None:
            return
        try:
            rapport = self.ctx.detection.analyse_benford(pid)
        except Exception as e:
            return afficher_erreur(self, e)
        self._fenetre_benford = BenfordChartWindow(self.ctx)
        self._fenetre_benford.afficher_rapport(rapport)
        self._fenetre_benford.resize(620, 420)
        self._fenetre_benford.show()

    def exporter(self) -> None:
        if not self._resultats:
            return afficher_erreur(self, ValueError("Lancez d'abord une analyse."))
        chemin = demander_fichier_excel(self, "detection.xlsx")
        if chemin:
            try:
                exporter_detection(chemin, self._resultats, {c.id: c.libelle for c in self.ctx.comptes.lister()}, self.ctx.entreprise)
            except Exception as e:
                afficher_erreur(self, e)
