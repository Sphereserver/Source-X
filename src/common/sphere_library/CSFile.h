/**
* @file CSFile.h
*/

#pragma once
#ifndef _INC_CSFILE_H
#define _INC_CSFILE_H

#include <cstdio>
#include "CSString.h"

#ifndef _WIN32
	 #include <fcntl.h>
#endif

#ifndef OF_WRITE
	#define OF_READ             O_RDONLY
	#define OF_WRITE            O_WRONLY
	#define OF_READWRITE        O_RDWR
	#define OF_SHARE_DENY_NONE	0x00
	#define OF_SHARE_DENY_WRITE	0x00	// not defined in LINUX
	#define OF_CREATE			O_CREAT
#endif //OF_WRITE

#define OF_NONCRIT			0x40000000	// just a test.
#define OF_TEXT				0x20000000
#define OF_BINARY			0x10000000
#define OF_DEFAULTMODE		0x80000000

#ifndef HFILE_ERROR
	#define HFILE_ERROR	-1
	#define HFILE int
#endif // HFILE_ERROR

#ifdef _WIN32
	#define INVALID_HANDLE ((HANDLE) -1)
	#define OSFILE_TYPE		HANDLE
	#define NOFILE_HANDLE	INVALID_HANDLE
#else
	#define OSFILE_TYPE		HFILE
	#define NOFILE_HANDLE	HFILE_ERROR
#endif


class CSError;

/**
* @brief Class that dupes the MFC functionality we need
*/
class CFile
{
public:
	static const char * m_sClassName;

	/** @name Constructors, Destructor, Asign operator:
	 */
	///@{
	CFile();
	virtual ~CFile();
private:
	/**
	* @brief No copy on construction allowed.
	*/
	CFile(const CFile& copy);
	/**
	* @brief No copy allowed.
	*/
	CFile& operator=(const CFile& other);
	///@}
public:
	/** @name File Management:
	 */
	///@{
	/**
	* @brief Closes the file if is open.
	*/
	virtual void Close();
	/**
	* @brief Get file name and path (for compatibility with MFC)
	* @return file name and path.
	*/
	const CSString & GetFilePath() const;
	/**
	* @brief Sets a new file path.
	*
	* If CFile has already have a file path, close it if is opened and open new
	* file.
	* @param pszName new file path.
	* @return true if new file name is setted, false otherwise.
	*/
	virtual bool SetFilePath( lpctstr pszName );
	/**
	* @brief Gets the basename of the file.
	* @return the basename of the file (name withouth paths).
	*/
	lpctstr GetFileTitle() const;
#ifdef _WIN32
	/**
	* @brief Notify a file input / output error (win32 only).
	* @param szMessage error to notify.
	*/
	void NotifyIOError( lpctstr szMessage ) const;
#endif
	/**
	* @brief Open a file in a specified mode.
	* @param pszName file to open.
	* @param uMode open mode.
	* @param e TODOC.
	* @return true if file is open, false otherwise.
	*/
	virtual bool Open( lpctstr pszName = NULL, uint uMode = OF_READ | OF_SHARE_DENY_NONE, CSError * e = NULL );
	///@}
	/** @name Content Management:
	 */
	///@{
	/**
	* @brief Get the length of the file.
	* @return the length of the file.
	*/
	size_t GetLength();
	/**
	* @brief Gets the position indicator of the file.
	* @return The position indicator of the file.
	*/
	virtual size_t GetPosition() const;
	/**
	* @brief Reads data from the file.
	* @param pData buffer where store the readed data.
	* @param stLength count of bytes to read.
	* @return count of bytes readed.
	*/
	virtual size_t Read( void * pData, size_t stLength ) const;
	/**
	* @brief Set the position indicator.
	* @param stOffset position to set.
	* @param stOrigin origin (current position or init of the file).
	* @return position where the position indicator is set on success, -1 on error.
	*/
	virtual size_t Seek( size_t stOffset = 0, int iOrigin = SEEK_SET );
	/**
	* @brief Sets the position indicator at the begin of the file.
	*/
	void SeekToBegin();
	/**
	* @brief Sets the position indicator at the end of the file.
	* @return The length of the file on success, -1 on error.
	*/
	size_t SeekToEnd();
	/**
	* @brief writes supplied data into file.
	* @param pData data to write.
	* @param dwLength lenght of the data to write.
	* @return true is success, false otherwise.
	*/
	virtual bool Write( const void * pData, size_t stLength ) const;
	///@}

public:
	llong m_llFile;			/// File descriptor (POSIX, int) or HANDLE (Windows, size of a pointer).

protected:
	CSString m_strFileName;	// File name (with path).
	uint m_uMode;   // MMSYSTEM may use 32 bit flags.
};


/**
* Try to be compatible with MFC CFile class.
*/
class CSFile : public CFile
{
public:
	static const char * m_sClassName;

