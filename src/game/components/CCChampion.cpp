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

#include "../../common/resource/CResourceLock.h"
#include "../../common/CException.h"
#include "../../common/CScript.h"
#include "../chars/CChar.h"
#include "../CServer.h"
#include "../CWorldGameTime.h"
#include "CCChampion.h"	// predef header.
#include <string>
#include <algorithm> 
#include <sstream> 
#include <cctype>

#define CANDLESNEXTRED 4

CCChampion::CCChampion(CItem *pLink) : CComponent(COMP_CHAMPION)
{
    ADDTOCALLSTACK("CCChampion::CCChampion");
    _pLink = pLink;
    _iLevelMax = 4;
    _iSpawnsMax = 2400;
    _idChampion = CREID_INVALID;
    Init();
}

CCChampion::~CCChampion()
{
    ADDTOCALLSTACK("CCChampion::~CCChampion");
    ClearWhiteCandles();
    ClearRedCandles();
}

CItem * CCChampion::GetLink() const
{
    return _pLink;
}

CCRET_TYPE CCChampion::OnTickComponent()
{
    ADDTOCALLSTACK("CCChampion::_OnTick");
    if (!_pRedCandles.empty())
    {
        _iSpawnsMax += _iSpawnsNextRed;
        DelRedCandle();
    }
    else
    {
        Stop();
    }
    GetLink()->SetTimeoutS(60 * 10);	//10 minutes
    return CCRET_CONTINUE;
}

CCSpawn* CCChampion::GetSpawnItem()
{
    return static_cast<CCSpawn*>(GetLink()->GetComponent(COMP_SPAWN));
}

void CCChampion::Init()
{
    ADDTOCALLSTACK("CCChampion::Init");
    _fActive = false;
    _iLevel = 0;
    _iSpawnsCur = 0;
    _iDeathCount = 0;
    _iSpawnsNextWhite = 2;
    _iSpawnsNextRed = 10;
    _iCandlesNextRed = CANDLESNEXTRED;
    _iCandlesNextLevel = 0;
    _iLastActivationTime = 0;
    _fChampionSummoned = false;
    if (_idSpawn.IsValidResource() == false)
    {
        return;
    }
    const int resId = _idSpawn.GetResIndex();
    const CResourceIDBase rid(RES_CHAMPION, resId);
    CResourceDef* pResDef = g_Cfg.RegisteredResourceGetDef(rid);
    const CCChampionDef* pChampDef = static_cast<CCChampionDef*>(pResDef);
    if (pChampDef == nullptr)
    {
        return;
    }
    _iLevelMax = pChampDef->_iLevelMax;
    _iSpawnsMax = pChampDef->_iSpawnsMax;
    _idChampion = pChampDef->_idChampion;
    _spawnGroupsId.clear();
    if (pChampDef->_idSpawn.size())
    {
        _spawnGroupsId.insert(pChampDef->_idSpawn.begin(), pChampDef->_idSpawn.end());
    }
}

void CCChampion::Start()
{
    ADDTOCALLSTACK("CCChampion::Start");
    // TODO store light in the area
    if (_fActive == true)
    {
        Stop();
    }
    _fActive = true;
    _iLastActivationTime = CWorldGameTime::GetCurrentTime().GetTimeRaw();

    _iSpawnsNextRed = GetCandlesPerLevel();
    _iCandlesNextRed = CANDLESNEXTRED;

    SetLevel(1);

    if (IsTrigUsed(TRIGGER_START))
    {
        if (OnTrigger(ITRIG_Start, &g_Serv, nullptr) == TRIGRET_RET_TRUE)
        {
            return;// Do not spawn anything if the trigger returns 1
        }
    }
    for (ushort i = 0; i < _iSpawnsNextWhite; ++i)   // Spawn all the monsters required to get the next White candle. // TODO: a way to prevent this from script.
        SpawnNPC();
}

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
    _fChampionSummoned = false;
    GetLink()->SetTimeout(_iLastActivationTime - CWorldGameTime::GetCurrentTime().GetTimeRaw());
    ClearWhiteCandles();
    ClearRedCandles();
}

void CCChampion::Complete()
{
    ADDTOCALLSTACK("CCChampion::Complete");
    Stop();	//Cleaning everything, just to be sure.
    // TODO: Add rewards, titles, etc
}

void CCChampion::OnKill()
{
    ADDTOCALLSTACK("CCChampion::OnKill");
    if (_iSpawnsNextWhite == 0)
    {
        AddWhiteCandle();
    }
    ++_iDeathCount;
    if (_iDeathCount >= _iSpawnsMax)
    {
        ClearWhiteCandles();
        ClearRedCandles();
        if (_fChampionSummoned == false)
        {
            SetLevel(UCHAR_MAX);
        }
    }
    SpawnNPC();
}

