
#include "../common/resource/CResourceLock.h"
#include "../common/CException.h"
#include "../common/sphereversion.h"
#include "../common/CLog.h"
#include "../network/CClientIterator.h"
#include "../network/send.h"
#include "../sphere/ProfileTask.h"
#include "chars/CChar.h"
#include "clients/CClient.h"
#include "components/CCChampion.h"
#include "components/CCPropsItemChar.h"
#include "components/CCPropsItemWeapon.h"
#include "components/CCSpawn.h"
#include "components/CCItemDamageable.h"
#include "CSector.h"
#include "CServer.h"
#include "CWorld.h"
#include "CWorldComm.h"
#include "CWorldMap.h"
#include "CWorldTickingList.h"
#include "CWorldTimedFunctions.h"
#include "CTimedFunction.h"
#include "spheresvr.h"
#include "triggers.h"
#include "CObjBase.h"


DIR_TYPE GetDirStr(lpctstr pszDir)
{
    char iDir2, iDir = static_cast<char>(toupper(pszDir[0]));

    switch (iDir)
    {
        case 'E':
        return DIR_E;
        case 'W':
        return DIR_W;
        case 'N':
        iDir2 = static_cast<char>(toupper(pszDir[1]));
        if (iDir2 == 'E')
            return DIR_NE;
        if (iDir2 == 'W')
            return DIR_NW;
        return DIR_N;
        case 'S':
        iDir2 = static_cast<char>(toupper(pszDir[1]));
        if (iDir2 == 'E')
            return DIR_SE;
        if (iDir2 == 'W')
            return DIR_SW;
        return DIR_S;
        default:
        if ((iDir >= '0') && (iDir <= '7'))
            return static_cast<DIR_TYPE>(iDir - '0');
    }
    return DIR_QTY;
}


static bool GetDeltaStr( CPointMap & pt, tchar * pszDir )
{
	tchar * ppCmd[3];
	size_t iQty = Str_ParseCmds( pszDir, ppCmd, ARRAY_COUNT(ppCmd));
	if (iQty <= 0)
		return false;

	tchar chDir = static_cast<tchar>(toupper(ppCmd[0][0]));
	int iTmp = Exp_GetVal( ppCmd[1] );

	if ( IsDigit( chDir ) || chDir == '-' )
	{
		pt.m_x += (Exp_GetSVal(ppCmd[0]));
		pt.m_y += (short)(iTmp);
		pt.m_z += (Exp_GetCVal(ppCmd[2]));
	}
	else	// a direction by name.
	{
		if ( iTmp == 0 )
			iTmp = 1;
		DIR_TYPE eDir = GetDirStr( ppCmd[0] );
		if ( eDir >= DIR_QTY )
			return false;
		pt.MoveN( eDir, iTmp );
	}

	return true;
}


/////////////////////////////////////////////////////////////////
// -CObjBase stuff
// Either a player, npc or item.

CObjBase::CObjBase( bool fItem )  // PROFILE_TIME_QTY is unused, CObjBase is not a real CTimedObject, it just needs its virtual inheritance.
{
	++ sm_iCount;

	_uiInternalStateFlags = 0;

	_iCreatedResScriptIdx	= _iCreatedResScriptLine	= -1;
    _iRunningTriggerId		= _iCallingObjTriggerId		= -1;

	m_iTimeStampS = 0;
	m_CanMask = 0;

	m_attackBase = m_attackRange = 0;
	m_defenseBase = m_defenseRange = 0;
	m_ModAr = 0;

	m_wHue = HUE_DEFAULT;
	m_ModMaxWeight = 0;

	m_fStatusUpdate = 0;
	m_PropertyList = nullptr;
	m_PropertyHash = 0;
	m_PropertyRevision = 0;

	if ( g_Serv.IsLoading())
	{
		// Don't do this yet if we are loading. UID will be set later.
		// Just say if this is an item or not.
		CObjBaseTemplate::SetUID(UID_O_DISCONNECT|UID_O_INDEX_MASK|(fItem ? UID_F_ITEM : 0));
	}
	else
	{
		// Find a free UID slot for this.
		SetUID(UID_PLAIN_CLEAR, fItem);
		ASSERT(IsValidUID());
		SetUIDContainerFlags(UID_O_DISCONNECT);	// it is no place for now
	}

	// Put in the idle list by default. (til placed in the world)
	g_World.AddIdleObj(this);
}

CObjBase::~CObjBase()
{
	EXC_TRY("Cleanup in destructor");
	ADDTOCALLSTACK("CObjBase::~CObjBase");
    if (CCSpawn *pSpawn = GetSpawn())    // If I was created from a Spawn
    {
		CItem* pSpawnLink = pSpawn->GetLink();
        if (CCChampion* pChampion = static_cast<CCChampion*>(pSpawnLink->GetComponent(COMP_CHAMPION)))
        {
            pChampion->DelObj(GetUID());
        }
        else
        {
            //pEntity->UnsubscribeComponent(pSpawn);    // Avoiding recursive calls from CCSpawn::DelObj when forcing the pChar/pItem to Delete();
            pSpawn->DelObj(GetUID());  // Then I should be removed from it's list.
        }
    }

	// As a safety net. If we are calling those methods via the class destructor, we know that calling virtual methods won't work,
	//  since the superclasses were already destructed. At least, do minimal cleanup here with CObjBase methods.
	DeletePrepare();	// virtual
	CObjBase::DeleteCleanup(true);	// it isn't virtual

	FreePropertyList();

	if (CBaseBaseDef* pBase = Base_GetDef())
	{
		pBase->DelInstance();
	}
	--sm_iCount;

	// ASSERT( IsDisconnected()); // It won't be disconnected if removed by CSObjCont::ClearCont

	// free up the UID slot.
	SetUID( UID_UNUSED, false );

	EXC_CATCH;
}


bool CObjBase::_IsIdle() const
{
	return (GetUID().IsValidUID() && g_World.IsObjIdle(this));
}

bool CObjBase::IsIdle() const
{
	MT_ENGINE_SHARED_LOCK_RETURN(_IsIdle());
}

bool CObjBase::_IsDeleted() const
{
	return (!GetUID().IsValidUID() || g_World.IsScheduledObjDeletion(this));
}

bool CObjBase::IsDeleted() const
{
	MT_ENGINE_SHARED_LOCK_RETURN(_IsDeleted());
}

void CObjBase::DeletePrepare()
{
	ADDTOCALLSTACK("CObjBase::DeletePrepare");
    // Preparations before actually deleting the object. Unlink it from the world and from other objects.

    CObjBase::_GoSleep();	// virtual, but superclass methods are called in their ::DeletePrepare methods

    const SERVMODE_TYPE servMode = g_Serv.GetServerMode();
    const bool fDestroyingWorld = (servMode == SERVMODE_Exiting || servMode == SERVMODE_Loading);
    if (!fDestroyingWorld)
    {
        RemoveFromView();
    }

    const bool fTopLevel = dynamic_cast<const CSectorObjCont*>(GetParent());
/*
#ifdef _DEBUG
    if (fTopLevel)
    {
        // This will not always work. When you clear a CSObjCont, OnRemoveObj isn't called automatically
        //  (in most of the cases it isn't actually needed). Though, this means that for some superclasses of CSObjCont
        //  (es containers for spawned objects), the top level flag (UID_O_DISCONNECT) is not set.
        //DEBUG_ASSERT(GetUID().IsObjTopLevel());
    }
#endif
*/

    // This HAS to be called before the CItem and CChar parts are destroyed.
	RemoveSelf();

    if (fTopLevel)
        	_uiInternalStateFlags |= SF_TOPLEVEL;
}

void CObjBase::DeleteCleanup(bool fForce)
{
	ADDTOCALLSTACK("CObjBase::DeleteCleanup");
	_uiInternalStateFlags |= SF_DELETING;

	CEntity::Delete(fForce);
	CWorldTimedFunctions::ClearUID(GetUID());
}

bool CObjBase::Delete(bool fForce)
{
	ADDTOCALLSTACK("CObjBase::Delete");

    bool fScheduleDeletion = true;
    const SERVMODE_TYPE servMode = g_Serv.GetServerMode();
    const bool fDestroyingWorld = (servMode == SERVMODE_Exiting || servMode == SERVMODE_Loading);
    if (fDestroyingWorld)
    {
        // Why resort to _uiInternalStateFlags and not simply check if GetParent() is a CSectorObjCont* ?
        //  Because at this point CSObjContRec::RemoveSelf might have been called (it depends on how CObjBase::Delete was called,
        //  if via a destructor or else), and GetParent() might already be nullptr.
        // There are cases (like sector closing) where the item is stored inside its CSectorObjCont just to later call the object destructor,
        //  even if its GetParent() is nullptr. That's also the reason because we set fScheduleDeletion to false: this CObjBase will be
        //  deleted/destructed by its parent container.
        if (HAS_FLAGS_ANY(_uiInternalStateFlags, SF_TOPLEVEL))
        {
            // If top level, the sector will tale care of destroying it.
            fScheduleDeletion = false;
        }
    }

	DeletePrepare();           // virtual, but if called by the destructor this will fail to call upper (CChar, CItem, etc) virtual methods.
    DeleteCleanup(fForce);     // not virtual!

    if (fScheduleDeletion) {
        g_World.ScheduleObjDeletion(this);
    }

	return true;
}

CBaseBaseDef* CObjBase::Base_GetDef() const noexcept
{
	return (static_cast <CBaseBaseDef*>(m_BaseRef.GetRef()));
}

bool CObjBase::IsContainer() const
{
	// Simple test if object is a container.
	return (dynamic_cast <const CContainer*>(this) != nullptr);
}

int64 CObjBase::GetTimeStampS() const
{
	return m_iTimeStampS;
}

void CObjBase::SetTimeStampS(int64 t_time)
{
	m_iTimeStampS = t_time;
}

void CObjBase::TimeoutRecursiveResync(int64 iDelta)
{
	ADDTOCALLSTACK("CObjBase::TimeoutRecursiveResync");
	if (_IsTimerSet())
	{
		_SetTimeout(_GetTimerAdjusted() + iDelta);
	}

	if (CContainer* pCont = dynamic_cast<CContainer*>(this))
	{
		for (CSObjContRec* pObjRec : pCont->GetIterationSafeContReverse())
		{
			CObjBase* pObj = dynamic_cast<CObjBase*>(pObjRec);
			ASSERT(pObj);
			pObj->TimeoutRecursiveResync(iDelta);
		}
	}
}

void CObjBase::SetHueQuick(HUE_TYPE wHue)
{
	m_wHue = wHue;
}

void CObjBase::SetHue( HUE_TYPE wHue, bool fAvoidTrigger, CTextConsole *pSrc, CObjBase *pSourceObj, llong iSound)
{
	ADDTOCALLSTACK("CObjBase::SetHue");
	if (g_Serv.IsLoading()) //We do not want tons of @Dye being called during world load, just set the hue then continue...
	{
		m_wHue = wHue;
		return;
	}

	/*	@Dye is now more universal, it is called on EVERY CObjBase color change.
		Sanity checks are recommended and if possible, avoid using it on universal events. */
	if (!fAvoidTrigger)
	{
        lpctstr ptcTrig = (IsChar() ? CChar::sm_szTrigName[CTRIG_DYE] : CItem::sm_szTrigName[ITRIG_DYE]);
		if (IsTrigUsed(ptcTrig))
		{
			CScriptTriggerArgs args(wHue, iSound, pSourceObj);
			TRIGRET_TYPE iRet = OnTrigger(ptcTrig, pSrc, &args);

			if (iRet == TRIGRET_RET_TRUE)
				return;

			if (args.m_iN2 > 0) // No sound? No checks for who can hear, packets...
			{
				Sound((SOUND_TYPE)(args.m_iN2));
			}

			m_wHue = (HUE_TYPE)(args.m_iN1);
			return;
		}
	}

	if (iSound > 0) // No sound? No checks for who can hear, packets...
	{
		Sound((SOUND_TYPE)iSound);
	}

	m_wHue = wHue;
}

HUE_TYPE CObjBase::GetHue() const
{
	return m_wHue;
}


int CObjBase::IsWeird() const
{
	ADDTOCALLSTACK_DEBUG("CObjBase::IsWeird");
	int iResultCode = CObjBaseTemplate::IsWeird();
	if ( iResultCode )
	{
		return( iResultCode );
	}
	if ( ! g_Serv.IsLoading())
	{
		if ( GetUID().ObjFind() != this )	// make sure it's linked both ways correctly.
		{
			return( 0x3201 );
		}
	}
	return 0;
}

void CObjBase::SetUID( dword dwIndex, bool fItem )
{
	ADDTOCALLSTACK("CObjBase::SetUID");
	// Move the serial number,
	// This is possibly dangerous if conflict arrises.

	dwIndex &= UID_O_INDEX_MASK;	// Make sure no flags in here.
	if ( IsValidUID())	// i already have a uid ?
	{
		if ( ! dwIndex )
			return;	// The point was just to make sure it was located.
		// remove the old UID.
		g_World.FreeUID( ((dword)GetUID()) & UID_O_INDEX_MASK );
	}

	if ( dwIndex != UID_O_INDEX_MASK )	// just wanted to remove it
		dwIndex = g_World.AllocUID( dwIndex, this );

	if ( fItem )
		dwIndex |= UID_F_ITEM;

	CObjBaseTemplate::SetUID( dwIndex );
}


lpctstr CObjBase::GetName() const	// resolve ambiguity w/CScriptObj
{
	return CObjBaseTemplate::GetName();
}

lpctstr CObjBase::GetResourceName() const
{
	return Base_GetDef()->GetResourceName();
}

void CObjBase::SetNamePool_Fail( tchar * ppTitles )
{
	ADDTOCALLSTACK("CObjBase::SetNamePool_Fail");
	g_Log.EventError("Name pool '%s' could not be found\n", ppTitles);
	CObjBase::SetName( ppTitles );
}

bool CObjBase::SetNamePool( lpctstr pszName )
{
	ADDTOCALLSTACK("CObjBase::SetNamePool");
	ASSERT(pszName);

	// Parse out the name from the name pool ?
	if ( pszName[0] == '#' )
	{
		++pszName;
		tchar *pszTmp = Str_GetTemp();
		Str_CopyLimitNull( pszTmp, pszName, Str_TempLength() );

		tchar * ppTitles[2];
		Str_ParseCmds( pszTmp, ppTitles, ARRAY_COUNT(ppTitles));

		CResourceLock s;
		if ( ! g_Cfg.ResourceLock( s, RES_NAMES, ppTitles[0] ))
		{
			SetNamePool_Fail( ppTitles[0] );
			return false;
		}

		// Pick a random name.
		if ( ! s.ReadKey())
		{
			SetNamePool_Fail( ppTitles[0] );
			return false;
		}
		int iCount = g_Rand.GetVal2( 1, atoi( s.GetKey()) );
		while ( iCount > 0 )
		{
			if ( ! s.ReadKey())
			{
				SetNamePool_Fail( ppTitles[0] );
				return false;
			}
			--iCount;
		}

		if ( CObjBaseTemplate::SetName( s.GetKey() ) == false )
			return false;
	}
	else
	{
		lpctstr pszTmp = pszName;

		// NOTE: Name must be <= MAX_NAME_SIZE
		tchar szTmp[ MAX_ITEM_NAME_SIZE + 1 ];
		if ( strlen( pszName ) >= MAX_ITEM_NAME_SIZE )
		{
			Str_CopyLimitNull( szTmp, pszName, MAX_ITEM_NAME_SIZE );
			pszTmp = szTmp;
		}

		// Can't be a dupe name with type ?
		lpctstr pszTypeName = Base_GetDef()->GetTypeName();
		if ( ! strcmpi( pszTypeName, pszTmp ))
			pszTmp = "";

		if ( CObjBaseTemplate::SetName( pszTmp ) == false )
			return false;
	}

	UpdatePropertyFlag();
	return true;
}

bool CObjBase::MoveNearObj( const CObjBaseTemplate *pObj, ushort iSteps )
{
	ADDTOCALLSTACK("CObjBase::MoveNearObj");
	ASSERT(pObj);
	if ( pObj->IsDisconnected() )	// nothing is "near" a disconnected item.
		return false;

	pObj = pObj->GetTopLevelObj();
	return MoveNear(pObj->GetTopPoint(), iSteps);
}

