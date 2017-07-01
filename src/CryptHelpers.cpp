#include "global.h"
#include "CryptHelpers.h"
#include "RageFile.h"
#include "RageUtil.h"
#include "RageLog.h"

// tomcrypt want to #define ENDIAN_LITTLE, which is also in our own config.h.  We're not using it in this file anyway.
#undef ENDIAN_LITTLE

#include "libtomcrypt/src/headers/tomcrypt.h"

/* XXX huge hack to get stuff working */
#if defined(WIN32)
	#if defined(_DEBUG)
		#pragma comment(lib, "libtomcrypt/Debug/tomcrypt.lib")
		#pragma comment(lib, "libtommath/Debug/tommath.lib")
	#else
		#if defined(WITH_SSE2)
			#pragma comment(lib, "libtomcrypt/Release-SSE2/tomcrypt.lib")
			#pragma comment(lib, "libtommath/Release-SSE2/tommath.lib")
		#else
			#pragma comment(lib, "libtomcrypt/Release/tomcrypt.lib")
			#pragma comment(lib, "libtommath/Release/tommath.lib")
		#endif
	#endif
#endif

static const ltc_prng_descriptor *g_PRNGDesc; // HACK: this _MIGHT_ be better off as g_SigHashDesc or something
static const ltc_hash_descriptor *g_SHA1Desc;

static int g_SHA1DescId;
static int g_PRNGDescId;
static prng_state g_PRNGState;

#define KEY_BITLENGTH 1024

void CryptHelpers::Init()
{
	ltc_mp = ltm_desc;
	g_PRNGDescId = register_prng( &yarrow_desc );
	if ( g_PRNGDescId == -1 )
		RageException::Throw( "Could not register PRNG Descriptor" );
	g_PRNGDesc = &yarrow_desc;
	rng_make_prng(1024, g_PRNGDescId, &g_PRNGState, NULL);

	g_SHA1DescId = register_hash( &sha1_desc );
	if ( g_SHA1DescId == -1 )
		RageException::Throw( "Could not register SHA1 Descriptor" );
	g_SHA1Desc = &sha1_desc;
}

static void PKCS8EncodePrivateKey( unsigned char *pkbuf, unsigned long bufsize, CString &sOut )
{
	
}

/* "Decode" */
static bool PKCS8DecodePrivateKey( const unsigned char *pkcsbuf, unsigned long bufsize, unsigned char *szOut, unsigned long &outsize )
{
	ltc_asn1_list *pkAlgObject;
	der_decode_sequence_flexi(pkcsbuf, &bufsize, &pkAlgObject);

#define RET_IF_NULL_THEN_ASSIGN(x ) { if ( (x) == NULL ) return false; pkAlgObject = (x); }
	RET_IF_NULL_THEN_ASSIGN( pkAlgObject );
	RET_IF_NULL_THEN_ASSIGN( pkAlgObject->child );
	RET_IF_NULL_THEN_ASSIGN( pkAlgObject->next );
	RET_IF_NULL_THEN_ASSIGN( pkAlgObject->next );
#undef RET_IF_NULL_THEN_ASSIGN

	outsize = pkAlgObject->size;
	memcpy( szOut, pkAlgObject->data, outsize );
	return true;
}

static bool GetSha1ForFile( RageFileBasic &f, unsigned char *szHash )
{
	hash_state sha1e;
	sha1_init(&sha1e);

	int origpos = f.Tell();
	f.Seek(0);
	unsigned char buf[4096];
	int got = f.Read(buf, 4096);
	if ( got == -1 )
		return false;
	while (got > 0)
	{
		sha1_process(&sha1e, buf, got);
		got = f.Read(buf, 4096);
		if ( got == -1 )
			return false;
	}
	sha1_done(&sha1e, szHash);
	f.Seek(origpos);
	return true;
}

#ifdef UNUSED_CODE
static bool GetSha1ForFile( CString &sFile, unsigned char *szHash )
{
	RageFile f;
	f.Open(sFile, RageFile::READ);
	bool bGot = GetSha1ForFile( f, szHash );
	f.Close();
	return bGot;
}
#endif

