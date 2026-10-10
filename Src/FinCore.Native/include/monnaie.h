/* ce fichier va contenir tous les propreites concernant les monnaies du systeme financier */
#ifndef MONNAIE_H
#define MONNAIE_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct {
    int64_t unites_min ;
    int8_t echelle;
}monnaie;

/* creation un montant nul a un echelle specefique */
monnaie initiation_monnaie(int8_t echelle);

/* Cree une monnaie a partir de sa valeur en unites mineures
 * Exemple : monnaie_depuis_unites_mineures(12345, 3)    = 12.345 */
monnaie monnaie_depuis_unites_mineures( int64_t unites_min, int8_t echelle);

/* Cree une monnaie a partir d une chaine de caracteres.
 * Exemple :    "123.456" avec une echelle de 3
 *     --> donne 123456 unites mineures.
 * Le parametre "success" indique si la conversion a reussi.
 */
monnaie monnaie_depuis_chaine(const char *chaine, int8_t echelle, bool *success);

/* --------------------------------
        operations arithmetiques
-----------------------------------*/

/*    addition de deux montants     */
monnaie ajouter_monnaie(monnaie m1 ,monnaie m2 , bool *success);

/* soustraction le deuxieme montant du premier */
monnaie soustraire_monnaie(monnaie m1,monnaie m2 , bool *success);

/* Multiplie un montant par un nombre de points de base.
 *
 * 100 points de base  = 1.00 %
 * 500 points de base  = 5.00 %
 * 1000 points de base = 10.00 %
 *
 * Utilise pour :
 *     - les taxes
 *     - les interets
 *     - les commissions
 *     - les taux financiers
 */
monnaie monnaie_multiplier_points_de_base(monnaie m, int32_t points_de_base, bool *success);

/* --------------------------------
     operations du comparison
-----------------------------------*/

/*
 * Compare deux montants
 * Retourne :
 --> -1 si a < b
 --> 0 si a == b
  --> 1 si a > b
 */
int monnaie_comparer(monnaie m1,monnaie m2);

bool monnaie_est_negative(monnaie m);
bool monnaie_est_nulle(monnaie m);

/* --------------------------------
     operation de formatage
-----------------------------------*/


/*
 * Convertit une monnaie en chaine de caracteres.
 *
 * Exemple :
 *     Monnaie = 12345, echelle = 3
 *     Resultat = "12.345"
 */
void monnaie_en_chaine(monnaie m,char *buffer,size_t taille_buffer);

#endif