void CObjBase::r_WriteSafe( CScript & s )
{
	ADDTOCALLSTACK("CObjBase::r_WriteSafe");
	// Write an object with some fault protection.
	dword uid = 0;
	try
	{
		uid = (dword)GetUID();

		//	objects with TAG.NOSAVE set are not saved
		const CVarDefCont* pVarNoSave = m_TagDefs.GetKey("NOSAVE");
		if (pVarNoSave && pVarNoSave->GetKey())
			return;

		if ( !g_Cfg.m_fSaveGarbageCollect )
		{
			if ( g_World.FixObj(this) )
				return;
		}
		r_Write(s);
	}
	catch ( const CSError& e )
	{
		g_Log.CatchEvent(&e, "Write Object 0%x", uid);
		GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
	}
	catch (...)	// catch all
	{
		g_Log.CatchEvent(nullptr, "Write Object 0%x", uid);
		GetCurrentProfileData().Count(PROFILE_STAT_FAULTS, 1);
	}
}

void CObjBase::Sound( SOUND_TYPE id, int iOnce ) const // Play sound effect for player
{
	ADDTOCALLSTACK("CObjBase::Sound");
	// play for everyone near by.

	if (( id <= 0 ) || !g_Cfg.m_fGenericSounds )
		return;

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if ( ! pClient->CanHear( this, TALKMODE_SOUND ) )
			continue;
		pClient->addSound( id, this, iOnce );
	}
}

void CObjBase::Effect(EFFECT_TYPE motion, ITEMID_TYPE id, const CObjBase * pSource,
    byte bSpeedSeconds, byte bLoop, bool fExplode, dword color, dword render, word effectid, word explodeid, word explodesound, dword effectuid, byte type) const
{
	ADDTOCALLSTACK("CObjBase::Effect");

    if ( motion == EFFECT_FADE_SCREEN )
    {
        // This effect must be used only on client chars (and send it only to this client)
        if (IsChar())
        {
            const CChar *pChar = static_cast<const CChar *>(this);
            if (pChar->IsClientActive())
            {
                pChar->GetClientActive()->addEffect(motion, id, pChar, nullptr);
            }
        }
        return;
    }

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if ( ! pClient->CanSee( this ) )
			continue;

		// Given the same bLoop, Enhanced Client shows the effect for a much shorter amount of time than Classic Client,
		//	so it may be a nice idea to adjust it automatically.
		byte bLoopAdjusted = bLoop;
		if (pClient->GetNetState()->isClientEnhanced())
			bLoopAdjusted *= 3;

		pClient->addEffect(motion, id, this, pSource, bSpeedSeconds, bLoopAdjusted, fExplode, color, render, effectid, explodeid, explodesound, effectuid, type);
	}
}

void CObjBase::EffectLocation(EFFECT_TYPE motion, ITEMID_TYPE id, const CPointMap *pptDest, const CPointMap *pptSrc,
    byte bSpeedSeconds, byte bLoop, bool fExplode, dword color, dword render, word effectid, word explodeid, word explodesound, dword effectuid, byte type) const
{
	ADDTOCALLSTACK("CObjBase::EffectLocation");

    if ( motion == EFFECT_FADE_SCREEN )
    {
        // This effect must be used only on client chars (and send it only to this client)
        if (IsChar())
        {
            const CChar *pChar = static_cast<const CChar *>(this);
            if (pChar->IsClientActive())
            {
                pChar->GetClientActive()->addEffect(motion, id, nullptr, nullptr);
            }
        }
        return;
    }

    // show for everyone nearby
	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if (!pClient->CanSee(this))
			continue;

		// Given the same bLoop, Enhanced Client shows the effect for a much shorter amount of time than Classic Client,
		//	so it may be a nice idea to adjust it automatically.
		byte bLoopAdjusted = bLoop;
		if (pClient->GetNetState()->isClientEnhanced())
			bLoopAdjusted *= 3;

		pClient->addEffectLocation(motion, id, pptDest, pptSrc, bSpeedSeconds, bLoopAdjusted, fExplode, color, render, effectid, explodeid, explodesound, effectuid, type);
	}
}

void CObjBase::Emote(lpctstr pText, CClient * pClientExclude, bool fForcePossessive)
{
	ADDTOCALLSTACK("CObjBase::Emote");
	// IF this is not the top level object then it might be possessive ?

	// "*You see NAME blah*" or "*You blah*"
	// fPosessive = "*You see NAME's blah*" or "*Your blah*"

	CObjBase *pObjTop = static_cast<CObjBase*>(GetTopLevelObj());
	if ( !pObjTop )
		return;

	tchar *pszThem = Str_GetTemp();
	tchar *pszYou = Str_GetTemp();

	if ( pObjTop->IsChar() )
	{
		// Someone has this equipped.

		if ( pObjTop != this )
		{
			snprintf(pszThem, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_1), pObjTop->GetName(), GetName(), pText);
			snprintf(pszYou, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_2), GetName(), pText);
		}
		else if ( fForcePossessive )
		{
			// ex. "You see joes poor shot ruin an arrow"
			snprintf(pszThem, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_3), GetName(), pText);
			snprintf(pszYou, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_4), pText);
		}
		else
		{
			snprintf(pszThem, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_5), GetName(), pText);
			snprintf(pszYou, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_6), pText);
		}
	}
	else
	{
		// Top level is an item. Article ?
		snprintf(pszThem, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_7), GetName(), pText);
		strcpy(pszYou, pszThem);
	}

	pObjTop->UpdateObjMessage(pszThem, pszYou, pClientExclude, HUE_TEXT_DEF, TALKMODE_EMOTE);
}

void CObjBase::EmoteObj(lpctstr pText)
{
	ADDTOCALLSTACK("CObjBase::EmoteObj");
	//This is function that only send an emote to a affacted character.

	CObjBase *pObjTop = static_cast<CObjBase*>(GetTopLevelObj());
	if ( !pObjTop )
		return;

	tchar *pszYou = Str_GetTemp();

	if ( pObjTop->IsChar() )
	{
		if ( pObjTop != this )
			snprintf(pszYou, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_2), GetName(), pText);
		else
			snprintf(pszYou, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_6), pText);
		pObjTop->UpdateObjMessage(nullptr, pszYou, nullptr, HUE_TEXT_DEF, TALKMODE_EMOTE);
	}
}

void CObjBase::Emote2(lpctstr pText, lpctstr pText1, CClient * pClientExclude, bool fForcePossessive)
{
	ADDTOCALLSTACK("CObjBase::Emote");
	// IF this is not the top level object then it might be possessive ?

	// "*You see NAME blah*" or "*You blah*"
	// fPosessive = "*You see NAME's blah*" or "*Your blah*"

	CObjBase *pObjTop = static_cast<CObjBase*>(GetTopLevelObj());
	if ( !pObjTop )
		return;

	tchar *pszThem = Str_GetTemp();
	tchar *pszYou = Str_GetTemp();

	if ( pObjTop->IsChar() )
	{
		// Someone has this equipped.
		if ( pObjTop != this )
		{
			snprintf(pszThem, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_1), pObjTop->GetName(), GetName(), pText1);
			snprintf(pszYou, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_2), GetName(), pText);
		}
		else if ( fForcePossessive )
		{
			// ex. "You see joes poor shot ruin an arrow"
			snprintf(pszThem, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_3), GetName(), pText1);
			snprintf(pszYou, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_4), pText);
		}
		else
		{
			snprintf(pszThem, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_5), GetName(), pText1);
			snprintf(pszYou, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_6), pText);
		}
	}
	else
	{
		// Top level is an item. Article ?
		snprintf(pszThem, Str_TempLength(), g_Cfg.GetDefaultMsg(DEFMSG_MSG_EMOTE_7), GetName(), pText1);
		strcpy(pszYou, pszThem);
	}

	pObjTop->UpdateObjMessage(pszThem, pszYou, pClientExclude, HUE_TEXT_DEF, TALKMODE_EMOTE);
}

// Speak to all clients in the area.
// ASCII packet
void CObjBase::Speak( lpctstr pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font )
{
	ADDTOCALLSTACK_DEBUG("CObjBase::Speak");
	CWorldComm::Speak( this, pText, wHue, mode, font );
}

// Speak to all clients in the area.
// Unicode packet
void CObjBase::SpeakUTF8( lpctstr pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	ADDTOCALLSTACK_DEBUG("CObjBase::SpeakUTF8");
	// convert UTF8 to UTF16 UNICODE.
	nachar szBuffer[ MAX_TALK_BUFFER ];
	CvtSystemToNETUTF16( szBuffer, ARRAY_COUNT(szBuffer), pText, -1 );
	CWorldComm::SpeakUNICODE( this, szBuffer, wHue, mode, font, lang );
}

// Speak to all clients in the area.
// Unicode packet
// Difference with SpeakUTF8: this method accepts as text input an nachar, which is a network aligned utf16 unicode characters array
void CObjBase::SpeakUTF8Ex( const nachar * pText, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, CLanguageID lang )
{
	ADDTOCALLSTACK_DEBUG("CObjBase::SpeakUTF8Ex");
	CWorldComm::SpeakUNICODE( this, pText, wHue, mode, font, lang );
}

bool CObjBase::MoveNear(CPointMap pt, ushort iSteps )
{
	ADDTOCALLSTACK("CObjBase::MoveNear");
	// Move to nearby this other object.
	// Actually move it within +/- iSteps

	CPointMap ptOld(pt);
	for ( uint i = 0; i < iSteps; ++i )
	{
		pt = ptOld;
		pt.m_x += (short)g_Rand.GetVal2(-iSteps, iSteps);
		pt.m_y += (short)g_Rand.GetVal2(-iSteps, iSteps);

		if ( !pt.IsValidPoint() )	// hit the edge of the world, so go back to the previous valid position
		{
			pt = ptOld;
			break;
		}
	}

	if ( IsChar() )
	{
		// Don't move to an position that we can't walk to
		CChar *pChar = static_cast<CChar *>(this);
		ASSERT(pChar);

		pChar->m_zClimbHeight = 0;
		if ( pChar->CanMoveWalkTo(pt, false) == nullptr )
			return false;
	}

	return MoveTo(pt);
}

void CObjBase::UpdateObjMessage( lpctstr pTextThem, lpctstr pTextYou, CClient * pClientExclude, HUE_TYPE wHue, TALKMODE_TYPE mode, FONT_TYPE font, bool fUnicode ) const
{
	ADDTOCALLSTACK("CObjBase::UpdateObjMessage");
	// Show everyone a msg coming from this object.

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if ( pClient == pClientExclude )
			continue;
		if ( ! pClient->CanSee( this ))
			continue;

		if (( pClient->GetChar() == this ) && pTextYou != nullptr )
			pClient->addBarkParse(pTextYou, this, wHue, mode, font, fUnicode );
		else if (( pClient->GetChar() != this ) && pTextThem != nullptr )
			pClient->addBarkParse(pTextThem, this, wHue, mode, font, fUnicode );

		//pClient->addBarkParse(( pClient->GetChar() == this )? pTextYou : pTextThem, this, wHue, mode, font, bUnicode );
	}
}

void CObjBase::UpdateCanSee(PacketSend *packet, CClient *exclude) const
{
	ADDTOCALLSTACK("CObjBase::UpdateCanSee");
	// Send this update message to everyone who can see this.
	// NOTE: Need not be a top level object. CanSee() will calc that.

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if (( pClient == exclude ) || !pClient->CanSee(this) )
			continue;

		packet->send(pClient);
	}
	delete packet;
}

TRIGRET_TYPE CObjBase::OnHearTrigger( CResourceLock & s, lpctstr pszCmd, CChar * pSrc, TALKMODE_TYPE & mode, HUE_TYPE wHue)
{
	ADDTOCALLSTACK("CObjBase::OnHearTrigger");
	// Check all the keys in this script section.
	// look for pattern or partial trigger matches.
	// RETURN:
	//  TRIGRET_ENDIF = no match.
	//  TRIGRET_DEFAULT = found match but it had no RETURN
    std::unique_ptr<CScriptTriggerArgs> Args;
	bool fMatch = false;

	while ( s.ReadKeyParse())
	{
		if ( s.IsKeyHead("ON",2))
		{
			// Look for some key word.
            tchar* ptcOn = s.GetArgStr();
			//_strupr(ptcOn); // Str_Match is already case insensitive
			if ( Str_Match( ptcOn, pszCmd ) == MATCH_VALID )
				fMatch = true;
			continue;
		}

		if ( ! fMatch )
			continue;	// look for the next "ON" section.

        if (!Args)
        {
            // Allocate when needed
            Args = std::make_unique<CScriptTriggerArgs>(pszCmd);
            Args->m_iN1 = mode;
            Args->m_iN2 = wHue;
        }
		TRIGRET_TYPE iRet = CObjBase::OnTriggerRunVal( s, TRIGRUN_SECTION_EXEC, pSrc, Args.get() );
		if ( iRet != TRIGRET_RET_FALSE )
			return iRet;

		fMatch = false;
	}

    if (Args)
	    mode = TALKMODE_TYPE(Args->m_iN1);
	return TRIGRET_ENDIF;	// continue looking.
}

enum OBR_TYPE
{
	OBR_ROOM,
	OBR_SECTOR,
	OBR_SPAWNITEM,
	OBR_TOPOBJ,
	OBR_TYPEDEF,
	OBR_QTY
};

lpctstr const CObjBase::sm_szRefKeys[OBR_QTY+1] =
{
	"ROOM",
	"SECTOR",
	"SPAWNITEM",
	"TOPOBJ",
	"TYPEDEF",
	nullptr
};

bool CObjBase::r_GetRef( lpctstr & ptcKey, CScriptObj * & pRef )
{
	ADDTOCALLSTACK("CObjBase::r_GetRef");
	int i = FindTableHeadSorted( ptcKey, sm_szRefKeys, ARRAY_COUNT(sm_szRefKeys)-1 );
	if ( i >= 0 )
	{
		ptcKey += strlen( sm_szRefKeys[i] );
		SKIP_SEPARATORS(ptcKey);
		switch (i)
		{
			case OBR_ROOM:
				pRef = GetTopLevelObj()->GetTopPoint().GetRegion( REGION_TYPE_ROOM );
				return true;
			case OBR_SECTOR:
				pRef = GetTopLevelObj()->GetTopSector();
				return true;
			case OBR_SPAWNITEM:
            {
                if (_uidSpawn.IsValidUID() && ptcKey[-1] != '.')
                    break;
                CItem *pItem = _uidSpawn.ItemFind();
                if (pItem)
                {
                    pRef = pItem;
                }
            }
				return true;
			case OBR_TOPOBJ:
				if ( ptcKey[-1] != '.' )	// only used as a ref !
					break;
				pRef = dynamic_cast <CObjBase*>(GetTopLevelObj());
				return true;
			case OBR_TYPEDEF:
				pRef = Base_GetDef();
				return true;
		}

	}
	return CScriptObj::r_GetRef(ptcKey, pRef);
}

enum OBC_TYPE
{
	#define ADD(a,b) OC_##a,
	#include "../tables/CObjBase_props.tbl"
	#undef ADD
	OC_QTY
};

lpctstr const CObjBase::sm_szLoadKeys[OC_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CObjBase_props.tbl"
	#undef ADD
	nullptr
};

