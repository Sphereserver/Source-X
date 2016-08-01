/**
* @file CObjBase.h
*
*/


#pragma once
#ifndef _INC_COBJBASE_H
#define _INC_COBJBASE_H

#include "../common/CObjBaseTemplate.h"
#include "../common/CScriptObj.h"
#include "../common/CResourceBase.h"
#include "CServerTime.h"
#include "CContainer.h"
#include "CBase.h"
#include "CResource.h"
#include "CWorld.h"


class CBaseBase;

/**
 * @enum    MEMORY_TYPE
 *
 * @brief   Values that represent memory types ( IT_EQ_MEMORY_OBJ ).
 *
 * Types of memory a CChar has about a game object. (m_wHue)
 */
enum MEMORY_TYPE
{
	MEMORY_NONE         = 0,
	MEMORY_SAWCRIME     = 0x0001,	///< I saw them commit a crime or i was attacked criminally. I can call the guards on them. the crime may not have been against me.
	MEMORY_IPET         = 0x0002,	///< I am a pet. (this link is my master) (never time out)
	MEMORY_FIGHT        = 0x0004,	///< Active fight going on now. may not have done any damage. and they may not know they are fighting me.
	MEMORY_IAGGRESSOR   = 0x0008,	///< I was the agressor here. (good or evil)
	MEMORY_HARMEDBY     = 0x0010,	///< I was harmed by them. (but they may have been retaliating)
	MEMORY_IRRITATEDBY  = 0x0020,	///< I saw them snoop from me or someone.
	MEMORY_SPEAK        = 0x0040,	///< We spoke about something at some point. (or was tamed) (NPC_MEM_ACT_TYPE)
	MEMORY_AGGREIVED    = 0x0080,	///< I was attacked and was the inocent party here !
	MEMORY_GUARD        = 0x0100,	///< Guard this item (never time out)
	MEMORY_ISPAWNED     = 0x0200,	///< UNUSED!!!! I am spawned from this item. (never time out)
	MEMORY_GUILD        = 0x0400,	///< This is my guild stone. (never time out) only have 1
	MEMORY_TOWN         = 0x0800,	///< This is my town stone. (never time out) only have 1
	MEMORY_UNUSED       = 0x1000,	///< UNUSED!!!! I am following this Object (never time out)
	MEMORY_UNUSED2		= 0x2000,	///< UNUSED!!!! (MEMORY_WAR_TARG) This is one of my current war targets.
	MEMORY_FRIEND       = 0x4000,	///< They can command me but not release me. (not primary blame)
	MEMORY_UNUSED3      = 0x8000	///< UNUSED!!!! Gump record memory (More1 = Context, More2 = Uid)
};

enum NPC_MEM_ACT_TYPE	///< A simgle primary memory about the object.
{
	NPC_MEM_ACT_NONE = 0,       ///< we spoke about something non-specific,
	NPC_MEM_ACT_SPEAK_TRAIN,    ///< I am speaking about training. Waiting for money
	NPC_MEM_ACT_SPEAK_HIRE,     ///< I am speaking about being hired. Waiting for money
	NPC_MEM_ACT_FIRSTSPEAK,     ///< I attempted (or could have) to speak to player. but have had no response.
	NPC_MEM_ACT_TAMED,          ///< I was tamed by this person previously.
	NPC_MEM_ACT_IGNORE          ///< I looted or looked at and discarded this item (ignore it)
};

class PacketSend;
class PacketPropertyList;

class CObjBase : public CObjBaseTemplate, public CScriptObj
{
	static lpctstr const sm_szLoadKeys[];   ///< All Instances of CItem or CChar have these base attributes.
	static lpctstr const sm_szVerbKeys[];   ///< All Instances of CItem or CChar have these base attributes.
	static lpctstr const sm_szRefKeys[];    ///< All Instances of CItem or CChar have these base attributes.

private:
	CServerTime m_timeout;		///< when does this rot away ? or other action. 0 = never, else system time
	CServerTime m_timestamp;    ///< TimeStam
	HUE_TYPE m_wHue;			///< Hue or skin color. (CItems must be < 0x4ff or so)
	lpctstr m_RunningTrigger;   ///< Current trigger being run on this object. Used to prevent the same trigger being called over and over.

protected:
	CResourceRef m_BaseRef;     ///< Pointer to the resource that describes this type.
public:

    /**
     * @fn  inline bool CObjBase::CallPersonalTrigger(tchar * pArgs, CTextConsole * pSrc, TRIGRET_TYPE & trResult, bool bFull);
     *
     * @brief   Call personal trigger (from scripts).
     *
     * @param [in,out]  pArgs       If non-null, the arguments.
     * @param [in,out]  pSrc        If non-null, source for the.
     * @param [in,out]  trResult    The tr result.
     * @param   bFull               true to full.
     *
     * @return  true if it succeeds, false if it fails.
     */
	inline bool CallPersonalTrigger(tchar * pArgs, CTextConsole * pSrc, TRIGRET_TYPE & trResult, bool bFull);
	static const char *m_sClassName;
	CVarDefMap m_TagDefs;		///< attach extra tags here.
	CVarDefMap m_BaseDefs;		///< New Variable storage system
	dword	m_Can;              ///< CAN_FLAGS
	int m_ModMaxWeight;			///< ModMaxWeight prop for chars

	word	m_attackBase;       ///< dam for weapons
	word	m_attackRange;      ///< variable range of attack damage.

	word	m_defenseBase;	    ///< Armor for IsArmor items
	word	m_defenseRange;     ///< variable range of defense.
	CUID m_uidSpawnItem;		///< SpawnItem for this item

	CResourceRefArray m_OEvents;
	static size_t sm_iCount;    ///< how many total objects in the world ?

    /**
     * @fn  CVarDefMap * CObjBase::GetTagDefs();
     *
     * @brief   Gets tags.
     *
     * @return  null if it fails, else the tag defs.
     */
	CVarDefMap * GetTagDefs();

    /**
     * @fn  virtual void CObjBase::DeletePrepare();
     *
     * @brief   Prepares to delete.
     */
	virtual void DeletePrepare();

    /**
     * @fn  bool CObjBase::IsTriggerActive(lpctstr trig);
     *
     * @brief   Queries if a trigger is active ( m_RunningTrigger ) .
     *
     * @param   trig    The trig.
     *
     * @return  true if the trigger is active, false if not.
     */
	bool IsTriggerActive(lpctstr trig) ;

