"""Contexte partage de l'application : tous les services + un signal 'donnees modifiees'.

Les fenetres ne parlent JAMAIS a bindings/ : elles passent par ces services.
"""
from __future__ import annotations

from PySide6.QtCore import QObject, Signal
from PySide6.QtWidgets import QMessageBox, QWidget

from bindings.fincore_ffi import ErreurFinCore, FinCoreLib, charger
from services.account_service import AccountService
from services.detection_service import DetectionService
from services.journal_service import JournalService
from services.period_service import PeriodService
from services.persistence_service import PersistenceService
from services.report_service import ReportService
from services.session_service import CompanyConfig, SessionService


class AppContext(QObject):
    modifie = Signal()      # emis apres toute modification de la session (les fenetres se rafraichissent)

    def __init__(self, ffi: FinCoreLib | None = None):
        super().__init__()
        self.session = SessionService(ffi or charger())
        self.comptes = AccountService(self.session)
        self.periodes = PeriodService(self.session)
        self.journal = JournalService(self.session)
        self.rapports = ReportService(self.session)
        self.detection = DetectionService(self.session)
        self.persistance = PersistenceService(self.session)
        self.non_sauvegarde = False

    def nouvelle_session(self, config: CompanyConfig) -> None:
        self.session.creer(config)
        self.comptes.charger_plan_par_defaut()
        self.non_sauvegarde = True
        self.modifie.emit()

    def signaler_modification(self) -> None:
        self.non_sauvegarde = True
        self.modifie.emit()

    @property
    def devise(self) -> str:
        return self.session.lire_config().devise if self.session.est_ouverte else "TND"

    @property
    def entreprise(self) -> str:
        return self.session.lire_config().nom_entreprise if self.session.est_ouverte else ""


def afficher_erreur(parent: QWidget | None, erreur: Exception, titre: str = "Erreur") -> None:
    """Message clair : le texte du moteur pour ErreurFinCore, sinon l'exception telle quelle."""
    texte = erreur.message if isinstance(erreur, ErreurFinCore) else str(erreur)
    QMessageBox.warning(parent, titre, texte)
