/*
todo: uppdatera class-check vid varje puls eller nåt?

todo: wishka kan bara kastas av en shaman i gruppen, så visa bara den som missing om vi har en shm i gruppen
todo: visa inte SUMMER som missing buff om COLD resist redan är cappat, vilket det e för mej
*/

#include "../MQ2Plugin.h"
//#include "../MQ2Cast/MQ2Cast.h"
#ifdef ISXEQ
#define ISINDEX() (argc>0)
#define ISNUMBER() (IsNumber(argv[0]))
#define GETNUMBER() (atoi(argv[0]))
#define GETFIRST()   argv[0]
#else
#define ISINDEX() (Index[0])
#define ISNUMBER() (IsNumber(Index))
#define GETNUMBER() (atoi(Index))
#define GETFIRST() Index
#endif

VOID TargetByHP(PSPAWNINFO pChar, PCHAR szLine);
VOID AssistTarget(PSPAWNINFO pChar, PCHAR szLine);
VOID AssistSmart(PSPAWNINFO pChar, PCHAR szLine);
VOID ShieldLowest(PSPAWNINFO pChar, PCHAR szLine);
VOID ReplyTarget(PSPAWNINFO pChar, PCHAR szLine);
VOID HealGroup(PSPAWNINFO pChar, PCHAR szLine);

void Heal(PSPAWNINFO spawn, int pct);

CHAR Buffer[MAX_STRING] = {0};               // Buffer for String manipulatsion
CHAR szName[MAX_STRING] = {0};               // Buffer for String manipulatsion

PreSetup("MQ2Krust");
PLUGIN_VERSION(1.0);

class MQ2KrustType *pKrustType=0;

class MQ2KrustType : public MQ2Type
{
private:
	char Temp[MAX_STRING];

	bool HasBST, HasCLR, HasDRU, HasSHD, HasSHM, HasENC, HasPAL, HasMAG, HasNEC, HasRNG;

	bool HasAego, HasSymbol, HasWOV, HasSpellhaste, HasSkin, HasPoS, HasMask, HasMammoth;
	bool HasCrack, HasGoD, HasSE, HasSV, HasBrells, HasForesight, HasFortitude, HasWishka;
	bool HasHunter, HasGuard, HasPred, HasATK, HasEye, HasFocus, HasMageConvert;
	bool HasJasinth, HasCorrupt, HasLife, HasSummer, HasWinter, HasHaste, HasDS;
	bool HasArmor, HasProc, HasMageRegen, HasNone;

	bool CasterClass, MeleeClass, HybridClass, TankClass, HealerClass;

	bool HasBuff(char *name)
	{
		for (int n=0; n<35; n++)
		{
			if (PSPELL pSpell=GetSpellByID(GetCharInfo2()->Buff[n].SpellID))
			{
				if 
					((!stricmp(name,pSpell->Name) || 
					strstr(pSpell->Name,"Rk. II") || 
					strstr(pSpell->Name,"Rk. III") || 
					strstr(pSpell->Name," II") || 
					strstr(pSpell->Name," III")) && 
					!strnicmp(name,pSpell->Name,strlen(pSpell->Name)-8)
					) return true;
					//(!stricmp(name,pSpell->Name)
					//) return true;
			} 
		}
		return false;
	}

	void InitClassCheck()
	{
		HasBST = false, HasCLR = false, HasDRU = false, HasSHD = false, HasSHM = false;
		HasENC = false, HasPAL = false, HasMAG = false, HasNEC = false, HasRNG = false;

		HasAego = false, HasSymbol = false, HasWOV = false, HasSpellhaste = false;
		HasSkin = false, HasPoS = false, HasMask = false, HasMammoth = false;
		HasCrack = false, HasGoD = false, HasSE = false, HasSV = false;
		HasBrells = false, HasForesight = false, HasFortitude = false, HasWishka = false;
		HasHunter = false, HasGuard = false, HasPred = false, HasATK = false, HasEye = false;
		HasJasinth = false, HasCorrupt = false, HasLife = false, HasDS = false;
		HasSummer = false, HasWinter = false, HasFocus = false, HasHaste = false;
		HasArmor = false, HasProc = false, HasMageConvert = false, HasNone = false;
		HasMageRegen = false;

		PCHARINFO pChInfo = GetCharInfo();

		if (pRaid && pRaid->RaidMemberCount)
		{
			//check raid classes
			//WriteChatf("In raid with %d members", pRaid->RaidMemberCount);

			for (DWORD index=0; index<pRaid->RaidMemberCount; index++)
			{
				switch ((PlayerClass) pRaid->RaidMember[index].nClass)
				{
					case Beastlord:		HasBST = true; break;
					case Cleric:		HasCLR = true; break;
					case Druid:			HasDRU = true; break;
					case Shadowknight:	HasSHD = true; break;
					case Shaman:		HasSHM = true; break;
					case Enchanter:		HasENC = true; break;
					case Paladin:		HasPAL = true; break;
					case Mage:			HasMAG = true; break;
					case Necromancer:	HasNEC = true; break;
					case Ranger:		HasRNG = true; break;
				}
			}

		}
		else if (pChInfo->pGroupInfo)
		{
			for (int index=0; index<5; index++)
			{ 
				if (PSPAWNINFO pMember = GetGroupMember(index))
				switch ((PlayerClass) pMember->Class)
				{	//check group classes
					case Beastlord:		HasBST = true; break;
					case Cleric:		HasCLR = true; break;
					case Druid:			HasDRU = true; break;
					case Shadowknight:	HasSHD = true; break;
					case Shaman:		HasSHM = true; break;
					case Enchanter:		HasENC = true; break;
					case Paladin:		HasPAL = true; break;
					case Mage:			HasMAG = true; break;
					case Necromancer:	HasNEC = true; break;
					case Ranger:		HasRNG = true; break;
				}
			}
		}

		switch ((PlayerClass) pChInfo->pSpawn->Class)
		{
			case Cleric:		HasCLR = true; break;
			case Druid:			HasDRU = true; break;
			case Shaman:		HasSHM = true; break;
			case Mage:			HasMAG = true; break;
			case Enchanter:		HasENC = true; break;
			case Necromancer:	HasNEC = true; break;
			case Wizard:		break;
			case Warrior:		break;
			case Shadowknight:	HasSHD = true; break;
			case Paladin:		HasPAL = true; break;
			case Beastlord:		HasBST = true; break;
			case Ranger:		HasRNG = true; break;
			case Bard:			break;
			case Monk:			break;
			case Rogue:			break;
			case Berserker:		break;
		}
	}

