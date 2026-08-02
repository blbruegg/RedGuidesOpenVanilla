#include "../MQ2Plugin.h"

#define		NO_ID				-1
#define		COLOR_BRIGHTGREEN	0x0E
#define		COLOR_BRIGHTYELLOW	0x0F
#define		COLOR_RED			0x0D
#define		HUDCOLOR_RED		0xFFFF0000
#define		HUDCOLOR_YELLOW		0xFFFFEA08
#define		HUDCOLOR_GREEN		0xFF00FF00
#define		GLOBAL_RECAST		1500
#define		MAX_AURAS			2

#include <map>
#include <set>

struct strCmp {
	bool operator()(const char* s1, const char* s2) const {
		return strcmp( s1, s2 ) < 0;
	}
};

set<unsigned long> ignoredSpells;

struct AuraTimer {
	CHAR name[0x40];				// name of the aura
	long duration;					// duration in seconds, obtained from auras dictionary (30 min default)
	long timeStamp;					// timestamp for when the aura was first detected
} pAuraTimer;

map<const char*, int, strCmp> auras;	// dictionary containing the duration of auras (the non 30 min ones)
AuraTimer activeAuras[MAX_AURAS];

struct GemTimer {
	DWORD ID;						// spell's ID (used to see if new spell was memorized)
	long timeStamp;					// time stamp for spell completion
	bool confirmed;					// used to determine if spell cast was interrupted due to movement (not for bards obv)
} pGemTimer;

BYTE currentCastGem;				// last recorded spell gem that was cast
DWORD currentCastETA;				// ETA of last recorded spell gem cast completion

GemTimer gemTimers[NUM_SPELL_GEMS];
int hudBar = true;					// true = bar is displayed, false = time in seconds
long barX = 70, barY = 12;			// offset for timer bar from top left corner of the spell gem
long barWidth = 4, scaleTo = 90;	// width and scale setting for the spell gem timer bars
int barDirection = 0;				// 0 = right, 1 = down, 2 = left, 3 = up
long textX = 70, textY = 12;		// offset for time in seconds display from top left corner of the spell gem

long auraX = 557,auraY = 111;		// absolute position of the aura timer bar
long auraLength=130;				// max length of the aura bar

char szSettingINISection[MAX_STRING]="";

bool BardClass() {
	return (strncmp(pEverQuest->GetClassDesc(GetCharInfo2()->Class & 0xFF),"Bard",5))?false:true;
}

/********************************************************************************
*	Purpose:		Calculates estimated time remaining on spell cast			*
*	Last Modified:	Oct 17, 2008												*
*	Parameters:																	*
*	Return value:	remaining casting time in milliseconds						*
********************************************************************************/
long CastingLeft() {
  long CL=0;
  if(pCastingWnd && (PCSIDLWND)pCastingWnd->Show) {
    CL=GetCharInfo()->pSpawn->CastingData.SpellETA - GetCharInfo()->pSpawn->TimeStamp;
    if(CL<1) CL=1;
  }
  return CL;
}

/********************************************************************************
*	Purpose:		Helper function to determine is spell should be ignored		*
*	Last Modified:	Feb 14, 2009												*
*	Parameters:																	*
*		spellID			ID of the spell in question								*
*	Return value:	boolean indicating presence of spell in ignore list			*
********************************************************************************/
bool IsIgnoredSpell(unsigned long spellID) {
	return (ignoredSpells.find(spellID) != ignoredSpells.end());
}

