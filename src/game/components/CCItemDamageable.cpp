
#include "CCItemDamageable.h"
#include "../CObjBase.h"
#include "../CWorld.h"
#include "../chars/CChar.h"
#include "../clients/CClient.h"

CCItemDamageable::CCItemDamageable(CObjBase * pLink) : CComponent(COMP_ITEMDAMAGEABLE, pLink)
{
    _iCurHits = 0;
    _iMaxHits = 0;
    g_World.m_ObjStatusUpdates.Add(pLink);
}

CCItemDamageable::~CCItemDamageable()
{
    g_World.m_ObjStatusUpdates.RemovePtr(GetLink());
}

void CCItemDamageable::SetCurHits(word iCurHits)
{
    if (!g_Serv.IsLoading() && (_iCurHits != iCurHits))
    {
        _fNeedUpdate = true;
    }
    _iCurHits = iCurHits;
}

void CCItemDamageable::SetMaxHits(word iMaxHits)
{
    if (!g_Serv.IsLoading() && (_iMaxHits != iMaxHits))
    {
        _fNeedUpdate = true;
    }
    _iMaxHits = iMaxHits;
}

word CCItemDamageable::GetCurHits()
{
    return _iCurHits;
}

word CCItemDamageable::GetMaxHits()
{
    return _iMaxHits;
}

void CCItemDamageable::OnTickStatsUpdate()
{
    ADDTOCALLSTACK("CCItemDamageable::OnTickStatsUpdate");
    if (!_fNeedUpdate)
    {
        return;
    }
    int64 iCurtime = g_World.GetCurrentTime().GetTimeRaw();

    if (_iTimeLastUpdate + g_Cfg.m_iRegenRate[0] < iCurtime)
    {
        _iTimeLastUpdate = iCurtime;

        CItem *pItem = static_cast<CItem*>(GetLink());
        CWorldSearch AreaChars(GetLink()->GetTopPoint(), UO_MAP_VIEW_SIZE_DEFAULT);
        AreaChars.SetSearchSquare(true);
        CChar *pChar = nullptr;
        for (;;)
        {
            pChar = AreaChars.GetChar();
            if (!pChar)
            {
                break;
            }
            CClient *pClient = pChar->GetClient();
            if (!pClient)
            {
                continue;
            }
            if (!pClient->CanSee(pItem))
            {
                continue;
            }
            pClient->addStatusWindow(pItem);
        }
    }
    _fNeedUpdate = false;
}

void CCItemDamageable::Delete(bool fForced)
{
    UNREFERENCED_PARAMETER(fForced);
}

enum CIDMGL_TYPE
{
    CIDMGL_HITS,
    CIDMGL_MAXHITS,
    CIDMGL_QTY
};

lpctstr const CCItemDamageable::sm_szLoadKeys[CIDMGL_QTY + 1] =
{
    "HITS",
    "MAXHITS",
    NULL
};

bool CCItemDamageable::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CCItemDamageable::r_LoadVal");
    int iKeyNum = FindTableHead(s.GetKey(), sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    if (iKeyNum < 0)
    {
        return false;
    }
    switch (iKeyNum)
    {
        case CIDMGL_HITS:
        {
            SetCurHits(s.GetArgWVal());
            return true;
        }
        case CIDMGL_MAXHITS:
        {
            SetMaxHits(s.GetArgWVal());
            return true;
        }
        default:
            break;
    }
    return false;
}

bool CCItemDamageable::r_WriteVal(lpctstr pszKey, CSString & s, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CCItemDamageable::r_WriteVal");
    UNREFERENCED_PARAMETER(pSrc);
    int iKeyNum = FindTableHead(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    if (iKeyNum < 0)
    {
        return false;
    }
    switch (iKeyNum)
    {
        case CIDMGL_HITS:
        {
            s.FormatWVal(GetCurHits());
            return true;
        }
        case CIDMGL_MAXHITS:
        {
            s.FormatWVal(GetMaxHits());
            return true;
        }
        default:
            break;
    }
    return false;
}

void CCItemDamageable::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CCItemDamageable::r_Write");
    word iCur = GetCurHits();
    word iMax = GetMaxHits();
    if (iCur > 0)
    {
        s.WriteKeyVal("HITS", iCur);
    }
    if (iMax > 0)
    {
        s.WriteKeyVal("MAXHITS", iMax);
    }
}

bool CCItemDamageable::r_GetRef(lpctstr & pszKey, CScriptObj *& pRef)
{
    UNREFERENCED_PARAMETER(pszKey);
    UNREFERENCED_PARAMETER(pRef);
    return false;
}

bool CCItemDamageable::r_Verb(CScript & s, CTextConsole * pSrc)
{
    UNREFERENCED_PARAMETER(s);
    UNREFERENCED_PARAMETER(pSrc);
    return false;
}

void CCItemDamageable::Copy(CComponent * target)
{
    ADDTOCALLSTACK("CCItemDamageable::Copy");
    CCItemDamageable *pTarget = static_cast<CCItemDamageable*>(target);
    if (!pTarget)
    {
        return;
    }
    SetCurHits(pTarget->GetCurHits());
    SetMaxHits(pTarget->GetMaxHits());
}

CCRET_TYPE CCItemDamageable::OnTick()
{
    ADDTOCALLSTACK("CCItemDamageable::OnTick");
    return CCRET_CONTINUE;  // Skip code here, OnTick is done separatelly from OnTickStatsUpdate
}