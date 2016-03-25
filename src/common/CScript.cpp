// NOTE: all scripts should be encoded in UTF-8.
// So they may have full unicode chars inside.

#include "../game/CLog.h"
#include "../sphere/threads.h"
#include "CException.h"
#include "CExpression.h"
#include "CScript.h"
#include "graycom.h"


void CScriptLineContext::Init()
{
	m_lOffset = -1;
	m_iLineNum = -1;
}

bool CScriptLineContext::IsValid() const
{
	return( m_lOffset >= 0 );
}

CScriptLineContext::CScriptLineContext()
{
	Init();
}


///////////////////////////////////////////////////////////////
// -CScriptKey


bool CScriptKey::IsKey( lpctstr pszName ) const
{
	ASSERT(m_pszKey);
	return( ! strcmpi( m_pszKey, pszName ));
}

bool CScriptKey::IsKeyHead( lpctstr pszName, size_t len ) const
{
	ASSERT(m_pszKey);
	return( ! strnicmp( m_pszKey, pszName, len ));
}

void CScriptKey::InitKey()
{
	ADDTOCALLSTACK("CScriptKey::InitKey");
	m_pszArg = m_pszKey = NULL;
}

lpctstr CScriptKey::GetKey() const
{
	// Get the key or section name.
	ASSERT(m_pszKey);
	return(m_pszKey);
}

// Args passed with the key.
bool CScriptKey::HasArgs() const
{
	ASSERT(m_pszArg);
	return(( m_pszArg[0] ) ? true : false );
}

tchar * CScriptKey::GetArgRaw() const	// Not need to parse at all.
{
	ASSERT(m_pszArg);
	return(m_pszArg);
}

tchar * CScriptKey::GetArgStr( bool * fQuoted )	// this could be a quoted string ?
{
	ADDTOCALLSTACK("CScriptKey::GetArgStr");
	ASSERT(m_pszKey);

	tchar * pStr = GetArgRaw();
	if ( *pStr != '"' )
		return( pStr );

	pStr++;
	//tchar * pEnd = strchr( pStr, '"' );
	// search for last qoute sybol starting from the end
	for (tchar * pEnd = pStr + strlen( pStr ) - 1; pEnd >= pStr; pEnd-- )
	{
		if ( *pEnd == '"' )
		{
			*pEnd = '\0';
			if ( fQuoted )
				*fQuoted = true;
			break;
		}
	}

	return( pStr );
}

tchar * CScriptKey::GetArgStr()
{
	return GetArgStr( NULL );
}

dword CScriptKey::GetArgFlag( dword dwStart, dword dwMask )
{
	ADDTOCALLSTACK("CScriptKey::GetArgFlag");
	// No args = toggle the flag.
	// 1 = set the flag.
	// 0 = clear the flag.

	ASSERT(m_pszKey);
	ASSERT(m_pszArg);

	if ( ! HasArgs())
		return ( dwStart ^ dwMask );
	else if ( GetArgVal())
		return ( dwStart | dwMask );
	else
		return ( dwStart &~ dwMask );
}

llong CScriptKey::GetArgLLVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgLLVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetLLVal( m_pszArg );
}

int CScriptKey::GetArgVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetVal( m_pszArg );
}

llong CScriptKey::GetArgLLRange()
{
	ADDTOCALLSTACK("CScriptKey::GetArgRange");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetLLRange(m_pszArg);
}

int CScriptKey::GetArgRange()
{
	ADDTOCALLSTACK("CScriptKey::GetArgRange");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetRange( m_pszArg );
}

CScriptKey::CScriptKey() : m_pszKey(NULL), m_pszArg(NULL)
{
}

CScriptKey::CScriptKey( tchar * pszKey, tchar * pszArg ) : m_pszKey( pszKey ), m_pszArg( pszArg )
{
}

CScriptKey::~CScriptKey()
{
}

///////////////////////////////////////////////////////////////
// -CScriptKeyAlloc

tchar * CScriptKeyAlloc::GetKeyBufferRaw( size_t iLen )
{
	ADDTOCALLSTACK("CScriptKeyAlloc::GetKeyBufferRaw");
	// iLen = length of the string we want to hold.
	if ( iLen > SCRIPT_MAX_LINE_LEN )
		iLen = SCRIPT_MAX_LINE_LEN;
	iLen ++;	// add null.

	if ( m_Mem.GetDataLength() < iLen )
	{
		m_Mem.Alloc( iLen );
	}

	m_pszKey = m_pszArg = GetKeyBuffer();
	m_pszKey[0] = '\0';

	return m_pszKey;
}

