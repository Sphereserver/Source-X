/**
* @file CCChampion.h
*/

#ifndef _INC_CCCHAMPION_H
#define _INC_CCCHAMPION_H

#include "../../common/sphere_library/CSString.h"
#include "../../common/resource/CResourceHolder.h"
#include "../../common/CScriptTriggerArgs.h"
#include "../uo_files/uofiles_enums_creid.h"
#include "../items/CItemMulti.h"
#include "../chars/CChar.h"
#include "../triggers.h"
#include "CCSpawn.h"

enum CHAMPION_ID
{
    // Original champions.
    CHAMPION_ABYSS = 1,
    CHAMPION_ARACHNID,
    CHAMPION_COLD_BLOOD,
    CHAMPION_FOREST_LORD,
    CHAMPION_UNHOLY_TERROR,
    CHAMPION_VERMIN_HORDE,
    // SE Champion.
    CHAMPION_SLEEPING_DRAGON,
    // ML Champions.
    CHAMPION_MINOTAUR,
    CHAMPION_CORRUPT,
    CHAMPION_GLADE,
    // SA Champions.
    CHAMPION_ABYSSAL_INFERNAL,
    CHAMPION_PRIMEVAL_LICH,
    // TOL Champion.
    CHAMPION_DRAGON_TURTLE,

    CHAMPION_QTY // End of OSI defined Champion spawns.
};

enum ICHMPL_TYPE
{
    ICHMPL_ACTIVE,
    ICHMPL_ADDREDCANDLE,
    ICHMPL_ADDWHITECANDLE,
    ICHMPL_CANDLESNEXTLEVEL,
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

enum CHAMPIONDEF_TYPE
{
    CHAMPIONDEF_CHAMPIONID,	///< Champion ID: _iChampion.
    CHAMPIONDEF_DEFNAME,	///< Champion's DEFNAME.
    CHAMPIONDEF_LEVELMAX,   ///< Max Level for this champion.
    CHAMPIONDEF_NAME,		///< Champion name: m_sName.
    CHAMPIONDEF_NPCGROUP,	///< Monster level / group: _iSpawn[n][n].
    CHAMPIONDEF_SPAWNSMAX,		///< Total amount of monsters: _iSpawnsMax.
    CHAMPIONDEF_QTY
};

enum CANDLEDELREASON_TYPE
{
    CANDLEDELREASON_TIMEOUT,
    CANDLEDELREASON_COMMAND,
    CANDLEDELREASON_CLEAR,
    CANDLEDELREASON_QTY
};

class CObjBase;
class CItem;
class CChar;

/**
* @brief This class manages a CItem making it to work as a Champion.
*/
class CCChampion : public CComponent
{
    // IT_CHAMPION
    // Champion Spawn
private:
    CItem *_pLink;
    // Scripts communication.
    static lpctstr const sm_szLoadKeys[];   ///< Script fields.
    static lpctstr const sm_szVerbKeys[];   ///< Action list.

    // Retrieved from CCChampionDef
    typedef std::vector<CREID_TYPE> idNPC;
    typedef std::map<uchar, idNPC> idSpawn;
    idSpawn _spawnGroupsId;     ///< Defining how many uchar (or levels) this Champion has and the group of monsters for each level.
    CResourceIDBase _idSpawn;   ///< legacy more1=ID of the Object to Spawn.
    CREID_TYPE _idChampion;     ///< Boss id
    CUID m_ChampionSummoned;    ///< True if the champion's boss has been summoned already (wether it was killed or not).

    std::vector<uchar> _MonstersList;
    std::vector<uchar> _CandleList;

    // Ingame Spawn behaviour.
    bool _fActive;                  ///< Champion status
    int64 _iLastActivationTime;     ///< raw time of the last activation, its supossed to have 6h between last one and next one.

