/**
* @file CCacheableScriptFile.h
*
*/

#ifndef _INC_CACHEABLESCRIPTFILE_H
#define _INC_CACHEABLESCRIPTFILE_H

#include <string>
#include <vector>
#include "sphere_library/CSFileText.h"


class CCacheableScriptFile : public CSFileText
{
public:
	CCacheableScriptFile();
	~CCacheableScriptFile();
private:
	CCacheableScriptFile(const CCacheableScriptFile& copy);
	CCacheableScriptFile& operator=(const CCacheableScriptFile& other);

protected:  virtual bool _Open(lpctstr ptcFilename = nullptr, uint uiModeFlags = OF_READ|OF_SHARE_DENY_NONE) override;
public:     virtual bool Open(lpctstr ptcFilename = nullptr, uint uiModeFlags = OF_READ|OF_SHARE_DENY_NONE) override;
protected:  virtual void _Close() override;
public:     virtual void Close() override;
            virtual bool _IsFileOpen() const override;
            virtual bool IsFileOpen() const override;
protected:  virtual int _Seek(int iOffset = 0, int iOrigin = SEEK_SET) override;
public:     virtual int Seek(int iOffset = 0, int iOrigin = SEEK_SET) override;

protected:  virtual bool _IsEOF() const override;
public:     virtual bool IsEOF() const override;
protected:  virtual int _GetPosition() const override;
public:     virtual int GetPosition() const override;

protected:  virtual tchar * _ReadString(tchar *pBuffer, int sizemax) override;
public:     virtual tchar * ReadString(tchar *pBuffer, int sizemax) override;

protected: 
    void _dupeFrom(CCacheableScriptFile *other);
    void dupeFrom(CCacheableScriptFile *other);
    
protected:  bool _HasCache() const;
public:     bool HasCache() const;

public:
	bool _fClosed;
	bool _fRealFile;
	int _iCurrentLine;

protected:
	std::vector<std::string>* _fileContent; // It's better to have a pointer so that CResourceLock can access to this

private:    bool _useDefaultFile() const;
//public:     bool useDefaultFile() const;
};

#endif // _INC_CACHEABLESCRIPTFILE_H