tchar * CScriptKeyAlloc::GetKeyBuffer()
{
	// Get the buffer the key is in.
	ASSERT(m_Mem.GetData());
	return reinterpret_cast<tchar *>(m_Mem.GetData());
}

bool CScriptKeyAlloc::ParseKey( lpctstr pszKey )
{
	ADDTOCALLSTACK("CScriptKeyAlloc::ParseKey");
	// Skip leading white space 
	if ( ! pszKey )
	{
		GetKeyBufferRaw(0);
		return false;
	}

	GETNONWHITESPACE( pszKey );

	tchar * pBuffer = GetKeyBufferRaw( strlen( pszKey ));
	ASSERT(pBuffer);

	size_t iLen = m_Mem.GetDataLength() - 1;
	strncpy( pBuffer, pszKey, iLen );
	pBuffer[iLen] = '\0';

	Str_Parse( pBuffer, &m_pszArg );
	return( true );
}

bool CScriptKeyAlloc::ParseKey( lpctstr pszKey, lpctstr pszVal )
{
	ADDTOCALLSTACK("CScriptKeyAlloc::ParseKey");
	ASSERT(pszKey);

	size_t lenkey = strlen( pszKey );
	if ( ! lenkey )
	{
		return ParseKey(pszVal);
	}

	ASSERT( lenkey < SCRIPT_MAX_LINE_LEN-2 );

	size_t lenval = 0;
	if ( pszVal )
	{
		lenval = strlen( pszVal );
	}

	m_pszKey = GetKeyBufferRaw( lenkey + lenval + 1 );

	strcpy( m_pszKey, pszKey );
	m_pszArg = m_pszKey + lenkey;

	if ( pszVal )
	{
		m_pszArg ++;
		lenval = m_Mem.GetDataLength() - 2;
		strcpylen( m_pszArg, pszVal, ( lenval - lenkey ) + 1 );	// strcpylen
	}

	return( true );
}

size_t CScriptKeyAlloc::ParseKeyEnd()
{
	ADDTOCALLSTACK("CScriptKeyAlloc::ParseKeyEnd");
	// Now parse the line for comments and trailing whitespace junk
	// NOTE: leave leading whitespace for now.

	ASSERT(m_pszKey);

	size_t len = 0;
	for ( ; len < SCRIPT_MAX_LINE_LEN; len++ )
	{
		tchar ch = m_pszKey[len];
		if ( ch == '\0' )
			break;
		if ( ch == '/' && m_pszKey[len + 1] == '/' )
		{
			// Remove comment at end of line.
			break;
		}
	}

	// Remove CR and LF from the end of the line.
	len = Str_TrimEndWhitespace( m_pszKey, len );
	if ( len <= 0 )		// fRemoveBlanks &&
		return 0;

	m_pszKey[len] = '\0';
	return( len );
}

void CScriptKeyAlloc::ParseKeyLate()
{
	ADDTOCALLSTACK("CScriptKeyAlloc::ParseKeyLate");
	ASSERT(m_pszKey);
	ParseKeyEnd();
	GETNONWHITESPACE(m_pszKey);
	Str_Parse(m_pszKey, &m_pszArg);
}

///////////////////////////////////////////////////////////////
// -CScript

CScript::CScript()
{
	InitBase();
}

CScript::CScript( lpctstr pszKey )
{
	InitBase();
	ParseKey(pszKey);
}

CScript::CScript( lpctstr pszKey, lpctstr pszVal )
{
	InitBase();
	ParseKey( pszKey, pszVal );
}

void CScript::InitBase()
{
	ADDTOCALLSTACK("CScript::InitBase");
	m_iLineNum		= 0;
	m_fSectionHead	= false;
	m_lSectionData	= 0;
	InitKey();
}

