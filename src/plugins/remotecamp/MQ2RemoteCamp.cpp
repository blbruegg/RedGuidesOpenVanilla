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
 * MQ2RemoteCamp - ported to the current MQ2 plugin API / eqlib (2026-08-02).
 *
 * Original: author anOrcPawn00 (per the plugin's own "about" text), v1.1.
 * Thread itself is a "lost source" recovery -- macroquest2.com/phpBB3
 * viewtopic.php?f=50&t=13837 ("MQ2RemoteCamp - Where did it go?"), Sep 2006.
 * Untouched original at original_2005-2013_source/MQ2RemoteCamp.cpp.
 *
 * Lets a party member /tell a password-protected command to remotely trigger
 * a delayed /camp on this character (e.g. to camp out a bot/mule safely from
 * another account). No memory offsets, patterns, or detours -- this plugin
 * only used documented command/chat/gamestate APIs, so the port is a
 * straightforward signature update, not a re-derivation.
 *
 * Porting notes:
 *   - AddCommand's modern signature takes std::function<void(PlayerClient*,
 *     const char*)>; a free function with (SPAWNINFO*, const char*) binds to
 *     it without change (SPAWNINFO* == PlayerClient*).
 *   - PCHAR/PSPAWNINFO -> const char* / SPAWNINFO* for current style.
 *   - OnIncomingChat's return type changed DWORD -> bool; return false in
 *     place of the old "return 0" (do-not-filter).
 *   - gGameState/GAMESTATE_INGAME unchanged; dropped the old MQ2Globals::
 *     qualifier (unnecessary, but harmless either way).
 *   - GetCharInfo()->pSpawn kept (still valid) rather than switching to
 *     pLocalPlayer, to keep the port minimal and behavior-identical; either
 *     works today.
 *   - clock()/CLOCKS_PER_SEC timing logic is untouched -- pure CRT, not
 *     client-memory-dependent, and the original's own countdown math (elapsed
 *     is really in clock ticks, only becomes "ms-like" for small process
 *     uptimes) is preserved as-is rather than silently "fixed", since this
 *     is a functional behavior question for the user to decide on, not an
 *     API-compat one.
 */

#include <mq/Plugin.h>
#include <ctime>

PreSetup("MQ2RemoteCamp");
PLUGIN_VERSION(1.1);

#define CLOCKS(x) (x * CLOCKS_PER_SEC)

static char password[MAX_STRING];
static int camping = 0;
static int countTime = 60;
static clock_t startTime = 0;
static SPAWNINFO* ps = nullptr;
static bool active = false;
static int lastPrinted = -1;

static bool VerifyPassword(const char* s);
static void InitiateCountdown(const char* szName);
static void DoCmdRemoteCamp(SPAWNINFO* pChar, const char* szLine);
static void PrintHelp();
static void PrintOptions();
static void PrintAbout();

PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2RemoteCamp");

	AddCommand("/remcamp", DoCmdRemoteCamp);

	GetPrivateProfileString("MQ2RemoteCamp", "Password", "ChangeMe", password, MAX_STRING, INIFileName);
	countTime = GetPrivateProfileInt("MQ2RemoteCamp", "Countdown", 60, INIFileName);
	active = GetPrivateProfileInt("MQ2RemoteCamp", "Active", 0, INIFileName) != 0;
}

PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("Shutting down MQ2RemoteCamp");

	RemoveCommand("/remcamp");
}

PLUGIN_API void SetGameState(int GameState)
{
	DebugSpewAlways("MQ2RemoteCamp::SetGameState()");
	if (GameState == GAMESTATE_INGAME)
	{
		if (active)
		{
			WriteChatColor("\atMQ2RemoteCamp\ax: Remote camping is currently \ayACTIVE\ax.");
			if (!_stricmp(password, "ChangeMe"))
			{
				WriteChatColor("\arWARNING: \axYou are currently using \atMQ2RemoteCamp\ax with the default password.  For your protection please use \ay/remcamp set password <newpassword>\ax to change it immediately.");
			}
		}
	}
}

PLUGIN_API void OnPulse()
{
	if (!active)
		return;
	if (gGameState != GAMESTATE_INGAME)
		return;

	char szTemp[MAX_STRING] = { 0 };

	if (camping == 1)
	{
		double elapsed = static_cast<double>(clock() - startTime);
		if (elapsed > CLOCKS(countTime))
		{
			WriteChatColor("Transferring countdown to EQ's /camp. You may still abort camping by standing up in the next 30 seconds! (\ay/sit off\ax)");
			camping = 2;
			if (gMacroBlock)
				DoCommand(ps, "/endmacro");
			DoCommand(ps, "/sit on");
			DoCommand(ps, "/camp desktop");
		}
		else
		{
			unsigned long el = static_cast<unsigned long>(elapsed / 1000);
			if ((countTime - static_cast<int>(el)) % 5 == 0)
			{
				if (static_cast<int>(el) != lastPrinted)
				{
					lastPrinted = static_cast<int>(el);
					sprintf_s(szTemp, "Remote camping countdown: \ay%d\axs", countTime - lastPrinted);
					WriteChatColor(szTemp);
				}
			}
		}
	}
}