    /**
     * @fn  lpctstr CObjBase::GetTriggerActive();
     *
     * @brief   Gets trigger active ( m_RunningTrigger ).
     *
     * @return  The trigger active.
     */
	lpctstr GetTriggerActive();

    /**
     * @fn  void CObjBase::SetTriggerActive(lpctstr trig = NULL);
     *
     * @brief   Sets trigger active ( m_RunningTrigger ).
     *
     * @param   trig    The trig.
     */
	void SetTriggerActive(lpctstr trig = NULL);

public:

    /**
     * @fn  byte CObjBase::RangeL() const;
     *
     * @brief   Returns RangeLow.
     *
     * @return  The Value.
     */
	byte	RangeL() const;

    /**
    * @fn  byte CObjBase::RangeH() const;
    *
    * @brief   Returns RangeHigh.
    *
    * @return  The Value.
    */
	byte	RangeH() const;

    /**
     * @fn  CServerTime CObjBase::GetTimeStamp() const;
     *
     * @brief   Gets time stamp.
     *
     * @return  The time stamp.
     */
	CServerTime GetTimeStamp() const;

    /**
     * @fn  void CObjBase::SetTimeStamp( int64 t_time);
     *
     * @brief   Sets time stamp.
     *
     * @param   t_time  The time.
     */
	void SetTimeStamp( int64 t_time);

    /**
     * @fn  lpctstr CObjBase::GetDefStr( lpctstr pszKey, bool fZero = false, bool fDef = false ) const;
     *
     * @brief   Gets definition string from m_BaseDefs.
     *
     * @param   pszKey  The key.
     * @param   fZero   true to zero.
     * @param   fDef    true to definition.
     *
     * @return  The definition string.
     */
	lpctstr GetDefStr( lpctstr pszKey, bool fZero = false, bool fDef = false ) const;

    /**
     * @fn  int64 CObjBase::GetDefNum( lpctstr pszKey, bool fZero = false, bool fDef = false ) const;
     *
     * @brief   Gets definition number from m_BaseDefs.
     *
     * @param   pszKey  The key.
     * @param   fZero   true to zero.
     * @param   fDef    true to definition.
     *
     * @return  The definition number.
     */

	int64 GetDefNum( lpctstr pszKey, bool fZero = false, bool fDef = false ) const;

    /**
     * @fn  void CObjBase::SetDefNum(lpctstr pszKey, int64 iVal, bool fZero = true);
     *
     * @brief   Sets definition number from m_BaseDefs.
     *
     * @param   pszKey  The key.
     * @param   iVal    Zero-based index of the value.
     * @param   fZero   true to zero.
     */

	void SetDefNum(lpctstr pszKey, int64 iVal, bool fZero = true);

    /**
     * @fn  void CObjBase::SetDefStr(lpctstr pszKey, lpctstr pszVal, bool fQuoted = false, bool fZero = true);
     *
     * @brief   Sets definition string to m_BaseDefs.
     *
     * @param   pszKey  The key.
     * @param   pszVal  The value.
     * @param   fQuoted true if quoted.
     * @param   fZero   true to zero.
     */

	void SetDefStr(lpctstr pszKey, lpctstr pszVal, bool fQuoted = false, bool fZero = true);

    /**
     * @fn  void CObjBase::DeleteDef(lpctstr pszKey);
     *
     * @brief   Deletes the definition from m_BaseDefs.
     *
     * @param   pszKey  The key to delete.
     */
	void DeleteDef(lpctstr pszKey);

    /**
     * @fn  CVarDefCont * CObjBase::GetDefKey( lpctstr pszKey, bool fDef ) const;
     *
     * @brief   Gets definition key from m_BaseDefs.
     *
     * @param   pszKey  The key.
     * @param   fDef    true to definition.
     *
     * @return  null if it fails, else the definition key.
     */
	CVarDefCont * GetDefKey( lpctstr pszKey, bool fDef ) const;

    /**
     * @fn  lpctstr CObjBase::GetKeyStr( lpctstr pszKey, bool fZero = false, bool fDef = false ) const;
     *
     * @brief   Gets key string from m_TagDefs.
     *
     * @param   pszKey  The key.
     * @param   fZero   true to zero.
     * @param   fDef    true to definition.
     *
     * @return  The key string.
     */
	lpctstr GetKeyStr( lpctstr pszKey, bool fZero = false, bool fDef = false ) const;

    /**
     * @fn  int64 CObjBase::GetKeyNum( lpctstr pszKey, bool fZero = false, bool fDef = false ) const;
     *
     * @brief   Gets key number from m_TagDefs.
     *
     * @param   pszKey  The key.
     * @param   fZero   true to zero.
     * @param   fDef    true to definition.
     *
     * @return  The key number.
     */
	int64 GetKeyNum( lpctstr pszKey, bool fZero = false, bool fDef = false ) const;

    /**
     * @fn  CVarDefCont * CObjBase::GetKey( lpctstr pszKey, bool fDef ) const;
     *
     * @brief   Gets a key from m_TagDefs.
     *
     * @param   pszKey  The key.
     * @param   fDef    true to definition.
     *
     * @return  null if it fails, else the key.
     */
	CVarDefCont * GetKey( lpctstr pszKey, bool fDef ) const;

    /**
     * @fn  void CObjBase::SetKeyNum(lpctstr pszKey, int64 iVal);
     *
     * @brief   Sets key number from m_TagDefs.
     *
     * @param   pszKey  The key.
     * @param   iVal    Zero-based index of the value.
     */
	void SetKeyNum(lpctstr pszKey, int64 iVal);

    /**
     * @fn  void CObjBase::SetKeyStr(lpctstr pszKey, lpctstr pszVal);
     *
     * @brief   Sets key string from m_TagDefs.
     *
     * @param   pszKey  The key.
     * @param   pszVal  The value.
     */
	void SetKeyStr(lpctstr pszKey, lpctstr pszVal);

    /**
     * @fn  void CObjBase::DeleteKey( lpctstr pszKey );
     *
     * @brief   Deletes the key described by pszKey from m_TagDefs.
     *
     * @param   pszKey  The key.
     */
	void DeleteKey( lpctstr pszKey );

protected:

