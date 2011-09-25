#include "global.h"
#include "CryptHelpers.h"
#include "RageFile.h"
#include "RageUtil.h"
#include "RageLog.h"

// crypt headers
#include "libtomcrypt/src/headers/tomcrypt.h"

static ltc_prng_descriptor *g_PRNGDesc; // HACK: this _MIGHT_ be better off as g_SigHashDesc or something
static ltc_hash_descriptor *g_SHA1Desc;

static int g_SHA1DescId;
static int g_PRNGDescId;

#define KEY_BITLENGTH 1024

void CryptHelpers::Init()
{
	ltc_mp = ltm_desc;
	g_PRNGDescId = register_prng( &yarrow_desc );
	if ( g_PRNGDescId == -1 )
		RageException::Throw( "Could not register PRNG Descriptor" );
	g_PRNGDesc = &yarrow_desc;

	g_SHA1DescId = register_hash( &sha1_desc );
	if ( g_SHA1DescId == -1 )
		RageException::Throw( "Could not register SHA1 Descriptor" );
	g_SHA1Desc = &sha1_desc;
}

static void PKCS8EncodePrivateKey( unsigned char *buf, unsigned long bufsize, CString &sPrivateKey )
{

}

static bool PKCS8DecodePrivateKey( /* ... */ )
{

}

bool CryptHelpers::GenerateRSAKey( unsigned int keyLength, CString sSeed, CString &sPublicKey, CString &sPrivateKey )
{
#ifdef _XBOX
	return false;
#else
	int iRet;
	rsa_key key;

	iRet = rsa_make_key( g_PRNGDesc, g_PRNGDescId, KEY_BITLENGTH/8, 65537, &key );
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
		LOG->Warn( "RSA Private Key Export error: %s", error_tO_string(iRet) );
		return false;
	}

	PKCS8EncodePrivateKey( buf, &bufsize, sPrivateKey );
	return true;
#endif
}

bool CryptHelpers::SignFile( RageFileBasic &file, CString sPrivKey, CString &sSignatureOut, CString &sError )
{
#ifdef _XBOX
	return false;
#else
	try {
		StringSource privFile( sPrivKey, true );
		RSASSA_PKCS1v15_SHA_Signer priv(privFile);
		NonblockingRng rng;

		/* RageFileSource will delete the file we give to it, so make a copy. */
		RageFileSource f( file.Copy(), true, new SignerFilter(rng, priv, new StringSink(sSignatureOut)) );
	} catch( const CryptoPP::Exception &s ) {
		LOG->Warn( "SignFileToFile failed: %s", s.what() );
		return false;
	}

	return true;
#endif
}

bool CryptHelpers::VerifyFile( RageFileBasic &file, CString sSignature, CString sPublicKey, CString &sError )
{
	try {
		StringSource pubFile( sPublicKey, true );
		RSASSA_PKCS1v15_SHA_Verifier pub(pubFile);

		if( sSignature.size() != pub.SignatureLength() )
		{
			sError = ssprintf( "Invalid signature length: got %i, expected %i", sSignature.size(), pub.SignatureLength() );
			return false;
		}

		VerifierFilter *verifierFilter = new VerifierFilter(pub);
		verifierFilter->Put( (byte *) sSignature.data(), sSignature.size() );

		/* RageFileSource will delete the file we give to it, so make a copy. */
		RageFileSource f( file.Copy(), true, verifierFilter );

		if( !verifierFilter->GetLastResult() )
		{
			sError = ssprintf( "Signature mismatch" );
			return false;
		}
		
		return true;
	} catch( const CryptoPP::Exception &s ) {
		sError = s.what();
		return false;
	}
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
