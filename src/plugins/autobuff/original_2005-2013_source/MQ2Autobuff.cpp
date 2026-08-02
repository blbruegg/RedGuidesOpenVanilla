
/****************************
* External Includes         *
****************************/

#include "../MQ2Plugin.h"
#include <stdio.h>
#include <map>
#include <string>
#include <windows.h>





/****************************
* Static Defines            * 
****************************/

// if compiling for EQEMU, comment out this line.
//#define LIVE




#define PLUGIN_NAME "MQ2Autobuff"

#ifdef LIVE
PLUGIN_VERSION(1.0);
#endif

#define error(message) echo("\at[\ax\ag" PLUGIN_NAME "\ax\at::\ax\ag" __FILE__ "\ax\at::\ax\ag%i\ax\at]\ax\ar%s\ax", __LINE__, message);

#define       CAST_SUCCESS      0
#define       CAST_INTERRUPTED  1
#define       CAST_RESIST       2
#define       CAST_COLLAPSE     3
#define       CAST_RECOVER      4
#define       CAST_FIZZLE       5
#define       CAST_STANDING     6
#define       CAST_STUNNED      7
#define       CAST_INVISIBLE    8
#define       CAST_NOTREADY     9
#define       CAST_OUTOFMANA   10
#define       CAST_OUTOFRANGE  11
#define       CAST_NOTARGET    12
#define       CAST_CANNOTSEE   13
#define       CAST_COMPONENTS  14
#define       CAST_OUTDOORS    15
#define       CAST_TAKEHOLD    16
#define       CAST_IMMUNE      17
#define       CAST_DISTRACTED  18
#define       CAST_ABORTED     19
#define       CAST_UNKNOWN     20


#define       TYPE_SPELL        1
#define       TYPE_ALT          2
#define       TYPE_ITEM         3










/****************************
* Structures                *
****************************/

struct SpellInfo
{
	bool abilityReady;
	PSPELL spell;
	int type;
	double castTime;
	double range;
	int duration;
	double efficiency;
	int totalHealedPerPerson;
};

struct SingleBuff 
{
	char name[100];
	unsigned long classes;
	unsigned long petclasses;
	bool active;

	bool inCombat;	// not used yet
	bool bounce;		// not used yet

	SpellInfo info;
};

struct BuffTarget 
{
	PSPAWNINFO spawn;
	bool shouldBuff;
	bool isPet;
	PlayerClass classCheck;

	map <string, unsigned long>	 recastTimes;
};


struct OtherBuffTarget 
{
	char name[100];
};










/****************************
* Function Pre-Declarations *
****************************/

char   * __cdecl va( char *format, ... );
void echo(PCHAR zFormat, ...);
long GetTimeNow();
VOID CallMemorize(char* spellName);
VOID CallCast(char* spellName, int targetID);
bool isCastingIdle();
int GetCastResult();
bool setupMQ2Cast(bool displayWarning);
bool getSpellInfo(char* spellName, SpellInfo* info);
bool isSpellGemReady(int nGem);
PSPELL getSpellInSlot(int nGem);
bool readyToCast();
int findGemForSpell(PSPELL spellparm);
bool readyToCastSpell(char* spellName);
bool isClass(PlayerClass pc, unsigned long classes);
unsigned long addClass(PlayerClass pc, unsigned long classes);
unsigned long parseClasses(char* srcstr);
void WriteINI();
void LoadINI();
VOID BuffCMD(PSPAWNINFO pChar, PCHAR szLine);
VOID RebuildBuffTargetsList();
VOID BuffDoneCasting(int result);

// Exported function for other plugins to call.
PLUGIN_API bool SingleBuffs(bool inCombat);


// Internal Pre-Declaration for callback functions called after casting a spell
typedef VOID (__cdecl *afterCastFunctionCALL) (int result);












/*********************************
* External Function Declarations *
*********************************/

// MQ2Cast exports
typedef VOID (__cdecl *CastCommandCALL) (PSPAWNINFO,PCHAR);
typedef VOID (__cdecl *MemoCommandCALL) (PSPAWNINFO,PCHAR);
// MQ2Cast exports 
// !!!!!!!!!!!!!!!  These are not standard! Must be added to MQ2Cast manually! (see readme) !!!!!!!!!!!
typedef int (__cdecl *getCastResultCALL) ();
typedef bool (__cdecl *isCastingIdleCALL) ();










/************************************
* Global Variable Declarations      *
************************************/

char						ToonName[64]					= "";

map <string, SingleBuff>	BuffList; 
map <string, BuffTarget>	buffTargets;

map <string, OtherBuffTarget>	outOfGroupTargets;

long						automate						= 0;         // Enable Automation
long						buffGem							= 8;         // Enable Automation


char						currentCastTarget[100];
char						currentCastBuff[100];

bool						waitingOnSpell = false;
unsigned long				waitingOnSpellFromTime = 0;

bool						doingBuffs = false;
int							numberOfBuffsRemaining = 0;

afterCastFunctionCALL		afterCastFunction = 0;

int							oldTarget = -1;
int							CastTargetSpawnID;