    /**
     * @fn  virtual void CObjBase::DupeCopy( const CObjBase * pObj );
     *
     * @brief   Dupe copy.
     *
     * @param   pObj    The object.
     */
	virtual void DupeCopy( const CObjBase * pObj );

public:

    /**
     * @fn  virtual bool CObjBase::OnTick() = 0;
     *
     * @brief   Executes the tick action.
     *
     * @return  true if it succeeds, false if it fails.
     */
	virtual bool OnTick() = 0;

    /**
     * @fn  virtual int CObjBase::FixWeirdness() = 0;
     *
     * @brief   Fix weirdness.
     *
     * @return  An int.
     */
	virtual int FixWeirdness() = 0;

    /**
     * @fn  virtual int CObjBase::GetWeight(word amount = 0) const = 0;
     *
     * @brief   Gets a weight.
     *
     * @param   amount  The amount.
     *
     * @return  The weight.
     */
	virtual int GetWeight(word amount = 0) const = 0;

    /**
     * @fn  virtual bool CObjBase::IsResourceMatch( RESOURCE_ID_BASE rid, dword dwArg ) = 0;
     *
     * @brief   Query if 'rid' is resource match.
     *
     * @param   rid     The rid.
     * @param   dwArg   The argument.
     *
     * @return  true if resource match, false if not.
     */
	virtual bool IsResourceMatch( CResourceIDBase rid, dword dwArg ) = 0;

    /**
     * @fn  virtual int CObjBase::IsWeird() const;
     *
     * @brief   Is weird.
     *
     * @return  An int.
     */
	virtual int IsWeird() const;

    /**
     * @fn  virtual void CObjBase::Delete(bool bforce = false);
     *
     * @brief   Deletes the given bforce.
     *
     * @param   bforce  The bforce to delete.
     */
	virtual void Delete(bool bforce = false);

	// Accessors

    /**
     * @fn  virtual word CObjBase::GetBaseID() const = 0;
     *
     * @brief   Gets base identifier.
     *
     * @return  The base identifier.
     */

	virtual word GetBaseID() const = 0;

    /**
     * @fn  CBaseBaseDef * CObjBase::Base_GetDef() const;
     *
     * @brief   Base get definition.
     *
     * @return  null if it fails, else a pointer to a CBaseBaseDef.
     */
	CBaseBaseDef * Base_GetDef() const;

    /**
     * @fn  void CObjBase::SetUID( dword dwVal, bool fItem );
     *
     * @brief   Sets an UID.
     *
     * @param   dwVal   The value.
     * @param   fItem   true to item.
     */
	void SetUID( dword dwVal, bool fItem );

    /**
     * @fn  CObjBase* CObjBase::GetNext() const;
     *
     * @brief   Gets the next item.
     *
     * @return  null if it fails, else the next.
     */
	CObjBase* GetNext() const;

    /**
     * @fn  CObjBase* CObjBase::GetPrev() const;
     *
     * @brief   Gets the previous item.
     *
     * @return  null if it fails, else the previous.
     */
	CObjBase* GetPrev() const;

    /**
     * @fn  virtual lpctstr CObjBase::GetName() const;
     *
     * @brief   Gets the name. Resolves ambiguity w/CScriptObj.
     *
     * @return  The name.
     */
	virtual lpctstr GetName() const;

    /**
     * @fn  lpctstr CObjBase::GetResourceName() const;
     *
     * @brief   Gets resource name.
     *
     * @return  The resource name.
     */
	lpctstr GetResourceName() const;

public:

    /**
     * @fn  void CObjBase::SetHue( HUE_TYPE wHue, bool bAvoidTrigger = true, CTextConsole *pSrc = NULL, CObjBase *SourceObj = NULL, llong sound = 0 );
     *
     * @brief   Sets Hue given.
     *
     * @param   wHue                The hue.
     * @param   bAvoidTrigger       true to avoid trigger.
     * @param [in,out]  pSrc        (Optional) If non-null, source for the.
     * @param [in,out]  SourceObj   (Optional) If non-null, source object.
     * @param   sound               The sound.
     */
	void SetHue( HUE_TYPE wHue, bool bAvoidTrigger = true, CTextConsole *pSrc = NULL, CObjBase *SourceObj = NULL, llong sound = 0 );

    /**
     * @fn  HUE_TYPE CObjBase::GetHue() const;
     *
     * @brief   Gets the hue.
     *
     * @return  The hue.
     */
	HUE_TYPE GetHue() const;

protected:

    /**
     * @fn  word CObjBase::GetHueAlt() const;
     *
     * @brief   Gets hue alternate (OColor).
     *
     * @return  The hue alternate.
     */

	word GetHueAlt() const;

    /**
     * @fn  void CObjBase::SetHueAlt( HUE_TYPE wHue );
     *
     * @brief   Sets hue alternate (OColor).
     *
     * @param   wHue    The hue.
     */
	void SetHueAlt( HUE_TYPE wHue );

public:

    /**
     * @fn  virtual void CObjBase::SetTimeout( int64 iDelayInTicks );
     *
     * @brief   &lt; Timer.
     *
     * @param   iDelayInTicks   Zero-based index of the delay in ticks.
     */
	virtual void SetTimeout( int64 iDelayInTicks );

    /**
     * @fn  bool CObjBase::IsTimerSet() const;
     *
     * @brief   Query if this object is timer set.
     *
     * @return  true if timer set, false if not.
     */
	bool IsTimerSet() const;

    /**
     * @fn  int64 CObjBase::GetTimerDiff() const;
     *
     * @brief   Gets timer difference between current time and stored time.
     *
     * @return  The timer difference.
     */
	int64 GetTimerDiff() const;

    /**
     * @fn  bool CObjBase::IsTimerExpired() const;
     *
     * @brief   Query if this object is timer expired.
     *
     * @return  true if timer expired, false if not.
     */
	bool IsTimerExpired() const;

    /**
     * @fn  int64 CObjBase::GetTimerAdjusted() const;
     *
     * @brief   Gets timer.
     *
     * @return  The timer adjusted.
     */
	int64 GetTimerAdjusted() const;

    /**
     * @fn  int64 CObjBase::GetTimerDAdjusted() const;
     *
     * @brief   Gets timer in tenths of second.
     *
     * @return  The timer d adjusted.
     */
	int64 GetTimerDAdjusted() const;

public:

