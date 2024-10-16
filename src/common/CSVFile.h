/**
* @file CSVFile.h
*
*/

#ifndef _INC_CSVFILE_H
#define _INC_CSVFILE_H

#include <map>
#include "CCacheableScriptFile.h"


typedef std::map<std::string, std::string> CSVRowData;

class CSVFile : public CCacheableScriptFile
{
    static constexpr uint kuiMaxColumns = 64; // maximum number of columns in a file
    tchar * _pszColumnTypes[kuiMaxColumns];
    tchar * _pszColumnNames[kuiMaxColumns];
	int _iColumnCount;
	int _iCurrentRow;

private:    virtual bool _Open(lpctstr ptcFilename = nullptr, uint uiModeFlags = OF_READ|OF_SHARE_DENY_NONE) override;
public:     virtual bool Open(lpctstr ptcFilename = nullptr, uint uiModeFlags = OF_READ|OF_SHARE_DENY_NONE) override;

public:
	CSVFile();
	~CSVFile();

	CSVFile(const CSVFile& copy) = delete;
	CSVFile& operator=(const CSVFile& other) = delete;

private:    int _GetColumnCount() const { return _iColumnCount; }
public:     int GetColumnCount() const;
private:    int _GetCurrentRow() const { return _iCurrentRow; }
public:     int GetCurrentRow() const;

private:
    int _ReadRowContent(tchar ** ppOutput, int row, int columns = kuiMaxColumns);
	bool _ReadRowContent(int row, CSVRowData& target);

	int _ReadNextRowContent(tchar ** ppOutput);
	bool _ReadNextRowContent(CSVRowData& target);

public:
    bool ReadNextRowContent(CSVRowData& target);
};

#endif // _INC_CSVFILE_H
