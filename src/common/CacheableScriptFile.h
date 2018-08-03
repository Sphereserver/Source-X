/**
* @file CacheableScriptFile.h
*
*/

#ifndef _INC_CACHEABLESCRIPTFILE_H
#define _INC_CACHEABLESCRIPTFILE_H

#include <string>
#include <vector>
#include "sphere_library/CSFileText.h"


class CacheableScriptFile : public CSFileText
{
protected:
	virtual bool OpenBase();
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
	virtual tchar * ReadString(tchar *pBuffer, int sizemax);
	virtual int Seek(int offset = 0, int origin = SEEK_SET);
	virtual int GetPosition() const;

private:
	bool m_closed;
	bool m_realFile;
	int m_currentLine;

protected:
	std::vector<std::string> m_fileContent;

private:
	bool useDefaultFile() const;
};

#endif // _INC_CACHEABLESCRIPTFILE_H