    /**
     * @fn  virtual bool CObjBase::MoveTo(CPointMap pt, bool bForceFix = false) = 0;
     *
     * @brief   Move To Location.
     *
     * @param   pt          The point.
     * @param   bForceFix   true to force fix.
     *
     * @return  true if it succeeds, false if it fails.
     */
	virtual bool MoveTo(CPointMap pt, bool bForceFix = false) = 0;	///< Move to a location at top level.

    /**
     * @fn  virtual bool CObjBase::MoveNear( CPointMap pt, word iSteps = 0 );
     *
     * @brief   Move near of a location.
     *
     * @param   pt      The point.
     * @param   iSteps  Zero-based index of the steps.
     *
     * @return  true if it succeeds, false if it fails.
     */
	virtual bool MoveNear( CPointMap pt, word iSteps = 0 );

    /**
     * @fn  virtual bool CObjBase::MoveNearObj( const CObjBaseTemplate *pObj, word iSteps = 0 );
     *
     * @brief   Move near of an object.
     *
     * @param   pObj    The object.
     * @param   iSteps  Zero-based index of the steps.
     *
     * @return  true if it succeeds, false if it fails.
     */
	virtual bool MoveNearObj( const CObjBaseTemplate *pObj, word iSteps = 0 );

    /**
     * @fn  void inline CObjBase::SetNamePool_Fail( tchar * ppTitles );
     *
     * @brief   SetNamePool() failed, so we set another name and throw an error.
     *
     * @param [in,out]  ppTitles    If non-null, the titles.
     */
	void inline SetNamePool_Fail( tchar * ppTitles );

    /**
     * @fn  bool CObjBase::SetNamePool( lpctstr pszName );
     *
     * @brief   Sets name pool.
     *
     * @param   pszName The name.
     *
     * @return  true if it succeeds, false if it fails.
     */
	bool SetNamePool( lpctstr pszName );

    /**
     * @fn  void CObjBase::Sound( SOUND_TYPE id, int iRepeat = 1 ) const;
     *
     * @brief   Play sound effect from this location.
     *
     * @param   id      The identifier.
     * @param   iRepeat Zero-based index of the repeat.
     */
	void Sound( SOUND_TYPE id, int iRepeat = 1 ) const;

    /**
     * @fn  void CObjBase::Effect(EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBase * pSource = NULL, byte bspeedseconds = 5, byte bloop = 1, bool fexplode = false, dword color = 0, dword render = 0, word effectid = 0, word explodeid = 0, word explodesound = 0, dword effectuid = 0, byte type = 0) const;
     *
     * @brief   Adds an Effect.
     *
     * @param   motion          The motion.
     * @param   id              The identifier.
     * @param   pSource         Source for the.
     * @param   bspeedseconds   The bspeedseconds.
     * @param   bloop           The bloop.
     * @param   fexplode        true to fexplode.
     * @param   color           The color.
     * @param   render          The render.
     * @param   effectid        The effectid.
     * @param   explodeid       The explodeid.
     * @param   explodesound    The explodesound.
     * @param   effectuid       The effectuid.
     * @param   type            The type.
     */
	void Effect(EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBase * pSource = NULL, byte bspeedseconds = 5, byte bloop = 1, bool fexplode = false, dword color = 0, dword render = 0, word effectid = 0, word explodeid = 0, word explodesound = 0, dword effectuid = 0, byte type = 0) const;

	void r_WriteSafe( CScript & s );

	virtual bool r_GetRef( lpctstr & pszKey, CScriptObj * & pRef );
	virtual void r_Write( CScript & s );
	virtual bool r_LoadVal( CScript & s );
	virtual bool r_WriteVal( lpctstr pszKey, CSString &sVal, CTextConsole * pSrc );
	virtual bool r_Verb( CScript & s, CTextConsole * pSrc );	///< some command on this object as a target

    /**
     * @fn  void CObjBase::Emote(lpctstr pText, CClient * pClientExclude = NULL, bool fPossessive = false);
     *
     * @brief   Emote ( You %s, You see %s %s ).
     *
     * @param   pText                   The text.
     * @param [in,out]  pClientExclude  (Optional) If non-null, the client exclude.
     * @param   fPossessive             true to possessive.
     */
	void Emote(lpctstr pText, CClient * pClientExclude = NULL, bool fPossessive = false);

    /**
     * @fn  void CObjBase::Emote2(lpctstr pText, lpctstr pText2, CClient * pClientExclude = NULL, bool fPossessive = false);
     *
     * @brief   Sends one Emote to this and a different text to others.
     *
     * @param   pText                   The text.
     * @param   pText2                  The second p text.
     * @param [in,out]  pClientExclude  (Optional) If non-null, the client exclude.
     * @param   fPossessive             true to possessive.
     */
	void Emote2(lpctstr pText, lpctstr pText2, CClient * pClientExclude = NULL, bool fPossessive = false);

    /**
     * @fn  virtual void CObjBase::Speak( lpctstr pText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL );
     *
     * @brief   Speaks.
     *
     * @param   pText   The text.
     * @param   wHue    The hue.
     * @param   mode    The mode.
     * @param   font    The font.
     */
	virtual void Speak( lpctstr pText, HUE_TYPE wHue = HUE_TEXT_DEF, TALKMODE_TYPE mode = TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL );

    /**
     * @fn  virtual void CObjBase::SpeakUTF8( lpctstr pText, HUE_TYPE wHue= HUE_TEXT_DEF, TALKMODE_TYPE mode= TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL, CLanguageID lang = 0 );
     *
     * @brief   Speak UTF 8.
     *
     * @param   pText   The text.
     * @param   wHue    The hue.
     * @param   mode    The mode.
     * @param   font    The font.
     * @param   lang    The language.
     */
	virtual void SpeakUTF8( lpctstr pText, HUE_TYPE wHue= HUE_TEXT_DEF, TALKMODE_TYPE mode= TALKMODE_SAY, FONT_TYPE font = FONT_NORMAL, CLanguageID lang = 0 );

    /**
     * @fn  virtual void CObjBase::SpeakUTF8Ex( const nword * pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang );
     *
     * @brief   Speak UTF 8 ex.
     *
     * @param   pText   The text.
     * @param   wHue    The hue.
     * @param   mode    The mode.
     * @param   font    The font.
     * @param   lang    The language.
     */
	virtual void SpeakUTF8Ex( const nword * pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang );

    /**
     * @fn  void CObjBase::RemoveFromView( CClient * pClientExclude = NULL , bool fHardcoded = true );
     *
     * @brief   Removes from view.

     *
     * @param [in,out]  pClientExclude  (Optional) If non-null, the client exclude.
     * @param   fHardcoded              true if hardcoded.
     */
	void RemoveFromView( CClient * pClientExclude = NULL , bool fHardcoded = true );	///< remove this item from all clients.

    /**
     * @fn  void CObjBase::ResendOnEquip( bool fAllClients = false );
     *
     * @brief   Resend on equip.
     *
     * @param   fAllClients true to all clients.
     */
	void ResendOnEquip( bool fAllClients = false );	///< Fix for Enhanced Client when equipping items via DClick, these must be removed from where they are and sent again.

    /**
     * @fn  void CObjBase::ResendTooltip( bool bSendFull = false, bool bUseCache = false );
     *
     * @brief   Resend tooltip.
     *
     * @param   bSendFull   true to send full.
     * @param   bUseCache   true to use cache.
     */
	void ResendTooltip( bool bSendFull = false, bool bUseCache = false );	///< force reload of tooltip for this object

    /**
     * @fn  void CObjBase::UpdateCanSee( PacketSend * pPacket, CClient * pClientExclude = NULL ) const;
     *
     * @brief   Updates the can see.
     *
     * @param [in,out]  pPacket         If non-null, the packet.
     * @param [in,out]  pClientExclude  (Optional) If non-null, the client exclude.
     */
	void UpdateCanSee( PacketSend * pPacket, CClient * pClientExclude = NULL ) const;

    /**
     * @fn  void CObjBase::UpdateObjMessage( lpctstr pTextThem, lpctstr pTextYou, CClient * pClientExclude, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font = FONT_NORMAL, bool bUnicode = false ) const;
     *
     * @brief   Updates the object message.
     *
     * @param   pTextThem               The text them.
     * @param   pTextYou                The text you.
     * @param [in,out]  pClientExclude  If non-null, the client exclude.
     * @param   wHue                    The hue.
     * @param   mode                    The mode.
     * @param   font                    The font.
     * @param   bUnicode                true to unicode.
     */
	void UpdateObjMessage( lpctstr pTextThem, lpctstr pTextYou, CClient * pClientExclude, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font = FONT_NORMAL, bool bUnicode = false ) const;

    /**
     * @fn  TRIGRET_TYPE CObjBase::OnHearTrigger(CResourceLock &s, lpctstr pCmd, CChar *pSrc, TALKMODE_TYPE &mode, HUE_TYPE wHue = HUE_DEFAULT);
     *
     * @brief   Executes the hear trigger action.
     *
     * @param [in,out]  s       The CResourceLock to process.
     * @param   pCmd            The command.
     * @param [in,out]  pSrc    If non-null, source for the.
     * @param [in,out]  mode    The mode.
     * @param   wHue            The hue.
     *
     * @return  A TRIGRET_TYPE.
     */
	TRIGRET_TYPE OnHearTrigger(CResourceLock &s, lpctstr pCmd, CChar *pSrc, TALKMODE_TYPE &mode, HUE_TYPE wHue = HUE_DEFAULT);

    /**
     * @fn  bool CObjBase::IsContainer() const;
     *
     * @brief   Query if this object is container.
     *
     * @return  true if container, false if not.
     */
	bool IsContainer() const;

    /**
     * @fn  virtual void CObjBase::Update(const CClient * pClientExclude = NULL) = 0;
     *
     * @brief   send this new item to clients.
     *
     * @param   pClientExclude  Do not send to this CClient.
     */
	virtual void Update(const CClient * pClientExclude = NULL)
		= 0;

    /**
     * @fn  virtual void CObjBase::Flip() = 0;
     *
     * @brief   Flips this object.
     */
	virtual void Flip()
		= 0;

    /**
     * @fn  virtual bool CObjBase::OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem, bool bReflecting = false ) = 0;
     *
     * @brief   Executes the spell effect action.
     *
     * @param   spell               The spell.
     * @param [in,out]  pCharSrc    If non-null, the character source.
     * @param   iSkillLevel         Zero-based index of the skill level.
     * @param [in,out]  pSourceItem If non-null, source item.
     * @param   bReflecting         true to reflecting.
     *
     * @return  true if it succeeds, false if it fails.
     */
	virtual bool OnSpellEffect( SPELL_TYPE spell, CChar * pCharSrc, int iSkillLevel, CItem * pSourceItem, bool bReflecting = false )
		= 0;

    /**
     * @fn  virtual TRIGRET_TYPE CObjBase::Spell_OnTrigger( SPELL_TYPE spell, SPTRIG_TYPE stage, CChar * pSrc, CScriptTriggerArgs * pArgs );
     *
     * @brief   Spell's trigger (@Effect, @Start...).
     *
     * @param   spell           The spell.
     * @param   stage           The stage.
     * @param [in,out]  pSrc    If non-null, source for the.
     * @param [in,out]  pArgs   If non-null, the arguments.
     *
     * @return  A TRIGRET_TYPE.
     */
	virtual TRIGRET_TYPE Spell_OnTrigger( SPELL_TYPE spell, SPTRIG_TYPE stage, CChar * pSrc, CScriptTriggerArgs * pArgs );

public:
	explicit CObjBase( bool fItem );
	virtual ~CObjBase();
private:
	CObjBase(const CObjBase& copy);
	CObjBase& operator=(const CObjBase& other);

public:
	///<	Some global object variables
	int m_ModAr;

#define SU_UPDATE_HITS      0x01    ///< update hits to others
#define SU_UPDATE_MODE      0x02    ///< update mode to all
#define SU_UPDATE_TOOLTIP   0x04    ///< update tooltip to all
	uchar m_fStatusUpdate;  ///< update flags for next tick

    /**
     * @fn  virtual void CObjBase::OnTickStatusUpdate();
     *
     * @brief   Update Status window if any flag requires it on m_fStatusUpdate.
     */
	virtual void OnTickStatusUpdate();

protected:
	PacketPropertyList* m_PropertyList;	///< currently cached property list packet
	dword m_PropertyHash;				///< latest property list hash
	dword m_PropertyRevision;			///< current property list revision

public:

