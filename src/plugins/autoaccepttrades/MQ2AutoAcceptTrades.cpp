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
 * MQ2AutoAcceptTrades - ported to the current MQ2 plugin API / eqlib (2026-08-02).
 *
 * Original: author Sadge (with help from Thez), v2.0000, Jan 2008.
 * macroquest2.com/phpBB3 viewtopic.php?f=50&t=15309
 * ("PLUGIN: MQ2AutoAcceptTrades v 2.0000"). Untouched original at
 * original_2005-2013_source/MQ2AutoAcceptTrades.cpp.
 *
 * PRIVATE-REPO-ONLY: the original author's post states "Please do not
 * distribute without my consent." Per the user's project decision this is
 * kept in a private repo for personal reference/use only -- do not push
 * this plugin (source or binary) anywhere public.
 *
 * Auto-accepts incoming /trade windows (with inventory-space and coin-dump
 * safety checks), optionally restricted to a configured name allowlist.
 * Entirely TLO-string-driven (${Window[...]}/${InvSlot[...]}/${Me...} via
 * ParseMacroData) -- no raw memory offsets or detours, so this port is a
 * straightforward signature/API modernization, not a re-derivation.
 *
 * Porting notes:
 *   - ParseMacroData gained a required `size_t BufferSize` second parameter
 *     (include/mq/api/MacroAPI.h) -- every call site updated to pass
 *     sizeof(buffer).
 *   - PCHAR/PSPAWNINFO/BOOL/VOID -> current C++ types throughout.
 *   - DoCommand(NULL, tradecommand) in the original relied on a null
 *     PSPAWNINFO being tolerated by the old DoCommand implementation; kept
 *     as nullptr here since mq::DoCommand's modern implementation still
 *     accepts a null PlayerClient* for command execution not tied to a
 *     specific character context (this plugin only ever has one local
 *     character, so pLocalPlayer would be equally correct -- used pLocalPlayer
 *     for clarity/safety since it's always available in these OnPulse-only
 *     call sites).
 *   - Arrays sized [100][64]/[8] preserved as-is from the original (not
 *     rewritten to std::vector/std::string) to keep the port minimal and
 *     behavior-identical; this does mean the original's own implicit
 *     "up to 8 distinct stackable item types per trade, up to 99 always-
 *     trade names" limits are carried forward unchanged.
 *   - gGameState/GAMESTATE_INGAME unchanged.
 */

#include <mq/Plugin.h>

#define   PLUGIN_NAME  "MQ2AutoAcceptTrades"
#define   PLUGIN_DATE  20080130
#define   PLUGIN_VERS  2.0000

static char PersonTrading[MAX_STRING];
static bool AutoAcceptEveryone;
static bool ReportTradeReject;
static bool EchoTradeInfo;
static bool AllowGoldandLowerDump;
static bool TradeDone;
static int MAXLIST = 30;
static int TotalNames = 0;
static char Name[100][64];
static char ItemReceived[100][64];
static char StackedItems[100][64];
static char tradecommand[MAX_STRING];
static int StackedItemsCount[8];
static int StacksCount[8];
static int RemainCount[8];
static int MaxStackedItemsCount[8];
static int CoinCount[4];
static int totalitemstaken;
static int bagsreceived;
static int openTLinv;
static bool TradeRejected;
static int numstackable;
static char ReportReject[MAX_STRING];
static int InventoryCheck;
static char reason[MAX_STRING];
static int totalitems;
static int AvailableRoom[8];

static void DoAutoTrade(SPAWNINFO* pChar, const char* szLine);

PreSetup("MQ2AutoAcceptTrades");
PLUGIN_VERSION(PLUGIN_VERS);

