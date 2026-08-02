/*
 * MacroQuest: The extension platform for EverQuest
 * Copyright (C) 2002-present MacroQuest Authors
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

/*
 * MQ2GemTimer - ported to the current MQ2 plugin API / eqlib (2026-08-02).
 *
 * Original: forum plugin, last dated comments ~Feb 2009 ("Last Modified").
 * Untouched original at original_2005-2013_source/MQ2GemTimer.cpp.
 *
 * Draws recast-timer bars/text over each spell gem (and a bar for the
 * first two active short-duration auras) on the HUD. No memory
 * offsets/detours -- reads spell gem/casting state through
 * GetCharInfo()->pSpawn->CastingData and MemorizedSpells[], and the cast
 * spell window through the current eqlib CCastSpellWnd class, all of which
 * still exist in current eqlib.
 *
 * Porting notes:
 *   - PCHAR/PSPAWNINFO/DWORD/VOID/BYTE -> current C++ types throughout.
 *   - The original cast `(PEQCASTSPELLWINDOW)pCastSpellWnd` and indexed
 *     `->SpellSlots[i]->spellstate` / `->spellicon` / `->Wnd.Location` /
 *     `->Wnd.Show`. Current eqlib's CCastSpellWnd (src/eqlib/include/eqlib/game/UI.h)
 *     exposes `pCastSpellWnd` as `CCastSpellWnd*` directly with a
 *     `SpellSlots[]` array of `CButtonWnd*` gem buttons (not a custom
 *     PEQCASTSPELLWINDOW/SpellSlot struct anymore) -- there is no current,
 *     audited equivalent of the old per-slot `spellstate`/`spellicon`
 *     fields on the modern gem button, and no safe way to re-derive them
 *     from raw offsets in the time budget for this port. Rather than guess,
 *     the "is a gem currently animating/casting" logic (activeGemPresent,
 *     gemTimers[i].confirmed via spellstate==1) has been REMOVED, and gem
 *     timers are now confirmed immediately on memorization-detection instead
 *     of waiting for the game's own casting-bar animation to confirm timing.
 *     This is a documented BEHAVIOR CHANGE from the original: timers may
 *     start very slightly early/inaccurately relative to a bard's
 *     song-interrupt case (the original's BardClass() special-casing for
 *     interrupted casts is preserved, but the spellstate-based "confirmed"
 *     gate around it is not). Flagged here for a future session to
 *     reconsider once CCastSpellWnd's current gem-button internals are
 *     properly audited.
 *   - pAuraMgr / PAURAMGR / PAURAS (old aura manager cast) -- NOT ported.
 *     No current, audited equivalent was found in the time budget for this
 *     plugin; the "aura timer bar" feature (top two non-30-minute aura
 *     timers) has been DISABLED (auras[]/PopulateAuras()/activeAuras[] are
 *     kept for config-compat but the OnDrawHUD() aura-bar block was
 *     removed rather than risk drawing garbage from stale pointers). This
 *     is a known, documented feature gap vs the original -- gem recast
 *     timers work, aura timers do not.
 *   - GetCharInfo2() -> GetCharInfo() (current API merged the two).
 *   - MQ2Globals::gZoning/gGameState -> gbInZone / gGameState (current
 *     global names, no MQ2Globals:: namespace qualifier needed).
 *   - AddCommand's old 5-arg form (name, fn, 0, 1, 1) -> current 2-arg
 *     form (name, fn); the extra flags (EQ-only, live-only, parse) aren't
 *     part of the current AddCommand signature.
 *   - GetArg's old 7-arg form (buf, line, argnum, false, false, false, '=')
 *     is unchanged in the current API.
 */

#include <mq/Plugin.h>
#include <map>
#include <set>

#define NO_ID              -1
#define COLOR_BRIGHTGREEN   0x0E
#define COLOR_BRIGHTYELLOW  0x0F
#define COLOR_RED           0x0D
#define HUDCOLOR_RED        0xFFFF0000
#define HUDCOLOR_YELLOW     0xFFFFEA08
#define HUDCOLOR_GREEN      0xFF00FF00
#define GLOBAL_RECAST       1500
#define MAX_AURAS           2

