/**
* @file CacheableScriptFile.h
*
*/

#pragma once
#ifndef _INC_CACHEABLESCRIPTFILE_H
#define _INC_CACHEABLESCRIPTFILE_H

#include "CFile.h"
#include <string>


class CacheableScriptFile : public CFileText
{
protected:
	virtual bool OpenBase(void *pExtra);
	virtual void CloseBase();
	void dupeFrom(CacheableScriptFile *other);

public:
	CacheableScriptFile();
	~CacheableScriptFile();
private:
	CacheableScriptFile(const CacheableScriptFile& copy);
	CacheableScriptFile& operator=(const CacheableScriptFile& other);

public:
	virtual bool IsFileOpen() const;
	virtual bool IsEOF() const;
	virtual TCHAR *ReadString(TCHAR *pBuffer, size_t sizemax);
	virtual dword Seek(int offset = 0, UINT origin = SEEK_SET);
	virtual dword GetPosition() const;

private:
	bool m_closed;
	bool m_realFile;
	size_t m_currentLine;

protected:
	std::vector<std::string> * m_fileContent;

private:
	bool useDefaultFile() const;
};

#endif // _INC_CACHEABLESCRIPTFILE_H