static void ShowHelp()
{
	WriteChatf("%s::Version [\ag%1.4f\ax] Loaded! Created by Sadge", PLUGIN_NAME, PLUGIN_VERS);
	WriteChatf("\ay====================\ax");
	WriteChatf("Status AutoAcceptEveryone is Currently: %s", AutoAcceptEveryone ? "\agTRUE" : "\arFALSE");
	WriteChatf("Status ReportTradeReject is Currently: %s", ReportTradeReject ? "\agTRUE" : "\arFALSE");
	WriteChatf("Status EchoTradeInfo is Currently: %s", EchoTradeInfo ? "\agTRUE" : "\arFALSE");
	WriteChatf("Status AllowGoldandLowerDump is Currently: %s", AllowGoldandLowerDump ? "\agTRUE" : "\arFALSE");
	WriteChatf("There are <%d> name(s) in your list of people to always trade with", TotalNames);
	WriteChatf("\ay====================\ax");
}

static void Load_INI()
{
	char szTemp[MAX_STRING];
	TotalNames = 0;

	GetPrivateProfileString("Settings", "AutoAcceptEveryone", "TRUE", szTemp, MAX_STRING, INIFileName);
	if (!_strnicmp(szTemp, "TRUE", 4))
	{
		WritePrivateProfileString("Settings", "AutoAcceptEveryone", "TRUE", INIFileName);
		AutoAcceptEveryone = true;
	}
	else
	{
		WritePrivateProfileString("Settings", "AutoAcceptEveryone", "FALSE", INIFileName);
		AutoAcceptEveryone = false;
	}

	GetPrivateProfileString("Settings", "ReportTradeReject", "TRUE", szTemp, MAX_STRING, INIFileName);
	if (!_strnicmp(szTemp, "TRUE", 4))
	{
		WritePrivateProfileString("Settings", "ReportTradeReject", "TRUE", INIFileName);
		ReportTradeReject = true;
	}
	else
	{
		WritePrivateProfileString("Settings", "ReportTradeReject", "FALSE", INIFileName);
		ReportTradeReject = false;
	}

	GetPrivateProfileString("Settings", "EchoTradeInfo", "TRUE", szTemp, MAX_STRING, INIFileName);
	if (!_strnicmp(szTemp, "TRUE", 4))
	{
		WritePrivateProfileString("Settings", "EchoTradeInfo", "TRUE", INIFileName);
		EchoTradeInfo = true;
	}
	else
	{
		WritePrivateProfileString("Settings", "EchoTradeInfo", "FALSE", INIFileName);
		EchoTradeInfo = false;
	}

	GetPrivateProfileString("Settings", "AllowGoldandLowerDump", "TRUE", szTemp, MAX_STRING, INIFileName);
	if (!_strnicmp(szTemp, "TRUE", 4))
	{
		WritePrivateProfileString("Settings", "AllowGoldandLowerDump", "TRUE", INIFileName);
		AllowGoldandLowerDump = true;
	}
	else
	{
		WritePrivateProfileString("Settings", "AllowGoldandLowerDump", "FALSE", INIFileName);
		AllowGoldandLowerDump = false;
	}

	for (int i = 0; i < (MAXLIST + 1); i++)
	{
		sprintf_s(szTemp, "Name%i", i);
		if (!GetPrivateProfileString("Names", szTemp, "", Name[i], 64, INIFileName))
			return;
		else
			TotalNames++;
	}
}

PLUGIN_API void InitializePlugin()
{
	DebugSpewAlways("Initializing MQ2AutoAcceptTrades");
	Load_INI();
	AddCommand("/autotrade", DoAutoTrade);

	WriteChatf("%s::Version [\ag%1.4f\ax] Loaded! Created by Sadge", PLUGIN_NAME, PLUGIN_VERS);
	WriteChatf("\ay====================\ax");
	WriteChatColor("You can view the current settings by typing /autotrade", CONCOLOR_GREEN);
	WriteChatColor("You can toggle auto-accepting with everyone by typing /autotrade TRUE (on) or FALSE (off)", CONCOLOR_GREEN);
	WriteChatColor("You can toggle reporting trade reject to person trading by typing /autotrade report", CONCOLOR_GREEN);
	WriteChatColor("You can toggle echoing trade information in the MQ2 window by typing /autotrade echo", CONCOLOR_GREEN);
	WriteChatColor("You can toggle allowing receiving gold and lower (Gold, Silver, or Copper Dump) by typing /autotrade gold", CONCOLOR_GREEN);
	WriteChatColor("You can add someone to your list of people to always trade with by typing /autotrade <CharName>", CONCOLOR_GREEN);
	WriteChatf("\ay====================\ax");
	WriteChatf("Status AutoAcceptEveryone is Currently: %s", AutoAcceptEveryone ? "\agTRUE" : "\arFALSE");
	WriteChatf("Status ReportTradeReject is Currently: %s", ReportTradeReject ? "\agTRUE" : "\arFALSE");
	WriteChatf("Status EchoTradeInfo is Currently: %s", EchoTradeInfo ? "\agTRUE" : "\arFALSE");
	WriteChatf("Status AllowGoldandLowerDump is Currently: %s", AllowGoldandLowerDump ? "\agTRUE" : "\arFALSE");
	WriteChatf("\ay====================\ax");
}

