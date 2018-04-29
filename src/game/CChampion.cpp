

#include "CChampion.h"	// predef header.
#include "chars/CChar.h"
#include "../common/CScript.h"
#include "../common/CResourceBase.h"
#include "../common/CException.h"

/*
* @file Champion.cpp
*
* @brief This file contains CChampion and CChampionDef classes declarations
*
* CChampion class manages a CItem making it to work as a Champion
* class is declared inside CObjBase.h
*
* CChampionDef class is used to store all the data placed in scripts under the [CHAMPION ] resource
* this data is read by CChampion to know Champion's type, npcs to spawn at each level, boss id....
* class is declared inside CResource.h
*/

CChampion::CChampion(CItem *pLink) : CComponent(COMP_CHAMPION, pLink)
{
    ADDTOCALLSTACK("CChampion::CChampion");
    g_Log.EventDebug("Constructor 1\n");
    _pLink = pLink;
    _fActive = false;
    _iLevel = 0;
    _iSpawnsCur = 0;
    _iDeathCount = 0;
    _iSpawnsNextWhite = 0;
    _iSpawnsNextRed = 0;
    _iCandlesNextRed = 0;
    _iCandlesNextLevel = 0;
    _iLastActivationTime = 0;

    CChampionDef *pChampDef = static_cast<CChampionDef*>(g_Cfg.ResourceGetDef(CResourceID(RES_CHAMPION, _pLink->m_itSpawnChar.m_CharID)));
    _iLevelMax = pChampDef->_iLevelMax;
    _iSpawnsMax = pChampDef->_iSpawnsMax;
    _idChampion = pChampDef->_idChampion;
    _iSpawn.insert(pChampDef->_iSpawn.begin(), pChampDef->_iSpawn.end());
};

CChampion::~CChampion()
{
    ADDTOCALLSTACK("CChampion::~CChampion");;
    Stop();
};

bool CChampion::OnTick()
{
    ADDTOCALLSTACK("CChampion::OnTick");
    if (_pRedCandles.size() > 0)
        DelRedCandle();
    else
        Stop();
    _pLink->SetTimeout(TICK_PER_SEC * 60 * 10);	//10 minutes
    return true;
};

void CChampion::Init()
{
    ADDTOCALLSTACK("CChampion::Init");
    // TODO store light in the area
    g_Log.EventDebug("CChampion::Init");
    _fActive = true;
    _iLastActivationTime = g_World.GetCurrentTime().GetTimeRaw();
    SetLevel(1);
    for (unsigned char i = 0; i < _iSpawnsNextWhite; i++)   // Spawn all the monsters required to get the next White candle. // TODO: a way to prevent this from script.
        SpawnNPC();
};

void CChampion::Stop()
{
    ADDTOCALLSTACK("CChampion::Stop");
    KillChildren();
    _fActive = false;
    _iSpawnsCur = 0;
    _iDeathCount = 0;
    _iLevel = 0;
    _iCandlesNextRed = 0;
    _iCandlesNextLevel = 0;
    _pLink->SetTimeout((_iLastActivationTime - g_World.GetCurrentTime().GetTimeRaw()) / TICK_PER_SEC);
};

void CChampion::Complete()
{
    ADDTOCALLSTACK("CChampion::Complete");
    Stop();	//Cleaning everything, just to be sure.
    // TODO: Add rewards, titles, etc
};