bool CScript::Open( lpctstr pszFilename, uint wFlags )
{
	ADDTOCALLSTACK("CScript::Open");
	// If we are in read mode and we have no script file.
	// ARGS: wFlags = OF_READ, OF_NONCRIT etc
	// RETURN: true = success.

	InitBase();

	if ( pszFilename == NULL )
	{
		pszFilename = GetFilePath();
	}
	else
	{
		SetFilePath( pszFilename );
	}

	lpctstr pszTitle = GetFileTitle();
	if ( pszTitle == NULL || pszTitle[0] == '\0' )
		return( false );

	lpctstr pszExt = GetFilesExt( GetFilePath() ); 
	if ( pszExt == NULL )
	{
		tchar szTemp[ _MAX_PATH ];
		strcpy( szTemp, GetFilePath() );
		strcat( szTemp, GRAY_SCRIPT );
		SetFilePath( szTemp );
		wFlags |= OF_TEXT;
	}

	if ( !PhysicalScriptFile::Open( GetFilePath(), wFlags ))
	{
		if ( ! ( wFlags & OF_NONCRIT ))
		{
			g_Log.Event(LOGL_WARN, "'%s' not found...\n", static_cast<lpctstr>(GetFilePath()));
		}
		return( false );
	}

	return( true );
}

bool CScript::ReadTextLine( bool fRemoveBlanks ) // Read a line from the opened script file
{
	ADDTOCALLSTACK("CScript::ReadTextLine");
	// ARGS:
	// fRemoveBlanks = Don't report any blank lines, (just keep reading)
	//

	while ( PhysicalScriptFile::ReadString( GetKeyBufferRaw(SCRIPT_MAX_LINE_LEN), SCRIPT_MAX_LINE_LEN ))
	{
		m_iLineNum++;
		if ( fRemoveBlanks )
		{
			if ( ParseKeyEnd() <= 0 )
				continue;
		}
		return( true );
	}

	m_pszKey[0] = '\0';
	return( false );
}

bool CScript::FindTextHeader( lpctstr pszName ) // Find a section in the current script
{
	ADDTOCALLSTACK("CScript::FindTextHeader");
	// RETURN: false = EOF reached.
	ASSERT(pszName);
	ASSERT( ! IsBinaryMode());

	SeekToBegin();

	size_t len = strlen( pszName );
	ASSERT(len);
	do
	{
		if ( ! ReadTextLine(false))
		{
			return( false );
		}
		if ( IsKeyHead( "[EOF]", 5 ))
		{
			return( false );
		}
	}
	while ( ! IsKeyHead( pszName, len ));
	return( true );
}

dword CScript::Seek( int offset, uint origin )
{
	ADDTOCALLSTACK("CScript::Seek");
	// Go to the start of a new section.
	// RETURN: the new offset in bytes from start of file.
	if ( offset == 0 && origin == SEEK_SET )
	{
		m_iLineNum = 0;	// so we don't have to override SeekToBegin
	}
	m_fSectionHead = false;		// unknown , so start at the beginning.
	m_lSectionData = offset;
	return( PhysicalScriptFile::Seek(offset,origin));
}

bool CScript::FindNextSection()
{
	ADDTOCALLSTACK("CScript::FindNextSection");
	EXC_TRY("FindNextSection");
	// RETURN: false = EOF.

	if ( m_fSectionHead )	// we have read a section already., (not at the start)
	{
		// Start from the previous line. It was the line that ended the last read.
		m_pszKey = GetKeyBuffer();
		ASSERT(m_pszKey);
		m_fSectionHead = false;
		if ( m_pszKey[0] == '[' )
			goto foundit;
	}

	for (;;)
	{
		if ( !ReadTextLine(true) )
		{
			m_lSectionData = GetPosition();
			return( false );
		}
		if ( m_pszKey[0] == '[' )
			break;
	}

foundit:
	// Parse up the section name.
	m_pszKey++;
	size_t len = strlen( m_pszKey );
	for ( size_t i = 0; i < len; i++ )
	{
		if ( m_pszKey[i] == ']' )
		{
			m_pszKey[i] = '\0';
			break;
		}
	}

	m_lSectionData = GetPosition();
	if ( IsSectionType( "EOF" ))
		return( false );

	Str_Parse( m_pszKey, &m_pszArg );
	return true;
	EXC_CATCH;
	return false;
}

