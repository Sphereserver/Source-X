/**
* @file CSFile.h
*/

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
    /**
    * @brief Return the last IO error code.
    * @return IO error code.
    */
    static int GetLastError();
	/**
	* @brief Notify a file input / output error (win32 only).
	* @param szMessage error to notify.
	*/
	void NotifyIOError( lpctstr szMessage ) const;
	/**
	* @brief Open a file in a specified mode.
	* @param pszName file to open.
	* @param uiMode open mode.
	* @return true if file is open, false otherwise.
	*/
	virtual bool Open( lpctstr pszName, uint uiMode = OF_READ | OF_SHARE_DENY_NONE );
	///@}
	/** @name Content Management:
	 */
	///@{
	/**
	* @brief Get the length of the file.
	* @return the length of the file.
	*/
	int GetLength();
	/**
	* @brief Gets the position indicator of the file.
	* @return The position indicator of the file.
	*/
	virtual int GetPosition() const;
	/**
	* @brief Reads data from the file.
	* @param pData buffer where store the readed data.
	* @param stLength count of bytes to read.
	* @return count of bytes readed.
	*/
	virtual int Read( void * pData, int iLength ) const;
	/**
	* @brief Set the position indicator.
	* @param stOffset position to set.
	* @param stOrigin origin (current position or init of the file).
	* @return position where the position indicator is set on success, -1 on error.
	*/
	virtual int Seek( int iOffset = 0, int iOrigin = SEEK_SET );
	/**
	* @brief Sets the position indicator at the begin of the file.
	*/
	void SeekToBegin();
	/**
	* @brief Sets the position indicator at the end of the file.
	* @return The length of the file on success, -1 on error.
	*/
    int SeekToEnd();
	/**
	* @brief writes supplied data into file.
	* @param pData data to write.
	* @param dwLength lenght of the data to write.
	* @return true is success, false otherwise.
	*/
	virtual bool Write( const void * pData, int iLength ) const;
	///@}

public:
	llong m_llFile;			/// File descriptor (POSIX, int) or HANDLE (Windows, size of a pointer).

protected:
	CSString m_strFileName;	// File name (with path).
	uint m_uiMode;   // MMSYSTEM may use 32 bit flags.
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
	* @brief Check if file is open.
	* @return true if file is open, false otherwise.
	*/
	virtual bool IsFileOpen() const;
	/**
	* @brief Open a file in a specified mode.
	* @param pszName file to open.
	* @param uiMode open mode.
	* @return true if file is open, false otherwise.
	*/
	virtual bool Open( lpctstr pszName, uint uiMode = OF_READ | OF_SHARE_DENY_NONE );
private:
	/**
	* @brief Open file with CFile method.
	* @return true if file is open, false otherwise.
	*/
	virtual bool OpenBase();
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

#endif // _INC_CSFILE_H