    /**
     * @fn  PacketPropertyList* CObjBase::GetPropertyList(void) const
     *
     * @brief   Gets property list.
     *
     * @return  null if it fails, else the property list.
     */
	PacketPropertyList* GetPropertyList(void) const { return m_PropertyList; }

    /**
     * @fn  void CObjBase::SetPropertyList(PacketPropertyList* propertyList);
     *
     * @brief   Sets property list.
     *
     * @param [in,out]  propertyList    If non-null, list of properties.
     */
	void SetPropertyList(PacketPropertyList* propertyList);

    /**
     * @fn  void CObjBase::FreePropertyList(void);
     *
     * @brief   Free property list.
     */
	void FreePropertyList(void);

    /**
     * @fn  dword CObjBase::UpdatePropertyRevision(dword hash);
     *
     * @brief   Updates the property revision described by hash.
     *
     * @param   hash    The hash.
     *
     * @return  A dword.
     */
	dword UpdatePropertyRevision(dword hash);

    /**
     * @fn  void CObjBase::UpdatePropertyFlag(int mask);
     *
     * @brief   Updates the property flag described by mask.
     *
     * @param   mask    The mask.
     */
	void UpdatePropertyFlag(int mask);
};

enum STONEALIGN_TYPE ///< Types of Guild/Town stones
{
	STONEALIGN_STANDARD = 0,
	STONEALIGN_ORDER,
	STONEALIGN_CHAOS
};

enum ITRIG_TYPE
{
	///< XTRIG_UNKNOWN = some named trigger not on this list.
	ITRIG_AfterClick=1,
	ITRIG_Buy,
	ITRIG_Click,
	ITRIG_CLIENTTOOLTIP,        ///< Sending tooltip to client for this item
	ITRIG_ContextMenuRequest,   ///< A context menu was requested over me.
	ITRIG_ContextMenuSelect,    ///< A context menu option was selected, perform actions.
	ITRIG_Create,               ///< Item is being created.
	ITRIG_DAMAGE,               ///< I have been damaged in some way.
	ITRIG_DCLICK,               ///< I have been dclicked.
	ITRIG_DESTROY,              ///<+I am nearly destroyed.
	ITRIG_DROPON_CHAR,          ///< I have been dropped on this char.
	ITRIG_DROPON_GROUND,        ///< I have been dropped on the ground here.
	ITRIG_DROPON_ITEM,          ///< An item has been.
	ITRIG_DROPON_SELF,          ///< An item has been dropped upon me.
	ITRIG_DROPON_TRADE,         ///< Droping an item in a trade window.
	///<ITRIG_DYE,
	ITRIG_EQUIP,                ///< I have been equipped.
	ITRIG_EQUIPTEST,            ///< I'm not yet equiped, but checking if I can.
	ITRIG_MemoryEquip,          ///< I'm a memory and I'm being equiped.
	ITRIG_PICKUP_GROUND,        ///< I'm being picked up from ground.
	ITRIG_PICKUP_PACK,          ///< picked up from inside some container.
	ITRIG_PICKUP_SELF,          ///< picked up from this container
	ITRIG_PICKUP_STACK,         ///< picked up from a stack (ARGO)
	ITRIG_Sell,                 ///< I'm being sold.
	ITRIG_Ship_Turn,            ///< I'm a ship and i'm turning around.
	ITRIG_SPELLEFFECT,          ///< cast some spell on me.
	ITRIG_STEP,                 ///< I have been walked on. (or shoved)
	ITRIG_TARGON_CANCEL,        ///< Someone requested me (item) to target, now the targeting was canceled.
	ITRIG_TARGON_CHAR,          ///< I'm targeting a char.
	ITRIG_TARGON_GROUND,        ///< I'm targeting the ground.
	ITRIG_TARGON_ITEM,          ///< I am being combined with an item
	ITRIG_TIMER,                ///< My timer has expired.
	ITRIG_ToolTip,              ///< A tooltip is being requested from me.
	ITRIG_UNEQUIP,              ///< I'm being unequiped.
	ITRIG_QTY
};

///<	number of steps to remember for pathfinding, default to 24 steps, will have 24*4 extra bytes per char
#define MAX_NPC_PATH_STORAGE_SIZE	UO_MAP_VIEW_SIGHT*2

enum WAR_SWING_TYPE	///< m_Act_War_Swing_State
{
	WAR_SWING_INVALID = -1,
	WAR_SWING_EQUIPPING = 0,	///< we are recoiling our weapon.
	WAR_SWING_READY,			///< we can swing at any time.
	WAR_SWING_SWINGING			///< we are swinging our weapon.
};

enum CTRIG_TYPE
{
	CTRIG_AAAUNUSED		= 0,
	CTRIG_AfterClick,       ///< I'm not yet clicked, name should be generated before.
	CTRIG_Attack,           ///< I am attacking someone (SRC).
	CTRIG_CallGuards,       ///< I'm calling guards.
    ///< Here starts @charXXX section
	CTRIG_charAttack,           ///< Calling this trigger over other char.
	CTRIG_charClick,            ///< Calling this trigger over other char.
	CTRIG_charClientTooltip,    ///< Calling this trigger over other char.
	CTRIG_charContextMenuRequest,///< Calling this trigger over other char.
	CTRIG_charContextMenuSelect,///< Calling this trigger over other char.
	CTRIG_charDClick,           ///< Calling this trigger over other char.
	CTRIG_charTradeAccepted,    ///< Calling this trigger over other char.

