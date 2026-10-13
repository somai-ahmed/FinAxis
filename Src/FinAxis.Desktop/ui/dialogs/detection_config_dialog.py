"""Reglage des methodes de detection et de leurs seuils."""
from __future__ import annotations

from PySide6.QtWidgets import (QCheckBox, QDialog, QDialogButtonBox, QDoubleSpinBox,
                               QFormLayout, QSpinBox)

from models.detection_result import DetectionConfig


class DetectionConfigDialog(QDialog):
    def __init__(self, config: DetectionConfig, parent=None):
        super().__init__(parent)
        self.setWindowTitle("Paramètres de détection")
        f = QFormLayout(self)
        self.benford = QCheckBox("Loi de Benford")
        self.doublons = QCheckBox("Doublons")
        self.ronds = QCheckBox("Montants ronds")
        self.aberrantes = QCheckBox("Valeurs aberrantes")
        for cb, v in ((self.benford, config.benford), (self.doublons, config.doublons),
                      (self.ronds, config.nombres_ronds), (self.aberrantes, config.valeurs_aberrantes)):
            cb.setChecked(v)
            f.addRow(cb)
        self.sigma = QDoubleSpinBox(); self.sigma.setRange(0.5, 10.0); self.sigma.setSingleStep(0.5); self.sigma.setValue(config.seuil_ecart_type)
        self.rond = QDoubleSpinBox(); self.rond.setRange(1, 1_000_000); self.rond.setDecimals(0); self.rond.setValue(config.seuil_nombre_rond)
        self.fenetre = QSpinBox(); self.fenetre.setRange(1, 90); self.fenetre.setValue(config.fenetre_jours_doublon)
        f.addRow("Seuil valeur aberrante (σ)", self.sigma)
        f.addRow("Seuil montant rond (DT)", self.rond)
        f.addRow("Fenêtre doublons (jours)", self.fenetre)
        boutons = QDialogButtonBox(QDialogButtonBox.StandardButton.Ok | QDialogButtonBox.StandardButton.Cancel)
        boutons.accepted.connect(self.accept)
        boutons.rejected.connect(self.reject)
        f.addRow(boutons)

    def config(self) -> DetectionConfig:
        return DetectionConfig(self.benford.isChecked(), self.doublons.isChecked(), self.ronds.isChecked(),
                               self.aberrantes.isChecked(), self.sigma.value(), self.rond.value(), self.fenetre.value())
