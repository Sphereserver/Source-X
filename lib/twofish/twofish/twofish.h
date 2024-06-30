/* ex aes.h */

#ifndef _INC_TWOFISH_H_
#define _INC_TWOFISH_H_

/* ---------- See examples at end of this file for typical usage -------- */

/* AES Cipher header file for ANSI C Submissions		// edited for SphereServer
Lawrence E. Bassham III
Computer Security Division
National Institute of Standards and Technology

This sample is to assist implementers developing to the
Cryptographic API Profile for AES Candidate Algorithm Submissions.
Please consult this document as a cross-reference.

ANY CHANGES, WHERE APPROPRIATE, TO INFORMATION PROVIDED IN THIS FILE
MUST BE DOCUMENTED. CHANGES ARE ONLY APPROPRIATE WHERE SPECIFIED WITH
THE STRING "CHANGE POSSIBLE". FUNCTION CALLS AND THEIR PARAMETERS
CANNOT BE CHANGED. STRUCTURES CAN BE ALTERED TO ALLOW IMPLEMENTERS TO
INCLUDE IMPLEMENTATION SPECIFIC INFORMATION.
*/

/*	Defines:
Add any additional defines you need
*/

#define 	DIR_ENCRYPT 	0 		/* Are we encrpyting? */
#define 	DIR_DECRYPT 	1 		/* Are we decrpyting? */
#define 	MODE_ECB 		1 		/* Are we ciphering in ECB mode? */
#define 	MODE_CBC 		2 		/* Are we ciphering in CBC mode? */
#define 	MODE_CFB1 		3 		/* Are we ciphering in 1-bit CFB mode? */

//#define 	TRUE 			1
//#define 	FALSE 			0

#define 	BAD_KEY_DIR 		-1	/* Key direction is invalid (unknown value) */
#define 	BAD_KEY_MAT 		-2	/* Key material not of correct length */
#define 	BAD_KEY_INSTANCE 	-3	/* Key passed is not valid */
#define 	BAD_CIPHER_MODE 	-4 	/* Params struct passed to cipherInit invalid */
#define 	BAD_CIPHER_STATE 	-5 	/* Cipher in wrong state (e.g., not initialized) */

/* CHANGE POSSIBLE: inclusion of algorithm specific defines */
/* TWOFISH specific definitions */
#define		MAX_KEY_SIZE		64	/* # of ASCII chars needed to represent a key */
#define		MAX_IV_SIZE			16	/* # of bytes needed to represent an IV */
#define		BAD_INPUT_LEN		-6	/* inputLen not a multiple of block size */
#define		BAD_PARAMS			-7	/* invalid parameters */
#define		BAD_IV_MAT			-8	/* invalid IV text */
#define		BAD_ENDIAN			-9	/* incorrect endianness define */
#define		BAD_ALIGN32			-10	/* incorrect 32-bit alignment */

#define		BLOCK_SIZE			128	/* number of bits per block */
#define		MAX_ROUNDS			 16	/* max # rounds (for allocating subkey array) */
#define		ROUNDS_128			 16	/* default number of rounds for 128-bit keys*/
#define		ROUNDS_192			 16	/* default number of rounds for 192-bit keys*/
#define		ROUNDS_256			 16	/* default number of rounds for 256-bit keys*/
#define		MAX_KEY_BITS		256	/* max number of bits of key */
#define		MIN_KEY_BITS		128	/* min number of bits of key (zero pad) */
#define		VALID_SIG	 0x48534946	/* initialization signature ('FISH') */
#define		MCT_OUTER			400	/* MCT outer loop */
#define		MCT_INNER		  10000	/* MCT inner loop */
#define		REENTRANT			  1	/* nonzero forces reentrant code (slightly slower) */

#define		INPUT_WHITEN		0	/* subkey array indices */
#define		OUTPUT_WHITEN		( INPUT_WHITEN + BLOCK_SIZE/32)
#define		ROUND_SUBKEYS		(OUTPUT_WHITEN + BLOCK_SIZE/32)	/* use 2 * (# rounds) */
#define		TOTAL_SUBKEYS		(ROUND_SUBKEYS + 2*MAX_ROUNDS)

/* Typedefs:
Typedef'ed data storage elements. Add any algorithm specific
parameters at the bottom of the structs as appropriate.
*/
// Note: In this file were intentionally substituted BYTE with unsigned char and DWORD with unsigned int,
//		 to avoid typedef redefinition in other sphere headers.
typedef unsigned int			fullSbox[4][256];

