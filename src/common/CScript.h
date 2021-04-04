/**
* @file CScript.h
* @brief Script entity.
*/

#ifndef _INC_CSCRIPT_H
#define _INC_CSCRIPT_H

#include "sphere_library/CSMemBlock.h"
#include "CScriptContexts.h"
#include "CCacheableScriptFile.h"


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
		return GetArgStr(nullptr);
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
	CScriptKey( tchar * ptcKey, tchar * ptcArg );
	virtual ~CScriptKey() = default;
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
	tchar * _GetKeyBufferRaw( size_t iSize );
    //tchar * GetKeyBufferRaw( size_t iSize );
	size_t ParseKeyEnd();

public:
	static const char *m_sClassName;
	tchar * GetKeyBuffer();
	bool ParseKey(lpctstr ptcKey, lpctstr pszArgs);
	bool ParseKey(lpctstr ptcKey);
	void ParseKeyLate();

public:
	CScriptKeyAlloc() = default;
	virtual ~CScriptKeyAlloc() = default;

private:
	CScriptKeyAlloc(const CScriptKeyAlloc& copy);
	CScriptKeyAlloc& operator=(const CScriptKeyAlloc& other);
};


class CResourceLock;

class CScript : public CCacheableScriptFile, public CScriptKeyAlloc
{
private:
	std::string	_sWriteBuffer_num;
	std::string _sWriteBuffer_keyVal;
	bool m_fSectionHead;	// Does the File Offset point to current section header? [HEADER]
	int  m_iSectionData;	// File Offset to current section data, under section header.

public:
	static const char *m_sClassName;

	enum class ParseFlags
	{
		None,
		IgnoreInvalidRef
	} _eParseFlags;

	int m_iLineNum;				// for debug purposes if there is an error.
	int	m_iResourceFileIndex;	// index in g_Cfg.m_ResourceFiles of the CResourceScript (script file) where the CScript originated

protected:
    bool _fCacheToBeUpdated;

protected:
	void _InitBase();

	// text only functions:
    friend class CResourceLock;
protected:  virtual bool _ReadTextLine( bool fRemoveBlanks );	// looking for a section or reading strangly formated section.
public:     virtual bool ReadTextLine( bool fRemoveBlanks );
public:     bool FindTextHeader( lpctstr pszName ); // Find a section in the current script

private:    virtual bool _Open( lpctstr ptcFilename = nullptr, uint uiFlags = OF_READ|OF_TEXT ) override;
public:     virtual bool Open( lpctstr ptcFilename = nullptr, uint uiFlags = OF_READ|OF_TEXT ) override;
private:	virtual void _Close() override;
public:     virtual void Close() override;
private:    virtual int _Seek( int iOffset = 0, int iOrigin = SEEK_SET ) override;
public:     virtual int Seek( int iOffset = 0, int iOrigin = SEEK_SET ) override;
private:    bool _SeekContext( CScriptLineContext LineContext );
public:     bool SeekContext( CScriptLineContext LineContext );
private:	CScriptLineContext _GetContext() const;
public:     CScriptLineContext GetContext() const;

public:
    virtual void CloseForce();
	// Find sections.
	bool FindNextSection();
	bool FindSection( lpctstr pszName, uint uiModeFlags ); // Find a section in the current script
	lpctstr GetSection() const;
	bool IsSectionType( lpctstr pszName );
	// Find specific keys in the current section.
	bool FindKey( lpctstr pszName ); // Find a key in the current section
	// read the sections keys.
	bool ReadKey( bool fRemoveBlanks = true );
	bool ReadKeyParse();

	// Write stuff out to a script file.
	bool _cdecl WriteSection(lpctstr pszSection, ...) __printfargs(2,3);
	void _cdecl WriteKeyFormat(lpctstr ptcKey, lpctstr pszFormat, ...) __printfargs(3,4);
	bool WriteKeySingle(lptstr ptcKey);
	bool WriteKeyStr(lpctstr ptcKey, lpctstr ptcVal);

	void WriteKeyVal(lpctstr ptcKey, int64 iVal);
	void WriteKeyHex(lpctstr ptcKey, int64 iVal);

	CScript();
	CScript( lpctstr ptcKey );
	CScript( lpctstr ptcKey, lpctstr pszVal );
	virtual ~CScript() = default;

	void CopyParseState(const CScript& other) noexcept;
};

#endif // CSCRIPT_H