/********************************************************************************
*	Purpose:		Populates gem timer array on startup, as well as maintains	*
*					timers when spell lineup changes							*
*	Last Modified:	Feb 14, 2009												*
*	Parameters:																	*
*	Return value:																*
********************************************************************************/
void PopulateGemTimers() {
	DWORD spellID;

	for(int GEM=0; GEM < NUM_SPELL_GEMS; GEM++) {
		spellID = GetCharInfo2()->MemorizedSpells[GEM];
		// if gem contains different spell than when it was last checked, update the timer struct for it
		if(spellID != gemTimers[GEM].ID) {
			gemTimers[GEM].ID = spellID;
			// if gem has a spell with a recast time, we create a timestamp for it
			if(spellID != NO_ID && !IsIgnoredSpell(spellID) && GetSpellByID(spellID)->RecastTime > GLOBAL_RECAST && 
				(long)((PEQCASTSPELLWINDOW)pCastSpellWnd)->SpellSlots[GEM]->spellicon==NO_ID) {
				gemTimers[GEM].timeStamp = GetCharInfo()->pSpawn->TimeStamp;
				gemTimers[GEM].confirmed = true;
			} else {
				gemTimers[GEM].timeStamp = 0;
			}
		}
	}
}

/********************************************************************************
*	Purpose:		Displays commandline parameters available for adjustment	*
*	Last Modified:	Oct 17, 2008												*
*	Parameters:																	*
*	Return value:																*
********************************************************************************/
void DisplayHelp() {
	WriteChatColor("GemTimer parameters:",COLOR_BRIGHTGREEN);
	WriteChatColor("-----------------------------------------",COLOR_BRIGHTGREEN);
	WriteChatf("hudmode          -- toggles between timer and bar");
	WriteChatf("display             -- displays current parameter values");
	WriteChatf("load                 -- loads parameter values for the character from INI file");
	WriteChatf("save                 -- saves parameter values for the character to INI file");
	WriteChatf("bardirection=#   -- sets bar direction (0-3)");
	WriteChatf("baroffset=x[,y]   -- sets offset value for bar from the top left of gem icon");
	WriteChatf("barwidth=#        -- sets the width of the timer bar");
	WriteChatf("scaleto=#          -- scale to value to which long recst spells are scaled to");
	WriteChatf("textoffset=x[,y]  -- sets offset value for text from the top left of gem icon");
	WriteChatf("auraoffset=x[,y] -- sets absoulte position of the aura bar");
	WriteChatf("auralength=#     -- sets the max length of the aura bar");
	WriteChatColor("-----------------------------------------",COLOR_BRIGHTGREEN);
}


/********************************************************************************
*	Purpose:		Parses /gemtimer parameters.								*
*					Yes I know, it's ugly, but I'm too lazy to research			*
*					the /mapfilter like parameter structure						*
*	Last Modified:	Oct 17, 2008												*
*	Parameters:																	*
*		pChar			spawninfo structure for character						*
*		Cmd				/gemtimer parameters									*
*	Return value:																*
********************************************************************************/
void DisplaySettings() {
	WriteChatColor("Current settings:",COLOR_BRIGHTGREEN);
	WriteChatColor("-----------------------------------------",COLOR_BRIGHTGREEN);
	WriteChatf("Bar width:                   %d",barWidth);
	WriteChatf("Gem timer scales to:   %d seconds",scaleTo);
	WriteChatf("Gem timer bar offset:   (%d,%d)",barX,barY);
	WriteChatf("Gem timer text offset:  (%d,%d)",textX,textY);
	WriteChatf("Aura bar position:        (%d,%d)",auraX,auraY);
	WriteChatf("Aura bar length:          %d",auraLength);
	WriteChatColor("-----------------------------------------",COLOR_BRIGHTGREEN);
}

