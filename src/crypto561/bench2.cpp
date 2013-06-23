// bench2.cpp - written and placed in the public domain by Wei Dai

#include "bench.h"
#include "validate.h"
#include "files.h"
#include "hex.h"

#include "rsa.h"
#include "nr.h"
#include "dsa.h"
#include "luc.h"
#include "rw.h"
#include "eccrypto.h"
#include "ecp.h"
#include "ec2n.h"
#include "asn.h"
#include "dh.h"
#include "mqv.h"
#include "xtrcrypt.h"
#include "esign.h"
#include "pssr.h"
#include "oids.h"
#include "randpool.h"

#include <time.h>
#include <math.h>
#include <iostream>
#include <iomanip>

USING_NAMESPACE(CryptoPP)
USING_NAMESPACE(std)

void OutputResultOperations(const char *name, const char *operation, bool pc, unsigned long iterations, double timeTaken);

void BenchMarkEncryption(const char *name, PK_Encryptor &key, double timeTotal, bool pc=false)
{
	unsigned int len = 16;
	SecByteBlock plaintext(len), ciphertext(key.CiphertextLength(len));
	GlobalRNG().GenerateBlock(plaintext, len);

	clock_t start = clock();
	unsigned int i;
	double timeTaken;
	for (timeTaken=(double)0, i=0; timeTaken < timeTotal; timeTaken = double(clock() - start) / CLOCK_TICKS_PER_SECOND, i++)
		key.Encrypt(GlobalRNG(), plaintext, len, ciphertext);

	OutputResultOperations(name, "Encryption", pc, i, timeTaken);

	if (!pc && key.GetMaterial().SupportsPrecomputation())
	{
		key.AccessMaterial().Precompute(16);
		BenchMarkEncryption(name, key, timeTotal, true);
	}
}

void BenchMarkDecryption(const char *name, PK_Decryptor &priv, PK_Encryptor &pub, double timeTotal)
{
	unsigned int len = 16;
	SecByteBlock ciphertext(pub.CiphertextLength(len));
	SecByteBlock plaintext(pub.MaxPlaintextLength(ciphertext.size()));
	GlobalRNG().GenerateBlock(plaintext, len);
	pub.Encrypt(GlobalRNG(), plaintext, len, ciphertext);

	clock_t start = clock();
	unsigned int i;
	double timeTaken;
	for (timeTaken=(double)0, i=0; timeTaken < timeTotal; timeTaken = double(clock() - start) / CLOCK_TICKS_PER_SECOND, i++)
		priv.Decrypt(GlobalRNG(), ciphertext, ciphertext.size(), plaintext);

	OutputResultOperations(name, "Decryption", false, i, timeTaken);
}

void BenchMarkSigning(const char *name, PK_Signer &key, double timeTotal, bool pc=false)
{
	unsigned int len = 16;
	AlignedSecByteBlock message(len), signature(key.SignatureLength());
	GlobalRNG().GenerateBlock(message, len);

	clock_t start = clock();
	unsigned int i;
	double timeTaken;
	for (timeTaken=(double)0, i=0; timeTaken < timeTotal; timeTaken = double(clock() - start) / CLOCK_TICKS_PER_SECOND, i++)
		key.SignMessage(GlobalRNG(), message, len, signature);

	OutputResultOperations(name, "Signature", pc, i, timeTaken);

	if (!pc && key.GetMaterial().SupportsPrecomputation())
	{
		key.AccessMaterial().Precompute(16);
		BenchMarkSigning(name, key, timeTotal, true);
	}
}

void BenchMarkVerification(const char *name, const PK_Signer &priv, PK_Verifier &pub, double timeTotal, bool pc=false)
{
	unsigned int len = 16;
	AlignedSecByteBlock message(len), signature(pub.SignatureLength());
	GlobalRNG().GenerateBlock(message, len);
	priv.SignMessage(GlobalRNG(), message, len, signature);

	clock_t start = clock();
	unsigned int i;
	double timeTaken;
	for (timeTaken=(double)0, i=0; timeTaken < timeTotal; timeTaken = double(clock() - start) / CLOCK_TICKS_PER_SECOND, i++)
		pub.VerifyMessage(message, len, signature, signature.size());

	OutputResultOperations(name, "Verification", pc, i, timeTaken);

	if (!pc && pub.GetMaterial().SupportsPrecomputation())
	{
		pub.AccessMaterial().Precompute(16);
		BenchMarkVerification(name, priv, pub, timeTotal, true);
	}
}

