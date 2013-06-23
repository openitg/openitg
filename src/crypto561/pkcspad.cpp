// pkcspad.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"

#ifndef CRYPTOPP_PKCSPAD_CPP	// SunCC workaround: compiler could cause this file to be included twice
#define CRYPTOPP_PKCSPAD_CPP

#include "pkcspad.h"
#include <assert.h>

NAMESPACE_BEGIN(CryptoPP)

// more in dll.cpp
template<> const byte PKCS_DigestDecoration<Weak1::MD2>::decoration[] = {0x30,0x20,0x30,0x0c,0x06,0x08,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x02,0x05,0x00,0x04,0x10};
template<> const unsigned int PKCS_DigestDecoration<Weak1::MD2>::length = sizeof(PKCS_DigestDecoration<Weak1::MD2>::decoration);

template<> const byte PKCS_DigestDecoration<Weak1::MD5>::decoration[] = {0x30,0x20,0x30,0x0c,0x06,0x08,0x2a,0x86,0x48,0x86,0xf7,0x0d,0x02,0x05,0x05,0x00,0x04,0x10};
template<> const unsigned int PKCS_DigestDecoration<Weak1::MD5>::length = sizeof(PKCS_DigestDecoration<Weak1::MD5>::decoration);

template<> const byte PKCS_DigestDecoration<RIPEMD160>::decoration[] = {0x30,0x21,0x30,0x09,0x06,0x05,0x2b,0x24,0x03,0x02,0x01,0x05,0x00,0x04,0x14};
template<> const unsigned int PKCS_DigestDecoration<RIPEMD160>::length = sizeof(PKCS_DigestDecoration<RIPEMD160>::decoration);

template<> const byte PKCS_DigestDecoration<Tiger>::decoration[] = {0x30,0x29,0x30,0x0D,0x06,0x09,0x2B,0x06,0x01,0x04,0x01,0xDA,0x47,0x0C,0x02,0x05,0x00,0x04,0x18};
template<> const unsigned int PKCS_DigestDecoration<Tiger>::length = sizeof(PKCS_DigestDecoration<Tiger>::decoration);

size_t PKCS_EncryptionPaddingScheme::MaxUnpaddedLength(size_t paddedLength) const
{
	return SaturatingSubtract(paddedLength/8, 10U);
}

void PKCS_EncryptionPaddingScheme::Pad(RandomNumberGenerator &rng, const byte *input, size_t inputLen, byte *pkcsBlock, size_t pkcsBlockLen, const NameValuePairs &parameters) const
{
	assert (inputLen <= MaxUnpaddedLength(pkcsBlockLen));	// this should be checked by caller

	// convert from bit length to byte length
	if (pkcsBlockLen % 8 != 0)
	{
		pkcsBlock[0] = 0;
		pkcsBlock++;
	}
	pkcsBlockLen /= 8;

	pkcsBlock[0] = 2;  // block type 2

	// pad with non-zero random bytes
	for (unsigned i = 1; i < pkcsBlockLen-inputLen-1; i++)
		pkcsBlock[i] = (byte)rng.GenerateWord32(1, 0xff);

	pkcsBlock[pkcsBlockLen-inputLen-1] = 0;     // separator
	memcpy(pkcsBlock+pkcsBlockLen-inputLen, input, inputLen);
}

DecodingResult PKCS_EncryptionPaddingScheme::Unpad(const byte *pkcsBlock, size_t pkcsBlockLen, byte *output, const NameValuePairs &parameters) const
{
	bool invalid = false;
	size_t maxOutputLen = MaxUnpaddedLength(pkcsBlockLen);

	// convert from bit length to byte length
	if (pkcsBlockLen % 8 != 0)
	{
		invalid = (pkcsBlock[0] != 0) || invalid;
		pkcsBlock++;
	}
	pkcsBlockLen /= 8;

	// Require block type 2.
	invalid = (pkcsBlock[0] != 2) || invalid;

	// skip past the padding until we find the separator
	size_t i=1;
	while (i<pkcsBlockLen && pkcsBlock[i++]) { // null body
		}
	assert(i==pkcsBlockLen || pkcsBlock[i-1]==0);

	size_t outputLen = pkcsBlockLen - i;
	invalid = (outputLen > maxOutputLen) || invalid;

	if (invalid)
		return DecodingResult();

	memcpy (output, pkcsBlock+i, outputLen);
	return DecodingResult(outputLen);
}

// ********************************************************

#ifndef CRYPTOPP_IMPORTS

void PKCS1v15_SignatureMessageEncodingMethod::ComputeMessageRepresentative(RandomNumberGenerator &rng, 
	const byte *recoverableMessage, size_t recoverableMessageLength,
	HashTransformation &hash, HashIdentifier hashIdentifier, bool messageEmpty,
	byte *representative, size_t representativeBitLength) const
{
	assert(representativeBitLength >= MinRepresentativeBitLength(hashIdentifier.second, hash.DigestSize()));

	size_t pkcsBlockLen = representativeBitLength;
	// convert from bit length to byte length
	if (pkcsBlockLen % 8 != 0)
	{
		representative[0] = 0;
		representative++;
	}
	pkcsBlockLen /= 8;

	representative[0] = 1;   // block type 1

	unsigned int digestSize = hash.DigestSize();
	byte *pPadding = representative + 1;
	byte *pDigest = representative + pkcsBlockLen - digestSize;
	byte *pHashId = pDigest - hashIdentifier.second;
	byte *pSeparator = pHashId - 1;

	// pad with 0xff
	memset(pPadding, 0xff, pSeparator-pPadding);
	*pSeparator = 0;
	memcpy(pHashId, hashIdentifier.first, hashIdentifier.second);
	hash.Final(pDigest);
}

#endif

NAMESPACE_END

#endif
