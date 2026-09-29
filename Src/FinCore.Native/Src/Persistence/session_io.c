/*
 * session_io.c -- implementation de la Session du moteur FinCore
 *
 * La Session est la "memoire" du moteur pour UNE entreprise :
 * elle garde le plan comptable, le journal et la configuration.
 * La structure est definie ici seulement (opaque) : le reste du code,
 * et Python via ctypes, ne voit qu'un pointeur Session*.
 */
