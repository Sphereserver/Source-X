/**
* @file CUOVersionBlock.h
*
*/

#ifndef _INC_CUOVERSIONBLOCK_H
#define _INC_CUOVERSIONBLOCK_H

#include "../../common/common.h"
#include "uofiles_enums.h"

#define VERDATA_MAKE_INDEX( f, i )	((f+1)<< 26 | (i))


// All these structures must be byte packed.
#if defined(_WIN32) && defined(_MSC_VER)
	// Microsoft dependant pragma
	#pragma pack(1)
	#define PACK_NEEDED
#else
	// GCC based compiler you can add:
	#define PACK_NEEDED __attribute__ ((packed))
#endif


/**
* Skew list. (verdata.mul)
*/
struct CUOVersionBlock
{
    // First 4 bytes of this file = the qty of records.
private:
    dword m_file;  // file type id. VERFILE_TYPE (ex.tiledata = 0x1E)
    dword m_block; // tile number. ( items = itemid + 0x200 )
public:
    dword m_filepos; // pos in this file to find the patch block.
    dword m_length;

    word  m_wVal3;  // stuff that would have been in CUOIndexRec
    word  m_wVal4;
public:
    /** @name File ops:
     */
    ///@{
    VERFILE_TYPE GetFileIndex() const;
    void SetFile(VERFILE_TYPE dwFile);
    ///@}
    /** @name Block ops:
     */
    ///@{
    // a single sortable index.
    dword GetIndex() const;
    dword GetBlockIndex() const;
    // This stuff is for GrayPatch
    void SetBlock(dword dwBlock);
    ///@}

} PACK_NEEDED;


// Turn off structure packing.
#if defined(_WIN32) && defined(_MSC_VER)
	#pragma pack()
#else
	#undef PACK_NEEDED
#endif

#endif //_INC_CUOVERSIONBLOCK_H