void CChampion::OnKill()
{
    ADDTOCALLSTACK("CChampion::OnKill");
    if (_iSpawnsNextWhite == 0)
        AddWhiteCandle();
    else
        SpawnNPC();
}
void CChampion::SpawnNPC()
{
    ADDTOCALLSTACK("CChampion::SpawnNPC");

    if (_iLevel == _iLevelMax)	// Already summoned the Champion, stop
        return;

    CResourceIDBase	rid = _pLink->m_itSpawnChar.m_CharID;
    g_Log.EventDebug("SpawnNPC = %d\n", rid);
    CREID_TYPE pNpc = CREID_INVALID;
    if (_pRedCandles.size() == _iLevelMax && _iSpawnsNextWhite == 0) // Reached 16th red candle and spawned all the normal npcs, so next one should be the champion
    {
        pNpc = _idChampion;
    }
    else
    {
        _iSpawnsNextWhite--;
        if (rid.GetResType() == RES_CHAMPION)
            g_Log.EventDebug("champion!!!!!!!!! = %d - 0%lx - 0%lx\n", rid.GetResIndex(), rid.GetObjUID(), rid.GetPrivateUID());
        else
            g_Log.EventDebug("type = %d - %d - 0%lx - 0%lx\n", rid.GetResType(), rid.GetResIndex(), rid.GetObjUID(), rid.GetPrivateUID());
        CResourceDef * pDef = g_Cfg.ResourceGetDef(rid);
        if (!pDef)
        {
            g_Log.EventDebug("nanai1\n");
            return;
        }
        CChampionDef * pChamp = static_cast<CChampionDef*>(pDef);
        if (!pChamp)
        {
            g_Log.EventDebug("nanai2\n");
            return;
        }
        uchar iLevel = _iLevel;
        uchar iRand = (uchar)Calc_GetRandVal2(1, _iLevelMax);
        while (iRand > 0 && !_iSpawn[iLevel][iRand])
            iRand--;	//Decrease iRand to 1 in case not all spots are filled
        pNpc = _iSpawn[iLevel][iRand];	//Find out the random npc.
        g_Log.EventDebug("pNpc = %s", rid.CharFind()->GetName());
        if (!pNpc)	// At least one NPC per level should be added, check just in case.
            return;
    }
    CChar * pChar = CChar::CreateBasic(pNpc);
    if (!pChar)
        return;
    pChar->SetTopPoint(_pLink->GetTopPoint());
    pChar->MoveNear(_pLink->GetTopPoint(), _pLink->m_itNormal.m_morep.m_z);
    AddObj(pChar->GetUID());

};

void CChampion::AddWhiteCandle(CUID uid)
{
    ADDTOCALLSTACK("CChampion::AddWhiteCandle");
    //TODO Trigger @AddWhiteCandle(uid)
    if (_pWhiteCandles.size() == 4)
    {
        AddRedCandle();
        return;
    }
    else
        _iSpawnsNextWhite = _iSpawnsNextRed / 5;

    CItem * pCandle = NULL;
    if (uid != static_cast<CUID>(UID_UNUSED))
        pCandle = uid.ItemFind();

    if (!pCandle)
    {
        pCandle = _pLink->CreateBase(ITEMID_SKULL_CANDLE);
        if (!pCandle)
            return;
        CPointMap pt = _pLink->GetTopPoint();
        switch (_pWhiteCandles.size())
        {
            case 1:
                pt.MoveN(DIR_SW, 1);
                break;
            case 2:
                pt.MoveN(DIR_SE, 1);
                break;
            case 3:
                pt.MoveN(DIR_NW, 1);
                break;
            case 4:
                pt.MoveN(DIR_NE, 1);
                break;
            default:
                break;
        }
        pCandle->SetAttr(ATTR_MOVE_NEVER);
        pCandle->MoveTo(pt);
        pCandle->SetTopZ(pCandle->GetTopZ() + 4);
        pCandle->Update();
        pCandle->GenerateScript(NULL);
    }
    _pWhiteCandles.push_back(pCandle);
    pCandle->m_uidLink = _pLink->GetUID();	// Link it to the champion, so if it gets removed the candle will be removed too
};

