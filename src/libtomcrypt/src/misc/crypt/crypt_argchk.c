/* LibTomCrypt, modular cryptographic library -- Tom St Denis
 *
 * LibTomCrypt is a library that provides various cryptographic
 * algorithms in a highly modular and flexible manner.
 *
 * The library is free for all purposes without any express
 * guarantee it works.
 *
 * Tom St Denis, tomstdenis@gmail.com, http://libtomcrypt.com
 */

#ifndef _MSC_VER
#include "tomcrypt.h"
#include <signal.h>
#endif

/**
  @file crypt_argchk.c
  Perform argument checking, Tom St Denis
*/  


#if (ARGTYPE == 0)
void crypt_argchk(char *v, char *s, int d)
{
	#ifndef _MSC_VER
 fprintf(stderr, "LTC_ARGCHK '%s' failure on line %d of file %s\n",
         v, d, s);
 (void)raise(SIGABRT);
	#endif
}
#endif

/* $Source$ */
/* $Revision$ */
/* $Date$ */