getCastResultCALL getCastResultExt = 0;
isCastingIdleCALL isCastingIdleExt = 0;
CastCommandCALL CastCommandExt = 0;
MemoCommandCALL MemoCommandExt = 0;










PreSetup(PLUGIN_NAME);







/****************************
* Global Functions          *
****************************/

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
char   * __cdecl va( char *format, ... ) {
   va_list      argptr;
   static char      string[2][32000];   // in case va is called by nested functions
   static int      index = 0;
   char   *buf;

   buf = string[index & 1];
   index++;

   va_start (argptr, format);
   vsprintf (buf, format,argptr);
   va_end (argptr);

   return buf;
}

void echo(PCHAR zFormat, ...) {
	typedef VOID (__cdecl *BandolierCALL) (PCHAR);
	char zOutput[MAX_STRING]; va_list vaList; va_start(vaList,zFormat);
	vsprintf(zOutput,zFormat,vaList);

	WriteChatColor(zOutput,USERCOLOR_DEFAULT);
}		

long GetTimeNow()
{
	SYSTEMTIME st;
	::GetSystemTime(&st);
	long lCurrentSecond=0;
	lCurrentSecond  = st.wDay    * 24 * 60 * 60;
	lCurrentSecond += st.wHour        * 60 * 60;
	lCurrentSecond += st.wMinute           * 60;
	lCurrentSecond += st.wSecond;
	return (lCurrentSecond);
}

VOID CallMemorize(char* spellName)
{
	if(MemoCommandExt)
		MemoCommandExt(GetCharInfo()->pSpawn,va("\"%s\" %i", spellName, buffGem));
}

VOID CallCast(char* spellName, int targetID)
{
	if (ppTarget && pTarget)
	{
		oldTarget = pTarget->Data.SpawnID;
	}
	else
	{
		oldTarget = 0;
	}

	CastTargetSpawnID = targetID;

	if(CastCommandExt)
		CastCommandExt(GetCharInfo()->pSpawn,va("\"%s\" gem%i -targetid|%i", spellName, buffGem, targetID));
}

bool isCastingIdle()
{
	if(isCastingIdleExt)
		return isCastingIdleExt();

	return false;
}

int GetCastResult()
{
	if(getCastResultExt)
		return getCastResultExt();

	return -1;
}


PLUGIN_API VOID CastCommandFallback(PSPAWNINFO pChar, PCHAR Cmd) 
{
	DoCommand(GetCharInfo()->pSpawn,va("/casting %s", Cmd));
}
PLUGIN_API VOID MemoCommandFallback(PSPAWNINFO pChar, PCHAR zLine)
{
	DoCommand(GetCharInfo()->pSpawn,va("/memorize %s", zLine));
}

PLUGIN_API int getCastResultFallback()
{
	char zOutput[MAX_STRING] = "${Cast.Result}";

	ParseMacroData(zOutput);

	if (!strcmp("CAST_SUCCESS", zOutput))
	{
		return CAST_SUCCESS;
	}
	if (!strcmp("CAST_INTERRUPTED", zOutput))
	{
		return CAST_INTERRUPTED;
	}
	if (!strcmp("CAST_RESIST", zOutput))
	{
		return CAST_RESIST;
	}
	if (!strcmp("CAST_COLLAPSE", zOutput))
	{
		return CAST_COLLAPSE;
	}
	if (!strcmp("CAST_RECOVER", zOutput))
	{
		return CAST_RECOVER;
	}
	if (!strcmp("CAST_FIZZLE", zOutput))
	{
		return CAST_FIZZLE;
	}
	if (!strcmp("CAST_STANDING", zOutput))
	{
		return CAST_STANDING;
	}
	if (!strcmp("CAST_STUNNED", zOutput))
	{
		return CAST_STUNNED;
	}
	if (!strcmp("CAST_INVISIBLE", zOutput))
	{
		return CAST_INVISIBLE;
	}
	if (!strcmp("CAST_NOTREADY", zOutput))
	{
		return CAST_NOTREADY;
	}
	if (!strcmp("CAST_OUTOFMANA", zOutput))
	{
		return CAST_OUTOFMANA;
	}
	if (!strcmp("CAST_OUTOFRANGE", zOutput))
	{
		return CAST_OUTOFRANGE;
	}
	if (!strcmp("CAST_NOTARGET", zOutput))
	{
		return CAST_NOTARGET;
	}
	if (!strcmp("CAST_CANNOTSEE", zOutput))
	{
		return CAST_CANNOTSEE;
	}
	if (!strcmp("CAST_COMPONENTS", zOutput))
	{
		return CAST_COMPONENTS;
	}
	if (!strcmp("CAST_OUTDOORS", zOutput))
	{
		return CAST_OUTDOORS;
	}
	if (!strcmp("CAST_TAKEHOLD", zOutput))
	{
		return CAST_TAKEHOLD;
	}
	if (!strcmp("CAST_IMMUNE", zOutput))
	{
		return CAST_IMMUNE;
	}
	if (!strcmp("CAST_DISTRACTED", zOutput))
	{
		return CAST_DISTRACTED;
	}
	if (!strcmp("CAST_CANCELLED", zOutput))
	{
		return CAST_ABORTED;
	}
	if (!strcmp("CAST_UNKNOWN", zOutput))
	{
		return CAST_UNKNOWN;
	}

	return -1;
}



