/*
* @file Champion.cpp
*
* @brief This file contains CCChampion and CCChampionDef classes declarations
*
* CCChampion class manages a CItem making it to work as a Champion
* class is declared inside CObjBase.h
*
* CCChampionDef class is used to store all the data placed in scripts under the [CHAMPION ] resource
* this data is read by CCChampion to know Champion's type, npcs to spawn at each level, boss id....
* class is declared inside CResource.h
*/

#include "CCChampion.h"	// predef header.
#include "../CWorld.h"
#include "../chars/CChar.h"
#include "../../common/CScript.h"
#include "../../common/CResourceBase.h"
#include "../../common/CException.h"


#define CANDLESNEXTRED 4

CCChampion::CCChampion(CObjBase *pLink) : CComponent(COMP_CHAMPION, pLink)
{
    ADDTOCALLSTACK("CCChampion::CCChampion");
    Init();
};

CCChampion::~CCChampion()
{
};

bool CCChampion::OnTick()
{
    ADDTOCALLSTACK("CCChampion::OnTick");
    if (!_pRedCandles.empty())
        DelRedCandle();
    else
        Stop();
    GetLink()->SetTimeout(TICK_PER_SEC * 60 * 10);	//10 minutes
    return true;
};

void CCChampion::Init()
{
    ADDTOCALLSTACK("CCChampion::Init");
    _fActive = false;
    _iLevel = 0;
    _iSpawnsCur = 0;
    _iDeathCount = 0;
    _iSpawnsNextWhite = 0;
    _iSpawnsNextRed = GetCandlesPerLevel();
    _iCandlesNextRed = CANDLESNEXTRED;
    _iCandlesNextLevel = 0;
    _iLastActivationTime = 0;

    CCSpawn *pSpawn = static_cast<CCSpawn*>(GetLink()->GetComponent(COMP_SPAWN));
    ASSERT(pSpawn);
    CCChampionDef *pChampDef = static_cast<CCChampionDef*>(g_Cfg.ResourceGetDef(pSpawn->GetSpawnID()));
    if (!pChampDef)
    {
        return;
    }
    ASSERT(pChampDef);
    _iLevelMax = pChampDef->_iLevelMax;
    _iSpawnsMax = pChampDef->_iSpawnsMax;
    _idChampion = pChampDef->_idChampion;
    _idSpawn.insert(pChampDef->_idSpawn.begin(), pChampDef->_idSpawn.end());
}

void CCChampion::Start()
{
    ADDTOCALLSTACK("CCChampion::Start");
    // TODO store light in the area
    _fActive = true;
    _iLastActivationTime = g_World.GetCurrentTime().GetTimeRaw();

    _iSpawnsNextRed = GetCandlesPerLevel();
    _iCandlesNextRed = CANDLESNEXTRED;

    SetLevel(1);
    for (unsigned char i = 0; i < _iSpawnsNextWhite; i++)   // Spawn all the monsters required to get the next White candle. // TODO: a way to prevent this from script.
        SpawnNPC();
};

void CCChampion::Stop()
{
    ADDTOCALLSTACK("CCChampion::Stop");
    KillChildren();
    _fActive = false;
    _iSpawnsCur = 0;
    _iDeathCount = 0;
    _iLevel = 0;
    _iCandlesNextRed = 0;
    _iCandlesNextLevel = 0;
    GetLink()->SetTimeout((_iLastActivationTime - g_World.GetCurrentTime().GetTimeRaw()) / TICK_PER_SEC);
    ClearWhiteCandles();
    ClearRedCandles();
};

void CCChampion::Complete()
{
    ADDTOCALLSTACK("CCChampion::Complete");
    Stop();	//Cleaning everything, just to be sure.
    // TODO: Add rewards, titles, etc
};

void CCChampion::OnKill()
{
    ADDTOCALLSTACK("CCChampion::OnKill");
    if (_iSpawnsNextWhite == 0)
    {
        AddWhiteCandle();
    }
    SpawnNPC();
}

