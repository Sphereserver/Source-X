// NOTE: all scripts should be encoded in UTF-8.
// So they may have full unicode chars inside.

#include "../common/CLog.h"
#include "../sphere/threads.h"
#include "CException.h"
#include "CExpression.h"
#include "CScript.h"
#include "common.h"



///////////////////////////////////////////////////////////////
// -CScriptKey


bool CScriptKey::IsKey( lpctstr pszName ) const
{
	ASSERT(m_pszKey);
	return ( ! strcmpi( m_pszKey, pszName ));
}

bool CScriptKey::IsKeyHead( lpctstr pszName, size_t len ) const
{
	ASSERT(m_pszKey);
	return ( ! strnicmp( m_pszKey, pszName, len ));
}

void CScriptKey::InitKey()
{
	m_pszArg = m_pszKey = nullptr;
}

lpctstr CScriptKey::GetKey() const
{
	// Get the key or section name.
	ASSERT(m_pszKey);
	return m_pszKey;
}

// Args passed with the key.
bool CScriptKey::HasArgs() const
{
	ASSERT(m_pszArg);
	return (( m_pszArg[0] ) ? true : false );
}

tchar * CScriptKey::GetArgRaw() const	// Not need to parse at all.
{
	ASSERT(m_pszArg);
	return m_pszArg;
}

tchar * CScriptKey::GetArgStr( bool * fQuoted )	// this could be a quoted string ?
{
	ADDTOCALLSTACK("CScriptKey::GetArgStr");
	ASSERT(m_pszKey);

	tchar * pStr = GetArgRaw();
	if ( *pStr != '"' )
		return pStr ;

	++pStr;
	//tchar * pEnd = strchr( pStr, '"' );
	// search for last quote symbol starting from the end
	for (tchar * pEnd = pStr + strlen( pStr ) - 1; pEnd >= pStr; --pEnd )
	{
		if ( *pEnd == '"' )
		{
			*pEnd = '\0';
			if ( fQuoted )
				*fQuoted = true;
			break;
		}
	}

	return pStr;
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

int64 CScriptKey::GetArgLLFlag(uint64 iStart, uint64 iMask)
{
    ADDTOCALLSTACK("CScriptKey::GetArgLLFlag");
    // No args = toggle the flag.
    // 1 = set the flag.
    // 0 = clear the flag.

    ASSERT(m_pszKey);
    ASSERT(m_pszArg);

    if (!HasArgs())
        return (iStart ^ iMask);
    else if (GetArgLLVal())
        return (iStart | iMask);
    else
        return (iStart &~iMask);
}

char CScriptKey::GetArgCVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgCVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetCVal( m_pszArg );
}
uchar CScriptKey::GetArgUCVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgUCVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetUCVal( m_pszArg );
}

short CScriptKey::GetArgSVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgSVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetSVal( m_pszArg );
}

ushort CScriptKey::GetArgUSVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgUSVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetUSVal( m_pszArg );
}

int CScriptKey::GetArgVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetVal( m_pszArg );
}

uint CScriptKey::GetArgUVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgUVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetUVal( m_pszArg );
}

llong CScriptKey::GetArgLLVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgLLVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetLLVal( m_pszArg );
}

ullong CScriptKey::GetArgULLVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgULLVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetULLVal( m_pszArg );
}

byte CScriptKey::GetArgBVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgBVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetBVal( m_pszArg );
}

word CScriptKey::GetArgWVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgWVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetWVal( m_pszArg );
}

dword CScriptKey::GetArgDWVal()
{
	ADDTOCALLSTACK("CScriptKey::GetArgDWVal");
	ASSERT(m_pszKey);
	ASSERT(m_pszArg);
	return Exp_GetDWVal( m_pszArg );
}

int8 CScriptKey::GetArg8Val()
{
    ADDTOCALLSTACK("CScriptKey::GetArg8Val");
    ASSERT(m_pszKey);
    ASSERT(m_pszArg);
    return Exp_Get8Val( m_pszArg );
}