void CCChampion::SpawnNPC()
{
    ADDTOCALLSTACK("CCChampion::SpawnNPC");

    CREID_TYPE pNpc = CREID_INVALID;
    CResourceIDBase rid;

    if (_iLevel == UCHAR_MAX)// Completed the champion's minor spawns, summon the boss (once).
    {
        if (_fChampionSummoned == true)// Already summoned the Champion, stop
        {
            return;
        }
        if (_iDeathCount >= _iSpawnsMax)    // Killed all npcs, summon the boss.
        {
            pNpc = _idChampion;
            _fChampionSummoned = true;
        }
    }
    else if (_iSpawnsCur < _iSpawnsMax)
    {
        int iSize = (int)_spawnGroupsId[_iLevel].size();
        idSpawn spawngroup;
        if ( iSize > 0)
        {
            spawngroup = _spawnGroupsId;
        }
        else
        {
            CResourceDef* pRes = g_Cfg.RegisteredResourceGetDef(_idSpawn);
            CCChampionDef* pChampDef = static_cast<CCChampionDef*>(pRes);
            if (pChampDef != nullptr)
            {
                iSize = (int)pChampDef->_idSpawn[_iLevel].size();
                if ( iSize > 0)
                {
                    spawngroup = pChampDef->_idSpawn;
                }
                else
                {
                    g_Log.EventError("CCChampion:: Trying to create NPCs from undefined NPCGROUP[%d]\n", _iLevel);
                    return;
                }
            }
        }
        if (_iSpawnsNextWhite > 0)
        {
            --_iSpawnsNextWhite;
        }
        if (iSize > 0 && iSize <= UCHAR_MAX)
        {
            uchar iRand = (uchar)Calc_GetRandVal2(0, (int)iSize -1);
            pNpc = spawngroup[_iLevel][iRand];	//Find out the random npc.
        }
        else
        {
            g_Log.EventError("Champion bad group index %d.\n", _iLevel);
        }
    }
    else
    {
        return;
    }
    rid = CResourceIDBase(RES_CHARDEF, pNpc);
    CResourceDef* pDef = g_Cfg.RegisteredResourceGetDef(rid);
    if (!pDef)
    {
        return;
    }
    //ASSERT(dynamic_cast<CCChampionDef*>(pDef));
    if (!pNpc)	// At least one NPC per level should be added, check just in case.
    {
        return;
    }
    CCSpawn* pSpawn = GetSpawnItem();
    if (pSpawn)
    {
        CChar* spawn = pSpawn->GenerateChar(rid);
        if (spawn != nullptr)
        {
            AddObj(spawn->GetUID());
        }
    }
    ++_iSpawnsCur;
}

void CCChampion::AddWhiteCandle(const CUID& uid)
{
    ADDTOCALLSTACK("CCChampion::AddWhiteCandle");
    if (_iLevel == UCHAR_MAX)
    {
        return;
    }
    if (_pWhiteCandles.size() >= _iCandlesNextRed)
    {
        AddRedCandle();
        return;
    }
    else
        _iSpawnsNextWhite = _iSpawnsNextRed / 5;

    CItem * pCandle = nullptr;
    CItem *pLink = static_cast<CItem*>(GetLink());
    if (uid.IsValidUID())
    {
        pCandle = uid.ItemFind();
    }

    if (!pCandle)
    {
        pCandle = pLink->CreateBase(ITEMID_SKULL_CANDLE);
        if (!pCandle)
            return;
        CPointMap pt = pLink->GetTopPoint();
        switch (_pWhiteCandles.size() + 1)    // +1 here because the candle is post placed.
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
        pCandle->SetTopPoint(pt);
        if (g_Serv.IsLoading() == false)
        {
            if (IsTrigUsed(TRIGGER_ADDWHITECANDLE))
            {
                CScriptTriggerArgs args(pCandle);
                if (OnTrigger(ITRIG_ADDWHITECANDLE, &g_Serv, &args) == TRIGRET_RET_TRUE)
                {
                    pCandle->Delete();
                    return;
                }
            }
        }
        pCandle->SetAttr(ATTR_MOVE_NEVER);
        pCandle->MoveTo(pt);
        pCandle->SetTopZ(pCandle->GetTopZ() + 4);
        pCandle->Update();
        pCandle->GenerateScript(nullptr);
    }
    _pWhiteCandles.emplace_back(pCandle->GetUID());
    pCandle->m_uidLink = pLink->GetUID();	// Link it to the champion, so if it gets removed the candle will be removed too
}