void CCChampion::SpawnNPC()
{
    ADDTOCALLSTACK("CCChampion::SpawnNPC");

    if (_iLevel == _iLevelMax)	// Already summoned the Champion, stop
        return;

    CREID_TYPE pNpc = CREID_INVALID;
    if (_pRedCandles.size() == _iLevelMax && _iSpawnsNextWhite == 0) // Reached 16th red candle and spawned all the normal npcs, so next one should be the champion
    {
        pNpc = _idChampion;
    }
    else
    {
        if (_idSpawn[_iLevel].size() <= 0)
        {
            g_Log.EventError("CCChampion:: Trying to create NPCs for undefined NPCGROUP[%d]\n", _iLevel);
            return;
        }
        uchar iRand = (uchar)Calc_GetRandVal2(1, (int)_idSpawn[_iLevel].size() - 1);
        pNpc = _idSpawn[_iLevel][iRand];	//Find out the random npc.
        CResourceIDBase rid = CResourceID(RES_CHARDEF, pNpc);
        --_iSpawnsNextWhite;
        CResourceDef * pDef = g_Cfg.ResourceGetDef(rid);
        if (!pDef)
        {
            return;
        }
        CCChampionDef * pChamp = static_cast<CCChampionDef*>(pDef);
        if (!pChamp)
        {
            return;
        }
        if (!pNpc)	// At least one NPC per level should be added, check just in case.
            return;
    }
    CChar * pChar = CChar::CreateBasic(pNpc);
    if (!pChar)
        return;
    pChar->SetTopPoint(GetLink()->GetTopPoint());
    pChar->MoveNear(GetLink()->GetTopPoint(), 10);
    pChar->Update();
    pChar->NPC_LoadScript(true);
    AddObj(pChar->GetUID());

};

void CCChampion::AddWhiteCandle(CUID uid)
{
    ADDTOCALLSTACK("CCChampion::AddWhiteCandle");
    //TODO Trigger @AddWhiteCandle(uid)
    if (_pWhiteCandles.size() >= _iCandlesNextRed)
    {
        AddRedCandle();
        return;
    }
    else
        _iSpawnsNextWhite = _iSpawnsNextRed / 5;

    CItem * pCandle = NULL;
    CItem *pLink = static_cast<CItem*>(GetLink());
    if (uid != static_cast<CUID>(UID_UNUSED))
    {
        pCandle = uid.ItemFind();
    }

    if (!pCandle)
    {
        pCandle = pLink->CreateBase(ITEMID_SKULL_CANDLE);
        if (!pCandle)
            return;
        CPointMap pt = pLink->GetTopPoint();
        switch (_pWhiteCandles.size()+1)    // +1 here because the candle is post placed.
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
    _pWhiteCandles.push_back(pCandle->GetUID());
    pCandle->m_uidLink = pLink->GetUID();	// Link it to the champion, so if it gets removed the candle will be removed too
};

void CCChampion::AddRedCandle(CUID uid)
{
    ADDTOCALLSTACK("CCChampion::AddRedCandle");
    CItem * pCandle = NULL;
    if (uid != static_cast<CUID>(UID_UNUSED))
    {
        pCandle = uid.ItemFind();
    }
    CItem *pLink = static_cast<CItem*>(GetLink());

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
        pCandle = pLink->CreateBase(ITEMID_SKULL_CANDLE);
        if (!pCandle)
            return;
        CPointMap pt = pLink->GetTopPoint();
        _iCandlesNextRed = CANDLESNEXTRED;
        switch (_pRedCandles.size()+1)  // +1 here because the candle is post placed.
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
        //TODO Trigger @AddRedCandle (pCandle, pt, _iCandlesNextRed,)

        pCandle->SetAttr(ATTR_MOVE_NEVER);
        pCandle->MoveTo(pt);
        pCandle->SetTopZ(pCandle->GetTopZ() + 4);
        pCandle->SetHue(static_cast<HUE_TYPE>(33));
        pCandle->Update();
        pCandle->GenerateScript(NULL);
        ClearWhiteCandles();
    }
    _pRedCandles.push_back(pCandle->GetUID());
    pCandle->m_uidLink = pLink->GetUID();	// Link it to the champion, so if it gets removed the candle will be removed too

};