void CChampion::AddRedCandle(CUID uid)
{
    ADDTOCALLSTACK("CChampion::AddRedCandle");
    CItem * pCandle = NULL;
    if (uid != static_cast<CUID>(UID_UNUSED))
        pCandle = uid.ItemFind();
    //TODO Trigger @AddRedCandle (pCandle)

    if (_pRedCandles.size() >= 14 && _iLevel < 4)
        SetLevel(4);
    else if (_pRedCandles.size() >= 10 && _iLevel < 3)
        SetLevel(3);
    else if (_pRedCandles.size() >= 6 && _iLevel < 2)
        SetLevel(2);
    _iSpawnsNextWhite = _iSpawnsNextRed / 5;
    if (!g_Serv.IsLoading())	// White candles may be created before red ones when restoring items from worldsave we must not remove them.
        ClearWhiteCandles();

    if (!pCandle)
    {
        pCandle = _pLink->CreateBase(ITEMID_SKULL_CANDLE);
        if (!pCandle)
            return;
        CPointMap pt = _pLink->GetTopPoint();
        switch (_pRedCandles.size())
        {
            case 1:
                pt.MoveN(DIR_NW, 2);
                break;
            case 2:
                pt.MoveN(DIR_N, 2);
                pt.MoveN(DIR_W, 1);
                break;
            case 3:
                pt.MoveN(DIR_N, 2);
                break;
            case 4:
                pt.MoveN(DIR_N, 2);
                pt.MoveN(DIR_E, 1);
                break;
            case 5:
                pt.MoveN(DIR_NE, 2);
                break;
            case 6:
                pt.MoveN(DIR_E, 2);
                pt.MoveN(DIR_N, 1);
                break;
            case 7:
                pt.MoveN(DIR_E, 2);
                break;
            case 8:
                pt.MoveN(DIR_E, 2);
                pt.MoveN(DIR_S, 1);
                break;
            case 9:
                pt.MoveN(DIR_SE, 2);
                break;
            case 10:
                pt.MoveN(DIR_S, 2);
                pt.MoveN(DIR_E, 1);
                break;
            case 11:
                pt.MoveN(DIR_S, 2);
                break;
            case 12:
                pt.MoveN(DIR_S, 2);
                pt.MoveN(DIR_W, 1);
                break;
            case 13:
                pt.MoveN(DIR_SW, 2);
                break;
            case 14:
                pt.MoveN(DIR_W, 2);
                pt.MoveN(DIR_S, 1);
                break;
            case 15:
                pt.MoveN(DIR_W, 2);
                break;
            case 16:
                pt.MoveN(DIR_W, 2);
                pt.MoveN(DIR_N, 1);
                break;
            default:
                break;
        }
        pCandle->SetAttr(ATTR_MOVE_NEVER);
        pCandle->MoveTo(pt);
        pCandle->SetTopZ(pCandle->GetTopZ() + 4);
        pCandle->SetHue(static_cast<HUE_TYPE>(33));
        pCandle->Update();
        pCandle->GenerateScript(NULL);
    }
    _pRedCandles.push_back(pCandle);
    pCandle->m_uidLink = _pLink->GetUID();	// Link it to the champion, so if it gets removed the candle will be removed too

};

void CChampion::SetLevel(byte iLevel)
{
    ADDTOCALLSTACK("CChampion::SetLevel");
    if (iLevel > 4)
        iLevel = 4;
    else if (iLevel < 1)
        iLevel = 1;
    ushort iLevelMonsters = GetMonstersPerLevel(_iSpawnsMax);
    _pLink->m_itNormal.m_morep.m_x = GetCandlesPerLevel();
    ushort iRedMonsters = iLevelMonsters / _pLink->m_itNormal.m_morep.m_x;
    ushort iWhiteMonsters = iRedMonsters / 5;

    // TODO: Trigger @Level (old level, new level, GetMonstersPerLevel, _iCandlesNextLevel)
    // TODO: As the level increases, the light on the area decreases.

    _iLevel = iLevel;
    _iSpawnsNextWhite = iWhiteMonsters;
    _iSpawnsNextRed = iRedMonsters;
    _pLink->SetTimeout(TICK_PER_SEC * 60 * 10);	//10 minutes
};

byte CChampion::GetCandlesPerLevel()
{
    ADDTOCALLSTACK("CChampion::GetCandlesPerLevel");
    switch (_iLevel)
    {
        case 4:
            return 2;
        case 3:
            return 4;
        case 2:
            return 4;
        default:
        case 1:
            return 6;
    }
    return 2;
};

ushort CChampion::GetMonstersPerLevel(ushort iMonsters)
{
    ADDTOCALLSTACK("CChampion::GetMonstersPerLevel");
    switch (_iLevel)
    {
        case 4:
            return (7 * iMonsters) / 100;
        case 3:
            return (13 * iMonsters) / 100;
        case 2:
            return (27 * iMonsters) / 100;
        default:
        case 1:
            return (53 * iMonsters) / 100;
    }
};

// Delete the last created white candle.
void CChampion::DelWhiteCandle()
{
    ADDTOCALLSTACK("CChampion::DelWhiteCandle");
    if (_pWhiteCandles.size() == 0)
        return;

    //TODO Trigger @DelWhiteCandle

    CItem * pCandle = _pWhiteCandles[_pWhiteCandles.size()];
    if (pCandle)
    {
        pCandle->Delete();
    }
    return;
};