void BenchMarkKeyGen(const char *name, SimpleKeyAgreementDomain &d, double timeTotal, bool pc=false)
{
	SecByteBlock priv(d.PrivateKeyLength()), pub(d.PublicKeyLength());

	clock_t start = clock();
	unsigned int i;
	double timeTaken;
	for (timeTaken=(double)0, i=0; timeTaken < timeTotal; timeTaken = double(clock() - start) / CLOCK_TICKS_PER_SECOND, i++)
		d.GenerateKeyPair(GlobalRNG(), priv, pub);

	OutputResultOperations(name, "Key-Pair Generation", pc, i, timeTaken);

	if (!pc && d.GetMaterial().SupportsPrecomputation())
	{
		d.AccessMaterial().Precompute(16);
		BenchMarkKeyGen(name, d, timeTotal, true);
	}
}

void BenchMarkKeyGen(const char *name, AuthenticatedKeyAgreementDomain &d, double timeTotal, bool pc=false)
{
	SecByteBlock priv(d.EphemeralPrivateKeyLength()), pub(d.EphemeralPublicKeyLength());

	clock_t start = clock();
	unsigned int i;
	double timeTaken;
	for (timeTaken=(double)0, i=0; timeTaken < timeTotal; timeTaken = double(clock() - start) / CLOCK_TICKS_PER_SECOND, i++)
		d.GenerateEphemeralKeyPair(GlobalRNG(), priv, pub);

	OutputResultOperations(name, "Key-Pair Generation", pc, i, timeTaken);

	if (!pc && d.GetMaterial().SupportsPrecomputation())
	{
		d.AccessMaterial().Precompute(16);
		BenchMarkKeyGen(name, d, timeTotal, true);
	}
}

void BenchMarkAgreement(const char *name, SimpleKeyAgreementDomain &d, double timeTotal, bool pc=false)
{
	SecByteBlock priv1(d.PrivateKeyLength()), priv2(d.PrivateKeyLength());
	SecByteBlock pub1(d.PublicKeyLength()), pub2(d.PublicKeyLength());
	d.GenerateKeyPair(GlobalRNG(), priv1, pub1);
	d.GenerateKeyPair(GlobalRNG(), priv2, pub2);
	SecByteBlock val(d.AgreedValueLength());

	clock_t start = clock();
	unsigned int i;
	double timeTaken;
	for (timeTaken=(double)0, i=0; timeTaken < timeTotal; timeTaken = double(clock() - start) / CLOCK_TICKS_PER_SECOND, i+=2)
	{
		d.Agree(val, priv1, pub2);
		d.Agree(val, priv2, pub1);
	}

	OutputResultOperations(name, "Key Agreement", pc, i, timeTaken);
}

void BenchMarkAgreement(const char *name, AuthenticatedKeyAgreementDomain &d, double timeTotal, bool pc=false)
{
	SecByteBlock spriv1(d.StaticPrivateKeyLength()), spriv2(d.StaticPrivateKeyLength());
	SecByteBlock epriv1(d.EphemeralPrivateKeyLength()), epriv2(d.EphemeralPrivateKeyLength());
	SecByteBlock spub1(d.StaticPublicKeyLength()), spub2(d.StaticPublicKeyLength());
	SecByteBlock epub1(d.EphemeralPublicKeyLength()), epub2(d.EphemeralPublicKeyLength());
	d.GenerateStaticKeyPair(GlobalRNG(), spriv1, spub1);
	d.GenerateStaticKeyPair(GlobalRNG(), spriv2, spub2);
	d.GenerateEphemeralKeyPair(GlobalRNG(), epriv1, epub1);
	d.GenerateEphemeralKeyPair(GlobalRNG(), epriv2, epub2);
	SecByteBlock val(d.AgreedValueLength());

	clock_t start = clock();
	unsigned int i;
	double timeTaken;
	for (timeTaken=(double)0, i=0; timeTaken < timeTotal; timeTaken = double(clock() - start) / CLOCK_TICKS_PER_SECOND, i+=2)
	{
		d.Agree(val, spriv1, epriv1, spub2, epub2);
		d.Agree(val, spriv2, epriv2, spub1, epub1);
	}

	OutputResultOperations(name, "Key Agreement", pc, i, timeTaken);
}

