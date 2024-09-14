/**
* @file CSFileText.h
*/

#ifndef _INC_CSFILETEXT_H
#define _INC_CSFILETEXT_H

#include "CSFile.h"

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

    /**
    * @brief No copy on construction allowed.
    */
    CSFileText(const CSFileText& copy) = delete;
    /**
    * @brief No copy allowed.
    */
    CSFileText& operator=(const CSFileText& other) = delete;
    ///@}
    /** @name File management:
    */
    ///@{
public:
    /**
    * @brief Check if file is open.
    * @return true if is open, false otherwise.
    */
protected:  virtual bool _IsFileOpen() const override;
public:     virtual bool IsFileOpen() const override;
    /**
    * @brief Open a file in a specified mode.
    * @param ptcName file to open.
    * @param uiMode open mode.
    * @return true if file is open, false otherwise.
    */
protected:  virtual bool _Open(lpctstr ptcFilename = nullptr, uint uiModeFlags = OF_READ|OF_SHARE_DENY_NONE) override;
public:     virtual bool Open(lpctstr ptcFilename = nullptr, uint uiModeFlags = OF_READ|OF_SHARE_DENY_NONE) override;
    /**
    * @brief Closes the file if is open.
    */
protected:  virtual void _Close() override;
public:     virtual void Close() override;
    ///@}
    /** @name Content management:
    */
    ///@{
public:
    /**
    * @brief Set the position indicator.
    * @param iOffset position to set.
    * @param iOrigin origin (current position or init of the file).
    * @return position where the position indicator is set on success, -1 on error.
    */
protected:  virtual int _Seek( int iOffset = 0, int iOrigin = SEEK_SET ) override;
public:     virtual int Seek( int iOffset = 0, int iOrigin = SEEK_SET ) override;
    /**
    * @brief Write changes to disk.
    */
protected:  void _Flush() const;
public:     void Flush() const;
    /**
    * @brief Check if EOF is reached.
    * @return true if EOF is reached, false otherwise.
    */
protected:  virtual bool _IsEOF() const;
public:     virtual bool IsEOF() const;
    /**
    * @brief print in file a string with arguments (printf like).
    * @param pFormat string in "printf like" format.
    * @param ... argument list.
    * @return total chars of the output.
    */
protected:  int _cdecl _Printf( lpctstr pFormat, ... ) __printfargs(2,3);
public:     int _cdecl Printf(lpctstr pFormat, ...) __printfargs(2, 3);
    /**
    * @brief Reads data from the file.
    * @param pBuffer buffer where store the readed data.
    * @param sizemax count of bytes to read.
    * @return count of bytes readed.
    */
    virtual int Read( void * pBuffer, int sizemax ) const override;
    /**
    * @brief Reads from a file a line (up to sizemax - 1 characters).
    * @param pBuffer buffer where store the readed data.
    * @param sizemax count of characters to read.
    * @return the str readed if success, nullptr on errors.
    */
protected:  virtual tchar * _ReadString( tchar * pBuffer, int sizemax );
public:     virtual tchar * ReadString( tchar * pBuffer, int sizemax );
    /**
    * @brief print in file a string with arguments (printf like).
    * @param pFormat string in "printf like" format.
    * @param args argument list.
    * @return total chars of the output.
    */
protected:  int _VPrintf( lpctstr pFormat, va_list args );
public:     int VPrintf(lpctstr pFormat, va_list args);
    /**
    * @brief writes supplied data into file.
    * @param pData data to write.
    * @param iLen lenght of the data to write.
    * @return true is success, false otherwise.
    */
protected:  virtual bool _Write( const void * pData, int iLen ) override;
public:     virtual bool Write(const void* pData, int iLen) override;
    /**
    * @brief write string into file.
    * @return true is success, false otherwise.
    */
protected:  bool _WriteString(lpctstr pStr);
public:     bool WriteString(lpctstr pStr);
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
    lpctstr _GetModeStr() const;
public:
    /**
    * @brief Check if file is open in binary mode.
    * @return false always.
    */
    virtual bool IsBinaryMode() const override { return false; };
    ///@}
public:
    FILE * _pStream;		// The current open script type file.
#ifdef _WIN32
protected:
    bool _fNoBuffer;		// TODOC.
#endif
};

#endif // _INC_CSFILETEXT_H