	/** @name Constructors, Destructor, Asign operator:
	 */
	///@{
public:
	CSFile();
	virtual ~CSFile();
private:
	/**
	* @brief No copy on construction allowed.
	*/
	CSFile(const CSFile& copy);
	/**
	* @brief No copy allowed.
	*/
	CSFile& operator=(const CSFile& other);
	///@}
public:
	/** @name File Management:
	 */
	///@{
	/**
	* @brief Closes the file if is open.
	*/
	virtual void Close();
private:
	/**
	* @brief Closes the file if is open.
	*/
	virtual void CloseBase();
public:
	/**
	* @brief Return the last IO error code.
	* @return IO error code.
	*/
	static int GetLastError();
	/**
	* @brief Check if file is open.
	* @return true if file is open, false otherwise.
	*/
	virtual bool IsFileOpen() const;
	/**
	* @brief Open a file in a specified mode.
	* @param pszName file to open.
	* @param uMode open mode.
	* @param pExtra TODOC.
	* @return true if file is open, false otherwise.
	*/
	virtual bool Open( lpctstr pszName = NULL, uint uMode = OF_READ | OF_SHARE_DENY_NONE, void * pExtra = NULL );
private:
	/**
	* @brief Open file with CFile method.
	* @param pExtra unused.
	* @return true if file is open, false otherwise.
	*/
	virtual bool OpenBase( void * pExtra );
	///@}
public:
	/** @name File name operations:
	 */
	///@{
	/**
	* @brief Gets the basename of the file.
	* @param pszPath file path.
	* @return the basename of the file (name withouth paths).
	*/
	static lpctstr GetFilesTitle( lpctstr pszPath );
	/**
	* @brief Gets the file extension, if any.
	* @return The extension of the file or NULL if the file has no extension.
	*/
	lpctstr GetFileExt() const;
	/**
	* @brief Gets the file extension, if any.
	* @param pszName file path where get the extension.
	* @return The extension of the file or NULL if the file has no extension.
	*/
	static lpctstr GetFilesExt( lpctstr pszName );
	/**
	* @brief Merges path and filename, adding slashes if needed.
	* @param pszBase path.
	* @param pszName filename.
	* @return merged path.
	*/
	static CSString GetMergedFileName( lpctstr pszBase, lpctstr pszName );
	///@}
	/** @name Mode operations:
	 */
	///@{
	/**
	* @brief Get open mode (full).
	* @return full mode flags.
	*/
	uint GetFullMode() const;
	/**
	 * @brief Get open mode (OF_NONCRIT, OF_TEXT, OF_BINARY, OF_DEFAULTMODE)
	 *
	 * Get rid of OF_NONCRIT type flags
	 * @return mode flags.
	*/
	uint GetMode() const;
	/**
	* @brief Check if file es in binary mode.
	* @return Always true (this method is virtual).
	*/
	virtual bool IsBinaryMode() const;
	/**
	* @brief Check if file is open for write.
	* @return true if file is open for write, false otherwise.
	*/
	bool IsWriteMode() const;
	///@}
};


/**
* @brief Text files. Try to be compatible with MFC CFile class.
*/
class CSFileText : public CSFile
{
public:
	static const char *m_sClassName;

	/** @name Constructors, Destructor, Asign operator:
	 */
	///@{
public:
	CSFileText();
	virtual ~CSFileText();
private:
	/**
	* @brief No copy on construction allowed.
	*/
	CSFileText(const CSFileText& copy);
	/**
	* @brief No copy allowed.
	*/
	CSFileText& operator=(const CSFileText& other);
	///@}
	/** @name File management:
	 */
	///@{
public:
	/**
	* @brief Check if file is open.
	* @return true if is open, false otherwise.
	*/
	bool IsFileOpen() const;
protected:
	virtual bool OpenBase( void * pExtra );
	virtual void CloseBase();
	///@}
	/** @name Content management:
	 */
	///@{
public:
	/**
	* @brief Write changes to disk.
	*/
	void Flush() const;
	/**
	* @brief Check if EOF is reached.
	* @return true if EOF is reached, false otherwise.
	*/
	bool IsEOF() const;
	/**
	* @brief print in file a string with arguments (printf like).
	* @param pFormat string in "printf like" format.
	* @param ... argument list.
	* @return total chars of the output.
	*/
	size_t _cdecl Printf( lpctstr pFormat, ... ) __printfargs(2,3);
	/**
	* @brief Reads data from the file.
	* @param pBuffer buffer where store the readed data.
	* @param sizemax count of bytes to read.
	* @return count of bytes readed.
	*/
	size_t Read( void * pBuffer, size_t sizemax ) const;
	/**
	* @brief Reads from a file a line (up to sizemax - 1 characters).
	* @param pBuffer buffer where store the readed data.
	* @param sizemax count of characters to read.
	* @return the str readed if success, NULL on errors.
	*/
	tchar * ReadString( tchar * pBuffer, size_t sizemax ) const;
	/**
	* @brief print in file a string with arguments (printf like).
	* @param pFormat string in "printf like" format.
	* @param args argument list.
	* @return total chars of the output.
	*/
	size_t VPrintf( lpctstr pFormat, va_list args );
	/**
	* @brief writes supplied data into file.
	* @param pData data to write.
	* @param iLen lenght of the data to write.
	* @return true is success, false otherwise.
	*/
#ifndef _WIN32
	bool Write( const void * pData, size_t stLen ) const;
#else
	bool Write( const void * pData, size_t stLen );
#endif
	/**
	* @brief write string into file.
	* @return true is success, false otherwise.
	*/
	bool WriteString( lpctstr pStr );
	///@}
	/** @name Mode operations:
	 */
	///@{
protected:
	/**
	* @brief Get open mode in string format.
	*
	* Formats:
	* - "rb"
	* - "w"
	* - "wb"
	* - "a+b"
	* @return string that describes the open mode.
	*/
	lpctstr GetModeStr() const;
public:
	/**
	* @brief Check if file is open in binary mode.
	* @return false always.
	*/
	bool IsBinaryMode() const;
	///@}
public:
	FILE * m_pStream;		// The current open script type file.
protected:
#ifdef _WIN32
	bool	bNoBuffer;		// TODOC.
#endif
};

#endif // _INC_CSFILE_H