PLUGIN_API void ShutdownPlugin()
{
	DebugSpewAlways("Shutting down MQ2AutoAcceptTrades");
	RemoveCommand("/autotrade");
}

static bool CheckWindow(char* windowinfo)
{
	ParseMacroData(windowinfo, MAX_STRING);
	if (strcmp(windowinfo, "TRUE") == 0) return true;
	else if (strcmp(windowinfo, "NULL") != 0) return false;
	else return true;
}

static void AcceptTrade()
{
	sprintf_s(tradecommand, "/nomodkey /notify tradewnd TRDW_Trade_Button leftmouseup ");
}

static void RejectTrade()
{
	sprintf_s(tradecommand, "/nomodkey /notify tradewnd TRDW_Cancel_Button leftmouseup ");
}

static void EchoTrade()
{
	char timetrade[64];
	sprintf_s(timetrade, "${Time}");
	ParseMacroData(timetrade, sizeof(timetrade));
	WriteChatf("\ay====================\ax");
	WriteChatf("\agTrade Information\ax");
	WriteChatf("\ay====================\ax");
	WriteChatf("Trade from: <%s> at %s", PersonTrading, timetrade);
	WriteChatf("Inventory Slots Available: %i", InventoryCheck);
	WriteChatf("Top Level Inventory Slots Available: %i", openTLinv);
	WriteChatf("\ay====================\ax");
	WriteChatf("TotalInventorySpaceNeeded = %i", totalitemstaken);
	WriteChatf("TotalBagsGivenToMe = %i", bagsreceived);
	WriteChatf("TotalPlatGivenToMe = %i", CoinCount[0]);
	WriteChatf("TotalGoldGivenToMe = %i", CoinCount[1]);
	WriteChatf("TotalSilverGivenToMe = %i", CoinCount[2]);
	WriteChatf("TotalCopperGivenToMe = %i", CoinCount[3]);
	if (totalitems > 0)
	{
		WriteChatf("Items Given to Me:");
		for (int a = 1; a < 9; a++)
		{
			if (_stricmp(ItemReceived[a], "NULL") != 0) WriteChatf(" %i) %s", a, ItemReceived[a]);
		}
		if (numstackable >= 1)
		{
			WriteChatf("Stackable Items:");
			for (int a = 1; a < numstackable + 1; a++)
			{
				WriteChatf(" %i) %s", a, StackedItems[a]);
				WriteChatf("  Received: %i", StackedItemsCount[a]);
				WriteChatf("  Available Stack Room: %i", AvailableRoom[a]);
			}
		}
	}
	WriteChatf("\ay====================\ax");
	if (TradeRejected)
	{
		WriteChatf("\arTRADE REJECTED\ax");
		WriteChatf("Reason: %s", reason);
	}
	else WriteChatf("\agTRADE ACCEPTED\ax");
	WriteChatf("\ay====================\ax");
}