void CCChampion::SetLevel(byte iLevel)
{
    ADDTOCALLSTACK("CCChampion::SetLevel");
    if (iLevel > 4)
        iLevel = 4;
    else if (iLevel < 1)
        iLevel = 1;
    ushort iLevelMonsters = GetMonstersPerLevel(_iSpawnsMax);
    _iCandlesNextLevel = GetCandlesPerLevel();
    _iCandlesNextRed = 4;
    ushort iRedMonsters = iLevelMonsters / _iCandlesNextLevel;
    ushort iWhiteMonsters = iRedMonsters / _iCandlesNextRed;

    // TODO: Trigger @Level (old level, new level, GetMonstersPerLevel, _iCandlesNextLevel)
    // TODO: As the level increases, the light on the area decreases.

    _iLevel = iLevel;
    _iSpawnsNextWhite = iWhiteMonsters;
    _iSpawnsNextRed = iRedMonsters;
    GetLink()->SetTimeout(TICK_PER_SEC * 60 * 10);	//10 minutes
};

byte CCChampion::GetCandlesPerLevel()
{
    ADDTOCALLSTACK("CCChampion::GetCandlesPerLevel");
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

ushort CCChampion::GetMonstersPerLevel(ushort iMonsters)
{
    ADDTOCALLSTACK("CCChampion::GetMonstersPerLevel");
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
void CCChampion::DelWhiteCandle()
{
    ADDTOCALLSTACK("CCChampion::DelWhiteCandle");
    if (_pWhiteCandles.size() == 0)
        return;

    //TODO Trigger @DelWhiteCandle

    CItem * pCandle = _pWhiteCandles[_pWhiteCandles.size()-1].ItemFind();
    if (pCandle)
    {
        pCandle->Delete();
    }
    _pWhiteCandles.pop_back();
};

// Delete the last created red candle.
void CCChampion::DelRedCandle()
{
    ADDTOCALLSTACK("CCChampion::DelRedCandle");
    if (_pRedCandles.size() == 0)
        return;

    //TODO Trigger @DelRedCandle

    CItem * pCandle = _pRedCandles[_pRedCandles.size() - 1].ItemFind();
    if (pCandle)
    {
        pCandle->Delete();
    }
    _pRedCandles.pop_back();
};

// Clear all white candles.
void CCChampion::ClearWhiteCandles()
{
    ADDTOCALLSTACK("CCChampion::ClearWhiteCandles");
    if (_pWhiteCandles.size() == 0)
        return;
    do
    {
        DelRedCandle();
    } while (_pWhiteCandles.size() > 0);
    /*for (std::vector<CUID>::iterator it = _pWhiteCandles.begin(); it != _pWhiteCandles.end(); ++it)
    {
        CItem * pCandle = it->ItemFind();;
        if (pCandle)
        {
            pCandle->Delete();
        }
    }*/
    _pWhiteCandles.clear();
};

// Clear all red candles.
void CCChampion::ClearRedCandles()
{
    ADDTOCALLSTACK("CCChampion::ClearRedCandles");
    if (_pRedCandles.size() == 0)
        return;
    for (std::vector<CUID>::iterator it = _pRedCandles.begin(); it != _pRedCandles.end(); ++it)
    {
        CItem * pCandle = it->ItemFind();;
        if (pCandle)
        {
            pCandle->Delete();
        }
    }
    _pRedCandles.clear();
};


// kill everything spawned from this spawn !
void CCChampion::KillChildren()
{
    ADDTOCALLSTACK("CCSpawn:KillChildren");
    CCSpawn *pSpawn = static_cast<CCSpawn*>(GetLink()->GetComponent(COMP_SPAWN));
    if (pSpawn)
    {
        pSpawn->KillChildren();
    }
}

// Deleting one object from Spawn's memory, reallocating memory automatically.
void CCChampion::DelObj(CUID uid)
{
    ADDTOCALLSTACK("CCSpawn:DelObj");
    if (!uid.IsValidUID())
    {
        return;
    }
    CCSpawn *pSpawn = static_cast<CCSpawn*>(GetLink()->GetComponent(COMP_SPAWN));
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
void CCChampion::AddObj(CUID uid)
{
    ADDTOCALLSTACK("CCSpawn:AddObj");
    if (!uid || !uid.CharFind()->m_pNPC)    // Only adding UIDs...
    {
        return;
    }
    if (uid.CharFind()->GetSpawn())	//... which doesn't have a SpawnItem already
    {
        return;
    }
    CCSpawn *pSpawn = static_cast<CCSpawn*>(GetLink()->GetComponent(COMP_SPAWN));
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
/*byte CCChampion::GetPercentMonsters()
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
/*byte CCChampion::GetCompletionMonsters()
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
    ICHMPL_NPCGROUP,
    ICHMPL_REDCANDLES,
    ICHMPL_SPAWNSCUR,
    ICHMPL_SPAWNSMAX,
    ICHMPL_WHITECANDLES,
    ICHMPL_QTY
};

lpctstr const CCChampion::sm_szLoadKeys[ICHMPL_QTY + 1] =
{
    "DEATHCOUNT",
    "LASTACTIVATIONTIME",
    "LEVEL",
    "LEVELMAX",
    "NPCGROUP",
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


lpctstr const CCChampion::sm_szVerbKeys[ICHMPV_QTY + 1] =
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

void CCChampion::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CCChampion::r_Write");
    s.WriteKeyVal("DEATHCOUNT", _iDeathCount);
    s.WriteKeyVal("SPAWNSCUR", _iSpawnsCur);
    s.WriteKeyVal("LEVEL", _iLevel);
    s.WriteKeyVal("LASTACTIVATIONTIME", _iLastActivationTime);

    for (std::vector<CUID>::iterator it = _pRedCandles.begin(); it != _pRedCandles.end()  ; ++it)
    {
        CItem * pCandle = static_cast<CUID&>(*it).ItemFind();
        if (!pCandle)
            continue;   // ??
        s.WriteKeyHex("ADDREDCANDLE", pCandle->GetUID());
    }

    for (std::vector<CUID>::iterator it = _pWhiteCandles.begin(); it != _pWhiteCandles.end(); ++it)
    {
        CItem * pCandle = static_cast<CUID&>(*it).ItemFind();
        if (!pCandle)
            continue;   // ??
        s.WriteKeyHex("ADDWHITECANDLE", pCandle->GetUID());
    }
    return;
};

bool CCChampion::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc)
{
    UNREFERENCED_PARAMETER(pSrc);
    ADDTOCALLSTACK("CCChampion::r_WriteVal");

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
        case ICHMPL_NPCGROUP:
        {
            pszKey += 8;
            uchar uiGroup = (uchar)Exp_GetSingle(pszKey);
            if (_idSpawn[uiGroup].empty() || uiGroup > UCHAR_MAX)
            {
                return false;
            }
            ++pszKey;
            uchar uiNpc = (uchar)Exp_GetSingle(pszKey);
            size_t uiGroupSize = _idSpawn[uiGroup].size();
            if (uiNpc && (uiNpc >= uiGroupSize))
                return false;
            if (uiGroupSize >= uiNpc)
            {
                sVal = g_Cfg.ResourceGetName(CResourceID(RES_CHARDEF, _idSpawn[uiGroup][uiNpc]));
                return true;
            }
            return false;
        }
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
};

bool CCChampion::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CCChampion::r_LoadVal");
    int iCmd = FindTableSorted(s.GetKey(), sm_szLoadKeys, (int)CountOf(sm_szLoadKeys) - 1);

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
            break;
    }
    return false;
};

void CCChampion::Delete(bool fForce)
{
    ADDTOCALLSTACK("CCChampion::Delete");
    UNREFERENCED_PARAMETER(fForce);
    // KillChildren is being called from CCSpawn, must not call it twice.
    ClearWhiteCandles();
    ClearRedCandles();
}

bool CCChampion::r_GetRef(lpctstr & pszKey, CScriptObj * & pRef)
{
    ADDTOCALLSTACK("CCChampion::r_GetRef");
    if (!strnicmp(pszKey, "WHITECANDLE", 11))
    {
        pszKey += 11;
        int iCandle = Exp_GetVal(pszKey);
        SKIP_SEPARATORS(pszKey);
        CItem * pCandle = _pWhiteCandles[iCandle].ItemFind();
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
        CItem * pCandle = _pRedCandles[iCandle].ItemFind();
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
        CCSpawn * pSpawn = static_cast<CCSpawn*>(GetLink()->GetComponent(COMP_SPAWN));
        if (pSpawn)
        {
            return pSpawn->r_GetRef(i, pRef);
        }
        return true;
    }

    return false;
};

bool CCChampion::r_Verb(CScript & s, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CCChampion::r_Verb");
    UNREFERENCED_PARAMETER(pSrc);
    int iCmd = FindTableSorted(s.GetKey(), sm_szVerbKeys, (int)CountOf(sm_szVerbKeys) - 1);
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
            return false;
    }
    return false;
}

void CCChampion::Copy(CComponent * target)
{
    ADDTOCALLSTACK("CCChampion::Copy");
    UNREFERENCED_PARAMETER(target);
    /*I don't see the point of duping a Champion, its insane and makes no sense,
    * if someone wants to totally dupe a champion it can be done from scripts.
    */
}


enum CHAMPIONDEF_TYPE
{
    CHAMPIONDEF_CHAMPION,	///< Champion ID: _iChampion.
    CHAMPIONDEF_DEFNAME,	///< Champion's DEFNAME.
    CHAMPIONDEF_LEVELMAX,   ///< Max Level for this champion.
    CHAMPIONDEF_NAME,		///< Champion name: m_sName.
    CHAMPIONDEF_NPCGROUP,	///< Monster level / group: _iSpawn[n][n].
    CHAMPIONDEF_SPAWNS,		///< Total amount of monsters: _iSpawnsMax.
    CHAMPIONDEF_QTY
};

lpctstr const CCChampionDef::sm_szLoadKeys[] =
{
    "CHAMPION",
    "DEFNAME",
    "LEVEL",
    "NAME",
    "NPCGROUP",
    "SPAWNS",
    "TYPE",
    NULL
};


CCChampionDef::CCChampionDef(CResourceID rid) : CResourceLink(rid)
{
    ADDTOCALLSTACK("CCChampionDef::CCChampionDef");
    _iSpawnsMax = 2368;
    _iLevelMax = 16;
};

CCChampionDef::~CCChampionDef()
{
};

bool CCChampionDef::r_WriteVal(lpctstr pszKey, CSString & sVal, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CCChampionDef::r_WriteVal");
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
            if (_idSpawn.empty())
            {
                return false;
            }
            pszKey += 8;
            uchar uiGroup = (uchar)Exp_GetSingle(pszKey);
            ++pszKey;
            uchar uiNPC = (uchar)Exp_GetSingle(pszKey);
            if (uiNPC && (uiGroup >= _idSpawn.size()))
                return false;
            if (_idSpawn[uiGroup].size() >= uiNPC)
            {
                CREID_TYPE idNPC = (CREID_TYPE)uiNPC;
                if (idNPC != CREID_INVALID)
                {
                    sVal = g_Cfg.ResourceGetName(CResourceID(RES_CHARDEF, _idSpawn[uiGroup][idNPC]));
                    return true;
                }
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

bool CCChampionDef::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CCChampionDef::r_LoadVal");
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
            tchar * piCmd[4];
            size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), piCmd, (int)CountOf(piCmd), ",");
            _idSpawn[iGroup].clear();
            for (uchar i = 0; i < iArgQty; ++i)
            {
                CREID_TYPE pCharDef = static_cast<CREID_TYPE>(g_Cfg.ResourceGetIndexType(RES_CHARDEF, piCmd[i]));
                if (pCharDef)
                {
                    _idSpawn[iGroup].emplace_back(pCharDef);
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