int16 CScriptKey::GetArg16Val()
{
    ADDTOCALLSTACK("CScriptKey::GetArg16Val");
    ASSERT(m_pszKey);
    ASSERT(m_pszArg);
    return Exp_Get16Val( m_pszArg );
}

int32 CScriptKey::GetArg32Val()
{
    ADDTOCALLSTACK("CScriptKey::GetArg32Val");
    ASSERT(m_pszKey);
    ASSERT(m_pszArg);
    return Exp_Get32Val( m_pszArg );
}

int64 CScriptKey::GetArg64Val()
{
    ADDTOCALLSTACK("CScriptKey::GetArg64Val");
    ASSERT(m_pszKey);
    ASSERT(m_pszArg);
    return Exp_Get64Val( m_pszArg );
}

uint8 CScriptKey::GetArgU8Val()
{
    ADDTOCALLSTACK("CScriptKey::GetArgU8Val");
    ASSERT(m_pszKey);
    ASSERT(m_pszArg);
    return Exp_Get8Val( m_pszArg );
}

uint16 CScriptKey::GetArgU16Val()
{
    ADDTOCALLSTACK("CScriptKey::GetArgU16Val");
    ASSERT(m_pszKey);
    ASSERT(m_pszArg);
    return Exp_GetU16Val( m_pszArg );
}

uint32 CScriptKey::GetArgU32Val()
{
    ADDTOCALLSTACK("CScriptKey::GetArgU32Val");
    ASSERT(m_pszKey);
    ASSERT(m_pszArg);
    return Exp_GetU32Val( m_pszArg );
}

uint64 CScriptKey::GetArgU64Val()
{
    ADDTOCALLSTACK("CScriptKey::GetArgU64Val");
    ASSERT(m_pszKey);
    ASSERT(m_pszArg);
    return Exp_GetU64Val( m_pszArg );
}

