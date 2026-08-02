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
 * MQ2Pop - ported to the current MQ2 plugin API / eqlib (2026-08-02).
 *
 * Original: "MQ2Pop.cpp : a simple, barebones plugin by notadruid. All it does
 * is spam the MQ2 Chat window when an NPC pops, and if the NPC is named it
 * will spam NAMED instead of pop." Version 3, macroquest2.com/phpBB3
 * viewtopic.php?f=50&t=12799 ("MQ2Pop: Simplest plugin ever!"), Feb 2006.
 * Untouched original at original_2005-2013_source/MQ2Pop.cpp (this directory).
 *
 * Porting notes:
 *   - Old code hand-rolled NamedStatus() via SEARCHSPAWN/ClearSearchSpawn/
 *     CountMatchingSpawns. The current codebase exposes a ready-made
 *     IsNamed(SPAWNINFO*) helper (src/main/MQ2Main.h) used by map/casttimer/
 *     the SpawnType TLO already -- use that directly instead of re-deriving
 *     the old search-spawn logic (which itself depended on 32-bit struct
 *     layouts not worth re-verifying when a current, already-audited
 *     equivalent exists).
 *   - PSPAWNINFO -> SPAWNINFO* (alias, unchanged behavior, just current style).
 *   - Old code dereferenced pNewSpawn->pSpawn->SpawnID, a Titanium-era
 *     double-indirection artifact; the modern SPAWNINFO/PlayerClient struct
 *     has no such pSpawn self-pointer, so IsNamed(pNewSpawn) is passed the
 *     spawn pointer directly.
 *   - Type/MasterID/Name/DisplayedName/WriteChatf are all unchanged in the
 *     current struct/API and needed no modification.
 */

#include <mq/Plugin.h>

PreSetup("MQ2Pop");
PLUGIN_VERSION(3.0);

static bool gbAreWeZoning = false;
static time_t gZoneInSeconds = 0;

PLUGIN_API void OnBeginZone()
{
	gbAreWeZoning = true;
}

PLUGIN_API void OnEndZone()
{
	gbAreWeZoning = false;
	gZoneInSeconds = time(nullptr);
}

PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2Pop");
	gZoneInSeconds = time(nullptr);
}

PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("Shutting down MQ2Pop");
}

PLUGIN_API void OnAddSpawn(SPAWNINFO* pNewSpawn)
{
	if (gbAreWeZoning
		|| time(nullptr) < gZoneInSeconds + 3
		|| pNewSpawn->Type != SPAWN_NPC
		|| pNewSpawn->MasterID
		|| strstr(pNewSpawn->Name, "s_Mount"))
	{
		return;
	}

	if (IsNamed(pNewSpawn))
	{
		WriteChatf("\arNAMED\ax > %s < \arNAMED\ax", pNewSpawn->DisplayedName);
	}
	else
	{
		WriteChatf("Pop > %s < Pop", pNewSpawn->DisplayedName);
	}
}