/********************************************************************************
*	Purpose:		Saves relevant variables to MQ2GemTimer.ini for each		*
*					character													*
*	Last Modified:	Oct 17, 2008												*
*	Parameters:																	*
*	Return value:																*
********************************************************************************/
void SaveSettings() {
	char Buffer[10];
	WritePrivateProfileString(szSettingINISection,"barX",itoa(barX, Buffer,10),INIFileName);
	WritePrivateProfileString(szSettingINISection,"barY",itoa(barY, Buffer,10),INIFileName);
	WritePrivateProfileString(szSettingINISection,"barDirection",itoa(barDirection, Buffer,10),INIFileName);
	WritePrivateProfileString(szSettingINISection,"barWidth",itoa(barWidth, Buffer,10),INIFileName);
	WritePrivateProfileString(szSettingINISection,"scaleTo",itoa(scaleTo, Buffer,10),INIFileName);
	WritePrivateProfileString(szSettingINISection,"textX",itoa(textX, Buffer,10),INIFileName);
	WritePrivateProfileString(szSettingINISection,"textY",itoa(textY, Buffer,10),INIFileName);
	WritePrivateProfileString(szSettingINISection,"hudBar",hudBar?"1":"0",INIFileName);
	WritePrivateProfileString(szSettingINISection,"auraX",itoa(auraX, Buffer,10),INIFileName);
	WritePrivateProfileString(szSettingINISection,"auraY",itoa(auraY, Buffer,10),INIFileName);
	WritePrivateProfileString(szSettingINISection,"auraLength",itoa(auraLength, Buffer,10),INIFileName);
}

/********************************************************************************
*	Purpose:		Loads relevant variables from the character's section in	*
*					MQ2GemTimer.ini												*
*					character													*
*	Last Modified:	Oct 17, 2008												*
*	Parameters:																	*
*	Return value:																*
********************************************************************************/
void LoadSettings() {
	barX = GetPrivateProfileInt(szSettingINISection,"barX",barX,INIFileName);
	barY = GetPrivateProfileInt(szSettingINISection,"barY",barY,INIFileName);
	barDirection = GetPrivateProfileInt(szSettingINISection,"barDirection",barDirection,INIFileName);
	barWidth = GetPrivateProfileInt(szSettingINISection,"barWidth",barWidth,INIFileName);
	scaleTo = GetPrivateProfileInt(szSettingINISection,"scaleTo",scaleTo,INIFileName);
	textX = GetPrivateProfileInt(szSettingINISection,"textX",textX,INIFileName);
	textY = GetPrivateProfileInt(szSettingINISection,"textY",textY,INIFileName);
	hudBar = (GetPrivateProfileInt(szSettingINISection,"hudBar",hudBar,INIFileName))?TRUE:FALSE;
	auraX = GetPrivateProfileInt(szSettingINISection,"auraX",auraX,INIFileName);
	auraY = GetPrivateProfileInt(szSettingINISection,"auraY",auraY,INIFileName);
	auraLength = GetPrivateProfileInt(szSettingINISection,"auraLength",auraLength,INIFileName);
}

void DisplayColors() {
	char temp[MAX_STRING];
	// rest of the colors are the same gray as 19
	for(int i=0;i<21;i++) {
		sprintf(temp,"color: %d",i);
		WriteChatColor(temp,i);
	}
}

/********************************************************************************
*	Purpose:		Populates the non 30 minute auras to store their durations	*
*	Last Modified:	Feb 14, 2009												*
*	Parameters:																	*
*	Return value:																*
********************************************************************************/
void PopulateAuras() {
	auras["Cirle of Power"] = 120000;
	auras["Guardian Circle"] = 120000;
	auras["Circle of Life"] = 120000;
	auras["Wake of Atrophy Aura"] = 120000;
	auras["Mana Reiterate Aura"] = 360000;
	auras["Mana Resurgence Aura"] = 360000;
	auras["Runic Shimmer Aura"] = 360000;
}