bool CObjBase::r_WriteVal( lpctstr ptcKey, CSString &sVal, CTextConsole * pSrc, bool fNoCallParent, bool fNoCallChildren )
{
	ADDTOCALLSTACK("CObjBase::r_WriteVal");
	EXC_TRY("WriteVal");

    int index = FindTableHeadSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 );
    if ( !fNoCallChildren && (index < 0) )
    {
        const size_t uiFunctionIndex = r_GetFunctionIndex(ptcKey);
        if (r_CanCall(uiFunctionIndex))
        {
            // RES_FUNCTION call
            // Is it a function returning a value ? Parse args ?
            lpctstr pszArgs = strchr(ptcKey, ' ');
            if ( pszArgs != nullptr )
            {
                ++pszArgs;
                SKIP_SEPARATORS(pszArgs);
            }

            CScriptTriggerArgs Args( pszArgs != nullptr ? pszArgs : "" );
            if (r_Call(uiFunctionIndex, pSrc, &Args, &sVal))
            {
                return true;
            }
        }

        // Just try to default to something reasonable ?
        // Even though we have not really specified it correctly !

        // WORLD. ?
        //if (g_World.r_WriteVal(ptcKey, sVal, pSrc))
        //{
        //    return true;
        //}

        // TYPEDEF. ?
        if (Base_GetDef()->r_WriteVal(ptcKey, sVal, pSrc, false, true)) // do not call CEntityProps on the base (already done that on CItem/CChar::r_WriteVal
        {
            return true;
        }

		if (!fNoCallParent && CScriptObj::r_WriteVal( ptcKey, sVal, pSrc, false ))
        {
            return true;
        }

        return false;
	}

	bool fZero = false;
	switch (index)
	{
		//return as string, empty if not set.
		//On these ones, check BaseDef if not found on dynamic (CObjBase)
		case OC_ABILITYPRIMARY:
		case OC_ABILITYSECONDARY:
		case OC_ONAME:
		case OC_SLAYER:
		case OC_SLAYERLESSER:
		case OC_SLAYERMISC:
		case OC_SLAYERSUPER:
		{
			const CVarDefCont * pVar = GetDefKey(ptcKey, true);
			sVal = pVar ? pVar->GetValStr() : "";
		}
		break;

		//return as hex number or 0 if not set.
		//On these ones, check BaseDef if not found on dynamic (CObjBase)
		case OC_NAMELOC:
        case OC_RECIPEALCHEMY:
        case OC_RECIPEBLACKSMITH:
        case OC_RECIPEBOWCRAFT:
        case OC_RECIPECARPENTRY:
        case OC_RECIPECARTOPGRAHY:
        case OC_RECIPECOOKING:
        case OC_RECIPEGLASSBLOWING:
        case OC_RECIPEINSCRIPTION:
        case OC_RECIPEMASONRY:
        case OC_RECIPETAILORING:
        case OC_RECIPETINKERING:
        {
            const CVarDefCont * pVar = GetDefKey(ptcKey, true);
            sVal.FormatLLHex(pVar ? pVar->GetValNum() : 0);
        }
        break;

		case OC_ARMOR:
			{
				if ( IsChar() )
				{
					const CChar * pChar = static_cast<const CChar*>(this);
					sVal.FormatVal( pChar->m_defense );
					break;
				}
                else
                {
					ptcKey += strlen(sm_szLoadKeys[index]); // 9;
					if ( *ptcKey == '.' )
					{
						SKIP_SEPARATORS( ptcKey );

						if ( !strnicmp( ptcKey, "LO", 2 ) )
						{
							sVal.Format( "%d", m_defenseBase );
						}
						else if ( !strnicmp( ptcKey, "HI", 2 ) )
						{
							sVal.Format( "%d", m_defenseBase+m_defenseRange );
						}
					}
					else
					{
						sVal.Format( "%d,%d", m_defenseBase, m_defenseBase+m_defenseRange );
					}
				} break;
			}
		case OC_DAM:
			{
				ptcKey += strlen(sm_szLoadKeys[index]); // 9;
				if ( *ptcKey == '.' )
				{
					SKIP_SEPARATORS( ptcKey );

					if ( !strnicmp( ptcKey, "LO", 2 ) )
						sVal.Format( "%d", m_attackBase );
					else if ( !strnicmp( ptcKey, "HI", 2 ) )
						sVal.Format( "%d", m_attackBase+m_attackRange );
				}
				else
				{
					sVal.Format( "%d,%d", m_attackBase, m_attackBase+m_attackRange );
				}
			} break;
		case OC_CAN:
			sVal.FormatULLHex( GetCanFlags() );
			break;
		case OC_CANMASK:
			sVal.FormatULLHex( m_CanMask );
			break;
		case OC_MODMAXWEIGHT:
			sVal.FormatVal(m_ModMaxWeight);
			return true;
		case OC_CANSEE:
		case OC_CANSEELOS:
		case OC_CANSEELOSFLAG:
			{
				bool bCanSee = ( index == OC_CANSEE );
				bool bFlags = ( index == OC_CANSEELOSFLAG );
				CChar *pChar = pSrc->GetChar();
				word flags = 0;

				ptcKey += ( bCanSee ? 6 : (bFlags ? 13 : 9) );
				SKIP_SEPARATORS(ptcKey);
				GETNONWHITESPACE(ptcKey);

				if ( bFlags && *ptcKey )
				{
					flags = Exp_GetWVal(ptcKey);
					SKIP_ARGSEP(ptcKey);
				}
				if ( *ptcKey )		// has an argument - UID to see(los) or POS to los only
				{
					CPointMap pt;
					CObjBase *pObj = nullptr;

					if ( !bCanSee )
						pt = g_Cfg.GetRegionPoint(ptcKey);

					if ( bCanSee || !pt.IsValidPoint() )
					{
						pObj = CUID::ObjFindFromUID(Exp_GetDWVal(ptcKey));
						if ( !bCanSee && pObj )
							pt = pObj->GetTopPoint();
					}

					pChar = GetUID().CharFind();

					if ( pChar )
						sVal.FormatVal(bCanSee ? pChar->CanSee(pObj) : pChar->CanSeeLOS(pt, nullptr, pChar->GetVisualRange(), flags));
					else
						sVal.FormatVal(0);
				}
				else if ( !pChar )		// no char -> no see
					sVal.FormatVal(0);
				else					// standart way src TO current object
					sVal.FormatVal(bCanSee ? pChar->CanSee(this) : pChar->CanSeeLOS(this, (word)(flags)));
			}
			break;
		case OC_COLOR:
			sVal.FormatHex( GetHue() );
			break;
		case OC_COMPLEXITY:
			{
				if ( IsDisconnected() || !GetTopLevelObj()->GetTopPoint().IsValidPoint() )
					return false;
				return GetTopLevelObj()->GetTopSector()->r_WriteVal( ptcKey, sVal, pSrc );
			}
		case OC_CTAGCOUNT:
			{
				CChar * pChar = dynamic_cast<CChar*>(this);
				if ( !pChar )
					sVal.FormatVal( 0 );
				else
					sVal.FormatSTVal( pChar->IsClientActive() ? (pChar->GetClientActive()->m_TagDefs.GetCount()) : 0 );
			}
			break;
		case OC_TEXTF:
			{
				tchar * key = const_cast<tchar*>(ptcKey);
				key += 5;
				tchar * ptcArg[4];
				int iArgQty = Str_ParseCmds(key, ptcArg, ARRAY_COUNT(ptcArg));
				if (iArgQty < 2)
				{
					g_Log.EventError("SysMessagef with less than 1 args for the given text\n");
					return false;
				}
				if (iArgQty > 4)
				{
					g_Log.EventError("Too many arguments given to SysMessagef (max = text + 3\n");
					return false;
				}
				//strip quotes if any
				if (*ptcArg[0] == '"')
					++ptcArg[0];
				//byte count = 0;
				for (tchar * pEnd = ptcArg[0] + strlen(ptcArg[0]) - 1; pEnd >= ptcArg[0]; --pEnd)
				{
					if (*pEnd == '"')
					{
						*pEnd = '\0';
						break;
					}
					//++count;
				}
				sVal.Format(ptcArg[0], ptcArg[1], ptcArg[2] ? ptcArg[2] : 0, ptcArg[3] ? ptcArg[3] : 0);
				return true;
			} break;
		case OC_DIALOGLIST:
			{
				ptcKey += 10;
				if ( *ptcKey == '.' )
				{
					SKIP_SEPARATORS( ptcKey );
					GETNONWHITESPACE( ptcKey );

					CClient * pThisClient = pSrc->GetChar() ? ( pSrc->GetChar()->IsClientActive() ? pSrc->GetChar()->GetClientActive() : nullptr ) : nullptr;
					sVal.FormatVal(0);

					if ( pThisClient )
					{
						if( !strnicmp(ptcKey, "COUNT", 5) )
						{
							sVal.FormatSTVal( pThisClient->m_mapOpenedGumps.size() );
						}
						else
						{
							const CClient::OpenedGumpsMap_t &ourMap = pThisClient->m_mapOpenedGumps;
							uint uiDialogIndex = Exp_GetUVal(ptcKey);
							SKIP_SEPARATORS(ptcKey);

							if ( uiDialogIndex <= ourMap.size() )
							{
								CClient::OpenedGumpsMap_t::const_iterator itGumpFound = std::next(ourMap.begin(), uiDialogIndex);

								if ( !strnicmp(ptcKey, "ID", 2) )
								{
									sVal.Format("%s", g_Cfg.ResourceGetName( CResourceID(RES_DIALOG, ResGetIndex(itGumpFound->first), RES_PAGE_ANY)) );
								}
								else if ( !strnicmp(ptcKey, "COUNT", 5) )
								{
									sVal.FormatVal( itGumpFound->second );
								}
							}
						}
					}
					else
					{
						DEBUG_ERR(( "DIALOGLIST called on non-client object.\n" ));
					}

					return true;
				}
				else
					return false;
			}
			break;
		case OC_DISTANCE:
			{
				ptcKey += 8;
				SKIP_SEPARATORS( ptcKey );
				GETNONWHITESPACE( ptcKey );
				CObjBase * pObj = pSrc->GetChar();

				CObjBase * pThis = this;
				if ( !IsTopLevel() )
					pThis = dynamic_cast <CObjBase*>( GetTopLevelObj() );
				if ( !pThis )
					return false;

				if ( *ptcKey )
				{
					CPointMap pt = g_Cfg.GetRegionPoint( ptcKey );

					if ( pt.IsValidPoint() )
					{
						if ( !pThis->GetTopPoint().IsValidPoint() )
							return false;
						sVal.FormatVal( pThis->GetTopDist( pt ) );
						return true;
					}

					pObj = CUID::ObjFindFromUID(Exp_GetVal(ptcKey));
                    SKIP_SEPARATORS(ptcKey);
                    GETNONWHITESPACE(ptcKey);
				}

				if ( pObj && !pObj->IsTopLevel() )
					pObj = dynamic_cast <CObjBase*>( pObj->GetTopLevelObj() );
				if ( !pObj )
					return false;
				sVal.FormatVal( pThis->GetDist( pObj ) );
			}
			break;
		case OC_EVENTS:
			m_OEvents.WriteResourceRefList( sVal );
			break;
		case OC_FACING:
			{
				ptcKey += 6;
				SKIP_SEPARATORS(ptcKey);
				GETNONWHITESPACE(ptcKey);

				CObjBase * pObj = pSrc->GetChar();

				CObjBase * pThis = this;
				if (!IsTopLevel())
					pThis = dynamic_cast <CObjBase*>(GetTopLevelObj());
				if (!pThis)
					return false;

				if (*ptcKey)
				{
					CPointMap pt = g_Cfg.GetRegionPoint(ptcKey);

					if (pt.IsValidPoint())
					{
						if (!pThis->GetTopPoint().IsValidPoint())
							return false;
						else
							sVal.FormatVal(pThis->GetTopPoint().GetDir(pt));
						return true;
					}

					CUID uid(Exp_GetVal(ptcKey));
					SKIP_SEPARATORS(ptcKey);
                    GETNONWHITESPACE(ptcKey);
					pObj = uid.ObjFind();
				}

				if (!pObj)
					return false;
				if (!pObj->IsTopLevel())
					pObj = dynamic_cast <CObjBase*>(pObj->GetTopLevelObj());

				sVal.FormatVal(pThis->GetDir(pObj));
				break;
			}
        case OC_HASCOMPONENTPROPS:
        {
            ptcKey += 17;
            SKIP_SEPARATORS(ptcKey);
            GETNONWHITESPACE(ptcKey);

            COMPPROPS_TYPE id = (COMPPROPS_TYPE)Exp_GetVal(ptcKey);
            bool fRes = (id < COMP_PROPS_QTY) && (nullptr != CEntityProps::GetComponentProps(id));
            sVal.FormatVal(int(fRes));
            break;
        }
		case OC_ISCHAR:
			sVal.FormatVal( IsChar());
			break;
		case OC_ISEVENT:
			if ( ptcKey[7] != '.' )
				return false;
			ptcKey += 8;
			sVal = m_OEvents.ContainsResourceName(RES_EVENTS, ptcKey) ? "1" : "0";
            break;
		case OC_ISTEVENT:
			if ( ptcKey[8] != '.' )
				return false;
			ptcKey += 9;
			sVal = Base_GetDef()->m_TEvents.ContainsResourceName(RES_EVENTS, ptcKey) ? "1" : "0";
            break;
		case OC_ISITEM:
			sVal.FormatVal( IsItem());
			break;
		case OC_ISCONT:
			sVal.FormatVal( IsContainer());
			break;
		case OC_ISNEARTYPETOP:
		case OC_ISNEARTYPE:
			{
				bool fP = false;
				ptcKey	+= ( index == OC_ISNEARTYPETOP ) ? 13 : 10;
				if ( !strnicmp( ptcKey, ".P", 2 ) )
				{
					fP	= true;
					ptcKey	+= 2;
				}
				SKIP_SEPARATORS( ptcKey );
				SKIP_ARGSEP( ptcKey );

				if ( !GetTopPoint().IsValidPoint() )
					sVal.FormatVal( 0 );
				else
				{
					int iType = g_Cfg.ResourceGetIndexType( RES_TYPEDEF, ptcKey );
					int iDistance;
					bool fCheckMulti;
					bool fCheckMultiZ = false;

					SKIP_IDENTIFIERSTRING( ptcKey );
					SKIP_SEPARATORS( ptcKey );
					SKIP_ARGSEP( ptcKey );

					if ( !*ptcKey )
						iDistance	= 0;
					else
						iDistance	= Exp_GetVal( ptcKey );

					if ( !*ptcKey )
						fCheckMulti = false;
					else
						fCheckMulti = Exp_GetVal( ptcKey ) != 0;

					if (fCheckMulti && *ptcKey)
						fCheckMultiZ = Exp_GetVal(ptcKey) != 0;

					if ( fP )
					{
						CPointMap pt = ( index == OC_ISNEARTYPETOP ) ? ( CWorldMap::FindTypeNear_Top(GetTopPoint(), (IT_TYPE)iType, iDistance ) ) : ( CWorldMap::FindItemTypeNearby(GetTopPoint(), (IT_TYPE)iType, iDistance, fCheckMulti, fCheckMultiZ) );

						if ( !pt.IsValidPoint() )
							sVal.FormatVal( 0 );
						else
							sVal = pt.WriteUsed();
					}
					else
						sVal.FormatVal( ( index == OC_ISNEARTYPETOP ) ? ( CWorldMap::IsTypeNear_Top(GetTopPoint(), (IT_TYPE)iType, iDistance ) ) : ( CWorldMap::IsItemTypeNear(GetTopPoint(), (IT_TYPE)iType, iDistance, fCheckMulti ) ) );
				}
			}
			break;
		case OC_ISPLAYER:
			{
				CChar * pChar = dynamic_cast<CChar*>(this);
				if ( pChar )
					sVal.FormatVal( (pChar->m_pPlayer == nullptr) ? 0 : 1 );
				else
					sVal.FormatVal( 0 );
                break;
			}
        case OC_ISSLEEPING:
        {
            sVal.FormatVal(IsSleeping());
            break;
        }
		case OC_ISDIALOGOPEN:
			{
				ptcKey += 12;
				SKIP_SEPARATORS( ptcKey );
				GETNONWHITESPACE( ptcKey );
				CChar * pCharToCheck = dynamic_cast<CChar*>(this);
				CClient * pClientToCheck = (pCharToCheck && pCharToCheck->IsClientActive()) ? (pCharToCheck->GetClientActive()) : nullptr;

				if ( pClientToCheck )
				{
					CResourceID rid = g_Cfg.ResourceGetIDType( RES_DIALOG, ptcKey );
					uint context;
					if ( pClientToCheck->GetNetState()->isClientKR() )
					{
						context = g_Cfg.GetKRDialog(rid.GetPrivateUID());
					}
					else
					{
						context = rid.GetPrivateUID();
					}

					CClient::OpenedGumpsMap_t::const_iterator itGumpFound = pClientToCheck->m_mapOpenedGumps.find( context );
					if ( itGumpFound != pClientToCheck->m_mapOpenedGumps.end() )
					{
						sVal.FormatVal( itGumpFound->second );
					}
					else
					{
						sVal.FormatVal( 0 );
					}
				}
				else
				{
					sVal.FormatVal( 0 );
				}

                break;
			}
		case OC_ISARMOR:
			{
			ptcKey += 7;
			SKIP_SEPARATORS( ptcKey );
			GETNONWHITESPACE( ptcKey );
			CItem * pItem = nullptr;
			if ( *ptcKey )
			{
				tchar * ptcArg = Str_GetTemp();
                Str_CopyLimitNull( ptcArg, ptcKey, strlen( ptcKey ) + 1 );

                pItem = CUID::ItemFindFromUID(Exp_GetDWVal(ptcKey));
				if (pItem == nullptr)
				{
					ITEMID_TYPE id = (ITEMID_TYPE)(g_Cfg.ResourceGetID(RES_ITEMDEF, ptcArg).GetResIndex());
					const CItemBase * pItemDef = CItemBase::FindItemBase( id );
					if ( pItemDef != nullptr )
					{
						sVal.FormatVal( CItemBase::IsTypeArmor( pItemDef->GetType() ) );
						break;
					}
				}
				sVal.FormatVal( pItem ? pItem->IsTypeArmor() : 0 );
				break;
			}
			pItem = dynamic_cast<CItem*> (this);
			sVal.FormatVal( pItem ? pItem->IsTypeArmor() : 0 );
			break;
			}
		case OC_ISTIMERF:
			{
				if ( ptcKey[8] != '.' )
					return false;
				ptcKey += 9;
				//sVal.FormatVal( (g_World._TimedFunctions.IsTimer(GetUID(),ptcKey)) ? 1 : 0 );
				sVal.FormatLLVal( CWorldTimedFunctions::IsTimer(GetUID(), ptcKey) );
                break;
			}
			break;
		case OC_ISWEAPON:
			{
			ptcKey += 8;
			SKIP_SEPARATORS( ptcKey );
			GETNONWHITESPACE( ptcKey );
			CItem * pItem = nullptr;
			if ( *ptcKey )
			{
				tchar * ptcArg = Str_GetTemp();
				Str_CopyLimitNull( ptcArg, ptcKey, strlen( ptcKey ) + 1 );

				pItem = CUID::ItemFindFromUID(Exp_GetDWVal(ptcKey));
				if ( pItem == nullptr )
				{
					ITEMID_TYPE id = (ITEMID_TYPE)(g_Cfg.ResourceGetID(RES_ITEMDEF, ptcArg).GetResIndex());
					const CItemBase * pItemDef = CItemBase::FindItemBase( id );
					if (pItemDef != nullptr)
					{
						sVal.FormatVal( CItemBase::IsTypeWeapon( pItemDef->GetType() ) );
						break;
					}
				}
				sVal.FormatVal( pItem ? pItem->IsTypeWeapon() : 0 );
				break;
			}
			pItem = dynamic_cast<CItem*> (this);
			sVal.FormatVal( pItem ? pItem->IsTypeWeapon() : 0 );
			break;
			}
		case OC_MAP:
			sVal.FormatVal( GetUnkPoint().m_map);
			break;
		case OC_MODAR:
		case OC_MODAC:
			sVal.FormatVal( m_ModAr );
			break;
		case OC_NAME:
			sVal = GetName();
			break;
		case OC_P:
			if ( ptcKey[1] == '.' )
				return GetUnkPoint().r_WriteVal( ptcKey+2, sVal );
			sVal = GetUnkPoint().WriteUsed();
			break;
		case OC_TAG0:
			fZero = true;
			++ptcKey;
			FALLTHROUGH;
		case OC_TAG:			// "TAG" = get/set a local tag.
			{
				if ( ptcKey[3] != '.' )
					return false;
				ptcKey += 4;

				CVarDefCont * pVarKey = m_TagDefs.GetKey( ptcKey );
				if ( !pVarKey )
					sVal = Base_GetDef()->m_TagDefs.GetKeyStr( ptcKey, fZero );
				else
                    sVal = m_TagDefs.GetKeyStr(ptcKey, fZero);
			}
            break;
		case OC_TIMER:
			sVal.FormatLLVal(_GetTimerSAdjusted() );
			break;
		case OC_TIMERD:
            sVal.FormatLLVal(_GetTimerDAdjusted());
            break;
        case OC_TIMERMS:
            sVal.FormatLLVal(_GetTimerAdjusted());
            break;
		case OC_TRIGGER:
			{
				ptcKey += 7;
				GETNONWHITESPACE( ptcKey );

				if ( *ptcKey )
				{
					TRIGRET_TYPE trReturn = TRIGRET_RET_FALSE;
                    _iCallingObjTriggerId = _iRunningTriggerId;
					const bool fTrigReturn = CallPersonalTrigger(const_cast<tchar *>(ptcKey), pSrc, trReturn);
					_iCallingObjTriggerId = -1;
					if ( fTrigReturn )
						sVal.FormatVal(trReturn);

					return fTrigReturn;
				}
			} return false;
		case OC_TOPOBJ:
			if ( ptcKey[6] == '.' )
				return CScriptObj::r_WriteVal( ptcKey, sVal, pSrc );
			sVal.FormatHex(GetTopLevelObj()->GetUID());
			break;
		case OC_UID:
			if ( ptcKey[3] == '.' )
				return CScriptObj::r_WriteVal( ptcKey, sVal, pSrc );
			FALLTHROUGH;
		case OC_SERIAL:
			sVal.FormatHex( GetUID() );
			break;
		case OC_SPAWNITEM:
            if (const CCSpawn* pSpawn = GetSpawn())
            {
                if (ptcKey[9] == '.')
                    return CScriptObj::r_WriteVal(ptcKey, sVal, pSrc);
                sVal.FormatHex(pSpawn->GetLink()->GetUID());
            }
            else
            {
                sVal.FormatHex(0);
            }
			break;
		case OC_SEXTANTP:
			{
				ptcKey += 8;
				SKIP_SEPARATORS( ptcKey );
                GETNONWHITESPACE( ptcKey );

				CPointMap pt;
				if ( *ptcKey )
					pt = g_Cfg.GetRegionPoint( ptcKey );
				else
					pt = this->GetUnkPoint();

				if ( !pt.IsValidPoint() )
					return false;

				sVal = g_Cfg.Calc_MaptoSextant(pt);
		} break;
		case OC_SPEED:
		{
			if ( !this->IsItem() )
				return false;
			CItem * pItem = static_cast<CItem*>(this);
			sVal.FormatVal( pItem->GetSpeed() );
		}	break;
		case OC_TIMESTAMP:
			sVal.FormatLLVal( GetTimeStampS() );
			break;
		case OC_VERSION:
			sVal = SPHERE_BUILD_INFO_STR;
			break;
		case OC_WEIGHT:
			sVal.FormatVal( GetWeight() );
			break;
		case OC_Z:
			sVal.FormatVal( GetUnkZ() );
			break;
		case OC_TAGAT:
			{
 				ptcKey += 5;	// eat the 'TAGAT'
 				if ( *ptcKey == '.' )	// do we have an argument?
 				{
 					SKIP_SEPARATORS( ptcKey );
 					size_t iQty = Exp_GetSTVal( ptcKey );
					if ( iQty >= m_TagDefs.GetCount() )
 						return false; // trying to get non-existant tag

 					const CVarDefCont * pTagAt = m_TagDefs.GetAt( iQty );
 					if ( !pTagAt )
 						return false; // trying to get non-existant tag

 					SKIP_SEPARATORS( ptcKey );
 					if ( ! *ptcKey )
 					{
 						sVal.Format("%s=%s", pTagAt->GetKey(), pTagAt->GetValStr());
 					}
 					else if ( !strnicmp( ptcKey, "KEY", 3 )) // key?
 					{
 						sVal = pTagAt->GetKey();
 					}
 					else if ( !strnicmp( ptcKey, "VAL", 3 )) // val?
 					{
 						sVal = pTagAt->GetValStr();
 					}
                    else
                    {
                        return false;
                    }
                    break;
 				}
			return false;
			}
			break;
		case OC_TAGCOUNT:
			sVal.FormatSTVal( m_TagDefs.GetCount() );
			break;
		case OC_CLILOC:
			{
				ptcKey += 6;
				if ( *ptcKey == '.' )
				{
					SKIP_SEPARATORS( ptcKey );
					size_t iQty = Exp_GetSTVal( ptcKey );
					if ( iQty >= m_TooltipData.size() )
						return false;

					CClientTooltip* ct = m_TooltipData[iQty].get();
					if (!ct)
						return false;

					SKIP_SEPARATORS( ptcKey );
					if (! *ptcKey )
						sVal.Format("%d=%s", ct->m_clilocid, ct->m_args);
					else if ( !strnicmp( ptcKey, "ID", 2 )) //Cliloc.
						sVal.FormatDWVal(ct->m_clilocid);
					else if ( !strnicmp( ptcKey, "VAL", 3 )) //Arguments.
						sVal = ct->m_args;
					else
						return false;
					break;
				}
			return false;
			}
			break;
		case OC_CLILOCCOUNT:
			sVal.FormatSTVal( m_TooltipData.size() );
			break;
		case OC_PROPSAT:
			{
 				ptcKey += 7;	// eat the 'TAGAT'
 				if ( *ptcKey == '.' )	// do we have an argument?
 				{
 					SKIP_SEPARATORS( ptcKey );
 					size_t iQty = Exp_GetSTVal( ptcKey );
					if ( iQty >= m_BaseDefs.GetCount() )
 						return false; // trying to get non-existant tag

 					const CVarDefCont * pTagAt = m_BaseDefs.GetAt( iQty );
 					if ( !pTagAt )
 						return false; // trying to get non-existant tag

 					SKIP_SEPARATORS( ptcKey );
 					if ( ! *ptcKey )
 					{
 						sVal.Format("%s=%s", pTagAt->GetKey(), pTagAt->GetValStr());
 					}
 					else if ( !strnicmp( ptcKey, "KEY", 3 )) // key?
 					{
 						sVal = pTagAt->GetKey();
 					}
 					else if ( !strnicmp( ptcKey, "VAL", 3 )) // val?
 					{
 						sVal = pTagAt->GetValStr();
 					}
                    else
                    {
                        return false;
                    }
                    break;
 				}

			return false;
			}
			break;
		case OC_PROPSCOUNT:
			sVal.FormatSTVal( m_BaseDefs.GetCount() );
			break;
		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_KEYRET(pSrc);
	EXC_DEBUG_END;
	return false;
}

