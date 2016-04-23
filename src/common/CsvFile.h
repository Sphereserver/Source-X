/**
* @file CsvFile.h
*
*/

#pragma once
#ifndef _INC_CSVFILE_H
#define _INC_CSVFILE_H

#include <map>

#include "sphere_library/CSFile.h"
#include "CacheableScriptFile.h"

#define MAX_COLUMNS	64	// maximum number of columns in a file


typedef std::map<std::string, std::string> CSVRowData;

class CSVFile : public CacheableScriptFile
{
private:
	tchar * m_pszColumnTypes[MAX_COLUMNS];
	tchar * m_pszColumnNames[MAX_COLUMNS];
	int m_iColumnCount;
	int m_iCurrentRow;

private:
	virtual bool OpenBase(void *pExtra);

public:
	CSVFile();
	~CSVFile();

private:
	CSVFile(const CSVFile& copy);
	CSVFile& operator=(const CSVFile& other);

public:
	int GetColumnCount() const { return m_iColumnCount; }
	int GetCurrentRow() const { return m_iCurrentRow; }

public:
	int ReadRowContent(tchar ** ppOutput, int row, int columns = MAX_COLUMNS);
	bool ReadRowContent(int row, CSVRowData& target);

	int ReadNextRowContent(tchar ** ppOutput);
	bool ReadNextRowContent(CSVRowData& target);
};

#endif // _INC_CSVFILE_H