	/******************************

		HP & MELEE BUFFS

	******************************/
	void UpdateMeleeBuffs(MQ2TYPEVAR &Dest)
	{
		PCHARINFO	pChInfo = GetCharInfo();

		Dest.Type=pStringType; 
		Dest.Ptr=&Temp[0];
		Temp[0] = 0;

		InitClassCheck();

		if (HasDRU &&
			// List of Druid HP/Mana Buffs
			!HasBuff("Shieldstone Skin")			&& !HasBuff("Shieldstone Blessing") &&				//DRU (105)
			!HasBuff("Granitebark Skin")			&& !HasBuff("Granitebark Blessing") &&				//DRU (100)
			!HasBuff("Stonebark Skin")				&& !HasBuff("Stonebark Blessing") &&				//DRU (95)
			!HasBuff("Timbercore Skin")				&& !HasBuff("Blessing of the Timbercore") &&		//DRU (90)
			!HasBuff("Heartwood Skin")				&& !HasBuff("Blessing of the Heartwood") &&			//DRU (85)
			!HasBuff("Ironwood Skin")				&& !HasBuff("Blessing of the Ironwood") &&			//DRU (80)
			!HasBuff("Direwild Skin")				&& !HasBuff("Blessing of the Direwild") &&			//DRU (75)
			!HasBuff("Steeloak Skin")				&& !HasBuff("Blessing of Steeloak") &&				//DRU (70)
			!HasBuff("Blessing of the Nine")		&& !HasBuff("Protection of the Nine") &&			//DRU (65)
			!HasBuff("Protection of the Cabbage")	&& !HasBuff("Protection of the Glades") &&			//DRU (60)
			// List of Cleric Aego Buffs
			!HasBuff("Surety") &&																		//CLR (105)
			!HasBuff("Certitude")					&& !HasBuff("Hand of Certitude") &&					//CLR (100)
			!HasBuff("Credence")					&& !HasBuff("Hand of Credence") &&					//CLR (95)
			!HasBuff("Reliance")					&& !HasBuff("Hand of Reliance") &&					//CLR (90)
			!HasBuff("Gallantry")					&& !HasBuff("Hand of Gallantry") &&					//CLR (85)
			!HasBuff("Temerity")					&& !HasBuff("Hand of Temerity") &&					//CLR (80)
			!HasBuff("Tenacity")					&& !HasBuff("Hand of Tenacity") &&					//CLR (75)
			!HasBuff("Conviction")					&& !HasBuff("Hand of Conviction") &&				//CLR (70)
			!HasBuff("Virtue")						&& !HasBuff("Hand of Virtue") &&					//CLR (65)
			!HasBuff("Aegolism")					&& !HasBuff("Blessing of Aegolism") &&				//CLR (60)
			!HasBuff("Temperance")					&& !HasBuff("Blessing of Temperance"))				//CLR (45)
		{
			strcat(Temp, "SKIN ");
		}

		if (HasCLR && HasDRU &&
			// Cleric Symbol spells (single and group)
			!HasBuff("Symbol of Nonia")			&&													//CLR (105)
			!HasBuff("Symbol of Gezat")			&&													//CLR (100)
			!HasBuff("Symbol of the Triumvirate") &&												//CLR (95)
			!HasBuff("Symbol of Ealdun")		&& !HasBuff("Ealdun's Mark") &&						//CLR (90)
			!HasBuff("Symbol of Darianna")		&& !HasBuff("Darianna's Mark") &&					//CLR (85)
			!HasBuff("Symbol of Kaerra")		&& !HasBuff("Kaerra's Mark") &&						//CLR (80)
			!HasBuff("Symbol of Elushar")		&& !HasBuff("Elushar's Mark") &&					//CLR (75)
			!HasBuff("Symbol of Balikor")		&& !HasBuff("Balikor's Mark") &&					//CLR (70)
			!HasBuff("Symbol of Kazad")			&& !HasBuff("Kazad`s Mark") &&						//CLR (65)
			!HasBuff("Symbol of Marzin")		&& !HasBuff("Marzin`s Mark") &&						//CLR (60)
			!HasBuff("Symbol of Naltron")		&& !HasBuff("Naltron`s Mark") &&					//CLR (58)
			// Paladin Symbol spells (Single and group)
			!HasBuff("Symbol of Niparson")		&&													//PAL (105)
			!HasBuff("Symbol of Burim")			&&													//PAL (100)
			!HasBuff("Symbol of Erillion")		&&													//PAL (95)
			!HasBuff("Symbol of Jyleel")		&&													//PAL (90)
			!HasBuff("Symbol of Jeneca")		&& !HasBuff("Jeneca's Mark") &&						//PAL (85)
			!HasBuff("Symbol of Bthur")			&& !HasBuff("Bthur's Mark") &&						//PAL (80)
			!HasBuff("Symbol of Fenegar")		&& !HasBuff("Fenegar's Mark") &&					//PAL (75)
			!HasBuff("Symbol of Jeron")			&& !HasBuff("Jeron's Mark") &&						//PAL (70)
			// Cleric Aego spells
			!HasBuff("Surety")					&&													//CLR (105)
			!HasBuff("Certitude")				&&													//CLR (100)
			!HasBuff("Credence")				&&													//CLR (95)
			!HasBuff("Reliance")				&& !HasBuff("Hand of Reliance") &&					//CLR (90)
			!HasBuff("Gallantry")				&& !HasBuff("Hand of Gallantry") &&					//CLR (85)
			!HasBuff("Temerity")				&& !HasBuff("Hand of Temerity") &&					//CLR (80)
			!HasBuff("Tenacity")				&& !HasBuff("Hand of Tenacity") &&					//CLR (75)
			!HasBuff("Conviction")				&& !HasBuff("Hand of Conviction") &&				//CLR (70)
			!HasBuff("Virtue")					&& !HasBuff("Hand of Virtue") &&					//CLR (65)
			!HasBuff("Aegolism")				&& !HasBuff("Blessing of Aegolism") &&				//CLR (60)
			!HasBuff("Temperance")				&& !HasBuff("Blessing of Temperance"))				//CLR (45)
			// Paladin Aego spells
			// need to add
		{
			strcat(Temp, "SYMBOL ");
		}

		if (!HasDRU && HasCLR &&
			// Druid Buffs 
			!HasBuff("Shieldstone Skin")			&& !HasBuff("Shieldstone Blessing") &&				//DRU (105)
			!HasBuff("Granitebark Skin")			&& !HasBuff("Granitebark Blessing") &&				//DRU (100)
			!HasBuff("Stonebark Skin")				&& !HasBuff("Stonebark Blessing") &&				//DRU (95)
			!HasBuff("Timbercore Skin")				&& !HasBuff("Blessing of the Timbercore") &&		//DRU (90)
			!HasBuff("Heartwood Skin")				&& !HasBuff("Blessing of the Heartwood") &&			//DRU (85)
			!HasBuff("Ironwood Skin")				&& !HasBuff("Blessing of the Ironwood") &&			//DRU (80)
			!HasBuff("Direwild Skin")				&& !HasBuff("Blessing of the Direwild") &&			//DRU (75)
			!HasBuff("Steeloak Skin")				&& !HasBuff("Blessing of Steeloak") &&				//DRU (70)
			!HasBuff("Blessing of the Nine")		&& !HasBuff("Protection of the Nine") &&			//DRU (65)
			!HasBuff("Protection of the Cabbage")	&& !HasBuff("Protection of the Glades") &&			//DRU (60)
			// Cleric Symbol spells (single and group)
			!HasBuff("Symbol of Nonia")				&&													//CLR (105)
			!HasBuff("Symbol of Gezat")				&&													//CLR (100)
			!HasBuff("Symbol of the Triumvirate") &&													//CLR (95)
			!HasBuff("Symbol of Ealdun")			&& !HasBuff("Ealdun's Mark") &&						//CLR (90)
			!HasBuff("Symbol of Darianna")			&& !HasBuff("Darianna's Mark") &&					//CLR (85)
			!HasBuff("Symbol of Kaerra")			&& !HasBuff("Kaerra's Mark") &&						//CLR (80)
			!HasBuff("Symbol of Elushar")			&& !HasBuff("Elushar's Mark") &&					//CLR (75)
			!HasBuff("Symbol of Balikor")			&& !HasBuff("Balikor's Mark") &&					//CLR (70)
			!HasBuff("Symbol of Kazad")				&& !HasBuff("Kazad`s Mark") &&						//CLR (65)
			!HasBuff("Symbol of Marzin")			&& !HasBuff("Marzin`s Mark") &&						//CLR (60)
			!HasBuff("Symbol of Naltron")			&& !HasBuff("Naltron`s Mark") &&					//CLR (58)
			// Cleric Aego spells
			!HasBuff("Surety")					&&													//CLR (105)
			!HasBuff("Certitude")					&&													//CLR (100)
			!HasBuff("Credence")					&&													//CLR (95)
			!HasBuff("Reliance")					&& !HasBuff("Hand of Reliance") &&					//CLR (90)
			!HasBuff("Gallantry")					&& !HasBuff("Hand of Gallantry") &&					//CLR (85)
			!HasBuff("Temerity")					&& !HasBuff("Hand of Temerity") &&					//CLR (80)
			!HasBuff("Tenacity")					&& !HasBuff("Hand of Tenacity") &&					//CLR (75)
			!HasBuff("Conviction")					&& !HasBuff("Hand of Conviction") &&				//CLR (70)
			!HasBuff("Virtue")						&& !HasBuff("Hand of Virtue") &&					//CLR (65)
			!HasBuff("Aegolism")					&& !HasBuff("Blessing of Aegolism") &&				//CLR (60)
			!HasBuff("Temperance")					&& !HasBuff("Blessing of Temperance"))				//CLR (45)
		{
			strcat(Temp, "TEME ");
		}

		if ((HasPAL || HasBST || HasRNG) &&
			// Pally Buffs
			!HasBuff("Brell's Stalwart Bulwark") &&		// PAL (105)
			!HasBuff("Brell's Steadfast Bulwark") &&	// PAL (100)
			!HasBuff("Brell's Adamantine Armor") &&		// PAL (95)
			!HasBuff("Brell's Tellurian Rampart") &&	// PAL (90)
			!HasBuff("Brell's Loamy Ward") &&			// PAL (85)
			!HasBuff("Brell's Earthen Aegis") &&		// PAL (80)
			!HasBuff("Brell's Stony Guard") &&			// PAL (75)
			!HasBuff("Brell's Brawny Bulwark") &&		// PAL (70)
			!HasBuff("Brell's Stalwart Shield") &&		// PAL (65)
			!HasBuff("Brell's Mountainous Barrier") &&	// PAL (60)
			!HasBuff("Brell's Steadfast Aegis") &&		// PAL (50)
			// Beastlord Buffs
			!HasBuff("Spiritual Vivification") &&		// BST (105)
			!HasBuff("Spiritual Vindication") &&		// BST (100)
			!HasBuff("Spiritual Valiance") &&			// BST (95)
			!HasBuff("Spiritual Vivacity") &&			// BST (80)
			!HasBuff("Spiritual Verve") &&				// BST (85)
			!HasBuff("Spiritual Vim") &&				// BST (75)
			!HasBuff("Spiritual Vitality") &&			// BST (70)
			!HasBuff("Spiritual Vigor") &&				// BST (65)
			!HasBuff("Spiritual Strength") &&			// BST (60)	
			// Ranger Buffs
			!HasBuff("Strength of the Copsestalker") &&		// RNG (105)
			!HasBuff("Strength of the Bosquestalker") &&	// RNG (100)
			!HasBuff("Strength of the Gladetender") &&		// RNG (95)
			!HasBuff("Strength of the Tracker") &&			// RNG (90)
			!HasBuff("Strength of the Thicket Stalker") &&	// RNG (85)
			!HasBuff("Protection of the Kirkoten") &&		// RNG (80)
			!HasBuff("Strength of the Gladewalker") &&		// RNG (80)
			!HasBuff("Strength of the Forest Stalker") &&	// RNG (75)
			!HasBuff("Strength of the Hunter") &&			// RNG (70)
			!HasBuff("Ward of the Hunter") &&				// RNG (70)
			!HasBuff("Strength of Tunare")) 				// RNG (65)
		{
			if (pChInfo->pSpawn->Class == 4) { //ranger(4)
				strcat(Temp, "PotM ");
			} else if (HasPAL) {
				strcat(Temp, "BRELLS ");
			} else if (HasBST && pChInfo->pSpawn->Class != 2) {//skip cleric(2), Yaulp dont stack with SV
				strcat(Temp, "SV ");
			} else if (HasRNG) {
				strcat(Temp, "SotGW ");
			}
		}


		//4 = Ranger
		if (pChInfo->pSpawn->Class == 4)
		{	
			if (
			!HasBuff("Strength of the Copsestalker") &&		// RNG (105)
			!HasBuff("Strength of the Bosquestalker") &&	// RNG (100)
			!HasBuff("Strength of the Gladetender") &&		// RNG (95)
			!HasBuff("Strength of the Tracker") &&			// RNG (90)
			!HasBuff("Strength of the Thicket Stalker") &&	// RNG (85)
			!HasBuff("Strength of the Gladewalker") &&		// RNG (80)
			!HasBuff("Strength of the Forest Stalker") &&	// RNG (75)
			!HasBuff("Strength of the Hunter") &&			// RNG (70)
			!HasBuff("Aura of Rage") &&						//Clicky?
			!HasBuff("Nature's Precision"))					//???
			{
				strcat(Temp, "ATK ");
			}

			if (
				!HasBuff("Eyes of the Harrier") &&		//RNG (105)
				!HasBuff("Eyes of the Howler") &&		//RNG (100)
				!HasBuff("Eyes of the Raptor") &&		//RNG (95)
				!HasBuff("Eyes of the Wolf") &&			//RNG (90)
				!HasBuff("Eyes of the Nocturnal") &&	//RNG (85)
				!HasBuff("Eyes of the Peregrine") &&	//RNG (80)
				!HasBuff("Eyes of the Owl") &&			//RNG (75)
				!HasBuff("Eagle Eye"))					//RNG (60)
			{
				strcat(Temp, "EYE ");
			}

			if (
				!HasBuff("Squall of Blades") &&		//RNG (105)
				!HasBuff("Deafening Edges") &&		//RNG (100)
				!HasBuff("Jolting Impact") &&		//RNG (95)
				!HasBuff("Crackling Edges") &&		//RNG (90)
				!HasBuff("Devastating Edges") &&	//RNG (89)
				!HasBuff("Jolting Edges") &&		//RNG (87)
				!HasBuff("Crackling Blades") &&		//RNG (85)
				!HasBuff("Devastating Blades") &&	//RNG (84)
				!HasBuff("Jolting Swings") &&		//RNG (82)
				!HasBuff("Deafening Blades") &&		//RNG (80)
				!HasBuff("Jolting Strikes") &&		//RNG (77)
				!HasBuff("Thundering Blades") &&	//RNG (75)
				!HasBuff("Call of Lightning") &&	//RNG (70)
				!HasBuff("Sylvan Call") &&			//RNG (65)
				!HasBuff("Cry of Thunder") &&		//RNG (65)
				!HasBuff("Jolting Blades")			//RNG (54)
				)
			{
				strcat(Temp, "PROC ");
			}

			if (!HasBuff("Consumed by the Hunt"))
			{
				strcat(Temp, "CbtH ");
			}
		} 

		if ((HasSHM || HasBST) &&
			!HasBuff("Doomscale Focusing") &&													//SHM (105)
			!HasBuff("Insistent Focusing") &&													//SHM (100)
			!HasBuff("Imperative Focusing") &&													//SHM (95)
			!HasBuff("Exigent Focusing") &&														//SHM (90)
			!HasBuff("Darkpaw Focusing") &&			!HasBuff("Talisman of the Darkpaw") &&		//SHM (85)
			!HasBuff("Bloodworg Focusing") &&		!HasBuff("Talisman of the Bloodworg") &&	//SHM (80)
			!HasBuff("Dire Focusing") &&			!HasBuff("Talisman of the Dire") &&			//SHM (75)
			!HasBuff("Wunshi's Focusing") &&		!HasBuff("Talisman of Wunshi") &&			//SHM (70)
			!HasBuff("Focus of the Seventh") &&		!HasBuff("Focus of Soul") &&				//SHM (65)
			// Beastlord Buffs
			!HasBuff("Focus of Okasi") &&			//BST (105)
			!HasBuff("Focus of Sanera") &&			//BST (100)
			!HasBuff("Focus of Klar") &&			//BST (95)
			!HasBuff("Focus of Emiq") &&			//BST (90)
			!HasBuff("Focus of Yemall") &&			//BST (85)
			!HasBuff("Focus of Zott") &&			//BST (80)
			!HasBuff("Focus of Amilan") &&			//BST (75)
			!HasBuff("Focus of Alladnu") &&			//BST (70)
			!HasBuff("Talisman of Kragg") &&		//BST (65)
			//Caster Shield Buffs
			!HasBuff("Shield of the Pellarus") &&		//CST (105)
			!HasBuff("Shield of the Dauntless") &&		//CST (100)
			!HasBuff("Shield of Bronze") &&				//CST (95)
			!HasBuff("Shield of Dreams") &&				//CST (90)
			!HasBuff("Shield of the Void") &&			//CST (85)
			!HasBuff("Prime Guard") &&					//MAG (80)
			!HasBuff("Bulwark of the Crystalwing") &&	//WIZ (80)
			!HasBuff("Spellbound Shield") &&			//ENC (80)
			!HasBuff("Bulwark of Shadows") &&			//NEC (80)
			!HasBuff("Prime Shielding") &&				//MAG (75)
			!HasBuff("Shield of the Crystalwing") &&	//WIZ (75)
			!HasBuff("Sorcerous Shield") &&				//ENC (75)
			!HasBuff("Shield of Darkness") &&			//NEC (75)
			!HasBuff("Elemental Aura") &&				//MAG (70)
			!HasBuff("Ether Shield") &&					//WIZ (70)
			!HasBuff("Mystic Shield") &&				//ENC (70)
			!HasBuff("Shadow Guard") &&					//NEC (70)
			!HasBuff("Shield of Maelin") &&				//CST (65)
			!HasBuff("Shield of the Arcane") &&			//CST (60)
			!HasBuff("Armor of the Zealous") &&			//CLR (95)
			!HasBuff("Armor of the Earnest") &&			//CLR (90)
			!HasBuff("Armor of the Devout") &&			//CLR (85)
			!HasBuff("Armor of the Solemn") &&			//CLR (80)
			!HasBuff("Armor of the Sacred") &&			//CLR (75)
			!HasBuff("Armor of the Pious") &&			//CLR (70)
			!HasBuff("Armor of the Zealot"))			//CLR (65)
			// Need to figure Paladin Buffs
		{
			strcat(Temp, "FOCUS ");
		}

		if ((HasENC || HasSHM || HasBST) && !CasterClass &&
			//Shaman
			!HasBuff("Celerity") &&						!HasBuff("Talisman of Celerity") &&						//SHM
			!HasBuff("Alacrity") &&						!HasBuff("Talisman of Alacrity") &&						//SHM
			!HasBuff("Swift like the Wind") &&			!HasBuff("Wonderous Rapidity") &&	//SHM
			//Enchanter
			!HasBuff("Speed of Prokev") &&				!HasBuff("Hastening of Prokev") &&		//ENC (105)
			!HasBuff("Speed of Sviir") &&				!HasBuff("Hastening of Sviir") &&		//ENC (100)
			!HasBuff("Speed of Aransir") &&				!HasBuff("Hastening of Aransir") &&		//ENC (95)
			!HasBuff("Speed of Novak") &&				!HasBuff("Hastening of Novak") &&		//ENC (90)
			!HasBuff("Speed of Erradien") &&			!HasBuff("Hastening of Erradien") &&	//ENC (80)
			!HasBuff("Speed of Yozan") &&				!HasBuff("Hastening of Ellowind") &&	//ENC (75)
			!HasBuff("Speed of Salik") &&				!HasBuff("Hastening of Salik") &&		//ENC (70)
			!HasBuff("Vallon's Quickening") &&			!HasBuff("Speed of Vallon") &&			//ENC (65)
			//Beastlord
			!HasBuff("Extraordinary Velocity") &&		//BST (100)
			!HasBuff("Exceptional Velocity") &&			//BST (95)
			!HasBuff("Incomparable Velocity") &&		//BST (90)
			!HasBuff("Twitching Speed") &&																				//???
			//Others
			!HasBuff("Elixir of Speed IX") &&			!HasBuff("Elixir of Speed X"))			//Potions
		{
			strcat(Temp, "HASTE ");
		}


		if ((HasMAG || HasDRU) && TankClass &&
			(
			// Druid DS
			!HasBuff("Daggerspur Bulwark") &&			!HasBuff("Legacy of Daggerspurs") &&	//Dru (105)
			!HasBuff("Spikethistle Bulwark") &&			!HasBuff("Legacy of Spikethistles") &&	//Dru (100)
			!HasBuff("Spineburr Bulwark") &&			!HasBuff("Legacy of Spineburrs") &&		//Dru (95)
			!HasBuff("Bonebriar Bulwark") &&			!HasBuff("Legacy of Bonebriar") &&		//Dru (90)
			!HasBuff("Brierbloom Bulwark") &&			!HasBuff("Legacy of Brierbloom") &&		//Dru (85)
			!HasBuff("Viridithorns Bulwark") &&			!HasBuff("Legacy of Viridithorns") &&	//DRU (80)
			!HasBuff("Viridifloral Shield") &&			!HasBuff("Legacy of Viridiflora") &&	//DRU (75)
			!HasBuff("Nettle Shield") &&				!HasBuff("Circle of Nettles") &&		//DRU
			// Mage DS
			!HasBuff("Flameweave") &&					!HasBuff("Circle of Flameweave") &&		//MAG (105)
			!HasBuff("Flameskin") &&					!HasBuff("Circle of Flameskin") &&		//MAG (100)
			!HasBuff("Embercoat") &&					!HasBuff("Circle of Embers") &&			//MAG (95)
			!HasBuff("Dreamfire Coat") &&				!HasBuff("Circle of Dreamfire") &&		//MAG (90)
			!HasBuff("Brimstoneskin") &&				!HasBuff("Circle of Brimstoneskin") &&	//MAG (85)
			!HasBuff("Lavaskin") &&						!HasBuff("Circle of Lavaskin") &&		//MAG (80)																						//MAG (SoF)
			!HasBuff("Magmaskin") &&					!HasBuff("Circle of Magmaskin") &&		//MAG (75)
			!HasBuff("Fireskin") &&						!HasBuff("Circle of Fireskin")			//MAG (70)
			))
		{
			strcat(Temp, "DS ");
		}

		if (HasRNG && TankClass &&
			!HasBuff("Cloak of Nettlespears")	&&	//RNG (105)
			!HasBuff("Cloak of Spurs")	&&			//RNG (100)
			!HasBuff("Cloak of Burrs")	&&			//RNG (95)
			!HasBuff("Cloak of Quills") &&			//RNG (90)
			!HasBuff("Shared Cloak of Burrs") &&	//RNG (90)
			!HasBuff("Cloak of Feathers") &&		//RNG (85)
			!HasBuff("Cloak of Scales") &&			//RNG (80)
			!HasBuff("Guard of the Earth"))			//
		{
			strcat(Temp, "CoS ");
		}

		if (HasRNG && !CasterClass && pChInfo->pSpawn->Class != 4 &&		//skip rangers(4)
			!HasBuff("Bellow of the Predator") &&	//RNG (105)
			!HasBuff("Shout of the Predator") &&	//RNG (100)
			!HasBuff("Cry of the Predator") &&		//RNG (95)
			!HasBuff("Roar of the Predator") &&		//RNG (90)
			!HasBuff("Yowl of the Predator") &&		//RNG (85)
			!HasBuff("Gnarl of the Predator") &&	//RNG (80)
			!HasBuff("Snarl of the Predator") &&	//RNG (75)
			!HasBuff("Howl of the Predator") &&		//RNG (70)
			!HasBuff("Call of the Predator") &&		//RNG (60)
			!HasBuff("Spirit of the Predator"))		//RNG (65)
		{
			strcat(Temp, "PREDATOR ");		//ATK buff
		}

		if ((HasSHM || HasDRU) && !CasterClass &&
			!HasBuff("Champion") &&
			!HasBuff("Mammoth's Force")&&
			!HasBuff("Mammoth's Strength")&&
			!HasBuff("Lion's Strength") &&							//DRU
			!HasBuff("Talisman of Might") &&
			!HasBuff("Spirit of Might"))									//SHM
		{
			strcat(Temp, "MAMMOTH ");
		}

		if (HasSHM &&
			!HasBuff("Preeminent Foresight") &&
			!HasBuff("Preternatural Foresight") &&
			!HasBuff("Preeminent Foresight") &&
			!HasBuff("Talisman of Foresight") &&		//SHM
			!HasBuff("Spirit of Sense") &&
			!HasBuff("Talisman of Sense") &&									//SHM
			!HasBuff("Transcendent Foresight"))
		{
			strcat(Temp, "FORESIGHT ");
		}

		if (HasSHM &&
			!HasBuff("Spirit of the Faithful") &&		//SHM (105)
			!HasBuff("Spirit of Dauntlessness") &&		//SHM (100)
			!HasBuff("Spirit of Resolve") &&			//SHM (95)
			!HasBuff("Spirit of Valor") &&				//SHM (90)
			!HasBuff("Spirit of Determination") &&		//SHM (85)
			!HasBuff("Spirit of Vehemence") &&			//SHM (80)
			!HasBuff("Talisman of Vehemence") &&		//SHM (80)																			//SHM (SoF)
			!HasBuff("Spirit of Persistence") &&		//SHM (75)
			!HasBuff("Talisman of Persistence") &&		//SHM (75)
			!HasBuff("Spirit of Fortitude") &&			//SHM (70)
			!HasBuff("Talisman of Fortitude")			//SHM (70)
			)
		{
			strcat(Temp, "FORTITUDE ");
		}

		if (HasCLR && TankClass &&
			(
				!HasBuff("Ward of the Ardent")   &&												//CLR (105)
				!HasBuff("Ward of the Reverent")   &&											//CLR (100)
				!HasBuff("Ward of the Zealous")   &&											//CLR (95)
				!HasBuff("Ward of the Earnest")   &&	!HasBuff("Order of the Earnest") &&		//CLR (90)
				!HasBuff("Ward of the Devout")   &&		!HasBuff("Order of the Devout") &&		//CLR (85)
				!HasBuff("Ward of the Resolute") &&		!HasBuff("Order of the Resolute") &&	//CLR (80)
				!HasBuff("Ward of the Dauntless") &&											//CLR (75)
				!HasBuff("Ward of Valiance") &&													//CLR (70)
				!HasBuff("Ward of Gallantry")													//CLR (65)
			)
			&&
			(
			HasBuff("Symbol of Nonia")				||												//CLR (105)
			HasBuff("Symbol of Gezat")				||												//CLR (100)
			HasBuff("Symbol of the Triumvirate")	||												//CLR (95)
			HasBuff("Symbol of Ealdun")				|| HasBuff("Ealdun's Mark") ||					//CLR (90)
			HasBuff("Symbol of Darianna")			|| HasBuff("Darianna's Mark") ||				//CLR (85)
			HasBuff("Symbol of Kaerra")				|| HasBuff("Kaerra's Mark") ||					//CLR (80)
			HasBuff("Symbol of Elushar")			|| HasBuff("Elushar's Mark") ||					//CLR (75)
			HasBuff("Symbol of Balikor")			|| HasBuff("Balikor's Mark") ||					//CLR (70)
			HasBuff("Symbol of Kazad")				|| HasBuff("Kazad`s Mark") ||					//CLR (65)
			HasBuff("Symbol of Marzin")				|| HasBuff("Marzin`s Mark") ||					//CLR (60)
			HasBuff("Symbol of Naltron")			|| HasBuff("Naltron`s Mark") 					//CLR (58)
			)
			&&
			(
			// Druid Buffs
			HasBuff("Shieldstone Skin")				|| HasBuff("Shieldstone Blessing") ||			//DRU (105)
			HasBuff("Granitebark Skin")				|| HasBuff("Granitebark Blessing") ||			//DRU (100)
			HasBuff("Stonebark Skin")				|| HasBuff("Stonebark Blessing") ||				//DRU (95)
			HasBuff("Timbercore Skin")				|| HasBuff("Blessing of the Timbercore") ||		//DRU (90)
			HasBuff("Heartwood Skin")				|| HasBuff("Blessing of the Heartwood") ||		//DRU (85)
			HasBuff("Ironwood Skin")				|| HasBuff("Blessing of the Ironwood") ||		//DRU (80)
			HasBuff("Direwild Skin")				|| HasBuff("Blessing of the Direwild") ||		//DRU (75)
			HasBuff("Steeloak Skin")				|| HasBuff("Blessing of Steeloak") ||			//DRU (70)
			HasBuff("Blessing of the Nine")			|| HasBuff("Protection of the Nine") ||			//DRU (65)
			HasBuff("Protection of the Cabbage")	|| HasBuff("Protection of the Glades") 			//DRU (60)
			))
		{
			strcat(Temp, "WOV ");	//AC buff
		}

		if (pChInfo->pSpawn->Class == 2 &&		//cleric (2) 
			(
			!HasBuff("Armor of the Ardent") &&			//CLR (105)
			!HasBuff("Armor of the Reverent") &&		//CLR (100)
			!HasBuff("Armor of the Zealous") &&			//CLR (95)
			!HasBuff("Armor of the Earnest") &&			//CLR (90)
			!HasBuff("Armor of the Devout") &&			//CLR (85)
			!HasBuff("Armor of the Solemn") &&			//CLR (80)
			!HasBuff("Armor of the Sacred") &&			//CLR (75)
			!HasBuff("Armor of the Pious") &&			//CLR (70)
			!HasBuff("Armor of the Zealot")				//CLR (65)
			) && (
			HasBuff("Surety")			||													//CLR (100)
			HasBuff("Certitude")			||													//CLR (100)
			HasBuff("Credence")				|| HasBuff("Hand of Credence") ||					//CLR (95)
			HasBuff("Reliance")				|| HasBuff("Hand of Reliance") ||					//CLR (90)
			HasBuff("Gallantry")			|| HasBuff("Hand of Gallantry") ||					//CLR (85)
			HasBuff("Temerity")				|| HasBuff("Hand of Temerity") ||					//CLR (80)
			HasBuff("Tenacity")				|| HasBuff("Hand of Tenacity") ||					//CLR (75)
			HasBuff("Conviction")			|| HasBuff("Hand of Conviction") ||					//CLR (70)
			HasBuff("Virtue")				|| HasBuff("Hand of Virtue") ||						//CLR (65)
			HasBuff("Aegolism")				|| HasBuff("Blessing of Aegolism") ||				//CLR (60)
			HasBuff("Temperance")			|| HasBuff("Blessing of Temperance")				//CLR (45)
			))
		{
			strcat(Temp, "ARMOR ");
		}

		//todo: ${Zone.ShortName.NotEqual[Tacvi]}
		if ((pChInfo->pSpawn->Class == 4 || pChInfo->pSpawn->Class == 6) &&		//ranger(4), druid(6)
			!HasBuff("Spurcoat") && 
			!HasBuff("Viridithorn Coat") && 
			!HasBuff("Viridicoat") && 
			!HasBuff("Nettlecoat") && 
			!HasBuff("Briarcoat"))
		{
			strcat(Temp, "COAT ");
		}

		if (!strlen(Temp)) {
			sprintf(Temp, "OK");
		}
	}

