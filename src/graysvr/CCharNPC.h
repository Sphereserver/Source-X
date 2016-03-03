#ifndef _INC_CCHARNPC_H
#define _INC_CCHARNPC_H

#include "CChar.h"

enum CNC_TYPE
{
#define ADD(a,b) CNC_##a,
#include "../tables/CCharNpc_props.tbl"
#undef ADD
	CNC_QTY
};

struct CCharNPC
{
	// This is basically the unique "brains" for any character.
public:
	static const char *m_sClassName;
	// Stuff that is specific to an NPC character instance (not an NPC type see CCharBase for that).
	// Any NPC AI stuff will go here.
	static LPCTSTR const sm_szVerbKeys[];

	NPCBRAIN_TYPE m_Brain;		// For NPCs: Number of the assigned basic AI block
	WORD m_Home_Dist_Wander;	// Distance to allow to "wander".
	BYTE m_Act_Motivation;		// 0-100 (100=very greatly) how bad do i want to do the current action.
	bool m_bonded;				// Bonded pet

								// We respond to what we here with this.
	CResourceRefArray m_Speech;	// Speech fragment list (other stuff we know)
	HUE_TYPE m_SpeechHue;

	CResourceQty m_Need;	// What items might i need/Desire ? (coded as resource scripts) ex "10 gold,20 logs" etc.

	static LPCTSTR const sm_szLoadKeys[];

	WORD	m_nextX[MAX_NPC_PATH_STORAGE_SIZE];	// array of X coords of the next step
	WORD	m_nextY[MAX_NPC_PATH_STORAGE_SIZE];	// array of Y coords of the next step
	CPointMap m_nextPt;							// where the array(^^) wants to go, if changed, recount the path
	CServTime	m_timeRestock;		//	when last restock happened in sell/buy container

	struct Spells {
		SPELL_TYPE	id;
	};
	std::vector<Spells> m_spells;	// Spells stored in this NPC

	int Spells_GetCount();
	SPELL_TYPE Spells_GetAt(unsigned char id);
	bool Spells_DelAt(unsigned char id);
	bool Spells_Add(SPELL_TYPE spell);
	int Spells_FindSpell(SPELL_TYPE spell);

public:
	void r_WriteChar( CChar * pChar, CScript & s );
	bool r_WriteVal( CChar * pChar, LPCTSTR pszKey, CGString & s );
	bool r_LoadVal( CChar * pChar, CScript & s );

	bool IsVendor() const;

	int GetNpcAiFlags( const CChar *pChar ) const;
public:
	CCharNPC( CChar * pChar, NPCBRAIN_TYPE NPCBrain );
	~CCharNPC();

private:
	CCharNPC(const CCharNPC& copy);
	CCharNPC& operator=(const CCharNPC& other);
};


#endif //_INC_CCHARNPC_H