/********************************************************************************
*	Purpose:		Loads ignored spells from INI file, and stores the ones		*
*					found in spellbook											*
*	Last Modified:	Feb 14, 2009												*
*	Parameters:																	*
*	Return value:																*
********************************************************************************/
void PopulateIgnoredSpells() {
	CHAR szTemp[MAX_STRING];
	CHAR szBuffer[MAX_STRING];

	ignoredSpells.clear();

	int i = 0;
	do {
		sprintf(szTemp, "%d", i);
		GetPrivateProfileString("Ignored Spells", szTemp, "notfound", szBuffer, MAX_STRING, INIFileName);
		if(!strcmp(szBuffer, "notfound")) break;

		GetCharInfo2()->SpellBook;
		PSPELL pSpell=0;
		for (DWORD N = 0 ; N < NUM_BOOK_SLOTS ; N++)
			if (PSPELL pTempSpell=GetSpellByID(GetCharInfo2()->SpellBook[N]))
			{
				// found ignored spell in spell book, add to list
				if (!stricmp(szBuffer,pTempSpell->Name))
				{
					ignoredSpells.insert(pTempSpell->ID);
					break;
				}
			}
	} while(++i);
}

/********************************************************************************
*	Purpose:		Parses /gemtimer parameters.								*
*					Yes I know, it's ugly, but I'm too lazy to research			*
*					the /mapfilter like parameter structure						*
*	Last Modified:	Oct 17, 2008												*
*	Parameters:																	*
*		pChar			spawninfo structure for character						*
*		Cmd				/gemtimer parameters									*
*	Return value:																*
********************************************************************************/
void GemTimer(PSPAWNINFO pChar, PCHAR Cmd) {
	char Tmp[MAX_STRING]; char Var[MAX_STRING]; char Values[MAX_STRING]; char Set1[MAX_STRING]; char Set2[MAX_STRING]; BYTE Parm=1; bool Help=true;
	int value;
	GetArg(Tmp,Cmd,Parm++); _strlwr(Tmp);
	GetArg(Var,Tmp,1,FALSE,FALSE,FALSE,'=');
	GetArg(Values,Tmp,2,FALSE,FALSE,FALSE,'=');

	GetArg(Set1,Values,1,FALSE,FALSE,FALSE,',');
	GetArg(Set2,Values,2,FALSE,FALSE,FALSE,',');

	if (Var[0]) {
		if(!stricmp(Var,"bardirection") && Set1[0]) {
			value = atoi(Set1);
			if (value >= 0 && value <= 3) {
				WriteChatf("Bar direction set to: %d",value);
				barDirection = value;
			} else {
				WriteChatf("Bar direction requires int values from 0-3.");
			}
		} else if(!stricmp(Var,"baroffset") && Set1[0]) {
			value = atoi(Set1);
			barX = value;
			if(Set2[0]) {
				value = atoi(Set2);
				barY = value;
			}
			WriteChatf("Bar offset set to: %d, %d",barX,barY);
		} else if(!stricmp(Var,"barWidth") && Set1[0]) {
			value = atoi(Set1);
			if (value > 0) {
				WriteChatf("Bar width set to: %d",value);
				barWidth = value;
			} else {
				WriteChatf("Bar width requires positive int values.");
			}
		} else if(!stricmp(Var,"scaleto") && Set1[0]) {
			value = atoi(Set1);
			if (value > 0) {
				WriteChatf("Scale set to: %d",value);
				scaleTo = value;
			} else {
				WriteChatf("Scale requires positive int values.");
			}
		} else if(!stricmp(Var,"textoffset") && Set1[0]) {
			value = atoi(Set1);
			textX = value;
			if(Set2[0]) {
				value = atoi(Set2);
				textY = value;
			}
			WriteChatf("Text offset set to: %d, %d",textX,textY);
		} else if(!stricmp(Var,"auraoffset") && Set1[0]) {
			value = atoi(Set1);
			auraX = value;
			if(Set2[0]) {
				value = atoi(Set2);
				auraY = value;
			}
			WriteChatf("Aura position set to: %d, %d",auraX,auraY);
		} else if(!stricmp(Var,"auralength") && Set1[0]) {
			value = atoi(Set1);
			if (value > 0) {
				WriteChatf("Aura length set to: %d",value);
				auraLength = value;
			} else {
				WriteChatf("Aura length requires positive int values.");
			}
		} else if(!stricmp(Var,"hudmode")) {
			hudBar = !hudBar;
		} else if(!stricmp(Var,"display")) {
			DisplaySettings();
		} else if(!stricmp(Var,"load")) {
			LoadSettings();
		} else if(!stricmp(Var,"save")) {
			SaveSettings();
		} else if(!stricmp(Var,"color")) {
			DisplayColors();
		} else if(!stricmp(Var,"test")) {
			PopulateIgnoredSpells();
		} else {
			if(stricmp(Var,"help")) WriteChatf("Invalid variable: '%s'",Var);
			DisplayHelp();
		}
	} else DisplayHelp();
}

