/**
* @file CCharNPC.h
* 
*/

#ifndef _INC_CCHARNPC_H
#define _INC_CCHARNPC_H

#include "../CPathFinder.h"
#include "CChar.h"


enum CNC_TYPE
{
#define ADD(a,b) CNC_##a,
#include "../../tables/CCharNpc_props.tbl"
#undef ADD
	CNC_QTY
};

class CCharNPC
{
	// This is basically the unique "brains" for any character.
public:
	static const char *m_sClassName;
	// Stuff that is specific to an NPC character instance (not an NPC type see CCharBase for that).
	// Any NPC AI stuff will go here.
	static lpctstr const sm_szVerbKeys[];
    static lpctstr const sm_szLoadKeys[];

	NPCBRAIN_TYPE m_Brain;		// For NPCs: Number of the assigned basic AI block
	word m_Home_Dist_Wander;	// Distance to allow to "wander".
	byte m_Act_Motivation;		// 0-100 (100=very greatly) how bad do i want to do the current action.
	bool m_bonded;				// Bonded pet

    int64	m_timeRestock;		//	when last restock happened in sell/buy container

	CResourceRefArray m_Speech;	// Speech fragment list (other stuff we know): We respond to what we hear with this.
	CResourceQty m_Need;	// What items might i need/Desire ? (coded as resource scripts) ex "10 gold,20 logs" etc.	

	short	m_nextX[MAX_NPC_PATH_STORAGE_SIZE];	// array of X coords of the next step
	short	m_nextY[MAX_NPC_PATH_STORAGE_SIZE];	// array of Y coords of the next step
	CPointMap m_nextPt;							// where the array(^^) wants to go, if changed, recount the path


	struct Spells {
		SPELL_TYPE	id;
	};
	std::vector<Spells> m_spells;	// Spells stored in this NPC

	int Spells_GetCount();
	SPELL_TYPE Spells_GetAt(uchar id);
	bool Spells_DelAt(uchar id);
	bool Spells_Add(SPELL_TYPE spell);
	int Spells_FindSpell(SPELL_TYPE spell);

public:
	void r_WriteChar( CChar * pChar, CScript & s );
	bool r_WriteVal( CChar * pChar, lpctstr ptcKey, CSString & s );
	bool r_LoadVal( CChar * pChar, CScript & s );

	bool IsVendor() const;

	int GetNpcAiFlags( const CChar *pChar ) const;

public:
	CCharNPC( CChar * pChar, NPCBRAIN_TYPE NPCBrain );
	~CCharNPC();

	CCharNPC(const CCharNPC& copy) = delete;
	CCharNPC& operator=(const CCharNPC& other) = delete;
};


#endif // _INC_CCHARNPC_H