void CCChampion::AddRedCandle(const CUID& uid)
{
    ADDTOCALLSTACK("CCChampion::AddRedCandle");
    CItem * pCandle = nullptr;
    if (uid.IsValidUID())
    {
        pCandle = uid.ItemFind();
    }
    CItem *pLink = static_cast<CItem*>(GetLink());

    size_t uiRedCandlesAmount = _pRedCandles.size();
    if (uiRedCandlesAmount >= _iCandlesNextLevel)
    {
        SetLevel(_iLevel + 1);
    }
    if (_iLevel >= _iLevelMax)
    {
        return;
    }
    _iSpawnsNextWhite = _iSpawnsNextRed / 5;
    if (!g_Serv.IsLoading())	// White candles may be created before red ones when restoring items from worldsave we must not remove them.
    {
        ClearWhiteCandles();
    }

    if (!pCandle)
    {
        pCandle = pLink->CreateBase(ITEMID_SKULL_CANDLE);
        if (!pCandle)
            return;
        _iCandlesNextRed = CANDLESNEXTRED;
        CPointMap pt = pLink->GetTopPoint();
        if (g_Serv.IsLoading() == false)
        {
            if (IsTrigUsed(TRIGGER_ADDREDCANDLE))
            {
                CScriptTriggerArgs args(pCandle);
                if (OnTrigger(ITRIG_ADDREDCANDLE, &g_Serv, &args) == TRIGRET_RET_TRUE)
                {
                    pCandle->Delete();
                    return;
                }
            }
        }
        switch (uiRedCandlesAmount+1)  // +1 here because the candle is post placed.
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
        pCandle->SetHue( (HUE_TYPE)33 );
        pCandle->Update();
        pCandle->GenerateScript(nullptr);
        ClearWhiteCandles();
    }
    _pRedCandles.emplace_back(pCandle->GetUID());
    pCandle->m_uidLink = pLink->GetUID();	// Link it to the champion, so if it gets removed the candle will be removed too
}

void CCChampion::SetLevel(byte iLevel)
{
    ADDTOCALLSTACK("CCChampion::SetLevel");
    _iLevel = iLevel;
    if (g_Serv.IsLoading() == true)
    {
        return;
    }
    if (_iLevel < 1)
    {
        _iLevel = 1;
    }
    ushort iLevelMonsters = GetMonstersPerLevel(_iSpawnsMax);
    //Get the current candles required - last required candles (if current level = 1, then gets 6-0 = 6, level 2 = 10 - 6 = 4, and so on).
    _iCandlesNextLevel = GetCandlesPerLevel();
    _iCandlesNextRed = 4;
    ushort iRedMonsters = iLevelMonsters / _iCandlesNextLevel;
    ushort iWhiteMonsters = iRedMonsters / _iCandlesNextRed;

    // TODO: Trigger @Level (old level, new level, GetMonstersPerLevel, _iCandlesNextLevel)
    // TODO: As the level increases, the light on the area decreases.

    _iSpawnsNextWhite = iWhiteMonsters;
    _iSpawnsNextRed = iRedMonsters;
    GetLink()->SetTimeoutS(60 * 10);	//10 minutes
}

byte CCChampion::GetCandlesPerLevel(byte iLevel) const
{
    ADDTOCALLSTACK("CCChampion::GetCandlesPerLevel");
    if (iLevel == 255)
    {
        iLevel = _iLevel;
    }
    switch (iLevel)
    {
        case 4:
            return 16;
        case 3:
            return 14;
        case 2:
            return 10;
        case 1:
            return 6;
        default:
            return 1;
    }
}

ushort CCChampion::GetMonstersPerLevel(ushort iMonsters) const
{
    ADDTOCALLSTACK("CCChampion::GetMonstersPerLevel");
    ushort iTotal = 0;
    switch (_iLevel)
    {
        case 4:
            iTotal = (7 * iMonsters) / 100;   // 7% of monsters are spawned in level 4.
            break;
        case 3:
            iTotal = (13 * iMonsters) / 100;  // 13% of monsters are spawned in level 3.
            break;
        case 2:
            iTotal = (27 * iMonsters) / 100;  // 27% of monsters are spawned in level 2.
            break;
        default:
        case 1:
            iTotal = (53 * iMonsters) / 100;  // 53% of monster are spawned in level 1.
            break;
    }
    return iTotal;
}

// Delete the last created white candle.
void CCChampion::DelWhiteCandle()
{
    ADDTOCALLSTACK("CCChampion::DelWhiteCandle");
    if (_pWhiteCandles.empty())
    {
        return;
    }

    //TODO Trigger @DelWhiteCandle

    CItem * pCandle = _pWhiteCandles.back().ItemFind();
    if (pCandle)
    {
        pCandle->Delete();
    }
    _pWhiteCandles.pop_back();
}

// Delete the last created red candle.
void CCChampion::DelRedCandle()
{
    ADDTOCALLSTACK("CCChampion::DelRedCandle");
    if (_pRedCandles.empty())
    {
        return;
    }

    //TODO Trigger @DelRedCandle

    CItem * pCandle = _pRedCandles.back().ItemFind();
    if (pCandle)
    {
        pCandle->Delete();
    }
    _pRedCandles.pop_back();
}

// Clear all white candles.
void CCChampion::ClearWhiteCandles()
{
    ADDTOCALLSTACK("CCChampion::ClearWhiteCandles");
    if (_pWhiteCandles.empty())
    {
        return;
    }

    for (size_t i = 0, uiTotal = _pWhiteCandles.size(); i < uiTotal; ++i)
    {
        DelWhiteCandle();
    }
}

// Clear all red candles.
void CCChampion::ClearRedCandles()
{
    ADDTOCALLSTACK("CCChampion::ClearRedCandles");
    if (_pRedCandles.empty())
    {
        return;
    }
    for (size_t i = 0, uiTotal = _pRedCandles.size(); i < uiTotal; ++i)
    {
        DelRedCandle();
    }
}