/********************************************************************************
*	Purpose:		Draws vertical bar on HUD using _ and . characters			*
*	Last Modified:	Oct 17, 2008												*
*	Parameters:																	*
*		x				x coordinate of the bar									*
*		y				y coordinate of the bar									*
*		width			width of the bar										*
*		length			length of the bar										*
*		direction		1 - right, -1 - left									*
*	Return value:																*
********************************************************************************/
void DrawHorizontalBar(long x, long y, DWORD color, long width, long length, int direction) {
	int pixelOffset = (direction==1)?-1:4;

	y-=width/2;
	while(length >= 6) {
		for (int i=0;i<width;i++) {
			DrawHUDText("_",x,y+i,color,2);
		}
		x=x+6*direction;
		length-=6;
	}
	while(length > 0) {
		for (int i=0;i<width;i++) {
			DrawHUDText(".",x+pixelOffset,y+i+2,color,2);
		}
		x=x+direction;
		length-=1;
	}	
}

/********************************************************************************
*	Purpose:		Draws vertical bar on HUD using | and . characters			*
*	Last Modified:	Oct 17, 2008												*
*	Parameters:																	*
*		x				x coordinate of the bar									*
*		y				y coordinate of the bar									*
*		width			width of the bar										*
*		height			height of the bar										*
*		direction		1 - down, -1 - up										*
*	Return value:																*
********************************************************************************/
void DrawVerticalBar(long x, long y, DWORD color, long width, long height, int direction) {
	int pixelOffset = (direction==1)?-7:2;

	x-=width/2;
	while(height >= 10) {
		for (int i=0;i<width;i++) {
			DrawHUDText("|",x+i,y,color,2);
		}
		y=y+10*direction;
		height-=10;
	}
	while(height > 0) {
		for (int i=0;i<width;i++) {
			DrawHUDText(".",x+i-1,y+pixelOffset,color,2);
		}
		y=y+direction;
		height-=1;
	}
}

