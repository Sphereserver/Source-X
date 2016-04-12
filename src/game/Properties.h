
#pragma once
#ifndef _INC_PROPERTIES_H
#define _INC_PROPERTIES_H
/**
* @brief Used to store, set and retrieve all properties that were added eventually since AOS (Non Vanilla properties/stats).
*
* -Most of them are attached to items, however we add them on both items and chars so chars can increase/decrease 
* the total count in order to do not continuously check for all items and their values.
* -While some of them are NOT related to items or chars, but we give them a spot here to provide some flexibility
* in scripts, like ResPhysicalMax/etc which would have defined values, but we give them the ability to change (Commented as 'Custom')*
* *The ones which doesnt fit in UO's 'properties' will, almost for sure, not have clilocs related, so they won't be shown by default
*  but they still can be given through items or set directly on chars to modify their values.
* (Bool*) = Some props should be bool, but adding it as char would allow incremental count in chars, so adding two pieces and removing one will keep the counter in 1
*/
struct NewProperties
{
private:
	// Stats bonuses
	short m_iBonusStr;		///< Storing stat
	short m_iBonusInt;		///< Storing stat
	short m_iBonusDex;		///< Storing stat
	short m_iBonusHits;		///< Storing stat
	short m_iBonusMana;		///< Storing stat
	short m_iBonusStam;		///< Storing stat

	// Stats regens
	short m_iRegenHits;		///< Storing stat
	short m_iRegenMana;		///< Storing stat
	short m_iRegenStam;		///< Storing stat

	// Elemental Resistances
	char m_iResPhysical;	///< Storing stat
	char m_iResFire;		///< Storing stat
	char m_iResCold;		///< Storing stat
	char m_iResPoison;		///< Storing stat
	char m_iResEnergy;		///< Storing stat
	char m_iResPhysicalMax;	///< Storing stat (Custom)
	char m_iResFireMax;		///< Storing stat (Custom)
	char m_iResColdMax;		///< Storing stat (Custom)
	char m_iResPoisonMax;	///< Storing stat (Custom)
	char m_iResEnergyMax;	///< Storing stat (Custom)

	// Elemental Damages
	char m_iDamPhysical;	///< Storing stat
	char m_iDamFire;		///< Storing stat
	char m_iDamCold;		///< Storing stat
	char m_iDamPoison;		///< Storing stat
	char m_iDamEnergy;		///< Storing stat
	char m_iDamChaos;		///< Storing stat
	char m_iDamDirect;		///< Storing stat

	// Elemental Eater
	char m_iEaterFire;		// ? storage type? char? short? big TODO
	char m_iEaterCold;		// ?
	char m_iEaterPoison;	// ?
	char m_iEaterEnergy;	// ?
	char m_iEaterKinetic;	// ?
	char m_iEaterDamage;	// ?

	// Elemental Resonance
	char m_iResonanceFire;		// Chance to resist spell-casting interruption if the damage received is the same type as the resonance.
	char m_iResonanceCold;		// storage type? char? short? big TODO
	char m_iResonancePoison;	// ?
	char m_iResonanceEnergy;	// ?
	char m_iResonanceKinetic;	// ?

	// Elemental SoulCharge
	char m_iSoulCharge;			// A chance to convert a percentage of damage dealt to the player into mana.
	char m_iSoulChargeFire;		// storage type? char? short? big TODO
	char m_iSoulChargeCold;		// ?
	char m_iSoulChargePoison;	// ?
	char m_iSoulChargeEnergy;	// ?
	char m_iSoulChargeKinetic;	// ?

	// Hit Bonuses
	char m_iHitMagicArrow;
	char m_iHitFireball;
	char m_iHitHarm;
	char m_iHitLightning;
	char m_iHitCurse;			// Works similarly to the spell ‘curse’ but has a 30 second cooldown.
	char m_iHitDispel;

	char m_iHitAreaPhysical;
	char m_iHitAreaFire;
	char m_iHitAreaCold;
	char m_iHitAreaPoison;
	char m_iHitAreaEnergy;