PLUGIN_API bool isCastingIdleFallback()
{
	char zOutput[MAX_STRING] = "${Cast.Status}";
	char* chr;

	ParseMacroData(zOutput);

	chr = zOutput;
	while (chr && *chr && chr-zOutput < MAX_STRING)
	{
		if (*chr == 'I')
			return true;

		chr++;
	}
	return false;
}


bool setupMQ2Cast(bool displayWarning)
{

	PMQPLUGIN pLook=pPlugins;
	bool mq2castloaded = false;


	while(pLook) 
	{
		if (pLook && pLook->Pulse && !stricmp(pLook->szFilename,"MQ2Cast"))
		{
			mq2castloaded = true;

			if( ! (getCastResultExt = (getCastResultCALL)GetProcAddress(pLook->hModule,"getCastResult")))
			{
				getCastResultExt = getCastResultFallback;
			}
			if( ! (isCastingIdleExt = (isCastingIdleCALL)GetProcAddress(pLook->hModule,"isCastingIdle")))
			{
				isCastingIdleExt = isCastingIdleFallback;
			}
			if( ! (CastCommandExt = (CastCommandCALL)GetProcAddress(pLook->hModule,"CastCommand")))
			{
				CastCommandExt = CastCommandFallback;
			}
			if( ! (MemoCommandExt = (MemoCommandCALL)GetProcAddress(pLook->hModule,"MemoCommand")))
			{
				MemoCommandExt = MemoCommandFallback;
			}
		}
		pLook=pLook->pNext;
	}

	if (!mq2castloaded)
	{
		if (displayWarning)
		{
			echo("\arMQ2Autobuff WARNING\ax: \agMQ2Cast not loaded - MQ2Autobuff disabled\ax! (2)");
		}
		automate = 0;
		return false;
	}

	return true;
}

PSPELL SpellBook(PCHAR ID) {
  if(ID[0]) {
    if(IsNumber(ID)) {
      int Number=atoi(ID);
      for(DWORD nSpell=0; nSpell < NUM_BOOK_SLOTS; nSpell++)
        if(GetCharInfo2()->SpellBook[nSpell]==Number)
          return GetSpellByID(Number);
    } else {
      for(DWORD nSpell=0; nSpell < NUM_BOOK_SLOTS; nSpell++)
        if(PSPELL pSpell=GetSpellByID(GetCharInfo2()->SpellBook[nSpell]))
	        if(!stricmp(ID,pSpell->Name)) return pSpell;
    }
  }
  return NULL;
}

bool getSpellInfo(char* spellName, SpellInfo* info) 
{
	MQ2TYPEVAR Ret;
	PCONTENTS pItem;

	if (!GetCharInfo() || !GetCharInfo()->pSpawn)
		return false;


	info->spell = 0;

	info->abilityReady = 0;

	if(spellName[0]) 
	{
		for (unsigned long nAbility=0 ; nAbility<AA_CHAR_MAX_REAL ; nAbility++) {
			if (!GetCharInfo2()->AAList[nAbility].AAIndex) break;
			if ( PALTABILITY pAbility=pAltAdvManager->GetAltAbility(GetCharInfo2()->AAList[nAbility].AAIndex)) {
				if (PCHAR pName=pCDBStr->GetString(pAbility->nName, 1, NULL)) {
					if (!stricmp(spellName,pName)) {

						info->abilityReady = pAltAdvManager->IsAbilityReady(pPCData,pAbility,0);
						info->spell = GetSpellByID(pAbility->SpellID);
						info->castTime = (double)((double)info->spell->CastTime / 1000.0f);
						info->duration = GetSpellDuration(info->spell,GetCharInfo()->pSpawn) * 6;
						info->range = info->spell->Range;
						info->type = TYPE_ALT; 
						info->efficiency = 0;

						return true;
					}
				}
			}
		}

		if (info->spell = SpellBook(spellName))
		{
			info->castTime = (double)((double)(pCharData1->GetAACastingTimeModifier((EQ_Spell*)(info->spell))+
				pCharData1->GetFocusCastingTimeModifier((EQ_Spell*)(info->spell),0)+
				info->spell->CastTime) / 1000.0f);
			info->duration = GetSpellDuration(info->spell,GetCharInfo()->pSpawn) * 6;
			info->range = info->spell->Range;
			info->type = TYPE_SPELL;
			info->totalHealedPerPerson = info->spell->Max[0];

			if (info->duration > 0)
			{
				info->totalHealedPerPerson *= info->duration / 6;
			}

			if (info->spell->Mana)
			{
				info->efficiency = info->totalHealedPerPerson/info->spell->Mana;
			}
			else
			{
				info->efficiency = 0;
			}

			//echo("Spell: %s, BaseDuration: %i, DurNew: %i", spellName, info->spell->DurationValue1, info->duration);

			return true;
		}

		if (dataFindItem(spellName, Ret))
		{
			pItem = (PCONTENTS)Ret.Ptr;

			if (pItem && pItem->Item && pItem->Item->Clicky.SpellID)
			{
				info->spell = GetSpellByID(pItem->Item->Clicky.SpellID);
				info->castTime = (double)((double)pItem->Item->CastTime / 1000.0f);
				info->duration = GetSpellDuration(info->spell,GetCharInfo()->pSpawn) * 6;
				info->range = info->spell->Range;
				info->type = TYPE_ITEM;
				info->efficiency = 0;

				return true;
			}
		}
	}

	return false;
}