// kill everything spawned from this spawn !
void CCChampion::KillChildren()
{
    ADDTOCALLSTACK("CCChampion:KillChildren");
    CCSpawn *pSpawn = static_cast<CCSpawn*>(GetLink()->GetComponent(COMP_SPAWN));
    if (pSpawn)
    {
        pSpawn->KillChildren();
    }
}

// Deleting one object from Spawn's memory, reallocating memory automatically.
void CCChampion::DelObj(const CUID& uid)
{
    ADDTOCALLSTACK("CCChampion:DelObj");
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
        CScript s("-e_spawn_champion");//Removing it here just for safety, preventing any additional DelObj being called from the trigger and causing an infinite loop.
        pChar->m_OEvents.r_LoadVal(s, RES_EVENTS);  //removing event from the char.
    }
    //Not checking HP or anything else, an NPC was created and counted so killing, removing or just taking it out of the lists counts towards the progression.
    OnKill();
}

// Storing one UID in Spawn's _pObj[]
void CCChampion::AddObj(const CUID& uid)
{
    ADDTOCALLSTACK("CCChampion:AddObj");
    CChar *pChar = uid.CharFind();
    if (pChar)
    {
        CScript s("events +e_spawn_champion");
        pChar->r_LoadVal(s);
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
}*/

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
}*/

enum ICHMPL_TYPE
{
    ICHMPL_ACTIVE,
    ICHMPL_ADDREDCANDLE,
    ICHMPL_ADDWHITECANDLE,
    ICHMPL_CANDLESNEXTLEVEL,
    ICHMPL_CANDLESNEXTRED,
    ICHMPL_CHAMPIONID,
    ICHMPL_CHAMPIONSPAWN,
    ICHMPL_CHAMPIONSUMMONED,
    ICHMPL_DEATHCOUNT,
    ICHMPL_KILLSNEXTRED,
    ICHMPL_KILLSNEXTWHITE,
    ICHMPL_LASTACTIVATIONTIME,
    ICHMPL_LEVEL,
    ICHMPL_LEVELMAX,
    ICHMPL_MORE,
    ICHMPL_MORE1,
    ICHMPL_NPCGROUP,
    ICHMPL_REDCANDLES,
    ICHMPL_SPAWNSCUR,
    ICHMPL_SPAWNSMAX,
    ICHMPL_WHITECANDLES,
    ICHMPL_QTY
};

lpctstr const CCChampion::sm_szLoadKeys[ICHMPL_QTY + 1] =
{
    "ACTIVE",
    "ADDREDCANDLE",
    "ADDWHITECANDLE",
    "CANDLESNEXTLEVEL",
    "CANDLESNEXTRED",
    "CHAMPIONID",
    "CHAMPIONSPAWN",
    "CHAMPIONSUMMONED",
    "DEATHCOUNT",
    "KILLSNEXTRED",
    "KILLSNEXTWHITE",
    "LASTACTIVATIONTIME",
    "LEVEL",
    "LEVELMAX",
    "MORE",
    "MORE1",
    "NPCGROUP",
    "REDCANDLES",
    "SPAWNSCUR",
    "SPAWNSMAX",
    "WHITECANDLES",
    nullptr
};

enum ICHMPV_TYPE
{
    ICHMPV_ADDOBJ,
    ICHMPV_ADDSPAWN,
    ICHMPV_DELOBJ,
    ICHMPV_DELREDCANDLE,
    ICHMPV_DELWHITECANDLE,
    ICHMPV_INIT,
    ICHMPV_MULTICREATE,
    ICHMPV_START,
    ICHMPV_STOP,
    ICHMPV_QTY
};


lpctstr const CCChampion::sm_szVerbKeys[ICHMPV_QTY + 1] =
{
    "ADDOBJ",
    "ADDSPAWN",
    "DELOBJ",
    "DELREDCANDLE",
    "DELWHITECANDLE",
    "INIT",
    "MULTICREATE",
    "START",
    "STOP",
    nullptr
};

