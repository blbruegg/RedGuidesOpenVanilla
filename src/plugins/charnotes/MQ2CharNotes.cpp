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
 * MQ2CharNotes - ported to the current MQ2 plugin API / eqlib (2026-08-02).
 *
 * Original: author Psycotic, v0.9. macroquest2.com/phpBB3
 * viewtopic.php?f=50&t=12783 ("PLUGIN: MQ2CharNotes - Adds a comment to a
 * PC/NPC"), Feb 2006. Untouched original at
 * original_2005-2013_source/MQ2CharNotes.cpp.
 *
 * Lets you attach a free-text note to a PC/NPC name, shown in chat when
 * that name spawns/zones in and in the HUD when it's your current target.
 * No memory offsets/detours -- purely command/INI/chat/HUD API, so this is
 * a straightforward signature-modernization port.
 *
 * Porting notes:
 *   - PCHAR/PSPAWNINFO/BOOL/VOID -> current C++ types throughout.
 *   - DrawHUDText gained a required Font size parameter in the current API
 *     (src/main/MQ2Main.h) -- the original had no font control at all;
 *     passed a reasonable default (0, "use HUD's own default size", matching
 *     how MQ2HUD.cpp callers pass pElement->Size verbatim without a
 *     magic-number guess of their own) is not available here since this
 *     plugin has no per-note font config, so 0 is used and documented as a
 *     behavior difference the user may want to make configurable later.
 *   - The global `pTarget` (SPAWNINFO*) is unchanged and used exactly like
 *     the original's `(PSPAWNINFO)pTarget` cast, minus the now-unnecessary
 *     cast and the old `ppTarget` double-pointer-validity check (current
 *     `pTarget` is a plain pointer, so a null check on `pTarget` alone is
 *     the modern equivalent of the old `pTarget && ppTarget` pattern).
 *   - AddCommand's modern std::function-based signature accepts a free
 *     function of (SPAWNINFO*, const char*) without change.
 *   - MAX_NOTES/CharNotesX/CharNotesY read via GetPrivateProfileString with
 *     no fallback in the original (NULL default -> atoi("") == 0); kept
 *     that exact behavior (config-required-in-INI) rather than silently
 *     adding defaults, since that's a functional choice for the user, not
 *     an API-compat fix -- but this means a fresh install has MAX_NOTES==0
 *     until MQCharNotes.ini's [Config] section is populated by hand, same
 *     as the 2006 original.
 */

#include <mq/Plugin.h>

PreSetup("MQ2CharNotes");
PLUGIN_VERSION(0.9);

constexpr int CHAT = 0;
constexpr int HUD = 1;

static char szTemp[MAX_STRING];
static bool ShowInChat = false;
static bool ShowInHUD = false;
static int MAX_NOTES = 0;
static int CharNotesX = 0;
static int CharNotesY = 0;
static int CurNote = 0;

struct CharNoteList
{
	char Name[35];
	char Note[90];
};

static CharNoteList CharNote[MAX_STRING];

static void ShowHelp()
{
	WriteChatColor("CharNotes - Usage", CONCOLOR_YELLOW);
	WriteChatColor("'/charnote This note will be displayed for the target'", CONCOLOR_YELLOW);
	WriteChatColor("'/charnote -delete'      (Will 'clear' the comment for current target)", CONCOLOR_YELLOW);
	WriteChatColor("'/charnotepos <x> <y>'   (X,Y display location on HUD)", CONCOLOR_YELLOW);
}

static void ClearArray()
{
	CurNote = 0;
	for (int i = 1; i <= MAX_NOTES && i < MAX_STRING; i++)
	{
		CharNote[i].Name[0] = '\0';
		CharNote[i].Note[0] = '\0';
	}
}

static void SaveINIArray()
{
	char NameArray[MAX_STRING];
	char NoteArray[MAX_STRING];

	for (int i = 1; i <= CurNote; i++)
	{
		sprintf_s(NameArray, "Name%i", i);
		sprintf_s(NoteArray, "Note%i", i);
		WritePrivateProfileString("SPAWNS", NameArray, CharNote[i].Name, INIFileName);
		WritePrivateProfileString("SPAWNS", NoteArray, CharNote[i].Note, INIFileName);
	}
}

static void LoadINIArray()
{
	char NameArray[MAX_STRING];
	char NoteArray[MAX_STRING];
	char Name[MAX_STRING];
	char Note[MAX_STRING];

	ClearArray();
	for (int i = 1; i <= MAX_NOTES && i < MAX_STRING; i++)
	{
		sprintf_s(NameArray, "Name%i", i);
		sprintf_s(NoteArray, "Note%i", i);
		GetPrivateProfileString("SPAWNS", NameArray, "", Name, MAX_STRING, INIFileName);
		GetPrivateProfileString("SPAWNS", NoteArray, "", Note, MAX_STRING, INIFileName);
		if (Name[0] != '\0')
		{
			strcpy_s(CharNote[i].Name, Name);
			strcpy_s(CharNote[i].Note, Note);
			CurNote = i;
		}
		else
		{
			return;
		}
	}
}

static void LoadConfig()
{
	GetPrivateProfileString("Config", "ShowInChat", "on", szTemp, MAX_STRING, INIFileName);
	ShowInChat = (strcmp(szTemp, "on") == 0);

	GetPrivateProfileString("Config", "ShowInHUD", "on", szTemp, MAX_STRING, INIFileName);
	ShowInHUD = (strcmp(szTemp, "on") == 0);

	GetPrivateProfileString("Config", "CharNotesPosX", "", szTemp, MAX_STRING, INIFileName);
	CharNotesX = atoi(szTemp);

	GetPrivateProfileString("Config", "CharNotesPosY", "", szTemp, MAX_STRING, INIFileName);
	CharNotesY = atoi(szTemp);

	GetPrivateProfileString("Config", "MaxNotes", "", szTemp, MAX_STRING, INIFileName);
	MAX_NOTES = atoi(szTemp);
	if (MAX_NOTES < 0) MAX_NOTES = 0;
	if (MAX_NOTES >= MAX_STRING) MAX_NOTES = MAX_STRING - 1;
}

static void DisplayCharNote(bool chat, const char* szLine)
{
	for (int i = 1; i <= MAX_NOTES && i < MAX_STRING; i++)
	{
		if (strcmp(szLine, CharNote[i].Name) == 0 && CharNote[i].Note[0] != '\0')
		{
			sprintf_s(szTemp, "%s - %s", CharNote[i].Name, CharNote[i].Note);
			if (!chat)
			{
				WriteChatColor(szTemp, CONCOLOR_YELLOW);
			}
			else
			{
				DrawHUDText(szTemp, CharNotesX, CharNotesY, 0xFFFFFFFF, 0);
			}
		}
	}
}

static void DoCharNotePos(SPAWNINFO* pChar, const char* szLine)
{
	char szArg1[MAX_STRING] = { 0 };
	char szArg2[MAX_STRING] = { 0 };

	GetArg(szArg1, szLine, 1);
	GetArg(szArg2, szLine, 2);

	if (strlen(szArg2))
	{
		WritePrivateProfileString("Config", "CharNotesPosX", szArg1, INIFileName);
		WritePrivateProfileString("Config", "CharNotesPosY", szArg2, INIFileName);
		CharNotesX = atoi(szArg1);
		CharNotesY = atoi(szArg2);
	}
	else
	{
		WriteChatColor("You must enter Xpos and Ypos in the format \"/charnotepos 500 200\"", CONCOLOR_YELLOW);
	}
}

static void DoCharNote(SPAWNINFO* pChar, const char* szLine)
{
	char NameArray[MAX_STRING];
	char NoteArray[MAX_STRING];

	if (CurNote >= MAX_NOTES)
	{
		sprintf_s(szTemp, "The maximum number of notes (%i) have been added.  Please increase MAX_NOTES to increase", MAX_NOTES);
		WriteChatColor(szTemp);
		return;
	}

	if (pTarget && strlen(szLine))
	{
		SPAWNINFO* psTarget = pTarget;
		for (int i = 1; i <= CurNote; i++)
		{
			if (strcmp(CharNote[i].Name, psTarget->DisplayedName) == 0)
			{
				if (strcmp(szLine, "-delete") == 0)
				{
					CharNote[i].Note[0] = '\0';
					sprintf_s(NoteArray, "Note%i", i);
					WritePrivateProfileString("SPAWNS", NoteArray, CharNote[i].Note, INIFileName);
					return;
				}

				strcpy_s(CharNote[i].Note, szLine);
				sprintf_s(NoteArray, "Note%i", i);
				WritePrivateProfileString("SPAWNS", NoteArray, CharNote[i].Note, INIFileName);
				return;
			}
		}

		if (CurNote + 1 >= MAX_STRING)
		{
			WriteChatColor("CharNotes storage is full.", CONCOLOR_YELLOW);
			return;
		}

		CurNote++;
		sprintf_s(NameArray, "Name%i", CurNote);
		sprintf_s(NoteArray, "Note%i", CurNote);
		strcpy_s(CharNote[CurNote].Name, psTarget->DisplayedName);
		strcpy_s(CharNote[CurNote].Note, szLine);
		WritePrivateProfileString("SPAWNS", NameArray, CharNote[CurNote].Name, INIFileName);
		WritePrivateProfileString("SPAWNS", NoteArray, CharNote[CurNote].Note, INIFileName);
		DisplayCharNote(CHAT, CharNote[CurNote].Name);
	}
	else
	{
		ShowHelp();
	}
}

PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2CharNotes");
	AddCommand("/charnote", DoCharNote);
	AddCommand("/charnotepos", DoCharNotePos);
	LoadConfig();
	LoadINIArray();
}

PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("Shutting down MQ2CharNotes");
	RemoveCommand("/charnote");
	RemoveCommand("/charnotepos");
	SaveINIArray();
}

PLUGIN_API void OnDrawHUD()
{
	if (ShowInHUD)
	{
		if (pTarget)
		{
			DisplayCharNote(HUD, pTarget->DisplayedName);
		}
	}
}

PLUGIN_API void OnAddSpawn(SPAWNINFO* pNewSpawn)
{
	if (ShowInChat)
	{
		if (pNewSpawn->Type != SPAWN_CORPSE)
			DisplayCharNote(CHAT, pNewSpawn->DisplayedName);
	}
}

PLUGIN_API void OnZoned()
{
	SaveINIArray();
}

PLUGIN_API void SetGameState(int GameState)
{
	SaveINIArray();
}
