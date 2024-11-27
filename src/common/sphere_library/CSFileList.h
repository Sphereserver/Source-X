/**
* @file CSFileList.h
*/

#ifndef _INC_CSFILELIST_H
#define _INC_CSFILELIST_H

#include "CSAssoc.h"


/**
* @brief List of CGStrings with methods to read
*/
class CSFileList : public CSStringList
{
public:
	static const char *m_sClassName;
	/**
	* @brief Get update time and size of a file.
	* @param pszFilePath file to check.
	* @param dwDateChange update time stored here.
	* @param dwSize size stored here.
	* @return false if can not read info from the file, true if info is setted.
	*/
	static bool ReadFileInfo( lpctstr pszFilePath, time_t & dwDateChange, dword & dwSize );
	/**
	* @brief Read a dir content and store inside the instance.
	*
	* Content with name that start with '.' are not listed.
	* @param pszFilePath dir to read.
	* @param bShowError Show debug info if can not open the dir.
	* @return -1 on error, otherwise the number of elements listed.
	*/
	int ReadDir( lpctstr pszFilePath, bool bShowError = true );

public:
	CSFileList();
    virtual ~CSFileList();

    /**
    * @brief No copy on construction allowed.
    */
	CSFileList(const CSFileList& copy) = delete;
	/**
    * @brief No copy allowed.
    */
	CSFileList& operator=(const CSFileList& other) = delete;
};

#endif	// _INC_CSFILELIST_H
