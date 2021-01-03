/**
* @file CSector.h
*
*/

#ifndef _INC_CSECTOR_H
#define _INC_CSECTOR_H

#include "../common/CScriptObj.h"
#include "CSectorEnviron.h"
#include "CSectorTemplate.h"
#include "CTimedObject.h"


class CChar;
class CItemStone;

class CSector : public CScriptObj, public CSectorBase, public CTimedObject	// square region of the world.
{
	// A square region of the world. ex: MAP0.MUL Dungeon Sectors are 256 by 256 meters
#define SECTOR_TICK_PERIOD (TICKS_PER_SEC / 2) // after how much ticks do we start a pulse.

public:
	static const char *m_sClassName;
	static lpctstr const sm_szVerbKeys[];
	static lpctstr const sm_szLoadKeys[];

private:
	bool   m_fSaveParity;		// has the sector been saved relative to the char entering it ?
	CSectorEnviron m_Env;		// Current Environment

	byte m_RainChance;		// 0 to 100%
	byte m_ColdChance;		// Will be snow if rain chance success.
	byte m_ListenItems;		// Items on the ground that listen ?

private:
	WEATHER_TYPE GetWeatherCalc() const;
	byte GetLightCalc( bool fQuickSet ) const;
	void SetLightNow( bool fFlash = false );
	bool IsMoonVisible( uint iPhase, int iLocalTime ) const;
	void SetDefaultWeatherChance();

public:
	CSector();
	~CSector();

private:
	CSector(const CSector& copy);
	CSector& operator=(const CSector& other);

public:
	virtual void Init(int index, uchar map, short x, short y) override;
	virtual bool OnTick();
	virtual bool IsDeleted() const override;

	// Time
	int GetLocalTime() const;
	lpctstr GetLocalGameTime() const;

	// Season
	SEASON_TYPE GetSeason() const;
	void SetSeason( SEASON_TYPE season );

	// Weather
	WEATHER_TYPE GetWeather() const;
	bool IsRainOverriden() const;
	byte GetRainChance() const;
	bool IsColdOverriden() const;
	byte GetColdChance() const;
	void SetWeather( WEATHER_TYPE w );
	void SetWeatherChance( bool fRain, int iChance );

	// Light
	bool IsLightOverriden() const;
	byte GetLight() const;
	bool IsDark() const;
	bool IsNight() const;
	void LightFlash();
	void SetLight( int light );

	// Items in the sector
	size_t GetItemComplexity() const;
	void CheckItemComplexity() const noexcept;
	bool IsItemInSector( const CItem * pItem ) const;
	void MoveItemToSector( CItem * pItem );

	void AddListenItem();
	void RemoveListenItem();
	bool HasListenItems() const;
	void OnHearItem( CChar * pChar, lpctstr pszText );

	// Chars in the sector.
	size_t GetCharComplexity() const;
	void CheckCharComplexity() const noexcept;
	bool IsCharActiveIn( const CChar * pChar );
	bool IsCharDisconnectedIn( const CChar * pChar );
	size_t GetInactiveChars() const;
	size_t GetClientsNumber() const;
	int64 GetLastClientTime() const;
	bool CanSleep(bool fCheckAdjacents) const;
	void SetSectorWakeStatus();	// Ships may enter a sector before it's riders !
	bool MoveCharToSector( CChar * pChar );

	// CTimedObject
private:
    virtual void GoSleep() override;
    virtual void GoAwake() override;

    // General.
public:
	virtual bool r_LoadVal( CScript & s ) override;
	virtual bool r_WriteVal( lpctstr ptcKey, CSString & sVal, CTextConsole * pSrc = nullptr, bool fNoCallParent = false, bool fNoCallChildren = false ) override;
	virtual void r_Write();
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc ) override;

	// AllThings Verbs
	bool v_AllChars( CScript & s, CTextConsole * pSrc );
	bool v_AllCharsIdle( CScript & s, CTextConsole * pSrc );
	bool v_AllItems( CScript & s, CTextConsole * pSrc );
	bool v_AllClients( CScript & s, CTextConsole * pSrc );

	// Other resources.
	void Restock();
	void RespawnDeadNPCs();

	void Close();
	lpctstr GetName() const { return "Sector"; }
};


#endif // _INC_CSECTOR_H