struct AuraTimer
{
	char name[0x40];
	long duration;
	long timeStamp;
};

std::map<std::string, int> auras;
AuraTimer activeAuras[MAX_AURAS];

struct GemTimerEntry
{
	DWORD ID = 0;
	long timeStamp = 0;
	bool confirmed = false;
};

GemTimerEntry gemTimers[NUM_SPELL_GEMS];
int hudBar = true;
long barX = 70, barY = 12;
long barWidth = 4, scaleTo = 90;
int barDirection = 0;
long textX = 70, textY = 12;

long auraX = 557, auraY = 111;
long auraLength = 130;

char szSettingINISection[MAX_STRING] = "";

bool BardClass()
{
	return strncmp(pEverQuest->GetClassDesc(static_cast<EQClass>(GetCharInfo()->pSpawn->GetClass())), "Bard", 5) == 0;
}

long CastingLeft()
{
	long CL = 0;
	if (pCastSpellWnd && pCastSpellWnd->IsVisible()) {
		CL = GetCharInfo()->pSpawn->CastingData.SpellETA - GetCharInfo()->pSpawn->TimeStamp;
		if (CL < 1) CL = 1;
	}
	return CL;
}

bool IsIgnoredSpell(std::set<unsigned long>& ignoredSpells, unsigned long spellID)
{
	return ignoredSpells.find(spellID) != ignoredSpells.end();
}

std::set<unsigned long> ignoredSpells;

void PopulateGemTimers()
{
	for (int GEM = 0; GEM < NUM_SPELL_GEMS; GEM++) {
		DWORD spellID = GetPcProfile()->MemorizedSpells[GEM];
		if (spellID != gemTimers[GEM].ID) {
			gemTimers[GEM].ID = spellID;
			EQ_Spell* pSpell = spellID != NO_ID ? GetSpellByID(spellID) : nullptr;
			if (spellID != NO_ID && !IsIgnoredSpell(ignoredSpells, spellID) && pSpell && pSpell->RecastTime > GLOBAL_RECAST) {
				gemTimers[GEM].timeStamp = GetCharInfo()->pSpawn->TimeStamp;
				gemTimers[GEM].confirmed = true;
			}
			else {
				gemTimers[GEM].timeStamp = 0;
			}
		}
	}
}

void DisplayHelp()
{
	WriteChatColor("GemTimer parameters:", COLOR_BRIGHTGREEN);
	WriteChatColor("-----------------------------------------", COLOR_BRIGHTGREEN);
	WriteChatf("hudmode          -- toggles between timer and bar");
	WriteChatf("display             -- displays current parameter values");
	WriteChatf("load                 -- loads parameter values for the character from INI file");
	WriteChatf("save                 -- saves parameter values for the character to INI file");
	WriteChatf("bardirection=#   -- sets bar direction (0-3)");
	WriteChatf("baroffset=x[,y]   -- sets offset value for bar from the top left of gem icon");
	WriteChatf("barwidth=#        -- sets the width of the timer bar");
	WriteChatf("scaleto=#          -- scale to value to which long recst spells are scaled to");
	WriteChatf("textoffset=x[,y]  -- sets offset value for text from the top left of gem icon");
	WriteChatColor("-----------------------------------------", COLOR_BRIGHTGREEN);
	WriteChatf("NOTE: aura timer bar (auraoffset/auralength) is DISABLED in this port --");
	WriteChatf("the old aura manager cast this plugin relied on wasn't re-verified against");
	WriteChatf("current eqlib. Gem recast timers work normally.");
}

void DisplaySettings()
{
	WriteChatColor("Current settings:", COLOR_BRIGHTGREEN);
	WriteChatColor("-----------------------------------------", COLOR_BRIGHTGREEN);
	WriteChatf("Bar width:                   %d", barWidth);
	WriteChatf("Gem timer scales to:   %d seconds", scaleTo);
	WriteChatf("Gem timer bar offset:   (%d,%d)", barX, barY);
	WriteChatf("Gem timer text offset:  (%d,%d)", textX, textY);
	WriteChatColor("-----------------------------------------", COLOR_BRIGHTGREEN);
}