bool CObjBase::r_LoadVal( CScript & s )
{
	ADDTOCALLSTACK("CObjBase::r_LoadVal");
	// load the basic stuff.
	EXC_TRY("LoadVal");
	// we're using FindTableSorted so we must do this here.
	// Using FindTableHeadSorted instead would result in keywords
	// starting with "P" not working, for instance :)

    lpctstr ptcKey = s.GetKey();
    if (!strnicmp("TAG", ptcKey, 3))
    {
        if ((ptcKey[3] == '.') || (ptcKey[3] == '0'))
        {
            const bool fZero = (ptcKey[3] == '0');
            ptcKey = ptcKey + (fZero ? 5 : 4);
            bool fQuoted = false;
            lpctstr ptcArg = s.GetArgStr(&fQuoted);
            m_TagDefs.SetStr(ptcKey, fQuoted, ptcArg, fZero);
            return true;
        }
    }

	int index = FindTableSorted( ptcKey, sm_szLoadKeys, ARRAY_COUNT( sm_szLoadKeys )-1 );
	if ( index < 0 )
	{
        return CScriptObj::r_LoadVal(s);
	}

    bool fResendTooltip = false;
	switch ( index )
	{
		//Set as Strings
		case OC_ABILITYPRIMARY:
		case OC_ABILITYSECONDARY:
		case OC_ONAME:
		case OC_SLAYER:
		case OC_SLAYERLESSER:
		case OC_SLAYERMISC:
		case OC_SLAYERSUPER:
		{
			bool fQuoted = false;
			SetDefStr(s.GetKey(), s.GetArgStr( &fQuoted ), fQuoted);
			fResendTooltip = true;
		}
		break;

		//Set as number only
		case OC_NAMELOC:
        case OC_RECIPEALCHEMY:
        case OC_RECIPEBLACKSMITH:
        case OC_RECIPEBOWCRAFT:
        case OC_RECIPECARPENTRY:
        case OC_RECIPECARTOPGRAHY:
        case OC_RECIPECOOKING:
        case OC_RECIPEGLASSBLOWING:
        case OC_RECIPEINSCRIPTION:
        case OC_RECIPEMASONRY:
        case OC_RECIPETAILORING:
        case OC_RECIPETINKERING:
            SetDefNum(s.GetKey(), s.GetArgLLVal());
            fResendTooltip = true;
            break;
		case OC_ARMOR:
        {
            if (IsChar())
                return false;

            int64 piVal[2];
            size_t iQty = Str_ParseCmds(s.GetArgStr(), piVal, ARRAY_COUNT(piVal));
            m_defenseBase = (word)(piVal[0]);
            if (iQty > 1)
                m_defenseRange = (word)(piVal[1]) - m_defenseBase;
            else
                m_defenseRange = 0;
            CChar * pChar = dynamic_cast <CChar*>(GetTopLevelObj());
            if (pChar)
                pChar->UpdateStatsFlag();
            fResendTooltip = true;
            break;
        }
        case OC_DAM:
        {
            int64 piVal[2];
            size_t iQty = Str_ParseCmds(s.GetArgStr(), piVal, ARRAY_COUNT(piVal));
            m_attackBase = (word)(piVal[0]);
            if (iQty > 1)
                m_attackRange = (word)(piVal[1]) - m_attackBase;
            else
                m_attackRange = 0;
            CChar * pChar = dynamic_cast <CChar*>(GetTopLevelObj());
            if (pChar)
                pChar->UpdateStatsFlag();
            fResendTooltip = true;
        }
        break;

        case OC_CAN:
            return false;
        case OC_CANMASK:
        {
            m_CanMask = s.GetArgULLVal();
            if (IsItem())
            {
                CItem* pItem = static_cast<CItem*>(this);
                CCItemDamageable *pItemDmg = static_cast<CCItemDamageable*>(GetComponent(COMP_ITEMDAMAGEABLE));
                if (!pItemDmg && CCItemDamageable::CanSubscribe(pItem))
                {
                    SubscribeComponent(new CCItemDamageable(pItem));
                }
                else if (pItemDmg && !CCItemDamageable::CanSubscribe(pItem))
                {
                    UnsubscribeComponent(pItemDmg);
                }
                else
                {
                    break;
                }
                Update();   // Required to force the client to allow dragging the item's bar or to do not allow it anymore before trying to do it.
            }
            break;
        }
        case OC_MODMAXWEIGHT:
        {
            m_ModMaxWeight = s.GetArgVal();
            CChar * pChar = dynamic_cast <CChar*>(GetTopLevelObj());
            if (pChar)
                pChar->UpdateStatsFlag();
            fResendTooltip = true;
        }
        break;
        case OC_COLOR:
        {
            const tchar* ptcArg = s.GetArgStr();
            if (!strnicmp(ptcArg, "match_shirt", 11) || !strnicmp(ptcArg, "match_hair", 10))
            {
                const CChar * pChar = dynamic_cast <CChar*>(GetTopLevelObj());
                if (pChar)
                {
                    const CItem * pHair = pChar->LayerFind(!strnicmp(ptcArg+6, "shirt", 5) ? LAYER_SHIRT : LAYER_HAIR);
                    if (pHair)
                    {
                        m_wHue = pHair->GetHue();
                        break;
                    }
                }
                m_wHue = HUE_DEFAULT;
                break;
            }
            const HUE_TYPE hue = (HUE_TYPE)s.GetArgVal();
            SetHue(hue, false, &g_Serv); //@Dye is called from @Create/.xcolor/script command here // since we can not receive pSrc on this r_LoadVal function ARGO/SRC will be null
            if (!g_Serv.IsLoading())
                Update();
        }
        break;
		case OC_EVENTS:
			return m_OEvents.r_LoadVal( s, RES_EVENTS );
		case OC_MAP:
			// Move to another map
			if ( ! IsTopLevel())
				return false;
			{
				CPointMap pt = GetTopPoint();
				pt.m_map = (uchar)(s.GetArgVal());

				//	is the desired mapplane allowed?
				if ( !g_MapList.IsMapSupported(pt.m_map) )
					return false;

				MoveTo(pt);
				if (IsItem())
					Update();
			}
			break;
		case OC_MODAR:
		case OC_MODAC:
			{
				m_ModAr = s.GetArgVal();
				CChar * pChar = dynamic_cast <CChar*>(GetTopLevelObj());
				if ( pChar )
				{
					pChar->m_defense = (word)(pChar->CalcArmorDefense());
					pChar->UpdateStatsFlag();
				}
                fResendTooltip = true;
			}
			break;
		case OC_NAME:
			SetName( s.GetArgStr());
            fResendTooltip = true;
			break;
		case OC_P:	// Must set the point via the CItem or CChar methods.
			return false;
		case OC_SPEED:
		{
			if (!IsItem())
				return false;
			CItem * pItem = static_cast<CItem*>(this);
			pItem->m_speed = s.GetArgBVal();
			fResendTooltip = true;
			break;
		}
		case OC_TIMER:
        {
            int64 iTimeout = s.GetArg64Val();
            if (g_Serv.IsLoading())
            {
                const int iPrevBuild = g_World.m_iPrevBuild;
                /*
                * Newer X builds have a different timer stored on saves (storing the msec in which it is going to tick instead of the seconds until it ticks)
                * So the new timer will be the current time in msecs (SetTimeout)
                * For older builds, the timer is stored in seconds (SetTimeoutD)
                */
                if (iPrevBuild && (iPrevBuild >= 2866)) // commit #e08723c54b0a4a3b1601eba6f34a6118891f1313
                {
					// If TIMER = 0 was saved it means that at the moment of the worldsave the timer was elapsed but its object could not tick,
					//	since it was waiting a GoAwake() call. Now set the timer to tick asap.
                    _SetTimeout(iTimeout);   // new timer: set msecs timer
                    break;
                }
            }
            fResendTooltip = true;  // not really need to even try to resend it on load, but resend otherwise.
            _SetTimeoutS(iTimeout); // old timer: in seconds.
            break;
        }
        case OC_TIMERD:
			_SetTimeoutD(s.GetArgLLVal());
            fResendTooltip = true;
            break;
        case OC_TIMERMS:
			_SetTimeout(s.GetArgLLVal());
            fResendTooltip = true;
            break;
		case OC_TIMESTAMP:
			SetTimeStampS(s.GetArgLLVal());
			break;
		case OC_SPAWNITEM:
            if ( !g_Serv.IsLoading() )	// SPAWNITEM is read-only
                return false;
            _uidSpawn.SetObjUID(s.GetArgDWVal());
            break;
		case OC_UID:
		case OC_SERIAL:
			// Don't set container flags through this.
			SetUID( s.GetArgDWVal(), (dynamic_cast <const CItem*>(this)) ? true : false );
			break;
		default:
			return false;
	}
    if (fResendTooltip)
    {
        UpdatePropertyFlag();
    }
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPT;
	EXC_DEBUG_END;
	return false;
}