bool CScript::FindSection( lpctstr pszName, uint uModeFlags )
{
	ADDTOCALLSTACK("CScript::FindSection");
	// Find a section in the current script
	// RETURN: true = success

	ASSERT(pszName);
	if ( strlen( pszName ) > 32 )
	{
		DEBUG_ERR(( "Bad script section name\n" ));
		return( false );
	}

	TemporaryString pszSec;
	sprintf(static_cast<tchar *>(pszSec), "[%s]", pszName);
	if ( FindTextHeader(pszSec))
	{
		// Success
		m_lSectionData = GetPosition();
		return( true );
	}

	// Failure Error display. (default)

	if ( ! ( uModeFlags & OF_NONCRIT ))
	{
		g_Log.Event(LOGL_WARN, "Did not find '%s' section '%s'\n", static_cast<lpctstr>(GetFileTitle()), static_cast<lpctstr>(pszName));
	}
	return( false );
}

lpctstr CScript::GetSection() const
{
	ASSERT(m_pszKey);
	return( m_pszKey );
}

bool CScript::IsSectionType( lpctstr pszName ) //const
{
	// Only valid after FindNextSection()
	return( ! strcmpi( GetKey(), pszName ));
}

bool CScript::ReadKey( bool fRemoveBlanks )
{
	ADDTOCALLSTACK("CScript::ReadKey");
	if ( ! ReadTextLine(fRemoveBlanks))
		return( false );
	if ( m_pszKey[0] == '[' )	// hit the end of our section.
	{
		m_fSectionHead = true;
		return( false );
	}
	return( true );
}

bool CScript::ReadKeyParse() // Read line from script
{
	ADDTOCALLSTACK("CScript::ReadKeyParse");
	EXC_TRY("ReadKeyParse");
	EXC_SET("read");
	if ( !ReadKey(true) )
	{
		EXC_SET("init");
		InitKey();
		return false;	// end of section.
	}

	ASSERT(m_pszKey);
	GETNONWHITESPACE( m_pszKey );
	EXC_SET("parse");
	Str_Parse( m_pszKey, &m_pszArg );

	//if ( !m_pszArg[0] || m_pszArg[1] != '=' || !strchr( ".*+-/%|&!^", m_pszArg[0] ) )
	if ( !m_pszArg[0] || ( m_pszArg[1] != '=' && m_pszArg[1] != '+' && m_pszArg[1] != '-' ) || !strchr( ".*+-/%|&!^", m_pszArg[0] ) )
		return true;

	static lpctstr const sm_szEvalTypes[] =
	{
		"eval",
		"floatval"
	};

	EXC_SET("parse");
	lpctstr	pszArgs	= m_pszArg;
	pszArgs += 2;
	GETNONWHITESPACE( pszArgs );
	TemporaryString buf;

	int iKeyIndex = (strnicmp(m_pszKey, "float.", 6) == 0) ? 1 : 0;

	if ( m_pszArg[0] == '.' )
	{
		if ( *pszArgs == '"' )
		{
			tchar *	pQuote	= const_cast<tchar*>(strchr( pszArgs+1, '"' ));
			if ( pQuote )
			{
				pszArgs++;
				*pQuote	= '\0';
			}
		}
		sprintf(buf, "<%s>%s", m_pszKey, pszArgs);
	}
	else if ( m_pszArg[0] == m_pszArg[1] && m_pszArg[1] == '+' )
	{
		if ( m_pszArg[2] != '\0' )
			return true;
		sprintf(buf, "<eval (<%s> +1)>", m_pszKey);
	}
	else if ( m_pszArg[0] == m_pszArg[1] && m_pszArg[1] == '-' )
	{
		if ( m_pszArg[2] != '\0' )
			return true;
		sprintf(buf, "<eval (<%s> -1)>", m_pszKey);
	}
	else
	{
		sprintf(buf, "<%s (<%s> %c (%s))>", sm_szEvalTypes[iKeyIndex], m_pszKey, *m_pszArg, pszArgs);
	}
	strcpy(m_pszArg, buf);

	return true;
	EXC_CATCH;
	return false;
}

bool CScript::FindKey( lpctstr pszName ) // Find a key in the current section
{
	ADDTOCALLSTACK("CScript::FindKey");
	if ( strlen( pszName ) > SCRIPT_MAX_SECTION_LEN )
	{
		DEBUG_ERR(( "Bad script key name\n" ));
		return( false );
	}
	Seek( m_lSectionData );
	while ( ReadKeyParse())
	{
		if ( IsKey( pszName ))
		{
			m_pszArg = Str_TrimWhitespace( m_pszArg );
			return true;
		}
	}
	return( false );
}