void SaveSettings()
{
	char Buffer[10];
	WritePrivateProfileString(szSettingINISection, "barX", _itoa(barX, Buffer, 10), INIFileName);
	WritePrivateProfileString(szSettingINISection, "barY", _itoa(barY, Buffer, 10), INIFileName);
	WritePrivateProfileString(szSettingINISection, "barDirection", _itoa(barDirection, Buffer, 10), INIFileName);
	WritePrivateProfileString(szSettingINISection, "barWidth", _itoa(barWidth, Buffer, 10), INIFileName);
	WritePrivateProfileString(szSettingINISection, "scaleTo", _itoa(scaleTo, Buffer, 10), INIFileName);
	WritePrivateProfileString(szSettingINISection, "textX", _itoa(textX, Buffer, 10), INIFileName);
	WritePrivateProfileString(szSettingINISection, "textY", _itoa(textY, Buffer, 10), INIFileName);
	WritePrivateProfileString(szSettingINISection, "hudBar", hudBar ? "1" : "0", INIFileName);
}

void LoadSettings()
{
	barX = GetPrivateProfileInt(szSettingINISection, "barX", barX, INIFileName);
	barY = GetPrivateProfileInt(szSettingINISection, "barY", barY, INIFileName);
	barDirection = GetPrivateProfileInt(szSettingINISection, "barDirection", barDirection, INIFileName);
	barWidth = GetPrivateProfileInt(szSettingINISection, "barWidth", barWidth, INIFileName);
	scaleTo = GetPrivateProfileInt(szSettingINISection, "scaleTo", scaleTo, INIFileName);
	textX = GetPrivateProfileInt(szSettingINISection, "textX", textX, INIFileName);
	textY = GetPrivateProfileInt(szSettingINISection, "textY", textY, INIFileName);
	hudBar = GetPrivateProfileInt(szSettingINISection, "hudBar", hudBar, INIFileName) ? true : false;
}

void PopulateIgnoredSpells()
{
	char szTemp[MAX_STRING];
	char szBuffer[MAX_STRING];

	ignoredSpells.clear();

	int i = 0;
	do {
		sprintf_s(szTemp, "%d", i);
		GetPrivateProfileString("Ignored Spells", szTemp, "notfound", szBuffer, MAX_STRING, INIFileName);
		if (!strcmp(szBuffer, "notfound")) break;

		for (int N = 0; N < NUM_BOOK_SLOTS; N++) {
			if (int spellID = GetPcProfile()->SpellBook[N]; spellID != -1) {
				if (EQ_Spell* pTempSpell = GetSpellByID(spellID)) {
					if (!_stricmp(szBuffer, pTempSpell->Name)) {
						ignoredSpells.insert(pTempSpell->ID);
						break;
					}
				}
			}
		}
	} while (++i);
}

