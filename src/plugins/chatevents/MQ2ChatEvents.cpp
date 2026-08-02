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
 * MQ2ChatEvents - ported to the current MQ2 plugin API / eqlib (2026-08-02).
 *
 * Original: authors Persnickety/BustedPretext, created 2006-08-05, last
 * updated 2013-05-24 (multi-command queue, in-game variable expansion in
 * match strings/commands). Untouched original at
 * original_2005-2013_source/MQ2ChatEvents.cpp (also
 * MQ2ChatEvents_inline_from_post.cpp, an earlier/alternate forum-post copy
 * kept for reference -- effectively the same plugin, the .cpp used here is
 * the more complete/later of the two).
 *
 * Watches incoming/outgoing chat lines for user-defined match strings
 * (optionally filtered by chat color) and, on a match, can play a sound,
 * show an on-screen popup, and/or queue one or more MQ2 commands for
 * execution (one per pulse). Config is entirely INI-driven
 * (MQ2ChatEvents.ini, per-character section). No memory offsets/detours --
 * purely command/chat/INI/PlaySound API, so this is a straightforward
 * signature-modernization port.
 *
 * Porting notes:
 *   - PCHAR/PSPAWNINFO/DWORD/VOID -> current C++ types throughout.
 *   - OnWriteChatColor/OnIncomingChat return bool now, not DWORD; "return 0"
 *     (do-not-filter) becomes "return false".
 *   - `using namespace std;` dropped (the original relied on an implicit
 *     `using namespace std` pulled in via MQ2Plugin.h); string/vector/queue
 *     usages qualified with std:: instead.
 *   - TextToColor()'s giant if/strcmp ladder, ProcessPopups/ProcessSounds/
 *     ProcessCommands, FindMatch, AddEvents, InitEvents, and all the on/off
 *     INI toggle plumbing are unchanged logic-wise; only signatures/types
 *     were modernized.
 *   - ParseMacroData(buf) -> ParseMacroData(buf, sizeof(buf)) at both call
 *     sites (expandedString in FindMatch, commandText in OnPulse) per the
 *     current API's required BufferSize parameter
 *     (include/mq/api/MacroAPI.h).
 *   - DoCommand(((PCHARINFO)pCharData)->pSpawn, ...) -> DoCommand(pLocalPlayer, ...)
 *     (current global for "my own spawn", simpler than the old cast).
 *   - GetPrivateProfileString's NULL default-value overload -> pass
 *     "MQ2ChatEvents_Error" sentinel exactly as before (no NULL default
 *     overload ambiguity issue since the original always passed a sentinel
 *     string default already).
 *   - DisplayOverlayText/GetGroupMember/PlaySound/WriteChatColor/
 *     SyntaxError/CONCOLOR_x/USERCOLOR_x constants all unchanged in the current API.
 */

#include <mq/Plugin.h>
#include <mmsystem.h>
#include <vector>
#include <queue>
#include <string>

#pragma comment(lib, "winmm.lib")

void ChatEventsHelp();
DWORD TextToColor(char[]);
void ToggleOption(const char*, bool*);
std::string GetOnOffLabel(bool);
void HandleChat(const char*, DWORD);
void AddEvents(char[]);

#define MAX_KEYLINES 100      // not max keys, but max number of EventKey# lines, each capable of containing multiple keys
#define MATCHABLE_STRINGS 100
#define MATCHABLE_COLORS 10
#define MAX_COMMANDS 100
#define SKIP_PULSES 5

struct ChatEvent
{
	char key[MAX_STRING];
	char matchStrings[MATCHABLE_STRINGS][MAX_STRING];
	char matchColors[MATCHABLE_COLORS][MAX_STRING];
	char soundFile[MAX_STRING];
	char popupText[MAX_STRING];
	char popupColor[MAX_STRING];
	char popupDuration[MAX_STRING];
	char command[MAX_COMMANDS][MAX_STRING];
	int commandCount;
	int matchCount;
	int matchColorCount;
};
std::vector<ChatEvent> eventVector;

char commandBuffer[MAX_STRING];
std::queue<std::string> commandQueue;

char CESection[MAX_STRING] = "MQ2ChatEvents";
bool popupsEnabled = true;
bool soundsEnabled = true;
bool missedChatPopup = true;
bool missedChatEcho = true;
bool commandsEnabled = true;
bool verboseCommands = false;
bool pluginEnabled = true;
bool tellFlag = false;
int pulseCounter = 0;
bool processFlag = true;

PreSetup("MQ2ChatEvents");

