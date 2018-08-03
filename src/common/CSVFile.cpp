
#include "../sphere/threads.h"
#include "CException.h"
#include "CExpression.h"
#include "CSVFile.h"
#include "CScript.h"
#include "common.h"

CSVFile::CSVFile()
{
	m_iColumnCount = 0;
	m_iCurrentRow = 0;
	m_pszColumnTypes[0] = nullptr;
	m_pszColumnNames[0] = nullptr;
}

CSVFile::~CSVFile()
{
	for (int i = 0; m_pszColumnTypes[i] != nullptr; ++i)
		delete[] m_pszColumnTypes[i];

	for (int i = 0; m_pszColumnNames[i] != nullptr; ++i)
		delete[] m_pszColumnNames[i];
}

bool CSVFile::OpenBase()
{
	ADDTOCALLSTACK("CSVFile::OpenBase");
	if ( !PhysicalScriptFile::OpenBase() )
		return false;

	m_iCurrentRow = 0;

	// remove all empty lines so that we just have data rows stored
	for (std::vector<std::string>::iterator i = m_fileContent.begin(); i != m_fileContent.end(); )
	{
		lpctstr pszLine = i->c_str();
		GETNONWHITESPACE(pszLine);
		if ( *pszLine == '\0' )
			i = m_fileContent.erase(i);
		else
			++i;
	}

	// find the types and names of the columns
	tchar * ppColumnTypes[MAX_COLUMNS];
	tchar * ppColumnNames[MAX_COLUMNS];

	// first row tells us how many columns there are
	m_iColumnCount = ReadRowContent(ppColumnTypes, 0);
	if ( m_iColumnCount <= 0 )
	{
		m_iColumnCount = 0;
		Close();
		return false;
	}
	
	// second row lets us validate the column count
	if ( ReadRowContent(ppColumnNames, 1) != m_iColumnCount )
	{
		m_iColumnCount = 0;
		Close();
		return false;
	}

	// copy the names
	for (int i = 0; i < m_iColumnCount; ++i)
	{
		m_pszColumnTypes[i] = new tchar[128];
		strcpy(m_pszColumnTypes[i], ppColumnTypes[i]);

		m_pszColumnNames[i] = new tchar[128];
		strcpy(m_pszColumnNames[i], ppColumnNames[i]);
	}

	m_pszColumnTypes[m_iColumnCount] = nullptr;
	m_pszColumnNames[m_iColumnCount] = nullptr;
	return true;
}

int CSVFile::ReadRowContent(tchar ** ppOutput, int rowIndex, int columns)
{
	ADDTOCALLSTACK("CSVFile::ReadRowContent");
	ASSERT(columns > 0 && columns <= MAX_COLUMNS);
	if ( (int)GetPosition() != rowIndex )
		Seek(rowIndex, SEEK_SET);

	tchar * pszLine = Str_GetTemp();
	if ( ReadString(pszLine, THREAD_STRING_LENGTH) == nullptr )
		return 0;

	return Str_ParseCmds(pszLine, ppOutput, columns, "\t");
}

int CSVFile::ReadNextRowContent(tchar ** ppOutput)
{
	ADDTOCALLSTACK("CSVFile::ReadNextRowContent");
	++m_iCurrentRow;
	return ReadRowContent(ppOutput, m_iCurrentRow);
}

bool CSVFile::ReadRowContent(int rowIndex, CSVRowData& target)
{
	ADDTOCALLSTACK("CSVFile::ReadRowContent");
	// get row data
	tchar * ppRowContent[MAX_COLUMNS];
	int columns = ReadRowContent(ppRowContent, rowIndex);
	if ( columns != m_iColumnCount )
		return false;

	// copy to target
	target.clear();
	for (int i = 0; i < columns; ++i)
		target[m_pszColumnNames[i]] = ppRowContent[i];

	return ! target.empty();
}

bool CSVFile::ReadNextRowContent(CSVRowData& target)
{
	ADDTOCALLSTACK("CSVFile::ReadNextRowContent");
	++m_iCurrentRow;
	return ReadRowContent(m_iCurrentRow, target);
}