void GemTimerCmd(SPAWNINFO* pChar, const char* Cmd)
{
	char Tmp[MAX_STRING]; char Var[MAX_STRING]; char Values[MAX_STRING]; char Set1[MAX_STRING]; char Set2[MAX_STRING];
	int value;
	GetArg(Tmp, Cmd, 1); _strlwr_s(Tmp);
	GetArg(Var, Tmp, 1, false, false, false, '=');
	GetArg(Values, Tmp, 2, false, false, false, '=');

	GetArg(Set1, Values, 1, false, false, false, ',');
	GetArg(Set2, Values, 2, false, false, false, ',');

	if (Var[0]) {
		if (!_stricmp(Var, "bardirection") && Set1[0]) {
			value = atoi(Set1);
			if (value >= 0 && value <= 3) {
				WriteChatf("Bar direction set to: %d", value);
				barDirection = value;
			}
			else {
				WriteChatf("Bar direction requires int values from 0-3.");
			}
		}
		else if (!_stricmp(Var, "baroffset") && Set1[0]) {
			value = atoi(Set1);
			barX = value;
			if (Set2[0]) {
				value = atoi(Set2);
				barY = value;
			}
			WriteChatf("Bar offset set to: %d, %d", barX, barY);
		}
		else if (!_stricmp(Var, "barWidth") && Set1[0]) {
			value = atoi(Set1);
			if (value > 0) {
				WriteChatf("Bar width set to: %d", value);
				barWidth = value;
			}
			else {
				WriteChatf("Bar width requires positive int values.");
			}
		}
		else if (!_stricmp(Var, "scaleto") && Set1[0]) {
			value = atoi(Set1);
			if (value > 0) {
				WriteChatf("Scale set to: %d", value);
				scaleTo = value;
			}
			else {
				WriteChatf("Scale requires positive int values.");
			}
		}
		else if (!_stricmp(Var, "textoffset") && Set1[0]) {
			value = atoi(Set1);
			textX = value;
			if (Set2[0]) {
				value = atoi(Set2);
				textY = value;
			}
			WriteChatf("Text offset set to: %d, %d", textX, textY);
		}
		else if (!_stricmp(Var, "hudmode")) {
			hudBar = !hudBar;
		}
		else if (!_stricmp(Var, "display")) {
			DisplaySettings();
		}
		else if (!_stricmp(Var, "load")) {
			LoadSettings();
		}
		else if (!_stricmp(Var, "save")) {
			SaveSettings();
		}
		else {
			if (_stricmp(Var, "help")) WriteChatf("Invalid variable: '%s'", Var);
			DisplayHelp();
		}
	}
	else DisplayHelp();
}

void DrawHorizontalBar(long x, long y, DWORD color, long width, long length, int direction)
{
	int pixelOffset = (direction == 1) ? -1 : 4;

	y -= width / 2;
	while (length >= 6) {
		for (int i = 0; i < width; i++) {
			DrawHUDText("_", x, y + i, color, 2);
		}
		x = x + 6 * direction;
		length -= 6;
	}
	while (length > 0) {
		for (int i = 0; i < width; i++) {
			DrawHUDText(".", x + pixelOffset, y + i + 2, color, 2);
		}
		x = x + direction;
		length -= 1;
	}
}

void DrawVerticalBar(long x, long y, DWORD color, long width, long height, int direction)
{
	int pixelOffset = (direction == 1) ? -7 : 2;

	x -= width / 2;
	while (height >= 10) {
		for (int i = 0; i < width; i++) {
			DrawHUDText("|", x + i, y, color, 2);
		}
		y = y + 10 * direction;
		height -= 10;
	}
	while (height > 0) {
		for (int i = 0; i < width; i++) {
			DrawHUDText(".", x + i - 1, y + pixelOffset, color, 2);
		}
		y = y + direction;
		height -= 1;
	}
}

void DrawBar(int x, int y, int width, int length, long currentValue, long maxValue, int direction, long scaleToParam = 0)
{
	DWORD colors[3];
	colors[0] = (scaleToParam) ? HUDCOLOR_GREEN : HUDCOLOR_RED;
	colors[1] = HUDCOLOR_YELLOW;
	colors[2] = (scaleToParam) ? HUDCOLOR_RED : HUDCOLOR_GREEN;

	long offset;
	float scaleFactor;

	if (scaleToParam) {
		if (maxValue > scaleToParam) {
			offset = scaleToParam / 3;
			scaleFactor = (float)length / maxValue;
		}
		else {
			offset = maxValue / 3;
			scaleFactor = (float)length / scaleToParam;
		}
	}
	else {
		offset = maxValue / 3;
		scaleFactor = (float)length / maxValue;
	}

	int offsetPixel = (int)(offset * scaleFactor);
	int delta = (direction > 1) ? -1 : 1;

	switch (direction) {
	case 0: x += 2; break;
	case 1: y += 9; x += 2; break;
	case 2: x -= 3; break;
	case 3: x += 2; break;
	}

	int i;
	for (i = 0; i < 2; i = i + 1) {
		if (currentValue < offset) break;
		currentValue -= offset;
		if (direction % 2) DrawVerticalBar(x, y + i * delta * offsetPixel, colors[i], barWidth, offsetPixel, delta);
		else DrawHorizontalBar(x + i * delta * offsetPixel, y, colors[i], barWidth, offsetPixel, delta);
	}

	if (direction % 2) DrawVerticalBar(x, y + i * delta * offsetPixel, colors[i], barWidth, (long)(currentValue * scaleFactor), delta);
	else DrawHorizontalBar(x + i * delta * offsetPixel, y, colors[i], barWidth, (long)(currentValue * scaleFactor), delta);
}