	CTRIG_Click,            ///< I got clicked on by someone.
	CTRIG_ClientTooltip,    ///< Sending tooltips for me to someone.
	CTRIG_CombatAdd,        ///< I add someone to my attacker list.
	CTRIG_CombatDelete,     ///< delete someone from my list.
	CTRIG_CombatEnd,        ///< I finished fighting.
	CTRIG_CombatStart,      ///< I begin fighting.
	CTRIG_ContextMenuRequest,///< Context menu requested on me.
	CTRIG_ContextMenuSelect,///< An option was selected on the Context Menu.
	CTRIG_Create,           ///< Newly created (not in the world yet).
	CTRIG_Criminal,         ///< Called before someone becomes 'gray' for someone.
	CTRIG_DClick,           ///< Someone has dclicked on me.
	CTRIG_Death,            ///< I just got killed.
	CTRIG_DeathCorpse,      ///< A Corpse is being created from my body.
	CTRIG_Destroy,          ///< I am nearly destroyed.
	CTRIG_Dismount,         ///< I'm dismounting.
	///<CTRIG_DYE,
	CTRIG_Eat,              ///< I'm eating something.
	CTRIG_EffectAdd,        ///< A spell effected me, i'm getting bonus/penalties from it.
	CTRIG_EnvironChange,    ///< my environment changed somehow (light,weather,season,region)
	CTRIG_ExpChange,        ///< EXP is going to change
	CTRIG_ExpLevelChange,   ///< Experience LEVEL is going to change

	CTRIG_FameChange,       ///< Fame chaged

	CTRIG_FollowersUpdate,  ///< Adding or removing CurFollowers.

	CTRIG_GetHit,           ///< I just got hit.
	CTRIG_Hit,              ///< I just hit someone. (TARG)
	CTRIG_HitCheck,         ///< A check made before anything else in the Hit proccess, meant to completelly override combat system.
	CTRIG_HitIgnore,        ///< I should ignore this target, just giving a record to scripts.
	CTRIG_HitMiss,          ///< I just missed.
	CTRIG_HitTry,           ///< I am trying to hit someone. starting swing.,
	CTRIG_HouseDesignCommit,///< I committed a new house design
	CTRIG_HouseDesignExit,  ///< I exited house design mode

	CTRIG_itemAfterClick,       ///< I'm going to click one item.
	CTRIG_itemBuy,              ///< I'm going to buy one item.
	CTRIG_itemClick,            ///< I clicked one item
	CTRIG_itemClientTooltip,    ///< Requesting ToolTip for one item.
	CTRIG_itemContextMenuRequest,///< Requesting Context Menu.
	CTRIG_itemContextMenuSelect,///< Selected one option from Context Menu.
	CTRIG_itemCreate,           ///< Created one item.
	CTRIG_itemDamage,           ///< Damaged one item.
	CTRIG_itemDCLICK,           ///< I have dclicked item.
	CTRIG_itemDestroy,          ///< Item is nearly destroyed.
	CTRIG_itemDROPON_CHAR,      ///< I have been dropped on this char.
	CTRIG_itemDROPON_GROUND,    ///< I dropped an item on the ground.
	CTRIG_itemDROPON_ITEM,      ///< I have been dropped on this item.
	CTRIG_itemDROPON_SELF,      ///< I have been dropped on this item.
	CTRIG_itemDROPON_TRADE,     ///< I have dropped this item on a Trade Window.
	CTRIG_itemEQUIP,            ///< I have equipped an item.
	CTRIG_itemEQUIPTEST,        ///< I'm checking if I can equip an item.
	CTRIG_itemMemoryEquip,      ///< I'm equiping a memory.
	CTRIG_itemPICKUP_GROUND,    ///< I'm picking up an item.
	CTRIG_itemPICKUP_PACK,      ///< picked up from inside some container.
	CTRIG_itemPICKUP_SELF,      ///< picked up from this (ACT) container.
	CTRIG_itemPICKUP_STACK,     ///< was picked up from a stack.
	CTRIG_itemSell,             ///< I'l selling an item.
	CTRIG_itemSPELL,            ///< cast some spell on the item.
	CTRIG_itemSTEP,             ///< stepped on an item
	CTRIG_itemTARGON_CANCEL,    ///< Canceled a target made from the item.
	CTRIG_itemTARGON_CHAR,      ///< Targeted a CChar with the item.
	CTRIG_itemTARGON_GROUND,    ///< Targeted the ground with the item.
	CTRIG_itemTARGON_ITEM,      ///< Targeted a CItem with the item.
	CTRIG_itemTimer,            ///< An item I have on is timing out.
	CTRIG_itemToolTip,          ///< Did tool tips on an item.
	CTRIG_itemUNEQUIP,          ///< i have unequipped (or try to unequip) an item.

	CTRIG_Jailed,               ///< I'm up to be send to jail, or to be forgiven.

	CTRIG_KarmaChange,          ///< Karma chaged

	CTRIG_Kill,         ///< I have just killed someone.
	CTRIG_LogIn,        ///< Client logs in.
	CTRIG_LogOut,       ///< Client logs out (21).
	CTRIG_Mount,        ///< I'm mounting some char.
	CTRIG_MurderDecay,  ///< I have decayed one of my kills.
	CTRIG_MurderMark,   ///< I am gonna to be marked as a murder.
	CTRIG_NotoSend,     ///< sending notoriety.

	CTRIG_NPCAcceptItem,    ///< (NPC only) i've been given an item i like (according to DESIRES).
	CTRIG_NPCActFight,      ///< (NPC only) I have to fight against my target.
	CTRIG_NPCActFollow,     ///< (NPC only) decided to follow someone.
	CTRIG_NPCAction,        ///< (NPC only) doing some action.
	CTRIG_NPCHearGreeting,  ///< (NPC only) i have been spoken to for the first time. (no memory of previous hearing).
	CTRIG_NPCHearUnknown,   ///< (NPC only) I heard something i don't understand.
	CTRIG_NPCLookAtChar,    ///< (NPC only) I found a char, i'm looking at it now.
	CTRIG_NPCLookAtItem,    ///< (NPC only) I found an item, i'm looking at it now.
	CTRIG_NPCLostTeleport,  ///< (NPC only) ready to teleport back to spawn.
	CTRIG_NPCRefuseItem,    ///< (NPC only) i've been given an item i don't want.
	CTRIG_NPCRestock,       ///< (NPC only) i'm restocking goods.
	CTRIG_NPCSeeNewPlayer,  ///< (NPC only) i see u for the first time. (in 20 minutes) (check memory time).
	CTRIG_NPCSeeWantItem,   ///< (NPC only) i see something good.
	CTRIG_NPCSpecialAction, ///< (NPC only) performing some special actions (spyder's web, dragon's breath...).

	CTRIG_PartyDisband, ///< I just disbanded my party.
	CTRIG_PartyInvite,  ///< SRC invited me to join a party, so I may chose.
	CTRIG_PartyLeave,   ///< I'm leaving this party.
	CTRIG_PartyRemove,  ///< I have ben removed from the party by SRC.