/* The structure for key information */
typedef struct keyInstance
{
	unsigned char direction;					/* Key used for encrypting or decrypting? */
#if ALIGN32
	unsigned char dummyAlign[3];				/* keep 32-bit alignment */
#endif
	int keyLen;					/* Length of the key */
	char keyMaterial[MAX_KEY_SIZE + 4];/* Raw key data in ASCII */

	/* Twofish-specific parameters: */
	unsigned int keySig;					/* set to VALID_SIG by makeKey() */
	int numRounds;				/* number of rounds in cipher */
	unsigned int key32[MAX_KEY_BITS / 32];	/* actual key bits, in dwords */
	unsigned int sboxKeys[MAX_KEY_BITS / 64];/* key bits used for S-boxes */
	unsigned int subKeys[TOTAL_SUBKEYS];	/* round subkeys, input/output whitening bits */
#if REENTRANT
	fullSbox sBox8x32;				/* fully expanded S-box */
#if defined(COMPILE_KEY) && defined(USE_ASM)
#undef	VALID_SIG
#define	VALID_SIG	 0x504D4F43		/* 'COMP':  C is compiled with -DCOMPILE_KEY */
	unsigned int cSig1;					/* set after first "compile" (zero at "init") */
	void* encryptFuncPtr;			/* ptr to asm encrypt function */
	void* decryptFuncPtr;			/* ptr to asm decrypt function */
	unsigned int codeSize;					/* size of compiledCode */
	unsigned int cSig2;					/* set after first "compile" */
	unsigned char compiledCode[5000];		/* make room for the code itself */
#endif

#endif
} keyInstance;

/* The structure for cipher information */
typedef struct cipherInstance
{
	unsigned char mode;						/* MODE_ECB, MODE_CBC, or MODE_CFB1 */
#if ALIGN32
	unsigned char dummyAlign[3];				/* keep 32-bit alignment */
#endif
	unsigned char IV[MAX_IV_SIZE];			/* CFB1 iv bytes  (CBC uses iv32) */

	/* Twofish-specific parameters: */
	unsigned int cipherSig;				/* set to VALID_SIG by cipherInit() */
	unsigned int iv32[BLOCK_SIZE / 32];		/* CBC IV bytes arranged as dwords */
}cipherInstance;

/* Function protoypes */
void makeKey(keyInstance* key, unsigned char direction, int keyLen/*, char* keyMaterial*/);

void cipherInit(cipherInstance* cipher, unsigned char mode, char* IV);

int blockEncrypt(cipherInstance* cipher, keyInstance* key, unsigned char* input, int inputLen, unsigned char* outBuffer);

int blockDecrypt(cipherInstance* cipher, keyInstance* key, unsigned char* input, int inputLen, unsigned char* outBuffer);

void reKey(keyInstance* key);	/* do key schedule using modified key.keyDwords */

/* API to check table usage, for use in ECB_TBL KAT */
#define		TAB_DISABLE			0
#define		TAB_ENABLE			1
#define		TAB_RESET			2
#define		TAB_QUERY			3
#define		TAB_MIN_QUERY		50
int TableOp(int op);

#ifndef CONST // warning C4005: 'CONST' : macro redefinition \microsoft visual studio\vc98\include\windef.h(138) : see previous definition of 'CONST'
	#define		CONST				/* helpful C++ syntax sugar, NOP for ANSI C */
#endif

#if BLOCK_SIZE == 128			/* optimize block copies */
	#define		Copy1(d,s,N)	((unsigned int *)(d))[N] = ((unsigned int *)(s))[N]
	#define		BlockCopy(d,s)	{ Copy1(d,s,0);Copy1(d,s,1);Copy1(d,s,2);Copy1(d,s,3); }
#else
	#define		BlockCopy(d,s)	{ memcpy(d,s,BLOCK_SIZE/8); }
#endif


/*
*	Macros used internally for encryption/decryption
*/

// Change this to compile on a BigEndian machine
#if !defined(Q_OS_MAC)
	#define IsLittleEndian 1	// Renamed it since it conflicted with windows.h
#endif
#define ALIGN32 1

#if IsLittleEndian
	#define		Bswap(x)			(x)		/* NOP for little-endian machines */
	#define		ADDR_XOR			0		/* NOP for little-endian machines */
#else
	#define		Bswap(x)			((ROR(x,8) & 0xFF00FF00) | (ROL(x,8) & 0x00FF00FF))
	#define		ADDR_XOR			3		/* convert byte address in dword */
#endif

/* use intrinsic rotate if possible */
#define ROL( x, y ) ( ( (x) << ((y)&0x1f) ) | ( (x) >> ( 32 - ((y)&0x1f) ) ) )
#define ROR( x, y ) ( ( (x) >> ((y)&0x1f) ) | ( (x) << ( 32 - ((y)&0x1f) ) ) )

#if defined(_MSC_VER)
	#include	<stdlib.h>					/* get prototypes for rotation functions */
	#undef	ROL
	#undef	ROR
	#pragma intrinsic(_lrotl,_lrotr)		/* use intrinsic compiler rotations */
	#define	ROL(x,n)	_lrotl(x,n)			
	#define	ROR(x,n)	_lrotr(x,n)
#endif

/*	Macros for extracting bytes from dwords (correct for endianness) */
#define	_b(x,N)	(((byte *)&x)[((N) & 3) ^ ADDR_XOR]) /* pick bytes out of a dword */

#define		b0(x)			_b(x,0)		/* extract LSB of DWORD */
#define		b1(x)			_b(x,1)
#define		b2(x)			_b(x,2)
#define		b3(x)			_b(x,3)		/* extract MSB of DWORD */

#endif