// Delete the last created red candle.
void CChampion::DelRedCandle()
{
    ADDTOCALLSTACK("CChampion::DelRedCandle");
    if (_pRedCandles.size() == 0)
        return;

    //TODO Trigger @DelRedCandle

    CItem * pCandle = _pRedCandles[_pRedCandles.size()];
    if (pCandle)
    {
        pCandle->Delete();
    }
};

// Clear all white candles.
void CChampion::ClearWhiteCandles()
{
    ADDTOCALLSTACK("CChampion::ClearWhiteCandles");
    if (_pWhiteCandles.size() == 0)
        return;
    _pWhiteCandles.clear();
};

// Clear all red candles.
void CChampion::ClearRedCandles()
{
    ADDTOCALLSTACK("CChampion::ClearRedCandles");
    if (_pRedCandles.size() == 0)
        return;
    _pRedCandles.clear();
};


// kill everything spawned from this spawn !
void CChampion::KillChildren()
{
    ADDTOCALLSTACK("CSpawn:KillChildren");
    CSpawn *pSpawn = static_cast<CSpawn*>(_pLink->GetComponent(COMP_SPAWN));
    if (pSpawn)
    {
        pSpawn->KillChildren();
    }
}

// Deleting one object from Spawn's memory, reallocating memory automatically.
void CChampion::DelObj(CUID uid)
{
    ADDTOCALLSTACK("CSpawn:DelObj");
    if (!uid.IsValidUID())
    {
        return;
    }
    CSpawn *pSpawn = static_cast<CSpawn*>(_pLink->GetComponent(COMP_SPAWN));
    ASSERT(pSpawn);
    pSpawn->DelObj(uid);

    CChar *pChar = uid.CharFind();
    if (pChar)
    {
        CScript s("-e_spawn_champion");
        pChar->m_OEvents.r_LoadVal(s, RES_EVENTS);  //removing event from the char.
        if (pChar->Stat_GetAdjusted(STAT_STR) <= 0) // NPC died.
        {
            OnKill();
        }
    }
}

// Storing one UID in Spawn's _pObj[]
void CChampion::AddObj(CUID uid)
{
    ADDTOCALLSTACK("CSpawn:AddObj");
    if (!uid || !uid.CharFind()->m_pNPC)    // Only adding UIDs...
    {
        return;
    }
    if (uid.CharFind()->GetSpawn())	//... which doesn't have a SpawnItem already
    {
        return;
    }
    CSpawn *pSpawn = static_cast<CSpawn*>(_pLink->GetComponent(COMP_SPAWN));
    ASSERT(pSpawn);
    if (pSpawn->GetCurrentSpawned() < _iSpawnsMax)
    {
        pSpawn->AddObj(uid);
    }

    CChar *pChar = uid.CharFind();
    if (pChar)
    {
        CScript s("+e_spawn_champion");
        uid.CharFind()->m_OEvents.r_LoadVal(s, RES_EVENTS);	//adding event to the char
    }
}
// Returns the monster's 'level' according to the champion's level (red candles).
/*byte CChampion::GetPercentMonsters()
{
    switch (_iLevel)
    {
        default:
        case 1:
            return 38;
        case 2:
        case 3:
            return 25;
        case 4:
            return 12;
    }
};*/

// Returns the percentaje of monsters killed.
/*byte CChampion::GetCompletionMonsters()
{
    byte iPercent = (m_iDeathCount * 100) / _iSpawnsMax;

    if (iPercent <= 53)
        return 1;
    else if ( iPercent <= 80 ) // 53 + 27
        return 2;
    else if ( iPercent <= 93 ) // 53 + 27 + 13
        return 3;
    else if ( iPercent <= 100 )
        return 4;
    return 1;
};*/

enum ICHMPL_TYPE
{
    ICHMPL_DEATHCOUNT,
    ICHMPL_LASTACTIVATIONTIME,
    ICHMPL_LEVEL,
    ICHMPL_LEVELMAX,
    ICHMPL_REDCANDLES,
    ICHMPL_SPAWNSCUR,
    ICHMPL_SPAWNSMAX,
    ICHMPL_WHITECANDLES,
    ICHMPL_QTY
};

lpctstr const CChampion::sm_szLoadKeys[ICHMPL_QTY + 1] =
{
    "DEATHCOUNT",
    "LASTACTIVATIONTIME",
    "LEVEL",
    "LEVELMAX",
    "REDCANDLES",
    "SPAWNSCUR",
    "SPAWNSMAX",
    "WHITECANDLES",
    NULL
};

