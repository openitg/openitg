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
#include "tomcrypt.h"

/**
  @file rsa_free.c
  Free an RSA key, Tom St Denis
*/  

#ifdef MRSA

/**
  Free an RSA key from memory
  @param key   The RSA key to free
*/
void rsa_free(rsa_key *key)
{
   LTC_ARGCHKVD(key != NULL);
   mp_clear_multi(key->e, key->d, key->N, key->dQ, key->dP, key->qP, key->p, key->q, NULL);
}

#endif

/* $Source: /cvsroot/stepmania/stepmania/src/libtomcrypt/src/pk/rsa/rsa_free.c,v $ */
/* $Revision: 1.1 $ */
/* $Date: 2007/01/24 05:16:53 $ */
