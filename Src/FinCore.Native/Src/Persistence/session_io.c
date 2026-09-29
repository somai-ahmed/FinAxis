/*
 * session_io.c -- implementation de la Session du moteur FinCore
 *
 * La Session est la "memoire" du moteur pour UNE entreprise :
 * elle garde le plan comptable, le journal et la configuration.
 * La structure est definie ici seulement (opaque) : le reste du code,
 * et Python via ctypes, ne voit qu'un pointeur Session*.
 */


#include <stdlib.h>  /* pour calloc/realloc/free : allocation dynamique */
#include <string.h>  /* pour memcpy/strncpy/strcmp : copier et comparer */

#include "Src/FinCore.Native/include/session.h"
#include "Src/FinCore.Native/include/comptes.h"
#include "Src/FinCore.Native/include/ecritures.h"