//VC60 workaround: compiler bug triggered without the extra dummy parameters
template <class SCHEME>
void BenchMarkCrypto(const char *filename, const char *name, double timeTotal, SCHEME *x=NULL)
{
	FileSource f(filename, true, new HexDecoder());
	typename SCHEME::Decryptor priv(f);
	typename SCHEME::Encryptor pub(priv);
	BenchMarkEncryption(name, pub, timeTotal);
	BenchMarkDecryption(name, priv, pub, timeTotal);
}

//VC60 workaround: compiler bug triggered without the extra dummy parameters
template <class SCHEME>
void BenchMarkSignature(const char *filename, const char *name, double timeTotal, SCHEME *x=NULL)
{
	FileSource f(filename, true, new HexDecoder());
	typename SCHEME::Signer priv(f);
	typename SCHEME::Verifier pub(priv);
	BenchMarkSigning(name, priv, timeTotal);
	BenchMarkVerification(name, priv, pub, timeTotal);
}

//VC60 workaround: compiler bug triggered without the extra dummy parameters
template <class D>
void BenchMarkKeyAgreement(const char *filename, const char *name, double timeTotal, D *x=NULL)
{
	FileSource f(filename, true, new HexDecoder());
	D d(f);
	BenchMarkKeyGen(name, d, timeTotal);
	BenchMarkAgreement(name, d, timeTotal);
}

extern double g_hertz;