void CCChampion::r_Write(CScript & s)
{
    ADDTOCALLSTACK("CCChampion::r_Write");
    CResourceDef* pRes = g_Cfg.RegisteredResourceGetDef(_idSpawn);
    CCChampionDef* pChampDef = static_cast<CCChampionDef*>(pRes);
    if (!pChampDef)
    {
        g_Log.EventDebug("Trying to save a champion spawn 0%" PRIx32 " with bad id 0%" PRIx32 ".\n", (dword)GetLink()->GetUID(), _idSpawn.GetPrivateUID());
        return;
    }
    s.WriteKeyVal("ACTIVE", _fActive);
    s.WriteKeyStr("CHAMPIONID", g_Cfg.ResourceGetName(CResourceID(RES_CHARDEF, _idChampion)));
    s.WriteKeyVal("CHAMPIONSUMMONED", _fChampionSummoned);
    s.WriteKeyVal("CANDLESNEXTLEVEL", _iCandlesNextLevel);
    s.WriteKeyVal("CANDLESNEXTRED", _iCandlesNextRed);
    s.WriteKeyVal("DEATHCOUNT", _iDeathCount);
    s.WriteKeyVal("KILLSNEXTRED", _iSpawnsNextRed);
    s.WriteKeyVal("KILLSNEXTWHITE", _iSpawnsNextWhite);
    s.WriteKeyVal("LASTACTIVATIONTIME", _iLastActivationTime);
    s.WriteKeyVal("LEVEL", _iLevel);
    s.WriteKeyVal("LEVELMAX", _iLevelMax);
    s.WriteKeyVal("SPAWNSCUR", _iSpawnsCur);
    s.WriteKeyVal("SPAWNSMAX", _iSpawnsMax);

    if (_idSpawn.IsValidResource())
    {
        s.WriteKeyStr("CHAMPIONSPAWN", g_Cfg.ResourceGetName(_idSpawn));
    }
    for (const CUID& uidCandle : _pRedCandles)
    {
        const CItem * pCandle = uidCandle.ItemFind();
        if (!pCandle)
            continue;   // ??
        s.WriteKeyHex("ADDREDCANDLE", (dword)uidCandle);
    }

    for (const CUID& uidCandle : _pWhiteCandles)
    {
        const CItem * pCandle = uidCandle.ItemFind();
        if (!pCandle)
            continue;   // ??
        s.WriteKeyHex("ADDWHITECANDLE", (dword)uidCandle);
    }

    if (!_spawnGroupsId.empty())
    {
        for (auto const& group : _spawnGroupsId)
        {
            std::stringstream groupStream;
            auto const& vec = group.second;
            if (vec.empty() == true)
            {
                continue;
            }
            for (CREID_TYPE npc : vec)
            {
                groupStream << g_Cfg.ResourceGetName(CResourceID(RES_CHARDEF, npc)) << ",";
            }

            std::string groupString = groupStream.str();
            groupString.pop_back(); //Remove the last comma.

            std::stringstream finalStream;
            finalStream << "npcgroup[" << (int)group.first << "]";
            
            s.WriteKeyStr(finalStream.str().c_str(), groupString.c_str());
        }
    }
    return;
}

bool CCChampion::r_WriteVal(lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc)
{
    UnreferencedParameter(pSrc);
    ADDTOCALLSTACK("CCChampion::r_WriteVal");
    int iCmd = FindTableSorted(ptcKey, sm_szLoadKeys, ARRAY_COUNT(sm_szLoadKeys) - 1);

    if (iCmd < 0)
    {
        if (!strnicmp(ptcKey, "NPCGROUP", 8))
        {
            ptcKey += 8;
            iCmd = ICHMPL_NPCGROUP;
        }
    }
    switch (iCmd)
    {
        case ICHMPL_ACTIVE:
            sVal.FormatBVal(_fActive);
            break;
        case ICHMPL_CANDLESNEXTLEVEL:
            sVal.FormatUCVal(_iCandlesNextLevel);
            break;
        case ICHMPL_CANDLESNEXTRED:
            sVal.FormatUCVal(_iCandlesNextRed);
            break;
        case ICHMPL_LASTACTIVATIONTIME:
            sVal.FormatLLVal(_iLastActivationTime);
            break;
        case ICHMPL_LEVEL:
            sVal.FormatUCVal(_iLevel);
            break;
        case ICHMPL_LEVELMAX:
            sVal.FormatUCVal(_iLevelMax);
            break;
        case ICHMPL_KILLSNEXTRED:
            sVal.FormatUSVal(_iSpawnsNextRed);
            break;
        case ICHMPL_KILLSNEXTWHITE:
            sVal.FormatUSVal(_iSpawnsNextWhite);
            break;
        case ICHMPL_REDCANDLES:
            sVal.FormatVal((int)_pRedCandles.size());
            break;
        case ICHMPL_WHITECANDLES:
            sVal.FormatVal((int)_pWhiteCandles.size());
            break;
        case ICHMPL_DEATHCOUNT:
            sVal.FormatUSVal(_iDeathCount);
            break;
        case ICHMPL_NPCGROUP:
        {
            uchar uiGroup = (uchar)Exp_GetSingle(ptcKey);
            int iSize = (int)_spawnGroupsId[uiGroup].size();    //Try to get custom spawngroups for this champion spawn.
            idSpawn spawnGroup;
            if (iSize > 0)
            {
                spawnGroup = _spawnGroupsId;
            }
            else // If it doesnt have, then try to retrieve the group from [CHAMPION ]
            {
                CResourceDef* pRes = g_Cfg.RegisteredResourceGetDef(_idSpawn);
                CCChampionDef* pChampDef = static_cast<CCChampionDef*>(pRes);
                if (pChampDef != nullptr)
                {
                    iSize = (int)pChampDef->_idSpawn[uiGroup].size();
                    if (iSize > 0)
                    {
                        spawnGroup = pChampDef->_idSpawn;
                    }
                }
            }
            if (spawnGroup.count(uiGroup) <= 0)  // Didn't found any group for the given level, stop!
            {
                sVal.FormatVal(-1);
                break;
            }
            ++ptcKey;
            uchar uiNpc = (uchar)Exp_GetSingle(ptcKey);
            size_t uiGroupSize = spawnGroup[uiGroup].size();
            if (uiNpc && (uiNpc >= uiGroupSize))
            {
                sVal.FormatVal(-1);
                break;
            }
            if (uiGroupSize >= uiNpc)
            {
                sVal = g_Cfg.ResourceGetName(CResourceID(RES_CHARDEF, spawnGroup[uiGroup][uiNpc]));
                break;
            }
            sVal.FormatVal(-1);  //Bad format?
            break;
        }
        case ICHMPL_CHAMPIONID:
        {
            sVal = g_Cfg.ResourceGetName(CResourceID(RES_CHARDEF, _idChampion));            
            break;
        }
        case ICHMPL_CHAMPIONSUMMONED:
            sVal.FormatCVal(_fChampionSummoned);
            break;
        case ICHMPL_CHAMPIONSPAWN:
        case ICHMPL_MORE:
        case ICHMPL_MORE1:
        {
            if (_idSpawn.IsValidResource())
            {
                sVal = g_Cfg.ResourceGetName(_idSpawn);
            }
            else
            {
                sVal.FormatVal(0);
            }
            break;
        }
        case ICHMPL_SPAWNSCUR:
            sVal.FormatUSVal(_iSpawnsCur);
            break;
        case ICHMPL_SPAWNSMAX:
            sVal.FormatUSVal(_iSpawnsMax);
            break;
        default:
            return false;
    }
    return true;
}

