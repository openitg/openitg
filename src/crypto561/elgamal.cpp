// elgamal.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "elgamal.h"
#include "asn.h"
#include "nbtheory.h"

NAMESPACE_BEGIN(CryptoPP)

void ElGamal_TestInstantiations()
{
	ElGamalEncryptor test1(1, 1, 1);
	ElGamalDecryptor test2(NullRNG(), 123);
	ElGamalEncryptor test3(test2);
}

NAMESPACE_END
