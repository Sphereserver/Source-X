/***************************************************************************
TWOFISH2.C	--	C API calls for TWOFISH AES submission		// edited for SphereServer
//	we are using as base the unoptimized version, since the optimized one caused a segmentation fault in the
//	reKey function when compiling with any level of optimization, it worked only without optimization flags

Submitters:
Bruce Schneier, Counterpane Systems
Doug Whiting,	Hi/fn
John Kelsey,	Counterpane Systems
Chris Hall,		Counterpane Systems
David Wagner,	UC Berkeley

Code Author:		Doug Whiting,	Hi/fn

Version  1.00		April 1998

Copyright 1998, Hi/fn and Counterpane Systems.  All rights reserved.

Notes:
*	Tab size is set to 4 characters in this file

***************************************************************************/
#include "twofish.h"
#include "twofish_aux.h"

#include <memory.h>
#include <inttypes.h>

/*
+*****************************************************************************
*			Constants/Macros/Tables
-****************************************************************************/

typedef uint_least8_t	BYTE;
typedef uint_least32_t	DWORD;
#define	CONST			//const

/* number of rounds for various key sizes:  128, 192, 256 */
CONST int numRounds[4] =
{
	0, ROUNDS_128, ROUNDS_192, ROUNDS_256
};

//#define	DebugDump(x,s,R,XOR,doRot,showT,needBswap)
//#define	DebugDumpKey(key)


/*
+*****************************************************************************
*
* Function Name:	f32
*
* Function:			Run four bytes through keyed S-boxes and apply MDS matrix
*
* Arguments:		x			=	input to f function
*					k32			=	pointer to key dwords
*					keyLen		=	total key length (k32 --> keyLey/2 bits)
*
* Return:			The output of the keyed permutation applied to x.
*
* Notes:
*	This function is a keyed 32-bit permutation.  It is the major building
*	block for the Twofish round function, including the four keyed 8x8
*	permutations and the 4x4 MDS matrix multiply.  This function is used
*	both for generating round subkeys and within the round function on the
*	block being encrypted.
*
*	This version is fairly slow and pedagogical, although a smartcard would
*	probably perform the operation exactly this way in firmware.   For
*	ultimate performance, the entire operation can be completed with four
*	lookups into four 256x32-bit tables, with three dword xors.
*
*	The MDS matrix is defined in TABLE.H.  To multiply by Mij, just use the
*	macro Mij(x).
*
-****************************************************************************/
dword f32(dword x, CONST dword* k32, int keyLen)
{
	byte b[4];

	/* Run each byte thru 8x8 S-boxes, xoring with key byte at each stage. */
	/* Note that each byte goes through a different combination of S-boxes.*/
#ifdef __GNUC__
	#pragma GCC diagnostic push
	#pragma GCC diagnostic ignored "-Wstrict-aliasing"	// disabling the warning for the type punning
#endif
	*((dword *)b) = Bswap(x);	/* make b[0] = LSB, b[3] = MSB */
#ifdef __GNUC__
	#pragma GCC diagnostic pop
#endif

	switch (((keyLen + 63) / 64) & 3)
	{
	case 0:
		/* 256 bits of key */
		b[0] = p8(04)[b[0]] ^ b0(k32[3]);
		b[1] = p8(14)[b[1]] ^ b1(k32[3]);
		b[2] = p8(24)[b[2]] ^ b2(k32[3]);
		b[3] = p8(34)[b[3]] ^ b3(k32[3]);
		/* fall thru, having pre-processed b[0]..b[3] with k32[3] */
	case 3:
		/* 192 bits of key */
		b[0] = p8(03)[b[0]] ^ b0(k32[2]);
		b[1] = p8(13)[b[1]] ^ b1(k32[2]);
		b[2] = p8(23)[b[2]] ^ b2(k32[2]);
		b[3] = p8(33)[b[3]] ^ b3(k32[2]);
		/* fall thru, having pre-processed b[0]..b[3] with k32[2] */
	case 2:
		/* 128 bits of key */
		b[0] = p8(00)[p8(01)[p8(02)[b[0]] ^ b0(k32[1])] ^ b0(k32[0])];
		b[1] = p8(10)[p8(11)[p8(12)[b[1]] ^ b1(k32[1])] ^ b1(k32[0])];
		b[2] = p8(20)[p8(21)[p8(22)[b[2]] ^ b2(k32[1])] ^ b2(k32[0])];
		b[3] = p8(30)[p8(31)[p8(32)[b[3]] ^ b3(k32[1])] ^ b3(k32[0])];
	}

	/* Now perform the MDS matrix multiply inline. */
	return	((M00(b[0]) ^ M01(b[1]) ^ M02(b[2]) ^ M03(b[3]))) ^ ((M10(b[0]) ^ M11(b[1]) ^ M12(b[2]) ^ M13(b[3])) << 8) ^ ((M20(b[0]) ^ M21(b[1]) ^ M22(b[2]) ^ M23(b[3])) << 16) ^ ((M30(b[0]) ^ M31(b[1]) ^ M32(b[2]) ^ M33(b[3])) << 24);
}


