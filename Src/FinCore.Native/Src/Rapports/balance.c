#include <stdio.h>
#include <string.h>
#include "src/FinCore.Native/include/rapports.h"
#include "src/FinCore.Native/include/ecritures.h"
#include "src/FinCore.Native/include/periodes.h"

/* la necessite des fonctions sous formes "STATIC" pour le bon fonctionnement du la generation du Balance 
  et la validation des champs */


/* Indice du compte dans comptes[] */
static Etat trouver_indice_compte(const Compte *comptes, size_t nb_comptes, id_compte id, size_t *indice_out){
    size_t i;

    for (i = 0; i < nb_comptes; i++) {
        if (comptes[i].id == id) {
            *indice_out = i;
            return ETAT_OK;
        }
    }
    return ERR_COMPTE_INTROUVABLE;
}

/* Une écriture compte si elle est comptabilisée et datée dans [debut, fin] */
static bool ecriture_retenue(const Ecriture *ecr, DATE debut, DATE fin){
  
    return ecritures_est_comptabilisee(ecr) && comparer_dates(ecr->date, debut) >= 0 && comparer_dates(ecr->date, fin) <= 0;
}

Etat balance_generer(const Compte *comptes, size_t nb_comptes, const Ecriture *ecritures, size_t nb_ecritures, DATE debut, DATE fin, LigneBalance *sortie, size_t capacite, size_t *nb_sortie){

    size_t i, j, k;          /* indices des boucles : comptes, ecritures, lignes */
    size_t nb_retenues = 0;  /* nombre d'ecritures prises en compte dans la periode */
    size_t indice = 0;       /* indice du compte trouve pour une ligne */
    Etat etat;

    if (!comptes || !nb_sortie) return ERR_POINTEUR_NULLE;
    if (!ecritures && nb_ecritures > 0) return ERR_POINTEUR_NULLE;
    if (!periodes_plage_valide(debut,fin)) return ERR_PLAGE_DATES_INVALIDE ; /* la date fin est avant la date de debut */

    *nb_sortie = nb_comptes;
    if (capacite < nb_comptes)
        return ERR_TRES_PETIT_BUFFER;
    if (sortie == NULL && nb_comptes > 0)
        return ERR_POINTEUR_NULLE;   /* il faut un tableau de sortie des qu'il y a des comptes */

    /* la generation d'un boucle for avec l appelle des deux onctions de la bilblio <string.h>
        - memset : ou cette fonction est de la forme memset(destination, valeur, nombre_octets) cad du l'adresse memoire(dest) remplit toute sa zone mémoire avec des octets à 0 
        - snprintf : ou cette fonction est de la forme snprintf(destination, taille_max, format, valeur) cad valeur vers destination, sans dépasser la taille du tableau destination (taille_max) avec
        une format donnee (%s --> chaine de caracteres ) */

    for (i = 0; i < nb_comptes; i++) {
        memset(&sortie[i], 0, sizeof(sortie[i]));
        sortie[i].compte.id = comptes[i].id;
        snprintf(sortie[i].compte.code, sizeof(sortie[i].compte.code), "%s", comptes[i].code);
        snprintf(sortie[i].compte.nom,  sizeof(sortie[i].compte.nom),  "%s", comptes[i].nom);
    }

    /* le parcours du tableau d'ecritures */
    for (j = 0; j < nb_ecritures; j++) {
        const Ecriture *ecr = &ecritures[j];

        if (!ecriture_retenue(ecr, debut, fin))
            continue;
        if (ecr->nombre_lignes > 0 && ecr->lignes == NULL)
            return ERR_POINTEUR_NULLE;
        nb_retenues++;

        for (k = 0; k < ecr->nombre_lignes; k++) {
            const ligne_journal *ligne = &ecr->lignes[k];
            Monnaie d = ligne->Debit;
            Monnaie c = ligne->credit;

            etat = trouver_indice_compte(comptes, nb_comptes, ligne->compte_id, &indice);
            if (etat != ETAT_OK)
                return etat;

            sortie[indice].total_debit  += d;
            sortie[indice].total_credit += c;
        }
    }
      for (i = 0; i < nb_comptes; i++) {
          Monnaie solde = sortie[i].total_debit - sortie[i].total_credit;
  
          if (solde > 0)
              sortie[i].solde_debit = solde;
          else
              sortie[i].solde_credit = -solde;    /* 0 si solde nul */
      }
  
      /* sortie reste remplie a zero :: au cas du aucune retenue */
      if (nb_retenues == 0)
          return ERR_AUCUNE_DONNEE_PERIODE;
  
      return ETAT_OK; /* l etat qui passe tous les erreurs et assure le travail du back-end du balance et la validation de tous les champs */
}