void CObjBase::r_Write( CScript & s )
{
	ADDTOCALLSTACK_DEBUG("CObjBase::r_Write");
	s.WriteKeyHex( "SERIAL", GetUID().GetObjUID());
	if ( IsIndividualName() )
		s.WriteKeyStr( "NAME", GetIndividualName());
	if ( m_wHue != HUE_DEFAULT )
		s.WriteKeyHex( "COLOR", GetHue());
	if ( _IsTimerSet() )
		s.WriteKeyVal( "TIMER", _GetTimerAdjusted());
	if ( m_iTimeStampS > 0 )
		s.WriteKeyVal( "TIMESTAMP", GetTimeStampS());
	if ( const CCSpawn* pSpawn = GetSpawn() )
		s.WriteKeyHex("SPAWNITEM", pSpawn->GetLink()->GetUID().GetObjUID());
	if ( m_ModAr )
		s.WriteKeyVal("MODAR", m_ModAr);
	if ( m_ModMaxWeight )
		s.WriteKeyVal("MODMAXWEIGHT", m_ModMaxWeight);

	// Write New variables
	m_BaseDefs.r_WritePrefix(s);

	m_TagDefs.r_WritePrefix(s, "TAG");
	m_OEvents.r_Write(s, "EVENTS");
}

enum OV_TYPE
{
	#define ADD(a,b) OV_##a,
	#include "../tables/CObjBase_functions.tbl"
	#undef ADD
	OV_QTY
};

lpctstr const CObjBase::sm_szVerbKeys[OV_QTY+1] =
{
	#define ADD(a,b) b,
	#include "../tables/CObjBase_functions.tbl"
	#undef ADD
	nullptr
};

