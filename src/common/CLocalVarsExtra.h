/**
* @file CLocalVarsExtra.h
* @brief Additional script variable types.
*/

#ifndef _INC_CLOCALVARSEXTRA_H
#define _INC_CLOCALVARSEXTRA_H

#include <flat_containers/flat_map.hpp>
#include "sphere_library/CSString.h"


class CLocalFloatVars
{
	struct LexNoCaseLess
	{
		bool operator()(const CSString& str, const char* pBeg2) const;
	};
	using MapType = fc::vector_map<CSString, realtype, LexNoCaseLess>;

	MapType m_VarMap;

public:
	CLocalFloatVars() = default;
	~CLocalFloatVars() = default;

    CLocalFloatVars(const CLocalFloatVars& copy) = delete;
    CLocalFloatVars& operator=(const CLocalFloatVars& other) = delete;

private: //setting, getting
	bool Set( const char* VarName, const char* VarValue );
	realtype GetVal( const char* VarName );

public: //setting, getting
	bool Insert( const char* VarName, const char* VarValue, bool fForceSet = false);
	CSString Get( const char* VarName );
	void Clear();
};


class CObjBase;

class CLocalObjMap
{
	using ObjMap = fc::vector_map<ushort, CObjBase*>;
	ObjMap m_ObjMap;

public:
	CLocalObjMap() = default;
	~CLocalObjMap() = default;
private:
	CLocalObjMap(const CLocalObjMap& copy);
	CLocalObjMap& operator=(const CLocalObjMap& other);

public:
	CObjBase * Get( ushort Number );
	bool Insert( ushort Number, CObjBase * pObj, bool fForceSet = false );
	void Clear();
};

#endif // _INC_CLOCALVARSEXTRA_H
