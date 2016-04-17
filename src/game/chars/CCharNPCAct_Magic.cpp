// Actions specific to an NPC.

#include "CCharNPC.h"

// Retrieves all the spells this character has to spells[x] list
int CCharNPC::Spells_GetCount()
{
	ADDTOCALLSTACK("CCharNPC::Spells_GetAll");
	if (m_spells.empty())
		return -1;
	return (int)m_spells.size();

	// This code was meant to check if found spells does really exist
	/*
	int total = 0;
	for (int count = 0; count < m_spells.size() ; count++)
	{
		Spells refSpell = m_spells.at(count);
		if (!refSpell.id)
			continue;
		total++;
	}
	return total;
	*/
}

// Retrieve the spell stored at index = n
SPELL_TYPE CCharNPC::Spells_GetAt(uchar id)
{
	ADDTOCALLSTACK("CCharNPC::Spells_GetAt");
	if (m_spells.empty())
		return SPELL_NONE;
	if (id >= m_spells.size())
		return SPELL_NONE;
	Spells refSpell = m_spells.at(id);
	if (refSpell.id)
		return refSpell.id;
	return SPELL_NONE;
}

// Delete the spell at the given index
bool CCharNPC::Spells_DelAt(uchar id)
{
	ADDTOCALLSTACK("CCharNPC::Spells_DelAt");
	if (m_spells.empty())
		return false;
	Spells refSpell = m_spells.at(id);
	if (refSpell.id)
	{
		std::vector<Spells>::iterator it = m_spells.begin() + id;
		m_spells.erase(it);
		return true;
	}
	return false;
}

// Add this spell to this NPC's list
bool CCharNPC::Spells_Add(SPELL_TYPE spell)
{
	ADDTOCALLSTACK("CCharNPC::Spells_Add");
	if (Spells_FindSpell(spell) >= 0)
		return false;
	CSpellDef * pSpell = g_Cfg.GetSpellDef(spell);
	if (!pSpell)
		return false;
	Spells refSpell;
	refSpell.id = spell;
	m_spells.push_back(refSpell);
	return true;
}

// Retrieves the indexed id for the given spell, if found.
int CCharNPC::Spells_FindSpell(SPELL_TYPE spellID)
{
	ADDTOCALLSTACK("CCharNPC::Spells_FindSpell");
	if (m_spells.empty())
		return -1;

	uint count = 0;
	while (count < m_spells.size())
	{
		Spells spell = m_spells.at(count);
		if (spell.id == spellID)
			return count;
		count++;
	}
	return -1;
}

bool CChar::NPC_GetAllSpellbookSpells()	// Retrieves a spellbook from the magic school given in iSpell
{
	ADDTOCALLSTACK("CChar::GetSpellbook");
	//	search for suitable book in hands first
	for ( CItem *pBook = GetContentHead(); pBook != NULL; pBook = pBook->GetNext() )
	{
		if (pBook->IsTypeSpellbook())
		{
			if (!NPC_AddSpellsFromBook(pBook))
				continue;
		}
	}

	//	then search in the top level of the pack
	CItemContainer *pPack = GetPack();
	if (pPack)
	{
		for ( CItem *pBook = pPack->GetContentHead(); pBook != NULL; pBook = pBook->GetNext() )
		{
			if (pBook->IsTypeSpellbook())
			{
				if (!NPC_AddSpellsFromBook(pBook))
					continue;
			}
		}
	}
	return true;
}

bool CChar::NPC_AddSpellsFromBook(CItem * pBook)
{
	if (!m_pNPC)
		return false;
	SKILL_TYPE skill = pBook->GetSpellBookSkill();
	int max = Spell_GetMax(skill);
	for (int i = 0; i <= max; i++)
	{
		SPELL_TYPE spell = static_cast<SPELL_TYPE>(i);
		if (pBook->IsSpellInBook(spell))
			m_pNPC->Spells_Add(spell);
	}
	return true;
}