	/******************************

		Caster buffs (C6+SA+spell haste)

	******************************/
	void UpdateCasterBuffs(MQ2TYPEVAR &Dest)
	{
		PCHARINFO	pChInfo = GetCharInfo();

		Dest.Type=pStringType; 
		Dest.Ptr=&Temp[0];
		Temp[0] = 0;

		InitClassCheck();
		InitMyClassCheck();

		if (MeleeClass) {
			sprintf(Temp, "OK");
			return;
		}

		if (HasCLR &&
			!HasBuff("Benediction of Piety") &&		!HasBuff("Hand of Zeal") &&				//CLR (100)
			!HasBuff("Blessing of Fervor") &&		!HasBuff("Hand of Fervor") &&			//CLR (100)
			!HasBuff("Blessing of Assurance") &&	!HasBuff("Hand of Assurance") &&		//CLR (95)
			!HasBuff("Blessing of Will") &&			!HasBuff("Hand of Will") && 			//CLR (90)
			!HasBuff("Aura of Loyalty") &&			!HasBuff("Blessing of Loyalty") &&		//CLR (85)
			!HasBuff("Aura of Resolve") &&			!HasBuff("Blessing of Resolve") && 		//CLR (80)
			!HasBuff("Aura of Purpose") &&			!HasBuff("Blessing of Purpose") &&		//CLR (75)
			!HasBuff("Aura of Devotion") &&			!HasBuff("Blessing of Devotion") &&		//CLR (70)
			!HasBuff("Aura of Reverence") &&		!HasBuff("Blessing of Reverence"))		//CLR (65)
		{
			strcat(Temp, "SPELLHASTE ");
		}

		if (HasBST &&
			!HasBuff("Spiritual Elaboration") &&	//BST (105)
			!HasBuff("Spiritual Evolution") &&		//BST (100)
			!HasBuff("Spiritual Enrichment") &&		//BST (95)
			!HasBuff("Spiritual Enhancement") &&	//BST (90)
			!HasBuff("Spiritual Epiphany") &&		//BST (80)
			!HasBuff("Spiritual Edification") &&	//BST (85)
			!HasBuff("Spiritual Enlightenment") &&	//BST (75)
			!HasBuff("Spiritual Ascendance") &&		//BST (70)
			!HasBuff("Spiritual Dominion"))			//BST (65)
		{
			strcat(Temp, "SE ");
		}

		if (HasENC &&
			!HasBuff("Precognition") &&					!HasBuff("Voice of Precognition") &&	//ENC (105)
			!HasBuff("Foresight") &&					!HasBuff("Voice of Foresight") &&		//ENC (100)
			!HasBuff("Premeditation") &&				!HasBuff("Voice of Premeditation") &&	//ENC (95)
			!HasBuff("Forethought ") &&					!HasBuff("Voice of Forethought") &&		//ENC (90)
			!HasBuff("Prescience") &&					!HasBuff("Voice of Prescience") &&		//ENC (85)
			!HasBuff("Seer's Cognizance") &&			!HasBuff("Voice of Cognizance") &&		//ENC (80)
			!HasBuff("Seer's Intuition") &&				!HasBuff("Voice of Intuition") &&		//ENC (75)
			!HasBuff("Clairvoyance") &&					!HasBuff("Voice of Clairvoyance") &&	//ENC (70)
			!HasBuff("Quellious") &&					!HasBuff("Voice of Tranquility") &&		//ENC (65)
			!HasBuff("Koadic's Endless Intellect") && 											//ENC (60)
			//Others
			!HasBuff("Elixir of Clarity X") &&			!HasBuff("Elixir of Clarity XI") && // Potions
			!HasBuff("Elixir of Clarity XII"))
		{
			strcat(Temp, "CRACK ");
		}

		if ((pChInfo->pSpawn->Class == 4 || pChInfo->pSpawn->Class == 6) &&		//ranger(4), druid(6)
			!HasBuff("Mask of the Copsetender") && //DRU (SoF)
			!HasBuff("Mask of the Bosquetender") && //DRU (SoF)
			!HasBuff("Mask of the Shadowcat") && //DRU (SoF)
			!HasBuff("Eyes of the Peregrine") && //RNG (SoF)
			!HasBuff("Mask of the Wild") && 
			!HasBuff("Eyes of the Owl") &&
			!HasBuff("Mask of the Forest") && 
			!HasBuff("Mask of the Stalker"))
		{
			strcat(Temp, "MASK ");
		}

		if ((pChInfo->pSpawn->Class == 6 || pChInfo->pSpawn->Class == 10)		//Druid(6), Shaman(10)
				&& pChInfo->pSpawn->Level>=75 &&
			!HasBuff("Preincarnation") &&
			!HasBuff("Second Life")
			)
		{
			strcat(Temp, "2LIFE ");
			//todo: Shaman gets this spell (cleric?)
		}

		//todo: check aura!
		//if ((${Me.Class.Name.Equal["Druid"]} && ${Me.Level}>=70) && !${Me.Song["Aura of Life Effect"].ID}) /varset str ${str} AURA

		if (!strlen(Temp)) {
			sprintf(Temp, "OK");
		}
	}

