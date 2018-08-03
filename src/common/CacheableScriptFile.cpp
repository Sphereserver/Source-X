
#include "../sphere/threads.h"
#include "CacheableScriptFile.h"

#ifdef _WIN32
    #include <io.h> // for _get_osfhandle (used by STDFUNC_FILENO)
#endif


CacheableScriptFile::CacheableScriptFile()
{
	m_closed = true;
	m_realFile = false;
	m_currentLine = 0;
}

CacheableScriptFile::~CacheableScriptFile() 
{
	Close();
}

bool CacheableScriptFile::OpenBase() 
{
    ADDTOCALLSTACK("CacheableScriptFile::OpenBase");

	if ( useDefaultFile() ) 
		return CSFileText::OpenBase();

	m_pStream = fopen(GetFilePath(), GetModeStr());
	if ( m_pStream == nullptr ) 
		return false;

	m_fileContent.clear();
	m_llFile = STDFUNC_FILENO(m_pStream);
	m_closed = false;
	TemporaryString buf;
	size_t nStrLen;
	bool fUTF = false, fFirstLine = true;
	
	while ( !feof(m_pStream) ) 
	{
		buf.setAt(0, '\0');
		fgets(buf, SCRIPT_MAX_LINE_LEN, m_pStream);
		nStrLen = strlen(buf);
        if (nStrLen > INT_MAX)
            nStrLen = INT_MAX;

		// first line may contain utf marker
		if ( fFirstLine && nStrLen >= 3 &&
			(uchar)(buf[0]) == 0xEF &&
			(uchar)(buf[1]) == 0xBB &&
			(uchar)(buf[2]) == 0xBF )
			fUTF = true;

		m_fileContent.emplace_back( (fUTF ? &buf[3]:buf), nStrLen - (fUTF ? 3:0) );
		fFirstLine = false;
		fUTF = false;
	}

	fclose(m_pStream);
	m_pStream = nullptr;
	m_llFile = 0;
	m_currentLine = 0;
	m_realFile = true;

	return true;
}

void CacheableScriptFile::CloseBase() 
{
    ADDTOCALLSTACK("CacheableScriptFile::CloseBase");
	if ( useDefaultFile() )
		CSFileText::CloseBase();
	else 
	{
		//	clear all data
		if ( m_realFile ) 
		{
			if ( !m_fileContent.empty() )
			{
				m_fileContent.clear();
			}
		}

		m_currentLine = 0;
		m_closed = true;
	}
}

bool CacheableScriptFile::IsFileOpen() const 
{
    ADDTOCALLSTACK("CacheableScriptFile::IsFileOpen");

	if ( useDefaultFile() ) 
		return CSFileText::IsFileOpen();

	return !m_closed;
}

bool CacheableScriptFile::IsEOF() const 
{
	if( useDefaultFile() ) 
		return CSFileText::IsEOF();

	ADDTOCALLSTACK("CacheableScriptFile::IsEOF");
	return ( m_fileContent.empty() || ((uint)m_currentLine == m_fileContent.size()) );
}

tchar * CacheableScriptFile::ReadString(tchar *pBuffer, int sizemax) 
{
    ADDTOCALLSTACK("CacheableScriptFile::ReadString");

	if ( useDefaultFile() ) 
		return CSFileText::ReadString(pBuffer, sizemax);

	*pBuffer = '\0';

	if ( !m_fileContent.empty() && ((uint)m_currentLine < m_fileContent.size()) )
	{
		strcpy(pBuffer, m_fileContent[m_currentLine].c_str() );
		++m_currentLine;
	}
	else
    {
		return nullptr;
    }

	return pBuffer;
}

void CacheableScriptFile::dupeFrom(CacheableScriptFile *other) 
{
	if ( useDefaultFile() ) 
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

int CacheableScriptFile::Seek(int offset, int origin)
{
	ADDTOCALLSTACK("CacheableScriptFile::Seek");
	if (useDefaultFile())
		return CSFileText::Seek(offset, origin);

	int linenum = offset;

	if (origin != SEEK_SET)
		linenum = 0;	//	do not support not SEEK_SET rotation

	if ((uint)linenum <= m_fileContent.size())
	{
		m_currentLine = linenum;
		return linenum;
	}

	return 0;
}

int CacheableScriptFile::GetPosition() const
{
	ADDTOCALLSTACK("CacheableScriptFile::GetPosition");
	if (useDefaultFile())
		return CSFileText::GetPosition();

	return m_currentLine;
}