/*
+*****************************************************************************
*
* Function Name:	RS_MDS_encode
*
* Function:			Use (12,8) Reed-Solomon code over GF(256) to produce
*					a key S-box dword from two key material dwords.
*
* Arguments:		k0	=	1st dword
*					k1	=	2nd dword
*
* Return:			Remainder polynomial generated using RS code
*
* Notes:
*	Since this computation is done only once per reKey per 64 bits of key,
*	the performance impact of this routine is imperceptible. The RS code
*	chosen has "simple" coefficients to allow smartcard/hardware implementation
*	without lookup tables.
*
-****************************************************************************/
dword RS_MDS_Encode(dword k0, dword k1)
{
	int i, j;
	dword r;

	for (i = r = 0; i < 2; i++)
	{
		r ^= (i) ? k0 : k1;			/* merge in 32 more key bits */
		for (j = 0; j < 4; j++)			/* shift one byte at a time */
			RS_rem(r);
	}
	return r;
}



/*
+*****************************************************************************
*
* Function Name:	ReverseRoundSubkeys
*
* Function:			Reverse order of round subkeys to switch between encrypt/decrypt
*
* Arguments:		key		=	ptr to keyInstance to be reversed
*					newDir	=	new direction value
*
* Return:			None.
*
* Notes:
*	This optimization allows both blockEncrypt and blockDecrypt to use the same
*	"fallthru" switch statement based on the number of rounds.
*	Note that key->numRounds must be even and >= 2 here.
*
-****************************************************************************/
void ReverseRoundSubkeys(keyInstance* key, byte newDir)
{
	dword t0, t1;
	dword * r0 = key->subKeys + ROUND_SUBKEYS;
	dword * r1 = r0 + 2 * key->numRounds - 2;

	for (; r0 < r1; r0 += 2, r1 -= 2)
	{
		t0 = r0[0];			/* swap the order */
		t1 = r0[1];
		r0[0] = r1[0];		/* but keep relative order within pairs */
		r0[1] = r1[1];
		r1[0] = t0;
		r1[1] = t1;
	}

	key->direction = newDir;
}

/*
+*****************************************************************************
*
* Function Name:	reKey
*
* Function:			Initialize the Twofish key schedule from key32
*
* Arguments:		key			=	ptr to keyInstance to be initialized
*
* Notes:
*	Here we precompute all the round subkeys, although that is not actually
*	required.  For example, on a smartcard, the round subkeys can
*	be generated on-the-fly	using f32()
*
-****************************************************************************/
void reKey(keyInstance* key)
{
	int keyLen = key->keyLen;
	int subkeyCnt = 8 + 2 * key->numRounds;
	unsigned int k32e[4], k32o[4];
	unsigned int A = 0, B = 0;

	int k64Cnt = ( keyLen + 63 ) / 64;

	for ( int i = 0; i < k64Cnt; ++i )
	{
		k32e[i] = key->key32[2 * i];
		k32o[i] = key->key32[2 * i + 1];
		key->sboxKeys[k64Cnt - 1 - i] = RS_MDS_Encode( k32e[i], k32o[i] );
	}

	for ( int i = 0; i < subkeyCnt / 2; ++i )
	{
		A = f32( i * 0x02020202u, k32e, keyLen );
		B = f32( i * 0x02020202u + 0x01010101u, k32o, keyLen );
		B = ROL( B, 8 );
		key->subKeys[2 * i] = A + B;
		key->subKeys[2 * i + 1] = ROL( A + 2 * B, 9 );
	}
}
/*
+*****************************************************************************
*
* Function Name:	makeKey
*
* Function:			Initialize the Twofish key schedule
*
* Arguments:		key			=	ptr to keyInstance to be initialized
*					direction	=	DIR_ENCRYPT or DIR_DECRYPT
*					keyLen		=	# bits of key text at *keyMaterial
*					keyMaterial	=	ptr to hex ASCII chars representing key bits
*
* Notes:	This parses the key bits from keyMaterial.  Zeroes out unused key bits
*
-****************************************************************************/
void makeKey(keyInstance* key, byte direction, int keyLen/*, CONST char* keyMaterial */)
{
	key->keySig = 0x48534946;
	key->direction = direction;
	key->keyLen = ( keyLen + 63 ) & ~63;
	key->numRounds = numRounds[( keyLen - 1 ) / 64];

	for ( int i = 0; i < 8; i++ )
		key->key32[i] = 0;

	key->keyMaterial[64] = 0;
}


