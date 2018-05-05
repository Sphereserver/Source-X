/**
* @file CScript.h
* @brief Script entity.
*/

#ifndef _INC_CSCRIPT_H
#define _INC_CSCRIPT_H

#include "sphere_library/CSMemBlock.h"
#include "CacheableScriptFile.h"

#define SPHERE_SCRIPT		".scp"
#define SCRIPT_MAX_SECTION_LEN 128


struct CScriptLineContext
{
public:
	size_t m_stOffset;
	int m_iLineNum;		// for debug purposes if there is an error.
public:
	void Init();
	bool IsValid() const;
	CScriptLineContext();
};

class CScriptKey
{
	// A single line form a script file.
	// This is usually in the form of KEY=ARG
	// Unknown allocation of pointers.
protected:
	tchar * m_pszKey;		// The key. (or just start of the line)
	tchar * m_pszArg;		// for parsing the last read line.	KEY=ARG or KEY ARG

public:
	static const char *m_sClassName;
	bool IsKey( lpctstr pszName ) const;
	bool IsKeyHead( lpctstr pszName, size_t len ) const;
	void InitKey();

	// Query the key.
	lpctstr GetKey() const;

	// Args passed with the key.
	bool HasArgs() const;
	tchar * GetArgRaw() const;				// Not need to parse at all.
	tchar * GetArgStr( bool * fQuoted );	// this could be a quoted string ?
	inline tchar * GetArgStr() {
		return GetArgStr(NULL);
	}

	char GetArgCVal();
	uchar GetArgUCVal();
	short GetArgSVal();
	ushort GetArgUSVal();
	int GetArgVal();
	uint GetArgUVal();
	llong GetArgLLVal();
	ullong GetArgULLVal();
	byte GetArgBVal();
	word GetArgWVal();
	dword GetArgDWVal();
    int8 GetArg8Val();
    int16 GetArg16Val();
    int32 GetArg32Val();
    int64 GetArg64Val();
    uint8 GetArgU8Val();
    uint16 GetArgU16Val();
    uint32 GetArgU32Val();
    uint64 GetArgU64Val();

	int64 GetArgLLRange();
	int GetArgRange();
	dword GetArgFlag( dword dwStart, dword dwMask );
    int64 GetArgLLFlag( uint64 iStart, uint64 iMask );

public:
	CScriptKey();
	CScriptKey( tchar * pszKey, tchar * pszArg );
	virtual ~CScriptKey();
private:
	CScriptKey(const CScriptKey& copy);
	CScriptKey& operator=(const CScriptKey& other);
};

class CScriptKeyAlloc : public CScriptKey
{
	// Dynamic allocated script key.
protected:
	CSMemLenBlock m_Mem;	// the buffer to hold data read.

protected:
	tchar * GetKeyBufferRaw( size_t iSize );
	size_t ParseKeyEnd();

public:
	static const char *m_sClassName;
	tchar * GetKeyBuffer();
	bool ParseKey(lpctstr pszKey, lpctstr pszArgs);
	bool ParseKey(lpctstr pszKey);
	void ParseKeyLate();

public:
	CScriptKeyAlloc() { }
	virtual ~CScriptKeyAlloc() { }

private:
	CScriptKeyAlloc(const CScriptKeyAlloc& copy);
	CScriptKeyAlloc& operator=(const CScriptKeyAlloc& other);
};

#ifdef _NOSCRIPTCACHE
 #define PhysicalScriptFile CSFileText
#else
 #define PhysicalScriptFile CacheableScriptFile
#endif

class CScript : public PhysicalScriptFile, public CScriptKeyAlloc
{
private:
	bool m_fSectionHead;	// Does the File Offset point to current section header? [HEADER]
	size_t  m_pSectionData;	// File Offset to current section data, under section header.

public:
	static const char *m_sClassName;
	int m_iLineNum;					// for debug purposes if there is an error.
	size_t	m_iResourceFileIndex;	// index in g_Cfg.m_ResourceFiles of the CResourceScript (script file) where the CScript originated
protected:
	void InitBase();
	virtual size_t Seek( size_t offset = 0, int iOrigin = SEEK_SET );

public:
	// text only functions:
	virtual bool ReadTextLine( bool fRemoveBlanks );	// looking for a section or reading strangly formated section.
	bool FindTextHeader( lpctstr pszName ); // Find a section in the current script

public:
	virtual bool Open( lpctstr szFilename = NULL, uint Flags = OF_READ|OF_TEXT );
	virtual void Close();
	virtual void CloseForce();
	bool SeekContext( CScriptLineContext LineContext );
	CScriptLineContext GetContext() const;

	// Find sections.
	bool FindNextSection();
	virtual bool FindSection( lpctstr pszName, uint uModeFlags ); // Find a section in the current script
	lpctstr GetSection() const;
	bool IsSectionType( lpctstr pszName );
	// Find specific keys in the current section.
	bool FindKey( lpctstr pszName ); // Find a key in the current section
	// read the sections keys.
	bool ReadKey( bool fRemoveBlanks = true );
	bool ReadKeyParse();

	// Write stuff out to a script file.
	bool _cdecl WriteSection( lpctstr pszSection, ... ) __printfargs(2,3);
	bool WriteKey( lpctstr pszKey, lpctstr pszVal );
	void _cdecl WriteKeyFormat( lpctstr pszKey, lpctstr pszFormat, ... ) __printfargs(3,4);

	void WriteKeyVal( lpctstr pszKey, int64 dwVal );
	void WriteKeyHex( lpctstr pszKey, int64 dwVal );

	CScript();
	CScript( lpctstr pszKey );
	CScript( lpctstr pszKey, lpctstr pszVal );
	virtual ~CScript();
};

#endif // CSCRIPT_H