bool isSpellGemReady(int nGem)
{
	if (nGem<9) 
	{ 
		if (!((PEQCASTSPELLWINDOW)pCastSpellWnd)->SpellSlots[nGem]) 
			return false; 
		else 
			return (((PEQCASTSPELLWINDOW)pCastSpellWnd)->SpellSlots[nGem]->spellstate!=1); 
	} 
	return false; 
}


PSPELL getSpellInSlot(int nGem)
{
	PSPELL tmpSpell = 0;

	if (nGem<9)
	{
		tmpSpell = GetSpellByID(GetCharInfo2()->MemorizedSpells[nGem]);
	}
	return tmpSpell;
}

bool readyToCast()
{
	if (
		isSpellGemReady(0) 
		|| isSpellGemReady(1) 
		|| isSpellGemReady(2) 
		|| isSpellGemReady(3) 
		|| isSpellGemReady(4) 
		|| isSpellGemReady(5) 
		|| isSpellGemReady(6) 
		|| isSpellGemReady(7) 
		|| isSpellGemReady(8) 
		)
	{
		return true;
	}
	return false;
}

int findGemForSpell(PSPELL spellparm)
{
	PSPELL pSpell;
	unsigned long nGem;

	for (nGem=0 ; nGem < 9 ; nGem++)
	{
		if (pSpell=GetSpellByID(GetCharInfo2()->MemorizedSpells[nGem]))
		{
			if (spellparm->ID == pSpell->ID)
			{
				return nGem;
			}
		}
	}

	return -1;
}

bool readyToCastSpell(char* spellName)
{
	SpellInfo info;
	int nGem;
	long time = GetTimeNow();

	// don't cast while invis
	if (GetCharInfo()->pSpawn->HideMode)
		return false;

	if (getSpellInfo(spellName, &info))
	{

		// TODO: item cooldowns should probably be checked here...
		if (info.type == TYPE_ITEM)
		{
			return true;
		}
		if (info.type == TYPE_ALT)
		{
			if (info.spell)
			{
				return info.abilityReady;
			}
			return false;
		}


		nGem = findGemForSpell(info.spell);
		if (nGem >= 0)
		{
			if ((nGem+1) == buffGem)
			{
				waitingOnSpell = true;
				waitingOnSpellFromTime = time;
			}
			return isSpellGemReady(nGem);
		}
		else // not memmed
		{
			if (!waitingOnSpell)
			{
				CallMemorize(spellName);
				
				// memorizing a spell counts as a cast, since it takes so long
				if (doingBuffs == true && numberOfBuffsRemaining > 0)
				{
					numberOfBuffsRemaining--;
					if (numberOfBuffsRemaining <= 0)
					{
						numberOfBuffsRemaining = 0;
						doingBuffs = false;
					}
				}
			}
		}

		if ((nGem+1) == buffGem || nGem < 0)
		{
			waitingOnSpell = true;
			waitingOnSpellFromTime = time;
		}
	}

	return false; 
}

bool isClass(PlayerClass pc, unsigned long classes)
{
	return (classes & 1 << (int)pc) ? true : false;
}

unsigned long addClass(PlayerClass pc, unsigned long classes)
{
	return classes | 1 << (int)pc;
}