enum ICHMPV_TYPE
{
    ICHMPV_ADDOBJ,
    ICHMPV_ADDREDCANDLE,
    ICHMPV_ADDSPAWN,
    ICHMPV_ADDWHITECANDLE,
    ICHMPV_DELOBJ,
    ICHMPV_DELREDCANDLE,
    ICHMPV_DELWHITECANDLE,
    ICHMPV_INIT,
    ICHMPV_MULTICREATE,
    ICHMPV_STOP,
    ICHMPV_QTY
};


lpctstr const CChampion::sm_szVerbKeys[ICHMPV_QTY + 1] =
{
    "ADDOBJ",
    "ADDREDCANDLE",
    "ADDSPAWN",
    "ADDWHITECANDLE",
    "DELOBJ",
    "DELREDCANDLE",
    "DELWHITECANDLE",
    "INIT",
    "MULTICREATE",
    "STOP",
    NULL
};

void CChampion::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CChampion::r_Write");
    //_pLink->r_Write(s);
    /*if (_pLink->m_pRegion)    // FIXME: ECS link with CItemMulti.
    {
        m_pRegion->r_WriteBody( s, "REGION." );
    }*/
    s.WriteKeyVal("DEATHCOUNT", _iDeathCount);
    s.WriteKeyVal("SPAWNSCUR", _iSpawnsCur);
    s.WriteKeyVal("LEVEL", _iLevel);
    s.WriteKeyVal("LASTACTIVATIONTIME", _iLastActivationTime);
    
    for (std::vector<CItem*>::iterator it = _pRedCandles.begin(); it != _pRedCandles.end()  ; ++it)
    {
        CItem * pCandle = *it;
        if (!pCandle)
            continue;   // ??
        s.WriteKeyHex("ADDREDCANDLE", pCandle->GetUID());
    }

    for (std::vector<CItem*>::iterator it = _pWhiteCandles.begin(); it != _pWhiteCandles.end(); ++it)
    {
        CItem * pCandle = *it;
        if (!pCandle)
            continue;   // ??
        s.WriteKeyHex("ADDWHITECANDLE", pCandle->GetUID());
    }
    return;
};

bool CChampion::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc)
{
    UNREFERENCED_PARAMETER(pSrc);
    ADDTOCALLSTACK("CChampion::r_WriteVal");

    int iCmd = FindTableSorted(pszKey, sm_szLoadKeys, CountOf(sm_szLoadKeys) - 1);
    switch (iCmd)
    {
        case ICHMPL_LASTACTIVATIONTIME:
            sVal.FormatLLVal(_iLastActivationTime);
            return true;
        case ICHMPL_LEVEL:
            sVal.FormatUCVal(_iLevel);
            return true;
        case ICHMPL_LEVELMAX:
            sVal.FormatUCVal(_iLevelMax);
            return true;
        case ICHMPL_REDCANDLES:
            sVal.FormatVal((int)_pRedCandles.size());
            return true;
        case ICHMPL_WHITECANDLES:
            sVal.FormatVal((int)_pWhiteCandles.size());
            return true;
        case ICHMPL_DEATHCOUNT:
            sVal.FormatUSVal(_iDeathCount);
            return true;
        case ICHMPL_SPAWNSCUR:
            sVal.FormatUSVal(_iSpawnsCur);
            return true;
        case ICHMPL_SPAWNSMAX:
            sVal.FormatUSVal(_iSpawnsMax);
            return true;
        default:
            return false;
    }
    return false;
    //return _pLink->r_WriteVal(pszKey, sVal, pSrc);  //CItemMulti::r_WriteVal(pszKey, sVal, pSrc); //FIXME: ECS link with CItemMulti
};