int64 CScriptKey::GetArgLLRange()
{
	ADDTOCALLSTACK("CScriptKey::GetArgLLRange");
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

CScriptKey::CScriptKey() :
	m_pszKey(nullptr), m_pszArg(nullptr)
{
}

CScriptKey::CScriptKey( tchar * ptcKey, tchar * ptcArg ) : 
	m_pszKey( ptcKey ), m_pszArg( ptcArg )
{
}


///////////////////////////////////////////////////////////////
// -CScriptKeyAlloc

tchar * CScriptKeyAlloc::_GetKeyBufferRaw( size_t iLen )
{
	//ADDTOCALLSTACK_INTENSIVE("CScriptKeyAlloc::_GetKeyBufferRaw");
	// iLen = length of the string we want to hold.
	if ( iLen > SCRIPT_MAX_LINE_LEN )
		iLen = SCRIPT_MAX_LINE_LEN;
	++iLen;	// add null.

	if ( m_Mem.GetDataLength() < iLen )
		m_Mem.Alloc( iLen );

	m_pszKey = m_pszArg = GetKeyBuffer();
	m_pszKey[0] = '\0';

	return m_pszKey;
}
/*
tchar * CScriptKeyAlloc::GetKeyBufferRaw( size_t iLen )
{
    ADDTOCALLSTACK("CScriptKeyAlloc::GetKeyBufferRaw");
    THREAD_UNIQUE_LOCK_RETURN(CScriptKeyAlloc::_GetKeyBufferRaw(iLen));
}
*/

tchar * CScriptKeyAlloc::GetKeyBuffer()
{
	// Get the buffer the key is in.
	ASSERT(m_Mem.GetData());
	return reinterpret_cast<tchar *>(m_Mem.GetData());
}

bool CScriptKeyAlloc::ParseKey( lpctstr ptcKey )
{
	//ADDTOCALLSTACK("CScriptKeyAlloc::ParseKey");
	// Skip leading white space
	if ( ! ptcKey )
	{
		_GetKeyBufferRaw(0);
		return false;
	}

	GETNONWHITESPACE( ptcKey );

	tchar * pBuffer = _GetKeyBufferRaw( strlen( ptcKey ));
	ASSERT(pBuffer);

	size_t iLen = m_Mem.GetDataLength();
	Str_CopyLimitNull( pBuffer, ptcKey, iLen );

	Str_Parse( pBuffer, &m_pszArg );
	return true;
}

bool CScriptKeyAlloc::ParseKey( lpctstr ptcKey, lpctstr pszVal )
{
	//ADDTOCALLSTACK("CScriptKeyAlloc::ParseKey");
	ASSERT(ptcKey);

	size_t lenkey = strlen( ptcKey );
	if ( ! lenkey )
		return ParseKey(pszVal);

	ASSERT( lenkey < SCRIPT_MAX_LINE_LEN-2 );

	size_t lenval = 0;
	if ( pszVal )
		lenval = strlen( pszVal );

	m_pszKey = _GetKeyBufferRaw( lenkey + lenval + 1 );

	strcpy( m_pszKey, ptcKey );
	m_pszArg = m_pszKey + lenkey;

	if ( pszVal )
	{
		++m_pszArg;
		lenval = m_Mem.GetDataLength();
		Str_CopyLimitNull( m_pszArg, pszVal, lenval - lenkey );
	}

	return true;
}

size_t CScriptKeyAlloc::ParseKeyEnd()
{
	//ADDTOCALLSTACK("CScriptKeyAlloc::ParseKeyEnd");
	// Now parse the line for comments and trailing whitespace junk
	// NOTE: leave leading whitespace for now.

	ASSERT(m_pszKey);

	int len = 0;
	for ( ; len < SCRIPT_MAX_LINE_LEN; ++len )
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
	return len;
}

void CScriptKeyAlloc::ParseKeyLate()
{
	//ADDTOCALLSTACK("CScriptKeyAlloc::ParseKeyLate");
	ASSERT(m_pszKey);
	ParseKeyEnd();
	GETNONWHITESPACE(m_pszKey);
	Str_Parse(m_pszKey, &m_pszArg);
}

///////////////////////////////////////////////////////////////
// -CScript

CScript::CScript()
{
	_InitBase();
}

CScript::CScript(lpctstr ptcKey) : CScript()
{
	ParseKey(ptcKey);
}

CScript::CScript(lpctstr ptcKey, lpctstr ptcVal) : CScript()
{
	ParseKey( ptcKey, ptcVal );
}

void CScript::_InitBase()
{
	_eParseFlags = ParseFlags::None;
	m_iResourceFileIndex = -1;
	m_iLineNum = m_iSectionData = 0;
	m_fSectionHead = _fCacheToBeUpdated = false;
	InitKey();
}

void CScript::CopyParseState(const CScript& other) noexcept
{
	_eParseFlags = other._eParseFlags;  // Special parsing conditions
	m_iResourceFileIndex = other.m_iResourceFileIndex;	// Index in g_Cfg.m_ResourceFiles of the CResourceScript (script file) where the CScript originated
	m_iLineNum = other.m_iLineNum;	// Line in the script file where Key/Arg were read
}

bool CScript::_Open( lpctstr ptcFilename, uint uiFlags )
{
	ADDTOCALLSTACK("CScript::_Open");
	// If we are in read mode and we have no script file.
	// ARGS: wFlags = OF_READ, OF_NONCRIT etc
	// RETURN: true = success.

    const bool fCacheToBeUpdated = _fCacheToBeUpdated;
	_InitBase();
    _fCacheToBeUpdated = fCacheToBeUpdated;

	if (ptcFilename == nullptr)
	{
		ptcFilename = _strFileName.GetBuffer();
		if (ptcFilename == nullptr)
			return false;
	}
	else if (_strFileName.Compare(ptcFilename))
	{
		_SetFilePath(ptcFilename);
	}

	lpctstr ptcTitle = _GetFileTitle();
	if ( !ptcTitle || (ptcTitle[0] == '\0') )
		return false;

	lpctstr ptcExt = GetFilesExt(ptcFilename);
	if ( !ptcExt )
	{
		tchar ptcTemp[_MAX_PATH];
		Str_CopyLimit(ptcTemp, ptcFilename, _MAX_PATH-4); // -4 beause of SPHERE_SCRIPT, which is = ".scp"
		strcat(ptcTemp, SPHERE_SCRIPT);
		_SetFilePath(ptcTemp);
		uiFlags |= OF_TEXT;
	}

	if ( (_fCacheToBeUpdated || !_HasCache()))
	{
        if (!CCacheableScriptFile::_Open(ptcFilename, uiFlags))
        {
            if ( ! ( uiFlags & OF_NONCRIT ))
                g_Log.Event(LOGL_WARN, "'%s' not found...\n", _strFileName.GetBuffer());
            return false;
        }
        _fCacheToBeUpdated = false;
	}

	return true;
}

bool CScript::Open( lpctstr ptcFilename, uint uiFlags )
{
    ADDTOCALLSTACK("CScript::Open");
    THREAD_UNIQUE_LOCK_RETURN(CScript::_Open(ptcFilename, uiFlags));
}

bool CScript::_ReadTextLine( bool fRemoveBlanks ) // Read a line from the opened script file
{
	// This function is called for each script line which is being parsed (so VERY frequently), and ADDTOCALLSTACK is expensive if called
	// this much often, so here it's to be preferred ADDTOCALLSTACK_INTENSIVE, even if we'll lose stack trace precision.
	//ADDTOCALLSTACK_INTENSIVE("CScript::_ReadTextLine");
	// ARGS:
	// fRemoveBlanks = Don't report any blank lines, (just keep reading)

    tchar* ptcBuf = _GetKeyBufferRaw(SCRIPT_MAX_LINE_LEN);
	while ( CCacheableScriptFile::_ReadString( ptcBuf, SCRIPT_MAX_LINE_LEN ))
	{
		++m_iLineNum;
		if ( fRemoveBlanks )
		{
			if ( ParseKeyEnd() <= 0 )
				continue;
		}
		return true;
	}

	m_pszKey[0] = '\0';
	return false;
}
bool CScript::ReadTextLine( bool fRemoveBlanks ) // Read a line from the opened script file
{
    THREAD_UNIQUE_LOCK_RETURN(CScript::_ReadTextLine(fRemoveBlanks));
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
			return false;
		}
		if ( IsKeyHead( "[EOF]", 5 ))
		{
			return false;
		}
	}
	while ( ! IsKeyHead( pszName, len ));
	return true;
}

