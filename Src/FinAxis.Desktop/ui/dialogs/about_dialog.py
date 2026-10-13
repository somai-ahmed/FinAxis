from PySide6.QtWidgets import QMessageBox


def afficher_a_propos(parent, version_moteur: str) -> None:
    QMessageBox.about(parent, "À propos de FinAxis",
                      f"<b>FinAxis Accounting &amp; Fraud Detection Engine</b><br>"
                      f"Moteur natif C : version {version_moteur}<br>"
                      "Interface Python / PySide6<br><br>"
                      "Comptabilité en partie double, rapports financiers<br>et détection d'anomalies (loi de Benford).")