    CTRIG_PayGold,          ///< I'm going to give out money for a service (Skill Training, hiring...).
	CTRIG_PersonalSpace,	///< i just got stepped on.
	CTRIG_PetDesert,        ///< I'm deserting from my owner ( starving, being hit by him ...).
	CTRIG_Profile,			///< someone hit the profile button for me.
	CTRIG_ReceiveItem,		///< I was just handed an item (Not yet checked if i want it).
	CTRIG_RegenStat,		///< Hits/mana/stam/food regeneration.

	CTRIG_RegionEnter,          ///< I'm entering a region.
	CTRIG_RegionLeave,          ///< I'm leaving a region.
	CTRIG_RegionResourceFound,	///< I just discovered a resource.
	CTRIG_RegionResourceGather, ///< I'm gathering a resource.

	CTRIG_Rename,       ///< Changing my name or pets one.

	CTRIG_Resurrect,    ///< I'm going to resurrect via function or spell.

	CTRIG_SeeCrime,     ///< I am seeing a crime.
	CTRIG_SeeHidden,    ///< I'm about to see a hidden char.
	CTRIG_SeeSnoop,     ///< I see someone Snooping something.

	///< SKTRIG_QTY
	CTRIG_SkillAbort,       ///< SKTRIG_ABORT
	CTRIG_SkillChange,      ///< A skill's value is being changed.
	CTRIG_SkillFail,        ///< SKTRIG_FAIL
	CTRIG_SkillGain,        ///< SKTRIG_GAIN
	CTRIG_SkillMakeItem,    ///< An item is being created by a skill.
	CTRIG_SkillMemu,        ///< A skillmenu is being called.
	CTRIG_SkillPreStart,    ///< SKTRIG_PRESTART
	CTRIG_SkillSelect,      ///< SKTRIG_SELECT
	CTRIG_SkillStart,       ///< SKTRIG_START
	CTRIG_SkillStroke,      ///< SKTRIG_STROKE
	CTRIG_SkillSuccess,     ///< SKTRIG_SUCCESS
	CTRIG_SkillTargetCancel,///< SKTRIG_TARGETCANCEL
	CTRIG_SkillUseQuick,    ///< SKTRIG_USEQUICK
	CTRIG_SkillWait,        ///< SKTRIG_WAIT

	CTRIG_SpellBook,        ///< Opening a spellbook
	CTRIG_SpellCast,        ///< Char is casting a spell.
	CTRIG_SpellEffect,      ///< A spell just hit me.
	CTRIG_SpellFail,        ///< The spell failed.
	CTRIG_SpellSelect,      ///< selected a spell.
	CTRIG_SpellSuccess,     ///< The spell succeeded.
	CTRIG_SpellTargetCancel,///< Cancelled spell target.
	CTRIG_StatChange,       ///< A stat value is being changed.
	CTRIG_StepStealth,      ///< Made a step while being in stealth .
	CTRIG_Targon_Cancel,    ///< Cancel target from TARGETF.
	CTRIG_ToggleFlying,     ///< Toggle between flying/landing.
	CTRIG_ToolTip,          ///< someone did tool tips on me.
	CTRIG_TradeAccepted,    ///< Everything went well, and we are about to exchange trade items
	CTRIG_TradeClose,       ///< Fired when a Trade Window is being deleted, no returns
	CTRIG_TradeCreate,      ///< Trade window is going to be created

	///< Packet related triggers
	CTRIG_UserBugReport,    ///< (Client iteraction) Reported a bug
	CTRIG_UserChatButton,   ///< (Client iteraction) Pressed Chat button.
	CTRIG_UserExtCmd,       ///< (Client iteraction) Requested an special action (Open Door, cast spell, start skill...).
	CTRIG_UserExWalkLimit,  ///< (Client iteraction) Walked more than I could.
	CTRIG_UserGuildButton,  ///< (Client iteraction) Pressed Guild button.
	CTRIG_UserKRToolbar,    ///< (Client iteraction) Using KR's ToolBar.
	CTRIG_UserMailBag,      ///< (Client iteraction) Pressed Mail bag.
	CTRIG_UserQuestArrowClick,///<(Client iteraction) Clicked ArrowQuest.
	CTRIG_UserQuestButton,  ///< (Client iteraction) Pressed Quest Button.
	CTRIG_UserSkills,       ///< (Client iteraction) Opened Skills' dialog.
	CTRIG_UserSpecialMove,  ///< (Client iteraction) Performing an special action from books.
	CTRIG_UserStats,        ///< (Client iteraction) Opening Stats' dialog.
	CTRIG_UserVirtue,       ///< (Client iteraction) Opening Virtue gump.
	CTRIG_UserVirtueInvoke, ///< (Client iteraction) Invoquing a Virtue.
	CTRIG_UserWarmode,      ///< (Client iteraction) Switching between War/Peace.

	CTRIG_QTY				///< 130
};

/**
 * @fn  DIR_TYPE GetDirStr( lpctstr pszDir );
 *
 * @brief   Gets dir string.
 *
 * @param   pszDir  The dir.
 *
 * @return  The dir string.
 */
DIR_TYPE GetDirStr( lpctstr pszDir );

/**
 * @fn  extern void DeleteKey(lpctstr pszKey);
 *
 * @brief   Deletes the key described by pszKey.
 *
 * @param   pszKey  The key.
 */

extern void DeleteKey(lpctstr pszKey);


/* Inline Methods Definitions */

inline CObjBase* CObjBase::GetPrev() const
{
	return static_cast <CObjBase*>(CSObjListRec::GetPrev());
}

inline CObjBase* CObjBase::GetNext() const
{
	return static_cast <CObjBase*>(CSObjListRec::GetNext());
}

inline int64 CObjBase::GetTimerDiff() const
{
	///< How long till this will expire ?
	return g_World.GetTimeDiff( m_timeout );
	///< return: < 0 = in the past ( m_timeout - CServerTime::GetCurrentTime() )
}

inline bool CObjBase::IsTimerSet() const
{
	return m_timeout.IsTimeValid();
}

inline bool CObjBase::IsTimerExpired() const
{
	return (GetTimerDiff() <= 0);
}

#endif ///< _INC_COBJBASE_H