bool CCChampion::r_LoadVal(CScript & s)
{
    ADDTOCALLSTACK("CCChampion::r_LoadVal");
    int iCmd = FindTableSorted(s.GetKey(), sm_szLoadKeys, (int)ARRAY_COUNT(sm_szLoadKeys) - 1);
    lpctstr ptcKey = s.GetKey();
       
    if (iCmd < 0)
    {
        if (!strnicmp(ptcKey, "NPCGROUP", 8))
        {
            ptcKey += 8;
            iCmd = ICHMPL_NPCGROUP;
        }
    }
    switch (iCmd)
    {
        case ICHMPL_ACTIVE:
        {
            if (g_Serv.IsLoading() == true)    //Only when the server is loading.
            {
                _fActive = (bool)s.GetArgBVal();
            }
            break;
        }
        case ICHMPL_ADDREDCANDLE:
        {
            AddRedCandle(CUID(s.GetArgVal()));
            break;
        }
        case ICHMPL_ADDWHITECANDLE:
        {
            AddWhiteCandle(CUID(s.GetArgVal()));
            break;
        }
        case ICHMPL_CANDLESNEXTLEVEL:
            _iCandlesNextLevel = s.GetArgUCVal();
            break;
        case ICHMPL_CANDLESNEXTRED:
            _iCandlesNextRed = s.GetArgUCVal();
            break;
        case ICHMPL_DEATHCOUNT:
            _iDeathCount = s.GetArgUSVal();
            break;
        case ICHMPL_LASTACTIVATIONTIME:
            _iLastActivationTime = s.GetArgLLVal();
            break;
        case ICHMPL_LEVEL:
            _iLevel = s.GetArgUCVal();
            break;
        case ICHMPL_LEVELMAX:
            _iLevelMax = s.GetArgUCVal();
            break;
        case ICHMPL_NPCGROUP:
        {
            uchar iGroup = Exp_GetUCVal(ptcKey);
            tchar* piCmd[UCHAR_MAX];
            size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), piCmd, (int)ARRAY_COUNT(piCmd), ",");
            _spawnGroupsId[iGroup].clear();
            for (uint i = 0; i < iArgQty; ++i)
            {
                CREID_TYPE pCharDef = (CREID_TYPE)g_Cfg.ResourceGetIndexType(RES_CHARDEF, piCmd[i]);
                if (pCharDef)
                {
                    _spawnGroupsId[iGroup].emplace_back(pCharDef);
                }
            }
            break;
        }
        case ICHMPL_KILLSNEXTRED:
            _iSpawnsNextRed = s.GetArgUCVal();
            break;
        case ICHMPL_KILLSNEXTWHITE:
            _iSpawnsNextWhite = s.GetArgUCVal();
            break;
        case ICHMPL_SPAWNSCUR:
            _iSpawnsCur = s.GetArgUSVal();
            break;
        case ICHMPL_SPAWNSMAX:
            _iSpawnsMax = s.GetArgUSVal();
            break;
        case ICHMPL_CHAMPIONID:
            _idChampion = (CREID_TYPE)g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgStr());
            break;
        case ICHMPL_CHAMPIONSUMMONED:
            _fChampionSummoned = s.GetArgCVal();
            break;
        case ICHMPL_CHAMPIONSPAWN:
        case ICHMPL_MORE:
        case ICHMPL_MORE1:
        {
            Stop();
            const dword dwPrivateUID = s.GetArgDWVal();
            if (!CUID::IsValidUID(dwPrivateUID))
            {
                break;
            }
            CResourceIDBase ridArg(dwPrivateUID);    // Not using CResourceID because res_chardef, spawn, itemdef, template do not use the "page" arg
            const int iRidIndex = ridArg.GetResIndex();
            const int iRidType = ridArg.GetResType();
            
            if ((iRidType == RES_CHAMPION))
            {
                // If i have the ResType probably i passed a Defname
                _idSpawn = ridArg;
            }
            else
            {
                _idSpawn = CResourceIDBase(RES_CHAMPION, iRidIndex);
            }
            if (_idSpawn.IsValidUID() == false)
            {
                g_Log.EventDebug("Invalid champion id"); //todo better log
            }
            else
            {
                Init();
            }
            break;
        }
        default:
            return false;
    }
    return true;
}