bool CObjBase::r_Verb( CScript & s, CTextConsole * pSrc ) // Execute command from script
{
	ADDTOCALLSTACK("CObjBase::r_Verb");
	lpctstr	ptcKey = s.GetKey();
	ASSERT(pSrc);

	EXC_TRY("Verb-Special");

	if ( !strnicmp(ptcKey, "CLEARTAGS", 9) )
	{
		ptcKey = s.GetArgStr();
		SKIP_SEPARATORS(ptcKey);
		m_TagDefs.ClearKeys(ptcKey);
		return true;
	}

	int	index;
	if ( !strnicmp( ptcKey, "TARGET", 6 ) )
		index = OV_TARGET;
	else
		index = FindTableSorted( ptcKey, sm_szVerbKeys, ARRAY_COUNT(sm_szVerbKeys)-1 );
    if (index < 0)
    {
        const size_t uiFunctionIndex = r_GetFunctionIndex(ptcKey);
        if (r_CanCall(uiFunctionIndex))
        {
            // RES_FUNCTION call
            CSString sVal;
            CScriptTriggerArgs Args( s.GetArgRaw() );
            if ( r_Call( uiFunctionIndex, pSrc, &Args, &sVal ) )
                return true;
        }

        if (CScriptObj::r_Verb(s, pSrc))
            return true;

        return false;
    }

	EXC_SET_BLOCK("Verb-Statement");
	CChar * pCharSrc = pSrc->GetChar();
	CClient * pClientSrc = (pCharSrc && pCharSrc->IsClientActive()) ? (pCharSrc->GetClientActive()) : nullptr ;

	switch (index)
	{
        case OV_ADDCLILOC:
            // Add cliloc in @ClientTooltip trigger
            {
                tchar * ppLocArgs[256];
                const int qty = Str_ParseCmds(s.GetArgRaw(), ppLocArgs, ARRAY_COUNT(ppLocArgs), ",");
				const dword clilocid = Exp_GetDWVal(ppLocArgs[0]);

                CSString sLocArgs;
                for (int y = 1 ; y < qty; ++y )
                {
                    if ( sLocArgs.GetLength() )
                        sLocArgs += "\t";

                    if ((*ppLocArgs[y] == '\0') || !strncmp(ppLocArgs[y], "NULL", 4))
                        sLocArgs += " ";
                    else
                        sLocArgs += ppLocArgs[y];
                }
                m_TooltipData.emplace_back(std::make_unique<CClientTooltip>(clilocid, sLocArgs.GetBuffer()));
            }
            break;
        case OV_REMOVECLILOC:
        	// Remove cliloc in @ClientTooltip trigger
        	{
                tchar * ppLocArgs[256];
                Str_ParseCmds(s.GetArgRaw(), ppLocArgs, ARRAY_COUNT(ppLocArgs), ",");
				const dword clilocid = Exp_GetDWVal(ppLocArgs[0]);
        		for (size_t i = 0; i < m_TooltipData.size(); ++i)
        		{
        			CClientTooltip* ct = m_TooltipData[i].get();
        			if (ct->m_clilocid == clilocid)
        			{
        				m_TooltipData.erase(m_TooltipData.begin() + i);
        				//I did a break, but if same tooltip added as script and default together there could be more than one for same cliloc so we need to push to check if there is another.
        				//break;
					}
				}
			}
			break;
		case OV_REPLACECLILOC:
			// Replace cliloc in @ClientTooltip trigger
			{
				tchar * ppLocArgs[256];
				const int qty = Str_ParseCmds(s.GetArgRaw(), ppLocArgs, ARRAY_COUNT(ppLocArgs), ",");
				const dword clilocid = Exp_GetDWVal(ppLocArgs[0]);

                CSString sLocArgs;
                for (int y = 1 ; y < qty; ++y )
                {
                    if ( sLocArgs.GetLength() )
                        sLocArgs += "\t";

                    if ((*ppLocArgs[y] == '\0') || !strncmp(ppLocArgs[y], "NULL", 4))
                        sLocArgs += " ";
                    else
                        sLocArgs += ppLocArgs[y];
                }

                for (size_t i = 0; i < m_TooltipData.size(); ++i)
                {
                	CClientTooltip* ct = m_TooltipData[i].get();
                	if (ct->m_clilocid == clilocid)
                	{
        	    	    m_TooltipData.erase(m_TooltipData.begin() + i);
        	    	    m_TooltipData.emplace(m_TooltipData.begin() + i, std::make_unique<CClientTooltip>(clilocid, sLocArgs.GetBuffer()));
						break;
					}
				}
			}
			break;
		case OV_DAMAGE:	//	"Dmg, SourceFlags, SourceCharUid, DmgPhysical(%), DmgFire(%), DmgCold(%), DmgPoison(%), DmgEnergy(%)" = do me some damage.
			{
				EXC_SET_BLOCK("DAMAGE");
				int64 piCmd[8];
				int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, ARRAY_COUNT(piCmd));
				if ( iArgQty < 1 )
					return false;
				if ( iArgQty > 2 )	// Give it a new source char UID
				{
					CObjBaseTemplate * pObj = CUID::ObjFindFromUID( (dword)(piCmd[2]) );
					if ( pObj )
						pObj = pObj->GetTopLevelObj();
					pCharSrc = dynamic_cast<CChar*>(pObj);
				}

				CChar *pChar = dynamic_cast<CChar *>(this);
				CItem *pItem = dynamic_cast<CItem *>(this);
				if (pChar)
				{
					pChar->OnTakeDamage((int)(piCmd[0]),
						pCharSrc,
						(iArgQty >= 1) ? (DAMAGE_TYPE)(piCmd[1]) : DAMAGE_HIT_BLUNT | DAMAGE_GENERAL,
						(iArgQty >= 3) ? (int)(piCmd[3]) : 0,	// physical damage %
						(iArgQty >= 4) ? (int)(piCmd[4]) : 0,	// fire damage %
						(iArgQty >= 5) ? (int)(piCmd[5]) : 0,	// cold damage %
						(iArgQty >= 6) ? (int)(piCmd[6]) : 0,	// poison damage %
						(iArgQty >= 7) ? (int)(piCmd[7]) : 0	// energy damage %
					);
				}
				else if (pItem)
				{
					pItem->OnTakeDamage((int)(piCmd[0]),
						pCharSrc,
						(iArgQty >= 1) ? (DAMAGE_TYPE)(piCmd[1]) : DAMAGE_HIT_BLUNT | DAMAGE_GENERAL
					);
				}
			}
			break;

		case OV_EDIT:
			{
				EXC_SET_BLOCK("EDIT");
				// Put up a list of items in the container. (if it is a container)
				if ( pClientSrc == nullptr )
					return false;
				pClientSrc->m_Targ_Text = s.GetArgStr();
				pClientSrc->Cmd_EditItem( this, -1 );
			}
			break;
		case OV_EFFECT: // some visual effect.
			{
				EXC_SET_BLOCK("EFFECT");
				int64 piCmd[12];
				int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, ARRAY_COUNT(piCmd) );
				if ( iArgQty < 2 )
					return false;
				const CObjBase * pThis = this;
				//DEBUG_ERR(("this->GetUID() 0%x \n", (dword)this->GetUID()));
				if ( piCmd[0] == -1 )
				{
					if ( pCharSrc )
					{
						piCmd[0]	= EFFECT_BOLT;
						pThis		= pCharSrc;
						pCharSrc	= dynamic_cast <CChar*>(this);
					}
				}
				//DEBUG_ERR(("this->GetUID() 0%x pThis->GetUID() 0%x pCharSrc->GetUID() 0%x\n",(dword)this->GetUID(),(dword)pThis->GetUID(),(dword)pCharSrc->GetUID()));
				pThis->Effect( (EFFECT_TYPE)(piCmd[0]), (ITEMID_TYPE)(ResGetIndex((dword)piCmd[1]) ),
					pCharSrc,
					(iArgQty >= 3)? (uchar)(piCmd[2]) : 5,		// byte bSpeedSeconds = 5,
					(iArgQty >= 4)? (uchar)(piCmd[3]) : 1,		// byte bLoop = 1,
					(iArgQty >= 5)? (piCmd[4] != 0) : false,	// bool fExplode = false
					(iArgQty >= 6)? (uint)(piCmd[5]) : 0,		// hue
					(iArgQty >= 7)? (uint)(piCmd[6]) : 0,		// render mode,
					(iArgQty >= 8) ? (word)(piCmd[7]) : 0,		// EffectID	//New Packet 0xc7
					(iArgQty >= 9) ? (word)(piCmd[8]) : 0,		// ExplodeID
					(iArgQty >= 10) ? (word)(piCmd[9]) : 0,		// ExplodeSound
					(iArgQty >= 11) ? (dword)(piCmd[10]) : 0,	// EffectUID
					(iArgQty >= 12) ? (uchar)(piCmd[11]) : 0	// Type
					);
			}
			break;
		case OV_EFFECTLOCATION: // visual effect at map point.
		{
			EXC_SET_BLOCK("EFFECTLOCATION");
			int64 piCmd[15];
			int iArgQty = Str_ParseCmds(s.GetArgStr(), piCmd, ARRAY_COUNT(piCmd));
			if (iArgQty < 5)
				return false;
			const CObjBase *pThis = this;

            const CPointMap ptDest((short)piCmd[0], (short)piCmd[1], (char)piCmd[2]);
			if (!ptDest.IsValidPoint())
				return false;

			//DEBUG_ERR(("this->GetUID() 0%x \n", (dword)this->GetUID()));
			if (piCmd[3] == -1)
			{
				if (pCharSrc)
				{
					piCmd[3] = EFFECT_BOLT;
					pThis = pCharSrc;
					pCharSrc = dynamic_cast <CChar*>(this);
				}

			}
			//DEBUG_ERR(("this->GetUID() 0%x pThis->GetUID() 0%x pCharSrc->GetUID() 0%x\n",(dword)this->GetUID(),(dword)pThis->GetUID(),(dword)pCharSrc->GetUID()));
			pThis->EffectLocation((EFFECT_TYPE)(piCmd[3]), (ITEMID_TYPE)(ResGetIndex((dword)piCmd[4])),
                pCharSrc ? &pCharSrc->GetTopPoint() : nullptr,
                &ptDest,
				(iArgQty >= 3) ? (uchar)(piCmd[5]) : 5,		// byte bSpeedSeconds = 5,
				(iArgQty >= 4) ? (uchar)(piCmd[6]) : 1,		// byte bLoop = 1,
				(iArgQty >= 5) ? (piCmd[7] != 0) : false,	// bool fExplode = false
				(iArgQty >= 6) ? (uint)(piCmd[8]) : 0,		// hue
				(iArgQty >= 7) ? (uint)(piCmd[9]) : 0,		// render mode,
				(iArgQty >= 8) ? (word)(piCmd[10]) : 0,		// EffectID	//New Packet 0xc7
				(iArgQty >= 9) ? (word)(piCmd[11]) : 0,		// ExplodeID
				(iArgQty >= 10) ? (word)(piCmd[12]) : 0,	// ExplodeSound
				(iArgQty >= 11) ? (dword)(piCmd[13]) : 0,	// EffectUID
				(iArgQty >= 12) ? (uchar)(piCmd[14]) : 0	// Type
			);
		}
		break;
		case OV_EMOTE:
			EXC_SET_BLOCK("EMOTE");
			Emote( s.GetArgStr() );
			break;
		case OV_FLIP:
			EXC_SET_BLOCK("FLIP");
			Flip();
			break;
		case OV_GOAWAKE:
			_GoAwake();
			break;
		case OV_GOSLEEP:
			_GoSleep();
			break;
		case OV_INPDLG:
			// "INPDLG" verb maxchars
			// else assume it was a property button.
			{
				EXC_SET_BLOCK("INPDLG");
				if ( pClientSrc == nullptr )
					return false;

				tchar *Arg_ppCmd[2];		// Maximum parameters in one line
				int iQty = Str_ParseCmds( s.GetArgStr(), Arg_ppCmd, ARRAY_COUNT( Arg_ppCmd ));

				CSString sOrgValue;
				if ( ! r_WriteVal( Arg_ppCmd[0], sOrgValue, pSrc ))
					sOrgValue = ".";

				pClientSrc->m_Targ_Text = Arg_ppCmd[0];	// The attribute we want to edit.

				int iMaxLength = iQty > 1 ? atoi(Arg_ppCmd[1]) : 1;

				CSString sPrompt;
				sPrompt.Format("%s (# = default)", (lpctstr)(Arg_ppCmd[0]));
				pClientSrc->addGumpInpVal( true, INPVAL_STYLE_TEXTEDIT,
					iMaxLength,	sPrompt, sOrgValue, this );
			}
			break;

		case OV_MENU:
			{
				EXC_SET_BLOCK("MENU");
				if ( pClientSrc == nullptr )
					return false;
				pClientSrc->Menu_Setup( g_Cfg.ResourceGetIDType( RES_MENU, s.GetArgStr()), this );
			}
			break;
		case OV_MESSAGE:	//put info message (for pSrc client only) over item.
		case OV_MSG:
			{
				EXC_SET_BLOCK("MESSAGE or MSG");
				lpctstr ptcArg = s.GetArgStr();
				if ( pCharSrc == nullptr )
					UpdateObjMessage(ptcArg, ptcArg, nullptr, HUE_TEXT_DEF, TALKMODE_ITEM);
				else
					pCharSrc->ObjMessage(ptcArg, this);
			}
			break;
		case OV_MESSAGEUA:
			{
				EXC_SET_BLOCK("MESSAGEUA");
				if ( pClientSrc == nullptr )
					break;	// We show the message only to players

				tchar * pszArgs[5];
				nachar ncBuffer[ MAX_TALK_BUFFER ];

				int iArgQty = Str_ParseCmds( s.GetArgRaw(), pszArgs, ARRAY_COUNT(pszArgs) );
				if ( iArgQty < 5 )
					break;

				CvtSystemToNETUTF16( ncBuffer, ARRAY_COUNT( ncBuffer ), pszArgs[4], -1 );
				pClientSrc->addBarkUNICODE( ncBuffer, this,
					(HUE_TYPE)( pszArgs[0][0] ? Exp_GetVal(pszArgs[0]) : HUE_TEXT_DEF ),
					(TALKMODE_TYPE)( pszArgs[1][0] ? Exp_GetVal(pszArgs[1]) : TALKMODE_SAY ),
					(FONT_TYPE)( pszArgs[2][0] ? Exp_GetVal(pszArgs[2]) : FONT_NORMAL ),
					CLanguageID(pszArgs[3]));
				break;
			}
		case OV_MOVE:
			// move without restriction. east,west,etc. (?up,down,)
			EXC_SET_BLOCK("MOVE");
			if ( IsTopLevel())
			{
				CPointMap pt = GetTopPoint();
				if ( ! GetDeltaStr( pt, s.GetArgStr()))
					return false;
                RemoveFromView();
				MoveTo( pt );
				Update();
			}
			break;
		case OV_MOVENEAR:
			{
				EXC_SET_BLOCK("MOVENEAR");
				CObjBase *pObjNear;
				int64 piCmd[4];

				int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, ARRAY_COUNT(piCmd) );
				if ( iArgQty <= 0 )
					return false;
				if ( iArgQty < 3 )
					piCmd[2] = 1;
				if ( iArgQty < 2 )
					piCmd[1] = 1;

				pObjNear = CUID::ObjFindFromUID((dword)piCmd[0]);
				if ( !pObjNear )
					return false;
                if ( piCmd[2] )
                    RemoveFromView();
				MoveNearObj( pObjNear, (word)(piCmd[1]) );
				if ( piCmd[2] )
					Update();
			}
			break;
		case OV_NUDGEDOWN:
			EXC_SET_BLOCK("NUDGEDOWN");
			if ( IsTopLevel())
			{
				char zdiff = s.GetArgCVal();
				SetTopZ( GetTopZ() - ( zdiff ? zdiff : 1 ));
				Update();
			}
			break;
		case OV_NUDGEUP:
			EXC_SET_BLOCK("NUDGEUP");
			if ( IsTopLevel())
			{
				char zdiff = s.GetArgCVal();
				SetTopZ( GetTopZ() + ( zdiff ? zdiff : 1 ));
				Update();
			}
			break;
		case OV_MOVETO:
		case OV_P:
        {
			EXC_SET_BLOCK("P or MOVETO");
			// The "@Click" trigger str should be the same between items and chars...
			if (0 == _sRunningTrigger.compare(CChar::sm_szTrigName[CTRIG_Click]))
			{
				g_Log.EventError("Can't set P in the current trigger. It would cause an infinite loop.\n");
				return false;
			}
            const CPointMap pt( g_Cfg.GetRegionPoint(s.GetArgStr()) );
            if (pt.IsValidPoint())
            {
                RemoveFromView();
                MoveTo(pt);
                Update();
            }
            else
                return false;
        }
			break;
		case OV_PROMPTCONSOLE:
		case OV_PROMPTCONSOLEU:
			{
				EXC_SET_BLOCK("PROMPTCONSOLE/U");
				if ( pClientSrc == nullptr )
					return false;

				tchar * pszArgs[2];

				int iArgQty = Str_ParseCmds( s.GetArgRaw(), pszArgs, ARRAY_COUNT(pszArgs) );
				if ( iArgQty == 0 )
					break;

				pClientSrc->addPromptConsoleFunction( pszArgs[0], pszArgs[1], (index == OV_PROMPTCONSOLEU) );
			}
			break;
		case OV_INFO:
			EXC_SET_BLOCK("INFO");
			if ( ! pClientSrc )
				return false;
			return pClientSrc->addGumpDialogProps( GetUID() );
		case OV_REMOVE:	//remove this object now.
			EXC_SET_BLOCK("REMOVE");
			Delete();
            break;
		case OV_REMOVEFROMVIEW:
			EXC_SET_BLOCK("REMOVEFROMVIEW");
			RemoveFromView( nullptr, false );	// remove this item from all clients.
            break;
		case OV_RESENDTOOLTIP:
			{
				EXC_SET_BLOCK("RESENDTOOLTIP");

				int64 piCmd[2];
				int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, ARRAY_COUNT(piCmd) );

				bool fSendFull = false;
				bool fUseCache = false;

				if (iArgQty >= 1)
					fSendFull = (piCmd[0] != 0);
				if (iArgQty >= 2)
					fUseCache = (piCmd[1] != 0);

				ResendTooltip(fSendFull, fUseCache);
                break;
			}
		case OV_SAY: //speak so everyone can here
			EXC_SET_BLOCK("SAY");
			Speak( s.GetArgStr() );
			break;

		case OV_SAYU:
			// Speak in unicode from the UTF8 system format.
			EXC_SET_BLOCK("SAYU");
			SpeakUTF8( s.GetArgStr(), HUE_TEXT_DEF, TALKMODE_SAY, FONT_NORMAL );
			break;

		case OV_SAYUA:
			// This can have full args. SAYUA Color, Mode, Font, Lang, Text Text
			{
				EXC_SET_BLOCK("SAYUA");
				tchar * pszArgs[5];
				int iArgQty = Str_ParseCmds( s.GetArgRaw(), pszArgs, ARRAY_COUNT(pszArgs) );
				if ( iArgQty < 5 )
					break;

				SpeakUTF8( pszArgs[4],
					(HUE_TYPE)( pszArgs[0][0] ? Exp_GetVal(pszArgs[0]) : HUE_TEXT_DEF ),
					(TALKMODE_TYPE)( pszArgs[1][0] ? Exp_GetVal(pszArgs[1]) : TALKMODE_SAY ),
					(FONT_TYPE)( pszArgs[2][0] ? Exp_GetVal(pszArgs[2]) : FONT_NORMAL ),
					CLanguageID(pszArgs[3]));
			}
			break;

		case OV_SOUND:
			{
				EXC_SET_BLOCK("SOUND");
				int64 piCmd[2];
				int iArgQty = Str_ParseCmds( s.GetArgStr(), piCmd, ARRAY_COUNT(piCmd));
				if (!iArgQty)
					return false;
				Sound( (SOUND_TYPE)(piCmd[0]), ( iArgQty > 1 ) ? (int)(piCmd[1]) : 1 );
			}
			break;
		case OV_SPELLEFFECT:	// spell, strength, noresist
			{
				EXC_SET_BLOCK("SPELLEFFECT");
				int64 piCmd[4];
				size_t iArgs = Str_ParseCmds( s.GetArgStr(), piCmd, ARRAY_COUNT(piCmd));
				CItem * pItemSrc = nullptr;
				switch( iArgs )
				{
				case 4:
					pItemSrc = CUID::ItemFindFromUID((dword)piCmd[3]);
					FALLTHROUGH;
				case 3:
					if ( piCmd[2] == -1 )
					{
						pCharSrc = dynamic_cast <CChar*> (this);
					}
					else
					{
						pCharSrc = CUID::CharFindFromUID((dword)piCmd[2]);
					}
					break;
				default:
					break;
				}
				OnSpellEffect((SPELL_TYPE)(ResGetIndex((dword)piCmd[0])), pCharSrc, (int)(piCmd[1]), pItemSrc);
			}
			break;

        case OV_TAGLIST:
        {
            EXC_SET_BLOCK("TAGLIST");
            if (! strcmpi(s.GetArgStr(), "log"))
                pSrc = &g_Serv;
            m_TagDefs.DumpKeys(pSrc, "TAG.");
        }break;
        case OV_CLILOCLIST:
        {
        	EXC_SET_BLOCK("CLILOCLIST");
            if (! strcmpi(s.GetArgStr(), "log"))
                pSrc = &g_Serv;

            if (!pSrc)
            	break;

            for (size_t i = 0; i < this->m_TooltipData.size(); i++)
            {
				CClientTooltip* ct = this->m_TooltipData[i].get();

				/*
				//Parse tchar into string.
                tchar *ppLocArgs = ct->m_args;
                CSString sLocArgs;
                for (int y = 1 ; y < qty; ++y )
                {
                    if ( sLocArgs.GetLength() )
                        sLocArgs += "\t";

                    if ((*ppLocArgs[y] == '\0') || !strncmp(ppLocArgs[y], "NULL", 4))
                        sLocArgs += " ";
                    else
                        sLocArgs += ppLocArgs[y];
                }
                */

				if (pSrc->GetChar())
					pSrc->SysMessagef("%d=%s", ct->m_clilocid, ct->m_args);
				else
					g_Log.Event(LOGL_EVENT, "%d=%s\n", ct->m_clilocid, ct->m_args);
			}
		}break;
        case OV_BASETAGLIST:
        {
            EXC_SET_BLOCK("BASETAGLIST");
            if (! strcmpi(s.GetArgStr(), "log"))
                pSrc = &g_Serv;
            Base_GetDef()->m_TagDefs.DumpKeys(pSrc, "[Base]TAG.");
        }break;

        case OV_PROPLIST:
        {
            EXC_SET_BLOCK("PROPLIST");
            if (! strcmpi(s.GetArgStr(), "log"))
                pSrc = &g_Serv;
            m_BaseDefs.DumpKeys(pSrc, nullptr);
            DumpComponentProps(pSrc);
        }break;
        case OV_BASEPROPLIST:
        {
            EXC_SET_BLOCK("BASEPROPLIST");
            if (! strcmpi(s.GetArgStr(), "log"))
                pSrc = &g_Serv;
            const CBaseBaseDef* pBase = Base_GetDef();
            pBase->m_BaseDefs.DumpKeys(pSrc, nullptr);
            pBase->DumpComponentProps(pSrc, "[Base]");
        }break;

		case OV_TARGET:
			{
				EXC_SET_BLOCK("TARGET");
				if ( !pClientSrc )
					return false;
				ptcKey	+= 6;
				bool fAllowGround = false;
				bool fCheckCrime = false;
				bool fFunction = false;
				bool fMulti	= false;
				tchar low = (tchar)(tolower(*ptcKey));

				while (( low >= 'a' ) && ( low <= 'z' ))
				{
					if ( low == 'f' )
						fFunction = true;
					else if ( low == 'g' )
						fAllowGround = true;
					else if ( low == 'w' )
						fCheckCrime = true;
					else if ( low == 'm' )
						fMulti = true;

					low = (tchar)(tolower(*(++ptcKey)));
				}

				pClientSrc->m_Targ_UID = GetUID();
				pClientSrc->m_tmUseItem.m_pParent = GetParent();	// Cheat Verify

				if ( fFunction )
				{
					if ( fMulti )
					{
						if ( IsStrEmpty(s.GetArgStr()) )
							break;
						char * ppArg[3];
						Str_ParseCmds( s.GetArgStr(), ppArg, ARRAY_COUNT(ppArg), "," );
						if ( !IsStrNumeric( ppArg[1] ))
							DEBUG_ERR(("Invalid argument in Target Multi\n"));
						ITEMID_TYPE itemid = (ITEMID_TYPE)(Exp_GetVal(ppArg[1]));
                        HUE_TYPE color = (HUE_TYPE)(Exp_GetVal(ppArg[2]));
						pClientSrc->addTargetFunctionMulti( ppArg[0], itemid, color, fAllowGround );
					}
					else
						pClientSrc->addTargetFunction( s.GetArgStr(), fAllowGround, fCheckCrime );
				}
				else
				{
					if ( fMulti )
					{
                        char *ppArg[2];
                        Str_ParseCmds(s.GetArgStr(), ppArg, ARRAY_COUNT(ppArg), ",");
						if ( !IsStrNumeric(ppArg[0]))
							DEBUG_ERR(("Invalid argument in Target Multi\n"));
						ITEMID_TYPE itemid = (ITEMID_TYPE)(Exp_GetVal(ppArg[0]));
                        HUE_TYPE color = (HUE_TYPE)(Exp_GetVal(ppArg[1]));
						pClientSrc->addTargetItems( CLIMODE_TARG_USE_ITEM, itemid, color, fAllowGround );
					}
					else
						pClientSrc->addTarget( CLIMODE_TARG_USE_ITEM, s.GetArgStr(), fAllowGround, fCheckCrime );
				}
			}
			break;

		case OV_TIMERF:
		case OV_TIMERFMS:
			{
				EXC_SET_BLOCK("TIMERF(MS)");
				const bool fSeconds = (index == OV_TIMERF);
				lpctstr ptcArgStr = s.GetArgStr();

				if ( !strnicmp(ptcArgStr, "CLEAR", 5 ) )
				{
					CWorldTimedFunctions::ClearUID(GetUID());
				}
				else if ( !strnicmp(ptcArgStr, "STOP", 4 ) )
				{
					lpctstr strFunction = ptcArgStr + 5;
					GETNONWHITESPACE(strFunction);
					CWorldTimedFunctions::Stop(GetUID(), strFunction);
				}
				else
				{
					tchar *ptcCmd = s.GetArgRaw();

					// Extract the timeout and advance the string pointer to the command position
					int64 iTimeout = Exp_Get64Val(ptcCmd);

					if (iTimeout < 0)
					{
						g_Log.EventError("Timed function: invalid parameter '%" PRId64 "'.\n", iTimeout);
						return false;
					}
					else
					{
						SKIP_ARGSEP(ptcCmd);
						if ( !(*ptcCmd) || (strlen(ptcCmd) >= CTimedFunction::kuiCommandSize) )
						{
							g_Log.EventError("TimerF function name empty or args too long - total length must be less than %u characters.\n", CTimedFunction::kuiCommandSize);
							return false;
						}
						else
						{
							if (fSeconds)
							{
								iTimeout *= MSECS_PER_SEC;
							}
							CWorldTimedFunctions::Add(GetUID(), iTimeout, ptcCmd);
						}
					}
				}
			}
			break;
		case OV_TRIGGER:
			{
				if ( s.HasArgs() )
				{
					TRIGRET_TYPE tResult;
					_iCallingObjTriggerId = _iRunningTriggerId;
					CallPersonalTrigger(s.GetArgRaw(), pSrc, tResult);
					_iCallingObjTriggerId = -1;
				}
			} break;
		case OV_DIALOG:
		case OV_SDIALOG:
			{
				EXC_SET_BLOCK("DIALOG or SDIALOG");
				if ( pClientSrc == nullptr )
					return false;

				tchar *	Arg_ppCmd[3];		// Maximum parameters in one line
				size_t iQty = Str_ParseCmds( s.GetArgStr(), Arg_ppCmd, ARRAY_COUNT( Arg_ppCmd ));
				if ( iQty < 1 )
					return false;

				if ( index == OV_SDIALOG )
				{
					CResourceID rid = g_Cfg.ResourceGetIDType( RES_DIALOG, Arg_ppCmd[0] );
					uint context;
					if ( pClientSrc->GetNetState()->isClientKR() )
					{
						context = g_Cfg.GetKRDialog(rid.GetPrivateUID());
					}
					else
					{
						context = rid.GetPrivateUID();
					}

					CClient::OpenedGumpsMap_t::const_iterator itGumpFound = pClientSrc->m_mapOpenedGumps.find( context );
					if ( pCharSrc && (( itGumpFound != pClientSrc->m_mapOpenedGumps.end() ) && ( (*itGumpFound).second > 0 )) )
						break;
				}
				pClientSrc->Dialog_Setup( CLIMODE_DIALOG, g_Cfg.ResourceGetIDType( RES_DIALOG, Arg_ppCmd[0] ),
					iQty > 1 ? Exp_GetVal( Arg_ppCmd[1]) : 0, this, Arg_ppCmd[2] );
			}
			break;
		case OV_DIALOGCLOSE:
			{
				EXC_SET_BLOCK("DIALOGCLOSE");
				if ( pClientSrc == nullptr )
					return false;

				tchar *	Arg_ppCmd[2];		// Maximum parameters in one line
				size_t iQty = Str_ParseCmds( s.GetArgStr(), Arg_ppCmd, ARRAY_COUNT( Arg_ppCmd ));
				if ( iQty < 1 )
					return false;

				dword dwRid = g_Cfg.ResourceGetIDType( RES_DIALOG, Arg_ppCmd[0] ).GetPrivateUID();
				if ( pClientSrc->GetNetState()->isClientKR() )
                    dwRid = g_Cfg.GetKRDialog( dwRid );
				pClientSrc->Dialog_Close( pClientSrc->GetChar(), dwRid, iQty > 1 ? Exp_GetVal( Arg_ppCmd[1]) : 0 );
			}
			break;
		case OV_TRYP:
			{
				EXC_SET_BLOCK("TRYP");
				int iMinPriv = s.GetArgVal();

				if ( iMinPriv >= PLEVEL_QTY )
				{
					pSrc->SysMessagef("The %s property can't be changed.", s.GetArgStr());
					return false;
				}

				if ( pSrc->GetPrivLevel() < iMinPriv )
				{
					pSrc->SysMessagef( "You lack the privilege to change the %s property.", s.GetArgStr());
					return false;
				}

				// do this verb only if we can touch it.
				if ( pSrc->GetPrivLevel() <= PLEVEL_Counsel )
				{
					if ( pCharSrc == nullptr || !pCharSrc->CanTouch(this) )
					{
						pSrc->SysMessagef("Can't touch %s object %s", s.GetArgStr(), GetName());
						return false;
					}
				}
			}
			// no break here, TRYP only does extra checks
			FALLTHROUGH;
		case OV_TRY:
			{
				EXC_SET_BLOCK("TRY or TRYP");
				lpctstr pszVerb = s.GetArgStr();
				CScript script(pszVerb);
				script.CopyParseState(s);
				if (index == OV_TRY)
					script._eParseFlags = CScript::ParseFlags::IgnoreInvalidRef;

				if ( !r_Verb(script, pSrc) )
				{
					if (script._eParseFlags == CScript::ParseFlags::IgnoreInvalidRef)
						return true;

					DEBUG_ERR(( "Can't try %s object %s (0%x)\n", pszVerb, GetName(), (dword)(GetUID())));
					return false;
				}
			}
            break;
		case OV_TRYSRC:
		case OV_TRYSRV:
			{
				EXC_SET_BLOCK("TRYSRC or TRYSRV");
				CUID NewSrc;
				CTextConsole * pNewSrc = nullptr;

				if ( index == OV_TRYSRC )
				{
					NewSrc.SetObjUID(s.GetArgDWVal());
					if ( NewSrc.IsValidUID() )
						pNewSrc = NewSrc.CharFind();
				}
				else
					pNewSrc = &g_Serv;

				lpctstr pszVerb = s.GetArgStr();
				if ( pNewSrc == nullptr )
				{
					if ( index == OV_TRYSRC )
						DEBUG_ERR(( "Can't trysrc %s object %s (0%x): invalid src uid 0%x\n", pszVerb, GetName(), (dword)(GetUID()), (dword)(NewSrc) ));
					else
						DEBUG_ERR(( "Can't trysrv %s object %s (0%x)\n", pszVerb, GetName(), (dword)(GetUID()) ));

					return false;
				}
				CScript script(pszVerb);
				script.CopyParseState(s);
				if (!r_Verb(script, pNewSrc))
				{
					if ( index == OV_TRYSRC )
						DEBUG_ERR(( "Can't trysrc %s object %s (0%x) with src %s (0%x)\n", pszVerb, GetName(), (dword)(GetUID()), pNewSrc->GetName(), (dword)(NewSrc) ));
					else
						DEBUG_ERR(( "Can't trysrv %s object %s (0%x)\n", pszVerb, GetName(), (dword)(GetUID()) ));

					return false;
				}
			}
            break;

		case OV_UID:
			EXC_SET_BLOCK("UID");
			// block anyone from ever doing this.
			if ( pSrc )
			{
				pSrc->SysMessage( "Setting the UID this way is not allowed" );
			}
			return false;

		case OV_UPDATE:
			EXC_SET_BLOCK("UPDATE");
			Update();
			break;
		case OV_UPDATEX:
			EXC_SET_BLOCK("UPDATEX");
			// Some things like equipped items need to be removed before they can be updated !
			RemoveFromView();
			Update();
			break;

		case OV_CLICK:
			EXC_SET_BLOCK("CLICK");

			if (!pCharSrc || !pCharSrc->IsClientActive())
				return false;

			if (s.HasArgs())
			{
				if (!IsChar())
					return false;

				const CUID uid(s.GetArgUVal());
				if (!uid.ObjFind())
					return false;
				pCharSrc->GetClientActive()->Event_SingleClick(uid);
			}
			else
				pCharSrc->GetClientActive()->Event_SingleClick(this->GetUID());
            break;

		case OV_DCLICK:
			EXC_SET_BLOCK("DCLICK");
			if (!pCharSrc)
				return false;
			if (s.HasArgs())
			{
				if (!IsChar())
					return false;

				CObjBase* pObj = CUID::ObjFindFromUID(s.GetArgDWVal());
				if (!pObj)
					return false;

				CChar* pChar = static_cast <CChar*> (this);
				return pChar->Use_Obj(pObj, true, true);
			}
			else
				return pCharSrc->Use_Obj(this, true, true);

		case OV_USEITEM:
			EXC_SET_BLOCK("USEITEM");
			if ( ! pCharSrc )
				return false;
			if ( s.HasArgs() )
			{
				if (!IsChar())
					return false;

				CObjBase* pObj = CUID::ObjFindFromUID(s.GetArgDWVal());
				if (!pObj)
					return false;

				CChar *pChar = static_cast <CChar *> (this);
				return pChar->Use_Obj( pObj, false, true );
			}
			else
				return pCharSrc->Use_Obj( this, false, true );

		case OV_FIX:
			s.GetArgStr()[0] = '\0';
			FALLTHROUGH;
		case OV_Z:	//	ussually in "SETZ" form
			EXC_SET_BLOCK("FIX or Z");
			if ( IsItemEquipped())
				return false;
			if ( s.HasArgs())
			{
				SetUnkZ( (char)(s.GetArgVal()));
			}
			else if ( IsTopLevel())
			{
				if (CChar* pChar = dynamic_cast <CChar*>(this))
					SetTopZ(pChar->GetFixZ(GetTopPoint()));
				else if (CItem* pItem = dynamic_cast <CItem*>(this))
					SetTopZ(pItem->GetFixZ(GetTopPoint()));
				else
				{
					g_Log.EventDebug("Failed to get reference in FIX or Z\n");
					break;
				}
			}
			Update();
			break;

		default:
			return false;
	}
	return true;
	EXC_CATCH;

	EXC_DEBUG_START;
	EXC_ADD_SCRIPTSRC;
	EXC_DEBUG_END;
	return false;
}

