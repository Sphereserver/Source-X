/**
* @file net_datatypes.h
* @brief Handling little/big endian conversions.
*/

#ifndef _INC_NET_DATATYPES_H
#define _INC_NET_DATATYPES_H

#include "../common/common.h"


// All these structures must be byte packed.

#if defined(_WIN32) && defined(_MSC_VER)
	// Microsoft dependant pragma
	#pragma pack(1)
	#define PACK_NEEDED
#else
	// GCC and Clang based compiler you can add:
	#define PACK_NEEDED __attribute__ ((packed))
#endif


// Pack/unpack in network order.

#define	nchar	nword			// a UNICODE text char on the network.

struct nword
{
    // Deal with the stupid Little Endian / Big Endian crap just once.
    // On little endian machines this code would do nothing.
    word m_val;
    operator word () const;
    nword& operator = (word val);

#define PACKWORD(p,w)	(p)[0]=HIBYTE(w); (p)[1]=LOBYTE(w)
#define UNPACKWORD(p)	MAKEWORD((p)[1],(p)[0])	// low,high

} PACK_NEEDED;


struct ndword
{
    dword m_val;
    operator dword () const;
    ndword& operator = (dword val);

#define PACKDWORD(p,d)	(p)[0]=((d)>>24)&0xFF; (p)[1]=((d)>>16)&0xFF; (p)[2]=HIBYTE(d); (p)[3]=LOBYTE(d)
#define UNPACKDWORD(p)	MAKEDWORD( MAKEWORD((p)[3],(p)[2]), MAKEWORD((p)[1],(p)[0]))

} PACK_NEEDED;


// Turn off structure packing.
#if defined(_WIN32) && defined(_MSC_VER)
    #pragma pack()
#endif


int CvtSystemToNUNICODE(nchar* pOut, int iSizeOutChars, lpctstr pInp, int iSizeInBytes);
int CvtNUNICODEToSystem(tchar* pOut, int iSizeOutBytes, const nchar* pInp, int iSizeInChars);


#endif // _INC_NET_DATATYPES_H
