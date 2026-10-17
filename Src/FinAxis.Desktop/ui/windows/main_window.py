"""Fenetre principale : menus + zone de travail a onglets."""
from __future__ import annotations

from PySide6.QtGui import QAction, QKeySequence
from PySide6.QtWidgets import (QFileDialog, QLabel, QMainWindow, QMessageBox, QTabWidget)

from services.demo_data import remplir_demo
from services.persistence_service import EXTENSION
from ui.context import AppContext, afficher_erreur
from ui.dialogs.about_dialog import afficher_a_propos

from .account_chart_window import AccountChartWindow
from .balance_window import BalanceWindow
from .bilan_window import BilanWindow
from .detection_window import DetectionWindow
from .grand_livre_window import GrandLivreWindow
from .import_window import ImportWindow
from .journal_entry_window import JournalEntryWindow
from .login_dialog import LoginDialog
from .period_manager_window import PeriodManagerWindow

FILTRE_FICHIER = f"Fichiers FinCore (*{EXTENSION})"


class MainWindow(QMainWindow):
    def __init__(self, ctx: AppContext):
        super().__init__()
        self.ctx = ctx
        self.setMinimumSize(1000, 650)
        self.onglets = QTabWidget()
        self.onglets.setTabsClosable(False)
        self.setCentralWidget(self.onglets)
        self.barre = QLabel()
        self.statusBar().addPermanentWidget(self.barre)
        self._construire_menus()
        ctx.modifie.connect(self._maj_titre)
        self._maj_titre()

    # ------------------------------------------------------------------
    def _action(self, menu, texte, fn, raccourci=None) -> QAction:
        a = QAction(texte, self)
        a.triggered.connect(lambda _=False: fn())
        if raccourci:
            a.setShortcut(QKeySequence(raccourci))
        menu.addAction(a)
        return a

    def _construire_menus(self) -> None:
        m = self.menuBar()
        f = m.addMenu("&Fichier")
        self._action(f, "Nouvelle entreprise…", self.demarrer, "Ctrl+N")
        self._action(f, "Ouvrir…", self.ouvrir, "Ctrl+O")
        self._action(f, "Enregistrer", self.enregistrer, "Ctrl+S")
        self._action(f, "Enregistrer sous…", self.enregistrer_sous, "Ctrl+Shift+S")
        f.addSeparator()
        self._action(f, "Quitter", self.close, "Ctrl+Q")
        d = m.addMenu("&Données")
        self._action(d, "Plan comptable", lambda: self.ouvrir_vue(AccountChartWindow))
        self._action(d, "Périodes fiscales", lambda: self.ouvrir_vue(PeriodManagerWindow))
        self._action(d, "Journal / saisie", lambda: self.ouvrir_vue(JournalEntryWindow))
        self._action(d, "Importer depuis Excel", lambda: self.ouvrir_vue(ImportWindow))
        r = m.addMenu("&Rapports")
        self._action(r, "Grand livre", lambda: self.ouvrir_vue(GrandLivreWindow))
        self._action(r, "Balance", lambda: self.ouvrir_vue(BalanceWindow))
        self._action(r, "Bilan et compte de résultat", lambda: self.ouvrir_vue(BilanWindow))
        a = m.addMenu("&Analyse")
        self._action(a, "Détection d'anomalies", lambda: self.ouvrir_vue(DetectionWindow))
        h = m.addMenu("&Aide")
        self._action(h, "À propos", lambda: afficher_a_propos(self, self.ctx.session.ffi.version()))

    def ouvrir_vue(self, classe) -> None:
        """Ouvre (ou ramene au premier plan) l'onglet de la vue demandee."""
        if not self.ctx.session.est_ouverte:
            return self.demarrer()
        for i in range(self.onglets.count()):
            if isinstance(self.onglets.widget(i), classe):
                self.onglets.setCurrentIndex(i)
                return
        vue = classe(self.ctx)
        self.onglets.setCurrentIndex(self.onglets.addTab(vue, vue.titre))

    def _fermer_vues(self) -> None:
        while self.onglets.count():
            w = self.onglets.widget(0)
            self.onglets.removeTab(0)
            w.deleteLater()

    def _maj_titre(self) -> None:
        if not self.ctx.session.est_ouverte:
            self.setWindowTitle("FinCore")
            return
        fichier = self.ctx.session.chemin or "non enregistré"
        marque = " *" if self.ctx.non_sauvegarde else ""
        self.setWindowTitle(f"FinCore — {self.ctx.entreprise} [{fichier}]{marque}")
        self.barre.setText(f"{len(self.ctx.comptes.lister())} comptes — {len(self.ctx.journal.lister())} écritures")

    # --- fichier -------------------------------------------------------
    def demarrer(self) -> None:
        if not self._confirmer_abandon():
            return
        d = LoginDialog(self)
        if not d.exec():
            return
        try:
            if d.choix == "nouveau":
                self._fermer_vues()
                self.ctx.nouvelle_session(d.config())
                self.ouvrir_vue(AccountChartWindow)
            elif d.choix == "demo":
                self._fermer_vues()
                remplir_demo(self.ctx)
                self.ouvrir_vue(DetectionWindow)
            elif d.choix == "ouvrir":
                self.ouvrir(confirme=True)
        except Exception as e:
            afficher_erreur(self, e)

    def ouvrir(self, confirme: bool = False) -> None:
        if not confirme and not self._confirmer_abandon():
            return
        chemin, _ = QFileDialog.getOpenFileName(self, "Ouvrir", "", FILTRE_FICHIER)
        if not chemin:
            return
        try:
            self._fermer_vues()
            self.ctx.persistance.charger(chemin)
            self.ctx.non_sauvegarde = False
            self.ctx.modifie.emit()
            self.ouvrir_vue(JournalEntryWindow)
        except Exception as e:
            afficher_erreur(self, e, "Ouverture impossible")

    def enregistrer(self) -> bool:
        if not self.ctx.session.est_ouverte:
            return True
        if not self.ctx.session.chemin:
            return self.enregistrer_sous()
        return self._sauver(None)

    def enregistrer_sous(self) -> bool:
        if not self.ctx.session.est_ouverte:
            return True
        chemin, _ = QFileDialog.getSaveFileName(self, "Enregistrer sous", "", FILTRE_FICHIER)
        return bool(chemin) and self._sauver(chemin)

    def _sauver(self, chemin: str | None) -> bool:
        try:
            self.ctx.persistance.sauvegarder(chemin)
        except Exception as e:
            afficher_erreur(self, e, "Enregistrement impossible")
            return False
        self.ctx.non_sauvegarde = False
        self._maj_titre()
        return True

    def _confirmer_abandon(self) -> bool:
        if not (self.ctx.session.est_ouverte and self.ctx.non_sauvegarde):
            return True
        r = QMessageBox.question(self, "Modifications non enregistrées", "Enregistrer avant de continuer ?",
                                 QMessageBox.StandardButton.Save | QMessageBox.StandardButton.Discard | QMessageBox.StandardButton.Cancel)
        if r == QMessageBox.StandardButton.Save:
            return self.enregistrer()
        return r == QMessageBox.StandardButton.Discard

    def closeEvent(self, event) -> None:
        if self._confirmer_abandon():
            self._fermer_vues()
            self.ctx.session.fermer()          # libere la memoire du moteur C
            event.accept()
        else:
            event.ignore()
