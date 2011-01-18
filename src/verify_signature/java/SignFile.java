import java.security.*;
import java.security.spec.*;
import java.io.*;
import java.security.interfaces.*;
import java.security.cert.*;
import javax.xml.transform.stream.*;
import javax.xml.transform.dom.*;
import javax.xml.transform.*;
import org.w3c.dom.*;
import javax.xml.parsers.*;

public class SignFile
{
	private KeyPairGenerator keyGen; // Key pair generator for RSA
	public PrivateKey privateKey; // Private Key Class
	public PublicKey publicKey; // Public Key Class
	public KeyPair keypair; // KeyPair Class
	private Signature sign; // Signature, used to sign the data
	/* Instantiates the key paths and signature algorithm. */
	public SignFile()
	{
		try
		{
			// Get the instance of Signature Engine.
			sign = Signature.getInstance("SHA1withRSA");
		}
		catch (NoSuchAlgorithmException	nsa)
		{
			System.out.println("" + nsa.getMessage());
		}
	}

	public boolean loadPrivateKey(byte[] privatekey)
	{
		KeyFactory kf;
		try
		{
			kf = KeyFactory.getInstance("RSA");
			EncodedKeySpec eks = new PKCS8EncodedKeySpec(privatekey);
			privateKey = kf.generatePrivate(eks);
		}
		catch ( NoSuchAlgorithmException e )
		{
			System.err.println("Error creating RSA key factory: "+e.getMessage());
			return false;
		}
		catch ( InvalidKeySpecException e )
		{
			System.err.println("Error decoding private key: "+e.getMessage());
			return false;
		}
		return true;
	}

	public boolean signDataFile(byte[] data, String outputfn)
	{
		FileOutputStream fos = null;
		try
		{
			sign.initSign(privateKey);
			fos = new FileOutputStream(outputfn);
			sign.update(data);
			fos.write( sign.sign() );
			fos.close();
		}
		catch ( FileNotFoundException e )
		{
			System.err.println("Error opening file: "+e.getMessage());
			return false;
		}
		catch ( InvalidKeyException e )
		{
			System.err.println("Invalid Key Exception: "+e.getMessage());
			return false;
		}
		catch ( SignatureException e )
		{
			System.err.println("Signature exception: "+e.getMessage());
			return false;
		}
		catch ( IOException e )
		{
			System.err.println("IOException: "+e.getMessage());
			return false;
		}	
		return true;
	}

	public static void main(String args[])
	{
		SignFile sm = new SignFile();

		if( args.length != 3 )
		{
			System.out.println("\nUsage:    SignFile <data_fn> <privatekey_fn> <signature_fn>");
			return;
		}

		String datafn = args[0];
		String privatekeyfn = args[1];
		String signaturefn = args[2];

		byte[] data	= sm.readBytesFromFile(datafn);
		byte[] privatekey = sm.readBytesFromFile(privatekeyfn);

		if( !sm.loadPrivateKey(privatekey) )
		{
			System.err.println("Could not load private key!");
			System.exit(1);
		}

		if( !sm.signDataFile(data,signaturefn) )
		{
			System.err.println("Could not generate signature!");
			System.exit(1);
		}
	}

	public byte[] readBytesFromFile(String fileName)
	{
		try
		{
			File file = new File(fileName);
			InputStream is = new FileInputStream(file);

			// Get the size of the file
			long length = file.length();

			// You cannot create an array	using a	long type.
			// It needs to be an int type.
			// Before converting to an int type, check
			// to ensure that file is not larger than Integer.MAX_VALUE.
			if (length > Integer.MAX_VALUE) {
				// File	is too large
			}

			// Create the byte array to hold the data
			byte[] bytes = new byte[ (int) length];

			// Read in the bytes
			int offset = 0;
			int numRead = 0;
			while (offset < bytes.length
				&& (numRead = is.read(bytes, offset, bytes.length - offset)) >= 0)
			{
				offset += numRead;
			}

			// Ensure all the bytes have been read in
			if (offset < bytes.length) {
				throw new IOException("Key File Error: Could not completely read file " + file.getName());
			}

			// Close the input stream and return bytes
			is.close();
			return bytes;
		}
		catch(IOException ioe)
		{
			System.out.println("Exception occured while writing file"+ioe.getMessage());
		}
		byte[] bytes = new byte[1];
		return bytes; 
	}
}
