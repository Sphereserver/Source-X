/**
* @file CacheableScriptFile.h
*
*/

#pragma once
#ifndef _INC_CACHEABLESCRIPTFILE_H
#define _INC_CACHEABLESCRIPTFILE_H

#include <string>
#include "sphere_library/CSFile.h"


class CacheableScriptFile : public CSFileText
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
	virtual tchar *ReadString(tchar *pBuffer, size_t sizemax);
	virtual size_t Seek(size_t offset = 0, int origin = SEEK_SET);
	virtual size_t GetPosition() const;

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
