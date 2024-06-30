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
#define MAXSPAWN 2400
#define MAXLEVEL 5

lpctstr const CCChampion::sm_szLoadKeys[ICHMPL_QTY + 1] =
{
    "ACTIVE",
    "ADDREDCANDLE",
    "ADDWHITECANDLE",
    "CANDLESNEXTLEVEL",
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

CCChampion::CCChampion(CItem* pLink) : CComponent(COMP_CHAMPION)
{
    ADDTOCALLSTACK("CCChampion::CCChampion");
    _pLink = pLink;
    _iLevelMax = MAXLEVEL;
    _iSpawnsMax = MAXSPAWN;
    ClearData();
    _idChampion = CREID_INVALID;
    _iLastActivationTime = 0;
    Init();
}

void CCChampion::Copy(const CComponent* target)
{
    ADDTOCALLSTACK("CCChampion::Copy");
    UnreferencedParameter(target);
    //I don't see the point of duping a Champion, its insane and makes no sense,
    // if someone wants to totally dupe a champion it can be done from scripts.
}

CCChampion::~CCChampion()
{
    ADDTOCALLSTACK("CCChampion::~CCChampion");
    ClearWhiteCandles();
    ClearRedCandles();
}

CItem* CCChampion::GetLink() const
{
    return _pLink;
}

CCRET_TYPE CCChampion::OnTickComponent()
{
    ADDTOCALLSTACK("CCChampion::_OnTick");
    if (!_pRedCandles.empty())
    {
        // TODO: Check
        // DONE
        _iSpawnsCur -= minimum(_iSpawnsCur, _iSpawnsNextRed);
        _iDeathCount -= minimum(_iDeathCount, _iSpawnsNextRed);
        DelRedCandle(CANDLEDELREASON_TIMEOUT);
    }
    else
    {
        Stop();
    }
    GetLink()->SetTimeoutS(60 * 10);
    return CCRET_CONTINUE;
}

CCSpawn* CCChampion::GetSpawnItem()
{
    return static_cast<CCSpawn*>(GetLink()->GetComponent(COMP_SPAWN));
}

void CCChampion::Init()
{
    ADDTOCALLSTACK("CCChampion::Init");

    if (_idSpawn.IsValidResource())
    {
        /*
        const CResourceIDBase rid(RES_CHAMPION, _idSpawn.GetResIndex());
        CResourceDef* pResDef = g_Cfg.RegisteredResourceGetDef(rid);
        const CCChampionDef* pChampDef = static_cast<CCChampionDef*>(pResDef);
        */

        const int resId = _idSpawn.GetResIndex();
        const CResourceIDBase rid(RES_CHAMPION, resId);
        CResourceDef* pResDef = g_Cfg.RegisteredResourceGetDef(rid);
        const CCChampionDef* pChampDef = static_cast<CCChampionDef*>(pResDef);

        if (pChampDef != nullptr)
        {
            _iLevelMax = pChampDef->_iLevelMax;
            _iSpawnsMax = pChampDef->_iSpawnsMax;
            _idChampion = pChampDef->_idChampion;
            _spawnGroupsId.clear();

            if (!pChampDef->_idSpawn.empty())
                _spawnGroupsId.insert(pChampDef->_idSpawn.begin(), pChampDef->_idSpawn.end());
            InitializeLists(); // Initialize lists.
        }
    }
}

void CCChampion::Start(CChar *pChar)
{
    ADDTOCALLSTACK("CCChampion::Start");
    // TODO store light in the area
    if (_fActive)
        return;
    _fActive = true;
    _iLastActivationTime = CWorldGameTime::GetCurrentTime().GetTimeRaw();

    // TODO: Check
    // DONE
    _iSpawnsNextRed = GetCandlesCount();

    SetLevel(1);

    if (pChar && IsTrigUsed(TRIGGER_START))
    {
        // TODO: add source?
        // DONE
        if (OnTrigger(ITRIG_Start, pChar, nullptr) == TRIGRET_RET_TRUE)
            return;
    }

    for (ushort i = 0; i < _iSpawnsNextWhite; ++i)
        SpawnNPC();
}

void CCChampion::Stop(CChar* pChar)
{
    // TODO: stop trigger
    ADDTOCALLSTACK("CCChampion::Stop");
    if (pChar)
    {
        if (IsTrigUsed(TRIGGER_STOP))
        {
            if (OnTrigger(ITRIG_STOP, pChar, nullptr) == TRIGRET_RET_TRUE)
                return;
        }
    }

    KillChildren();
    ClearData();
    GetLink()->SetTimeout(-1);
    ClearWhiteCandles();
    ClearRedCandles();
}

void CCChampion::ClearData()
{
    _fActive = false;
    _iLevel = 1;
    _iSpawnsCur = 0;
    _iDeathCount = 0;
    _iCandlesNextLevel = 0;
    m_ChampionSummoned.InitUID();
    _MonstersList.clear();
    _CandleList.clear();
}

void CCChampion::Complete()
{
    ADDTOCALLSTACK("CCChampion::Complete");
    if (_fActive)
        Stop();

    // TODO: add new trigger
    // DONE
    // TODO: Add attacker list in trigger.
    if (IsTrigUsed(TRIGGER_COMPLETE))
    {
        OnTrigger(ITRIG_COMPLETE, &g_Serv, nullptr);
    }
    // TODO: add rewards, titles, etc?
}

void CCChampion::OnKill(const CUID& uid)
{
    ADDTOCALLSTACK("CCChampion::OnKill");
    // TODO: code for on kill, also add trigger?
    if (uid.IsValidUID())
    {
        if (uid == m_ChampionSummoned)
        {
            Complete();
            return;
        }
    }

    // TODO: addwhite candle
    // DONE
    if (!_iSpawnsNextWhite)
        AddWhiteCandle();

    ++_iDeathCount;
    if (_iDeathCount >= _iSpawnsMax && !m_ChampionSummoned.IsChar())
    {
        // Is player death count exceed _iSpawnMax?
        // Force start the boss fight.
        SetLevel(_iLevelMax);
    }
    SpawnNPC();
}

void CCChampion::SpawnNPC()
{
    ADDTOCALLSTACK("CCChampion::SpawnNPC");
    CREID_TYPE pNpc = CREID_INVALID;
    CResourceIDBase rid;

    bool _fChampionSummoned = false;
    if (_iLevel >= _iLevelMax)
    {
        // Max level? Time for boss challenge!
        if (m_ChampionSummoned.IsChar()) //Is boss already spawned?
            return;
        _iSpawnsNextWhite = 1;
        _iSpawnsCur = _iSpawnsMax - 1;
        _fChampionSummoned = true;
        pNpc = _idChampion;
    }
    else
    {
        // Not ready for boss fight yet?
        size_t uiSize = _spawnGroupsId[_iLevel].size();
        idSpawn idGroup;
        if (uiSize > 0)
            idGroup = _spawnGroupsId;
        else
        {
            CResourceDef* pRes = g_Cfg.RegisteredResourceGetDef(_idSpawn);
            CCChampionDef* pChampDef = static_cast<CCChampionDef*>(pRes);
            if (pChampDef)
            {
                uiSize = pChampDef->_idSpawn[_iLevel].size();
                if (uiSize > 0)
                    idGroup = pChampDef->_idSpawn;
                else
                {
                    g_Log.EventError("CCChampion:: Trying to create NPCs from undefined NPCGROUP[%d]\n", _iLevel);
                    return;
                }
            }
        }

        if (uiSize > 0 && uiSize <= UCHAR_MAX)
        {
            uchar ucRand = (uchar)g_Rand.GetVal((int)uiSize);
            pNpc = idGroup[_iLevel][ucRand]; // Get the npc randomly from the list.
        }
        else
        {
            g_Log.EventError("CCChampion:: Champion bad group index %d.\n", _iLevel);
            return;
        }
    }

    if (_iSpawnsNextWhite == 0) // Do not spawn anymore if _iSpawnNextWhite equals to 0, should never happen but just for avoid any unnecessary loop.
        return;

    if (_iSpawnsCur >= _iSpawnsMax) // Total spawn amount should never be higher than max.
    {
        _iSpawnsNextWhite = 0; // Set Next White spawn rate to 0 to make sure not bugged.
        return;
    }

    if (!pNpc)
        return;

    rid = CResourceIDBase(RES_CHARDEF, pNpc);
    CResourceDef* pRes = g_Cfg.RegisteredResourceGetDef(rid);
    if (!pRes)
        return;

    CCSpawn* pSpawn = GetSpawnItem();
    if (pSpawn)
    {
        CChar* pChar = pSpawn->GenerateChar(rid);
        if (pChar)
        {
            AddObj(pChar->GetUID());
            if (_fChampionSummoned)
                m_ChampionSummoned = pChar->GetUID();
        }
    }
    // TODO: code spawn system.
    // DONE
    ++_iSpawnsCur;
    --_iSpawnsNextWhite;

}

void CCChampion::AddWhiteCandle(const CUID& uid)
{
    ADDTOCALLSTACK("CCChampion::AddWhiteCandle");
    // TODO: code add white candle system.
    // DONE
    if (_iLevel >= _iLevelMax)
        return;

    if (_pWhiteCandles.size() >= CANDLESNEXTRED)
    {
        AddRedCandle();
        return;
    }
    else
        _iSpawnsNextWhite = _iSpawnsNextRed / (CANDLESNEXTRED + 1);

    CItem* pCandle = nullptr;
    CItem* pLink = static_cast<CItem*>(GetLink());
    if (uid.IsValidUID())
    {
        _pWhiteCandles.emplace_back(uid);
        return;
    }

    if (!pCandle)
    {
        pCandle = pLink->CreateBase(ITEMID_SKULL_CANDLE);
        if (!pCandle)
        {
            // If cannot create candle, force boss to spawn to be able to finish.
            SetLevel(_iLevelMax);
            return;
        }
        CPointMap pt = pLink->GetTopPoint();
        switch (_pWhiteCandles.size())
        {
            case 0:
                pt.MoveN(DIR_SW, 1);
                break;
            case 1:
                pt.MoveN(DIR_SE, 1);
                break;
            case 2:
                pt.MoveN(DIR_NW, 1);
                break;
            case 3:
                pt.MoveN(DIR_NE, 1);
                break;
            default:
                break;
        }

        pCandle->SetTopPoint(pt);
        if (!g_Serv.IsLoading())
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
    pCandle->m_uidLink = pLink->GetUID();
}

void CCChampion::AddRedCandle(const CUID& uid)
{
    ADDTOCALLSTACK("CCChampion::AddRedCandle");
    // TODO: code add red candle system.
    // DONE

    CItem* pCandle = nullptr;
    CItem* pLink = static_cast<CItem*>(GetLink());
    if (uid.IsValidUID())
    {
        _pRedCandles.emplace_back(uid);
        return;
    }

    if (_pRedCandles.size() >= _iCandlesNextLevel)
        SetLevel(_iLevel + 1);

    if (_iLevel >= _iLevelMax)
        return;

    _iSpawnsNextWhite = _iSpawnsNextRed / (CANDLESNEXTRED + 1);
    if (!g_Serv.IsLoading()) // Do not remove white candles, while server is loading them from save.
    {
        ClearWhiteCandles();
    }

    if (!pCandle)
    {
        pCandle = pLink->CreateBase(ITEMID_SKULL_CANDLE);
        if (!pCandle)
        {
            // If cannot create candle, force boss to spawn to be able to finish.
            SetLevel(_iLevelMax);
            return;
        }

        CPointMap pt = pLink->GetTopPoint();
        switch (_pRedCandles.size())
        {
            case 0:
                pt.MoveN(DIR_NW, 2);
                break;
            case 1:
                pt.MoveN(DIR_N, 2);
                pt.MoveN(DIR_W, 1);
                break;
            case 2:
                pt.MoveN(DIR_N, 2);
                break;
            case 3:
                pt.MoveN(DIR_N, 2);
                pt.MoveN(DIR_E, 1);
                break;
            case 4:
                pt.MoveN(DIR_NE, 2);
                break;
            case 5:
                pt.MoveN(DIR_E, 2);
                pt.MoveN(DIR_N, 1);
                break;
            case 6:
                pt.MoveN(DIR_E, 2);
                break;
            case 7:
                pt.MoveN(DIR_E, 2);
                pt.MoveN(DIR_S, 1);
                break;
            case 8:
                pt.MoveN(DIR_SE, 2);
                break;
            case 9:
                pt.MoveN(DIR_S, 2);
                pt.MoveN(DIR_E, 1);
                break;
            case 10:
                pt.MoveN(DIR_S, 2);
                break;
            case 11:
                pt.MoveN(DIR_S, 2);
                pt.MoveN(DIR_W, 1);
                break;
            case 12:
                pt.MoveN(DIR_SW, 2);
                break;
            case 13:
                pt.MoveN(DIR_W, 2);
                pt.MoveN(DIR_S, 1);
                break;
            case 14:
                pt.MoveN(DIR_W, 2);
                break;
            case 15:
                pt.MoveN(DIR_W, 2);
                pt.MoveN(DIR_N, 1);
                break;
            default:
                break;
        }
        if (!g_Serv.IsLoading())
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
        pCandle->SetAttr(ATTR_MOVE_NEVER);
        pCandle->MoveTo(pt);
        pCandle->SetTopZ(pCandle->GetTopZ() + 4);
        pCandle->SetHue((HUE_TYPE)33);
        pCandle->Update();
        pCandle->GenerateScript(nullptr);
        ClearWhiteCandles();
    }

    _pRedCandles.emplace_back(pCandle->GetUID());
    pCandle->m_uidLink = pLink->GetUID();
}

void CCChampion::SetLevel(byte iLevel)
{
    ADDTOCALLSTACK("CCChampion::SetLevel");
    if (g_Serv.IsLoading())
        return;

    _iLevel = iLevel;
    if (_iLevel < 1)
        _iLevel = 1;

    ushort iLevelMonsters = GetMonstersCount();
    _iCandlesNextLevel += GetCandlesCount();
    // TODO: Trigger @Level (level, GetMonstersPerLevel, _iCandlesNextLevel)
    // DONE
    if (IsTrigUsed(TRIGGER_LEVEL))
    {
        CScriptTriggerArgs args(_iLevel, iLevelMonsters, _iCandlesNextLevel);
        OnTrigger(ITRIG_LEVEL, &g_Serv, &args);
    }

    if (_iLevel >= _iLevelMax) // Start boss fight when level maxed.
    {
        KillChildren();
        ClearWhiteCandles();
        ClearRedCandles();
        SpawnNPC();
        return;
    }

    // TODO: check and code
    // DONE
    // TODO: As the level increases, the light on the area decreases.
    ushort iRedMonsters = iLevelMonsters / _iCandlesNextLevel;
    ushort iWhiteMonsters = iRedMonsters / (CANDLESNEXTRED + 1);
    _iSpawnsNextWhite = iWhiteMonsters;
    _iSpawnsNextRed = iRedMonsters;
    GetLink()->SetTimeoutS(60 * 10);
}

void CCChampion::InitializeLists()
{
    ADDTOCALLSTACK("CCChampion::InitializeLists");

    /*
    * As we have _iLevelMax overrideable, we can't use static switch for it.
    * The closest algorithm I could fine for it is;
    * [(100 / _iLevelMax) / (_iLevel - 1)] + (_iLevelMax - _iLevel)
    */
    _MonstersList.clear();
    _CandleList.clear();

    uchar ucPerc = 100 / _iLevelMax;
    uchar ucMonsterTotal = 0;
    uchar ucCandleTotal = 0;
    for (uchar i = (_iLevelMax - 2); i > 0; --i)
    {
        uchar ucMonster = (ucPerc / i) + (_iLevelMax - (i + 1));
        _MonstersList.insert(_MonstersList.begin(), ucMonster); // Push the value from beginning.
        ucMonsterTotal += ucMonster;
    }
    _MonstersList.insert(_MonstersList.begin(), (100 - ucMonsterTotal)); // Push the left over as first element.

    for (uchar i = (_iLevelMax - 1); i > 1; --i)
    {
        uchar ucCandle = ((16 - ucCandleTotal) / i);
        _CandleList.insert(_CandleList.begin(), ucCandle);
        ucCandleTotal += ucCandle;
    }
    _CandleList.insert(_CandleList.begin(), (16 - ucCandleTotal));
}

// TODO: as we have hardcoded switch base level system, _iLevelMax should always equals to 5! Shouldn't be overrideable.
// DONE
uchar CCChampion::GetCandlesCount()
{
    ADDTOCALLSTACK("CCChampion::GetCandlesCount");
    if (_iLevel == UCHAR_MAX)
        return 16;

    if (_iLevel < 1) // Should never happen but put here to make sure avoid invalid index.
        _iLevel = 1;

    if (_CandleList.empty())
        InitializeLists();

    if (_CandleList.empty()) // Should never be empty after InitializeLists() but added to avoid any bug
        return 16;

    if (_iLevel <= _CandleList.size())
        return _CandleList[_iLevel - 1];
    return 16;

}

// TODO: iMonsters? Change to something specific or remove?
// DONE
ushort CCChampion::GetMonstersCount()
{
    ADDTOCALLSTACK("CCChampion::GetMonstersCount");
    if (_iLevel < 1) // Should never happen but put here to make sure avoid invalid index.
        _iLevel = 1;

    if (_MonstersList.empty())
        InitializeLists();

    if (_MonstersList.empty()) // Should never be empty after InitializeLists() but added to avoid any bug
        return 1;

    if (_iLevel <= _MonstersList.size())
    {
        ushort ucPerc = (ushort)_MonstersList[_iLevel - 1];
        return (ucPerc * _iSpawnsMax) / 100;
    }
    return 1;
}

// Delete the last created white candle.
void CCChampion::DelWhiteCandle(CANDLEDELREASON_TYPE reason)
{
    ADDTOCALLSTACK("CCChampion::DelWhiteCandle");
    if (_pWhiteCandles.empty())
        return;

    CItem* pCandle = _pWhiteCandles.back().ItemFind();
    if (pCandle)
    {
        // TODO: trigger: @DelWhiteCandle
    // DONE
        if (IsTrigUsed(TRIGGER_DELWHITECANDLE))
        {
            CScriptTriggerArgs args(reason);
            args.m_pO1 = pCandle;
            if (OnTrigger(ITRIG_DELWHITECANDLE, &g_Serv, &args) == TRIGRET_RET_TRUE)
                return;
        }

        if (pCandle) // Is candle still exists after trigger?
            pCandle->Delete();
    }
    _pWhiteCandles.pop_back();
}

// Delete the last created red candle.
void CCChampion::DelRedCandle(CANDLEDELREASON_TYPE reason)
{
    ADDTOCALLSTACK("CCChampion::DelRedCandle");
    if (_pRedCandles.empty())
        return;

    CItem* pCandle = _pRedCandles.back().ItemFind();
    if (pCandle)
    {
        // TODO: trigger: @DelRedCandle
        // DONE
        if (IsTrigUsed(TRIGGER_DELREDCANDLE))
        {
            CScriptTriggerArgs args(reason);
            args.m_pO1 = pCandle;
            if (OnTrigger(ITRIG_DELREDCANDLE, &g_Serv, &args) == TRIGRET_RET_TRUE)
                return;
        }

        if (pCandle) // Is candle still exists after trigger?
            pCandle->Delete();
    }
    _pRedCandles.pop_back();
}

// Clear all white candles.
void CCChampion::ClearWhiteCandles()
{
    if (_pWhiteCandles.empty())
        return;

    for (size_t i = 0, iTotal = _pWhiteCandles.size(); i < iTotal; ++i)
        DelWhiteCandle();
}

// Clear all red candles.
void CCChampion::ClearRedCandles()
{
    if (_pRedCandles.empty())
        return;

    for (size_t i = 0, iTotal = _pRedCandles.size(); i < iTotal; ++i)
        DelRedCandle(CANDLEDELREASON_CLEAR);
}

// kill everything spawned from this spawn !
void CCChampion::KillChildren()
{
    ADDTOCALLSTACK("CCChampion:KillChildren");
    CCSpawn *pSpawn = GetSpawnItem();
    if (pSpawn)
        pSpawn->KillChildren();
}

// Deleting one object from Spawn's memory, reallocating memory automatically.
void CCChampion::DelObj(const CUID& uid)
{
    ADDTOCALLSTACK("CCChampion:DelObj");
    if (!uid.IsValidUID())
    {
        return;
    }
    CCSpawn* pSpawn = static_cast<CCSpawn*>(GetLink()->GetComponent(COMP_SPAWN));
    ASSERT(pSpawn);
    pSpawn->DelObj(uid);

    CChar* pChar = uid.CharFind();
    if (pChar)
    {
        // Should it called in any time? As DelObj called when obj deleting?
        CScript s("-e_spawn_champion");//Removing it here just for safety, preventing any additional DelObj being called from the trigger and causing an infinite loop.
        pChar->m_OEvents.r_LoadVal(s, RES_EVENTS);  //removing event from the char.
        OnKill(uid);
    }
    //Not checking HP or anything else, an NPC was created and counted so killing, removing or just taking it out of the lists counts towards the progression.
    OnKill(uid);
}

// Storing one UID in Spawn's _pObj[]
void CCChampion::AddObj(const CUID& uid)
{
    ADDTOCALLSTACK("CCChampion:AddObj");
    CChar* pChar = uid.CharFind();
    if (pChar)
    {
        // TODO: check if event exists.
        // DONE
        if (IsValidResourceDef("e_spawn_champion"))
        {
            CScript s("events +e_spawn_champion");
            pChar->r_LoadVal(s);
        }
    }
}

void CCChampion::r_Write(CScript& s)
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
    s.WriteKeyVal("CHAMPIONSUMMONED", m_ChampionSummoned.GetObjUID());
    s.WriteKeyVal("CANDLESNEXTLEVEL", _iCandlesNextLevel);
    s.WriteKeyVal("DEATHCOUNT", _iDeathCount);
    s.WriteKeyVal("KILLSNEXTRED", _iSpawnsNextRed);
    s.WriteKeyVal("KILLSNEXTWHITE", _iSpawnsNextWhite);
    s.WriteKeyVal("LASTACTIVATIONTIME", _iLastActivationTime);
    s.WriteKeyVal("LEVEL", _iLevel);
    s.WriteKeyVal("LEVELMAX", _iLevelMax);
    s.WriteKeyVal("SPAWNSCUR", _iSpawnsCur);
    s.WriteKeyVal("SPAWNSMAX", _iSpawnsMax);

    if (_idSpawn.IsValidResource())
        s.WriteKeyStr("CHAMPIONSPAWN", g_Cfg.ResourceGetName(_idSpawn));

    for (const CUID& uidCandle : _pRedCandles)
    {
        const CObjBase* pCandle = uidCandle.ObjFind();
        if (!pCandle)
            continue;
        s.WriteKeyHex("ADDREDCANDLE", uidCandle.GetObjUID());
    }

    for (const CUID& uidCandle : _pWhiteCandles)
    {
        const CObjBase* pCandle = uidCandle.ObjFind();
        if (!pCandle)
            continue;
        s.WriteKeyHex("ADDWHITECANDLE", uidCandle.GetObjUID());
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

bool CCChampion::r_WriteVal(lpctstr ptcKey, CSString& sVal, CTextConsole* pSrc)
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
        sVal.FormatDWVal(m_ChampionSummoned.GetObjUID());
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

bool CCChampion::r_LoadVal(CScript& s)
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
            AddRedCandle(CUID(s.GetArgDWVal()));
            break;
        }
    case ICHMPL_ADDWHITECANDLE:
        {
            AddWhiteCandle(CUID(s.GetArgDWVal()));
            break;
        }
    case ICHMPL_CANDLESNEXTLEVEL:
        _iCandlesNextLevel = s.GetArgUCVal();
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
        m_ChampionSummoned.SetObjUID(s.GetArgDWVal());
        break;
    case ICHMPL_CHAMPIONSPAWN:
    case ICHMPL_MORE:
    case ICHMPL_MORE1:
        {
            const CUID uid(s.GetArgDWVal());
            if (!uid.IsValidUID())
                return true; // Should return true because we don't want to see undefined keyword exception.

            if (!uid.IsValidResource())
                return true;

            CResourceIDBase rid(uid.GetPrivateUID());
            _idSpawn = (rid.GetResType() == RES_CHAMPION ? rid : CResourceIDBase(RES_CHAMPION, rid.GetResIndex()));

            if (!_idSpawn.IsValidUID())
            {
                g_Log.EventDebug("Invalid champion id, champion spawn stopped. uid=0%x\n", (dword)uid);
                Stop();
                return true;
            }
            Init();
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
    //UnreferencedParameter(pSrc);
    int iCmd = FindTableSorted(s.GetKey(), sm_szVerbKeys, (int)ARRAY_COUNT(sm_szVerbKeys) - 1);
    CChar* pCharSrc = pSrc->GetChar();

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
            DelRedCandle(CANDLEDELREASON_COMMAND);
            return true;
        case ICHMPV_DELWHITECANDLE:
            DelWhiteCandle(CANDLEDELREASON_COMMAND);
            return true;
        case ICHMPV_INIT:
            Init();
            return true;
        case ICHMPV_MULTICREATE:
        {
            //CUID	uid( s.GetArgVal() );   // FIXME: ECS link with CItemMulti.
            //CChar *	pCharSrc = uid.CharFind();
            //Multi_Setup( pCharSrc, 0 );
            return true;
        }
        case ICHMPV_START:
            Start(pCharSrc);
            return true;
        case ICHMPV_STOP:
            Stop(pCharSrc);
            return true;
    }
    return false;
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
    _iSpawnsMax = MAXSPAWN;
    _iLevelMax = MAXLEVEL;
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
