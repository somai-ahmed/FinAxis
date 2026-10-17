"""Plan comptable : liste des comptes + ajout d'un compte."""
from __future__ import annotations

from PySide6.QtWidgets import (QComboBox, QFormLayout, QGroupBox, QHBoxLayout, QLineEdit,
                               QPushButton, QVBoxLayout)

from helpers.currency_helper import formater_montant
from helpers.validation_helper import classe_depuis_code, code_compte_valide
from models.account import Account, ClasseCompte, SoldeNormal, TypeCompte
from ui.context import afficher_erreur
from ui.widgets.account_combo_box import AccountComboBox
from ui.widgets.report_viewer_widget import ReportViewerWidget

from .base_window import BaseWindow

TYPES = [("Actif", TypeCompte.ACTIF), ("Passif", TypeCompte.PASSIF), ("Capitaux", TypeCompte.CAPITAUX),
         ("Produit", TypeCompte.PRODUIT), ("Charge", TypeCompte.CHARGE)]
# type et sens proposes automatiquement selon la classe du code
DEFAUT_PAR_CLASSE = {1: (TypeCompte.CAPITAUX, SoldeNormal.CREDITEUR), 2: (TypeCompte.ACTIF, SoldeNormal.DEBITEUR),
                     3: (TypeCompte.ACTIF, SoldeNormal.DEBITEUR), 4: (TypeCompte.ACTIF, SoldeNormal.DEBITEUR),
                     5: (TypeCompte.ACTIF, SoldeNormal.DEBITEUR), 6: (TypeCompte.CHARGE, SoldeNormal.DEBITEUR),
                     7: (TypeCompte.PRODUIT, SoldeNormal.CREDITEUR)}


class AccountChartWindow(BaseWindow):
    titre = "Plan comptable"

    def __init__(self, ctx, parent=None):
        super().__init__(ctx, parent)
        v = QVBoxLayout(self)
        self.table = ReportViewerWidget()
        v.addWidget(self.table)

        boite = QGroupBox("Nouveau compte")
        f = QFormLayout(boite)
        self.code, self.nom = QLineEdit(), QLineEdit()
        self.code.setPlaceholderText("ex. 512")
        self.type = QComboBox()
        for t, val in TYPES:
            self.type.addItem(t, val)
        self.sens = QComboBox()
        self.sens.addItem("Débiteur", SoldeNormal.DEBITEUR)
        self.sens.addItem("Créditeur", SoldeNormal.CREDITEUR)
        self.parent_cb = AccountComboBox()
        f.addRow("Code", self.code)
        f.addRow("Nom", self.nom)
        f.addRow("Type", self.type)
        f.addRow("Solde normal", self.sens)
        f.addRow("Compte parent", self.parent_cb)
        bas = QHBoxLayout()
        ajouter = QPushButton("Ajouter")
        ajouter.clicked.connect(self.ajouter)
        defaut = QPushButton("Compléter depuis le plan de base")
        defaut.clicked.connect(self.charger_defaut)
        bas.addWidget(ajouter)
        bas.addWidget(defaut)
        f.addRow(bas)
        v.addWidget(boite)
        self.code.textChanged.connect(self._proposer)
        self.rafraichir()

    def _proposer(self, texte: str) -> None:
        classe = classe_depuis_code(texte)
        if classe:
            t, s = DEFAUT_PAR_CLASSE[classe]
            self.type.setCurrentIndex(self.type.findData(t))
            self.sens.setCurrentIndex(self.sens.findData(s))

    def rafraichir(self) -> None:
        if not self.ctx.session.est_ouverte:
            return
        comptes = self.ctx.comptes.lister()
        dev = self.ctx.devise
        self.table.afficher(["Code", "Nom", "Classe", "Sens normal", f"Solde ({dev})"],
                            [[c.code, c.nom, int(c.classe), c.solde_normal.name.capitalize(), formater_montant(c.solde)] for c in sorted(comptes, key=lambda c: c.code)],
                            colonnes_numeriques={4})
        self.parent_cb.definir_comptes(comptes, inclure_vide=True)

    def ajouter(self) -> None:
        code = self.code.text().strip()
        if not code_compte_valide(code) or classe_depuis_code(code) is None:
            return afficher_erreur(self, ValueError("Code invalide : 3 à 15 chiffres, premier chiffre entre 1 et 7."))
        try:
            self.ctx.comptes.ajouter(Account(code, self.nom.text().strip(), ClasseCompte(int(code[0])),
                                             self.type.currentData(), self.sens.currentData(), self.parent_cb.id_compte()))
        except Exception as e:
            return afficher_erreur(self, e)
        self.code.clear()
        self.nom.clear()
        self.ctx.signaler_modification()

    def charger_defaut(self) -> None:
        try:
            self.ctx.comptes.charger_plan_par_defaut()
        except Exception as e:
            return afficher_erreur(self, e)
        self.ctx.signaler_modification()
