
#include "../sphere/threads.h"
#include "CacheableScriptFile.h"

CacheableScriptFile::CacheableScriptFile()
{
	m_closed = true;
	m_realFile = false;
	m_currentLine = 0;
	m_fileContent = NULL;
}

CacheableScriptFile::~CacheableScriptFile() 
{
	Close();
}

bool CacheableScriptFile::OpenBase(void *pExtra) 
{
	if ( useDefaultFile() ) 
		return CSFileText::OpenBase(pExtra);

	ADDTOCALLSTACK("CacheableScriptFile::OpenBase");

	m_pStream = fopen(GetFilePath(), GetModeStr());
	if ( m_pStream == NULL ) 
		return false;

	m_fileContent = new std::vector<std::string>();
	m_llFile = STDFUNC_FILENO(m_pStream);
	m_closed = false;
	TemporaryString buf;
	size_t nStrLen;
	bool bUTF = false, bFirstLine = true;
	
	while ( !feof(m_pStream) ) 
	{
		buf.setAt(0, '\0');
		fgets(buf, SCRIPT_MAX_LINE_LEN, m_pStream);
		nStrLen = strlen(buf);

		// first line may contain utf marker
		if ( bFirstLine && nStrLen >= 3 &&
			(uchar)(buf[0]) == 0xEF &&
			(uchar)(buf[1]) == 0xBB &&
			(uchar)(buf[2]) == 0xBF )
			bUTF = true;

		std::string strLine((bUTF ? &buf[3]:buf), nStrLen - (bUTF ? 3:0));
		m_fileContent->push_back(strLine);
		bFirstLine = false;
		bUTF = false;
	}

	fclose(m_pStream);
	m_pStream = NULL;
	m_llFile = 0;
	m_currentLine = 0;
	m_realFile = true;

	return true;
}

void CacheableScriptFile::CloseBase() 
{
	if( useDefaultFile() ) 
		CSFileText::CloseBase();
	else 
	{
		ADDTOCALLSTACK("CacheableScriptFile::CloseBase");

		//	clear all data
		if( m_realFile ) 
		{
			if ( m_fileContent != NULL )
			{
				m_fileContent->clear();
				delete m_fileContent;
			}
		}

		m_fileContent = NULL;
		m_currentLine = 0;
		m_closed = true;
	}
}

bool CacheableScriptFile::IsFileOpen() const 
{
	if( useDefaultFile() ) 
		return CSFileText::IsFileOpen();

	ADDTOCALLSTACK("CacheableScriptFile::IsFileOpen");
	return !m_closed;
}

bool CacheableScriptFile::IsEOF() const 
{
	if( useDefaultFile() ) 
		return CSFileText::IsEOF();

	ADDTOCALLSTACK("CacheableScriptFile::IsEOF");
	return ( m_fileContent == NULL || m_currentLine == m_fileContent->size() );
}

tchar * CacheableScriptFile::ReadString(tchar *pBuffer, size_t sizemax) 
{
	if( useDefaultFile() ) 
		return CSFileText::ReadString(pBuffer, sizemax);

	ADDTOCALLSTACK("CacheableScriptFile::ReadString");
	*pBuffer = '\0';

	if ( m_fileContent != NULL && m_currentLine < m_fileContent->size() )
	{
		strcpy(pBuffer, (m_fileContent->at(m_currentLine)).c_str() );
		m_currentLine++;
	}
	else 
		return NULL;

	return pBuffer;
}

void CacheableScriptFile::dupeFrom(CacheableScriptFile *other) 
{
	if( useDefaultFile() ) 
		return;

	m_closed = other->m_closed;
	m_realFile = false;
	m_fileContent = other->m_fileContent;
}

bool CacheableScriptFile::useDefaultFile() const 
{
	if( IsWriteMode() || ( GetFullMode() & OF_DEFAULTMODE )) 
		return true;

	return false;
}

size_t CacheableScriptFile::Seek(size_t offset, int origin)
{
	ADDTOCALLSTACK("CacheableScriptFile::Seek");
	if (useDefaultFile())
		return CSFileText::Seek(offset, origin);

	size_t linenum = offset;

	if (origin != SEEK_SET)
		linenum = 0;	//	do not support not SEEK_SET rotation

	if (linenum <= m_fileContent->size())
	{
		m_currentLine = linenum;
		return linenum;
	}

	return 0;
}

size_t CacheableScriptFile::GetPosition() const
{
	ADDTOCALLSTACK("CacheableScriptFile::GetPosition");
	if (useDefaultFile())
		return CSFileText::GetPosition();

	return m_currentLine;
}
