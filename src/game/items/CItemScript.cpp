
#include "CItemScript.h"
#include "CItemVendable.h"


//////////////////////////////////////
// -CItemScript

CItemScript::CItemScript( ITEMID_TYPE id, CItemBase * pItemDef ) : CItemVendable( id, pItemDef ) {
}

CItemScript::~CItemScript() {
    DeletePrepare();	// Must remove early because virtuals will fail in child destructor.
}

lpctstr const CItemScript::sm_szLoadKeys[] = {
    NULL,
};

lpctstr const CItemScript::sm_szVerbKeys[] = {
    NULL,
};

void CItemScript::r_Write(CScript & s)
{
    ADDTOCALLSTACK_INTENSIVE("CItemScript::r_Write");
    CItemVendable::r_Write(s);
}

bool CItemScript::r_WriteVal(lpctstr pszKey, CString & sVal, CTextConsole *pSrc)
{
    ADDTOCALLSTACK("CItemScript::r_WriteVal");
    return CItemVendable::r_WriteVal(pszKey, sVal, pSrc);
}

bool CItemScript::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CItemScript::r_LoadVal");
    return CItemVendable::r_LoadVal(s);
}

bool CItemScript::r_Verb(CScript & s, CTextConsole *pSrc)
{
    ADDTOCALLSTACK("CItemScript::r_Verb");
    return CItemVendable::r_Verb(s, pSrc);
}

void CItemScript::DupeCopy(const CItem *pItem)
{
    ADDTOCALLSTACK("CItemScript::DupeCopy");
    CItemVendable::DupeCopy(pItem);
}