"""Feuille 'Détection' : anomalies colorees selon la gravite."""
from ..styles import cell_styles as st
from ..styles.color_palette import GRAVITE_COULEURS

METHODES = ["Benford", "Doublons", "Montants ronds", "Valeurs aberrantes"]
GRAVITES = ["Info", "Faible", "Moyenne", "Élevée", "Critique"]


def ecrire(ws, resultats, comptes_par_id: dict[int, str], entreprise: str) -> None:
    ws.title = "Détection"
    ws["A1"], ws["A1"].font = f"Détection d'anomalies - {entreprise}", st.TITRE
    st.ecrire_entetes(ws, 3, ["Méthode", "Gravité", "Score", "Compte", "Ligne", "Description"])
    ligne = 4
    for r in resultats:
        valeurs = [METHODES[int(r.methode)], GRAVITES[int(r.gravite)], r.score,
                   comptes_par_id.get(r.id_compte, str(r.id_compte)), r.id_ligne, r.description]
        for col, v in enumerate(valeurs, start=1):
            cell = ws.cell(row=ligne, column=col, value=v)
            cell.border = st.BORDURE
            cell.fill = st.fond(GRAVITE_COULEURS[int(r.gravite)])
            if col == 3:
                cell.number_format = "0.00"
        ligne += 1
    ws.freeze_panes = "A4"
    st.largeurs(ws, [20, 12, 8, 30, 8, 80])