unsigned long parseClasses(char* srcstr)
{
	char	buffer[MAX_STRING]			= "";
	unsigned long ret = 0;

	strcpy(buffer, srcstr);
	_strlwr(buffer);

	if (strstr(buffer, "warrior") || strstr(buffer, "war"))
	{
		ret = addClass(Warrior, ret);
	}
	if (strstr(buffer, "cleric") || strstr(buffer, "clr"))
	{
		ret = addClass(Cleric, ret);
	}
	if (strstr(buffer, "paladin") || strstr(buffer, "pal"))
	{
		ret = addClass(Paladin, ret);
	}
	if (strstr(buffer, "ranger") || strstr(buffer, "rng"))
	{
		ret = addClass(Ranger, ret);
	}
	if (strstr(buffer, "shadow knight") || strstr(buffer, "shadowknight") || strstr(buffer, "shd") || strstr(buffer, "sk"))
	{
		ret = addClass(Shadowknight, ret);
	}
	if (strstr(buffer, "druid") || strstr(buffer, "dru"))
	{
		ret = addClass(Druid, ret);
	}
	if (strstr(buffer, "monk") || strstr(buffer, "mnk"))
	{
		ret = addClass(Monk, ret);
	}
	if (strstr(buffer, "bard") || strstr(buffer, "brd"))
	{
		ret = addClass(Bard, ret);
	}
	if (strstr(buffer, "rogue") || strstr(buffer, "rog"))
	{
		ret = addClass(Rogue, ret);
	}
	if (strstr(buffer, "shaman") || strstr(buffer, "shm") || strstr(buffer, "shammy"))
	{
		ret = addClass(Shaman, ret);
	}
	if (strstr(buffer, "necromancer") || strstr(buffer, "nec"))
	{
		ret = addClass(Necromancer, ret);
	}
	if (strstr(buffer, "wizard") || strstr(buffer, "wiz"))
	{
		ret = addClass(Wizard, ret);
	}
	if (strstr(buffer, "mage") || strstr(buffer, "mag") || strstr(buffer, "magician"))
	{
		ret = addClass(Mage, ret);
	}
	if (strstr(buffer, "enchanter") || strstr(buffer, "enc") || strstr(buffer, "chanter"))
	{
		ret = addClass(Enchanter, ret);
	}
	if (strstr(buffer, "beastlord") || strstr(buffer, "bst") || strstr(buffer, "beast"))
	{
		ret = addClass(Beastlord, ret);
	}
	if (strstr(buffer, "berserker") || strstr(buffer, "ber") || strstr(buffer, "zerker"))
	{
		ret = addClass(Berserker, ret);
	}

	return ret;
}

void WriteINI() 
{
	if (!GetCharInfo())
	{
		return;
	}

	setupMQ2Cast(true);

	if (ToonName[0])
	{
		WritePrivateProfileString("General Settings", "automate", va("%i", automate), INIFileName);
		WritePrivateProfileString("General Settings", "buffGem", va("%i", buffGem), INIFileName);
	}

}

void LoadINI() 
{

	CHAR szBuffer[MAX_STRING] = {0};
	CHAR szSection[MAX_STRING] = {0};
	CHAR szTemp[MAX_STRING] = {0};
	int prev=0;

	if (!GetCharInfo())
	{
		return;
	}

	CreateDirectory(va("%s\\MQ2Autobuff Settings", gszINIPath), NULL);

	strcpy(ToonName,GetCharInfo()->Name); 
	sprintf(INIFileName,"%s\\MQ2Autobuff Settings\\%s.ini",gszINIPath, ToonName);


	if (ToonName[0])
	{

		automate		= GetPrivateProfileInt("General Settings",		"automate"			,0,INIFileName); 
		buffGem			= GetPrivateProfileInt("General Settings",		"buffGem"			,8,INIFileName); 



		BuffList.clear();

		GetPrivateProfileString(NULL,NULL,NULL,szBuffer,MAX_STRING,INIFileName);


		for (int i=0; ((szBuffer[i] != 0) || (i > 0 && szBuffer[i-1] != 0)); i++) 
		{
			szSection[i-prev] = szBuffer[i];
			if (szBuffer[i]==0) {

				if (!strstr(szSection,"Spell Name Here") && !strstr(szSection,"General Settings"))
				{
					GetPrivateProfileString(szSection,"active","1",szTemp,MAX_STRING,INIFileName);
					BuffList[szSection].active = atoi(szTemp) ? true : false;

					GetPrivateProfileString(szSection,"inCombat","0",szTemp,MAX_STRING,INIFileName);
					BuffList[szSection].inCombat = atoi(szTemp) ? true : false;

					GetPrivateProfileString(szSection,"bounce","0",szTemp,MAX_STRING,INIFileName);
					BuffList[szSection].bounce = atoi(szTemp) ? true : false;

					GetPrivateProfileString(szSection,"classes","Bard,Beastlord,Berserker,Cleric,Druid,Enchanter,Magician,"
						"Monk,Necromancer,Paladin,Ranger,Rogue,Shadow Knight,Shaman,Warrior,Wizard",szTemp,MAX_STRING,INIFileName);
					BuffList[szSection].classes = parseClasses(szTemp);

					GetPrivateProfileString(szSection,"petclasses","Beastlord,Magician,Necromancer,Shaman",szTemp,MAX_STRING,INIFileName);
					BuffList[szSection].petclasses = parseClasses(szTemp);

					strcpy(BuffList[szSection].name, szSection);

					getSpellInfo(szSection, &BuffList[szSection].info);

					if (!BuffList[szSection].info.spell)
					{
						// spell wasn't able to be found... disable it.
						BuffList[szSection].active = false;
					}
				}

				prev=i+1;
			}
		}

		if (BuffList.size() == 0)
		{
			WritePrivateProfileString("Spell Name Here", "active", "1", INIFileName);
			WritePrivateProfileString("Spell Name Here", "classes", "Bard,Beastlord,Berserker,Cleric,Druid,Enchanter,"
				"Magician,Monk,Necromancer,Paladin,Ranger,Rogue,Shadow Knight,Shaman,Warrior,Wizard", INIFileName);
			WritePrivateProfileString("Spell Name Here", "petclasses", "Beastlord,Magician,Necromancer,Shaman", INIFileName);
			WritePrivateProfileString("Spell Name Here", "inCombat", "0", INIFileName);
			WritePrivateProfileString("Spell Name Here", "bounce", "0", INIFileName);
		}


		WriteINI();

		echo("Buff list loaded.");
	}


} 



