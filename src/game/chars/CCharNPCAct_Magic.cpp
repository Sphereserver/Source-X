// Actions specific to an NPC.

#include "../CWorldMap.h"
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
    size_t size = m_spells.size();
    for (int count = 0; count < size; ++count)
    {
        Spells refSpell = m_spells[count];
        if (!refSpell.id)
            continue;
        ++total;
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
    if (m_spells.size() <= id)
        return SPELL_NONE;
    Spells refSpell = m_spells[id];
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
    if (m_spells.size() <= id)
        return SPELL_NONE;
    Spells refSpell = m_spells[id];
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
        Spells spell = m_spells[count];
        if (spell.id == spellID)
            return count;
        ++count;
    }
    return -1;
}

void CChar::NPC_GetAllSpellbookSpells()		// Add all spells found on spellbooks to the NPC internal spell list
{
    ADDTOCALLSTACK("CChar::GetSpellbook");
    ASSERT(m_pNPC);

    //	search for suitable book in hands first
    for (CSObjContRec* pObjRec : *this)
    {
        CItem* pBook = static_cast<CItem*>(pObjRec);
        if (pBook->IsTypeSpellbook())
            NPC_AddSpellsFromBook(pBook);
    }

    //	then search in the top level of the pack
    CItemContainer *pPack = GetPack();
    if (pPack)
    {
        for (CSObjContRec* pObjRec : *pPack)
        {
            CItem* pBook = static_cast<CItem*>(pObjRec);
            if (pBook->IsTypeSpellbook())
                NPC_AddSpellsFromBook(pBook);
        }
    }
}

void CChar::NPC_AddSpellsFromBook(CItem * pBook)
{
    ADDTOCALLSTACK("CChar::NPC_AddSpellsFromBook");
    ASSERT(m_pNPC);

    CItemBase *pBookDef = pBook->Item_GetDef();
    if (!pBookDef)
        return;

    const uint min = pBookDef->m_ttSpellbook.m_iOffset + 1;
    const uint max = pBookDef->m_ttSpellbook.m_iOffset + pBookDef->m_ttSpellbook.m_iMaxSpells;

    for (uint i = min; i <= max; ++i)
    {
        SPELL_TYPE spell = (SPELL_TYPE)i;
        if (pBook->IsSpellInBook(spell))
            m_pNPC->Spells_Add(spell);
    }
}

// cast a spell if i can ?
// or i can throw or use archery ?
// RETURN:
//  false = revert to melee type fighting.
bool CChar::NPC_FightMagery(CChar * pChar)
{
    ADDTOCALLSTACK("CChar::NPC_FightMagery");
    ASSERT(m_pNPC);

    if (!NPC_FightMayCast(false))	// not checking skill here since it will do a search later and it's an expensive function.
    {
        return false;
    }

    int iSpellCount = m_pNPC->Spells_GetCount();
    CItem * pWand = LayerFind(LAYER_HAND1);		//Try to get a working wand.
    CObjBase * pTarg = pChar;
    if (pWand)
    {
        // If the item is really a wand and have it charges it's a valid wand, if not ... we get rid of it.
        if (pWand->GetType() != IT_WAND || pWand->m_itWeapon.m_spellcharges <= 0)
            pWand = nullptr;
    }
    if ((iSpellCount < 1) && !pWand)
        return false;

    int iDist = GetTopDist3D(pChar);
    if (iDist > ((UO_MAP_VIEW_SIGHT * 3) / 4))	// way too far away . close in.
        return false;

    if ((iDist <= 1) && (Skill_GetBase(SKILL_TACTICS) > 200) && (!Calc_GetRandVal(2)))
    {
        // Within striking distance.
        // Stand and fight for a bit.
        return false;
    }
    int skill = (int)SKILL_NONE;
    ushort uiStatInt = Stat_GetBase(STAT_INT);
    ushort uiMana = Stat_GetVal(STAT_INT);
    int iChance = ((uiMana >= (uiStatInt / 2)) ? uiMana : (uiStatInt - uiMana));

    CObjBase * pSrc = this;
    if (Calc_GetRandVal(iChance) < uiStatInt / 4)
    {
        // we failed this test, but we could be casting next time
        // back off from the target a bit
        if (uiMana > (uiStatInt / 3) && Calc_GetRandVal(uiStatInt))
        {
            if (iDist < 4 || iDist > 8)	// Here is fine?
                NPC_Act_Follow(false, Calc_GetRandVal(3) + 2, true);
            
            return true;
        }
        return false;
    }

    // We have the total count of spells inside iSpellCount, so we use 'iRandSpell' to store a rand representing the spell that will be casted
    uchar iRandSpell = pWand ? 1 : 0;	// Having wand adding +1 spell to the total count
    iRandSpell += (uchar)(Calc_GetRandVal2(0, iSpellCount - 1));	// spells are being stored using a vector, so it's assumed to be zero-based.

    if (iRandSpell > iSpellCount)	// if iRandSpell > iSpellCount then we've got the roll pointing to use the wand's spell.
    {
        ASSERT(pWand);
        SPELL_TYPE spell = (SPELL_TYPE)(pWand->m_itWeapon.m_spell);
        const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
        if (!pSpellDef)	// wand check failed ... we go on melee, next cast try might select another type of spell :)
            return false;
        
        pSrc = pWand;	// Seting pWand as SRC, this will force @SpellCast to have the wand as ARGO.
        if (NPC_FightCast(pTarg, pWand, spell))
            goto BeginCast;	//if can cast this spell we jump the for() and go directly to it's casting.
    }

    for (; iRandSpell < iSpellCount; ++iRandSpell)
    {
        SPELL_TYPE spell = m_pNPC->Spells_GetAt(iRandSpell);
        const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
        if (!pSpellDef)	//If it reached here it should exist, checking anyway.
            continue;

        int iSkillReq = 0;
        if (!pSpellDef->GetPrimarySkill(&skill, &iSkillReq))
            skill = SKILL_MAGERY;

        if (Skill_GetBase((SKILL_TYPE)skill) < iSkillReq)
            continue;
        if (NPC_FightCast(pTarg, this, spell, (SKILL_TYPE)skill))
            goto BeginCast;	//if can cast this spell we jump the for() and go directly to it's casting.
    }
    return false;	// No castable spell found, go back on melee.

BeginCast:	//Start casting
            // KRJ - give us some distance
            // if the opponent is close, get away from him
    if ((uiMana > uiStatInt / 3) && Calc_GetRandVal(uiStatInt << 1))
    {
        if (iDist < 4 || iDist > 8)	// Here is fine?
            NPC_Act_Follow(false, 5, true);
    }
    else
        NPC_Act_Follow();

    Reveal();

    m_Act_UID = pTarg->GetUID();
    m_Act_Prv_UID = pSrc->GetUID();
    m_Act_p = pTarg->GetTopPoint();

    // Calculate the difficulty
    return Skill_Start((SKILL_TYPE)skill);
}

