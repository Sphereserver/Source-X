/**
* @file CScript.h
*
*/

#pragma once
#ifndef CSCRIPT_H
#define CSCRIPT_H

#include "common.h"
#include "CMemBlock.h"
#include "CacheableScriptFile.h"

#define GRAY_SCRIPT		".scp"

#define SCRIPT_MAX_SECTION_LEN 128


struct CScriptLineContext
{
public:
	int m_lOffset;
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
	TCHAR * m_pszKey;		// The key. (or just start of the line)
	TCHAR * m_pszArg;		// for parsing the last read line.	KEY=ARG or KEY ARG

public:
	static const char *m_sClassName;
	bool IsKey( LPCTSTR pszName ) const;
	bool IsKeyHead( LPCTSTR pszName, size_t len ) const;
	void InitKey();

	// Query the key.
	LPCTSTR GetKey() const;
	// Args passed with the key.
	bool HasArgs() const;
	TCHAR * GetArgRaw() const; // Not need to parse at all.
	TCHAR * GetArgStr( bool * fQuoted );	// this could be a quoted string ?
	TCHAR * GetArgStr();
	long long GetArgLLVal();
	int GetArgVal();
	int GetArgRange();
	DWORD GetArgFlag( DWORD dwStart, DWORD dwMask );

public:
	CScriptKey();
	CScriptKey( TCHAR * pszKey, TCHAR * pszArg );
	virtual ~CScriptKey();
private:
	CScriptKey(const CScriptKey& copy);
	CScriptKey& operator=(const CScriptKey& other);
};

class CScriptKeyAlloc : public CScriptKey
{
	// Dynamic allocated script key.
protected:
	CMemLenBlock m_Mem;	// the buffer to hold data read.

protected:
	TCHAR * GetKeyBufferRaw( size_t iSize );

	bool ParseKey( LPCTSTR pszKey, LPCTSTR pszArgs );
	size_t ParseKeyEnd();

public:
	static const char *m_sClassName;
	TCHAR * GetKeyBuffer();
	bool ParseKey( LPCTSTR pszKey );
	void ParseKeyLate();

public:
	CScriptKeyAlloc() { }
	virtual ~CScriptKeyAlloc() { }

private:
	CScriptKeyAlloc(const CScriptKeyAlloc& copy);
	CScriptKeyAlloc& operator=(const CScriptKeyAlloc& other);
};

#ifdef _NOSCRIPTCACHE
 #define PhysicalScriptFile CFileText
#else
 #define PhysicalScriptFile CacheableScriptFile
#endif

class CScript : public PhysicalScriptFile, public CScriptKeyAlloc
{
private:
	bool m_fSectionHead;	// File Offset to current section header. [HEADER]
	int  m_lSectionData;	// File Offset to current section data, under section header.

public:
	static const char *m_sClassName;
	int m_iLineNum;		// for debug purposes if there is an error.
protected:
	void InitBase();

	virtual DWORD Seek( int offset = 0, UINT origin = SEEK_SET );

public:
	// text only functions:
	virtual bool ReadTextLine( bool fRemoveBlanks );	// looking for a section or reading strangly formated section.
	bool FindTextHeader( LPCTSTR pszName ); // Find a section in the current script

public:
	virtual bool Open( LPCTSTR szFilename = NULL, UINT Flags = OF_READ|OF_TEXT );
	virtual void Close();
	virtual void CloseForce();
	bool SeekContext( CScriptLineContext LineContext );
	CScriptLineContext GetContext() const;

	// Find sections.
	bool FindNextSection();
	virtual bool FindSection( LPCTSTR pszName, UINT uModeFlags ); // Find a section in the current script
	LPCTSTR GetSection() const;
	bool IsSectionType( LPCTSTR pszName );
	// Find specific keys in the current section.
	bool FindKey( LPCTSTR pszName ); // Find a key in the current section
	// read the sections keys.
	bool ReadKey( bool fRemoveBlanks = true );
	bool ReadKeyParse();

	// Write stuff out to a script file.
	bool _cdecl WriteSection( LPCTSTR pszSection, ... ) __printfargs(2,3);
	bool WriteKey( LPCTSTR pszKey, LPCTSTR pszVal );
	void _cdecl WriteKeyFormat( LPCTSTR pszKey, LPCTSTR pszFormat, ... ) __printfargs(3,4);

	void WriteKeyVal( LPCTSTR pszKey, INT64 dwVal );
	void WriteKeyHex( LPCTSTR pszKey, INT64 dwVal );

	CScript();
	CScript( LPCTSTR pszKey );
	CScript( LPCTSTR pszKey, LPCTSTR pszVal );
	virtual ~CScript();
};

#endif // CSCRIPT_H
