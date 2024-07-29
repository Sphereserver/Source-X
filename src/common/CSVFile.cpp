
#include "../sphere/threads.h"
#include "CException.h"
#include "CExpression.h"
#include "CSVFile.h"
#include "CScript.h"
#include "common.h"

CSVFile::CSVFile()
{
	_iColumnCount = 0;
	_iCurrentRow = 0;
	_pszColumnTypes[0] = nullptr;
	_pszColumnNames[0] = nullptr;
}

CSVFile::~CSVFile()
{
	for (int i = 0; _pszColumnTypes[i] != nullptr; ++i)
		delete[] _pszColumnTypes[i];

	for (int i = 0; _pszColumnNames[i] != nullptr; ++i)
		delete[] _pszColumnNames[i];
}

int CSVFile::GetColumnCount() const
{
    ADDTOCALLSTACK("CSVFile::GetColumnCount");
    MT_SHARED_LOCK_RETURN(_iColumnCount);
}
int CSVFile::GetCurrentRow() const
{
    ADDTOCALLSTACK("CSVFile::GetCurrentRow");
    MT_SHARED_LOCK_RETURN(_iCurrentRow);
}

bool CSVFile::_Open(lpctstr ptcFilename, uint uiModeFlags)
{
	ADDTOCALLSTACK("CSVFile::_Open");
	if ( !CCacheableScriptFile::_Open(ptcFilename, uiModeFlags) )
		return false;

	_iCurrentRow = 0;

	// remove all empty lines so that we just have data rows stored
	for (std::vector<std::string>::iterator i = _fileContent->begin(); i != _fileContent->end(); )
	{
		lpctstr pszLine = i->c_str();
		GETNONWHITESPACE(pszLine);
		if ( *pszLine == '\0' )
			i = _fileContent->erase(i);
		else
			++i;
	}

	// find the types and names of the columns
	tchar * ppColumnTypes[MAX_COLUMNS];
	tchar * ppColumnNames[MAX_COLUMNS];

	// first row tells us how many columns there are
	_iColumnCount = _ReadRowContent(ppColumnTypes, 0);
	if ( _iColumnCount <= 0 )
	{
		_iColumnCount = 0;
		_Close();
		return false;
	}

	// second row lets us validate the column count
	if ( _ReadRowContent(ppColumnNames, 1) != _iColumnCount )
	{
		_iColumnCount = 0;
		_Close();
		return false;
	}

	// copy the names
	for (int i = 0; i < _iColumnCount; ++i)
	{
		_pszColumnTypes[i] = new tchar[128];
		Str_CopyLimitNull(_pszColumnTypes[i], ppColumnTypes[i], 128);

		_pszColumnNames[i] = new tchar[128];
		Str_CopyLimitNull(_pszColumnNames[i], ppColumnNames[i], 128);
	}

	_pszColumnTypes[_iColumnCount] = nullptr;
	_pszColumnNames[_iColumnCount] = nullptr;
	return true;
}
bool CSVFile::Open(lpctstr ptcFilename, uint uiModeFlags)
{
    ADDTOCALLSTACK("CSVFile::Open");
    MT_UNIQUE_LOCK_RETURN(CSVFile::_Open(ptcFilename, uiModeFlags));
}

int CSVFile::_ReadRowContent(tchar ** ppOutput, int rowIndex, int columns)
{
	ADDTOCALLSTACK("CSVFile::_ReadRowContent");
	ASSERT(columns > 0 && columns <= MAX_COLUMNS);
	if ( _GetPosition() != rowIndex )
		_Seek(rowIndex, SEEK_SET);

	tchar * pszLine = Str_GetTemp();
	if ( _ReadString(pszLine, THREAD_STRING_LENGTH) == nullptr )
		return 0;

	return Str_ParseCmds(pszLine, ppOutput, columns, "\t");
}

int CSVFile::_ReadNextRowContent(tchar ** ppOutput)
{
	ADDTOCALLSTACK("CSVFile::_ReadNextRowContent");
	++_iCurrentRow;
	return _ReadRowContent(ppOutput, _iCurrentRow);
}

bool CSVFile::_ReadRowContent(int rowIndex, CSVRowData& target)
{
	ADDTOCALLSTACK("CSVFile::_ReadRowContent");
	// get row data
	tchar * ppRowContent[MAX_COLUMNS];
	int columns = _ReadRowContent(ppRowContent, rowIndex);
	if ( columns != _iColumnCount )
		return false;

	// copy to target
	target.clear();
	for (int i = 0; i < columns; ++i)
		target[_pszColumnNames[i]] = ppRowContent[i];

	return ! target.empty();
}

bool CSVFile::_ReadNextRowContent(CSVRowData& target)
{
	ADDTOCALLSTACK("CSVFile::_ReadNextRowContent");
	++_iCurrentRow;
	return _ReadRowContent(_iCurrentRow, target);
}

bool CSVFile::ReadNextRowContent(CSVRowData& target)
{
    ADDTOCALLSTACK("CSVFile::ReadNextRowContent");
    MT_UNIQUE_LOCK_RETURN(CSVFile::_ReadNextRowContent(target));
}