	/******************************

		RESIST BUFFS

	******************************/
	void UpdateResistBuffs(MQ2TYPEVAR &Dest)
	{
		PCHARINFO	pChInfo = GetCharInfo();

		Dest.Type=pStringType; 
		Dest.Ptr=&Temp[0];
		Temp[0] = 0;

		InitClassCheck();

		//fixme: läs in zoneinfo från INI
		///declare ZoneAllowsLevitate int local ${Ini[ZONEINFO_FILE, ZoneInfo, ${Zone.ShortName}_Levitate, 0]}
		//if (${HasNEC} && ${ZoneAllowsLevitate} && !${Me.Buff["Dead Men Floating"].ID} && !${Me.Buff["Dead Man Floating"].ID}) /varset str ${str} DMF


		if (HasENC && !HasBuff("Guard of Druzzil"))
		{
			strcat(Temp, "GoD ");
		}

		if (HasDRU && !HasBuff("Protection of Seasons"))
		{
			strcat(Temp, "PoS ");
		}

		if ((HasDRU || HasRNG) && pRaid && pRaid->RaidMemberCount && !HasBuff("Circle of Summer"))
		{
			strcat(Temp, "SUMMER ");
		}

		//Wishka har CORRUPT resist komponent, den behövs i AG/FC raids
		if ((HasSHM || HasBST) &&
			!HasBuff("Protection of Mystrae") && 
			!HasBuff("Protection of Wishka") && 
			!HasBuff("Talisman of the Tribunal") &&	
			!HasBuff("Talisman of Jasinth"))
		{
			if (HasSHM) {
				strcat(Temp, "Wishka ");
			} else {
				strcat(Temp, "Jasinth ");
			}
		}

		//todo: visa bara i AG/FC raider
		//zone shortname: vergalid_raid  = här behövs corrupt resists
		//Slot 1 Corrupt Buffs
		if ((HasDRU || HasCLR) && pRaid && pRaid->RaidMemberCount &&
			!HasBuff("Rescind Corruption") &&	//CLR DRU (95)
			!HasBuff("Thwart Corruption") &&	//CLR DRU (95)
			!HasBuff("Reject Corruption") &&	//CLR DRU (90)
			!HasBuff("Repel Corruption") &&		//CLR DRU (85)
			!HasBuff("Forbear Corruption") &&	//CLR DRU (80)
			!HasBuff("Shared Purity") &&		//CLR (75)
			!HasBuff("Resist Corruption")		//CLR DRU (75)
			)     
		{
			strcat(Temp, "CORRUPT ");
		}

		if (!strlen(Temp)) {
			sprintf(Temp, "OK");
		}
	}

public:
	enum KrustMembers
	{
		MeleeBuffs=1,		//String
		CasterBuffs,		//String
		ResistBuffs,		//String
		Caster,				//Bool
		Melee,				//Bool
		Hybrid,				//Bool
		Tank,				//Bool
		Healer,				//Bool
		Buff,				//Bool
		
		//bool:
		GroupedBST,
		GroupedCLR,
		GroupedDRU,
		GroupedSHD,
		GroupedSHM,
		GroupedENC,
		GroupedPAL,
		GroupedMAG,
		GroupedNEC,
		GroupedRNG
	};

	//Constructor
	MQ2KrustType():MQ2Type("Krust")
	{
		TypeMember(MeleeBuffs);
		TypeMember(CasterBuffs);
		TypeMember(ResistBuffs);
		TypeMember(Caster);
		TypeMember(Melee);
		TypeMember(Hybrid);
		TypeMember(Tank);
		TypeMember(Healer);
		TypeMember(Buff);
		TypeMember(GroupedBST);
		TypeMember(GroupedCLR);
		TypeMember(GroupedDRU);
		TypeMember(GroupedSHD);
		TypeMember(GroupedSHM);
		TypeMember(GroupedENC);
		TypeMember(GroupedPAL);
		TypeMember(GroupedMAG);
		TypeMember(GroupedNEC);
		TypeMember(GroupedRNG);
	}

	//Destructor
	~MQ2KrustType()
	{
	}

	bool GetMember(MQ2VARPTR VarPtr, PCHAR Member, PCHAR Index, MQ2TYPEVAR &Dest) 
	{
		PMQ2TYPEMEMBER pMember = MQ2KrustType::FindMember(Member); 

		if (!pMember) return false; 

		switch ((KrustMembers)pMember->ID) 
		{ 
			case MeleeBuffs:
				UpdateMeleeBuffs(Dest);
				return true;

			case CasterBuffs:
				UpdateCasterBuffs(Dest);
				return true;

			case ResistBuffs:
				UpdateResistBuffs(Dest);
				return true;

			case Caster:
				InitMyClassCheck();
				Dest.DWord=CasterClass;
				Dest.Type=pBoolType;
				return true;

			case Melee:
				InitMyClassCheck();
				Dest.DWord=MeleeClass;
				Dest.Type=pBoolType;
				return true;

			case Hybrid:
				InitMyClassCheck();
				Dest.DWord=HybridClass;
				Dest.Type=pBoolType;
				return true;

			case Tank:
				InitMyClassCheck();
				Dest.DWord=TankClass;
				Dest.Type=pBoolType;
				return true;

			case Healer:
				InitMyClassCheck();
				Dest.DWord=HealerClass;
				Dest.Type=pBoolType;
				return true;

			case Buff:
				if(ISINDEX())
				{
					BuffOn(Index,Dest);
					return true;
				}
				return false;

			case GroupedBST:
				InitClassCheck();
				Dest.DWord=HasBST;
				Dest.Type=pBoolType;
				return true;

			case GroupedCLR:
				InitClassCheck();
				Dest.DWord=HasCLR;
				Dest.Type=pBoolType;
				return true;

			case GroupedDRU:
				InitClassCheck();
				Dest.DWord=HasDRU;
				Dest.Type=pBoolType;
				return true;

			case GroupedSHD:
				InitClassCheck();
				Dest.DWord=HasSHD;
				Dest.Type=pBoolType;
				return true;

			case GroupedSHM:
				InitClassCheck();
				Dest.DWord=HasSHM;
				Dest.Type=pBoolType;
				return true;

			case GroupedENC:
				InitClassCheck();
				Dest.DWord=HasENC;
				Dest.Type=pBoolType;
				return true;

			case GroupedPAL:
				InitClassCheck();
				Dest.DWord=HasPAL;
				Dest.Type=pBoolType;
				return true;

			case GroupedMAG:
				InitClassCheck();
				Dest.DWord=HasMAG;
				Dest.Type=pBoolType;
				return true;

			case GroupedNEC:
				InitClassCheck();
				Dest.DWord=HasNEC;
				Dest.Type=pBoolType;
				return true;

			case GroupedRNG:
				InitClassCheck();
				Dest.DWord=HasRNG;
				Dest.Type=pBoolType;
				return true;
		}

		return false;
	}

	bool FromData(MQ2VARPTR &VarPtr, MQ2TYPEVAR &Source) {
		return false;
	}

