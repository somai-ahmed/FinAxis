"""Histogramme de Benford : frequences observees (barres) contre theoriques (ligne)."""
from __future__ import annotations

from PySide6.QtCore import QRectF, Qt
from PySide6.QtGui import QColor, QPainter, QPen
from PySide6.QtWidgets import QLabel, QVBoxLayout, QWidget

from models.detection_result import BenfordReport

from .base_window import BaseWindow


class _Histogramme(QWidget):
    def __init__(self):
        super().__init__()
        self.rapport: BenfordReport | None = None
        self.setMinimumSize(480, 300)

    def definir(self, rapport: BenfordReport | None) -> None:
        self.rapport = rapport
        self.update()

    def paintEvent(self, _):
        p = QPainter(self)
        p.setRenderHint(QPainter.RenderHint.Antialiasing)
        p.fillRect(self.rect(), QColor("white"))
        if not self.rapport or not self.rapport.chiffres:
            p.drawText(self.rect(), Qt.AlignmentFlag.AlignCenter, "Pas assez de données pour l'analyse de Benford")
            return
        marge = 40
        zone = QRectF(marge, 15, self.width() - 2 * marge, self.height() - 55)
        chiffres = self.rapport.chiffres
        maxi = max(max(c.attendu, c.observe) for c in chiffres) * 1.15 or 1.0
        largeur = zone.width() / len(chiffres)
        p.setPen(QPen(QColor("#888"), 1))
        p.drawLine(zone.bottomLeft(), zone.bottomRight())
        p.drawLine(zone.topLeft(), zone.bottomLeft())
        points = []
        for i, c in enumerate(chiffres):
            haut = zone.height() * c.observe / maxi
            p.fillRect(QRectF(zone.left() + i * largeur + largeur * 0.15, zone.bottom() - haut, largeur * 0.7, haut), QColor("#4472C4"))
            p.setPen(QColor("black"))
            p.drawText(QRectF(zone.left() + i * largeur, zone.bottom() + 3, largeur, 18), Qt.AlignmentFlag.AlignCenter, str(c.chiffre))
            p.drawText(QRectF(zone.left() + i * largeur, zone.bottom() - haut - 16, largeur, 14), Qt.AlignmentFlag.AlignCenter, f"{c.observe * 100:.1f}%")
            points.append((zone.left() + (i + 0.5) * largeur, zone.bottom() - zone.height() * c.attendu / maxi))
        p.setPen(QPen(QColor("#C00000"), 2))
        for a, b in zip(points, points[1:]):
            p.drawLine(int(a[0]), int(a[1]), int(b[0]), int(b[1]))
        for x, y in points:
            p.drawEllipse(int(x) - 3, int(y) - 3, 6, 6)
        p.setPen(QColor("black"))
        p.drawText(QRectF(zone.left(), zone.bottom() + 20, zone.width(), 18), Qt.AlignmentFlag.AlignCenter,
                   "Barres : observé   —   Ligne rouge : loi de Benford")


class BenfordChartWindow(BaseWindow):
    titre = "Analyse de Benford"

    def __init__(self, ctx, parent=None):
        super().__init__(ctx, parent)
        v = QVBoxLayout(self)
        self.histo = _Histogramme()
        self.resume = QLabel()
        v.addWidget(self.histo, 1)
        v.addWidget(self.resume)

    def afficher_rapport(self, rapport: BenfordReport | None) -> None:
        self.histo.definir(rapport)
        if rapport is None:
            self.resume.setText("Échantillon insuffisant.")
        else:
            verdict = "conforme à la loi de Benford ✓" if rapport.est_normal else "⚠ s'écarte de la loi de Benford"
            self.resume.setText(f"{rapport.taille_echantillon} montants — χ² = {rapport.chi_carre:.2f} (seuil 15,51) — {verdict}")
