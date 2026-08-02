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
 * MQ2ScreenShot - ported to the current MQ2 plugin API / eqlib (2026-08-02).
 *
 * Original: author "spat", macroquest2.com/phpBB3 viewtopic.php?f=50&t=15749
 * ("MQ2ScreenShot! (8/19/2008)"), Aug 2008. Untouched original at
 * original_2005-2013_source/MQ2ScreenShot.cpp.
 *
 * Runs a configurable list of commands before/after taking a screenshot
 * (e.g. hide MQ2 UI captions/HUD, press the screenshot key, restore UI).
 * No memory offsets/detours -- purely command/chat API, so this is a
 * straightforward signature-modernization port.
 *
 * Porting notes:
 *   - PCHAR/PSPAWNINFO -> const char* / SPAWNINFO* throughout.
 *   - AddCommand's modern std::function-based signature accepts a free
 *     function of (SPAWNINFO*, const char*) without change.
 *   - strcpy/strcmp -> strcpy_s/strcmp (still fine; kept simple char arrays
 *     as in the original rather than switching to std::string, to keep the
 *     port minimal and behavior-identical).
 *   - OnWriteChatColor/OnIncomingChat return bool now, not DWORD; "return 0"
 *     (do-not-filter) becomes "return false".
 *   - The original's ShutdownPlugin() removed "/MQ2SS"/"/MQ2SSList", which
 *     don't match the commands actually registered in InitializePlugin()
 *     ("/SS"/"/SSHelp"/etc) -- this looks like a pre-existing bug in the
 *     2008 source (dead RemoveCommand calls that never matched anything
 *     registered). Fixed here to remove the commands that are actually
 *     added, so unload doesn't leave stale command registrations behind.
 */

#include <mq/Plugin.h>

PreSetup("MQ2ScreenShot");
PLUGIN_VERSION(1.0);

constexpr int MAX_COMMANDS = 15;
using mqCMD = char[80];

static mqCMD preSSCmds[MAX_COMMANDS];
static mqCMD postSSCmds[MAX_COMMANDS];

PLUGIN_API void ScreenShot(SPAWNINFO* pChar, const char* Cmd)
{
	for (int n = 0; n < MAX_COMMANDS; n++)
	{
		if (strlen(preSSCmds[n]) > 0)
			DoCommand(pLocalPlayer, preSSCmds[n]);
		Sleep(10);
	}

	Sleep(100);
	DoCommand(pLocalPlayer, "/keypress SCREENCAP");

	for (int n = 0; n < MAX_COMMANDS; n++)
	{
		if (strlen(postSSCmds[n]) > 0)
			DoCommand(pLocalPlayer, postSSCmds[n]);
		Sleep(10);
	}
}

PLUGIN_API void ListCMDS(SPAWNINFO* pChar, const char* Cmd)
{
	WriteChatf("Pre-Screenshot Commands");
	for (int n = 0; n < MAX_COMMANDS; n++)
	{
		if (strlen(preSSCmds[n]) > 0)
			WriteChatf("%d) %s", n + 1, preSSCmds[n]);
	}
	WriteChatf("Post-Screenshot Commands");
	for (int n = 0; n < MAX_COMMANDS; n++)
	{
		if (strlen(postSSCmds[n]) > 0)
			WriteChatf("%d) %s", n + 1, postSSCmds[n]);
	}
}

PLUGIN_API void PreCMD(SPAWNINFO* pChar, const char* args)
{
	bool added = false;
	if (strlen(args) == 0)
	{
		WriteChatf("Please provide a command to add!");
		return;
	}
	for (int n = 0; n < MAX_COMMANDS; n++)
	{
		if (added) break;
		if (strlen(preSSCmds[n]) == 0)
		{
			strcpy_s(preSSCmds[n], args);
			added = true;
		}
	}
	if (added)
		WriteChatf("Added \"%s\" to the pre-screenshot command list!", args);
	else
		WriteChatf("Pre-screenshot command list is full! Unable to add \"%s\" to the command list", args);
}

