"""Point d'entree de FinCore Desktop : python main.py"""
from __future__ import annotations

import sys
from pathlib import Path

from PySide6.QtWidgets import QApplication, QMessageBox

def _racine() -> Path:
    """Dossier des ressources : _MEIPASS si empaquete, sinon le dossier de main.py."""
    if getattr(sys, "_MEIPASS", None):
        return Path(sys._MEIPASS)
    return Path(__file__).resolve().parent


# permet de lancer "python main.py" depuis n'importe quel dossier
sys.path.insert(0, str(_racine() if getattr(sys, "frozen", False) else Path(__file__).resolve().parent))

from bindings.fincore_ffi import BibliothequeIntrouvable, charger   # noqa: E402


def main() -> int:
    app = QApplication(sys.argv)
    app.setApplicationName("FinAxis")
    qss = _racine() / "resources" / "style.qss"
    if qss.is_file():
        app.setStyleSheet(qss.read_text(encoding="utf-8"))
    try:
        ffi = charger()
    except BibliothequeIntrouvable as e:
        QMessageBox.critical(None, "FinAxis", str(e))
        return 1

    from ui.context import AppContext
    from ui.windows.main_window import MainWindow

    ctx = AppContext(ffi)
    fenetre = MainWindow(ctx)
    fenetre.show()
    fenetre.demarrer()                 # choix : nouvelle entreprise / fichier / demo
    return app.exec()


if __name__ == "__main__":
    sys.exit(main())