/********************************************************************************
*	Purpose:		Draws tri-color timer bar									*
*	Last Modified:	Oct 17, 2008												*
*	Parameters:																	*
*		x				x coordinate of the bar									*
*		y				y coordinate of the bar									*
*		width			width of the bar										*
*		length			maximum length available for the bar					*
*		maxValue		max value for the variable, used to	determine color		*
*						change intervals										*
*		currentValue	value of the scale to be displayed						*
*		direction		0 - right, 1 - down, 2 - left, 3 - right				*
*		scaleTo			if currentValue > scaleTo, more red is shown to			*
*						indicate longer recast time (used for DA and such)		*
*	Return value:		none													*
********************************************************************************/
void DrawBar(int x, int y, int width, int length, long currentValue, long maxValue, int direction, long scaleTo=0) {
	DWORD colors[3];
	colors[0]=(scaleTo)?HUDCOLOR_GREEN:HUDCOLOR_RED;
	colors[1]=HUDCOLOR_YELLOW;
	colors[2]=(scaleTo)?HUDCOLOR_RED:HUDCOLOR_GREEN;

	long offset;		// length of green or yellow segment representation in seconds
	float scaleFactor;	// the number of pixels that represents one second

	// figure out offset and scaleFactor based on the 3 scenarios
	// 1 -	|G|Y|RRRRRRRRRRRRRRRR|		ie: long recast spell (DA) scaled to 90 seconds
	// 2 -	|GG|YY|RR|           |		ie: short recast spell scaled to 90 seconds, colors scaled to recast time
	// 3 -  |RRRRRR|YYYYYY|GGGGGG|		ie: evenly distributed colors for auras, colors scaled to bar length
	if (scaleTo) {
		if(maxValue > scaleTo) {
			offset = scaleTo / 3;
			scaleFactor = (float) length/maxValue;
		} else {
			offset = maxValue / 3;
			scaleFactor = (float) length/scaleTo;
		}
	} else {
		offset = maxValue / 3;
		scaleFactor = (float) length/maxValue;
	}

	int offsetPixel = offset * scaleFactor;
	int delta = (direction>1)?-1:1;

	// minor position adjustments to bar positions so when bar direction is changed,
	// it pivots around roughly the same point
	switch (direction) {
	case 0:
		x+=2;
		break;
	case 1:
		y+=9;
		x+=2;
		break;
	case 2:
		x-=3;
		break;
	case 3:
		x+=2;
		break;
	}

	int i;
	for (i = 0; i< 2; i=i+1) {
		if (currentValue < offset) break;
		currentValue-=offset;
		if(direction%2) DrawVerticalBar(x,y+i*delta*offsetPixel,colors[i],barWidth, offsetPixel, delta);
		else DrawHorizontalBar(x+i*delta*offsetPixel,y,colors[i],barWidth, offsetPixel, delta);
	}

	if(direction%2) DrawVerticalBar(x,y+i*delta*offsetPixel,colors[i],barWidth, currentValue * scaleFactor, delta);
	else DrawHorizontalBar(x+i*delta*offsetPixel,y,colors[i],barWidth, currentValue * scaleFactor, delta);
}

void DrawBars(int x, int y, int width, int length, long currentValue, long maxValue, long scaleTo=0) {
	DrawBar(x,y,width,length,currentValue,maxValue,0,scaleTo);
	DrawBar(x,y,width,length,currentValue,maxValue,1,scaleTo);
	DrawBar(x,y,width,length,currentValue,maxValue,2,scaleTo);
	DrawBar(x,y,width,length,currentValue,maxValue,3,scaleTo);
}

PreSetup("MQ2GemTimer");

// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializePlugin(VOID)
{
	DebugSpewAlways("Initializing MQ2GemTimer");
	AddCommand("/gemtimer",GemTimer,0,1,1);
	PopulateAuras();
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownPlugin(VOID)
{
	DebugSpewAlways("Shutting down MQ2GemTimer");
	SaveSettings();
	RemoveCommand("/gemtimer");
}

// Called after entering a new zone
PLUGIN_API VOID OnZoned(VOID)
{
	DebugSpewAlways("MQ2GemTimer::OnZoned()");
}

// Called once directly before shutdown of the new ui system, and also
// every time the game calls CDisplay::CleanGameUI()
PLUGIN_API VOID OnCleanUI(VOID)
{
	DebugSpewAlways("MQ2GemTimer::OnCleanUI()");
	// destroy custom windows, etc
}

// Called once directly after the game ui is reloaded, after issuing /loadskin
PLUGIN_API VOID OnReloadUI(VOID)
{
	DebugSpewAlways("MQ2GemTimer::OnReloadUI()");
	// recreate custom windows, etc
}

/********************************************************************************
*	Purpose:		Called every frame that the "HUD" is drawn --				*
*						e.g. net status / packet loss bar						*
*	Last Modified:	Feb 14, 2009												*
*	Parameters:																	*
*	Return value:		none													*
********************************************************************************/
PLUGIN_API VOID OnDrawHUD(VOID)
{
	if (MQ2Globals::gZoning || MQ2Globals::gGameState != GAMESTATE_INGAME || !GetCharInfo2() || !GetCharInfo() || !GetCharInfo()->pSpawn) return;
	if (!pCastSpellWnd) return;

	long x, y, offset, recastTime, leftOverRecast;
	float scaleFactor = 1.0f;
	bool activeGemPresent = false;
	int i, direction = 1;

	DWORD colors[3];
	colors[0]=(scaleTo)?HUDCOLOR_GREEN:HUDCOLOR_RED;
	colors[1]=HUDCOLOR_YELLOW;
	colors[2]=(scaleTo)?HUDCOLOR_RED:HUDCOLOR_GREEN;
	
	if(PAURAMGR pAura=(PAURAMGR)pAuraMgr) {
		PAURAS pAuras = (PAURAS)(*pAura->pAuraInfo);
		if(pAura->NumAuras) {
			char *auraName = pAuras[0].Aura->Name;
			// top aura has changed, update structure
			if(strcmp(activeAuras[0].name,auraName)) {
				strcpy(activeAuras[0].name,auraName);
				activeAuras[0].duration = auras[auraName]?auras[auraName]:1800000;
				if (activeAuras[1].timeStamp) {
					// bump up timestamp from slot 2 to top slot
					activeAuras[0].timeStamp = activeAuras[1].timeStamp;
				} else {
					// no 2nd aura active, this aura was freshly casted
					activeAuras[0].timeStamp = GetCharInfo()->pSpawn->TimeStamp;
				}
			}
			// clear bottom timestamp when aura moves to top slot
			if(pAura->NumAuras == 1 && !activeAuras[2].timeStamp) activeAuras[1].timeStamp = 0;
		} else if (activeAuras[0].timeStamp) {
			activeAuras[0].timeStamp = 0;
			strcpy(activeAuras[0].name,"");
		}
		if(pAura->NumAuras == 2 && !activeAuras[1].timeStamp) {
			// new aura added to slot 2, we do not know what aura it is, but timestamp is saved
			activeAuras[1].timeStamp = GetCharInfo()->pSpawn->TimeStamp;
		}
	}

	if(activeAuras[0].timeStamp) {
		leftOverRecast = activeAuras[0].duration - GetCharInfo()->pSpawn->TimeStamp + activeAuras[0].timeStamp;
		DrawBar(auraX,auraY,barWidth,auraLength,leftOverRecast,activeAuras[0].duration,0);
	}

	if(!((PEQCASTSPELLWINDOW)pCastSpellWnd)->Wnd.Show) return;

	for(i = 0; i < NUM_SPELL_GEMS; i++) {
		if ((long)((PEQCASTSPELLWINDOW)pCastSpellWnd)->SpellSlots[i]->spellstate!=1 || BardClass()) {
			activeGemPresent = true;
			break;
		}
	}

	for(i = 0; i < NUM_SPELL_GEMS; i++) {
		if (!gemTimers[i].timeStamp) continue;
		if(GetCharInfo2()->MemorizedSpells[i] == NO_ID) {
			gemTimers[i].timeStamp = 0;
			continue;
		}
		if(!gemTimers[i].confirmed) {
			if (activeGemPresent && (long)((PEQCASTSPELLWINDOW)pCastSpellWnd)->SpellSlots[i]->spellstate==1) gemTimers[i].confirmed = true;
			else {
				continue;
			}
		}

		recastTime = GetSpellByID(GetCharInfo2()->MemorizedSpells[i])->RecastTime/1000;
		leftOverRecast = recastTime-(GetCharInfo()->pSpawn->TimeStamp-gemTimers[i].timeStamp)/1000;
		if (leftOverRecast <=0) {
			gemTimers[i].timeStamp = 0;
			continue;
		}

		scaleFactor = (leftOverRecast > scaleTo) ? (float) scaleTo / leftOverRecast:1.0f;
		offset = (leftOverRecast > scaleTo) ? scaleTo/3*scaleFactor:((recastTime > scaleTo)? scaleTo/ 3:recastTime/3);

		x = ((PEQCASTSPELLWINDOW)pCastSpellWnd)->SpellSlots[i]->Wnd.Location.left + ((PEQCASTSPELLWINDOW)pCastSpellWnd)->Wnd.Location.left;
		y = ((PEQCASTSPELLWINDOW)pCastSpellWnd)->SpellSlots[i]->Wnd.Location.top + ((PEQCASTSPELLWINDOW)pCastSpellWnd)->Wnd.Location.top;

		x = (hudBar)?x+barX:x+textX;
		y = (hudBar)?y+barY:y+textY;

		if(!hudBar) {
			char timeString[20];
			sprintf(timeString, "%5.1fs",(GetSpellByID(GetCharInfo2()->MemorizedSpells[i])->RecastTime -(GetCharInfo()->pSpawn->TimeStamp-gemTimers[i].timeStamp))/1000.0);
			if(leftOverRecast > 2*offset) {
				DrawHUDText(timeString,x,y,HUDCOLOR_RED,2);
			} else if (leftOverRecast > offset) {
				DrawHUDText(timeString,x,y,HUDCOLOR_YELLOW,2);
			} else {
				DrawHUDText(timeString,x,y,HUDCOLOR_GREEN,2);
			}
			continue;
		}
		
		DrawBar(x,y,barWidth,scaleTo,leftOverRecast,recastTime,barDirection,scaleTo);
	}
}

// Called once directly after initialization, and then every time the gamestate changes
// shouldn't be loading/saving ini file here, but it needs to be loaded / saved when toons are logged in / camped out
// and gem timers need to be initialized once a toon loads into the world
PLUGIN_API VOID SetGameState(DWORD GameState)
{
	DebugSpewAlways("MQ2GemTimer::SetGameState()");
	if (GameState==GAMESTATE_INGAME) {
		sprintf(szSettingINISection,"Settings.%s.%s",EQADDR_SERVERNAME,GetCharInfo()->pSpawn->Name);
		currentCastGem = GetCharInfo()->pSpawn->CastingData.SpellSlot;
		currentCastETA = GetCharInfo()->pSpawn->CastingData.SpellETA;
		PopulateIgnoredSpells();
		PopulateGemTimers();
		LoadSettings();
	} else if(szSettingINISection[0]) SaveSettings();
}


// This is called every time MQ pulses
PLUGIN_API VOID OnPulse(VOID)
{
	if(!gbInZone || !GetCharInfo() || !GetCharInfo()->pSpawn) return;

	BYTE castingGem = GetCharInfo()->pSpawn->CastingData.SpellSlot;
	DWORD castingID = GetCharInfo()->pSpawn->CastingData.SpellID;

	// called to check if new spells were memmed
	PopulateGemTimers();

	// SpellSlot == NUM_SPELL_GEMS indicates item click
	if (castingGem != 0xFF && castingGem >= NUM_SPELL_GEMS) {
		return;
	}

	if (castingGem != currentCastGem) {
		// deal with spell cast completion
		if (currentCastGem != 0xFF && !IsIgnoredSpell(gemTimers[currentCastGem].ID) && GetSpellByID(gemTimers[currentCastGem].ID)->RecastTime > GLOBAL_RECAST) {
			// if casting completed or bard song is interrupted, start the timer for the gem
			if(GetCharInfo()->pSpawn->TimeStamp >= currentCastETA || BardClass()) {
				gemTimers[currentCastGem].timeStamp = GetCharInfo()->pSpawn->TimeStamp;
				gemTimers[currentCastGem].confirmed = (BardClass())?true:false;
			}
		}
		// update variables to reflect the current spell (or lackthereof) being cast
		currentCastGem = castingGem;
		currentCastETA = 0;
		if (castingGem != 0xFF)	currentCastETA = GetCharInfo()->pSpawn->TimeStamp + CastingLeft();
	}
}