int CScript::_Seek( int iOffset, int iOrigin )
{
	ADDTOCALLSTACK("CScript::_Seek");
	// Go to the start of a new section.
	// RETURN: the new offset in bytes from start of file.
	if ( (iOffset == 0) && (iOrigin == SEEK_SET) )
		m_iLineNum = 0;	// so we don't have to override SeekToBegin
	m_fSectionHead = false;		// unknown, so start at the beginning.
	m_iSectionData = iOffset;
	return CCacheableScriptFile::_Seek(iOffset,iOrigin);
}
int CScript::Seek( int iOffset, int iOrigin )
{
    ADDTOCALLSTACK("CScript::Seek");
    THREAD_UNIQUE_LOCK_RETURN(CScript::_Seek(iOffset, iOrigin));
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
			m_iSectionData = GetPosition();
			return false;
		}
        tchar* ptcScriptLine = m_pszKey;
        GETNONWHITESPACE(ptcScriptLine);
        if (ptcScriptLine[0] == '[')
        {
            m_pszKey = ptcScriptLine;
            break;
        }
	}

foundit:
	// Parse up the section name.
	++m_pszKey;
	size_t len = strlen( m_pszKey );
	for ( size_t i = 0; i < len; ++i )
	{
		if ( m_pszKey[i] == ']' )
		{
			m_pszKey[i] = '\0';
			break;
		}
	}

	m_iSectionData = GetPosition();
	if ( IsSectionType( "EOF" ))
		return false;

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
		return false;
	}

	TemporaryString tsSec;
	snprintf(tsSec.buffer(), tsSec.capacity(), "[%s]", pszName);
	if ( FindTextHeader(tsSec.buffer()) )
	{
		// Success
		m_iSectionData = GetPosition();
		return true;
	}

	// Failure Error display. (default)

	if ( ! ( uModeFlags & OF_NONCRIT ))
		g_Log.Event(LOGL_WARN, "Did not find '%s' section '%s'\n", GetFileTitle(), pszName);
	return false;
}

