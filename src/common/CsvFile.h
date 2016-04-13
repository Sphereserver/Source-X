/**
* @file CsvFile.h
*
*/

#pragma once
#ifndef _INC_CSVFILE_H
#define _INC_CSVFILE_H

#include <map>

#include "./sphere_library/CFile.h"
#include "CacheableScriptFile.h"

#define MAX_COLUMNS	64	// maximum number of columns in a file


typedef std::map<std::string, std::string> CSVRowData;

class CSVFile : public CacheableScriptFile
{
private:
	tchar * m_pszColumnTypes[MAX_COLUMNS];
	tchar * m_pszColumnNames[MAX_COLUMNS];
	size_t m_iColumnCount;
	size_t m_iCurrentRow;

private:
	virtual bool OpenBase(void *pExtra);

public:
	CSVFile();
	~CSVFile();

private:
	CSVFile(const CSVFile& copy);
	CSVFile& operator=(const CSVFile& other);

public:
	size_t GetColumnCount() const { return m_iColumnCount; }
	size_t GetCurrentRow() const { return m_iCurrentRow; }

public:
	size_t ReadRowContent(tchar ** ppOutput, size_t row, size_t columns = MAX_COLUMNS);
	bool ReadRowContent(size_t row, CSVRowData& target);

	size_t ReadNextRowContent(tchar ** ppOutput);
	bool ReadNextRowContent(CSVRowData& target);
};

#endif // _INC_CSVFILE_H
