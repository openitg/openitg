

/*
 ---------------------------------------------------------------------------
 Copyright (c) 2003, Dr Brian Gladman <brg@gladman.me.uk>, Worcester, UK.
 All rights reserved.

 LICENSE TERMS

 The free distribution and use of this software in both source and binary
 form is allowed (with or without changes) provided that:

   1. distributions of this source code include the above copyright
      notice, this list of conditions and the following disclaimer;

   2. distributions in binary form include the above copyright
      notice, this list of conditions and the following disclaimer
      in the documentation and/or other associated materials;

   3. the copyright holder's name is not used to endorse products
      built using this software without specific written permission.

 ALTERNATIVELY, provided that this notice is retained in full, this product
 may be distributed under the terms of the GNU General Public License (GPL),
 in which case the provisions of the GPL apply INSTEAD OF those given above.

 DISCLAIMER

 This software is provided 'as is' with no explicit or implied warranties
 in respect of its properties, including, but not limited to, correctness
 and/or fitness for purpose.
 ---------------------------------------------------------------------------
 Issue Date: 1/06/2003

 This file contains the definitions required to use AES (Rijndael) in C++.
*/

#ifndef _AESCPP_H
#define _AESCPP_H

#include "aes.h"

extern "C" {
	aes_rval aes_encrypt(const unsigned char *in, unsigned char *out, const aes_encrypt_ctx cx[1]);
	aes_rval aes_decrypt(const unsigned char *in, unsigned char *out, const aes_decrypt_ctx cx[1]);
}

class AESencrypt
{   aes_encrypt_ctx cx[1];
public:
    AESencrypt(void)
            { };
#ifdef  AES_128
    AESencrypt(const unsigned char key[])
            { aes_encrypt_key128(key, cx); }
    aes_rval key128(const unsigned char key[])
            { return aes_encrypt_key128(key, cx); }
#endif
#ifdef  AES_192
    aes_rval key192(const unsigned char key[])
            { return aes_encrypt_key192(key, cx); }
#endif
#ifdef  AES_256
    aes_rval key256(const unsigned char key[])
            { return aes_encrypt_key256(key, cx); }
#endif
#ifdef  AES_VAR
    aes_rval key(const unsigned char key[], int key_len)
            { return aes_encrypt_key(key, key_len, cx); }
#endif
    aes_rval encrypt(const unsigned char in[], unsigned char out[]) const
            { return aes_encrypt(in, out, cx);  }
};

class AESdecrypt
{   aes_decrypt_ctx cx[1];
public:
    AESdecrypt(void)
            { };
#ifdef  AES_128
    AESdecrypt(const unsigned char key[])
            { aes_decrypt_key128(key, cx); }
    aes_rval key128(const unsigned char key[])
            { return aes_decrypt_key128(key, cx); }
#endif
#ifdef  AES_192
    aes_rval key192(const unsigned char key[])
            { return aes_decrypt_key192(key, cx); }
#endif
#ifdef  AES_256
    aes_rval key256(const unsigned char key[])
            { return aes_decrypt_key256(key, cx); }
#endif
#ifdef  AES_VAR
    aes_rval key(const unsigned char key[], int key_len)
            { return aes_decrypt_key(key, key_len, cx); }
#endif
    aes_rval decrypt(const unsigned char in[], unsigned char out[]) const
            { return aes_decrypt(in, out, cx);  }
};

#endif
