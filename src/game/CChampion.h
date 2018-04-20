
#include "items/CItemMulti.h"
#include "chars/CChar.h"
#include "uo_files/uofiles_enums_creid.h"
#include "../common/sphere_library/CSString.h"
#include "../common/datatypes.h"
#include "../common/CResourceBase.h"

/**
* @brief This class manages a CItem making it to work as a Champion.
*/
class CItemChampion : public CItemMulti
{
    // IT_CHAMPION
    // Champion Spawn
private:
    static LPCTSTR const sm_szLoadKeys[];   ///< Script fields.
    static LPCTSTR const sm_szVerbKeys[];   ///< Action list.

    bool _fActive;                 ///< Champion status
    long long m_iLastActivationTime;///< raw time of the last activation, its supossed to have 6h between last one and next one.
    BYTE m_RedCandles;              ///< This handles the champion's 'level'
    BYTE m_WhiteCandles;            ///< Sublevel: 4 white candles = increasing 1 red candle. (Max = 4)
    BYTE m_LevelMax;                ///< maximum level this champion can reach. (Max = 16)

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
    BYTE m_Level;
    unsigned short m_iSpawnsMax;        ///< Max amount of monsters OSI default value = 2368.
    unsigned short m_iSpawnsCur;        ///< TOTAL Current created monsters ( alive + dead ones).
    /**
    @brief Total monsters needed to gain the next Red candle.
    *
    * This is based on the total amount of monsters and this percentaje (calculated from OSI's default 'm_iSpawnsMax'):
    * Level 1: 256 (53%)
    * Level 2: 128 (27%)
    * Level 3: 64 (13%)
    * Level 4: 32 (7%)
    */
    unsigned short m_iSpawnsNextRed;
    unsigned short m_iSpawnsNextWhite;  ///< Total monsters needed to the next White candle ( m_iSpawnsNextRed / 5 ).
    unsigned short m_iDeathCount;       ///< Keeping track of how many monsters died already.
    //CResource m_iSpawn[4][4];         ///< Storing in memory monster's id: m_iSpawn[ChampionPhases][MonsterIDForThisLevel], default = 4 different monsters per Level.
    CItem * m_RedCandle[16];            ///< Storing the Red Candles.
    CItem * m_WhiteCandle[4];           ///< Storing the White Candles.
    CUID m_obj[UCHAR_MAX];          ///< CItemSpawn's replica to store spawned characters.


public:
    static const char *m_sClassName;    ///< Class definition
    /**
    * @brief Start the champion, setup everything needed.
    *
    * Calls SetLevel(1), which automatically set the relevant data to work from now.
    * SetTimeout( TICK_PER_SEC * 60 * 10 ) // 10 minutes.
    */
    void Init();
    /**
    * @brief Stop the champion, cleaning everything.
    *
    * Clean killed monsters, level, spawned monsters and required monsters per both next White candle and Red candle.
    */
    void Stop();
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
    void OnKill();
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
    void AddWhiteCandle(CUID uid = UID_UNUSED);
    /**
    * @brief Red candles check, add new one or I am in boss phase?
    *
    * @param uid, if given (mostly during world loading) instead of creating a new candle, only the uid will be set to hold the already existing candle and do not have duplicates.
    */
    void AddRedCandle(CUID uid = UID_UNUSED);
    /**
    * @brief Deleting last added White Candle.
    */
    void DelWhiteCandle();
    /**
    * @brief Deleting last added Red Candle.d.
    */
    void DelRedCandle();
    /**
    * @brief Clear all White Candles.
    */
    void ClearWhiteCandles();
    /**
    * @brief Clear all Red Candles.
    */
    void ClearRedCandles();
    /**
    * @brief CItemSpawn's replica: Remove all spawned characters.
    */
    void KillChildren();
    /**
    * @brief CItemSpawn's replica: Add one char to this 'spawn'.
    *
    * @param uid UID of the character to add to the 'spawn'.
    */
    void AddObj(CUID uid);
    /**
    * @brief CItemSpawn's replica: Remove one char from this 'spawn'.
    *
    * @param uid UID of the character to remove from the 'spawn'.
    */
    void DelObj(CUID uid);
    /**
    * @brief How many monsters do we need to kill to gain a White Candle.
    *
    * @param iMonsters total amount of monsters of the champion.
    * @return amount of killed monsters needed to reach the next level.
    */
    unsigned short GetMonstersPerLevel(unsigned short iMonsters);
    /**
    * @brief Retrieves how much Red Candles are needed to reach next Champion's Level.
    *
    * More detailed description ...
    * Called from SetLevel() to set the amount of candles.
    * Value stored in morex (m_itNormal.m_morep.m_x).
    * @return how much red candles are needed to reach the next level
    */
    BYTE GetCandlesPerLevel();
    /**
    * @brief Set a new Level for this champion.
    *
    * More detailed description ...
    * Called from AddRedCandle() when RedCandles > GetCandlesPerLevel()
    * Sets the new amount of Red Candles needed for the next level ( GetCandlesPerLevel() )
    * Checks how many monsters are needed for each White Candles and the total for the next Red Candle.
    * Set m_iSpawnsNextWhite to the amount of killed monsters needed for next white candle.
    * Set m_iSpawnsNextRed to the amount of monsters needed to kill to reach next red candle.
    * Reset timeout to 10 minutes.
    * @param iLevel the new Level for the champion.
    */
    void SetLevel(BYTE iLevel);

    /************************************************************************
    * SCP related section.
    ************************************************************************/
    bool r_GetRef(LPCTSTR & pszKey, CScriptObj * & pRef);
    void r_Write(CScript & s);
    bool r_WriteVal(LPCTSTR pszKey, CSString & sVal, CTextConsole * pSrc);
    bool r_LoadVal(CScript & s);
    bool r_Verb(CScript & s, CTextConsole * pSrc); // Execute command from script

    /************************************************************************
    * CItem related section.
    ************************************************************************/
    CItemChampion(ITEMID_TYPE id, CItemBase * pItemDef);
    /**
    @brief Champion is being removed, spawns and candles must be removed too!
    */
    virtual ~CItemChampion();
    /**
    * @brief Timer expired, overriding default tick:
    *
    * timeout is set to 10 minutes when a new Level is gained or when Champion starts.
    * if players don't gain the needed candles for a new Level before timer expires, a red candle is removed.
    */
    virtual bool OnTick();

};



/**
* @brief Storing information from scripted [CHAMPION ] resource
*/
class CChampionDef : public CResourceLink
{

    //static LPCTSTR const sm_szTrigName[CHAMPIONTRIG_QTY+1];
    static LPCTSTR const sm_szLoadKeys[];
    static const char *m_sClassName;
private:
    CSString m_sKey;				///< script key word for champion.
public:
    CSString m_sName;				///< Champion name
    int8 m_iType;					///< Champion type
    uint16 m_iSpawnsMax;	///< Max amount of monsters OSI default value = 480.
    CREID_TYPE m_iSpawn[4][4];		///< Storing in memory monster's id: m_iSpawn[ChampionPhases][MonsterIDForThisLevel], default = 4 different monsters per Level.
    CREID_TYPE m_Champion;			///< Boss id
    explicit CChampionDef(CResourceID rid);
    virtual ~CChampionDef();
    lpctstr GetName() const { return(m_sName); }
    bool r_WriteVal(LPCTSTR pszKey, CSString & sVal, CTextConsole * pSrc);
    bool r_LoadVal(CScript & s);
};