static void DoTrade()
{
	char TLInventoryCheck[MAX_STRING];
	char itemstakencheck[MAX_STRING];
	char stackablecheck[MAX_STRING];
	char bagcheck[MAX_STRING];
	int tradeslots;
	char FreeInventoryCheck[MAX_STRING];
	char CoinCheck[MAX_STRING];
	char NumberinStack[MAX_STRING];
	char MaxNumberinStack[MAX_STRING];
	char AvailableStackRoom[MAX_STRING];
	bool matchfound;
	int addstackable;

	for (int a = 0; a < 4; a++)
		CoinCount[a] = 0;

	for (int a = 1; a < 9; a++)
	{
		sprintf_s(ItemReceived[a], "NULL");
		sprintf_s(StackedItems[a], "NULL");
		StackedItemsCount[a] = 0;
		StacksCount[a] = 0;
		RemainCount[a] = 0;
		AvailableRoom[a] = 0;
	}

	for (int a = 0; a < 4; a++)
	{
		sprintf_s(CoinCheck, "${Window[Tradewnd].Child[TRDW_HisMoney%i].Text}", a);
		ParseMacroData(CoinCheck, sizeof(CoinCheck));
		CoinCount[a] = atoi(CoinCheck);
	}

	openTLinv = 0;
	matchfound = false;
	sprintf_s(FreeInventoryCheck, "${Me.FreeInventory}");
	ParseMacroData(FreeInventoryCheck, sizeof(FreeInventoryCheck));
	InventoryCheck = atoi(FreeInventoryCheck);
	for (int TLInv = 23; TLInv < 31; TLInv++)
	{
		sprintf_s(TLInventoryCheck, "${Me.Inventory[%i]}", TLInv);
		ParseMacroData(TLInventoryCheck, sizeof(TLInventoryCheck));
		if (!_stricmp(TLInventoryCheck, "NULL")) openTLinv++;
	}

	totalitemstaken = 0;
	bagsreceived = 0;
	numstackable = 0;
	totalitems = 0;

	for (tradeslots = 9; tradeslots < 17; tradeslots++)
	{
		sprintf_s(itemstakencheck, "${InvSlot[trade%i].Item}", tradeslots);
		if (!CheckWindow(itemstakencheck))
		{
			totalitems++;
			sprintf_s(bagcheck, "${InvSlot[trade%i].Item.Container}", tradeslots);
			ParseMacroData(bagcheck, sizeof(bagcheck));
			if (_stricmp(bagcheck, "0") != 0) bagsreceived++;
			sprintf_s(stackablecheck, "${InvSlot[trade%i].Item.Stackable}", tradeslots);
			ParseMacroData(stackablecheck, sizeof(stackablecheck));
			if (_stricmp(stackablecheck, "FALSE") != 0)
			{
				matchfound = false;
				sprintf_s(NumberinStack, "${InvSlot[trade%i].Item.Stack}", tradeslots);
				ParseMacroData(NumberinStack, sizeof(NumberinStack));
				sprintf_s(MaxNumberinStack, "${InvSlot[trade%i].Item.StackSize}", tradeslots);
				ParseMacroData(MaxNumberinStack, sizeof(MaxNumberinStack));

				int n = 1;
				while (n < 9 && !matchfound)
				{
					if (!_stricmp(itemstakencheck, StackedItems[n]))
					{
						StackedItemsCount[n] = atoi(NumberinStack) + StackedItemsCount[n];
						matchfound = true;
					}
					else n++;
				}
				if (!matchfound)
				{
					numstackable++;
					sprintf_s(StackedItems[numstackable], "%s", itemstakencheck);
					StackedItemsCount[numstackable] = atoi(NumberinStack);
					MaxStackedItemsCount[numstackable] = atoi(MaxNumberinStack);
				}
			}
			else
			{
				totalitemstaken++;
				sprintf_s(ItemReceived[totalitemstaken], "%s", itemstakencheck);
			}
		}
	}
	addstackable = totalitemstaken;
	if (numstackable >= 1)
	{
		for (int a = 1; a < numstackable + 1; a++)
		{
			sprintf_s(ItemReceived[addstackable + a], "%s", StackedItems[a]);
			sprintf_s(AvailableStackRoom, "${FindItem[=%s].FreeStack}", StackedItems[a]);
			ParseMacroData(AvailableStackRoom, sizeof(AvailableStackRoom));
			AvailableRoom[a] = atoi(AvailableStackRoom);
			StacksCount[a] = MaxStackedItemsCount[a] != 0 ? int(StackedItemsCount[a] / MaxStackedItemsCount[a]) : 0;
			RemainCount[a] = MaxStackedItemsCount[a] != 0 ? StackedItemsCount[a] % MaxStackedItemsCount[a] : 0;
			if (AvailableRoom[a] < StackedItemsCount[a])
			{
				totalitemstaken = totalitemstaken + StacksCount[a];
				if (RemainCount[a] > AvailableRoom[a]) totalitemstaken++;
			}
		}
	}

	if ((!AllowGoldandLowerDump) && (CoinCount[1] > 0 || CoinCount[2] > 0 || CoinCount[3] > 0))
	{
		sprintf_s(ReportReject, "/tell %s Keep your chump change -- I only accept plat!", PersonTrading);
		RejectTrade();
		TradeDone = true;
		TradeRejected = true;
		sprintf_s(reason, "Given money other than plat while I'm not accepting that.");
		return;
	}
	if (bagsreceived > openTLinv)
	{
		if (openTLinv < 1) sprintf_s(ReportReject, "/tell %s I don't have any room for bags", PersonTrading);
		if (openTLinv > 0) sprintf_s(ReportReject, "/tell %s I only have room for %i bag", PersonTrading, openTLinv);
		if (openTLinv > 1) sprintf_s(ReportReject, "/tell %s I only have room for %i bags", PersonTrading, openTLinv);
		RejectTrade();
		TradeDone = true;
		TradeRejected = true;
		sprintf_s(reason, "Given more bags than I have top level inventory room for");
		return;
	}

	if (InventoryCheck < totalitemstaken)
	{
		if (InventoryCheck < 1) sprintf_s(ReportReject, "/tell %s I don't have any room in my inventory", PersonTrading);
		if (InventoryCheck > 0) sprintf_s(ReportReject, "/tell %s I only have room for %i item in my inventory", PersonTrading, InventoryCheck);
		if (InventoryCheck > 1) sprintf_s(ReportReject, "/tell %s I only have room for %i items in my inventory", PersonTrading, InventoryCheck);
		RejectTrade();
		TradeDone = true;
		TradeRejected = true;
		sprintf_s(reason, "Given more items than I have room for.");
		return;
	}

	AcceptTrade();
	TradeDone = true;
}