lpctstr CScript::GetSection() const
{
	ASSERT(m_pszKey);
	return m_pszKey;
}

bool CScript::IsSectionType( lpctstr pszName ) //const
{
	// Only valid after FindNextSection()
	return( ! strcmpi( GetKey(), pszName ) );
}

bool CScript::ReadKey( bool fRemoveBlanks )
{
	// This function is called for each script line which is being parsed (so VERY frequently), and ADDTOCALLSTACK is expensive
	//ADDTOCALLSTACK("CScript::ReadKey");

	if ( ! _ReadTextLine(fRemoveBlanks))	// Assume it's an internal method or, if interface, i already acquired the lock
		return false;

    tchar* ptcKey = m_pszKey;
    GETNONWHITESPACE(ptcKey);
	if ( ptcKey[0] == '[' )	// hit the end of our section.
	{
		m_fSectionHead = true;
		return false;
	}
	return true;
}

bool CScript::ReadKeyParse() // Read line from script
{
	// This function is called for each script line which is being parsed (so VERY frequently), and ADDTOCALLSTACK is expensive
	//ADDTOCALLSTACK("CScript::ReadKeyParse");

	EXC_TRY("ReadKeyParse");
	EXC_SET_BLOCK("read");
	if ( !ReadKey(true) )
	{
		EXC_SET_BLOCK("init");
		InitKey();
		return false;	// end of section.
	}

	ASSERT(m_pszKey);
	GETNONWHITESPACE( m_pszKey );
	EXC_SET_BLOCK("parse");
	Str_Parse( m_pszKey, &m_pszArg );

	//if ( !m_pszArg[0] || m_pszArg[1] != '=' || !strchr( ".*+-/%|&!^", m_pszArg[0] ) )
	if ( !m_pszArg[0] || ( m_pszArg[1] != '=' && m_pszArg[1] != '+' && m_pszArg[1] != '-' ) || !strchr( ".*+-/%|&!^", m_pszArg[0] ) )
		return true;
    //_strupr(m_pszKey);  // make the KEY uppercase

	static lpctstr constexpr sm_szEvalTypes[] =
	{
		"EVAL",
		"FLOATVAL"
	};

	EXC_SET_BLOCK("parse");
	tchar* pszArgs = m_pszArg;
	pszArgs += 2;
	GETNONWHITESPACE( pszArgs );
	
	const int iKeyIndex = (strnicmp(m_pszKey, "FLOAT.", 6) == 0) ? 1 : 0;
	TemporaryString tsBuf;
	if ( m_pszArg[0] == '.' )	// ".=" concatenation operator
	{
		if ( *pszArgs == '"' )
		{
			tchar *	pQuote	= strchr( pszArgs+1, '"' );
			if ( pQuote )
			{
				++pszArgs;
				*pQuote	= '\0';
			}
		}
		snprintf(tsBuf.buffer(), tsBuf.capacity(), "<%s>%s", m_pszKey, pszArgs);
	}
	else if ( m_pszArg[0] == m_pszArg[1] && m_pszArg[1] == '+' )
	{
		if ( m_pszArg[2] != '\0' )
			return true;
		snprintf(tsBuf.buffer(), tsBuf.capacity(), "<EVAL (<%s> +1)>", m_pszKey);
	}
	else if ( m_pszArg[0] == m_pszArg[1] && m_pszArg[1] == '-' )
	{
		if ( m_pszArg[2] != '\0' )
			return true;
		snprintf(tsBuf.buffer(), tsBuf.capacity(), "<EVAL (<%s> -1)>", m_pszKey);
	}
	else
	{
		snprintf(tsBuf.buffer(), tsBuf.capacity(), "<%s (<%s> %c (%s))>", sm_szEvalTypes[iKeyIndex], m_pszKey, *m_pszArg, pszArgs);
	}
	strcpy(m_pszArg, tsBuf.buffer());

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
		return false;
	}
	Seek( m_iSectionData );
	while ( ReadKeyParse() )
	{
		if ( IsKey( pszName ) )
		{
			m_pszArg = Str_TrimWhitespace( m_pszArg );
			return true;
		}
	}
	return false;
}