	bool FromString(MQ2VARPTR &VarPtr, PCHAR Source) {
		return false;
	}

	void InitMyClassCheck()
	{
		PCHARINFO pChInfo = GetCharInfo();

		if (!gbInZone || !pChInfo || !pChInfo->pSpawn) return;

		CasterClass = false, MeleeClass = false, HybridClass = false, TankClass = false, HealerClass = false;

		switch ((PlayerClass) pChInfo->pSpawn->Class)
		{
			case Cleric:		CasterClass = true; HealerClass = true; break;
			case Druid:			CasterClass = true; HealerClass = true; break;
			case Shaman:		CasterClass = true; HealerClass = true; break;
			case Mage:			CasterClass = true; break;
			case Enchanter:		CasterClass = true; break;
			case Necromancer:	CasterClass = true; break;
			case Wizard:		CasterClass = true; break;
			case Warrior:		TankClass = true; MeleeClass = true; break;
			case Shadowknight:	TankClass = true; HybridClass = true; break;
			case Paladin:		TankClass = true; HybridClass = true; break;
			case Beastlord:		HybridClass = true; break;
			case Ranger:		HybridClass = true; break;
			case Bard:			HybridClass = true; break;
			case Monk:			MeleeClass = true; break;
			case Rogue:			MeleeClass = true; break;
			case Berserker:		MeleeClass = true; break;
		}
	}