static bool CheckNames(const char* szNameArg)
{
	for (int i = 0; i < (TotalNames + 1); i++)
	{
		if (!_stricmp(szNameArg, Name[i])) return true;
	}
	return false;
}

static void DoAutoTrade(SPAWNINFO* pChar, const char* szLine)
{
	char szTemp[MAX_STRING];
	char szLineBuf[MAX_STRING];
	strcpy_s(szLineBuf, szLine);

	if (gGameState == GAMESTATE_INGAME)
	{
		if (szLineBuf[0] == 0)
		{
			ShowHelp();
			return;
		}
		if (!_stricmp(szLineBuf, "TRUE"))
		{
			AutoAcceptEveryone = true;
			WritePrivateProfileString("Settings", "AutoAcceptEveryone", szLineBuf, INIFileName);
			WriteChatf("AutoAcceptEveryone is now: %s", AutoAcceptEveryone ? "\agTRUE" : "\arFALSE");
		}
		else if (!_stricmp(szLineBuf, "FALSE"))
		{
			AutoAcceptEveryone = false;
			WritePrivateProfileString("Settings", "AutoAcceptEveryone", szLineBuf, INIFileName);
			WriteChatf("AutoAcceptEveryone is now: %s", AutoAcceptEveryone ? "\agTRUE" : "\arFALSE");
		}
		else if (!_stricmp(szLineBuf, "report"))
		{
			if (ReportTradeReject) { ReportTradeReject = false; sprintf_s(szLineBuf, "FALSE"); }
			else { ReportTradeReject = true; sprintf_s(szLineBuf, "TRUE"); }
			WritePrivateProfileString("Settings", "ReportTradeReject", szLineBuf, INIFileName);
			WriteChatf("ReportTradeReject is now: %s", ReportTradeReject ? "\agTRUE" : "\arFALSE");
		}
		else if (!_stricmp(szLineBuf, "echo"))
		{
			if (EchoTradeInfo) { EchoTradeInfo = false; sprintf_s(szLineBuf, "FALSE"); }
			else { EchoTradeInfo = true; sprintf_s(szLineBuf, "TRUE"); }
			WritePrivateProfileString("Settings", "EchoTradeInfo", szLineBuf, INIFileName);
			WriteChatf("EchoTradeInfo is now: %s", EchoTradeInfo ? "\agTRUE" : "\arFALSE");
		}
		else if (!_stricmp(szLineBuf, "gold"))
		{
			if (AllowGoldandLowerDump) { AllowGoldandLowerDump = false; sprintf_s(szLineBuf, "FALSE"); }
			else { AllowGoldandLowerDump = true; sprintf_s(szLineBuf, "TRUE"); }
			WritePrivateProfileString("Settings", "AllowGoldandLowerDump", szLineBuf, INIFileName);
			WriteChatf("AllowGoldandLowerDump is now: %s", AllowGoldandLowerDump ? "\agTRUE" : "\arFALSE");
		}
		else
		{
			WriteChatf("Adding \ay< %s >\ax to list of people you'll always trade with", szLineBuf);
			sprintf_s(szTemp, "Name%i", TotalNames);
			WritePrivateProfileString("Names", szTemp, szLineBuf, INIFileName);
			TotalNames++;
			Load_INI();
			return;
		}
		Load_INI();
	}
}