bool CChampion::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CChampion::r_LoadVal");
    int iCmd = FindTable(s.GetKey(), sm_szLoadKeys, (int)CountOf(sm_szLoadKeys) - 1);

    switch (iCmd)
    {
        case ICHMPL_LASTACTIVATIONTIME:
            _iLastActivationTime = s.GetArgLLVal();
            return true;
        case ICHMPL_LEVEL:
            _iLevel = s.GetArgUCVal();
            return true;
        case ICHMPL_LEVELMAX:
            _iLevelMax = s.GetArgUCVal();
            return true;
        case ICHMPL_DEATHCOUNT:
            _iDeathCount = s.GetArgUSVal();
            return true;
        case ICHMPL_SPAWNSCUR:
            _iSpawnsCur = s.GetArgUSVal();
            return true;
        case ICHMPL_SPAWNSMAX:
            _iSpawnsMax = s.GetArgUSVal();
            return true;
        default:
            break; //return _pLink->r_LoadVal(s);    // CItemMulti::r_LoadVal(s); // FIXME: ECS Link with CItemMulti
    }
    return false;
};

bool CChampion::r_GetRef(lpctstr & pszKey, CScriptObj * & pRef)
{
    ADDTOCALLSTACK("CChampion::r_GetRef");
    if (!strnicmp(pszKey, "WHITECANDLE", 11))
    {
        pszKey += 11;
        int iCandle = Exp_GetVal(pszKey);
        SKIP_SEPARATORS(pszKey);
        CItem * pCandle = _pWhiteCandles[iCandle];
        if (pCandle)
        {
            pRef = pCandle;
            return true;
        }
    }
    if (!strnicmp(pszKey, "REDCANDLE", 9))
    {
        pszKey += 9;
        int iCandle = Exp_GetVal(pszKey);
        SKIP_SEPARATORS(pszKey);
        CItem * pCandle = _pRedCandles[iCandle];
        if (pCandle)
        {
            pRef = pCandle;
            return true;
        }
    }
    if (!strnicmp(pszKey, "SPAWN", 5))
    {
        pszKey += 5;
        lpctstr i = pszKey;
        SKIP_SEPARATORS(pszKey);
        CSpawn * pSpawn = static_cast<CSpawn*>(_pLink->GetComponent(COMP_SPAWN));
        if (pSpawn)
        {
            return pSpawn->r_GetRef(i, pRef);
        }
        return true;
    }

    return false;// return _pLink->r_GetRef(pszKey, pRef);  // CItemMulti::r_GetRef(pszKey, pRef);// FIXME: ECS Link with CItemMulti
};

bool CChampion::r_Verb(CScript & s, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CChampion::r_Verb");
    UNREFERENCED_PARAMETER(pSrc);
    lpctstr key = s.GetKey();
    int iCmd = FindTableSorted(key, sm_szVerbKeys, (int)CountOf(sm_szVerbKeys) - 1);
    switch (iCmd)
    {
        case ICHMPV_ADDOBJ:
        {
            CUID uid = static_cast<CUID>(s.GetArgVal());
            if (uid.ObjFind())
                AddObj(uid);
            return true;
        }
        case ICHMPV_ADDREDCANDLE:
        {
            CUID uid = static_cast<CUID>(s.GetArgVal());
            AddRedCandle(uid);
            return true;
        }
        case ICHMPV_ADDWHITECANDLE:
        {
            CUID uid = static_cast<CUID>(s.GetArgVal());
            AddWhiteCandle(uid);
            return true;
        }
        case ICHMPV_ADDSPAWN:
            SpawnNPC();
            return true;
        case ICHMPV_DELOBJ:
        {
            CUID uid = static_cast<CUID>(s.GetArgVal());
            if (uid.ObjFind())
                DelObj(uid);
            return true;
        }
        case ICHMPV_DELREDCANDLE:
            DelRedCandle();
            return true;
        case ICHMPV_DELWHITECANDLE:
            DelWhiteCandle();
            return true;
        case ICHMPV_INIT:
            Init();
            return true;
        case ICHMPV_MULTICREATE:
        {
            /*CUID	uid( s.GetArgVal() );   // FIXME: ECS link with CItemMulti.
            CChar *	pCharSrc = uid.CharFind();
            Multi_Create( pCharSrc, 0 );*/
            return true;
        }
        case ICHMPV_STOP:
            Stop();
            return true;

        default:
            return false;// return _pLink->r_Verb(s, pSrc);
    }
    return false;
};


enum CHAMPIONDEF_TYPE
{
    CHAMPIONDEF_CHAMPION,	///< Champion ID: _iChampion.
    CHAMPIONDEF_DEFNAME,	///< Champion's DEFNAME
    CHAMPIONDEF_NAME,		///< Champion name: m_sName.
    CHAMPIONDEF_NPCGROUP,	///< Monster level / group: _iSpawn[n][n].
    CHAMPIONDEF_SPAWNS,		///< Total amount of monsters: _iSpawnsMax.
    CHAMPIONDEF_LEVELMAX,   ///< Max Level for this champion.
    CHAMPIONDEF_QTY
};