// cast a spell if i can ?
// or i can throw or use archery ?
// RETURN:
//  false = revert to melee type fighting.
bool CChar::NPC_FightMagery(CChar * pChar)
{
	ADDTOCALLSTACK("CChar::NPC_FightMagery");
	if (!NPC_FightMayCast(false))	// not checking skill here since it will do a search later and it's an expensive function.
		return false;

	size_t count = m_pNPC->Spells_GetCount();
	CItem * pWand = LayerFind(LAYER_HAND1);		//Try to get a working wand.
	CObjBase * pTarg = pChar;
	if (pWand)
	{
		if (pWand->GetType() != IT_WAND || pWand->m_itWeapon.m_spellcharges <= 0)// If the item is really a wand and have it charges it's a valid wand, if not ... we get rid of it.
			pWand = NULL;
	}
	if (count < 1 && !pWand)
		return false;

	int iDist = GetTopDist3D(pChar);
	if (iDist >((UO_MAP_VIEW_SIGHT * 3) / 4))	// way too far away . close in.
		return false;

	if (iDist <= 1 &&
		Skill_GetBase(SKILL_TACTICS) > 200 &&
		!Calc_GetRandVal(2))
	{
		// Within striking distance.
		// Stand and fight for a bit.
		return false;
	}
	int skill = SKILL_NONE;
	int iStatInt = Stat_GetBase(STAT_INT);
	int mana = Stat_GetVal(STAT_INT);
	int iChance = ((mana >= (iStatInt / 2)) ? mana : (iStatInt - mana));

	CObjBase * pSrc = this;
	if (Calc_GetRandVal(iChance) < iStatInt / 4)
	{
		// we failed this test, but we could be casting next time
		// back off from the target a bit
		if (mana > (iStatInt / 3) && Calc_GetRandVal(iStatInt))
		{
			if (iDist < 4 || iDist > 8)	// Here is fine?
			{
				NPC_Act_Follow(false, Calc_GetRandVal(3) + 2, true);
			}
			return true;
		}
		return false;
	}
	uchar i = 0;
	if (pWand)
		i = (uchar)(Calc_GetRandVal2(0, count));	//chance between all spells + wand
	else
		i = (uchar)(Calc_GetRandVal2(0, count-1));

	if (i > count)	// if i > count then we use wand to cast.
	{
		SPELL_TYPE spell = static_cast<SPELL_TYPE>(pWand->m_itWeapon.m_spell);
		const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
		if (!pSpellDef)	// wand check failed ... we go on melee, next cast try might select another type of spell :)
			return false;
		pSrc = pWand;
		if (NPC_FightCast(pTarg, pWand, spell))
			goto BeginCast;	//if can cast this spell we jump the for() and go directly to it's casting.
	}

	for (; i < count; i++)
	{
		SPELL_TYPE spell = m_pNPC->Spells_GetAt(i);
		const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
		if (!pSpellDef)	//If it reached here it should exist, checking anyway.
			continue;

		int iSkillReq = 0;
		if (!pSpellDef->GetPrimarySkill(&skill, &iSkillReq))
			skill = SKILL_MAGERY;

		if (Skill_GetBase(static_cast<SKILL_TYPE>(skill)) < iSkillReq)
			continue;
		if (NPC_FightCast(pTarg, this, spell, static_cast<SKILL_TYPE>(skill)))
			goto BeginCast;	//if can cast this spell we jump the for() and go directly to it's casting.
	}
	return false;	// No castable spell found, go back on melee.

BeginCast:	//Start casting
			// KRJ - give us some distance
			// if the opponent is using melee
			// the bigger the disadvantage we have in hitpoints, the further we will go
	if (mana > iStatInt / 3 && Calc_GetRandVal(iStatInt << 1))
	{
		if (iDist < 4 || iDist > 8)	// Here is fine?
		{
			NPC_Act_Follow(false, 5, true);
		}
	}
	else NPC_Act_Follow();

	Reveal();

	m_Act_Targ = pTarg->GetUID();
	m_Act_TargPrv = pSrc->GetUID();	// I'm using a wand ... or casting this directly?.
	m_Act_p = pTarg->GetTopPoint();

	// Calculate the difficulty
	return Skill_Start(static_cast<SKILL_TYPE>(skill)); 
}