void CScript::_Close()
{
	ADDTOCALLSTACK("CScript::_Close");
	// EndSection();
    CCacheableScriptFile::_Close();
}
void CScript::Close()
{
    ADDTOCALLSTACK("CScript::Close");
    // EndSection();
    CCacheableScriptFile::Close();
}

void CScript::CloseForce()
{
    ADDTOCALLSTACK("CScript::CloseForce");
	CScript::Close();
}

bool CScript::_SeekContext( CScriptLineContext LineContext )
{
	m_iLineNum = LineContext.m_iLineNum;
	return (_Seek( LineContext.m_iOffset, SEEK_SET ) == LineContext.m_iOffset);
}
bool CScript::SeekContext( CScriptLineContext LineContext )
{
    THREAD_UNIQUE_LOCK_RETURN(CScript::_SeekContext(LineContext));
}

CScriptLineContext CScript::_GetContext() const
{
	CScriptLineContext LineContext;
	LineContext.m_iLineNum = m_iLineNum;
	LineContext.m_iOffset = _GetPosition();
	return LineContext;
}

CScriptLineContext CScript::GetContext() const
{
    THREAD_UNIQUE_LOCK_SET;
    CScriptLineContext LineContext;
    LineContext.m_iLineNum = m_iLineNum;
    LineContext.m_iOffset = _GetPosition();
    return LineContext;
}

bool _cdecl CScript::WriteSection( lpctstr ptcSection, ... )
{
	ADDTOCALLSTACK_INTENSIVE("CScript::WriteSection");
	// Write out the section header.

	// EndSection();	// End any previous section.
	TemporaryString tsHeader;
	TemporaryString tsSectionFormatted;

	{
		va_list vargs;
		va_start(vargs, ptcSection);
		vsnprintf(tsSectionFormatted.buffer(), tsSectionFormatted.capacity(), ptcSection, vargs);
		va_end(vargs);
	}

	
	memcpy(tsHeader.buffer(), "\n[", 2u);	// 2 -> not counting the string terminator
	size_t offset = 2;
	offset += Str_CopyLimitNull(tsHeader.buffer() + offset, tsSectionFormatted.buffer(), tsHeader.capacity() - 2);
	offset += Str_ConcatLimitNull(tsHeader.buffer() + offset, "]\n", tsHeader.capacity() - offset);

	THREAD_UNIQUE_LOCK_SET;
	_Write(tsHeader.buffer(), int(offset));

	return true;
}

bool CScript::WriteKeySingle(lptstr ptcKey)
{
	ADDTOCALLSTACK_INTENSIVE("CScript::WriteKeySingle");
	if (ptcKey == nullptr || ptcKey[0] == '\0')
	{
		return false;
	}

	tchar ch = '\0';
	tchar* ptcSep = strpbrk(ptcKey, "\n\r");

	if (ptcSep != nullptr)
	{
		g_Log.Event(LOGL_WARN | LOGM_CHEAT, "Carriage return in key string (book?) - truncating.\n");
		ch = *ptcSep;
		*ptcSep = '\0';
	}

	// Books are like this. No real keys.
	// Write: "KEY\n"
	size_t uiStrLen = strlen(ptcKey);
	ptcKey[uiStrLen++] = '\n'; // No need for string terminator, since i'm explicitly passing the number of bytes to write
	Write(ptcKey, int(uiStrLen));

	if (ptcSep != nullptr)
		*ptcSep = ch;

	return true;
}