    /**
    * @brief Current level
    *
    * Red candles needed per Level:
    * Level 1: 1 to 5/6 = 31.25 - 37.5%
    * Level 2: 5/6 to 9/10 = 25%
    * Level 3: 9/10 to 13/14 = 25%
    * Level 4: 13/14 to 16 = 12.5 - 18.5%
    * these OSI values are based on some unknown factors, final percentajes used: 38 - 25 - 25 - 12.
    */
    uchar _iLevel;
    uchar _iLevelMax;        ///< Max level should be 16 (4 Red candles * 4 White candles per red one), but it may be modified by script.
    ushort _iSpawnsMax;     ///< Max amount of monsters OSI default value = 2368.
    ushort _iSpawnsCur;     ///< TOTAL Current created monsters ( alive + dead ones).
    /**
    @brief Total monsters needed to gain the next Red candle.
    *
    * This is based on the total amount of monsters and this percentaje (calculated from OSI's default '_iSpawnsMax'):
    * Level 1: 256 (53%)
    * Level 2: 128 (27%)
    * Level 3: 64 (13%)
    * Level 4: 32 (7%)
    */
    ushort _iSpawnsNextRed;
    ushort _iSpawnsNextWhite;           ///< Total monsters needed to the next White candle ( m_iSpawnsNextRed / 5 ).
    ushort _iDeathCount;                ///< Keeping track of how many monsters died already.
    uchar _iCandlesNextLevel;           ///< Required amount of Red candles to reach the next level.
    std::vector<CUID> _pRedCandles;   ///< Storing the Red Candles, reaching a certain amount of these increases the Champion's Level (refer to _iLevel).
    std::vector<CUID> _pWhiteCandles; ///< Storing the White Candles (sublevel): 4 white candles = 1 red candle (Default max = 4).

    /**
    * @brief Calculates monsters and candles needed per level and saves them to _MonsterList and _CandleList to avoid unnecessary loops.
    */
    void InitializeLists();

public:
    //static const char *m_sClassName;    ///< Class definition

    CCSpawn* GetSpawnItem();

    /**
    * @brief Start the champion, setup everything needed.
    *
    * Retrieves relevant data from [CHAMPION ] definition.
    */
    void Init();
    /**
    * @brief Start the champion.
    *
    * Calls SetLevel(1), which automatically set the relevant data to work from now.
    * SetTimeoutS( 60 * 10 ) // 10 minutes.
    */
    void Start(CChar* pChar = nullptr);
    /**
    * @brief Stop the champion, cleaning everything.
    *
    * Clean killed monsters, level, spawned monsters and required monsters per both next White candle and Red candle.
    */
    void Stop(CChar* pChar = nullptr);
    /**
    * @brief Just cleaning everything.
    */
    void ClearData();
    /**
    * @brief Everything is done, call Stop.
    *
    * Calls Stop() to clean everything
    * TODO: generate rewards
    */
    void Complete();
    /**
    * @brief  A monster was killed
    *
    * if m_iSpawnsNextWhite < 1 calls AddWhiteCandle
    * otherwise sustracts one monsters from m_iSpawnsNextWhite
    */
    void OnKill(const CUID& uid = CUID());
    /**
    * @brief Spawns a new character
    *
    * check champion's level to choose the next NPC
    * check if next NPC is normal or champion.
    */
    void SpawnNPC();
    /**
    * @brief White candles check, add new one or add a red one?
    *
    * More detailed description ...
    * If m_WhiteCandles == 4 resets m_iSpawnsNextWhite to the required monsters for the next White Candle.
    * otherwise it calls AddRedCandle and stop execution.
    * Create a pCandle pointer and set it's properties, color, and places it or sets the pointer to the item given in the param 'uid'.
    * Store in m_WhiteCandle[m_WhiteCandles] the CItem stored in pCandle.
    * @param uid, if given (mostly during world loading) instead of creating a new candle, only the uid will be set to hold the already existing candle and do not have duplicates.
    */
    void AddWhiteCandle(const CUID& uid = CUID());
    /**
    * @brief Red candles check, add new one or I am in boss phase?
    *
    * @param uid, if given (mostly during world loading) instead of creating a new candle, only the uid will be set to hold the already existing candle and do not have duplicates.
    */
    void AddRedCandle(const CUID& uid = CUID());
    /**
    * @brief Deleting last added White Candle.
    */
    void DelWhiteCandle(CANDLEDELREASON_TYPE reason = CANDLEDELREASON_CLEAR);
    /**
    * @brief Deleting last added Red Candle.d.
    */
    void DelRedCandle(CANDLEDELREASON_TYPE reason = CANDLEDELREASON_CLEAR);
    /**
    * @brief Clear all White Candles.
    */
    void ClearWhiteCandles();
    /**
    * @brief Clear all Red Candles.
    */
    void ClearRedCandles();
    /**
    * @brief CCSpawn's replica: Remove all spawned characters.
    */
    void KillChildren();
    /**
    * @brief CCSpawn's replica: Add one char to this 'spawn'.
    *
    * @param uid UID of the character to add to the 'spawn'.
    */
    void AddObj(const CUID& uid);
    /**
    * @brief CCSpawn's replica: Remove one char from this 'spawn'.
    *
    * @param uid UID of the character to remove from the 'spawn'.
    */
    void DelObj(const CUID& uid);