void CObjBase::RemoveFromView( CClient * pClientExclude, bool fHardcoded )
{
	ADDTOCALLSTACK("CObjBase::RemoveFromView");
	// Remove this item from all clients.
	// In a destructor this can do funny things.

	if ( IsDisconnected())
		return;	// not in the world.

	CObjBaseTemplate * pObjTop = GetTopLevelObj();
	CItem * pItem = fHardcoded ? (dynamic_cast<CItem*>(this)) : (nullptr);
	CChar * pChar = nullptr;

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if ( pClientExclude == pClient )
			continue;
		pChar = pClient->GetChar();
		if ( pChar == nullptr )
			continue;
		if ( pChar->GetTopDistSight( pObjTop ) > pChar->GetVisualRange() )
			//Client does not support removing of items which are farther (will be removed from the radar on the next step, because the server won't resend it)
			continue;
		if ( pItem && pItem->IsItemEquipped() )
		{
			const LAYER_TYPE iItemLayer = pItem->GetEquipLayer();
			if ((iItemLayer > LAYER_HORSE) && (iItemLayer != LAYER_BANKBOX) && (iItemLayer != LAYER_DRAGGING))
				continue;
		}

		if (GetEquipLayer() == LAYER_BANKBOX)
			pClient->closeContainer(this);

		pClient->addObjectRemove( this );
	}
}

void CObjBase::ResendOnEquip( bool fAllClients )
{
	ADDTOCALLSTACK("CObjBase::RemoveFromView");
	// Remove this item from all clients if fAllClients == true, from Enhanced Client only if not.
	// Then resend it.

	if ( IsDisconnected())
		return;	// not in the world.

	CObjBaseTemplate * pObjTop = GetTopLevelObj();
	CItem * pItem = dynamic_cast<CItem*>(this);
	CChar * pChar = nullptr;

	ClientIterator it;
	for (CClient* pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
		if ( fAllClients == false && !pClient->GetNetState()->isClientEnhanced() )
			continue;
		pChar = pClient->GetChar();
		if ( pChar == nullptr )
			continue;
		if ( pChar->GetTopDistSight( pObjTop ) > pChar->GetVisualRange() )
			continue;	//Client does not support removing of items which are farther (will be removed from the radar on the next step, because the server won't resend it)
		if ( pItem )
		{
			if (( pItem->IsItemEquipped() ) && ( !pChar->IsPriv(PRIV_GM) ))
			{
				const LAYER_TYPE iItemLayer = pItem->GetEquipLayer();
				if ((iItemLayer > LAYER_HORSE) && (iItemLayer != LAYER_BANKBOX) && (iItemLayer != LAYER_DRAGGING))
					continue;
			}

			if ( pItem->IsTypeSpellbook() && pItem->IsItemInContainer())
				continue;	// items must be removed from view before equipping in EC when on the floor, however spellbooks cannot be removed from view or client will crash
		}

		if (GetEquipLayer() == LAYER_BANKBOX)
			pClient->closeContainer(this);

		pClient->addObjectRemove( this );
		pClient->addItem( pItem );
	}
}

void CObjBase::SetPropertyList(PacketPropertyList* propertyList)
{
	ADDTOCALLSTACK("CObjBase::SetPropertyList");
	// set the property list for this object

	if (propertyList == GetPropertyList())
		return;

	FreePropertyList();
	m_PropertyList = propertyList;
}

void CObjBase::FreePropertyList()
{
	ADDTOCALLSTACK("CObjBase::FreePropertyList");
	// free m_PropertyList

	if (m_PropertyList == nullptr)
		return;

	delete m_PropertyList;
	m_PropertyList = nullptr;
}

dword CObjBase::UpdatePropertyRevision(dword hash)
{
	ADDTOCALLSTACK("CObjBase::UpdatePropertyRevision");

	if (hash != m_PropertyHash)
	{
		// the property list has changed, increment the revision number
		m_PropertyHash = hash;
		++m_PropertyRevision;
	}

	return m_PropertyRevision;
}

void CObjBase::UpdatePropertyFlag()
{
	ADDTOCALLSTACK("CObjBase::UpdatePropertyFlag");
	if (!(g_Cfg.m_iFeatureAOS & FEATURE_AOS_UPDATE_B) || g_Serv.IsLoading())
		return;

	m_fStatusUpdate |= SU_UPDATE_TOOLTIP;

	// Items equipped, inside containers or with timer expired doesn't receive ticks and need to be added to a list of items to be processed separately
	if (!IsTopLevel() || _IsTimerExpired())
	{
		CWorldTickingList::AddObjStatusUpdate(this, false);
	}
}

dword CObjBase::GetPropertyHash() const
{
	return m_PropertyHash;
}

void CObjBase::OnTickStatusUpdate()
{
    // process m_fStatusUpdate flags

	//ADDTOCALLSTACK("CObjBase::OnTickStatusUpdate"); // Called very frequently.
    EXC_TRY("CObjBase::OnTickStatusUpdate");

	if (m_fStatusUpdate & SU_UPDATE_TOOLTIP)
	{
		ResendTooltip();
	}

	if (IsItem())
	{
		if (auto pItemDmg = static_cast<CCItemDamageable*>(GetComponent(COMP_ITEMDAMAGEABLE)))
		{
			pItemDmg->OnTickStatsUpdate();
		}
	}

    EXC_CATCH;
}

void CObjBase::_GoAwake()
{
	ADDTOCALLSTACK("CObjBase::_GoAwake");
	CTimedObject::_GoAwake();
	if (auto pContainer = dynamic_cast<CContainer*>(this))
	{
		pContainer->_GoAwake(); // This method isn't virtual
	}

	if (_IsTimerSet())
	{
		CWorldTickingList::AddObjSingle(_GetTimeoutRaw(), this, true);
	}
	// CWorldTickingList::AddObjStatusUpdate(this, false);	// Don't! It's done when needed in UpdatePropertyFlag()
}

void CObjBase::_GoSleep()
{
	ADDTOCALLSTACK("CObjBase::_GoSleep");
	CTimedObject::_GoSleep();

	if (_IsTimerSet())
	{
		CWorldTickingList::DelObjSingle(this);
	}
	CWorldTickingList::DelObjStatusUpdate(this, false);
}

bool CObjBase::_CanTick(bool fParentGoingToSleep) const
{
	//ADDTOCALLSTACK_DEBUG("CObjBase::_CanTick");   // Called very frequently.
	// This doesn't check the sector sleeping status, it's only about this object.
    EXC_TRY("Can tick?");

    // Directly call the method specifying the belonging class, to avoid the overhead of vtable lookup under the hood.
    bool fCanTick = fParentGoingToSleep ? false : !CTimedObject::_IsSleeping();
    const bool fIgnoreCont = (HAS_FLAGS_STRICT(g_Cfg.m_uiItemTimers, ITEM_CANTIMER_IN_CONTAINER) || Can(CAN_I_TIMER_CONTAINED));

    if (fCanTick)
    {
        if (const CSObjCont* pParent = GetParent())
        {
            const CObjBase* pObjParent = dynamic_cast<const CObjBase*>(pParent);
            // The parent can be another CObjBase (or a Sector, but we are not interested in that case)
			if (pObjParent)
			{
				if (fParentGoingToSleep)
					fCanTick = false;
				else if (!fIgnoreCont)
					fCanTick = pObjParent->CanTick(fParentGoingToSleep);
			}
        }
    }

    if (!fCanTick)
    {
        // Try to call the Can method the less often possible.
		//
		// This should happen only if the item was manually put to sleep.
		// CAN_O_NOSLEEP items should not be put to sleep by the source.
	    fCanTick = Can(CAN_O_NOSLEEP);
    }

    return fCanTick;

    EXC_CATCH;

    return false;
}

void CObjBase::ResendTooltip(bool fSendFull, bool fUseCache)
{
	ADDTOCALLSTACK("CObjBase::UpdatePropertyFlag");
    // Send tooltip packet to all nearby clients

    if (g_Serv.IsLoading())
        return;
	else if ( IsAosFlagEnabled(FEATURE_AOS_UPDATE_B) == false )
		return; // tooltips are disabled.
	else if ( IsDisconnected())
		return;	// not in the world.

	if (fUseCache == false)
		FreePropertyList();

    bool fSentLeastOnce = false;
	ClientIterator it;
	for (CClient *pClient = it.next(); pClient != nullptr; pClient = it.next())
	{
        CChar *pChar = pClient->GetChar();
		if ( pChar == nullptr )
			continue;
		if ( !pChar->CanSee( this ) )
			continue;

		if (pClient->addAOSTooltip(this, fSendFull))
            fSentLeastOnce = true;
	}

    // If i haven't sent it even once, it means also that i don't have stored the updated tooltip in the cache, so we need to check again until there's some client to send it to
    if (fSentLeastOnce)
        m_fStatusUpdate &= ~SU_UPDATE_TOOLTIP;
}

CCSpawn * CObjBase::GetSpawn()
{
    if (_uidSpawn.IsValidUID())
    {
        CItem *pItem = _uidSpawn.ItemFind();
        if (pItem)
        {
            CCSpawn *pSpawn = static_cast<CCSpawn*>(pItem->GetComponent(COMP_SPAWN));
            if (pSpawn)
                return pSpawn;
        }
        _uidSpawn.InitUID();    // for some reason there is an UID assigned but not related to a CItem or CCSpawn, clear it.
    }
    return nullptr;
}

void CObjBase::SetSpawn(CCSpawn * spawn)
{
    if (spawn)
        _uidSpawn.SetObjUID(spawn->GetLink()->GetUID());
    else
        _uidSpawn.InitUID();
}

CCFaction * CObjBase::GetFaction()
{
    return static_cast<CCFaction*>(GetComponent(COMP_FACTION));
}

