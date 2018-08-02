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
    virtual bool IsFileOpen() const;
protected:
    virtual bool OpenBase();
    virtual void CloseBase();
    ///@}
    /** @name Content management:
    */
    ///@{
public:
    /**
    * @brief Set the position indicator.
    * @param stOffset position to set.
    * @param stOrigin origin (current position or init of the file).
    * @return position where the position indicator is set on success, -1 on error.
    */
    virtual int Seek( int iOffset = 0, int iOrigin = SEEK_SET );
    /**
    * @brief Write changes to disk.
    */
    virtual void Flush() const;
    /**
    * @brief Check if EOF is reached.
    * @return true if EOF is reached, false otherwise.
    */
    virtual bool IsEOF() const;
    /**
    * @brief print in file a string with arguments (printf like).
    * @param pFormat string in "printf like" format.
    * @param ... argument list.
    * @return total chars of the output.
    */
    int _cdecl Printf( lpctstr pFormat, ... ) __printfargs(2,3);
    /**
    * @brief Reads data from the file.
    * @param pBuffer buffer where store the readed data.
    * @param sizemax count of bytes to read.
    * @return count of bytes readed.
    */
    virtual int Read( void * pBuffer, int sizemax ) const;
    /**
    * @brief Reads from a file a line (up to sizemax - 1 characters).
    * @param pBuffer buffer where store the readed data.
    * @param sizemax count of characters to read.
    * @return the str readed if success, NULL on errors.
    */
    virtual tchar * ReadString( tchar * pBuffer, int sizemax ) const;
    /**
    * @brief print in file a string with arguments (printf like).
    * @param pFormat string in "printf like" format.
    * @param args argument list.
    * @return total chars of the output.
    */
    int VPrintf( lpctstr pFormat, va_list args );
    /**
    * @brief writes supplied data into file.
    * @param pData data to write.
    * @param iLen lenght of the data to write.
    * @return true is success, false otherwise.
    */
#ifndef _WIN32
    virtual bool Write( const void * pData, int iLen ) const;
#else
    virtual bool Write( const void * pData, int iLen );
#endif
    /**
    * @brief write string into file.
    * @return true is success, false otherwise.
    */
    virtual bool WriteString( lpctstr pStr );
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
    virtual bool IsBinaryMode() const;
    ///@}
public:
    FILE * m_pStream;		// The current open script type file.
protected:
#ifdef _WIN32
    bool _fNoBuffer;		// TODOC.
#endif
};

#endif // _INC_CSFILETEXT_H