bool CScript::WriteKeyStr(lpctstr ptcKey, lpctstr ptcVal)
{
	ADDTOCALLSTACK_INTENSIVE("CScript::WriteKeyStr");
	if (ptcKey == nullptr || ptcKey[0] == '\0')
	{
		return false;
	}
	ASSERT(ptcVal);

#ifdef _DEBUG
	ASSERT(nullptr == strpbrk(ptcKey, "\n\r"));	// Ensure that the Key value is always valid!
#endif

	if (const tchar* ptcSep = strpbrk(ptcVal, "\n\r"))
	{
		g_Log.Event(LOGL_WARN | LOGM_CHEAT, "Carriage return in value string - truncating.\n");
		lptstr ptcTruncated = Str_GetTemp();
		Str_CopyLimitNull(ptcTruncated, ptcVal, size_t(ptcSep - ptcVal));
		ptcVal = ptcTruncated;
	}

	// Write: "KEY=VAL\n"
	_sWriteBuffer_keyVal.resize(SCRIPT_MAX_LINE_LEN);
	const size_t uiCapacity = _sWriteBuffer_keyVal.capacity();
	lptstr ptcBuf = _sWriteBuffer_keyVal.data();

	size_t uiStrLen = Str_CopyLimitNull(ptcBuf, ptcKey, uiCapacity);
	ptcBuf[uiStrLen++] = '=';
	ptcBuf[uiStrLen]   = '\0'; // Needed by Str_ConcatLimitNull
	uiStrLen += Str_ConcatLimitNull(ptcBuf + uiStrLen, ptcVal, uiCapacity - uiStrLen);
	ptcBuf[uiStrLen++] = '\n';
	// ptcBuf[uiStrLen] = '\0';		// Not needed, since the whole buffer is zero-filled.
	Write(ptcBuf, int(uiStrLen));

	return true;
}

//void _cdecl CScript::WriteKeyFormat( lpctstr ptcKey, lpctstr pszVal, ... )
//{
//	ADDTOCALLSTACK("CScript::WriteKeyFormat");
//	tchar	*pszTemp = Str_GetTemp();
//	va_list vargs;
//	va_start( vargs, pszVal );
//	vsprintf(pszTemp, pszVal, vargs);
//	WriteKeyVal(ptcKey, pszTemp);
//	va_end( vargs );
//}

void _cdecl CScript::WriteKeyFormat(lpctstr ptcKey, lpctstr pszVal, ...)
{
	ADDTOCALLSTACK_INTENSIVE("CScript::WriteKeyFormat");
	_sWriteBuffer_num.resize(SCRIPT_MAX_LINE_LEN);
	va_list vargs;
	va_start( vargs, pszVal );
	vsnprintf(_sWriteBuffer_num.data(), _sWriteBuffer_num.capacity(), pszVal, vargs);
	WriteKeyStr(ptcKey, _sWriteBuffer_num.data());
	va_end( vargs );
}

void CScript::WriteKeyVal(lpctstr ptcKey, int64 iVal)
{
	_sWriteBuffer_num.resize(SCRIPT_MAX_LINE_LEN);
	Str_FromLL(iVal, _sWriteBuffer_num.data(), _sWriteBuffer_num.capacity(), 10);
	WriteKeyStr(ptcKey, _sWriteBuffer_num.data());
}

void CScript::WriteKeyHex(lpctstr ptcKey, int64 iVal)
{
	_sWriteBuffer_num.resize(SCRIPT_MAX_LINE_LEN);
	Str_FromLL(iVal, _sWriteBuffer_num.data(), _sWriteBuffer_num.capacity(), 16);
	WriteKeyStr(ptcKey, _sWriteBuffer_num.data());
}

