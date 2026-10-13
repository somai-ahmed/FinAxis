"""Boite 'Enregistrer sous' pour les exports Excel."""
from PySide6.QtWidgets import QFileDialog


def demander_fichier_excel(parent, nom_propose: str) -> str | None:
    chemin, _ = QFileDialog.getSaveFileName(parent, "Exporter vers Excel", nom_propose, "Classeur Excel (*.xlsx)")
    return chemin or None