VOID BuffCMD(PSPAWNINFO pChar, PCHAR szLine)
{	
	map <string, SingleBuff>::iterator il;

	if (!setupMQ2Cast(true))
		return;

	_strlwr(szLine);

	if (strstr(szLine,"off"))
	{
		automate = 0;

		WriteINI();

		echo("Automatic buffing is now %s", automate ? "\agON\ax" : "\arOFF\ax");
	}
	else if (strstr(szLine,"on"))
	{
		automate = 1;

		WriteINI();

		echo("Automatic buffing is now %s", automate ? "\agON\ax" : "\arOFF\ax");
	}	
	else if (strstr(szLine,"auto") || strstr(szLine,"toggle"))
	{
		automate = automate ? 0 : 1;

		WriteINI();

		echo("Automatic buffing is now %s", automate ? "\agON\ax" : "\arOFF\ax");
	}
	else if (strstr(szLine,"list"))
	{
		echo("Buff List:");
		for (il = BuffList.begin(); il != BuffList.end(); il++)
		{
			if (il->second.active)
			{
				echo(va("\ag%s\ax", il->second.name));
			} 
			else if (il->second.bounce)
			{
				echo(va("\ar%s\ax", il->second.name));
			}
			else
			{
				echo(va("\ay%s\ax", il->second.name));
			}
		}
		echo(va("Key: \agActive\ax, \ayInactive\ax, \arBounce\ax"));
		echo("If you have a spell set as inactive in your ini, but it's listed as"
			" active here, then the plugin couldn't find it. Check your spelling.");
	
	}
	else if (strstr(szLine,"test"))
	{
		map <string, BuffTarget>::iterator it;
		map <string, unsigned long>::iterator ir;
		long time = GetTimeNow();

		echo("Buffee List:");
		RebuildBuffTargetsList();

		for (it = buffTargets.begin(); it != buffTargets.end(); it++)
		{
			if (it->second.spawn && it->second.shouldBuff)
			{
				echo("[%s]", it->second.spawn->Name);
				for (ir = it->second.recastTimes.begin(); ir != it->second.recastTimes.end(); ir++)
				{
					echo("        %s: %i", ir->first.data(), (ir->second - time));
				}
			}
		}
	
	}
	else if (strstr(szLine,"reload"))
	{
		LoadINI();
	}
	else if (strstr(szLine,"add"))
	{
		if (ppTarget && pTarget)
		{
			strcpy(outOfGroupTargets[pTarget->Data.Name].name, pTarget->Data.Name);
		}
	}
	else if (strstr(szLine,"remove"))
	{
		if (ppTarget && pTarget)
		{
			outOfGroupTargets.erase(pTarget->Data.Name);
		}
	}
	else if (strstr(szLine,"reset"))
	{
		if (ppTarget && pTarget)
		{
			buffTargets.erase(pTarget->Data.Name);
		}
	}
	else if (strstr(szLine,"resetall"))
	{
		buffTargets.clear();
	}
	else if (strstr(szLine,"clear"))
	{
		outOfGroupTargets.clear();
	}
	else
	{
		// calling /buff by it's self will do a full buff of any buffs which need to be cast
		doingBuffs = true;
		if (atoi(szLine) > 0)
		{
			numberOfBuffsRemaining = atoi(szLine);
		}
		else
		{
			numberOfBuffsRemaining = -1;
		}
	}
}


PLUGIN_API VOID InitializePlugin(VOID)
{
	AddCommand("/buff",BuffCMD,0,1);

	afterCastFunction = NULL;
}

PLUGIN_API VOID ShutdownPlugin(VOID)
{
	RemoveCommand("/buff");
}

PLUGIN_API VOID SetGameState(DWORD GameState)
{
	if (!GetCharInfo() || !GetCharInfo()->pSpawn
#ifndef LIVE
		|| !GetCharInfo()->pSpawn->pActorInfo
#endif
		)
	{
		// if we're not completely loaded in yet, do nothing
		return;
	}

	LoadINI();
}





