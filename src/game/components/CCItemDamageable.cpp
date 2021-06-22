
#include "CCItemDamageable.h"
#include "../CServer.h"
#include "../CObjBase.h"
#include "../CWorldGameTime.h"
#include "../CWorldMap.h"
#include "../CWorldTickingList.h"
#include "../chars/CChar.h"
#include "../clients/CClient.h"


CCItemDamageable::CCItemDamageable(CItem * pLink) : CComponent(COMP_ITEMDAMAGEABLE)
{
    _pLink = pLink;
    _iCurHits = 0;
    _iMaxHits = 0;
    _iTimeLastUpdate = 0;
    _fNeedUpdate = true;
}

CCItemDamageable::~CCItemDamageable()
{
}

CItem * CCItemDamageable::GetLink() const
{
    return _pLink;
}

bool CCItemDamageable::CanSubscribe(const CItem* pItem) // static
{
    return pItem->Can(CAN_I_DAMAGEABLE);
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

word CCItemDamageable::GetCurHits() const
{
    return _iCurHits;
}

word CCItemDamageable::GetMaxHits() const
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
    const int64 iCurtime = CWorldGameTime::GetCurrentTime().GetTimeRaw();

    if (_iTimeLastUpdate + g_Cfg._iItemHitpointsUpdate < iCurtime)
    {
        _iTimeLastUpdate = iCurtime;

        CItem *pItem = static_cast<CItem*>(GetLink());
        CWorldSearch AreaChars(pItem->GetTopPoint(), UO_MAP_VIEW_SIZE_DEFAULT);
        AreaChars.SetSearchSquare(true);
        CChar *pChar = nullptr;
        for (;;)
        {
            pChar = AreaChars.GetChar();
            if (!pChar)
            {
                break;
            }
            CClient *pClient = pChar->GetClientActive();
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
    nullptr
};

bool CCItemDamageable::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CCItemDamageable::r_LoadVal");
    int iKeyNum = FindTableSorted(s.GetKey(), sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
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

bool CCItemDamageable::r_WriteVal(lpctstr ptcKey, CSString & s, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CCItemDamageable::r_WriteVal");
    UNREFERENCED_PARAMETER(pSrc);
    int iKeyNum = FindTableSorted(ptcKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
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
    const word iCur = GetCurHits();
    const word iMax = GetMaxHits();
    if (iCur > 0)
    {
        s.WriteKeyVal("HITS", iCur);
    }
    if (iMax > 0)
    {
        s.WriteKeyVal("MAXHITS", iMax);
    }
}

bool CCItemDamageable::r_GetRef(lpctstr & ptcKey, CScriptObj *& pRef)
{
    UNREFERENCED_PARAMETER(ptcKey);
    UNREFERENCED_PARAMETER(pRef);
    return false;
}

bool CCItemDamageable::r_Verb(CScript & s, CTextConsole * pSrc)
{
    UNREFERENCED_PARAMETER(s);
    UNREFERENCED_PARAMETER(pSrc);
    return false;
}

void CCItemDamageable::Copy(const CComponent * target)
{
    ADDTOCALLSTACK("CCItemDamageable::Copy");
    const CCItemDamageable *pTarget = static_cast<const CCItemDamageable*>(target);
    if (!pTarget)
    {
        return;
    }
    SetCurHits(pTarget->GetCurHits());
    SetMaxHits(pTarget->GetMaxHits());
}

CCRET_TYPE CCItemDamageable::OnTickComponent()
{
    ADDTOCALLSTACK("CCItemDamageable::_OnTick");
    return CCRET_CONTINUE;  // Skip code here, _OnTick is done separately from OnTickStatsUpdate
}