/*
+*****************************************************************************
*
* Function Name:	cipherInit
*
* Function:			Initialize the Twofish cipher in a given mode
*
* Arguments:		cipher		=	ptr to cipherInstance to be initialized
*					mode		=	MODE_ECB, MODE_CBC, or MODE_CFB1
*					IV			=	ptr to hex ASCII test representing IV bytes
*
-****************************************************************************/
void cipherInit(cipherInstance* cipher, byte mode, CONST char* IV)
{
	cipher->cipherSig = 0x48534946;

	if ( ( mode != 1 ) && ( IV ) )
	{
		for ( int i = 0; i < 4; i++ )
		( (unsigned int*)cipher->IV )[i] = Bswap( cipher->iv32[i] );
	}

	cipher->mode = mode;
}

/*
+*****************************************************************************
*
* Function Name:	blockEncrypt
*
* Function:			Encrypt block(s) of data using Twofish
*
* Arguments:		cipher		=	ptr to already initialized cipherInstance
*					key			=	ptr to already initialized keyInstance
*					input		=	ptr to data blocks to be encrypted
*					inputLen	=	# bits to encrypt (multiple of blockSize)
*					outBuffer	=	ptr to where to put encrypted blocks
*
* Return:			# bits ciphered (>= 0)
*					else error code (e.g., BAD_CIPHER_STATE, BAD_KEY_MATERIAL)
*
* Notes: The only supported block size for ECB/CBC modes is BLOCK_SIZE bits.
*		 If inputLen is not a multiple of BLOCK_SIZE bits in those modes,
*		 an error BAD_INPUT_LEN is returned.  In CFB1 mode, all block
*		 sizes can be supported.
*
-****************************************************************************/
int blockEncrypt(cipherInstance* cipher, keyInstance* key, CONST byte* input, int inputLen, byte* outBuffer)
{
	int rounds = key->numRounds;
	unsigned int x[4];
	unsigned int t0, t1, tmp;
	unsigned char bit = 0, ctBit = 0, carry = 0;

	if ( cipher->mode == 3 )
	{
		cipher->mode = 1;
		for ( int n = 0; n < inputLen; n++ )
		{
			blockEncrypt( cipher, key, cipher->IV, 128, (unsigned char*)x );
			bit = 0x80 >> ( n & 7 );
			ctBit = ( input[n / 8] & bit ) ^ ( ( ( (unsigned char*)x )[0] & 0x80 ) >> ( n & 7 ) );
			outBuffer[n / 8] = ( outBuffer[n / 8] & ~bit ) | ctBit;
			carry = ctBit >> ( 7 - ( n & 7 ) );
			for ( int i = 15; i >= 0; i-- )
			{
				bit = cipher->IV[i] >> 7;
				cipher->IV[i] = ( cipher->IV[i] << 1 ) ^ carry;
				carry = bit;
			}
		}
		cipher->mode = 3;
	}

	for ( int n = 0; n < inputLen; n += 128, input += 16, outBuffer += 16 )
	{
		for ( int i = 0; i < 4; i++ )
		{
			x[i] = Bswap( ( (unsigned int*)input )[i] ) ^ key->subKeys[i];
			if ( cipher->mode == 2 )
				x[i] ^= cipher->iv32[i];
		}

		for ( int r = 0; r < rounds; r++ )
		{
			t0 = f32( x[0], key->sboxKeys, key->keyLen );
			t1 = f32( ROL( x[1], 8 ), key->sboxKeys, key->keyLen );

			x[3] = ROL( x[3], 1 );
			x[2] ^= t0 + t1 + key->subKeys[8 + 2 * r];
			x[3] ^= t0 + 2 * t1 + key->subKeys[8 + 2 * r + 1];
			x[2] = ROR( x[2], 1 );

			if ( r < rounds - 1 )
			{
				tmp = x[0];
				x[0] = x[2];
				x[2] = tmp;
				tmp = x[1];
				x[1] = x[3];
				x[3] = tmp;
			}
		}

		for ( int i = 0; i < 4; i++ )
		{
			( (unsigned int*)outBuffer )[i] = Bswap( x[i] ^ key->subKeys[4 + i] );
			if ( cipher->mode == 2 )
				cipher->iv32[i] = Bswap( ( (unsigned int*)outBuffer )[i] );
		}
	}
	
	return inputLen;
}