// FIXME: Should really be detecting group changes, and only calling this when changes are detected...
VOID RebuildBuffTargetsList()
{
	int j;
	EQPlayer* Pet;

	map <string, BuffTarget>::iterator ibt;
	map <string, OtherBuffTarget>::iterator iobt;
	map <string, PSPAWNINFO>::iterator spawnIterator;

	for (ibt = buffTargets.begin(); ibt != buffTargets.end(); ibt++)
	{
		ibt->second.shouldBuff = false;
		ibt->second.spawn = 0;
	}

	// Add group members to the list
	for (j=0; j<5; j++)
	{
		if (pGroup && pGroup->MemberExists[j] && pGroup->pMember[j])
		{
			buffTargets[pGroup->pMember[j]->Name].shouldBuff = true;
			ibt = buffTargets.find(pGroup->pMember[j]->Name);

			ibt->second.spawn = pGroup->pMember[j];
			ibt->second.classCheck = (PlayerClass)pGroup->pMember[j]->Class;
			ibt->second.isPet = false;
			
#ifdef LIVE
			Pet = GetSpawnByID(pGroup->pMember[j]->PetID);
#else
			Pet = GetSpawnByID(pGroup->pMember[j]->pActorInfo->PetID);
#endif

			if (Pet)
			{
				buffTargets[Pet->Data.Name].shouldBuff = true;
				ibt = buffTargets.find(Pet->Data.Name);

				ibt->second.spawn = &Pet->Data;

				// pets use their master's class for checking whether to cast spells on them
				ibt->second.classCheck = (PlayerClass)pGroup->pMember[j]->Class;
				ibt->second.isPet = true;
			}
		}
	}


	// Add ourself into the list
	buffTargets[GetCharInfo()->Name].shouldBuff = true;
	ibt = buffTargets.find(GetCharInfo()->Name);

	ibt->second.spawn = GetCharInfo()->pSpawn;
	ibt->second.classCheck = (PlayerClass)GetCharInfo()->pSpawn->Class;
	ibt->second.isPet = false;
	
#ifdef LIVE
	Pet = GetSpawnByID(GetCharInfo()->pSpawn->PetID);
#else
	Pet = GetSpawnByID(GetCharInfo()->pSpawn->pActorInfo->PetID);
#endif

	if (Pet)
	{
		buffTargets[Pet->Data.Name].shouldBuff = true;
		ibt = buffTargets.find(Pet->Data.Name);

		ibt->second.spawn = &Pet->Data;

		// pets use their master's class for checking whether to cast spells on them
		ibt->second.classCheck = (PlayerClass)GetCharInfo()->pSpawn->Class;
		ibt->second.isPet = true;
	}


	for (iobt = outOfGroupTargets.begin(); iobt != outOfGroupTargets.end(); ++iobt)
	{
		spawnIterator = SpawnByName.find(iobt->second.name);

		if (spawnIterator != SpawnByName.end() && spawnIterator->second)
		{
			buffTargets[spawnIterator->second->Name].shouldBuff = true;
			ibt = buffTargets.find(spawnIterator->second->Name);

			ibt->second.spawn = spawnIterator->second;
			ibt->second.classCheck = (PlayerClass)spawnIterator->second->Class;
			ibt->second.isPet = false;
			
#ifdef LIVE
			Pet = GetSpawnByID(spawnIterator->second->PetID);
#else
			Pet = GetSpawnByID(spawnIterator->second->pActorInfo->PetID);
#endif

			if (Pet)
			{
				buffTargets[Pet->Data.Name].shouldBuff = true;
				ibt = buffTargets.find(Pet->Data.Name);

				ibt->second.spawn = &Pet->Data;

				// pets use their master's class for checking whether to cast spells on them
				ibt->second.classCheck = (PlayerClass)spawnIterator->second->Class;
				ibt->second.isPet = true;
			}
		}
	}
	
}




VOID BuffDoneCasting(int result)
{
	long time;

	time = GetTimeNow();

	echo("Buff Done, result: %i", result);

	switch (result)
	{
	// set recast time to right now on a fizzle, so next pulse it will attempt to cast again.
	// don't automatically recast here, since this gives macros and plugins the option of handling this themselves
	// in case something has changed in the last quarter second since starting the buffing, etc
	case CAST_FIZZLE:
		buffTargets[currentCastTarget].recastTimes[currentCastBuff] = time - 1;
		break;

	// should probably do some better error reporting for buffs which won't take hold...
	// for now, we just pretend they landed, so we don't chain cast them.
	case CAST_TAKEHOLD:
	case CAST_OUTDOORS:
	case CAST_SUCCESS:
		buffTargets[currentCastTarget].recastTimes[currentCastBuff] = time + (unsigned long)(BuffList[currentCastBuff].info.duration - BuffList[currentCastBuff].info.castTime);
		break;

	// out of range and all other errors will set the buff timer to 5 seconds from now, which delays trying again immediately.
	// this prevents chain casting in strange circumstances.
	case CAST_OUTOFRANGE:
	default:
		buffTargets[currentCastTarget].recastTimes[currentCastBuff] = time + 5;


	}

	afterCastFunction = 0;
}