// InitEvents - loads custom events and match strings from INI file
void InitEvents()
{
	gbInZone = true;
	char tempSetting[MAX_STRING];
	eventVector.clear();

	// return if character not yet in game
	if (strcmp(CESection, "MQ2ChatEvents") == 0) return;

	sprintf_s(CESection, "%s_%s", GetCharInfo()->Name, EQADDR_SERVERNAME);

	GetPrivateProfileString(CESection, "PluginEnabled", "MQ2ChatEvents_Error", tempSetting, MAX_STRING, INIFileName);
	if (strcmp(tempSetting, "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection, "PluginEnabled", "TRUE", INIFileName);
		sprintf_s(tempSetting, "TRUE");
	}
	if (!_strnicmp(tempSetting, "true", 4) || !_strnicmp(tempSetting, "on", 2) || !_strnicmp(tempSetting, "1", 1)) pluginEnabled = true;
	else pluginEnabled = false;
	if (!pluginEnabled) return;

	GetPrivateProfileString(CESection, "PopupsEnabled", "MQ2ChatEvents_Error", tempSetting, MAX_STRING, INIFileName);
	if (strcmp(tempSetting, "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection, "PopupsEnabled", "TRUE", INIFileName);
		sprintf_s(tempSetting, "TRUE");
	}
	if (!_strnicmp(tempSetting, "true", 4) || !_strnicmp(tempSetting, "on", 2) || !_strnicmp(tempSetting, "1", 1)) popupsEnabled = true;
	else popupsEnabled = false;

	GetPrivateProfileString(CESection, "SoundsEnabled", "MQ2ChatEvents_Error", tempSetting, MAX_STRING, INIFileName);
	if (strcmp(tempSetting, "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection, "SoundsEnabled", "TRUE", INIFileName);
		sprintf_s(tempSetting, "TRUE");
	}
	if (!_strnicmp(tempSetting, "true", 4) || !_strnicmp(tempSetting, "on", 2) || !_strnicmp(tempSetting, "1", 1)) soundsEnabled = true;
	else soundsEnabled = false;

	GetPrivateProfileString(CESection, "MissedChatEcho", "MQ2ChatEvents_Error", tempSetting, MAX_STRING, INIFileName);
	if (strcmp(tempSetting, "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection, "MissedChatEcho", "FALSE", INIFileName);
		sprintf_s(tempSetting, "FALSE");
	}
	if (!_strnicmp(tempSetting, "true", 4) || !_strnicmp(tempSetting, "on", 2) || !_strnicmp(tempSetting, "1", 1)) missedChatEcho = true;
	else missedChatEcho = false;

	GetPrivateProfileString(CESection, "MissedChatPopup", "MQ2ChatEvents_Error", tempSetting, MAX_STRING, INIFileName);
	if (strcmp(tempSetting, "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection, "MissedChatPopup", "FALSE", INIFileName);
		sprintf_s(tempSetting, "FALSE");
	}
	if (!_strnicmp(tempSetting, "true", 4) || !_strnicmp(tempSetting, "on", 2) || !_strnicmp(tempSetting, "1", 1)) missedChatPopup = true;
	else missedChatPopup = false;

	GetPrivateProfileString(CESection, "CommandsEnabled", "MQ2ChatEvents_Error", tempSetting, MAX_STRING, INIFileName);
	if (strcmp(tempSetting, "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection, "CommandsEnabled", "TRUE", INIFileName);
		sprintf_s(tempSetting, "TRUE");
	}
	if (!_strnicmp(tempSetting, "true", 4) || !_strnicmp(tempSetting, "on", 2) || !_strnicmp(tempSetting, "1", 1)) commandsEnabled = true;
	else commandsEnabled = false;

	GetPrivateProfileString(CESection, "VerboseCommands", "MQ2ChatEvents_Error", tempSetting, MAX_STRING, INIFileName);
	if (strcmp(tempSetting, "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection, "VerboseCommands", "FALSE", INIFileName);
		sprintf_s(tempSetting, "FALSE");
	}
	if (!_strnicmp(tempSetting, "true", 4) || !_strnicmp(tempSetting, "on", 2) || !_strnicmp(tempSetting, "1", 1)) verboseCommands = true;
	else verboseCommands = false;

	static char szAllKeys[MAX_KEYLINES][MAX_STRING] = { 0 };
	memset(szAllKeys, 0, sizeof(szAllKeys));

	// Get event keys : EventKeys=
	GetPrivateProfileString(CESection, "EventKeys", "MQ2ChatEvents_Error", szAllKeys[0], MAX_STRING, INIFileName);
	if (strcmp(szAllKeys[0], "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection, "EventKeys", "test123|", INIFileName);
	}
	AddEvents(szAllKeys[0]);

	// Add event keys : EventKeys#=
	char tmpEventKeysLine[MAX_STRING];
	char tmpEventKeysKey[MAX_STRING];
	for (int i = 0; i < MAX_KEYLINES; i++) {
		sprintf_s(tmpEventKeysKey, "EventKeys%d", i);
		GetPrivateProfileString(CESection, tmpEventKeysKey, "MQ2ChatEvents_Error", tmpEventKeysLine, MAX_STRING, INIFileName);
		if (strcmp(tmpEventKeysLine, "MQ2ChatEvents_Error") == 0) continue;
		strcpy_s(szAllKeys[i], tmpEventKeysLine);
	}

	for (int keyIndex = 0; keyIndex < MAX_KEYLINES; keyIndex++) {
		AddEvents(szAllKeys[keyIndex]);
	}
}

void AddEvents(char KeyLine[MAX_STRING])
{
	std::string strAllKeys(KeyLine);
	std::string::size_type pos = strAllKeys.find("|", 0);
	while (pos != std::string::npos)
	{
		if (strlen(strAllKeys.substr(0, pos).c_str()) == 0) {
			// Erase key from AllKeys variable (so we process the next one on next loop)
			strAllKeys.erase(0, pos + 1);
			pos = strAllKeys.find("|");
			continue;
		}

		// Create new event structure
		ChatEvent newEvent = {};
		// Add event name to events structure
		sprintf_s(newEvent.key, "%s", strAllKeys.substr(0, pos).c_str());

		// Get event match strings from INI
		char tmpMatchString[MAX_STRING];
		char tmpMatchStringKey[MAX_STRING];
		newEvent.matchCount = 0;
		for (int i = 0; i < MATCHABLE_STRINGS; i++) {
			sprintf_s(tmpMatchStringKey, "MatchString%d", i);
			GetPrivateProfileString(newEvent.key, tmpMatchStringKey, "MQ2ChatEvents_Error", tmpMatchString, MAX_STRING, INIFileName);
			if (strcmp(tmpMatchString, "MQ2ChatEvents_Error") == 0) continue;
			strcpy_s(newEvent.matchStrings[newEvent.matchCount], tmpMatchString);
			newEvent.matchCount++;
		}

		// Get matchColors from INI
		char tmpMatchColor[MAX_STRING];
		char tmpMatchColorKey[MAX_STRING];
		newEvent.matchColorCount = 0;
		for (int i = 0; i < MATCHABLE_COLORS; i++) {
			sprintf_s(tmpMatchColorKey, "MatchColor%d", i);
			GetPrivateProfileString(newEvent.key, tmpMatchColorKey, "MQ2ChatEvents_Error", tmpMatchColor, MAX_STRING, INIFileName);
			if (strcmp(tmpMatchColor, "MQ2ChatEvents_Error") == 0) continue;
			strcpy_s(newEvent.matchColors[newEvent.matchColorCount], tmpMatchColor);
			newEvent.matchColorCount++;
		}

		// Get sound file from INI
		GetPrivateProfileString(newEvent.key, "SoundFile", "MQ2ChatEvents_Error", newEvent.soundFile, MAX_STRING, INIFileName);
		if (strlen(newEvent.soundFile) == 0) strcpy_s(newEvent.soundFile, "MQ2ChatEvents_Error");

		// Get popup message from INI
		GetPrivateProfileString(newEvent.key, "PopupText", "MQ2ChatEvents_Error", newEvent.popupText, MAX_STRING, INIFileName);
		if (strlen(newEvent.popupText) == 0) strcpy_s(newEvent.popupText, "MQ2ChatEvents_Error");

		// Get popup text color from INI
		GetPrivateProfileString(newEvent.key, "PopupColor", "CONCOLOR_YELLOW", newEvent.popupColor, MAX_STRING, INIFileName);
		if (strlen(newEvent.popupColor) == 0) strcpy_s(newEvent.popupColor, "CONCOLOR_YELLOW");

		// Get popup display duration from INI
		GetPrivateProfileString(newEvent.key, "PopupDuration", "3000", newEvent.popupDuration, MAX_STRING, INIFileName);
		if (strlen(newEvent.popupDuration) == 0) strcpy_s(newEvent.popupDuration, "3000");

		// Get commands from INI
		char tmpCommandKey[MAX_STRING];
		char tmpCommandString[MAX_STRING];
		newEvent.commandCount = 0;
		for (int i = 0; i < MAX_COMMANDS; i++) {
			sprintf_s(tmpCommandKey, "Command%d", i);
			GetPrivateProfileString(newEvent.key, tmpCommandKey, "MQ2ChatEvents_Error", tmpCommandString, MAX_STRING, INIFileName);
			if (strcmp(tmpCommandString, "MQ2ChatEvents_Error") == 0) continue;
			strcpy_s(newEvent.command[newEvent.commandCount], tmpCommandString);
			newEvent.commandCount++;
		}

		// Add the event to the vector
		eventVector.push_back(newEvent);

		// Erase current key from AllKeys variable (so we process the next one on next loop)
		strAllKeys.erase(0, pos + 1);
		pos = strAllKeys.find("|");
	}
}

// Cycle through events looking for a string (and color) match
// Returns the index of the event where the match is found
// Returns -1 if no match found
int FindMatch(const char* Line, DWORD Color)
{
	std::string strLine(Line);
	std::string::size_type pos;
	bool NoColorCheck = false;
	char expandedString[MAX_STRING] = { 0 };

	if (Color == 0) NoColorCheck = true;

	for (size_t eventIndex = 0; eventIndex < eventVector.size(); eventIndex++)
	{
		for (int matchStringIndex = 0; matchStringIndex < eventVector[eventIndex].matchCount; matchStringIndex++) {
			strcpy_s(expandedString, eventVector[eventIndex].matchStrings[matchStringIndex]);
			// Expand any variables in the match string
			ParseMacroData(expandedString, sizeof(expandedString));

			pos = strLine.find(expandedString, 0);
			if (pos != std::string::npos) {
				// string match found, scan for matched colors if specified
				if (NoColorCheck) return static_cast<int>(eventIndex);

				// If no colors specified, successful match for all colors
				if (eventVector[eventIndex].matchColorCount == 0) return static_cast<int>(eventIndex);

				// for each MatchColor# entry in the event
				for (int matchColorIndex = 0; matchColorIndex < eventVector[eventIndex].matchColorCount; matchColorIndex++) {
					if (Color == TextToColor(eventVector[eventIndex].matchColors[matchColorIndex])) {
						return static_cast<int>(eventIndex);
					}
				}
			}
		}
	}
	// No match found
	return -1;
}

void ProcessPopups(const char* Line, unsigned int i)
{
	DWORD popupColor = 0;
	uint32_t popupTransparency = 100;
	uint32_t popupFadeIn = 500;
	uint32_t popupFadeOut = 500;
	uint32_t popupHold = 3000;
	std::string::size_type pos;
	char popupText[MAX_STRING];

	if (popupsEnabled && strcmp(eventVector[i].popupText, "MQ2ChatEvents_Error") != 0) {
		popupColor = TextToColor(eventVector[i].popupColor);
		popupHold = static_cast<uint32_t>(atoi(eventVector[i].popupDuration));

		// Replace placeholder #FULLTEXT# with the entire line that was matched
		std::string popupString(eventVector[i].popupText);
		pos = popupString.find("#FULLTEXT#");
		if (pos != std::string::npos) {
			popupString.replace(pos, strlen("#FULLTEXT#"), Line);
		}
		sprintf_s(popupText, "%s", popupString.c_str());

		DisplayOverlayText(popupText, popupColor, popupTransparency, popupFadeIn, popupFadeOut, popupHold);
	}
}

void ProcessSounds(unsigned int i)
{
	if (soundsEnabled && strcmp(eventVector[i].soundFile, "MQ2ChatEvents_Error") != 0) {
		char szTemp[MAX_STRING] = { 0 };
		sprintf_s(szTemp, "%s\\%s", gszINIPath, eventVector[i].soundFile);
		PlaySoundA(szTemp, nullptr, SND_FILENAME | SND_ASYNC);
	}
}

void ProcessCommands(const char* Line, unsigned int eventIndex, unsigned int commandIndex)
{
	char commandText[MAX_STRING];
	std::string::size_type pos;
	std::string strLine(Line);

	if (!commandsEnabled) return;

	strcpy_s(commandText, eventVector[eventIndex].command[commandIndex]);

	// Replace placeholder #FULLTEXT# with the entire line that was matched
	std::string commandString(commandText);
	pos = commandString.find("#FULLTEXT#");
	if (pos != std::string::npos) {
		commandString.replace(pos, strlen("#FULLTEXT#"), strLine);
	}
	sprintf_s(commandText, "%s", commandString.c_str());

	if (strcmp(commandText, "MQ2ChatEvents_Error") != 0) {
		commandQueue.push(commandText);
	}
}

void CheckMissedChat(const char* Line, DWORD Color)
{
	// Check if you received any tells or group chat while zoning
	std::string strLine(Line);
	std::string::size_type pos;

	if (!gbInZone && missedChatEcho) {
		pos = strLine.find(" tells ", 0);
		if (pos != std::string::npos) {
			processFlag = false;
			DebugSpewAlways("Missed chat: %s", Line);
			WriteChatColor(Line, Color);
			processFlag = true;
			tellFlag = true;
		}
		pos = strLine.find(" told ", 0);
		if (pos != std::string::npos) {
			processFlag = false;
			DebugSpewAlways("Missed chat: %s", Line);
			WriteChatColor(Line, Color);
			processFlag = true;
			tellFlag = true;
		}
	}

	if (tellFlag && missedChatPopup) {
		DisplayOverlayText("You missed some chat while zoning!  Check MQ window.",
			CONCOLOR_YELLOW, 100, 500, 500, 10000);
		tellFlag = false;
	}
}

void HandleChat(const char* Line, DWORD Color)
{
	/* ****************************************************************************************
	Be very careful writing anything to the EQ or MQ chat windows inside this function!
	It can create an infinite recursive loop by re-calling HandleChat for any line that is written!
	**************************************************************************************** */
	int eventIndex = -1;

	if (!pluginEnabled || !processFlag) return;

	CheckMissedChat(Line, Color);
	// Cycle through events looking for a match.  If match is found, that event's index is returned
	eventIndex = FindMatch(Line, Color);
	if (eventIndex < 0 || eventIndex >= static_cast<int>(eventVector.size())) return;

	// Found a matching chat line.  Now process all defined events for that match
	ProcessPopups(Line, eventIndex);
	ProcessSounds(eventIndex);

	for (int cmdIndex = 0; cmdIndex < eventVector[eventIndex].commandCount; cmdIndex++) {
		ProcessCommands(Line, eventIndex, cmdIndex);
	}
}

PLUGIN_API bool OnWriteChatColor(const char* Line, DWORD Color, DWORD Filter)
{
	if (processFlag && pluginEnabled) HandleChat(Line, Color);
	return false;
}

PLUGIN_API bool OnIncomingChat(const char* Line, DWORD Color)
{
	if (processFlag && pluginEnabled) HandleChat(Line, Color);
	return false;
}

// Handle commands passed through /ce
void ChatEventsCmd(SPAWNINFO* pChar, const char* szLine)
{
	char Arg[MAX_STRING];
	char tempString[MAX_STRING];

	GetArg(Arg, szLine, 1);

	if (!strlen(Arg) ||
		!_strnicmp(Arg, "help", strlen(Arg)) ||
		!_strnicmp(Arg, "?", strlen(Arg))) {
		ChatEventsHelp();
		return;
	}
	if (!_strnicmp(Arg, "sound", strlen(Arg)) ||
		!_strnicmp(Arg, "sounds", strlen(Arg)) ||
		!_strnicmp(Arg, "togglesound", strlen(Arg)) ||
		!_strnicmp(Arg, "togglesounds", strlen(Arg))) {
		ToggleOption("SoundsEnabled", &soundsEnabled);
		return;
	}
	if (!_strnicmp(Arg, "popup", strlen(Arg)) ||
		!_strnicmp(Arg, "popups", strlen(Arg)) ||
		!_strnicmp(Arg, "togglepopup", strlen(Arg)) ||
		!_strnicmp(Arg, "togglepopups", strlen(Arg))) {
		ToggleOption("PopupsEnabled", &popupsEnabled);
		return;
	}
	if (!_strnicmp(Arg, "missedchat", strlen(Arg)) ||
		!_strnicmp(Arg, "missedchatecho", strlen(Arg)) ||
		!_strnicmp(Arg, "togglemissedchat", strlen(Arg)) ||
		!_strnicmp(Arg, "togglemissedchatecho", strlen(Arg))) {
		ToggleOption("MissedChatEcho", &missedChatEcho);
		return;
	}
	if (!_strnicmp(Arg, "missedchatpopup", strlen(Arg)) ||
		!_strnicmp(Arg, "togglemissedchatpopup", strlen(Arg))) {
		ToggleOption("MissedChatPopup", &missedChatPopup);
		return;
	}
	if (!_strnicmp(Arg, "command", strlen(Arg)) ||
		!_strnicmp(Arg, "commands", strlen(Arg)) ||
		!_strnicmp(Arg, "togglecommand", strlen(Arg)) ||
		!_strnicmp(Arg, "togglecommands", strlen(Arg))) {
		ToggleOption("CommandsEnabled", &commandsEnabled);
		return;
	}
	if (!_strnicmp(Arg, "verbose", strlen(Arg)) ||
		!_strnicmp(Arg, "verbosecommands", strlen(Arg)) ||
		!_strnicmp(Arg, "toggleverbose", strlen(Arg)) ||
		!_strnicmp(Arg, "togglerverbosecommands", strlen(Arg))) {
		ToggleOption("VerboseCommands", &verboseCommands);
		return;
	}
	if (!_strnicmp(Arg, "on", strlen(Arg))) {
		WritePrivateProfileString(CESection, "PluginEnabled", "TRUE", INIFileName);
		pluginEnabled = true;
		std::string enabledTxt = GetOnOffLabel(pluginEnabled);
		sprintf_s(tempString, "ChatEvents: Plugin = %s", enabledTxt.c_str());
		WriteChatColor(tempString);
		InitEvents();
		return;
	}
	if (!_strnicmp(Arg, "off", strlen(Arg))) {
		WritePrivateProfileString(CESection, "PluginEnabled", "FALSE", INIFileName);
		pluginEnabled = false;
		std::string enabledTxt = GetOnOffLabel(pluginEnabled);
		sprintf_s(tempString, "ChatEvents: Plugin = %s", enabledTxt.c_str());
		WriteChatColor(tempString);
		return;
	}
	if (!_strnicmp(Arg, "reload", strlen(Arg)) ||
		!_strnicmp(Arg, "reloadini", strlen(Arg))) {
		InitEvents();
		WriteChatColor("ChatEvents INI reloaded");
		return;
	}
	else {
		ChatEventsHelp();
		return;
	}
}

void ToggleOption(const char* optionName, bool* optionVal)
{
	char tempString[MAX_STRING];

	*optionVal = !(*optionVal);
	if (*optionVal) sprintf_s(tempString, "TRUE");
	else sprintf_s(tempString, "FALSE");
	WritePrivateProfileString(CESection, optionName, tempString, INIFileName);

	std::string enabledTxt = GetOnOffLabel(*optionVal);
	sprintf_s(tempString, "ChatEvents: %s = %s", optionName, enabledTxt.c_str());
	WriteChatColor(tempString);

	InitEvents();
}

void ChatEventsHelp()
{
	char szTemp[MAX_STRING];
	std::string tempStr;

	WriteChatColor(" --------------------------------", CONCOLOR_LIGHTBLUE);
	WriteChatColor("MQ2ChatEvents    ", CONCOLOR_LIGHTBLUE);
	WriteChatColor(" --------------------------------", CONCOLOR_LIGHTBLUE);
	tempStr = GetOnOffLabel(pluginEnabled);
	sprintf_s(szTemp, "[%s] Plugin", tempStr.c_str());
	WriteChatColor(szTemp);
	tempStr = GetOnOffLabel(popupsEnabled);
	sprintf_s(szTemp, "[%s] Popups", tempStr.c_str());
	WriteChatColor(szTemp);
	tempStr = GetOnOffLabel(soundsEnabled);
	sprintf_s(szTemp, "[%s] Sounds", tempStr.c_str());
	WriteChatColor(szTemp);
	tempStr = GetOnOffLabel(missedChatPopup);
	sprintf_s(szTemp, "[%s] MissedChatPopup", tempStr.c_str());
	WriteChatColor(szTemp);
	tempStr = GetOnOffLabel(missedChatEcho);
	sprintf_s(szTemp, "[%s] MissedChatEcho", tempStr.c_str());
	WriteChatColor(szTemp);
	tempStr = GetOnOffLabel(commandsEnabled);
	sprintf_s(szTemp, "[%s] Commands", tempStr.c_str());
	WriteChatColor(szTemp);
	tempStr = GetOnOffLabel(verboseCommands);
	sprintf_s(szTemp, "[%s] Verbose Commands", tempStr.c_str());
	WriteChatColor(szTemp);
	WriteChatColor(" --------------------------------", CONCOLOR_LIGHTBLUE);
	SyntaxError("Usage: /ce <Sound|Popup|MissedChat|MissedChatEcho|Commands|verbose|on|off|reload>");
}

std::string GetOnOffLabel(bool val)
{
	std::string s;

	if (val) s = "\agON\ax";
	else s = "\auOFF\ax";

	return s;
}

DWORD TextToColor(char colorText[MAX_STRING])
{
	if (!strcmp(colorText, "COLOR_DEFAULT"))                  return COLOR_DEFAULT;
	if (!strcmp(colorText, "COLOR_DARKGREY"))                 return COLOR_DARKGREY;
	if (!strcmp(colorText, "COLOR_DARKGREEN"))                return COLOR_DARKGREEN;
	if (!strcmp(colorText, "COLOR_DARKBLUE"))                 return COLOR_DARKBLUE;
	if (!strcmp(colorText, "COLOR_PURPLE"))                   return COLOR_PURPLE;
	if (!strcmp(colorText, "COLOR_LIGHTGREY"))                return COLOR_LIGHTGREY;

	if (!strcmp(colorText, "CONCOLOR_GREEN"))                 return CONCOLOR_GREEN;
	if (!strcmp(colorText, "CONCOLOR_LIGHTBLUE"))             return CONCOLOR_LIGHTBLUE;
	if (!strcmp(colorText, "CONCOLOR_BLUE"))                  return CONCOLOR_BLUE;
	if (!strcmp(colorText, "CONCOLOR_BLACK"))                 return CONCOLOR_BLACK;
	if (!strcmp(colorText, "CONCOLOR_YELLOW"))                return CONCOLOR_YELLOW;
	if (!strcmp(colorText, "CONCOLOR_RED"))                   return CONCOLOR_RED;

	if (!strcmp(colorText, "USERCOLOR_SAY"))                  return USERCOLOR_SAY;
	if (!strcmp(colorText, "USERCOLOR_TELL"))                 return USERCOLOR_TELL;
	if (!strcmp(colorText, "USERCOLOR_GROUP"))                return USERCOLOR_GROUP;
	if (!strcmp(colorText, "USERCOLOR_GUILD"))                return USERCOLOR_GUILD;
	if (!strcmp(colorText, "USERCOLOR_OOC"))                  return USERCOLOR_OOC;
	if (!strcmp(colorText, "USERCOLOR_AUCTION"))              return USERCOLOR_AUCTION;
	if (!strcmp(colorText, "USERCOLOR_SHOUT"))                return USERCOLOR_SHOUT;
	if (!strcmp(colorText, "USERCOLOR_EMOTE"))                return USERCOLOR_EMOTE;
	if (!strcmp(colorText, "USERCOLOR_SPELLS"))               return USERCOLOR_SPELLS;
	if (!strcmp(colorText, "USERCOLOR_YOU_HIT_OTHER"))        return USERCOLOR_YOU_HIT_OTHER;
	if (!strcmp(colorText, "USERCOLOR_OTHER_HIT_YOU"))        return USERCOLOR_OTHER_HIT_YOU;
	if (!strcmp(colorText, "USERCOLOR_YOU_MISS_OTHER"))       return USERCOLOR_YOU_MISS_OTHER;
	if (!strcmp(colorText, "USERCOLOR_OTHER_MISS_YOU"))       return USERCOLOR_OTHER_MISS_YOU;
	if (!strcmp(colorText, "USERCOLOR_DUELS"))                return USERCOLOR_DUELS;
	if (!strcmp(colorText, "USERCOLOR_SKILLS"))               return USERCOLOR_SKILLS;
	if (!strcmp(colorText, "USERCOLOR_DISCIPLINES"))          return USERCOLOR_DISCIPLINES;
	if (!strcmp(colorText, "USERCOLOR_DEFAULT"))              return USERCOLOR_DEFAULT;
	if (!strcmp(colorText, "USERCOLOR_MERCHANT_OFFER"))       return USERCOLOR_MERCHANT_OFFER;
	if (!strcmp(colorText, "USERCOLOR_MERCHANT_EXCHANGE"))    return USERCOLOR_MERCHANT_EXCHANGE;
	if (!strcmp(colorText, "USERCOLOR_YOUR_DEATH"))           return USERCOLOR_YOUR_DEATH;
	if (!strcmp(colorText, "USERCOLOR_OTHER_DEATH"))          return USERCOLOR_OTHER_DEATH;
	if (!strcmp(colorText, "USERCOLOR_OTHER_HIT_OTHER"))      return USERCOLOR_OTHER_HIT_OTHER;
	if (!strcmp(colorText, "USERCOLOR_OTHER_MISS_OTHER"))     return USERCOLOR_OTHER_MISS_OTHER;
	if (!strcmp(colorText, "USERCOLOR_WHO"))                  return USERCOLOR_WHO;
	if (!strcmp(colorText, "USERCOLOR_YELL"))                 return USERCOLOR_YELL;
	if (!strcmp(colorText, "USERCOLOR_NON_MELEE"))            return USERCOLOR_NON_MELEE;
	if (!strcmp(colorText, "USERCOLOR_SPELL_WORN_OFF"))       return USERCOLOR_SPELL_WORN_OFF;
	if (!strcmp(colorText, "USERCOLOR_MONEY_SPLIT"))          return USERCOLOR_MONEY_SPLIT;
	if (!strcmp(colorText, "USERCOLOR_LOOT"))                 return USERCOLOR_LOOT;
	if (!strcmp(colorText, "USERCOLOR_RANDOM"))               return USERCOLOR_RANDOM;
	if (!strcmp(colorText, "USERCOLOR_OTHERS_SPELLS"))        return USERCOLOR_OTHERS_SPELLS;
	if (!strcmp(colorText, "USERCOLOR_SPELL_FAILURE"))        return USERCOLOR_SPELL_FAILURE;
	if (!strcmp(colorText, "USERCOLOR_CHAT_CHANNEL"))         return USERCOLOR_CHAT_CHANNEL;
	if (!strcmp(colorText, "USERCOLOR_CHAT_1"))               return USERCOLOR_CHAT_1;
	if (!strcmp(colorText, "USERCOLOR_CHAT_2"))               return USERCOLOR_CHAT_2;
	if (!strcmp(colorText, "USERCOLOR_CHAT_3"))               return USERCOLOR_CHAT_3;
	if (!strcmp(colorText, "USERCOLOR_CHAT_4"))               return USERCOLOR_CHAT_4;
	if (!strcmp(colorText, "USERCOLOR_CHAT_5"))               return USERCOLOR_CHAT_5;
	if (!strcmp(colorText, "USERCOLOR_CHAT_6"))               return USERCOLOR_CHAT_6;
	if (!strcmp(colorText, "USERCOLOR_CHAT_7"))               return USERCOLOR_CHAT_7;
	if (!strcmp(colorText, "USERCOLOR_CHAT_8"))               return USERCOLOR_CHAT_8;
	if (!strcmp(colorText, "USERCOLOR_CHAT_9"))               return USERCOLOR_CHAT_9;
	if (!strcmp(colorText, "USERCOLOR_CHAT_10"))              return USERCOLOR_CHAT_10;
	if (!strcmp(colorText, "USERCOLOR_MELEE_CRIT"))           return USERCOLOR_MELEE_CRIT;
	if (!strcmp(colorText, "USERCOLOR_SPELL_CRIT"))           return USERCOLOR_SPELL_CRIT;
	if (!strcmp(colorText, "USERCOLOR_TOO_FAR_AWAY"))         return USERCOLOR_TOO_FAR_AWAY;
	if (!strcmp(colorText, "USERCOLOR_NPC_RAMPAGE"))          return USERCOLOR_NPC_RAMPAGE;
	if (!strcmp(colorText, "USERCOLOR_NPC_FLURRY"))           return USERCOLOR_NPC_FLURRY;
	if (!strcmp(colorText, "USERCOLOR_NPC_ENRAGE"))           return USERCOLOR_NPC_ENRAGE;
	if (!strcmp(colorText, "USERCOLOR_ECHO_SAY"))             return USERCOLOR_ECHO_SAY;
	if (!strcmp(colorText, "USERCOLOR_ECHO_TELL"))            return USERCOLOR_ECHO_TELL;
	if (!strcmp(colorText, "USERCOLOR_ECHO_GROUP"))           return USERCOLOR_ECHO_GROUP;
	if (!strcmp(colorText, "USERCOLOR_ECHO_GUILD"))           return USERCOLOR_ECHO_GUILD;
	if (!strcmp(colorText, "USERCOLOR_ECHO_OOC"))             return USERCOLOR_ECHO_OOC;
	if (!strcmp(colorText, "USERCOLOR_ECHO_AUCTION"))         return USERCOLOR_ECHO_AUCTION;
	if (!strcmp(colorText, "USERCOLOR_ECHO_SHOUT"))           return USERCOLOR_ECHO_SHOUT;
	if (!strcmp(colorText, "USERCOLOR_ECHO_EMOTE"))           return USERCOLOR_ECHO_EMOTE;
	if (!strcmp(colorText, "USERCOLOR_ECHO_CHAT_1"))          return USERCOLOR_ECHO_CHAT_1;
	if (!strcmp(colorText, "USERCOLOR_ECHO_CHAT_2"))          return USERCOLOR_ECHO_CHAT_2;
	if (!strcmp(colorText, "USERCOLOR_ECHO_CHAT_3"))          return USERCOLOR_ECHO_CHAT_3;
	if (!strcmp(colorText, "USERCOLOR_ECHO_CHAT_4"))          return USERCOLOR_ECHO_CHAT_4;
	if (!strcmp(colorText, "USERCOLOR_ECHO_CHAT_5"))          return USERCOLOR_ECHO_CHAT_5;
	if (!strcmp(colorText, "USERCOLOR_ECHO_CHAT_6"))          return USERCOLOR_ECHO_CHAT_6;
	if (!strcmp(colorText, "USERCOLOR_ECHO_CHAT_7"))          return USERCOLOR_ECHO_CHAT_7;
	if (!strcmp(colorText, "USERCOLOR_ECHO_CHAT_8"))          return USERCOLOR_ECHO_CHAT_8;
	if (!strcmp(colorText, "USERCOLOR_ECHO_CHAT_9"))          return USERCOLOR_ECHO_CHAT_9;
	if (!strcmp(colorText, "USERCOLOR_ECHO_CHAT_10"))         return USERCOLOR_ECHO_CHAT_10;
	if (!strcmp(colorText, "USERCOLOR_LINK"))                 return USERCOLOR_LINK;
	if (!strcmp(colorText, "USERCOLOR_RAID"))                 return USERCOLOR_RAID;
	if (!strcmp(colorText, "USERCOLOR_PET"))                  return USERCOLOR_PET;
	if (!strcmp(colorText, "USERCOLOR_DAMAGESHIELD"))         return USERCOLOR_DAMAGESHIELD;
	if (!strcmp(colorText, "USERCOLOR_LEADER"))               return USERCOLOR_LEADER;
	if (!strcmp(colorText, "USERCOLOR_PETRAMPFLURRY"))        return USERCOLOR_PETRAMPFLURRY;
	if (!strcmp(colorText, "USERCOLOR_PETCRITS"))             return USERCOLOR_PETCRITS;
	if (!strcmp(colorText, "USERCOLOR_FOCUS"))                return USERCOLOR_FOCUS;
	if (!strcmp(colorText, "USERCOLOR_XP"))                   return USERCOLOR_XP;
	if (!strcmp(colorText, "USERCOLOR_SYSTEM"))               return USERCOLOR_SYSTEM;
	return 0;
}

// PlaySound - plays sound events when using the /playsound command
// Inspired by the MQ2PlaySound plug-in by Digitalxero
void PlaySoundCmd(SPAWNINFO* pChar, const char* szLine)
{
	char Arg[MAX_STRING] = { 0 };

	// Get sound file from INI
	GetArg(Arg, szLine, 1);
	if (!_stricmp(Arg, "stop")) {
		PlaySoundA(nullptr, nullptr, SND_ASYNC);
		return;
	}
	for (size_t i = 0; i < eventVector.size(); i++) {
		if (!_strnicmp(Arg, eventVector[i].key, strlen(Arg))) {
			char szSoundFile[MAX_STRING] = { 0 };
			GetPrivateProfileString(eventVector[i].key, "SoundFile", "MQ2ChatEvents_Error", szSoundFile, MAX_STRING, INIFileName);
			if (!_stricmp(szSoundFile, "MQ2ChatEvents_Error")) return;
			char szTemp[MAX_STRING] = { 0 };
			sprintf_s(szTemp, "%s\\%s", gszINIPath, eventVector[i].soundFile);
			PlaySoundA(szTemp, nullptr, SND_FILENAME | SND_ASYNC);
			break;
		}
	}
}

PLUGIN_API void SetGameState(int GameState)
{
	if (GameState == GAMESTATE_INGAME)
		sprintf_s(CESection, "%s_%s", GetCharInfo()->Name, EQADDR_SERVERNAME);
	else
		strcpy_s(CESection, "MQ2ChatEvents");

	InitEvents();
}

PLUGIN_API void OnPulse()
{
	// Execute one command per pulse
	// If there are commands in the queue, turn off chat line processing until the queue is empty again
	if (!commandQueue.empty()) {
		processFlag = false;
		char commandText[MAX_STRING];
		// Expand any variables in the command string
		sprintf_s(commandText, "%s", commandQueue.front().c_str());
		ParseMacroData(commandText, sizeof(commandText));

		if (verboseCommands) {
			char verboseMessage[MAX_STRING];
			sprintf_s(verboseMessage, "MQ2ChatEvents::Executing command=\ag%s\ax", commandText);
			WriteChatColor(verboseMessage);
		}
		DoCommand(pLocalPlayer, commandText);
		commandQueue.pop();
	}
	else {
		processFlag = true;
	}
}

PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2ChatEvents");
	AddCommand("/chatevents", ChatEventsCmd);
	AddCommand("/ce", ChatEventsCmd);
	AddCommand("/playsound", PlaySoundCmd);
	InitEvents();
}

PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("Shutting down MQ2ChatEvents");
	RemoveCommand("/chatevents");
	RemoveCommand("/ce");
	RemoveCommand("/playsound");
}
