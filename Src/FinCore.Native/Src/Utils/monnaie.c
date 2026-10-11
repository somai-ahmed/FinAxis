#include <Src/FinCore.Native/include/monnaie.h>
#include <ctype.h>


/* creation d'un montant nul a une echelle donnee */
monnaie initiation_monnaie(int8_t echelle) {
    monnaie m;
    m.unites_min = 0;
    m.echelle = echelle;
    return m;
}

monnaie monnaie_depuis_unites_mineures(int64_t unites_min, int8_t echelle) {
    monnaie m = initiation_monnaie(echelle);
    m.unites_min = unites_min;
    return m;
}

monnaie monnaie_depuis_chaine(const char *chaine, int8_t echelle, bool *success) {
    monnaie m = initiation_monnaie(echelle);
    const char *p = chaine;
    int negatif = 0;
    int64_t valeur = 0;
    int decimales_lues = 0;   /* nombre de chiffres lus apres la virgule */

    if (success) *success = false;   /* echec par defaut, on le passe a true a la fin */
    if (chaine == NULL || echelle < 0) return m;

    /* 1. signe optionnel */
    if (*p == '+' || *p == '-') {
        negatif = (*p == '-');
        p++;
    }

    /* il faut au moins un chiffre apres le signe */
    if (!isdigit((unsigned char)*p)) return m;

    /* 2. partie entiere */
    while (isdigit((unsigned char)*p)) {
        int chiffre = *p - '0';
        if (valeur > (INT64_MAX - chiffre) / 10) return m;   /* depassement -> echec */
        valeur = valeur * 10 + chiffre;
        p++;
    }

    /* 3. partie fractionnaire (optionnelle, '.' ou ',') */
    if (*p == '.' || *p == ',') {
        p++;
        while (isdigit((unsigned char)*p)) {
            int chiffre = *p - '0';
            if (decimales_lues >= echelle) return m;   /* trop de decimales pour l'echelle */
            if (valeur > (INT64_MAX - chiffre) / 10) return m;
            valeur = valeur * 10 + chiffre;
            decimales_lues++;
            p++;
        }
    }

    /* 4. il ne doit rien rester apres le nombre */
    if (*p != '\0') return m;

    /* 5. completer avec des zeros si on a lu moins de decimales que l'echelle */
    while (decimales_lues < echelle) {
        if (valeur > INT64_MAX / 10) return m;
        valeur *= 10;
        decimales_lues++;
    }

    /* 6. appliquer le signe */
    m.unites_min = negatif ? -valeur : valeur;
    if (success) *success = true;
    return m;
}

monnaie ajouter_monnaie(monnaie m1, monnaie m2, bool *success) {
    monnaie resultat = initiation_monnaie(m1.echelle);
    if (success) *success = false;

    /* regle d'or : on n'additionne que des montants de meme echelle */
    if (m1.echelle != m2.echelle) return resultat;

    /* verification du depassement AVANT l'addition, jamais apres */
    if ((m2.unites_min > 0 && m1.unites_min > INT64_MAX - m2.unites_min) ||
        (m2.unites_min < 0 && m1.unites_min < INT64_MIN - m2.unites_min)) {
        return resultat;
    }

    resultat.unites_min = m1.unites_min + m2.unites_min;
    if (success) *success = true;
    return resultat;
}

monnaie soustraire_monnaie(monnaie m1, monnaie m2, bool *success) {
    monnaie resultat = initiation_monnaie(m1.echelle);
    if (success) *success = false;

    if (m1.echelle != m2.echelle) return resultat;

    /* m1 - m2 deborde si m2 > 0 et m1 trop proche de INT64_MIN,
     * ou si m2 < 0 et m1 trop proche de INT64_MAX */
    if ((m2.unites_min > 0 && m1.unites_min < INT64_MIN + m2.unites_min) ||
        (m2.unites_min < 0 && m1.unites_min > INT64_MAX + m2.unites_min)) {
        return resultat;
    }

    resultat.unites_min = m1.unites_min - m2.unites_min;
    if (success) *success = true;
    return resultat;
}