void BenchmarkAll2(double t, double hertz)
{
	g_hertz = hertz;
#if 0
	cout << "<TABLE border=1><COLGROUP><COL align=left><COL align=right><COL align=right>" << endl;
	cout << "<THEAD><TR><TH>Operation<TH>Milliseconds/Operation" << (g_hertz ? "<TH>Megacycles/Operation" : "") << endl;

	cout << "\n<TBODY style=\"background: yellow\">";
	BenchMarkCrypto<RSAES<OAEP<SHA> > >("TestData/rsa1024.dat", "RSA 1024", t);
	BenchMarkCrypto<LUCES<OAEP<SHA> > >("TestData/luc1024.dat", "LUC 1024", t);
	BenchMarkCrypto<DLIES<> >("TestData/dlie1024.dat", "DLIES 1024", t);
	BenchMarkCrypto<LUC_IES<> >("TestData/lucc512.dat", "LUCELG 512", t);

	cout << "\n<TBODY style=\"background: white\">";
	BenchMarkCrypto<RSAES<OAEP<SHA> > >("TestData/rsa2048.dat", "RSA 2048", t);
	BenchMarkCrypto<LUCES<OAEP<SHA> > >("TestData/luc2048.dat", "LUC 2048", t);
	BenchMarkCrypto<DLIES<> >("TestData/dlie2048.dat", "DLIES 2048", t);
	BenchMarkCrypto<LUC_IES<> >("TestData/lucc1024.dat", "LUCELG 1024", t);

	cout << "\n<TBODY style=\"background: yellow\">";
	BenchMarkSignature<RSASS<PSSR, SHA> >("TestData/rsa1024.dat", "RSA 1024", t);
	BenchMarkSignature<RWSS<PSSR, SHA> >("TestData/rw1024.dat", "RW 1024", t);
	BenchMarkSignature<LUCSS<PSSR, SHA> >("TestData/luc1024.dat", "LUC 1024", t);
	BenchMarkSignature<NR<SHA> >("TestData/nr1024.dat", "NR 1024", t);
	BenchMarkSignature<DSA>("TestData/dsa1024.dat", "DSA 1024", t);
	BenchMarkSignature<LUC_HMP<SHA> >("TestData/lucs512.dat", "LUC-HMP 512", t);
	BenchMarkSignature<ESIGN<SHA> >("TestData/esig1023.dat", "ESIGN 1023", t);
	BenchMarkSignature<ESIGN<SHA> >("TestData/esig1536.dat", "ESIGN 1536", t);

	cout << "\n<TBODY style=\"background: white\">";
	BenchMarkSignature<RSASS<PSSR, SHA> >("TestData/rsa2048.dat", "RSA 2048", t);
	BenchMarkSignature<RWSS<PSSR, SHA> >("TestData/rw2048.dat", "RW 2048", t);
	BenchMarkSignature<LUCSS<PSSR, SHA> >("TestData/luc2048.dat", "LUC 2048", t);
	BenchMarkSignature<NR<SHA> >("TestData/nr2048.dat", "NR 2048", t);
	BenchMarkSignature<LUC_HMP<SHA> >("TestData/lucs1024.dat", "LUC-HMP 1024", t);
	BenchMarkSignature<ESIGN<SHA> >("TestData/esig2046.dat", "ESIGN 2046", t);

	cout << "\n<TBODY style=\"background: yellow\">";
	BenchMarkKeyAgreement<XTR_DH>("TestData/xtrdh171.dat", "XTR-DH 171", t);
	BenchMarkKeyAgreement<XTR_DH>("TestData/xtrdh342.dat", "XTR-DH 342", t);
	BenchMarkKeyAgreement<DH>("TestData/dh1024.dat", "DH 1024", t);
	BenchMarkKeyAgreement<DH>("TestData/dh2048.dat", "DH 2048", t);
	BenchMarkKeyAgreement<LUC_DH>("TestData/lucd512.dat", "LUCDIF 512", t);
	BenchMarkKeyAgreement<LUC_DH>("TestData/lucd1024.dat", "LUCDIF 1024", t);
	BenchMarkKeyAgreement<MQV>("TestData/mqv1024.dat", "MQV 1024", t);
	BenchMarkKeyAgreement<MQV>("TestData/mqv2048.dat", "MQV 2048", t);
#endif
	cout << "\n<TBODY style=\"background: white\">";
	{
		ECIES<ECP>::Decryptor cpriv(GlobalRNG(), ASN1::secp256k1());
		ECIES<ECP>::Encryptor cpub(cpriv);
		ECDSA<ECP, SHA>::Signer spriv(cpriv);
		ECDSA<ECP, SHA>::Verifier spub(spriv);
		ECDH<ECP>::Domain ecdhc(ASN1::secp256k1());
		ECMQV<ECP>::Domain ecmqvc(ASN1::secp256k1());

		BenchMarkEncryption("ECIES over GF(p) 256", cpub, t);
		BenchMarkDecryption("ECIES over GF(p) 256", cpriv, cpub, t);
		BenchMarkSigning("ECDSA over GF(p) 256", spriv, t);
		BenchMarkVerification("ECDSA over GF(p) 256", spriv, spub, t);
		BenchMarkKeyGen("ECDHC over GF(p) 256", ecdhc, t);
		BenchMarkAgreement("ECDHC over GF(p) 256", ecdhc, t);
		BenchMarkKeyGen("ECMQVC over GF(p) 256", ecmqvc, t);
		BenchMarkAgreement("ECMQVC over GF(p) 256", ecmqvc, t);
	}

	cout << "<TBODY style=\"background: yellow\">" << endl;
	{
		ECIES<EC2N>::Decryptor cpriv(GlobalRNG(), ASN1::sect233r1());
		ECIES<EC2N>::Encryptor cpub(cpriv);
		ECDSA<EC2N, SHA>::Signer spriv(cpriv);
		ECDSA<EC2N, SHA>::Verifier spub(spriv);
		ECDH<EC2N>::Domain ecdhc(ASN1::sect233r1());
		ECMQV<EC2N>::Domain ecmqvc(ASN1::sect233r1());

		BenchMarkEncryption("ECIES over GF(2^n) 233", cpub, t);
		BenchMarkDecryption("ECIES over GF(2^n) 233", cpriv, cpub, t);
		BenchMarkSigning("ECDSA over GF(2^n) 233", spriv, t);
		BenchMarkVerification("ECDSA over GF(2^n) 233", spriv, spub, t);
		BenchMarkKeyGen("ECDHC over GF(2^n) 233", ecdhc, t);
		BenchMarkAgreement("ECDHC over GF(2^n) 233", ecdhc, t);
		BenchMarkKeyGen("ECMQVC over GF(2^n) 233", ecmqvc, t);
		BenchMarkAgreement("ECMQVC over GF(2^n) 233", ecmqvc, t);
	}
	cout << "</TABLE>" << endl;
}
