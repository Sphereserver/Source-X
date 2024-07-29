/**
* @file CSFile.h
*/

#ifndef _INC_CSFILE_H
#define _INC_CSFILE_H

#include "CSString.h"

#ifndef _WIN32
	 #include <fcntl.h>
#endif

#ifndef OF_WRITE // happens on Linux?
    // O_* macros are defined in fcntl.h
	#define OF_READ             O_RDONLY
	#define OF_WRITE            O_WRONLY
	#define OF_READWRITE        O_RDWR
	#define OF_SHARE_DENY_NONE	0x00
	#define OF_SHARE_DENY_WRITE	0x00	// not defined in LINUX
	#define OF_CREATE			O_CREAT
#endif //OF_WRITE

// Custom flags
#define OF_NONCRIT			0x40000000	// just a test.
#define OF_TEXT				0x20000000
#define OF_BINARY			0x10000000
#define OF_DEFAULTMODE		0x80000000  // don't cache this script (use CFileText methods instead of CCacheableScriptFile ones)


/**
* @brief Custom cross platform File I/O class, which is similar to and extends the functionalities of MFC CFile classes.
*/
class CSFile
{
protected:
    MT_CMUTEX_DEF;

public:
	static const char * m_sClassName;

	/** @name Constructors, Destructor, Asign operator:
	 */
	///@{
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
    /** @name Error Logging:
    */
    ///@{
    /**
    * @brief Return the last IO error code.
    * @return IO error code.
    */
public:   static int GetLastError();
    /**
    * @brief Notify a file input / output error (win32 only).
    * @param szMessage error to notify.
    */
protected: void _NotifyIOError( lpctstr szMessage ) const;
    ///@}
	/** @name File Management:
	 */
public:
	///@{
	/**
	* @brief Closes the file if is open.
	*/
protected:  virtual void _Close();
public:     virtual void Close();
    /**
    * @brief Open a file in a specified mode.
    * @param ptcName file to open.
    * @param uiMode open mode.
    * @return true if file is open, false otherwise.
    */
protected:  virtual bool _Open( lpctstr ptcFilename = nullptr, uint uiModeFlags = OF_READ|OF_SHARE_DENY_NONE );
public:     virtual bool Open( lpctstr ptcFilename = nullptr, uint uiModeFlags = OF_READ|OF_SHARE_DENY_NONE );
    /**
    * @brief Check if file is open.
    * @return true if file is open, false otherwise.
    */
protected:  virtual bool _IsFileOpen() const;
public:     virtual bool IsFileOpen() const;
	/**
	* @brief Get file name and path (for compatibility with MFC)
	* @return file name and path.
	*/
protected:  lpctstr _GetFilePath() const;
public:     lpctstr GetFilePath() const;
	/**
	* @brief Sets a new file path.
	*
	* If CFile has already have a file path, close it if is opened and open new
	* file.
	* @param pszName new file path.
	* @return true if new file name is setted, false otherwise.
	*/
protected:  virtual bool _SetFilePath( lpctstr pszName );
public:     virtual bool SetFilePath( lpctstr pszName );

	///@}
	/** @name Content Management:
	 */
	///@{
	/**
	* @brief Get the length of the file.
	* @return the length of the file.
	*/
protected: int _GetLength();
public:    int GetLength();
	/**
	* @brief Gets the position indicator of the file.
	* @return The position indicator of the file.
	*/
protected: virtual int _GetPosition() const;
public:    virtual int GetPosition() const;
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
protected:  virtual int _Seek( int iOffset = 0, int iOrigin = SEEK_SET );
public:     virtual int Seek( int iOffset = 0, int iOrigin = SEEK_SET );
	/**
	* @brief Sets the position indicator at the begin of the file.
	*/
protected:  void _SeekToBegin();
public:     void SeekToBegin();
	/**
	* @brief Sets the position indicator at the end of the file.
	* @return The length of the file on success, -1 on error.
	*/
protected:  int _SeekToEnd();
public:     int SeekToEnd();

	/**
	* @brief writes supplied data into file.
	* @param pData data to write.
	* @param dwLength lenght of the data to write.
	* @return true is success, false otherwise.
	*/
protected:  virtual bool _Write(const void* pData, int iLength);
public:	    virtual bool Write (const void* pData, int iLength);
	///@}

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
    * @brief Gets the basename of the file.
    * @return the basename of the file (name withouth paths).
    */
protected:  lpctstr _GetFileTitle() const;
public:     lpctstr GetFileTitle() const;
    /**
    * @brief Gets the file extension, if any.
    * @param pszName file path where get the extension.
    * @return The extension of the file or nullptr if the file has no extension.
    */
    static lpctstr GetFilesExt( lpctstr pszName );
    /**
    * @brief Gets the file extension, if any.
    * @return The extension of the file or nullptr if the file has no extension.
    */
    lpctstr GetFileExt() const;
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
protected: uint _GetFullMode() const;
public:    uint GetFullMode() const;
    /**
    * @brief Get open mode (OF_NONCRIT, OF_TEXT, OF_BINARY, OF_DEFAULTMODE)
    *
    * Get rid of OF_NONCRIT type flags
    * @return mode flags.
    */
protected: uint _GetMode() const;
public:    uint GetMode() const;
    /**
    * @brief Check if file es in binary mode.
    * @return Always true (this method is virtual).
    */
    virtual bool IsBinaryMode() const { return true; }
    /**
    * @brief Check if file is open for write.
    * @return true if file is open for write, false otherwise.
    */
protected: bool _IsWriteMode() const;
public:    bool IsWriteMode() const;
    ///@}

protected:
#ifdef _WIN32
    using file_descriptor_t = HANDLE;
#else
    using file_descriptor_t = int;
#endif
    const file_descriptor_t _kInvalidFD = (file_descriptor_t)-1;

protected:
	CSString _strFileName;	            // File name (with path).
	uint _uiMode;                       // MMSYSTEM may use 32 bit flags.
public:
    file_descriptor_t _fileDescriptor;  // File descriptor (POSIX, int) or HANDLE (Windows, size of a pointer).


// static methods
public:
    static bool FileExists(lpctstr ptcFilePath);

};


#endif // _INC_CSFILE_H
