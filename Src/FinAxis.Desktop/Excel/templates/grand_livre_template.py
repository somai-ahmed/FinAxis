"""Feuille 'Grand livre' d'un compte."""
from datetime import date

from ..styles import cell_styles as st


def ecrire(ws, lignes, compte_libelle: str, entreprise: str, debut: date, fin: date, devise: str = "TND") -> None:
    ws.title = "Grand livre"
    ws["A1"], ws["A1"].font = f"Grand livre - {compte_libelle}", st.TITRE
    ws["A2"], ws["A2"].font = f"{entreprise} - du {debut:%d/%m/%Y} au {fin:%d/%m/%Y} ({devise})", st.SOUS_TITRE
    st.ecrire_entetes(ws, 4, ["Date", "Référence", "Libellé", "Débit", "Crédit", "Solde cumulé"])
    ligne = 5
    for r in lignes:
        valeurs = [r.date, r.reference, r.libelle, r.debit / 1000, r.credit / 1000, r.solde_cumule / 1000]
        for col, v in enumerate(valeurs, start=1):
            cell = ws.cell(row=ligne, column=col, value=v)
            cell.border = st.BORDURE
            if col == 1:
                cell.number_format = st.FORMAT_DATE
            elif col > 3:
                cell.number_format = st.FORMAT_MONTANT
        ligne += 1
    ws.cell(row=ligne, column=3, value="TOTAUX")
    for col in (4, 5):
        lettre = chr(64 + col)
        ws.cell(row=ligne, column=col, value=f"=SUM({lettre}5:{lettre}{ligne - 1})" if lignes else 0).number_format = st.FORMAT_MONTANT
    for col in range(1, 7):
        cell = ws.cell(row=ligne, column=col)
        cell.font, cell.fill, cell.border = st.TOTAL_FONT, st.TOTAL_FOND, st.BORDURE
    ws.freeze_panes = "A5"
    st.largeurs(ws, [12, 14, 44, 16, 16, 16])