void CCChampion::Delete(bool fForce)
{
    ADDTOCALLSTACK("CCChampion::Delete");
    UnreferencedParameter(fForce);
    // KillChildren is being called from CCSpawn, must not call it twice.
    ClearWhiteCandles();
    ClearRedCandles();
}

bool CCChampion::r_GetRef(lpctstr & ptcKey, CScriptObj * & pRef)
{
    ADDTOCALLSTACK("CCChampion::r_GetRef");
    if (!strnicmp(ptcKey, "WHITECANDLE", 11))
    {
        ptcKey += 11;
        size_t uiCandle = Exp_GetSTVal(ptcKey);
        SKIP_SEPARATORS(ptcKey);
        CItem * pCandle = _pWhiteCandles[uiCandle].ItemFind();
        if (pCandle)
        {
            pRef = pCandle;
            return true;
        }
    }
    if (!strnicmp(ptcKey, "REDCANDLE", 9))
    {
        ptcKey += 9;
        size_t uiCandle = Exp_GetSTVal(ptcKey);
        SKIP_SEPARATORS(ptcKey);
        CItem * pCandle = _pRedCandles[uiCandle].ItemFind();
        if (pCandle)
        {
            pRef = pCandle;
            return true;
        }
    }
    if (!strnicmp(ptcKey, "SPAWN", 5))
    {
        ptcKey += 5;
        lpctstr i = ptcKey;
        SKIP_SEPARATORS(ptcKey);
        CCSpawn * pSpawn = static_cast<CCSpawn*>(GetLink()->GetComponent(COMP_SPAWN));
        if (pSpawn)
        {
            return pSpawn->r_GetRef(i, pRef);
        }
        return true;
    }

    return false;
}

bool CCChampion::r_Verb(CScript & s, CTextConsole * pSrc)
{
    ADDTOCALLSTACK("CCChampion::r_Verb");
    UnreferencedParameter(pSrc);
    int iCmd = FindTableSorted(s.GetKey(), sm_szVerbKeys, (int)ARRAY_COUNT(sm_szVerbKeys) - 1);
    switch (iCmd)
    {
        case ICHMPV_ADDOBJ:
        {
            CUID uid(s.GetArgVal());
            if (uid.ObjFind())
                AddObj(uid);
            return true;
        }
        case ICHMPV_ADDSPAWN:
            SpawnNPC();
            return true;
        case ICHMPV_DELOBJ:
        {
            CUID uid(s.GetArgVal());
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
            Multi_Setup( pCharSrc, 0 );*/
            return true;
        }
        case ICHMPV_START:
            Start();
            return true;
        case ICHMPV_STOP:
            Stop();
            return true;
    }
    return false;
}

void CCChampion::Copy(const CComponent * target)
{
    ADDTOCALLSTACK("CCChampion::Copy");
    UnreferencedParameter(target);
    /*I don't see the point of duping a Champion, its insane and makes no sense,
    * if someone wants to totally dupe a champion it can be done from scripts.
    */
}

TRIGRET_TYPE CCChampion::OnTrigger(ITRIG_TYPE trig, CTextConsole* pSrc, CScriptTriggerArgs* pArgs)
{
    lpctstr pszTrigName = CItem::sm_szTrigName[trig];

    CResourceDef* pRes = g_Cfg.RegisteredResourceGetDef(_idSpawn);
    CCChampionDef* pChampDef = static_cast<CCChampionDef*>(pRes);
    CResourceLink* pResourceLink = static_cast <CResourceLink*>(pChampDef);
    ASSERT(pResourceLink);
    TRIGRET_TYPE iRet = TRIGRET_RET_DEFAULT;
    if (pResourceLink->HasTrigger(trig))
    {
        CResourceLock s;
        if (pResourceLink->ResourceLock(s))
        {
            iRet = GetLink()->OnTriggerScript(s, pszTrigName, pSrc, pArgs);
        }
    }
    if (iRet == TRIGRET_RET_DEFAULT)
    {
        iRet = GetLink()->OnTrigger(trig, pSrc, pArgs);
    }
    return iRet;
}


enum CHAMPIONDEF_TYPE
{
    CHAMPIONDEF_CHAMPIONID,	///< Champion ID: _iChampion.
    CHAMPIONDEF_DEFNAME,	///< Champion's DEFNAME.
    CHAMPIONDEF_LEVELMAX,   ///< Max Level for this champion.
    CHAMPIONDEF_NAME,		///< Champion name: m_sName.
    CHAMPIONDEF_NPCGROUP,	///< Monster level / group: _iSpawn[n][n].
    CHAMPIONDEF_SPAWNSMAX ,		///< Total amount of monsters: _iSpawnsMax.
    CHAMPIONDEF_QTY
};