	char m_iHitLowerAtk;
	char m_iHitLowerDef;

	char m_iHitLeechLife;
	char m_iHitLeechMana;
	char m_iHitLeechStam;

	char m_iHitManaDrain;	// A successful hit with such a weapon reduces a target's mana by 20% of the damage dealt by the wielder who triggered the effect. The intensity ranges between 2% - 50%.
	char m_iHitFatigue;		// A successful hit with such a weapon reduces a target's stamina by 20% of the damage dealt by the wielder who triggered the effect. The intensity ranges between 2% - 50%.

	// Dam / Def Bonuses
	char m_iIncreaseSwingSpeed;
	char m_iIncreaseDam;
	char m_iIncreaseHitChance;
	char m_iIncreaseDefChance;
	char m_iReflectPhysicalDam;

	// Magic Bonuses
	char m_iSpellChanneling;
	char m_iFasterCasting;
	char m_iFasterCastRecovery;
	char m_iLowerManaCost;
	char m_iLowerReagentCost;
	char m_iIncreaseSpellDam;
	char m_iSpellFocusing;
	char m_iMageArmor;		// (Bool*)
	char m_iMageWeapon;
	char m_iCastingFocus;	// A percentage chance to resist interruptions while casting spells (capped at 12% from items), GM Inscription gives a 5% bonus. Cannot be imbued
	ushort m_iHitSpell;

	// Misc Weapons
	char m_iLowerAmmoCost;
	char m_iBalanced;
	char m_iVelocity;
	ushort m_iAbilityPrimary;
	ushort m_iAbilitySecondary;
	char m_iUseBestWeaponSkill;
	ullong m_iSlayerMisc;
	ullong m_iSlayerLesser;

	// Misc data
	char m_iLuck;
	char m_iSelfRepair;
	bool m_iNoDrop;
	bool m_iNoTrade;
	bool m_iBlessed;
	bool m_iCursed;
	char m_iWeightReduction;
	char m_iDurability;
	uchar m_iUsesRemaining;
	char m_iNightSight;		//(Bool*)
	char m_iBonusCrafting;
	char m_iBonusCraftingExcept;
	char m_iEnhancePotions;
	uchar m_iSkillBonus;
	uchar m_iArtifactRarity;
	bool m_iInsured;
	ullong m_iLifespan;
	dword m_dwOwnedBy;		// Storing UID
	char m_iPrized;
	RESOURCE_ID m_ridAlterItem;
	// Uncategorized (yet?)
	char m_iBattleLust;
	char m_iSearingWeapon;
	char m_iSplinteringWeapon;
	char m_iRageFocus;
	char m_iReactiveParalyze;
	char m_iBrittle;
	char m_iLavaInfused;
	char m_iManaPhase;
	char m_iDamModifier;
	char m_iEphemeral;
	char m_iManaBurst;
	char m_iRecharge;
	char m_iBloodDrinker;
	char m_iBane;
	char m_iIncreaseKarmaLoss;
	char m_iMassive;
	char m_iUnwieldy;
	char m_iAntique;
public:
//Retrieving values.

// Elemental Resistances
	char GetResPhysical();
	char GetResFire();
	char GetResCold();
	char GetResPoison();
	char GetResEnergy();
	char GetResPhysicalMax();
	char GetResFireMax();
	char GetResColdMax();
	char GetResPoisonMax();
	char GetResEnergyMax();

//Setting values.

// Elemental Resistances
	void SetResPhysical(char iVal);
	void SetResFire(char iVal);
	void SetResCold(char iVal);
	void SetResPoison(char iVal);
	void SetResEnergy(char iVal);
	void SetResPhysicalMax(char iVal);
	void SetResFireMax(char iVal);
	void SetResColdMax(char iVal);
	void SetResPoisonMax(char iVal);
	void SetResEnergyMax(char iVal);


};

#endif // _INC_PROPERTIES_H