	/**
	* This function is part of the extension written by Gnits and helped by IEatAcid
	*
	*  This function is what makes ${Krust.Buff[aego]} return true if the buff is on
	*/
	bool BuffOn(PCHAR szName, MQ2TYPEVAR &Dest)
	{
		char szBuffClass[MAX_STRING];
		strcpy(szBuffClass,strlwr(szName));

		PCHARINFO pChInfo = GetCharInfo();

		Dest.Type=pBoolType;

		// Start of cleric specific buffs
		if (!stricmp(szBuffClass, "aego")) {
			// Starting with cleric aego
			if (
			HasBuff("Surety")				||													//CLR (95)
			HasBuff("Certitude")			||													//CLR (95)
			HasBuff("Credence")				||													//CLR (95)
			HasBuff("Reliance")				|| HasBuff("Hand of Reliance") ||					//CLR (90)
			HasBuff("Gallantry")			|| HasBuff("Hand of Gallantry") ||					//CLR (85)
			HasBuff("Temerity")				|| HasBuff("Hand of Temerity") ||					//CLR (80)
			HasBuff("Tenacity")				|| HasBuff("Hand of Tenacity") ||					//CLR (75)
			HasBuff("Conviction")			|| HasBuff("Hand of Conviction") ||					//CLR (70)
			HasBuff("Virtue")				|| HasBuff("Hand of Virtue") ||						//CLR (65)
			HasBuff("Aegolism")				|| HasBuff("Blessing of Aegolism") ||				//CLR (60)
			HasBuff("Temperance")			|| HasBuff("Blessing of Temperance"))				//CLR (45)
			{
				HasAego=true;
				Dest.DWord=HasAego;
				return true;
			} else {
				HasAego=false;
				Dest.DWord=HasAego;
				return false;
			}
		}

		if (!stricmp(szBuffClass, "wov")) {
			// Starting with Cleric AC
			DebugSpewAlways("Testing for class (%s)",szBuffClass);
			if (
				HasBuff("Ward of the Ardent") ||										//CLR (105)
				HasBuff("Ward of the Reverent") ||										//CLR (100)
				HasBuff("Ward of the Zealous") ||										//CLR (95)
				HasBuff("Ward of the Earnest") ||	HasBuff("Order of the Earnest") ||	//CLR (90)
				HasBuff("Ward of the Devout") ||	HasBuff("Order of the Devout") ||	//CLR (85)
				HasBuff("Ward of the Resolute") ||	HasBuff("Order of the Resolute") ||	//CLR (80)
				HasBuff("Ward of the Dauntless") ||										//CLR (75)
				HasBuff("Ward of Valiance") ||											//CLR (70)
				HasBuff("Ward of Gallantry") ||											//CLR (65)
				// Starting Paladin AC Buffs
				HasBuff("Bulwark of Piety")
				)
			{
				DebugSpewAlways("Testing for class (%s) TRUE",szBuffClass);
				HasWOV=true;
				Dest.DWord=HasWOV;
				return true;
			} else {
				HasWOV=false;
				Dest.DWord=HasWOV;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"spellhaste")) {
			DebugSpewAlways("Testing for class (%s)",szBuffClass);
			if (
			HasBuff("Benediction of Piety") ||		HasBuff("Hand of Zeal") ||			//CLR (105)
			HasBuff("Blessing of Ferver") ||		HasBuff("Hand of Ferver") ||		//CLR (100)
			HasBuff("Blessing of Assurance") ||		HasBuff("Hand of Assurance") ||		//CLR (95)
			HasBuff("Blessing of Will") ||			HasBuff("Hand of Will") || 			//CLR (90)
			HasBuff("Aura of Loyalty") ||			HasBuff("Blessing of Loyalty") ||	//CLR (85)
			HasBuff("Aura of Resolve") ||			HasBuff("Blessing of Resolve") ||	//CLR (80)
			HasBuff("Aura of Purpose") ||			HasBuff("Blessing of Purpose") ||	//CLR (75)
			HasBuff("Aura of Devotion") ||			HasBuff("Blessing of Devotion") ||	//CLR (70)
			HasBuff("Aura of Reverence") ||			HasBuff("Blessing of Reverence")	//CLR (65)
				)
			{
				DebugSpewAlways("Testing for class (%s) TRUE",szBuffClass);
				HasSpellhaste=true;
				Dest.DWord=HasSpellhaste;
				return true;
			} else {
				HasSpellhaste=false;
				Dest.DWord=HasSpellhaste;
				return false;
			}
		}

		// Start of druid specific buffs
		if(!stricmp(szBuffClass,"skin")) {
			DebugSpewAlways("Testing for class (%s)",szBuffClass);
			if (
			HasBuff("Shieldstone Skin")				|| HasBuff("Shieldstone Blessing") ||				//DRU (105)
			HasBuff("Granitebark Skin")				|| HasBuff("Granitebark Blessing") ||				//DRU (100)
			HasBuff("Stonebark Skin")				|| HasBuff("Stonebark Blessing") ||					//DRU (95)
			HasBuff("Timbercore Skin")				|| HasBuff("Blessing of the Timbercore") ||			//DRU (90)
			HasBuff("Heartwood Skin")				|| HasBuff("Blessing of the Heartwood") ||			//DRU (85)
			HasBuff("Ironwood Skin")				|| HasBuff("Blessing of the Ironwood") ||			//DRU (80)
			HasBuff("Direwild Skin")				|| HasBuff("Blessing of the Direwild") ||			//DRU (75)
			HasBuff("Steeloak Skin")				|| HasBuff("Blessing of Steeloak") ||				//DRU (70)
			HasBuff("Blessing of the Nine")			|| HasBuff("Protection of the Nine") ||				//DRU (65)
			HasBuff("Protection of the Cabbage")	|| HasBuff("Protection of the Glades")) 			//DRU (60)				)
			{
				DebugSpewAlways("Testing for class (%s) TRUE",szBuffClass);
				HasSkin=true;
				Dest.DWord=HasSkin;
				return true;
			} else {
				HasSkin=false;
				Dest.DWord=HasSkin;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"pos")) {
			if (HasBuff("Circle of Seasons") ||
				HasBuff("Protection of Seasons"))
			{
				HasPoS=true;
				Dest.DWord=HasPoS;
				return true;
			} else {
				HasPoS=false;
				Dest.DWord=HasPoS;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"mask")) {
			// Starting with Druid Mask
			if (
				HasBuff("Mask of the Hunter") ||				// Dru (60)
				HasBuff("Mask of the Forest") ||				// Dru (65)
				HasBuff("Mask of the Wild") ||					// Dru (70)
				HasBuff("Mask of the Shadowcat") ||				// Dru (80)
				HasBuff("Mask of the Raptor") ||				// Dru (85)
				HasBuff("Mask of the Arboreal") ||				// Dru (90)
				HasBuff("Mask of the Thicket Dweller") ||		// Dru (95)
				HasBuff("Mask of the Bosquetender")	||			// Dru (100)
				HasBuff("Mask of the Copsetender")				// Dru (105)
				)
			{
				HasMask=true;
				Dest.DWord=HasMask;
				return true;
			} else {
				HasMask=false;
				Dest.DWord=HasMask;
				return false;
			}
		}

		// Start of Enchater specific buffs
		if(!stricmp(szBuffClass,"crack")) {
			// Starting with enchanter haste
			if (
				HasBuff("Precognition") ||					HasBuff("Voice of Precognition") ||		//ENC (105)
				HasBuff("Forsight") ||						HasBuff("Voice of Foresight") ||		//ENC (100)
				HasBuff("Premeditation") ||					HasBuff("Voice of Premeditation") ||	//ENC (95)
				HasBuff("Forethought ") ||					HasBuff("Voice of Forethought") ||		//ENC (90)
				HasBuff("Prescience") ||					HasBuff("Voice of Prescience") ||		//ENC (85)
				HasBuff("Seer's Cognizance") ||				HasBuff("Voice of Cognizance") ||		//ENC (80)
				HasBuff("Seer's Intuition") ||				HasBuff("Voice of Intuition") ||		//ENC (75)
				HasBuff("Clairvoyance") ||					HasBuff("Voice of Clairvoyance") ||		//ENC (70)
				HasBuff("Quellious") ||						HasBuff("Voice of Tranquility") ||		//ENC (65)
				HasBuff("Koadic's Endless Intellect") || 											//ENC (60)
				// Other haste buffs (potions, items)
				HasBuff("Elixir of Clarity X"))
			{
				HasCrack=true;
				Dest.DWord=HasCrack;
				return true;
			} else {
				HasCrack=false;
				Dest.DWord=HasCrack;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"god")) {
			if (HasBuff("Guard of Druzzil"))
			{
				HasGoD=true;
				Dest.DWord=HasGoD;
				return true;
			} else {
				HasGoD=false;
				Dest.DWord=HasGoD;
				return false;
			}
		}

		// Start of Beastlord specific buffs
		if(!stricmp(szBuffClass,"se")) {
			if (
			HasBuff("Spiritual Elaboration") ||		//BST (105)
			HasBuff("Spiritual Evolution") ||		//BST (100)
			HasBuff("Spiritual Enrichment") ||		//BST (95)
			HasBuff("Spiritual Enhancement") ||		//BST (90)
			HasBuff("Spiritual Epiphany") ||		//BST (80)
			HasBuff("Spiritual Edification") ||		//BST (85)
			HasBuff("Spiritual Enlightenment") ||	//BST (75)
			HasBuff("Spiritual Ascendance") ||		//BST (70)
			HasBuff("Spiritual Dominion"))			//BST (65)
			{
				HasSE=true;
				Dest.DWord=HasSE;
				return true;
			} else {
				HasSE=false;
				Dest.DWord=HasSE;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"sv")) {
			// Starting with enchanter haste
			if (
				HasBuff("Spiritual Vivification") ||		// BST (105)
				HasBuff("Spiritual Vindication") ||			// BST (100)
				HasBuff("Spiritual Valiance") ||			// BST (95)
				HasBuff("Spiritual Vivacity") ||			// BST (80)
				HasBuff("Spiritual Verve") ||				// BST (85)
				HasBuff("Spiritual Vim") ||					// BST (75)
				HasBuff("Spiritual Vitality") ||			// BST (70)
				HasBuff("Spiritual Vigor") ||				// BST (65)
				HasBuff("Spiritual Strength") 			// BST (60)	
				)
			{
				HasSV=true;
				Dest.DWord=HasSV;
				return true;
			} else {
				HasSV=false;
				Dest.DWord=HasSV;
				return false;
			}
		}

		// Start of Paladin specific buffs
		if(!stricmp(szBuffClass,"brells")) {
			if (
				HasBuff("Brell's Stalwart Bulwark") ||		// PAL (105)
				HasBuff("Brell's Steadfast Bulwark") ||		// PAL (100)
				HasBuff("Brell's Adamantine Armor") ||		// PAL (95)
				HasBuff("Brell's Tellurian Rampart") ||		// PAL (90)
				HasBuff("Brell's Loamy Ward") ||			// PAL (85)
				HasBuff("Brell's Earthen Aegis") ||			// PAL (80)
				HasBuff("Brell's Stony Guard") ||			// PAL (75)
				HasBuff("Brell's Brawny Bulwark") ||		// PAL (70)
				HasBuff("Brell's Stalwart Shield") ||		// PAL (65)
				HasBuff("Brell's Mountainous Barrier") ||	// PAL (60)
				HasBuff("Brell's Steadfast Aegis")			// PAL (50)
				)
			{
				HasBrells=true;
				Dest.DWord=HasBrells;
				return true;
			} else {
				HasBrells=false;
				Dest.DWord=HasBrells;
				return false;
			}
		}

		// Start of Ranger specific buffs
		if(!stricmp(szBuffClass,"atk")) {
			DebugSpewAlways("Testing for class (%s)",szBuffClass);
			if (
				HasBuff("Strength of the Copsestalker") ||		// RNG (105)
				HasBuff("Strength of the Bosquestalker") ||		// RNG (100)
				HasBuff("Strength of the Gladetender") ||		// RNG (95)
				HasBuff("Strength of the Tracker") ||			// RNG (90)
				HasBuff("Strength of the Thicket Stalker") ||	// RNG (85)
				HasBuff("Protection of the Kirkoten") ||		// RNG (80)
				HasBuff("Strength of the Gladewalker") ||		// RNG (80)
				HasBuff("Strength of the Forest Stalker") ||	// RNG (75)
				HasBuff("Strength of the Hunter") ||			// RNG (70)
				HasBuff("Ward of the Hunter") ||				// RNG (70)
				HasBuff("Strength of Tunare") ||				// RNG (65)
				HasBuff("Aura of Rage") ||
				HasBuff("Nature's Precision"))
			{
				HasATK=true;
				Dest.DWord=HasATK;
				return true;
			} else {
				HasATK=false;
				Dest.DWord=HasATK;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"guard")) {
			if (
				HasBuff("Cloak of Nettlespears") ||			// RNG (105)
				HasBuff("Shared Cloak of Nettlespears") ||	// RNG (105)
				HasBuff("Cloak of Spurs") ||				// RNG (100)
				HasBuff("Shared Cloak of Spurs") ||			// RNG (100)
				HasBuff("Cloak of Burrs") ||				// RNG (95)
				HasBuff("Cloak of Quills") ||				// RNG (90)
				HasBuff("Shared Cloak of Burrs") ||			// RNG (95)
				HasBuff("Cloak of Feathers") ||				// RNG (85)
				HasBuff("Cloak of Scales") ||				// RNG (75)
				HasBuff("Guard of the Earth")				// RNG (75)
				)
			{
				HasGuard=true;
				Dest.DWord=HasGuard;
				return true;
			} else {
				HasGuard=false;
				Dest.DWord=HasGuard;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"predator")) {
			if (
				HasBuff("Shout of the Copsestalker") ||	//RNG (105)
				HasBuff("Shout of the Predator") ||		//RNG (100)
				HasBuff("Cry of the Predator") ||		//RNG (95)
				HasBuff("Roar of the Predator") ||		//RNG (90)
				HasBuff("Yowl of the Predator") ||		//RNG (85)
				HasBuff("Gnarl of the Predator") ||		//RNG (80)
				HasBuff("Snarl of the Predator") ||		//RNG (75)
				HasBuff("Howl of the Predator") ||		//RNG (70)
				HasBuff("Call of the Predator") ||		//RNG (60)
				HasBuff("Spirit of the Predator"))		//RNG (65)
			{
				HasPred=true;
				Dest.DWord=HasPred;
				return true;
			} else {
				HasPred=false;
				Dest.DWord=HasPred;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"eye")) {
			if (
				HasBuff("Eyes of the Harrier") ||		//RNG (105)
				HasBuff("Eyes of the Howler") ||		//RNG (100)
				HasBuff("Eyes of the Raptor") ||		//RNG (95)
				HasBuff("Eyes of the Wolf") ||			//RNG (90)
				HasBuff("Eyes of the Nocturnal") ||		//RNG (85)
				HasBuff("Eyes of the Peregrine") ||		//RNG (80)
				HasBuff("Eyes of the Owl") ||			//RNG (75)
				HasBuff("Eagle Eye"))				{
				HasEye=true;
				Dest.DWord=HasEye;
				return true;
			} else {
				HasEye=false;
				Dest.DWord=HasEye;
				return false;
			}
		}

		// Start of Shaman specific buffs
		if(!stricmp(szBuffClass,"foresight")) {
			if (
				HasBuff("Preeminent Foresight") || 													//SHM (90-100)
				HasBuff("Transcendent Foresight") ||												//SHM (85)
				HasBuff("Preternatural Foresight") ||
				HasBuff("Talisman of Foresight") ||			//SHM (75)
				
				HasBuff("Spirit of Sense") ||
				HasBuff("Talisman of Sense"))
			{
				HasForesight=true;
				Dest.DWord=HasForesight;
				return true;
			} else {
				HasForesight=false;
				Dest.DWord=HasForesight;
				return false;
			}
		}
  
		if(!stricmp(szBuffClass,"fortitude")) {
			if (
				HasBuff("Spirit of Dedication") ||			//SHM (105)
				HasBuff("Spirit of Dauntlessness") ||		//SHM (100)
				HasBuff("Spirit of Resolve") ||				//SHM (95)
				HasBuff("Spirit of Valor") ||				//SHM (90)
				HasBuff("Spirit of Determination") ||		//SHM (85)
				HasBuff("Spirit of Vehemence") ||			//SHM (80)
				HasBuff("Talisman of Vehemence") ||			//SHM (80)																			//SHM (SoF)
				HasBuff("Spirit of Persistence") ||			//SHM (75)
				HasBuff("Talisman of Persistence") ||		//SHM (75)
				HasBuff("Spirit of Fortitude") ||			//SHM (70)
				HasBuff("Talisman of Fortitude")			//SHM (70)
				)
			{
				HasFortitude=true;
				Dest.DWord=HasFortitude;
				return true;
			} else {
				HasFortitude=false;
				Dest.DWord=HasFortitude;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"wishka")) {
			if (HasBuff("Protection of Wishka") ||
				HasBuff("Protection of Mystrae") || 
				HasBuff("Talisman of the Tribunal"))
			{
				HasWishka=true;
				Dest.DWord=HasWishka;
				return true;
			} else {
				HasWishka=false;
				Dest.DWord=HasWishka;
				return true;
			}
		}

		if(!stricmp(szBuffClass,"mageconvert")) {
			if (	
				HasBuff("Dark Symbiosis Recourse") ||			//MAG (105)
				HasBuff("Phantasmal Symbiosis Recourse") ||		//MAG (100)
				HasBuff("Arcane Symbiosis Recourse") ||			//MAG (95)
				HasBuff("Spectral Symbiosis Recourse") ||		//MAG (90)
				HasBuff("Ethereal Symbiosis Recourse") ||		//MAG (85)
				HasBuff("Prime Symbiosis Recourse") ||			//MAG (80)
				HasBuff("Elemental Symbiosis Recourse") ||		//MAG (75)
				HasBuff("Elemental Simulacrum Recourse") ||
				HasBuff("Elemental Siphon Recourse") ||
				HasBuff("Elemental Draw Recourse"))
			{
				HasMageConvert=true;
				Dest.DWord=HasMageConvert;
				return true;
			} else {
				HasMageConvert=false;
				Dest.DWord=HasMageConvert;
				return true;
			}
		}

		if(!stricmp(szBuffClass,"mageregen")) {
			if (
				HasBuff("Praetorian Guardian") ||				//MAG (105)
				HasBuff("Phantasmal Guardian") ||				//MAG (100)
				HasBuff("Splendrous Guardian") ||				//MAG (95)
				HasBuff("Cognitive Guardian") ||				//MAG (90)
				HasBuff("Empyrean Guardian") ||					//MAG (85)
				HasBuff("Eidolic Guardian") ||					//MAG (80)
				HasBuff("Phantasmal Warden") ||
				HasBuff("Phantom Shield") ||
				HasBuff("Xegony's Phantasmal Guard") ||
				HasBuff("Transon's Phantasmal Protection"))
			{
				HasMageRegen=true;
				Dest.DWord=HasMageRegen;
				return true;
			} else {
				HasMageConvert=false;
				Dest.DWord=HasMageRegen;
				return true;
			}
		}

		// Start of multiclass buffs
		if(!stricmp(szBuffClass,"symbol")) {
			// Starting with Cleric Symbol
			if (
				HasBuff("Symbol of Nonia") ||															//CLR (100)
				HasBuff("Symbol of Gezat") ||															//CLR (100)
				HasBuff("Symbol of the Triumvirate") ||													//CLR (95)
				HasBuff("Symbol of Ealdun")			|| HasBuff("Ealdun's Mark") ||						//CLR (90)
				HasBuff("Symbol of Darianna")		|| HasBuff("Darianna's Mark") ||					//CLR (85)
				HasBuff("Symbol of Kaerra")			|| HasBuff("Kaerra's Mark") ||						//CLR (80)
				HasBuff("Symbol of Elushar")		|| HasBuff("Elushar's Mark") ||						//CLR (75)
				HasBuff("Symbol of Balikor")		|| HasBuff("Balikor's Mark") ||						//CLR (70)
				HasBuff("Symbol of Kazad")			|| HasBuff("Kazad`s Mark") ||						//CLR (65)
				HasBuff("Symbol of Marzin")			|| HasBuff("Marzin`s Mark") ||						//CLR (60)
				HasBuff("Symbol of Naltron")		|| HasBuff("Naltron`s Mark") ||						//CLR (58)
				// Paladin Symbol spells (Single and group)
				HasBuff("Symbol of Niparson")		||													//PAL (100)
				HasBuff("Symbol of Burim")			||													//PAL (100)
				HasBuff("Symbol of Erillion")		||													//PAL (95)
				HasBuff("Symbol of Jyleel")			||													//PAL (90)
				HasBuff("Symbol of Jeneca")			|| HasBuff("Jeneca's Mark") ||						//PAL (85)
				HasBuff("Symbol of Bthur")			|| HasBuff("Bthur's Mark") ||						//PAL (80)
				HasBuff("Symbol of Fenegar")		|| HasBuff("Fenegar's Mark") ||						//PAL (75)
				HasBuff("Symbol of Jeron")			|| HasBuff("Jeron's Mark")							//PAL (70)
				)
			{
				HasSymbol=true;
				Dest.DWord=HasSymbol;
				return true;
			} else {
				HasSymbol=false;
				Dest.DWord=HasSymbol;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"jasinth")) {
			if (HasBuff("Talisman of Jasinth"))
			{
				HasJasinth=true;
				Dest.DWord=HasJasinth;
				return true;
			} else {
				HasJasinth=false;
				Dest.DWord=HasJasinth;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"corrupt")) {
			if (
				HasBuff("Rescind Corruption") ||	//CLR DRU (95)
				HasBuff("Thwart Corruption") ||		//CLR DRU (95)
				HasBuff("Reject Corruption") ||		//CLR DRU (90)
				HasBuff("Repel Corruption") ||		//CLR DRU (85)
				HasBuff("Forbear Corruption") ||	//CLR DRU (80)
				HasBuff("Shared Purity") ||			//CLR (75)
				HasBuff("Resist Corruption")		//CLR DRU (75)
				)
			{
				HasCorrupt=true;
				Dest.DWord=HasCorrupt;
				return true;
			} else {
				HasCorrupt=false;
				Dest.DWord=HasCorrupt;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"life")) {
			if (HasBuff("Second Life"))
			{
				HasLife=true;
				Dest.DWord=HasLife;
				return true;
			} else {
				HasLife=false;
				Dest.DWord=HasLife;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"summer")) {
			if (HasBuff("Circle of Summer"))
			{
				HasSummer=true;
				Dest.DWord=HasSummer;
				return true;
			} else {
				HasSummer=false;
				Dest.DWord=HasSummer;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"winter")) {
			if (HasBuff("Circle of Winter"))
			{
				HasWinter=true;
				Dest.DWord=HasWinter;
				return true;
			} else {
				HasWinter=false;
				Dest.DWord=HasWinter;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"focus")) {
			// Starting with shaman focus
			if (
				HasBuff("Doomscale Focusing") ||											//SHM (100)
				HasBuff("Insistent Focusing") ||											//SHM (100)
				HasBuff("Imperative Focusing") ||											//SHM (95)
				HasBuff("Exigent Focusing") ||												//SHM (90)
				HasBuff("Darkpaw Focusing") ||		HasBuff("Talisman of the Darkpaw") ||	//SHM (85)
				HasBuff("Bloodworg Focusing") ||	HasBuff("Talisman of the Bloodworg") ||	//SHM (80)
				HasBuff("Dire Focusing") ||			HasBuff("Talisman of the Dire") ||		//SHM (75)
				HasBuff("Wunshi's Focusing") ||		HasBuff("Talisman of Wunshi") ||		//SHM (70)
				HasBuff("Focus of the Seventh") ||	HasBuff("Focus of Soul") ||				//SHM (65)
				// Beastlord Buffs
				HasBuff("Focus of Okasi") ||		//BST (105)
				HasBuff("Focus of Sanera") ||		//BST (100)
				HasBuff("Focus of Klar") ||			//BST (95)
				HasBuff("Focus of Emiq") ||			//BST (90)
				HasBuff("Focus of Yemall") ||		//BST (85)
				HasBuff("Focus of Zott") ||			//BST (80)
				HasBuff("Focus of Amilan") ||		//BST (75)
				HasBuff("Focus of Alladnu") ||		//BST (70)
				HasBuff("Talisman of Kragg")		//BST (65)
				)
			{
				HasFocus=true;
				Dest.DWord=HasFocus;
				return true;
			} else {
				HasFocus=false;
				Dest.DWord=HasFocus;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"haste")) {
			// Starting with enchanter haste
			if (
				//Shaman
				HasBuff("Celerity") ||						HasBuff("Talisman of Celerity") ||	//SHM
				HasBuff("Alacrity") ||						HasBuff("Talisman of Alacrity") ||	//SHM
				HasBuff("Swift like the Wind") ||			HasBuff("Wonderous Rapidity") ||	//SHM
				//Enchanter
				HasBuff("Speed of Prokev") ||				HasBuff("Hastening of Prokev") ||	//ENC (105)
				HasBuff("Speed of Sviir") ||				HasBuff("Hastening of Sviir") ||	//ENC (100)
				HasBuff("Speed of Aransir") ||				HasBuff("Hastening of Aransir") ||	//ENC (95)
				HasBuff("Speed of Novak") ||				HasBuff("Hastening of Novak") ||	//ENC (90)
				HasBuff("Speed of Erradien") ||				HasBuff("Hastening of Erradien") ||	//ENC (80)
				HasBuff("Speed of Yozan") ||				HasBuff("Hastening of Ellowind") ||	//ENC (75)
				HasBuff("Speed of Salik") ||				HasBuff("Hastening of Salik") ||	//ENC (70)
				HasBuff("Vallon's Quickening") ||			HasBuff("Speed of Vallon") ||		//ENC (65)
				//Beastlord
				HasBuff("Etraordinary Velocity") ||			//BST (100)
				HasBuff("Exceptional Velocity") ||			//BST (95)
				HasBuff("Incomparable Velocity") ||			//BST (90)
				HasBuff("Twitching Speed") ||				//???
				// Other haste buffs (potions, items)
				HasBuff("Twitching Speed") ||
				HasBuff("Elixir of Speed IX") ||
				HasBuff("Elixir of Speed X"))
			{
				HasHaste=true;
				Dest.DWord=HasHaste;
				return true;
			} else {
				HasHaste=false;
				Dest.DWord=HasHaste;
				return false;
			}
		}

		if(!stricmp(szBuffClass,"ds")) {
			// Starting with Magician Damage Shield
			if (
				// Druid DS
				HasBuff("Daggerspur Bulwark") ||		HasBuff("Legacy of Daggerspurs") ||	//Dru (100)
				HasBuff("Spikethistle Bulwark") ||		HasBuff("Legacy of Spikethistle") ||	//Dru (100)
				HasBuff("Spineburr Bulwark") ||			HasBuff("Legacy of Spineburrs") ||		//Dru (95)
				HasBuff("Bonebriar Bulwark") ||			HasBuff("Legacy of Bonebriar") ||		//Dru (90)
				HasBuff("Brierbloom Bulwark") ||		HasBuff("Legacy of Brierbloom") ||		//Dru (85)
				HasBuff("Viridithorns Bulwark") ||		HasBuff("Legacy of Viridithorns") ||	//DRU (80)
				HasBuff("Viridifloral Shield") ||		HasBuff("Legacy of Viridiflora") ||		//DRU (75)
				HasBuff("Nettle Shield") ||				HasBuff("Circle of Nettles") ||			//DRU
				// Mage DS
				HasBuff("Flameweave") ||				HasBuff("Circle of Flameweave") ||		//MAG (100)
				HasBuff("Flameskin") ||					HasBuff("Circle of Flameskin") ||		//MAG (100)
				HasBuff("Embercoat") ||					HasBuff("Circle of Embers") ||			//MAG (95)
				HasBuff("Dreamfire Coat") ||			HasBuff("Circle of Dreamfire") ||		//MAG (90)
				HasBuff("Brimstoneskin") ||				HasBuff("Circle of Brimstoneskin") ||	//MAG (85)
				HasBuff("Lavaskin") ||					HasBuff("Circle of Lavaskin") ||		//MAG (80)																						//MAG (SoF)
				HasBuff("Magmaskin") ||					HasBuff("Circle of Magmaskin") ||		//MAG (75)
				HasBuff("Fireskin") ||					HasBuff("Circle of Fireskin")			//MAG (70)
				)
			{
				HasDS=true;
				Dest.DWord=HasDS;
				return true;
			} else {
				HasDS=false;
				Dest.DWord=HasDS;
				return false;
			}
		}

		// The most extensive list of buffs.  Need more class specific ones.
		if(!stricmp(szBuffClass,"armor")) {
			if(
				//Caster Shield Buffs
				HasBuff("Shield of the Pellarus") ||		//CST (105)
				HasBuff("Shield of the Dauntless") ||		//CST (100)
				HasBuff("Shield of Bronze") ||				//CST (95)
				HasBuff("Shield of Dreams") ||				//CST (90)
				HasBuff("Shield of the Void") ||			//CST (85)
				HasBuff("Prime Guard") ||					//MAG (80)
				HasBuff("Bulwark of the Crystalwing") ||	//WIZ (80)
				HasBuff("Spellbound Shield") ||				//ENC (80)
				HasBuff("Bulwark of Shadows") ||			//NEC (80)
				HasBuff("Prime Shielding") ||				//MAG (75)
				HasBuff("Shield of the Crystalwing") ||		//WIZ (75)
				HasBuff("Sorcerous Shield") ||				//ENC (75)
				HasBuff("Shield of Darkness") ||			//NEC (75)
				HasBuff("Elemental Aura") ||				//MAG (70)
				HasBuff("Ether Shield") ||					//WIZ (70)
				HasBuff("Mystic Shield") ||					//ENC (70)
				HasBuff("Shadow Guard") ||					//NEC (70)
				HasBuff("Shield of Maelin") ||				//CST (65)
				HasBuff("Shield of the Arcane") ||			//CST (60)
				HasBuff("Armor of the Zealous") ||			//CLR (95)
				HasBuff("Armor of the Earnest") ||			//CLR (90)
				HasBuff("Armor of the Devout") ||			//CLR (85)
				HasBuff("Armor of the Solemn") ||			//CLR (80)
				HasBuff("Armor of the Sacred") ||			//CLR (75)
				HasBuff("Armor of the Pious") ||			//CLR (70)
				HasBuff("Armor of the Zealot"))				//CLR (65)

			{
				HasArmor=true;
				Dest.DWord=HasArmor;
				return true;
			} else {
				HasArmor=false;
				Dest.DWord=HasArmor;
				return false;
			}
			DebugSpewAlways("Done testing for class (%s) and they are NOT happy",szBuffClass);
		}

		if(!stricmp(szBuffClass,"proc")) {
			if (
				HasBuff("Squall of Blades") ||		//RNG (105)
				HasBuff("Crackling Blades") ||		//RNG (100)
				HasBuff("Deafening Edges") ||		//RNG (95)
				HasBuff("Jolting Impact") ||		//RNG (95)
				HasBuff("Crackling Edges") ||		//RNG (90)
				HasBuff("Devastating Edges") ||		//RNG (89)
				HasBuff("Jolting Edges") ||			//RNG (87)
				HasBuff("Crackling Blades") ||		//RNG (85)
				HasBuff("Devastating Blades") ||	//RNG (84)
				HasBuff("Jolting Swings") ||		//RNG (82)
				HasBuff("Deafening Blades") ||		//RNG (80)
				HasBuff("Jolting Strikes") ||		//RNG (77)
				HasBuff("Thundering Blades") ||		//RNG (75)
				HasBuff("Call of Lightning") ||		//RNG (70)
				HasBuff("Sylvan Call") ||			//RNG (65)
				HasBuff("Cry of Thunder") ||		//RNG (65)
				HasBuff("Jolting Blades")			//RNG (54)
				)
			{
				HasProc=true;
				Dest.DWord=HasProc;
				return true;
			} else {
				HasProc=false;
				Dest.DWord=HasProc;
				return false;
			}
		}

		WriteChatf("\axInvalid Buff Class.\ax");
		WriteChatf("\axHere is the list of current buff classes:,\ax");
		WriteChatf("\ayAego, Symbol, WOV, Spellhaste, Skin, POS, Mask, Mammoth,\ax");
		WriteChatf("\ayCrack, GOD, SE, SV, Brells, Foresight, Fortitude, Wishka,\ax");
		WriteChatf("\ayHunter, Guard, Predator, ATK, Eye, Jasinth, Corrupt,\ax");
		WriteChatf("\ayLife, DS, Summer, Winter, Focus, Haste, Armore, Proc,\ax");
		WriteChatf("\ayMageConvert, MageRegen\ax");
		WriteChatf("\arNOTE: \axWhile \ayspelling\ax is important, \aycase\ax is not.\ax");

		HasNone=false;
		Dest.DWord=HasNone;
		return false;
	}

};