PLUGIN_API bool SingleBuffs(bool inCombat)
{
	map <string, BuffTarget>::iterator it;
	map <string, SingleBuff>::iterator il;
	map <string, unsigned long>::iterator ir;
	unsigned long classList;
	double distance;
	unsigned long time;
	bool doneBuffs = true;
	int nGem;

	time = GetTimeNow();

	if (!GetCharInfo() || !GetCharInfo()->pSpawn
#ifndef LIVE
		|| !GetCharInfo()->pSpawn->pActorInfo
#endif
		)
	{
		// if we're not completely loaded in yet, do nothing
		return false;
	}

	RebuildBuffTargetsList();

	for (il = BuffList.begin(); il != BuffList.end(); il++)
	{
		if (il->second.info.spell && il->second.active && (!inCombat || il->second.inCombat))
		{
			for (it = buffTargets.begin(); it != buffTargets.end(); it++)
			{
				if (it->second.spawn && it->second.shouldBuff)
				{
					// buffing pets are checked against a different class list
					if (it->second.isPet)
					{
						classList = il->second.petclasses;
					}
					else
					{
						classList = il->second.classes;
					}
					// is this person in this of classes set up for this buff?
					if (isClass(it->second.classCheck, classList))
					{
						// Don't buff other people who are the same class as me - they can buff themselves
						if (it->second.spawn->SpawnID == GetCharInfo()->pSpawn->SpawnID
							|| it->second.spawn->Class != GetCharInfo()->pSpawn->Class)
						{
							// do we have enough mana?
							if (GetCharInfo2()->Mana > il->second.info.spell->Mana)
							{
								distance = GetDistance(GetCharInfo()->pSpawn, it->second.spawn);

								// are we close enough?
								if (distance < il->second.info.range || it->second.spawn->SpawnID == GetCharInfo()->pSpawn->SpawnID)
								{
									// TODO: Some buffs require line of sight... 
									// need to figure out some way to determine if this one does or not, and do a LOS test here.
									
									
									ir = it->second.recastTimes.find(il->second.name);
									// has this buff been cast before?
									if (ir == it->second.recastTimes.end())
									{
										// nope.. set up for first time cast
										it->second.recastTimes[il->second.name] = time-1;
										ir = it->second.recastTimes.find(il->second.name);
										if (ir == it->second.recastTimes.end())
										{
											error("This should never happen.");
											return false;
										}
									}


									// time to recast?
									if (time > ir->second)
									{
										doneBuffs = false;

										// can we cast it now?
										// if it's not memorized, this function will do that now.
										if (!readyToCastSpell(il->second.name))
										{
											continue;
										}
										// Finally got here... time to cast the buff!

										echo("Casting: %s on %s", il->second.name, it->second.spawn->Name);

										CallCast(il->second.name, it->second.spawn->SpawnID);

										strcpy(currentCastTarget, it->second.spawn->Name);
										strcpy(currentCastBuff, il->second.name);

										afterCastFunction = BuffDoneCasting;

										
										nGem = findGemForSpell(il->second.info.spell);
										if ((nGem+1) == buffGem)
										{
											waitingOnSpell = false;
										}

										if (doingBuffs == true && numberOfBuffsRemaining > 0)
										{
											numberOfBuffsRemaining--;
											if (numberOfBuffsRemaining <= 0)
											{
												numberOfBuffsRemaining = 0;
												doingBuffs = false;
											}
										}
										return true;
									}
								}
							}
						}
					}
				}
			}
		}
	}

	if (doneBuffs)
	{
		// all buffs finished
		doingBuffs = false;
	}

	return false;
}


// This is called every time MQ pulses
PLUGIN_API VOID OnPulse(VOID)
{	
	EQPlayer* tmpSpawn;
	long time = GetTimeNow();

	if (!GetCharInfo() || !GetCharInfo()->pSpawn
#ifndef LIVE
		|| !GetCharInfo()->pSpawn->pActorInfo
#endif
		)
	{
		// if we're not completely loaded in yet, do nothing
		return;
	}

	if (!setupMQ2Cast(false))
		return;



	if (ppTarget && pTarget && oldTarget != -1)
	{
		if (pTarget->Data.SpawnID == CastTargetSpawnID && GetSpellByID(GetCharInfo()->pSpawn->pActorInfo->CastingSpellID))
		{
			tmpSpawn = GetSpawnByID(oldTarget);
			if (tmpSpawn)
			{
				pTarget = tmpSpawn;
			}
			else
			{
				pTarget = 0;
			}

			oldTarget = -1;
		}
	}

	if (((((gbMoving) && ((PSPAWNINFO)pCharSpawn)->SpeedRun==0.0f) && (GetCharInfo()->pSpawn->
#ifndef LIVE
		pActorInfo->
#endif
		Mount ==  NULL )) || (fabs(FindSpeed((PSPAWNINFO)pCharSpawn)) > 0.0f )))
	{
		// if we're moving, do nothing
		return;
	}

	if (!isCastingIdle())
	{
		// still trying to cast - patience.
		return;
	}

	if (afterCastFunction)
	{

		afterCastFunction(GetCastResult());

		if (afterCastFunction)
		{
			// not done casting
			return;
		}
	}

	if (waitingOnSpell && !(pSpellBookWnd && (PCSIDLWND)pSpellBookWnd->Show))
		waitingOnSpell = false;


	if (!automate && !doingBuffs)
	{
		// auto-buffing is off... 
		// check for this after handling afterCastFunctions, so that manual calls to the autoBuff function still work.
		return;
	}

#ifdef LIVE
	if (GetSpellByID(GetCharInfo()->pSpawn->CastingSpellID))
#else
	if (GetSpellByID(GetCharInfo()->pSpawn->pActorInfo->CastingSpellID))
#endif
	{
		// if we're currently casting something, wait until we're done
		return;
	}

	SingleBuffs(false);
	
}