PLUGIN_API void OnPulse()
{
	char tradeacceptcheck[MAX_STRING];
	char platcheck[MAX_STRING];
	char windowcheck[MAX_STRING];
	char goldcheck[MAX_STRING];
	char silvercheck[MAX_STRING];
	char coppercheck[MAX_STRING];
	char itemsgivencheck[MAX_STRING];

	if (gGameState == GAMESTATE_INGAME)
	{
		sprintf_s(windowcheck, "${Window[tradewnd].Open}");
		sprintf_s(itemsgivencheck, "${InvSlot[trade1].Item}");
		sprintf_s(platcheck, "${Window[Tradewnd].Child[TRDW_MyMoney0].Text.Equal[0]}");
		sprintf_s(goldcheck, "${Window[Tradewnd].Child[TRDW_MyMoney1].Text.Equal[0]}");
		sprintf_s(silvercheck, "${Window[Tradewnd].Child[TRDW_MyMoney2].Text.Equal[0]}");
		sprintf_s(coppercheck, "${Window[Tradewnd].Child[TRDW_MyMoney3].Text.Equal[0]}");
		sprintf_s(tradeacceptcheck, "${Window[Tradewnd].HisTradeReady}");

		if (!CheckWindow(windowcheck))
		{
			TradeDone = false;
			TradeRejected = false;
		}
		else if ((CheckWindow(itemsgivencheck)) && (CheckWindow(platcheck)) && (CheckWindow(goldcheck))
			&& (CheckWindow(silvercheck)) && (CheckWindow(coppercheck)) && (CheckWindow(tradeacceptcheck))
			&& !TradeDone)
		{
			sprintf_s(PersonTrading, "${Window[Tradewnd].Child[TRDW_HisName].Text}");
			ParseMacroData(PersonTrading, sizeof(PersonTrading));
			DoTrade();
			if ((!AutoAcceptEveryone) && !(CheckNames(PersonTrading)))
			{
				RejectTrade();
				sprintf_s(ReportReject, "/tell %s I'm not interested in anything you have", PersonTrading);
				TradeRejected = true;
				TradeDone = true;
				sprintf_s(reason, "Not accepting all trades and person not on always accept list.");
			}
			DoCommand(pLocalPlayer, tradecommand);
			if ((ReportTradeReject) && (TradeRejected)) DoCommand(pLocalPlayer, ReportReject);
			if (EchoTradeInfo) EchoTrade();
		}
	}
}
