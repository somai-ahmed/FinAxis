"""Feuille 'Balance generale'."""
from datetime import date

from ..styles import cell_styles as st


def ecrire(ws, lignes, entreprise: str, debut: date, fin: date, devise: str = "TND") -> None:
    ws.title = "Balance"
    ws["A1"], ws["A1"].font = f"Balance générale - {entreprise}", st.TITRE
    ws["A2"], ws["A2"].font = f"Du {debut:%d/%m/%Y} au {fin:%d/%m/%Y} ({devise})", st.SOUS_TITRE
    st.ecrire_entetes(ws, 4, ["Code", "Compte", "Total débit", "Total crédit", "Solde débiteur", "Solde créditeur"])
    ligne = 5
    for r in lignes:
        valeurs = [r.code, r.nom] + [v / 1000 for v in (r.total_debit, r.total_credit, r.solde_debit, r.solde_credit)]
        for col, v in enumerate(valeurs, start=1):
            cell = ws.cell(row=ligne, column=col, value=v)
            cell.border = st.BORDURE
            if col > 2:
                cell.number_format = st.FORMAT_MONTANT
        ligne += 1
    # totaux avec de vraies formules : l'utilisateur peut les verifier dans Excel
    ws.cell(row=ligne, column=2, value="TOTAUX")
    for col in range(3, 7):
        lettre = chr(64 + col)
        ws.cell(row=ligne, column=col, value=f"=SUM({lettre}5:{lettre}{ligne - 1})" if lignes else 0).number_format = st.FORMAT_MONTANT
    for col in range(1, 7):
        cell = ws.cell(row=ligne, column=col)
        cell.font, cell.fill, cell.border = st.TOTAL_FONT, st.TOTAL_FOND, st.BORDURE
    ws.freeze_panes = "A5"
    st.largeurs(ws, [12, 40, 16, 16, 16, 16])