// I'm able to use magery
// test if I can cast this spell
// Specific behaviours for each spell and spellflag
bool CChar::NPC_FightCast(CObjBase * &pTarg, CObjBase * pSrc, SPELL_TYPE &spell, SKILL_TYPE skill)
{
    ADDTOCALLSTACK("CChar::NPC_FightCast");
    ASSERT(m_pNPC);

    const CSpellDef * pSpellDef = g_Cfg.GetSpellDef(spell);
    if (skill == SKILL_NONE)
    {
        int iSkillTest = 0;
        if (!pSpellDef->GetPrimarySkill(&iSkillTest, nullptr))
            iSkillTest = SKILL_MAGERY;
        skill = (SKILL_TYPE)iSkillTest;
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
                pFriend[1] = pFriend[2] = pFriend[3] = nullptr;
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
                            if (iFriendIndex >= 4)
                                break;
                        }
                    }
                }

                //	i cannot cast this on self. ok, then friends only
                if (pSpellDef->IsSpellType(SPELLFLAG_TARG_NOSELF))
                {
                    pFriend[0] = pFriend[1];
                    pFriend[1] = pFriend[2];
                    pFriend[2] = pFriend[3];
                    pFriend[3] = nullptr;
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
                    m_atMagery.m_iSpell = spell;
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
                switch (spell)
                {
                    //No spells added ATM until they are created, good example spell to here = SPELL_Healing_Stone
                    case SPELL_Healing_Stone:
                    {
                        CItem * pStone = GetBackpackItem(ITEMID_HEALING_STONE);
                        if (!pStone)
                            break;
                        if ((pStone->m_itNormal.m_morep.m_z == 0) && (Stat_GetVal(STAT_STR) < pStone->m_itNormal.m_more2) && (pStone->m_itNormal.m_more1 >= pStone->m_itNormal.m_more2))
                        {
                            Use_Obj(pStone, false);
                            return true; // we are not casting any spell but suceeded at using the stone created by this one, we are done now.
                        }
                        return false;	// Already have a stone, no need of more
                    }
                    default:
                        break;
                }

                pTarg = this;
                m_atMagery.m_iSpell = spell;
                return true;
            }
        }
        else if (pSpellDef->IsSpellType(SPELLFLAG_SUMMON))
        {
            m_atMagery.m_iSpell = spell;
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
    m_atMagery.m_iSpell = spell;
    return true;
}