// I'm able to use magery
// test if I can cast this spell
// Specific behaviours for each spell and spellflag
bool CChar::NPC_FightCast(CObjBase * &pTarg, CObjBase * pSrc, SPELL_TYPE &spell, SKILL_TYPE skill)
{
	ADDTOCALLSTACK("CChar::NPC_FightCast");
	const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
	if (skill == SKILL_NONE)
	{
		int iSkillTest = 0;
		if (!pSpellDef->GetPrimarySkill(&iSkillTest, NULL))
			iSkillTest = SKILL_MAGERY;
		skill = static_cast<SKILL_TYPE>(iSkillTest);
	}
	if (!Spell_CanCast(spell, true, pSrc, false))
		return false;
	if (pSpellDef->IsSpellType(SPELLFLAG_PLAYERONLY))
		return false;

	if (!pSpellDef->IsSpellType(SPELLFLAG_HARM))
	{
		if (pSpellDef->IsSpellType(SPELLFLAG_GOOD))
		{
			if (pSpellDef->IsSpellType(SPELLFLAG_TARG_CHAR) && pTarg->IsChar())
			{
				//	help self or friends if needed. support 3 friends + self for castings
				bool	bSpellSuits = false;
				CChar	*pFriend[4];
				int		iFriendIndex = 0;
				CChar	*pTarget = pTarg->GetUID().CharFind();

				pFriend[0] = this;
				pFriend[1] = pFriend[2] = pFriend[3] = NULL;
				iFriendIndex = 1;

				if (NPC_GetAiFlags()&NPC_AI_COMBAT)
				{
					//	search for the neariest friend in combat
					CWorldSearch AreaChars(GetTopPoint(), UO_MAP_VIEW_SIGHT);
					for (;;)
					{
						pTarget = AreaChars.GetChar();
						if (!pTarget)
							break;

						CItemMemory *pMemory = pTarget->Memory_FindObj(pTarg);
						if (pMemory && pMemory->IsMemoryTypes(MEMORY_FIGHT | MEMORY_HARMEDBY | MEMORY_IRRITATEDBY))
						{
							pFriend[iFriendIndex++] = pTarget;
							if (iFriendIndex >= 4) break;
						}
					}
				}

				//	i cannot cast this on self. ok, then friends only
				if (pSpellDef->IsSpellType(SPELLFLAG_TARG_NOSELF))
				{
					pFriend[0] = pFriend[1];
					pFriend[1] = pFriend[2];
					pFriend[2] = pFriend[3];
					pFriend[3] = NULL;
				}
				for (iFriendIndex = 0; iFriendIndex < 4; iFriendIndex++)
				{
					pTarget = pFriend[iFriendIndex];
					if (!pTarget) 
						break;
					//	check if the target need that
					switch (spell)
					{
						// Healing has the top priority?
						case SPELL_Heal:
						case SPELL_Great_Heal:
							if (pTarget->Stat_GetVal(STAT_STR) < pTarget->Stat_GetAdjusted(STAT_STR) / 3)
								bSpellSuits = true;
							break;
						case SPELL_Gift_of_Renewal:
							if (pTarget->Stat_GetVal(STAT_STR) < pTarget->Stat_GetAdjusted(STAT_STR) / 2)
								bSpellSuits = true;
							break;
							// Then curing poison.
						case SPELL_Cure:
							if (pTarget->LayerFind(LAYER_FLAG_Poison)) bSpellSuits = true;
							break;

							// Buffs are coming now.

						case SPELL_Reactive_Armor:	// Deffensive ones first
							if (!pTarget->LayerFind(LAYER_SPELL_Reactive))
								bSpellSuits = true;
							break;
						case SPELL_Protection:
							if (!pTarget->LayerFind(LAYER_SPELL_Protection))
								bSpellSuits = true;
							break;
						case SPELL_Magic_Reflect:
							if (!pTarget->LayerFind(LAYER_SPELL_Magic_Reflect))
								bSpellSuits = true;
							break;

						case SPELL_Bless:		// time for the others ...
							if (!pTarget->LayerFind(LAYER_SPELL_STATS))
								bSpellSuits = true;
							break;
						default:
							break;
					}
					if (bSpellSuits)
						break;

					LAYER_TYPE layer = pSpellDef->m_idLayer;
					if (layer != LAYER_NONE)	// If the spell applies an effect.
					{
						if (!pTarget->LayerFind(layer))	// and target doesn't have this effect already...
						{
							bSpellSuits = true;	//then it may need it
							break;
						}
					}

				}
				if (bSpellSuits)
				{
					pTarg = pTarget;
					m_atMagery.m_Spell = spell;
					return true;
				}
				return false;
			}
			else if (pSpellDef->IsSpellType(SPELLFLAG_TARG_ITEM))
			{
				//	spell is good, but must be targeted at an item
				switch (spell)
				{
					case SPELL_Immolating_Weapon:
						pTarg = m_uidWeapon.ObjFind();
						if (pTarg)
							break;
					default:
						break;
				}
			}
			if (pSpellDef->IsSpellType(SPELLFLAG_HEAL)) //Good spells that cannot be targeted
			{
				bool bSpellSuits = true;
				switch (spell)
				{
					//No spells added ATM until they are created, good example spell to here = SPELL_Healing_Stone
					case SPELL_Healing_Stone:
					{
						CItem * pStone = GetBackpackItem(ITEMID_HEALING_STONE);
						if (!pStone)
							break;
						if ((pStone->m_itNormal.m_morep.m_z == 0) && (Stat_GetVal(STAT_STR) < (int)(pStone->m_itNormal.m_more2)) && (pStone->m_itNormal.m_more1 >= pStone->m_itNormal.m_more2))
						{
							Use_Obj(pStone, false);
							return true; // we are not casting any spell but suceeded at using the stone created by this one, we are done now.
						}
						return false;	// Already have a stone, no need of more
					}
					default:
						break;
				}

				if (!bSpellSuits) 
					return false;
				pTarg = this;
				m_atMagery.m_Spell = spell;
				return true;
			}
		}
		else if (pSpellDef->IsSpellType(SPELLFLAG_SUMMON))
		{
			m_atMagery.m_Spell = spell;
			return true;	// if flag is present ... we leave the rest to the incoming code
		}
	}
	else
	{
		/*if (NPC_GetAiFlags()&NPC_AI_STRICTCAST)
		return false;*/
		//if ( /*!pVar &&*/ !pSpellDef->IsSpellType(SPELLFLAG_HARM))
		//	return false;

		// less chance for berserker spells
		/*if (pSpellDef->IsSpellType(SPELLFLAG_SUMMON) && Calc_GetRandVal(2))
		return false;

		// less chance for field spells as well
		if (pSpellDef->IsSpellType(SPELLFLAG_FIELD) && Calc_GetRandVal(4))
		return false;*/
	}
	m_atMagery.m_Spell = spell;
	return true;
}