/* la somme des debits de toute la balance 
   on parcourt chaque ligne du tableau et on additionne son champ total_debit */
Monnaie balance_total_debit(const LigneBalance *lignes, size_t nombre){
    Monnaie somme = 0;
    size_t i;
 
    /* le cas d absence de tableau : NULL a additionner */
    if (!lignes) return 0; /* sous le type monnaie */
 
    for (i = 0; i < nombre; i++) {
        somme += lignes[i].total_debit;
    }
 
    return somme;
}

/* la somme des credits de toute la balance 
   meme principe que balance_total_debit mais avec total_credit */
Monnaie balance_total_credit(const LigneBalance *lignes, size_t nombre){
    Monnaie somme = 0;
    size_t i;
 
    /* pas de tableau : il n y a rien a additionner */
    if (!lignes) return 0;
 
    for (i = 0; i < nombre; i++) {
        somme += lignes[i].total_credit;
    }
 
    return somme;
}

/* la balance est equilibree quand la somme de tous les debits est egale a la somme de tous les credits
   (c est le principe de la partie double : chaque ecriture debite autant qu elle credite)
   true = equilibree, false = desequilibree (ou pas de tableau a verifier)
  ceci est l'affirmation generale pour la verification d une balance equilibree 
  
  voir documentation/explication_comptabilite/balance_equilibre :: pour plus d'informations et mieux du professionalisme et bonne explication du
  cette fonction
  */

bool balance_est_equilibree(const LigneBalance *lignes, size_t nombre){
 
    if (!lignes) return false; /* la verification comme toujouts du l'existance du ligne du balance comme les deux fonctions precedants "balance_total_debit/balance_total_credit" */
 
    return balance_total_debit(lignes, nombre) == balance_total_credit(lignes, nombre); /* la fonction retourne la verification si le total_debit = tital_credit */

  /*puisque la fonction du type output BOOLEEN , elle recoit TRUE si et seulement si total_debit = tital_credit sinon retourne FALSE dans tout autres cas */
}

/* la verification complete de la balance (meme role que ecritures_valider pour une ecriture) 
   SANS calculer , elle controle que les chiffres de la balance --> elle retourne le premier probleme trouve sous forme de code d erreur */

Etat balance_verifier(const LigneBalance *lignes, size_t nombre){
    size_t i;
 
    if (!lignes) return ERR_POINTEUR_NULLE;
 
    if (nombre == 0) return ERR_AUCUNE_DONNEE_PERIODE; /* l absence des donness du periode*/
 
    /* 1. le total des debits doit etre egal au total des credits */
    if (!balance_est_equilibree(lignes, nombre)) return ERR_BALANCE_NON_EQUILIBREE; /*le total des debits doit etre egal au total des credit */
 
    /* l adoption du boucle for qui va contenir plusieurs conditions de verifications*/
    for (i = 0; i < nombre; i++) {
        const LigneBalance *ligne = &lignes[i];
 
        /* un montant negatif n a pas de sens dans une balance */
        if (ligne->total_debit < 0 || ligne->total_credit < 0 || ligne->solde_debit < 0 || ligne->solde_credit < 0)
            return ERR_BALANCE_DEBIT_CREDIT_INCOHERENTS;
 
        /* le solde est d un seul cote : debiteur OU crediteur, jamais les deux en meme temps */
        if (ligne->solde_debit > 0 && ligne->solde_credit > 0)
            return ERR_BALANCE_DEBIT_CREDIT_INCOHERENTS;
 
        /* le solde doit correspondre aux totaux : solde_debit - solde_credit == total_debit - total_credit */
        if (ligne->solde_debit - ligne->solde_credit != ligne->total_debit - ligne->total_credit)
            return ERR_BALANCE_DEBIT_CREDIT_INCOHERENTS;
    }
 
    return ETAT_OK;
}
 