BOOL dataKrust(PCHAR Index, MQ2TYPEVAR &Dest)
{
	Dest.DWord=1;
	Dest.Type=pKrustType;
	return true;
} 


// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializePlugin(VOID)
{
	DebugSpewAlways("Initializing MQ2Krust");

	pKrustType = new MQ2KrustType;
	AddMQ2Data("Krust", dataKrust); 

	// Add commands, MQ2Data items, hooks, etc.
	AddCommand("/targethp", TargetByHP);
	AddCommand("/assistsmart", AssistSmart);
	AddCommand("/assisttarget", AssistTarget);
	AddCommand("/shieldlowest", ShieldLowest);
	AddCommand("/healgroup", HealGroup);
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownPlugin(VOID)
{
	DebugSpewAlways("Shutting down MQ2Krust");

	RemoveMQ2Data("Krust");
	delete pKrustType;

	// Remove commands, MQ2Data items, hooks, etc.
	RemoveCommand("/targethp");
	RemoveCommand("/assistsmart");
	RemoveCommand("/assisttarget");
	RemoveCommand("/shieldlowest");
	RemoveCommand("/healgroup");
}

// Called after entering a new zone
PLUGIN_API VOID OnZoned(VOID)
{
	DebugSpewAlways("MQ2Krust::OnZoned()");

	pKrustType->InitMyClassCheck();
} 

//assisttarget - gives you the HoTT target instantly, as a /assist without a name as parameter works but quick
VOID AssistTarget(PSPAWNINFO pChar, PCHAR szLine)
{
	PCHARINFO	pChInfo = GetCharInfo();

	if (pTarget && ppTarget)
	{
		PSPAWNINFO	Target = (PSPAWNINFO)pTarget;

		if (Target->SpawnID == pChInfo->pSpawn->SpawnID) {	//ignore if i am targeting myself
			return;
		}

		/* 1. If there is a target in HoTT, change to this target instantly */
		if (pChInfo->pSpawn->TargetOfTarget)
		{
			if (pChInfo->pSpawn->TargetOfTarget != Target->SpawnID) //dont do anything if target is targething himself
			if (ppTarget) {
				PSPAWNINFO	*psTarget = (PSPAWNINFO*)ppTarget;
				*psTarget = (PSPAWNINFO) GetSpawnByID(pChInfo->pSpawn->TargetOfTarget);
			}
		}
		/* 2. Else, do an old fashioned /assist */
		//fixme: can't tell if hott is enabled and my target has no target, or if hott is disabled!
		//	would save me from sending /assist in these cases
		else
		{
			//fixme: what is longest distance for /assist to work?
			if (GetDistance(Target->X,Target->Y) > 200) {
				WriteChatf("# Warning: /assisttarget - HoTT not enabled and target is too far away, aborting");
			} else {
				DoCommand(pChar, "/assist");
				WriteChatf("/assisttarget - Did old fashioned /assist");
			}
		}
	}
}


//assistsmart - assist group if in group. raid if in raid
VOID AssistSmart(PSPAWNINFO pChar, PCHAR szLine)
{
	PCHARINFO   pChInfo = GetCharInfo();
	PSPAWNINFO me = pChInfo->pSpawn;
	PSPAWNINFO   *psTarget = (PSPAWNINFO*)ppTarget;

	if (pRaid && pRaid->RaidMemberCount) {
		//Raid assist 1-3
		if (pChInfo->pSpawn && pChInfo->pSpawn->RaidAssistNPC[0]) {
			*psTarget = (PSPAWNINFO) GetSpawnByID(pChInfo->pSpawn->RaidAssistNPC[0]);
			WriteChatf("selected target from raid delegate-ma0");
		} else if (pChInfo->pSpawn && pChInfo->pSpawn->RaidAssistNPC[1]) {
			*psTarget = (PSPAWNINFO) GetSpawnByID(pChInfo->pSpawn->RaidAssistNPC[1]);
			WriteChatf("selected target from raid delegate-ma1");
		} else if (pChInfo->pSpawn && pChInfo->pSpawn->RaidAssistNPC[2]) {
			*psTarget = (PSPAWNINFO) GetSpawnByID(pChInfo->pSpawn->RaidAssistNPC[2]);
			WriteChatf("selected target from raid delegate-ma2");
		} else {
			//assist closest warrior when we have no raid target
			float distance = 200.0;
			int assist_raid_id=0;

			for (int i=0; i < (int)pRaid->RaidMemberCount; i++) {
				float tmp_distance = 100.0; //GetDistance(me, pRaid->RaidMember[i]);   //FIXME: how to get PSPAWNINFO ?
				if (pRaid->RaidMember[i].nClass == Warrior && tmp_distance < distance) {
					assist_raid_id = i;
					distance = tmp_distance;
				}
			}

			if (assist_raid_id) {
				char assistname[MAX_PATH];
				sprintf(assistname,  "/assist %s", pRaid->RaidMember[assist_raid_id].Name);
				DoCommand(pChar, assistname);
				WriteChatf("assisted %s", pRaid->RaidMember[assist_raid_id].Name);
			} else {
				WriteChatf("found nothing to assist in raid");
			}
		}
	} else if (pChInfo->pGroupInfo && GetGroupMember(1)) {
		//Group assist
		if (pChInfo->pSpawn && pChInfo->pSpawn->GroupAssistNPC[0]) {
			*psTarget = (PSPAWNINFO) GetSpawnByID(pChInfo->pSpawn->GroupAssistNPC[0]);
			WriteChatf("selected target from group delegate-ma");
		} else {
			DoCommand(pChar, "/assist group");
			WriteChatf("used /assist group");
		}
	} else {
		WriteChatf("Error: /assistsmart UNGROUPED!");
	}
}


/* Target group member or HoTT depending on lowest HP% */
VOID TargetByHP(PSPAWNINFO pChar, PCHAR szLine)
{
	PCHARINFO	pChInfo = GetCharInfo();

	PSPAWNINFO me = GetCharInfo()->pSpawn; 

	PSPAWNINFO	pNewTarget = NULL;
	int			lowestHP = pChInfo->pSpawn->HPCurrent * 100 / pChInfo->pSpawn->HPMax; 

	/* 1. If I'm below 100% HP, start with myself */
	if (lowestHP < 100) {
		pNewTarget = pChInfo->pSpawn;
	}

	/* 2. Check target (if target PC is lower health than any else, we keep this as target) */
	if (pTarget)
	{
		PSPAWNINFO Target = (PSPAWNINFO)pTarget;
		if (Target->Type == SPAWN_PLAYER)	//ignore corpses, npc's
		if (GetDistance(me, Target) < 200)	//only care about players in range
		if (Target->SpawnID != pChInfo->pSpawn->SpawnID) //ignore myself
		if (Target->HPCurrent < lowestHP)
		{
			lowestHP = Target->HPCurrent;
			pNewTarget = Target;
		}
	}

	/* 3. Check target in HoTT */
	if (pTarget && pChInfo->pSpawn->TargetOfTarget)
	{
		PSPAWNINFO target = (PSPAWNINFO)GetSpawnByID(pChInfo->pSpawn->TargetOfTarget);
		if (target->Type == SPAWN_PLAYER)   //ignore corpses, npc's
		{
			if (GetDistance(me, target) < 200)   //only care about players in range
			{
				if (target->SpawnID != pChInfo->pSpawn->SpawnID) //ignore myself
				{
					if (target->HPCurrent < lowestHP) {
						//HoTT has lower % HP than I do, use HoTT instead
						lowestHP = target->HPCurrent;
						pNewTarget = target;
					}
				}
			}
		}
	}

	/* 4. Checking group members if someone has even less % HP */
	if (pChInfo->pGroupInfo)
	for (int index=0; index<5; index++) {
		if (PSPAWNINFO pMember = GetGroupMember(index))
		if (pMember->Type == SPAWN_PLAYER)   //ignore dead group mates
		if (GetDistance(me, pMember) < 200)   //only care about players in range
		if (pMember->HPCurrent < lowestHP) {
			lowestHP = pMember->HPCurrent;
			pNewTarget = pMember;
		}
	}

	/* 5. Target the spawn with least HP */
	if (ppTarget && pNewTarget) {
		PSPAWNINFO	*psTarget = (PSPAWNINFO*)ppTarget;
		*psTarget = pNewTarget;
		//WriteChatf("/targethp - %s selected at %d%% HP", pNewTarget->Name, pNewTarget->HPCurrent);
	}
}

/* /shieldlowest, warrior target the lowest % HP group member and shield them if they are 30% hp or below */
VOID ShieldLowest(PSPAWNINFO pChar, PCHAR szLine)
{
	PSPAWNINFO me = GetCharInfo()->pSpawn; 

	PCHARINFO	pChInfo = GetCharInfo();
	PSPAWNINFO	pNewTarget = NULL;
	int			lowestHP = 100; 

	//todo: can out of group members be /shield'ed? in that case check target and hott for players

	/* 1. Checking group members if someone has even less % HP */
	if (pChInfo->pGroupInfo)
	for (int index=0; index<5; index++) {
		if (PSPAWNINFO pMember = GetGroupMember(index))
		if (pMember->Type == SPAWN_PLAYER)   //ignore dead group mates
		if (GetDistance(me, pMember) < 200)   //only care about players in range
		if (pMember->HPCurrent < lowestHP) {
			lowestHP = pMember->HPCurrent;
			pNewTarget = pMember;
		}
	}

	/* 2. Target the spawn with least HP */
	if (ppTarget && pNewTarget) {
		PSPAWNINFO	*psTarget = (PSPAWNINFO*)ppTarget;
		*psTarget = pNewTarget;
		WriteChatf("/shieldlowest - Shielding %s %d%% HP", pNewTarget->Name, pNewTarget->HPCurrent);
		DoCommand(pChar, "/shield");
	}
}

VOID HealGroup(PSPAWNINFO pChar, PCHAR szLine)
{
	PCHARINFO	pChInfo = GetCharInfo();
	PSPAWNINFO me = GetCharInfo()->pSpawn;
	PSPAWNINFO	pNewTarget = NULL;

	int heal_pct = 65;	//FIXME take heal % parameter. /healgroup 80

	WriteChatf("/healgroup !");

	//if (${Window[LootWnd].Open} || !${Krust.Healer} || ${Me.Casting.ID}) return;


	/*
	/if (${Me.PctHPs} <= 10) {
		WriteChatf("Using heal potion at %d%", ${Me.PctHPs} );
		//call MQ2Cast "Distillate of Divine Healing X" item
		Cast("\"Distillate of Divine Healing X\" item");
		return;
	}*/

	/* Heal target if its a player outside group
	/if (${Target.ID} && ${Target.Type.Equal["PC"]} &&
		(${Target.Distance} < 200) && ${Target.PctHPs} <= ${PctHPs} &&
		${Target.Type.NotEqual["Corpse"]} &&
		${Target.State.NotEqual["DEAD"]} &&
		${Me.SpellReady[DRUID_QH1]}) {
		PerformHeal( ${Target.ID}, heal_pct);
		return;
	}*/

	/* See if HoTT is a player that needs a heal	
	/if (${Me.LAHoTT} && ${Me.TargetOfTarget.Type.Equal["PC"]} && (${Me.TargetOfTarget.Distance} < 200) && ${Me.TargetOfTarget.PctHPs} <= ${PctHPs} && ${Me.TargetOfTarget.Type.NotEqual["Corpse"]} && ${Me.TargetOfTarget.State.NotEqual["DEAD"]} && ${Me.SpellReady[DRUID_QH1]}) {
		PerformHeal( ${Me.TargetOfTarget.ID}, heal_pct);
		return;
	}
	*/

	/* See if we should cast a group heal
	/if (${Group.Members} && ${Me.SpellReady[DRUID_GROUPHEAL]}) {
		int x = 0, n = 0;

		for (int i=0; i<=5; i++) {
			/if (${Group.Member[${i}].ID} && (${Group.Member[${i}].Distance} < 150) && ${Group.Member[${i}].Type.NotEqual["Corpse"]} && ${Group.Member[${i}].State.NotEqual["DEAD"]} && ${Group.Member[${i}].Class.Name.NotEqual["Warrior"]} && ${Group.Member[${i}].Class.Name.NotEqual["Shadow Knight"]} && ${Group.Member[${i}].Class.Name.NotEqual["Paladin"]}) {
				/if (${Group.Member[${i}].PctHPs} <= 98) {
					| only count non-100% ppl, to avoid logic from deciding a group heal would be best to cast on a single-damaged player in group
					/varcalc x ${x}+${Group.Member[${i}].PctHPs}
					/varcalc n ${n}+1

					| dont do group heal if one need a big heal
					/if (${Group.Member[${i}].PctHPs} <= ${PctHPs}) {
						PerformHeal( ${Group.Member[${i}].ID}, heal_pct );
						return;
					}
				}
			}
		}

		int grphealpct = 84;
		if (${Me.PctMana} >= 90) grphealpct = 88;

		/if (${n}) {
			/varcalc x ${x} / ${n}
			/if (${n} >=3 && ${x} <= ${grphealpct}) {
				/call DebugMsg "GROUP HEAL (avg hp ${x}% for ${n} players)"
				/call MQ2Cast "DRUID_GROUPHEAL"
				return;
			}
		}
	}
	*/

	// Check if someone in group needs a heal
	/*
	for (int i=0; i<=5; i++) {
		if (
			${Group.Member[${i}].ID} &&
			(${Group.Member[${i}].Distance} < 200) &&
			(${Group.Member[${i}].PctHPs} <= ${PctHPs}) &&
			${Group.Member[${i}].Type.NotEqual["Corpse"]} &&
			${Group.Member[${i}].State.NotEqual["DEAD"]} &&
			${Me.SpellReady[DRUID_QH1]}) {
			PerformHeal( ${Group.Member[${i}].ID}, heal_pct);
			return;
		}
	}
*/

	int lowestHP = 100;
	if (pChInfo->pGroupInfo)
	for (int index=0; index<5; index++) {
		if (PSPAWNINFO pMember = GetGroupMember(index))
		if (pMember->Type == SPAWN_PLAYER)   //ignore dead group mates
		if (GetDistance(me, pMember) < 200)   //only care about players in range
		if (pMember->HPCurrent < lowestHP) {
			lowestHP = pMember->HPCurrent;
			pNewTarget = pMember;    
		}
	}

	if (lowestHP < 100) {
		Heal(pNewTarget, lowestHP);
	}
}

void Heal(PSPAWNINFO spawn, int pct)
{
	PCHARINFO	pChInfo = GetCharInfo();
	PSPAWNINFO	*psTarget = (PSPAWNINFO*)ppTarget;

	//sanity checks
	if (spawn->Type != SPAWN_PLAYER) {
		WriteChatf("Heal() called to heal dead player %s", spawn->Name);
		return;
	}

	//Target player to heal
	*psTarget = spawn;

	//figure out what spell to use
	switch (pChInfo->pSpawn->Class) {
		case Druid:
			WriteChatf("choosing a druid heal...");
			if (pct < 30 /*&& SpellReady("Convergence of Spirits|alt")*/) {
				WriteChatf("DRU: Conv. of Spirits AA on %s", spawn->Name);
				return;
			}
			if (pct < 40 /*&& SpellReady("Adrenaline Swell|gem")*/) {
				WriteChatf("DRU: Adrenaline Swell on %s", spawn->Name);
				return;
			}
			/*if (SpellReady("Puravida|gem")) {
				WriteChatf("DRU: Puravida on %s", spawn->Name);
				return;
			}*/
			WriteChatf("no druid heals ready...");
			break;

		default:
			WriteChatf("dunno how to heal with current class...");
			return;
	}

}