void CScript::Close()
{
	ADDTOCALLSTACK("CScript::Close");
	// EndSection();
	PhysicalScriptFile::Close();
}

void CScript::CloseForce()
{
	CScript::Close();
}

bool CScript::SeekContext( CScriptLineContext LineContext )
{
	m_iLineNum = LineContext.m_iLineNum;
	return Seek( LineContext.m_lOffset, SEEK_SET ) == static_cast<dword>(LineContext.m_lOffset);
}

CScriptLineContext CScript::GetContext() const
{
	CScriptLineContext LineContext;
	LineContext.m_iLineNum = m_iLineNum;
	LineContext.m_lOffset = GetPosition();
	return( LineContext );
}

bool _cdecl CScript::WriteSection( lpctstr pszSection, ... )
{
	ADDTOCALLSTACK_INTENSIVE("CScript::WriteSection");
	// Write out the section header.
	va_list vargs;
	va_start( vargs, pszSection );

	// EndSection();	// End any previous section.
	Printf( "\n[");
	VPrintf( pszSection, vargs );
	Printf( "]\n" );
	va_end( vargs );

	return( true );
}

bool CScript::WriteKey( lpctstr pszKey, lpctstr pszVal )
{
	ADDTOCALLSTACK_INTENSIVE("CScript::WriteKey");
	if ( pszKey == NULL || pszKey[0] == '\0' )
	{
		return false;
	}

	tchar ch = '\0';
	tchar * pszSep;
	if ( pszVal == NULL || pszVal[0] == '\0' )
	{
		pszSep = const_cast<tchar*>(strchr( pszKey, '\n' ));
		if ( pszSep == NULL )
			pszSep = const_cast<tchar*>(strchr( pszKey, '\r' )); // acts like const_cast

		if ( pszSep != NULL )
		{
			g_Log.Event( LOGL_WARN|LOGM_CHEAT, "carriage return in key (book?) - truncating\n" );
			ch = *pszSep;
			*pszSep	= '\0';
		}

		// Books are like this. No real keys.
		Printf( "%s\n", pszKey );

		if ( pszSep != NULL )
			*pszSep	= ch;
	}
	else
	{
		pszSep = const_cast<tchar*>(strchr( pszVal, '\n' ));
		if ( pszSep == NULL )
			pszSep = const_cast<tchar*>(strchr( pszVal, '\r' )); // acts like const_cast

		if ( pszSep != NULL )
		{
			g_Log.Event( LOGL_WARN|LOGM_CHEAT, "carriage return in key value - truncating\n" );
			ch = *pszSep;
			*pszSep	= '\0';
		}

		Printf( "%s=%s\n", pszKey, pszVal );

		if ( pszSep != NULL )
			*pszSep	= ch;
	}

	return( true );
}

//void _cdecl CScript::WriteKeyFormat( lpctstr pszKey, lpctstr pszVal, ... )
//{
//	ADDTOCALLSTACK("CScript::WriteKeyFormat");
//	tchar	*pszTemp = Str_GetTemp();
//	va_list vargs;
//	va_start( vargs, pszVal );
//	vsprintf(pszTemp, pszVal, vargs);
//	WriteKey(pszKey, pszTemp);
//	va_end( vargs );
//}

void _cdecl CScript::WriteKeyFormat( lpctstr pszKey, lpctstr pszVal, ... )
{
	ADDTOCALLSTACK_INTENSIVE("CScript::WriteKeyFormat");
	TemporaryString pszTemp;
	va_list vargs;
	va_start( vargs, pszVal );
	_vsnprintf(pszTemp, pszTemp.realLength(), pszVal, vargs);
	WriteKey(pszKey, pszTemp);
	va_end( vargs );
}

void CScript::WriteKeyVal( lpctstr pszKey, int64 dwVal )
{
#ifdef __MINGW32__
	WriteKeyFormat( pszKey, "%I64d", dwVal );
#else  // __MINGW32__
	WriteKeyFormat( pszKey, "%" PRId64 , dwVal );
#endif  // __MINGW32__
}

void CScript::WriteKeyHex( lpctstr pszKey, int64 dwVal )
{
#ifdef __MINGW32__
	WriteKeyFormat( pszKey, "0%I64x", dwVal );
#else  // __MINGW32__
	WriteKeyFormat( pszKey, "0%" PRIx64 , dwVal );
#endif  // __MINGW32__
}

CScript::~CScript()
{
}