    /**
    * @brief How many monsters do we need to kill to gain a White Candle.
    *
    * @param iMonsters total amount of monsters of the champion.
    * @return amount of killed monsters needed to reach the next level.
    */
    uint16 GetMonstersCount();

    /**
    * @brief Retrieves how much Red Candles are needed to reach next Champion's Level.
    *
    * @param iLevel the level to check for (0 = reach for next Level).
    * More detailed description ...
    * Called from SetLevel() to set the amount of candles.
    * Value stored in morex (m_itNormal.m_morep.m_x).
    * @return how much red candles are needed to reach the next level
    */
    uchar GetCandlesCount();
    /**
    * @brief Set a new Level for this champion.
    *
    * More detailed description ...
    * Called from AddRedCandle() when RedCandles > GetCandlesPerLevel()
    * Sets the new amount of Red Candles needed for the next level ( GetCandlesPerLevel() )
    * Checks how many monsters are needed for each White Candles and the total for the next Red Candle.
    * Set _iSpawnsNextWhite to the amount of killed monsters needed for next white candle.
    * Set _iSpawnsNextRed to the amount of monsters needed to kill to reach next red candle.
    * Reset timeout to 10 minutes.
    * @param iLevel the new Level for the champion.
    */
    void SetLevel(byte iLevel);

    /************************************************************************
    * SCP related section.
    ************************************************************************/
    virtual void Delete(bool fForce = false) override;
    virtual bool r_GetRef(lpctstr & ptcKey, CScriptObj * & pRef) override;
    virtual void r_Write(CScript & s) override;
    virtual bool r_WriteVal(lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc) override;
    virtual bool r_LoadVal(CScript & s) override;
    virtual bool r_Verb(CScript & s, CTextConsole * pSrc) override; // Execute command from script
    virtual void Copy(const CComponent *target) override;
    TRIGRET_TYPE OnTrigger(ITRIG_TYPE trig, CTextConsole *pSrc, CScriptTriggerArgs *args);

    /************************************************************************
    * CItem related section.
    ************************************************************************/
    CCChampion(CItem *pLink);
    /**
    @brief Champion is being removed, spawns and candles must be removed too!
    */
    virtual ~CCChampion();

    CItem *GetLink() const;

    /**
    * @brief Timer expired, overriding default tick:
    *
    * timeout is set to 10 minutes when a new Level is gained or when Champion starts.
    * if players don't gain the needed candles for a new Level before timer expires, a red candle is removed.
    */
    virtual CCRET_TYPE OnTickComponent() override;

};



/**
* @brief Storing information from scripted [CHAMPION ] resource
*/
class CCChampionDef : public CResourceLink
{

    //static lpctstr const sm_szTrigName[CHAMPIONTRIG_QTY+1];
    static lpctstr const sm_szLoadKeys[];
    static const char *m_sClassName;
private:
    CSString m_sKey;                    ///< script keyword for champion.
    typedef std::vector<CREID_TYPE> idNPC;
    typedef std::map<uchar, idNPC> idSpawn;    ///< Defining how many uchar (or levels) this Champion has and the group of monsters for each level.
public:
    CSString m_sName;                   ///< Champion name
    ushort _iSpawnsMax;                 ///< Max amount of monsters OSI default value = 480.
    uchar _iLevelMax;                   ///< Max level this champion can have
    idSpawn _idSpawn;
    CREID_TYPE _idChampion;             ///< Boss id

    explicit CCChampionDef(CResourceID rid);
    virtual ~CCChampionDef();
    virtual lpctstr GetName() const override { return(m_sName); }
    virtual bool r_WriteVal(lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false) override;
    virtual bool r_LoadVal(CScript& s) override;
    virtual bool r_Load(CScript& s) override;

};

#endif //_INC_CCCHAMPION_H