bool CryptHelpers::GenerateRSAKey( unsigned int keyLength, CString sSeed, CString &sPublicKey, CString &sPrivateKey )
{
#ifdef _XBOX
	return false;
#else
	int iRet;
	rsa_key key;

	iRet = rsa_make_key( &g_PRNGState, g_PRNGDescId, KEY_BITLENGTH/8, 65537, &key );
	if ( iRet != CRYPT_OK )
	{
		LOG->Warn( "GenerateRSAKey error: %s", error_to_string(iRet) );
		return false;
	}

	unsigned char buf[2048];
	unsigned long bufsize = sizeof(buf);

	iRet = rsa_export( buf, &bufsize, PK_PUBLIC, &key );
	if ( iRet != CRYPT_OK )
	{
		LOG->Warn( "RSA Public Key Export error: %s", error_to_string(iRet) );
		return false;
	}
	sPublicKey = CString( (const char*)buf, bufsize );

	iRet = rsa_export( buf, &bufsize, PK_PRIVATE, &key );
	if ( iRet != CRYPT_OK )
	{
		LOG->Warn( "RSA Private Key Export error: %s", error_to_string(iRet) );
		return false;
	}

	PKCS8EncodePrivateKey( buf, bufsize, sPrivateKey );
	return true;
#endif
}

bool CryptHelpers::SignFile( RageFileBasic &file, CString sPrivKey, CString &sSignatureOut, CString &sError )
{
#ifdef _XBOX
	return false;
#else
	unsigned char embedded_key[4096], filehash[20], sig[128];
	unsigned long keysize = 4096, sigsize = 128;
	int ret;
	rsa_key key;

	bool bDecoded = PKCS8DecodePrivateKey((const unsigned char *)sPrivKey.data(), sPrivKey.size(), embedded_key, keysize);
	if ( bDecoded )
		sPrivKey.assign((const char*)embedded_key, keysize);

	ret = rsa_import((const unsigned char*)sPrivKey.data(), sPrivKey.size(), &key);
	if ( ret != CRYPT_OK )
	{
		LOG->Warn("Could not import private key: %s", error_to_string(ret));
		return false;
	}
	bool bShaRet = GetSha1ForFile(file, filehash);
	if ( !bShaRet )
	{
		LOG->Warn("Could not get SHA1 for file");
		return false;
	}
	ret = rsa_sign_hash_ex( filehash, 20, sig, &sigsize, LTC_PKCS_1_V1_5, &g_PRNGState, g_PRNGDescId, g_SHA1DescId, 0, &key );
	if ( ret != CRYPT_OK )
	{
		LOG->Warn("Could not sign hash file: %s", error_to_string(ret));
		return false;
	}
	ASSERT( sigsize == 128 );

	sSignatureOut.assign( (const char *)sig, sigsize );
	return true;
#endif
}

bool CryptHelpers::VerifyFile( RageFileBasic &file, CString sSignature, CString sPublicKey, CString &sError )
{
	unsigned char buf_hash[20];
	rsa_key key;
	int ret = rsa_import( (const unsigned char *)sPublicKey.data(), sPublicKey.size(), &key );
	if ( ret != CRYPT_OK )
	{
		sError = ssprintf("Could not import public key: %s", error_to_string(ret));
		LOG->Warn(sError);
		return false;
	}
	bool bHashed = GetSha1ForFile( file, buf_hash );
	if ( !bHashed )
	{
		sError = ssprintf("Error while SHA1 hashing file");
		LOG->Warn(sError);
		return false;
	}
	
	int iMatch = 0;
	ret = rsa_verify_hash_ex( (const unsigned char*)sSignature.data(), sSignature.size(),
		buf_hash, sizeof(buf_hash), LTC_PKCS_1_V1_5, g_SHA1DescId, 0, &iMatch, &key );
	if ( ret != CRYPT_OK )
	{
		sError = ssprintf("Could not verify hash: %s", error_to_string(ret));
		LOG->Warn(sError);
		return false;
	}
	if ( iMatch == 0 )
	{
		sError = "Signature Mismatch";
		LOG->Warn(sError);
		return false;
	}
	return true;
}

/*
 * (c) 2001-2004 Chris Danford
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
