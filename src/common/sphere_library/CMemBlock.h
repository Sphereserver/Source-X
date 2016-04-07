/**
* @file CMemBlock.h
*/

#pragma once
#ifndef _INC_CMEMBLOCK_H
#define _INC_CMEMBLOCK_H

#include "common.h"
#include "graycom.h"

/**
* @brief Data buffer with no length info.
*/
class CMemBlock
{
public:
	/** @name Constructors, Destructor, Asign operator:
	 */
	///@{
	CMemBlock();
	virtual ~CMemBlock();
private:
	/**
	* @brief No copy on construction allowed.
    */
	CMemBlock(const CMemBlock& copy);
	/**
	* @brief No copy allowed.
    */
	CMemBlock& operator=(const CMemBlock& other);
	///@}
	/** @name Modifiers:
	 */
	///@{
public:
	/**
	* @brief Clear internal data pointer and, if size is valid, alloc mem, updating internal data pointer.
	* @param dwSize size to alloc.
	*/
	void Alloc( size_t dwSize );
protected:
	/**
	* @brief Alloc mem (new byte[*] wrapper). Fails if can not alloc or if size is invalid.
	* @param dwSize size to alloc.
	* @return pointer to the allocated data.
	*/
	byte * AllocBase( size_t dwSize );
public:
	/**
	* @brief Clear internal data pointer if it is not NULL.
	*/
	void Free();
protected:
	/**
	* @brief Sets the internal data pointer. Fails when internal data pointer is not NULL.
	*/
	void MemLink( byte * pData );
	///@}
	/** @name Operations:
	 */
	///@{
public:
	/**
	* @brief Gets the internal data pointer.
	* @return The internal data pointer (can be NULL).
	*/
	byte * GetData() const;
	///@}

private:
	byte * m_pData;	 ///< the actual data bytes of the bitmap.
};

/**
* @brief Buffer implementation.
*/
class CMemLenBlock : public CMemBlock
{
public:
	/** @name Constructors, Destructor, Asign operator:
	 */
	///@{
	CMemLenBlock();
private:
	/**
	* @brief No copy on construction allowed.
    */
	CMemLenBlock(const CMemLenBlock& copy);
	/**
	* @brief No copy allowed.
    */
	CMemLenBlock& operator=(const CMemLenBlock& other);
	///@}
public:
	/** @name Modifiers:
	 */
	///@{
	/**
	* @brief Set the size of the buffer and alloc mem.
	+ @param dwSize new size of the buffer.
    */
	void Alloc( size_t dwSize );
	/**
	* @brief Clears the buffer.
    */
	void Free();
	/**
	* @brief Resizes the buffer, maintaining the current data.
	+ @param dwSizeNew new size of the buffer.
    */
	void Resize( size_t dwSizeNew );
	///@}
	/** @name Operations:
	 */
	///@{
	/**
	* @brief Get the buffer len.
	+ @return Length of the buffer.
    */
	size_t GetDataLength() const;
	///@}

private:
	size_t m_dwLength;  ///< Buffer len.
};


#endif	// _INC_CMEMBLOCK_H