CSString CObjBase::GetPropStr( const CComponentProps* pCompProps, CComponentProps::PropertyIndex_t iPropIndex, bool fZero, const CComponentProps* pBaseCompProps ) const
{
    CSString sProp;
    if (pCompProps && pCompProps->GetPropertyStrPtr(iPropIndex, &sProp, fZero))
        return sProp;
    if (pBaseCompProps && pBaseCompProps->GetPropertyStrPtr(iPropIndex, &sProp, fZero))
        return sProp;
    return sProp;
}

CSString CObjBase::GetPropStr( COMPPROPS_TYPE iCompPropsType, CComponentProps::PropertyIndex_t iPropIndex, bool fZero, bool fDef ) const
{
    CSString sProp;
    const CComponentProps* pCompProps = GetComponentProps(iCompPropsType);
    if (!pCompProps)
        return sProp;
    if (pCompProps->GetPropertyStrPtr(iPropIndex, &sProp, fZero) || !fDef)
        return sProp;

    const CBaseBaseDef* pBase = Base_GetDef();
    const CComponentProps* pBaseCompProps = pBase->GetComponentProps(iCompPropsType);
    if (!pBaseCompProps)
        return sProp;
    pBaseCompProps->GetPropertyStrPtr(iPropIndex, &sProp, fZero);
    return sProp;
}

CComponentProps::PropertyValNum_t CObjBase::GetPropNum( const CComponentProps* pCompProps, CComponentProps::PropertyIndex_t iPropIndex, const CComponentProps* pBaseCompProps ) const
{
    CComponentProps::PropertyValNum_t iProp = 0;
    if (pCompProps && pCompProps->GetPropertyNumPtr(iPropIndex, &iProp))
        return iProp;
    if (pBaseCompProps && pBaseCompProps->GetPropertyNumPtr(iPropIndex, &iProp))
        return iProp;
    return iProp;
}

CComponentProps::PropertyValNum_t CObjBase::GetPropNum( COMPPROPS_TYPE iCompPropsType, CComponentProps::PropertyIndex_t iPropIndex, bool fDef ) const
{
    CComponentProps::PropertyValNum_t iProp = 0;
    const CComponentProps* pCompProps = GetComponentProps(iCompPropsType);
    if (!pCompProps)
        return iProp;
    if (pCompProps->GetPropertyNumPtr(iPropIndex, &iProp) || !fDef)
        return iProp;

    const CBaseBaseDef* pBase = Base_GetDef();
    const CComponentProps* pBaseCompProps = pBase->GetComponentProps(iCompPropsType);
    if (!pBaseCompProps)
        return iProp;
    pBaseCompProps->GetPropertyNumPtr(iPropIndex, &iProp);
    return iProp;
}

void CObjBase::SetPropStr( CComponentProps* pCompProps, CComponentProps::PropertyIndex_t iPropIndex, lpctstr ptcVal, bool fDeleteZero )
{
    ASSERT(pCompProps);
    const RESDISPLAY_VERSION iLimitToEra = Base_GetDef()->_iEraLimitProps;
    pCompProps->SetPropertyStr(iPropIndex, ptcVal, this, iLimitToEra, fDeleteZero);
}

void CObjBase::SetPropStr( COMPPROPS_TYPE iCompPropsType, CComponentProps::PropertyIndex_t iPropIndex, lpctstr ptcVal, bool fDeleteZero )
{
    CComponentProps* pCompProps = GetComponentProps(iCompPropsType);
    if (!pCompProps)
    {
        g_Log.EventDebug("CEntityProps: SetPropStr on unsubscribed CCProps. iCompPropsType %d, iPropIndex %d.\n", iCompPropsType, iPropIndex);
		ASSERT(pCompProps);
    }
    const RESDISPLAY_VERSION iLimitToEra = Base_GetDef()->_iEraLimitProps;
    pCompProps->SetPropertyStr(iPropIndex, ptcVal, this, iLimitToEra, fDeleteZero);
}

void CObjBase::SetPropNum( CComponentProps* pCompProps, CComponentProps::PropertyIndex_t iPropIndex, CComponentProps::PropertyValNum_t iVal )
{
    ASSERT(pCompProps);
    const RESDISPLAY_VERSION iLimitToEra = Base_GetDef()->_iEraLimitProps;
    pCompProps->SetPropertyNum(iPropIndex, iVal, this, iLimitToEra);
}

void CObjBase::SetPropNum( COMPPROPS_TYPE iCompPropsType, CComponentProps::PropertyIndex_t iPropIndex, CComponentProps::PropertyValNum_t iVal )
{
    CComponentProps* pCompProps = GetComponentProps(iCompPropsType);
    if (!pCompProps)
    {
        g_Log.EventDebug("CEntityProps: SetPropNum on unsubscribed CCProps. iCompPropsType %d, iPropIndex %d.\n", iCompPropsType, iPropIndex);
		ASSERT(pCompProps);
    }
    const RESDISPLAY_VERSION iLimitToEra = Base_GetDef()->_iEraLimitProps;
    pCompProps->SetPropertyNum(iPropIndex, iVal, this, iLimitToEra);
}

void CObjBase::ModPropNum( CComponentProps* pCompProps, CComponentProps::PropertyIndex_t iPropIndex, CComponentProps::PropertyValNum_t iMod, const CComponentProps* pBaseCompProps )
{
    ASSERT(pCompProps);
    CComponentProps::PropertyValNum_t iVal = 0;
    const bool fPropExists = pCompProps->GetPropertyNumPtr(iPropIndex, &iVal);
    if (!fPropExists && pBaseCompProps)
    {
        pBaseCompProps->GetPropertyNumPtr(iPropIndex, &iVal);
    }
    if (!iVal && !iMod)
        return;
    const RESDISPLAY_VERSION iLimitToEra = Base_GetDef()->_iEraLimitProps;
    pCompProps->SetPropertyNum(iPropIndex, iMod + iVal, this, iLimitToEra);
}

void CObjBase::ModPropNum( COMPPROPS_TYPE iCompPropsType, CComponentProps::PropertyIndex_t iPropIndex, CComponentProps::PropertyValNum_t iMod, bool fBaseDef )
{
    CComponentProps::PropertyValNum_t iVal = 0;
    CComponentProps* pCompProps = GetComponentProps(iCompPropsType);
    bool fPropExists;
    if (!pCompProps)
    {
        g_Log.EventDebug("CEntityProps: ModPropNum on unsubscribed CCProps. iCompPropsType %d, iPropIndex %d, fBaseDef %d.\n", iCompPropsType, iPropIndex, (int)fBaseDef);
		ASSERT(pCompProps);
        fPropExists = false;
    }
    else
    {
        fPropExists = pCompProps->GetPropertyNumPtr(iPropIndex, &iVal);
    }
    if (!fPropExists && fBaseDef)
    {
        const CBaseBaseDef* pBase = Base_GetDef();
        const CComponentProps* pBaseCompProps = pBase->GetComponentProps(iCompPropsType);
        pBaseCompProps->GetPropertyNumPtr(iPropIndex, &iVal);
    }
    if (!iVal && !iMod)
        return;
    const RESDISPLAY_VERSION iLimitToEra = Base_GetDef()->_iEraLimitProps;
    pCompProps->SetPropertyNum(iPropIndex, iVal + iMod, this, iLimitToEra);
}

lpctstr CObjBase::GetDefStr( lpctstr ptcKey, bool fZero, bool fDef ) const
{
    const CVarDefCont * pVar = GetDefKey( ptcKey, fDef );
	if ( pVar == nullptr )
		return (fZero ? "0" : "");
	return pVar->GetValStr();
}

int64 CObjBase::GetDefNum( lpctstr ptcKey, bool fDef, int64 iDefault) const
{
	const CVarDefCont * pVar = GetDefKey( ptcKey, fDef );
	if ( pVar == nullptr )
		return iDefault;
	return pVar->GetValNum();
}

void CObjBase::SetDefNum(lpctstr ptcKey, int64 iVal, bool fZero )
{
	m_BaseDefs.SetNum(ptcKey, iVal, fZero);
}

void CObjBase::ModDefNum(lpctstr ptcKey, int64 iMod, bool fBaseDef, bool fZero )
{
    bool fVarFromBase = false;
    CVarDefCont	* pVar	= m_BaseDefs.GetKey( ptcKey );
    if (fBaseDef && !pVar)
    {
        const CBaseBaseDef* pBase = Base_GetDef();
        ASSERT (pBase);
        pVar = pBase->m_BaseDefs.GetKey( ptcKey );
        fVarFromBase = true;
    }

    if (!pVar)
    {
        // It doesn't exist yet, so create a new def
        m_BaseDefs.SetNum(ptcKey, iMod, fZero);
        return;
    }
    CVarDefContNum* pVarNum = dynamic_cast<CVarDefContNum*>(pVar);
    if (!pVarNum)
    {
        // Actually there's a def with that name, but it's a CVarDefContStr, so we need to clear that and create a new CVarDefContNum
        m_BaseDefs.SetNum(ptcKey, iMod, fZero);
        return;
    }
    const int64 iNewVal = pVarNum->GetValNum() + iMod;
    if ((iNewVal == 0) && fZero && !fVarFromBase)   // Shouldn't delete a CBaseBaseDef VarDef
    {
        // If fZero and the new value of the def is 0, just delete the def
        m_BaseDefs.DeleteKey(ptcKey);
        return;
    }
    if (fVarFromBase)
    {
        // Shouldn't change the value of a CBaseBaseDef VarDef, so get the value from that, add the iMod and set
        //  this value to a new CObjBase VarDef.
        m_BaseDefs.SetNum(ptcKey, iNewVal, fZero);
        return;
    }
    // Update the CObjBase VarDef value.
    pVarNum->SetValNum(iNewVal);
}

void CObjBase::SetDefStr(lpctstr ptcKey, lpctstr pszVal, bool fQuoted, bool fZero )
{
	m_BaseDefs.SetStr(ptcKey, fQuoted, pszVal, fZero);
}

void CObjBase::DeleteDef(lpctstr ptcKey)
{
	m_BaseDefs.DeleteKey(ptcKey);
}

CVarDefCont * CObjBase::GetDefKey( lpctstr ptcKey, bool fDef ) const
{
	CVarDefCont	* pVar	= m_BaseDefs.GetKey( ptcKey );
	if ( !fDef || pVar )
		return pVar;
    const CBaseBaseDef* pBase = Base_GetDef();
    ASSERT (pBase);
    return pBase->m_BaseDefs.GetKey( ptcKey );
}

lpctstr CObjBase::GetKeyStr( lpctstr ptcKey, bool fZero, bool fDef ) const
{
    const CVarDefCont * pVar = GetKey( ptcKey, fDef );
	if ( pVar == nullptr )
		return (fZero ? "0" : "");
	return pVar->GetValStr();
}

int64 CObjBase::GetKeyNum( lpctstr ptcKey, bool fDef ) const
{
    const CVarDefCont * pVar = GetKey( ptcKey, fDef );
	if ( pVar == nullptr )
		return 0;
	return pVar->GetValNum();
}

CVarDefCont * CObjBase::GetKey( lpctstr ptcKey, bool fDef ) const
{
	CVarDefCont	* pVar	= m_TagDefs.GetKey( ptcKey );
	if ( !fDef || pVar )
		return pVar;
    const CBaseBaseDef* pBase = Base_GetDef();
    ASSERT(pBase);
    return pBase->m_TagDefs.GetKey( ptcKey );
}

void CObjBase::SetKeyNum(lpctstr ptcKey, int64 iVal)
{
	m_TagDefs.SetNum(ptcKey, iVal);
}

void CObjBase::SetKeyStr(lpctstr ptcKey, lpctstr pszVal)
{
	m_TagDefs.SetStr(ptcKey, false, pszVal);
}

void CObjBase::DeleteKey(lpctstr ptcKey)
{
	m_TagDefs.DeleteKey(ptcKey);
}

void CObjBase::DupeCopy( const CObjBase * pObj )
{
	CObjBaseTemplate::DupeCopy( pObj );
	m_wHue = pObj->GetHue();
    if (pObj->IsTimerSet())
    {
        _SetTimeout(pObj->GetTimerAdjusted());
    }
	m_TagDefs.Copy( &(pObj->m_TagDefs) );
	m_BaseDefs.Copy(&(pObj->m_BaseDefs));
    CEntityProps::Copy(pObj);
}

TRIGRET_TYPE CObjBase::Spell_OnTrigger( SPELL_TYPE spell, SPTRIG_TYPE stage, CChar * pSrc, CScriptTriggerArgs * pArgs )
{
	ADDTOCALLSTACK("CObjBase::Spell_OnTrigger");
	CSpellDef * pSpellDef = g_Cfg.GetSpellDef( spell );
	if ( !pSpellDef )
		return TRIGRET_RET_TRUE;

	if ( pSpellDef->HasTrigger( stage ) )
	{
		// RES_SKILL
		CResourceLock s;
		if ( pSpellDef->ResourceLock( s ))
		{
			return CScriptObj::OnTriggerScript( s, CSpellDef::sm_szTrigName[stage], pSrc, pArgs );
		}
	}
	return TRIGRET_RET_DEFAULT;
}

bool CObjBase::IsRunningTrigger() const
{
	return ((_iRunningTriggerId >= 0) || !_sRunningTrigger.empty());
}

bool CObjBase::CallPersonalTrigger(tchar * pArgs, CTextConsole * pSrc, TRIGRET_TYPE & trResult)
{
	ADDTOCALLSTACK("CObjBase::CallPersonalTrigger");
	tchar * ppCmdTrigger[3];
	size_t iResultArgs = Str_ParseCmds(pArgs, ppCmdTrigger, ARRAY_COUNT(ppCmdTrigger), ",");

	if ( iResultArgs > 0 )
	{
		lpctstr callTrigger = ppCmdTrigger[0];
		CScriptTriggerArgs csTriggerArgs;

		if ( iResultArgs == 3 )
		{
			int iTriggerArgType = atoi(ppCmdTrigger[1]);

			if ( iTriggerArgType == 1 ) // 3 ARGNs
			{
				int64 Arg_piCmd[3];
				iResultArgs = Str_ParseCmds(ppCmdTrigger[2], Arg_piCmd, ARRAY_COUNT(Arg_piCmd), ",");

				if ( iResultArgs == 3 )
					csTriggerArgs.m_iN3 = Arg_piCmd[2];

				if ( iResultArgs >= 2 )
					csTriggerArgs.m_iN2 = Arg_piCmd[1];

				if ( iResultArgs >= 1 )
					csTriggerArgs.m_iN1 = Arg_piCmd[0];
			}
			else if ( iTriggerArgType == 2 ) // ARGS
			{
				csTriggerArgs.m_s1 = ppCmdTrigger[2];
				csTriggerArgs.m_s1_buf_vec = ppCmdTrigger[2];
			}
			else if ( iTriggerArgType == 3 ) // ARGO
			{
				CUID guTriggerArg(Exp_GetVal(ppCmdTrigger[2]));
				CObjBase * pTriggerArgObj = guTriggerArg.ObjFind();
				if ( pTriggerArgObj )
					csTriggerArgs.m_pO1 = pTriggerArgObj;
			}
			else if ( iTriggerArgType == 4 ) // FULL TRIGGER
			{
				tchar * Arg_ppCmd[5];
				iResultArgs = Str_ParseCmds(ppCmdTrigger[2], Arg_ppCmd, ARRAY_COUNT(Arg_ppCmd), ",");

				// ARGS
				if ( iResultArgs == 5 )
				{
					csTriggerArgs.m_s1 = Arg_ppCmd[4];
					csTriggerArgs.m_s1_buf_vec = Arg_ppCmd[4];
				}
				// ARGNs
				if ( iResultArgs >= 4 )
					csTriggerArgs.m_iN3 = Exp_GetVal(Arg_ppCmd[3]);
				if ( iResultArgs >= 3 )
					csTriggerArgs.m_iN2 = Exp_GetVal(Arg_ppCmd[2]);
				if ( iResultArgs >= 2 )
					csTriggerArgs.m_iN1 = Exp_GetVal(Arg_ppCmd[1]);
				// ARGO
				if ( iResultArgs >= 1 )
				{
					CObjBase * pTriggerArgObj = CUID::ObjFindFromUID(Exp_GetVal(Arg_ppCmd[0]));
					if ( pTriggerArgObj )
						csTriggerArgs.m_pO1 = pTriggerArgObj;
				}
			}
		}

		trResult = OnTrigger(callTrigger, pSrc, &csTriggerArgs);
		return true;
	}

	return false;
}