PLUGIN_API bool OnIncomingChat(const char* Line, DWORD Color)
{
	if (gGameState != GAMESTATE_INGAME)
		return false;
	if (!active)
		return false;

	if (camping == 0)
	{
		const char* s = strstr(Line, "tells you, 'camp");
		if (s)
		{
			char szName[MAX_STRING] = { 0 };
			int namelen = static_cast<int>(s - Line - 1);
			if (namelen > 0 && namelen < MAX_STRING)
			{
				strncpy_s(szName, Line, namelen);

				s += 17;
				if (VerifyPassword(s))
				{
					PcProfile* pc = GetPcProfile();
					if (!pc)
					{
						WriteChatColor("NULL character profile pointer.");
						return false;
					}

					ps = pLocalPlayer;
					if (!ps)
					{
						WriteChatColor("NULL SpawnInfo pointer.");
						return false;
					}

					InitiateCountdown(szName);
				}
			}
		}
	}
	else if (camping == 2)
	{
		const char* s = strstr(Line, "You abandon your preparations to camp.");
		if (s == Line)
			camping = 0;
	}

	return false;
}

static void InitiateCountdown(const char* szName)
{
	char szTemp[MAX_STRING] = { 0 };
	sprintf_s(szTemp, "Remote camping command received from \ar%s\ax.", szName);
	WriteChatColor(szTemp);
	sprintf_s(szTemp, "Starting \ay%u\axs countdown.", countTime);
	WriteChatColor(szTemp);
	WriteChatColor("To stop countdown type: \ay/remcamp abort");

	camping = 1;
	startTime = clock();
	lastPrinted = countTime;
}

static bool VerifyPassword(const char* s)
{
	char szTemp[MAX_STRING];
	strcpy_s(szTemp, password);
	strcat_s(szTemp, "'");
	return _stricmp(s, szTemp) == 0;
}

static void DoCmdRemoteCamp(SPAWNINFO* pChar, const char* szLine)
{
	char szTemp[MAX_STRING] = { 0 };
	char Arg[3][MAX_STRING] = { 0 };
	for (int i = 0; i < 3; i++)
		GetArg(Arg[i], szLine, i + 1);

	if (Arg[0][0] == 0)
	{
		PrintHelp();
		return;
	}
	if (!_stricmp(Arg[0], "on"))
	{
		active = true;
		WritePrivateProfileString("MQ2RemoteCamp", "Active", "1", INIFileName);
		WriteChatColor("MQ2RemoteCamp now active.");
		camping = 0;
	}
	else if (!_stricmp(Arg[0], "off"))
	{
		active = false;
		WritePrivateProfileString("MQ2RemoteCamp", "Active", "0", INIFileName);
		WriteChatColor("MQ2RemoteCamp now inactive.");
		camping = 0;
	}
	else if (!_stricmp(Arg[0], "options"))
	{
		PrintOptions();
	}
	else if (!_stricmp(Arg[0], "set"))
	{
		if (Arg[1][0] == 0 || Arg[2][0] == 0)
		{
			PrintHelp();
			return;
		}

		if (!_stricmp(Arg[1], "countdown"))
		{
			countTime = atoi(Arg[2]);
			WritePrivateProfileString("MQ2RemoteCamp", "Countdown", Arg[2], INIFileName);
			sprintf_s(szTemp, "Countdown time set to \ay%d\axs", countTime);
			WriteChatColor(szTemp);
		}
		else if (!_stricmp(Arg[1], "password"))
		{
			strcpy_s(password, Arg[2]);
			WritePrivateProfileString("MQ2RemoteCamp", "Password", Arg[2], INIFileName);
			sprintf_s(szTemp, "Password set to \ay%s\ax", password);
			WriteChatColor(szTemp);
		}
		else
		{
			PrintHelp();
			return;
		}
	}
	else if (!_stricmp(Arg[0], "about"))
	{
		PrintAbout();
	}
	else if (!_stricmp(Arg[0], "abort"))
	{
		camping = 0;
		WriteChatColor("\agCountdown aborted.\ax");
	}
	else
	{
		PrintHelp();
	}
}

static void PrintHelp()
{
	WriteChatColor("\atMQ2RemoteCamp\ax by anOrcPawn00", USERCOLOR_DEFAULT);
	WriteChatColor("Syntax: /remcamp <command> <parameters>", USERCOLOR_DEFAULT);
	WriteChatColor("Commands:", USERCOLOR_DEFAULT);
	WriteChatColor("   \ayon\ax - enable remote camp", USERCOLOR_DEFAULT);
	WriteChatColor("   \ayoff\ax - disable remote camp", USERCOLOR_DEFAULT);
	WriteChatColor("   \ayabort\ax - aborts countdown in progress", USERCOLOR_DEFAULT);
	WriteChatColor("   \ayoptions\ax - displays current options", USERCOLOR_DEFAULT);
	WriteChatColor("   \ayset <option> <value>\ax - changes the value of an option", USERCOLOR_DEFAULT);
	WriteChatColor("   \ayabout\ax - credits & version", USERCOLOR_DEFAULT);
}

static void PrintOptions()
{
	char szTemp[MAX_STRING] = { 0 };
	WriteChatColor("MQ2RemoteCamp options:");
	sprintf_s(szTemp, "Countdown = \ay%d\axs", countTime);
	WriteChatColor(szTemp);
	sprintf_s(szTemp, "Password = \ay%s\ax", password);
	WriteChatColor(szTemp);
}

static void PrintAbout()
{
	WriteChatColor("MQ2RemoteCamp");
	WriteChatColor("  Author: \ayanOrcPawn00\ax");
	WriteChatColor("  Version: \ay1.1\ax");
}