/*
+*****************************************************************************
*
* Function Name:	blockDecrypt
*
* Function:			Decrypt block(s) of data using Twofish
*
* Arguments:		cipher		=	ptr to already initialized cipherInstance
*					key			=	ptr to already initialized keyInstance
*					input		=	ptr to data blocks to be decrypted
*					inputLen	=	# bits to encrypt (multiple of blockSize)
*					outBuffer	=	ptr to where to put decrypted blocks
*
* Return:			# bits ciphered (>= 0)
*					else error code (e.g., BAD_CIPHER_STATE, BAD_KEY_MATERIAL)
*
* Notes: The only supported block size for ECB/CBC modes is BLOCK_SIZE bits.
*		 If inputLen is not a multiple of BLOCK_SIZE bits in those modes,
*		 an error BAD_INPUT_LEN is returned.  In CFB1 mode, all block
*		 sizes can be supported.
*
-****************************************************************************/
int blockDecrypt(cipherInstance* cipher, keyInstance* key, CONST byte* input, int inputLen, byte* outBuffer)
{
	int   i,n,r;					/* loop counters */
	dword x[BLOCK_SIZE/32];			/* block being encrypted */
	dword t0,t1;					/* temp variables */
	int	  rounds=key->numRounds;	/* number of rounds */
	byte  bit,ctBit,carry;			/* temps for CFB */

	if (cipher->mode == MODE_CFB1)
	{	/* use blockEncrypt here to handle CFB, one block at a time */
		cipher->mode = MODE_ECB;	/* do encryption in ECB */
		for (n=0;n<inputLen;n++)
		{
			blockEncrypt(cipher,key,cipher->IV,BLOCK_SIZE,(byte *)x);
			bit	  = 0x80 >> (n & 7);
			ctBit = input[n/8] & bit;
			outBuffer[n/8] = (outBuffer[n/8] & ~ bit) |
							 (ctBit ^ ((((byte *) x)[0] & 0x80) >> (n&7)));
			carry = ctBit >> (7 - (n&7));
			for (i=BLOCK_SIZE/8-1;i>=0;i--)
			{
				bit = cipher->IV[i] >> 7;	/* save next "carry" from shift */
				cipher->IV[i] = (cipher->IV[i] << 1) ^ carry;
				carry = bit;
			}
		}
		cipher->mode = MODE_CFB1;	/* restore mode for next time */
		return inputLen;
	}

	/* here for ECB, CBC modes */
	for (n=0;n<inputLen;n+=BLOCK_SIZE,input+=BLOCK_SIZE/8,outBuffer+=BLOCK_SIZE/8)
	{
		//DebugDump(input,"\n",rounds+1,0,0,0,1);

		for (i=0;i<BLOCK_SIZE/32;i++)	/* copy in the block, add whitening */
			x[i]=Bswap(((dword *)input)[i]) ^ key->subKeys[OUTPUT_WHITEN+i];

		for (r=rounds-1;r>=0;r--)			/* main Twofish decryption loop */
		{
			t0	 = f32(    x[0]   ,key->sboxKeys,key->keyLen);
			t1	 = f32(ROL(x[1],8),key->sboxKeys,key->keyLen);

			//DebugDump(x,"",r+1,2*(r&1),0,1,0);/* make format compatible with optimized code */
			x[2] = ROL(x[2],1);
			x[2]^= t0 +   t1 + key->subKeys[ROUND_SUBKEYS+2*r  ]; /* PHT, round keys */
			x[3]^= t0 + 2*t1 + key->subKeys[ROUND_SUBKEYS+2*r+1];
			x[3] = ROR(x[3],1);

			if (r)									/* unswap, except for last round */
			{
				t0   = x[0]; x[0]= x[2]; x[2] = t0;	
				t1   = x[1]; x[1]= x[3]; x[3] = t1;
			}
		}
		//DebugDump(x,"",0,0,0,0,0);/* make final output match encrypt initial output */

		for (i=0;i<BLOCK_SIZE/32;i++)	/* copy out, with whitening */
		{
			x[i] ^= key->subKeys[INPUT_WHITEN+i];
			if (cipher->mode == MODE_CBC)
			{
				x[i] ^= Bswap(cipher->iv32[i]);
				cipher->iv32[i] = ((dword *)input)[i];
			}
			((dword *)outBuffer)[i] = Bswap(x[i]);
		}
		//DebugDump(outBuffer,"",-1,0,0,0,1);
	}

	return inputLen;
}