lpctstr const CCChampionDef::sm_szLoadKeys[CHAMPIONDEF_QTY + 1] =
{
    "CHAMPIONID",
    "DEFNAME",
    "LEVELMAX",
    "NAME",
    "NPCGROUP",
    "SPAWNSMAX",
    nullptr
};


CCChampionDef::CCChampionDef(CResourceID rid) : CResourceLink(rid)
{
    ADDTOCALLSTACK("CCChampionDef::CCChampionDef");
    _iSpawnsMax = 2400;
    _iLevelMax = 4;
    _idChampion = CREID_INVALID;
}

CCChampionDef::~CCChampionDef()
{
}

bool CCChampionDef::r_WriteVal(lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren)
{
    UnreferencedParameter(pSrc);
    UnreferencedParameter(fNoCallParent);
    UnreferencedParameter(fNoCallChildren);
    ADDTOCALLSTACK("CCChampionDef::r_WriteVal");
    CHAMPIONDEF_TYPE iCmd = (CHAMPIONDEF_TYPE)(int)FindTableSorted(ptcKey, sm_szLoadKeys, CHAMPIONDEF_QTY);

    if (iCmd < 0)
    {
        if (!strnicmp(ptcKey, "NPCGROUP", 8))
        {
            ptcKey += 8;
            iCmd = CHAMPIONDEF_NPCGROUP;
        }
    }
    switch (iCmd)
    {
        case CHAMPIONDEF_CHAMPIONID:
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
                sVal.FormatVal(-1);
                return true;
            }
            uchar uiGroup = (uchar)Exp_GetSingle(ptcKey);
            ++ptcKey;
            uchar uiNPC = (uchar)Exp_GetSingle(ptcKey);
            if (uiNPC && (_idSpawn.find(uiGroup) == _idSpawn.end()))
            {
                sVal.FormatVal(-1);
                return true;
            }
            int npcCount = (int)_idSpawn.at(uiGroup).size();
            if (npcCount == 0)
            {
                sVal.FormatVal(-1);
                return true;
            }
            if ( uiNPC < npcCount )
            {
                auto npc = _idSpawn[uiGroup].at(uiNPC);
                if (npc != CREID_INVALID)
                {
                    sVal = g_Cfg.ResourceGetName(CResourceID(RES_CHARDEF, npc));
                    return true;
                }
            }
            return true;
        }
        case CHAMPIONDEF_SPAWNSMAX:
            sVal.FormatUSVal(_iSpawnsMax);
            return true;
        case CHAMPIONDEF_LEVELMAX:
            sVal.FormatUCVal(_iLevelMax);
            return true;
        default:
            return true;
    }
    return false;
}

bool CCChampionDef::r_LoadVal(CScript& s)
{
    ADDTOCALLSTACK("CCChampionDef::r_LoadVal");
    lpctstr ptcKey = s.GetKey();
    CHAMPIONDEF_TYPE iCmd = (CHAMPIONDEF_TYPE)FindTableSorted(ptcKey, sm_szLoadKeys, (int)ARRAY_COUNT(sm_szLoadKeys) - 1);

    if (iCmd < 0)
    {
        if (!strnicmp(ptcKey, "NPCGROUP", 8))
        {
            ptcKey += 8;
            iCmd = CHAMPIONDEF_NPCGROUP;
        }
    }
    switch (iCmd)
    {
    case CHAMPIONDEF_CHAMPIONID:
        _idChampion = (CREID_TYPE)g_Cfg.ResourceGetIndexType(RES_CHARDEF, s.GetArgStr());
        break;
    case CHAMPIONDEF_DEFNAME:
        return SetResourceName(s.GetArgStr());
    case CHAMPIONDEF_NAME:
        m_sName = s.GetArgStr();
        break;
    case CHAMPIONDEF_NPCGROUP:
    {
        uchar iGroup = Exp_GetUCVal(ptcKey);
        tchar* piCmd[UCHAR_MAX];
        size_t iArgQty = Str_ParseCmds(s.GetArgRaw(), piCmd, (int)ARRAY_COUNT(piCmd), ",");
        _idSpawn[iGroup].clear();
        for (uint i = 0; i < iArgQty; ++i)
        {
            CREID_TYPE pCharDef = (CREID_TYPE)g_Cfg.ResourceGetIndexType(RES_CHARDEF, piCmd[i]);
            if (pCharDef)
            {
                _idSpawn[iGroup].emplace_back(pCharDef);
            }
        }
        return true;
    }
    case CHAMPIONDEF_SPAWNSMAX:
        _iSpawnsMax = s.GetArgUSVal();
        break;
    case CHAMPIONDEF_LEVELMAX:
        _iLevelMax = s.GetArgUCVal();
        break;
    default:
        return false;
    }
    return true;
}

bool CCChampionDef::r_Load(CScript& s)
{
    if (r_LoadVal(s))
    {
        return true;
    }
    return CScriptObj::r_Load(s);
}