lpctstr const CChampionDef::sm_szLoadKeys[] =
{
    "CHAMPION",
    "DEFNAME",
    "NAME",
    "NPCGROUP",
    "SPAWNS",
    "TYPE",
    "LEVEL",
    NULL
};


CChampionDef::CChampionDef(CResourceID rid) : CResourceLink(rid)
{
    ADDTOCALLSTACK("CChampionDef::CChampionDef");
    _iSpawnsMax = 0;
    _iLevelMax = 16;
};

CChampionDef::~CChampionDef()
{
    ADDTOCALLSTACK("CChampionDef::~CChampionDef");
};

bool CChampionDef::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CChampionDef::r_WriteVal");
    UNREFERENCED_PARAMETER(pSrc);
    CHAMPIONDEF_TYPE iCmd = (CHAMPIONDEF_TYPE)FindTableHeadSorted(pszKey, sm_szLoadKeys, CHAMPIONDEF_QTY);

    switch (iCmd)
    {
        case CHAMPIONDEF_CHAMPION:
            sVal = g_Cfg.ResourceGetName(CResourceID(RES_CHARDEF, _idChampion));
            return true;
        case CHAMPIONDEF_DEFNAME:
            sVal = g_Cfg.ResourceGetName(GetResourceID());
            return true;
        case CHAMPIONDEF_NAME:
            sVal.Format(m_sName);
            return true;
        case CHAMPIONDEF_NPCGROUP:
        {
            pszKey += 8;
            int8 iGroup = (int8)Exp_GetSingle(pszKey);
            if (iGroup < 1 || iGroup > 4)
                return false;
            ++pszKey;
            int8 iNpc = (int8)Exp_GetSingle(pszKey);
            if (iNpc < 1 || iNpc > 4)
                return false;
            g_Log.EventDebug("npcgroup %d - %d\n",iGroup, iNpc);
            if (_iSpawn[iGroup][iNpc])
            {
                sVal = g_Cfg.ResourceGetName(CResourceID(RES_CHARDEF, _iSpawn[iGroup][iNpc]));
                return true;
            }
            return false;
        }
        case CHAMPIONDEF_SPAWNS:
            sVal.FormatUSVal(_iSpawnsMax);
            return true;
        case CHAMPIONDEF_LEVELMAX:
            sVal.FormatUCVal(_iLevelMax);
            return true;
        default:
            return true;
            break;
    }
    return false;
};

bool CChampionDef::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CChampionDef::r_LoadVal");
    lpctstr pszKey = s.GetKey();
    CHAMPIONDEF_TYPE iCmd = static_cast<CHAMPIONDEF_TYPE>(FindTableHeadSorted(pszKey, sm_szLoadKeys, (int)CountOf(sm_szLoadKeys) - 1));

    switch (iCmd)
    {
        case CHAMPIONDEF_CHAMPION:
            _idChampion = static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgStr()));
            return true;
        case CHAMPIONDEF_DEFNAME:
            return SetResourceName(s.GetArgStr());
        case CHAMPIONDEF_NAME:
            m_sName = s.GetArgStr();
            return true;
        case CHAMPIONDEF_NPCGROUP:
        {
            pszKey += 8;
            uchar iGroup = (uchar)Exp_GetVal(pszKey);
            if (iGroup < 1)
                return false;
            tchar * piCmd[4];
            size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), piCmd, (int)CountOf(piCmd), ",");
            for (uchar i = 0; i < iArgQty; i++)
            {
                CREID_TYPE pCharDef = static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, piCmd[i]));
                if (pCharDef)
                {
                    g_Log.EventDebug("loaded NPCGROUP[%d] = %lld\n", iGroup, piCmd[i]);
                    _iSpawn[iGroup].emplace_back(pCharDef);
                }
            }
            return true;
        }
        case CHAMPIONDEF_SPAWNS:
            _iSpawnsMax = (ushort)s.GetArgUSVal();
            return true;
        case CHAMPIONDEF_LEVELMAX:
            _iLevelMax = (uchar)s.GetArgUCVal();
            return true;
        default:
            break;
    }
    return false;
};