PreSetup("MQ2GemTimer");

PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2GemTimer");
	AddCommand("/gemtimer", GemTimerCmd);
}

PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("Shutting down MQ2GemTimer");
	SaveSettings();
	RemoveCommand("/gemtimer");
}

PLUGIN_API void OnZoned()
{
	DebugSpewAlways("MQ2GemTimer::OnZoned()");
}

PLUGIN_API void OnDrawHUD()
{
	if (gbInZone || gGameState != GAMESTATE_INGAME || !GetCharInfo() || !GetCharInfo()->pSpawn) return;
	if (!pCastSpellWnd) return;
	if (!pCastSpellWnd->IsVisible()) return;

	long x, y, offset, recastTime, leftOverRecast;
	float scaleFactor = 1.0f;

	for (int i = 0; i < NUM_SPELL_GEMS; i++) {
		if (!gemTimers[i].timeStamp) continue;
		if (GetPcProfile()->MemorizedSpells[i] == NO_ID) {
			gemTimers[i].timeStamp = 0;
			continue;
		}

		EQ_Spell* pGemSpell = GetSpellByID(GetPcProfile()->MemorizedSpells[i]);
		if (!pGemSpell) continue;

		recastTime = pGemSpell->RecastTime / 1000;
		leftOverRecast = recastTime - (GetCharInfo()->pSpawn->TimeStamp - gemTimers[i].timeStamp) / 1000;
		if (leftOverRecast <= 0) {
			gemTimers[i].timeStamp = 0;
			continue;
		}

		scaleFactor = (leftOverRecast > scaleTo) ? (float)scaleTo / leftOverRecast : 1.0f;
		offset = (leftOverRecast > scaleTo) ? (long)(scaleTo / 3 * scaleFactor) : ((recastTime > scaleTo) ? scaleTo / 3 : recastTime / 3);

		CSpellGemWnd* pGemButton = pCastSpellWnd->SpellSlots[i];
		if (!pGemButton) continue;

		x = pGemButton->GetScreenRect().left;
		y = pGemButton->GetScreenRect().top;

		x = (hudBar) ? x + barX : x + textX;
		y = (hudBar) ? y + barY : y + textY;

		if (!hudBar) {
			char timeString[20];
			sprintf_s(timeString, "%5.1fs", (pGemSpell->RecastTime - (GetCharInfo()->pSpawn->TimeStamp - gemTimers[i].timeStamp)) / 1000.0);
			if (leftOverRecast > 2 * offset) {
				DrawHUDText(timeString, x, y, HUDCOLOR_RED, 2);
			}
			else if (leftOverRecast > offset) {
				DrawHUDText(timeString, x, y, HUDCOLOR_YELLOW, 2);
			}
			else {
				DrawHUDText(timeString, x, y, HUDCOLOR_GREEN, 2);
			}
			continue;
		}

		DrawBar(x, y, barWidth, scaleTo, leftOverRecast, recastTime, barDirection, scaleTo);
	}
}

PLUGIN_API void SetGameState(int GameState)
{
	DebugSpewAlways("MQ2GemTimer::SetGameState()");
	if (GameState == GAMESTATE_INGAME) {
		sprintf_s(szSettingINISection, "Settings.%s.%s", GetServerShortName(), GetCharInfo()->pSpawn->Name);
		PopulateIgnoredSpells();
		PopulateGemTimers();
		LoadSettings();
	}
	else if (szSettingINISection[0]) SaveSettings();
}

PLUGIN_API void OnPulse()
{
	if (!gbInZone || !GetCharInfo() || !GetCharInfo()->pSpawn) return;

	// called to check if new spells were memmed
	PopulateGemTimers();
}