/* ============================================================
 * Multiplication par points de base (10000 points = 100 %)
 * ============================================================ */
monnaie monnaie_multiplier_points_de_base(monnaie m, int32_t points_de_base, bool *success) {
    monnaie resultat = initiation_monnaie(m.echelle);
    int64_t valeur = m.unites_min;
    int64_t pdb = (int64_t)points_de_base;

    if (success) *success = false;
    if (m.echelle < 0) return resultat;

    /* verification du depassement AVANT la multiplication */
    if (valeur != 0 && pdb != 0) {
        if ((valeur > 0 && pdb > 0 && valeur > INT64_MAX / pdb) ||
            (valeur < 0 && pdb < 0 && valeur < INT64_MAX / pdb) ||
            (valeur > 0 && pdb < 0 && pdb < INT64_MIN / valeur) ||
            (valeur < 0 && pdb > 0 && valeur < INT64_MIN / pdb)) {
            return resultat;
        }
    }

    /* division entiere par 10000 : arrondi vers zero */
    resultat.unites_min = (valeur * pdb) / 10000;
    if (success) *success = true;
    return resultat;
}

/*--------------------------
        COMPARAISON
---------------------------*/

/* retourne -1 si m1 < m2, 0 si egaux, 1 si m1 > m2
 * (0 aussi si les echelles different : comparaison impossible) */
int monnaie_comparer(monnaie m1, monnaie m2) {
    if (m1.echelle != m2.echelle) return 0;
    if (m1.unites_min < m2.unites_min) return -1;
    if (m1.unites_min > m2.unites_min) return 1;
    return 0;
}

bool monnaie_est_negative(monnaie m) {
    return m.unites_min < 0;
}

bool monnaie_est_nulle(monnaie m) {
    return m.unites_min == 0;
}

/*----------------------------
        FORMATAGE
-----------------------------*/
void monnaie_en_chaine(monnaie m, char *buffer, size_t taille_buffer) {
    char chiffres[32];   /* chiffres du montant, ecrits a l'envers */
    int n = 0;           /* nombre de chiffres ecrits dans chiffres[] */
    size_t pos = 0;
    int partie_entiere_len;
    int i;
    bool negatif = m.unites_min < 0;
    /* valeur absolue sans risque pour INT64_MIN */
    uint64_t valeur_abs = negatif ? (uint64_t)(-(m.unites_min + 1)) + 1 : (uint64_t)m.unites_min;

    if (buffer == NULL || taille_buffer == 0) return;
    buffer[0] = '\0';
    if (m.echelle < 0 || m.echelle > 18) return;

    if (valeur_abs == 0) {
        chiffres[n++] = '0';
    } else {
        while (valeur_abs > 0 && n < (int)sizeof(chiffres)) {
            chiffres[n++] = (char)('0' + (valeur_abs % 10));
            valeur_abs /= 10;
        }
    }

    /* il faut au moins un chiffre avant la virgule : on complete avec des zeros */
    while (n < m.echelle + 1 && n < (int)sizeof(chiffres)) {
        chiffres[n++] = '0';
    }

    if (negatif) {
        if (pos + 1 >= taille_buffer) { buffer[0] = '\0'; return; }
        buffer[pos++] = '-';
    }

    partie_entiere_len = n - m.echelle;

    /* partie entiere : on relit chiffres[] a l'endroit */
    for (i = 0; i < partie_entiere_len; i++) {
        if (pos + 1 >= taille_buffer) { buffer[0] = '\0'; return; }
        buffer[pos++] = chiffres[n - 1 - i];
    }

    /* partie decimale */
    if (m.echelle > 0) {
        if (pos + 1 >= taille_buffer) { buffer[0] = '\0'; return; }
        buffer[pos++] = '.';
        for (i = 0; i < m.echelle; i++) {
            if (pos + 1 >= taille_buffer) { buffer[0] = '\0'; return; }
            buffer[pos++] = chiffres[m.echelle - 1 - i];
        }
    }

    buffer[pos] = '\0';
}