PLUGIN_API void RemPreCMD(SPAWNINFO* pChar, const char* args)
{
	bool removed = false;
	if (strlen(args) == 0)
	{
		WriteChatf("Please provide a command to remove!");
		return;
	}
	for (int n = 0; n < MAX_COMMANDS; n++)
	{
		if (removed) break;
		if (strcmp(preSSCmds[n], args) == 0)
		{
			strcpy_s(preSSCmds[n], "");
			removed = true;
		}
	}
	if (removed)
		WriteChatf("Removed \"%s\" from the pre-screenshot command list!", args);
	else
		WriteChatf("Unable to locate \"%s\" in the command list", args);
}

PLUGIN_API void PostCMD(SPAWNINFO* pChar, const char* args)
{
	bool added = false;
	if (strlen(args) == 0)
	{
		WriteChatf("Please provide a command to add!");
		return;
	}
	for (int n = 0; n < MAX_COMMANDS; n++)
	{
		if (added) break;
		if (strlen(postSSCmds[n]) == 0)
		{
			strcpy_s(postSSCmds[n], args);
			added = true;
		}
	}
	if (added)
		WriteChatf("Added \"%s\" to the post-screenshot command list!", args);
	else
		WriteChatf("Post-screenshot command list is full! Unable to add \"%s\" to the command list", args);
}

PLUGIN_API void RemPostCMD(SPAWNINFO* pChar, const char* args)
{
	bool removed = false;
	if (strlen(args) == 0)
	{
		WriteChatf("Please provide a command to remove!");
		return;
	}
	for (int n = 0; n < MAX_COMMANDS; n++)
	{
		if (removed) break;
		if (strcmp(postSSCmds[n], args) == 0)
		{
			strcpy_s(postSSCmds[n], "");
			removed = true;
		}
	}
	if (removed)
		WriteChatf("Removed \"%s\" from the post-screenshot command list!", args);
	else
		WriteChatf("Unable to locate \"%s\" in the command list", args);
}

PLUGIN_API void SSHelp(SPAWNINFO* pChar, const char* args)
{
	WriteChatColor("MQ2ScreenShot Command list:", USERCOLOR_SYSTEM);
	WriteChatColor("/SS - Take a screenshot.");
	WriteChatColor("/SSHelp - Command list.");
	WriteChatColor("/SSList - Lists all the commands that will execute when /SS is called.");
	WriteChatColor("/SSPre arg - Adds the provided argument to the list of commands that will be run prior to taking a screenshot.");
	WriteChatColor("/SSRemPre arg - Removes the provided argument from the list of commands that will be run prior to taking a screenshot.");
	WriteChatColor("/SSPost arg - Adds the provided argument to the list of commands that will be run after to taking a screenshot.");
	WriteChatColor("/SSRemPost arg - Removes the provided argument from the list of commands that will be run after to taking a screenshot.");
}

PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2ScreenShot");

	AddCommand("/SS", ScreenShot);
	AddCommand("/SSHelp", SSHelp);
	AddCommand("/SSList", ListCMDS);
	AddCommand("/SSPRE", PreCMD);
	AddCommand("/SSREMPRE", RemPreCMD);
	AddCommand("/SSPOST", PostCMD);
	AddCommand("/SSREMPOST", RemPostCMD);

	strcpy_s(preSSCmds[0], "/caption MQCaptions off");
	strcpy_s(preSSCmds[1], "/keypress NETSTAT");
	strcpy_s(preSSCmds[2], "/keypress FULLSCREEN");
	strcpy_s(postSSCmds[0], "/caption MQCaptions on");
	strcpy_s(postSSCmds[1], "/keypress NETSTAT");
	strcpy_s(postSSCmds[2], "/keypress FULLSCREEN");
}

PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("Shutting down MQ2ScreenShot");
	RemoveCommand("/SS");
	RemoveCommand("/SSHelp");
	RemoveCommand("/SSList");
	RemoveCommand("/SSPRE");
	RemoveCommand("/SSREMPRE");
	RemoveCommand("/SSPOST");
	RemoveCommand("/SSREMPOST");
}

PLUGIN_API void OnAddSpawn(SPAWNINFO* pNewSpawn)
{
	DebugSpewAlways("MQ2ScreenShot::OnAddSpawn(%s)", pNewSpawn->Name);
}

PLUGIN_API void OnRemoveSpawn(SPAWNINFO* pSpawn)
{
	DebugSpewAlways("MQ2ScreenShot::OnRemoveSpawn(%s)", pSpawn->Name);
}
