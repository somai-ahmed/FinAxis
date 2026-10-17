"""Import d'ecritures depuis un classeur Excel (stretch)."""
from __future__ import annotations

from PySide6.QtWidgets import QCheckBox, QFileDialog, QLabel, QPushButton, QVBoxLayout

from excel.importer import ErreurImport, importer_ecritures
from ui.context import afficher_erreur

from .base_window import BaseWindow


class ImportWindow(BaseWindow):
    titre = "Importer des écritures (Excel)"

    def __init__(self, ctx, parent=None):
        super().__init__(ctx, parent)
        v = QVBoxLayout(self)
        v.addWidget(QLabel("Colonnes attendues : Date | Référence | Libellé | Code compte | Débit | Crédit\n"
                           "Les lignes de même référence forment une seule écriture."))
        self.brouillon = QCheckBox("Importer comme brouillons (à comptabiliser ensuite)")
        self.brouillon.setChecked(True)
        v.addWidget(self.brouillon)
        b = QPushButton("Choisir un fichier .xlsx…")
        b.clicked.connect(self.choisir)
        v.addWidget(b)
        self.resultat = QLabel()
        v.addWidget(self.resultat)

    def choisir(self) -> None:
        chemin, _ = QFileDialog.getOpenFileName(self, "Importer", "", "Classeur Excel (*.xlsx)")
        if chemin:
            self.importer(chemin)

    def importer(self, chemin: str) -> tuple[int, list[str]]:
        """Importe le fichier ; retourne (nombre importe, messages d'erreur)."""
        try:
            ecritures = importer_ecritures(chemin, {c.code: c.id for c in self.ctx.comptes.lister()})
        except (ErreurImport, OSError) as e:
            afficher_erreur(self, e)
            return 0, [str(e)]
        erreurs, ok = [], 0
        for e in ecritures:
            p = self.ctx.periodes.periode_pour_date(e.date)
            e.periode_id = p.id if p else 0
            e.est_validee = not self.brouillon.isChecked()
            try:
                self.ctx.journal.ajouter(e)
                ok += 1
            except Exception as ex:
                erreurs.append(f"{e.reference} : {getattr(ex, 'message', ex)}")
        if ok:
            self.ctx.signaler_modification()
        self.resultat.setText(f"{ok} écriture(s) importée(s), {len(erreurs)} refusée(s).\n" + "\n".join(erreurs[:10]))
        return ok, erreurs
