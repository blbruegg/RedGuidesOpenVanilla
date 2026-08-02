// MQ2AutoAccept.cpp : RedGuides exclusive auto accept group/raid invite, clicking of dialog boxes (group task adds, expeditions, and wizard teleports among others)
// v1.0 - Sym - 04-23-2012
// v1.01 - Sym - 08-05-2012 - Site name update
// v1.02 - Sym - 09-27-2012 - Added GlobalNames ini section. Needs to be edited in ini file, add/del does not affect globals.
// v1.03 - Eqmule - 04-23-2016 - removed ParseMacroData because this is a plugin not a macro ;) added accept for another dialog.
// v2.0 - Eqmule 07-22-2016 - Added string safety.
// v2.1 - Sym - 07-23-2017 - Added specific anchor name checking and addanchor/delanchor commands.
//    /autoaccept addanchor VALUE :: Add VALUE as a valid anchor target. Proper case matters for matching later. Put the entire address inside quotes as it shows in the portal dialog box such as "Willow Circle Bay, 100 Vanward Heights"
//    /autoaccept delanchor VALUE :: Delete VALUE from your valid anchor target list. Put the entire address inside quotes as it shows in the portal dialog box such as "Willow Circle Bay, 100 Vanward Heights"
// v2.11 - Sym - 07-28-2017 - Cleaned up anchor port code, added selfanchor option
//    /autoaccept selfanchor on|off :: Toggle acceptance of primary/secondary real estate anchor port when you cast it.  Default *OFF*
// v2.12 - Sym - 08-07-2017 - Added wizard translocate and druid zephyr casts to translocate toggle. Previously it was translocate to bind only.
// v2.13 - Eqmule - 06-04-2018 - Fixed a null ptr crash and added a check for a rez message so it wont accept that, its not this plugins job to accept rezzes.
#include <mq/Plugin.h>

PreSetup("MQ2AutoAccept");
PLUGIN_VERSION(2.13);

std::vector <std::string> vIniNames;
std::vector <std::string> vGlobalNames;
std::vector <std::string> vNames;
std::vector <std::string> vAnchors;

constexpr int MAX_TRADE_COIN_SLOTS = 4;

char szList[MAX_STRING];
bool bAutoAccept = false;
bool bTranslocate = false;
bool bAnchor = false;
bool bSelfAnchor = false;
bool bTrade = true;
bool bTradeAlways = false;
bool bGroup = true;
bool bFellowship = true;
bool bRaid = true;
bool bInitDone = false;
bool bTradeReject = false;
bool bUseServerNames = false;

ULONGLONG rejectTimer = 0;


void CombineNames() {
	vNames.clear();
	for (auto& vRef : vGlobalNames)
	{
		vNames.emplace_back(vRef);
	}
	for (auto& vRef : vIniNames)
	{
		vNames.emplace_back(vRef);
	}
}

bool WindowOpen(PCHAR WindowName) {
	const auto pWnd = dynamic_cast<CSidlScreenWnd*>(FindMQ2Window(WindowName));
	return pWnd && pWnd->IsVisible();
}

void ListUsers() {
	WriteChatf("User list contains \ay%d\ax %s", vNames.size(), vNames.size() > 1 ? "entries" : "entry");
	for (unsigned int a = 0; a < vNames.size(); a++) {
		std::string& vRef = vNames[a];
		WriteChatf("\at%d: %s\ax", a+1, vRef.c_str());
	}
}

void ListAnchors() {
	WriteChatf("Anchor list contains \ay%d\ax %s", vAnchors.size(), vAnchors.size() > 1 ? "entries" : "entry");
	for (unsigned int a = 0; a < vAnchors.size(); a++) {
		std::string& vRef = vAnchors[a];
		WriteChatf("\at%d: %s\ax", a+1, vRef.c_str());
	}
}

std::string GetPrefix(bool UseServerNames)
{
	std::string Prefix = "unknown";
	if (pLocalPC)
	{
		Prefix = fmt::format("{}_", pLocalPC->Name);

		if (UseServerNames)
		{
			Prefix = fmt::format("{}_{}", GetServerShortName(), Prefix);
		}
	}
	return Prefix;
}

void SaveINI()
{
	std::string Prefix = GetPrefix(bUseServerNames);
	WritePrivateProfileBool("General", "UseServerNames", bUseServerNames, INIFileName);

	// write on/off settings
	std::string strSettings = Prefix + "Settings";
	WritePrivateProfileSection(strSettings, "", INIFileName);
	WritePrivateProfileBool(strSettings, "Enabled", bAutoAccept, INIFileName);
	WritePrivateProfileBool(strSettings, "Translocate", bTranslocate, INIFileName);
	WritePrivateProfileBool(strSettings, "Anchor", bAnchor, INIFileName);
	WritePrivateProfileBool(strSettings, "SelfAnchor", bSelfAnchor, INIFileName);
	WritePrivateProfileBool(strSettings, "Trade", bTrade, INIFileName);
	WritePrivateProfileBool(strSettings, "TradeAlways", bTradeAlways, INIFileName);
	WritePrivateProfileBool(strSettings, "TradeReject", bTradeReject, INIFileName);
	WritePrivateProfileBool(strSettings, "Group", bGroup, INIFileName);
	WritePrivateProfileBool(strSettings, "Fellowship", bFellowship, INIFileName);
	WritePrivateProfileBool(strSettings, "Raid", bRaid, INIFileName);

	// write all names
	std::string strNames = Prefix + "Names";
	WritePrivateProfileSection(strNames.c_str(), "", INIFileName);
	for (auto& vRef : vIniNames)
	{
		WritePrivateProfileString(strNames, vRef, "1", INIFileName);
	}

	char szA[MAX_STRING];
	// write all anchors
	std::string strAnchors = Prefix + "Anchors";
	WritePrivateProfileSection(strAnchors.c_str(), "", INIFileName);
	for (unsigned int a = 0; a < vAnchors.size(); a++) {
		std::string& vRef = vAnchors[a];
		sprintf_s(szA,"Anchor%d",a);
		WritePrivateProfileString(strAnchors, szA, vRef, INIFileName);
	}
	WriteChatf("MQ2AutoAccept :: Settings updated");
}

void LoadINI()
{
	// get on/off settings
	PCHARINFO pChar = GetCharInfo();
	if (!pChar)
		return;

	bUseServerNames = GetPrivateProfileBool("General", "UseServerNames", bUseServerNames, INIFileName);

	std::string Prefix = GetPrefix(bUseServerNames);

	std::string strSettings = Prefix + "Settings";

	bAutoAccept = GetPrivateProfileBool(strSettings, "Enabled", true, INIFileName);
	bTranslocate = GetPrivateProfileBool(strSettings, "Translocate", false, INIFileName);
	bAnchor = GetPrivateProfileBool(strSettings, "Anchor", false, INIFileName);
	bSelfAnchor = GetPrivateProfileBool(strSettings, "SelfAnchor", false, INIFileName);
	bTrade = GetPrivateProfileBool(strSettings, "Trade", true, INIFileName);
	bTradeAlways = GetPrivateProfileBool(strSettings, "TradeAlways", false, INIFileName);
	bTradeReject = GetPrivateProfileBool(strSettings, "TradeReject", false, INIFileName);
	bGroup = GetPrivateProfileBool(strSettings, "Group", true, INIFileName);
	bFellowship = GetPrivateProfileBool(strSettings, "Fellowship", true, INIFileName);
	bRaid = GetPrivateProfileBool(strSettings, "Raid", true, INIFileName);

	// get all names
	std::string strNames = Prefix + "Names";
	GetPrivateProfileSection(strNames.c_str(), szList, MAX_STRING, INIFileName);

	// clear list
	vIniNames.clear();

	// TODO: Rather than duplicate the below code multiple times, abstract to function
	char* p = (char*)szList;
	char *pch = 0;
	char *Next_Token1 = 0;
	size_t length = 0;
	int i = 0;
	// loop through all entries under _Names
	// values are terminated by \0, final value is teminated with \0\0
	// values look like
	// Charactername=1
	while (*p)
	{
		length = strlen(p);
		// split entries on =
		pch = strtok_s(p,"=",&Next_Token1);
		while (pch != nullptr)
		{
			// Odd entries are the names. Add it to the list
			vIniNames.push_back(pch);

			// next is value. Don't use it so skip it
			pch = strtok_s(nullptr, "=", &Next_Token1);

			// next name
			pch = strtok_s(nullptr, "=", &Next_Token1);
			i++;
		}
		p += length;
		p++;
	}
	// if we have entries show them
	GetPrivateProfileSection("Global_Names",szList,MAX_STRING,INIFileName);
	vGlobalNames.clear();

	p = (char*)szList;
	i = 0;
	// loop through all entries under _Names
	// values are terminated by \0, final value is teminated with \0\0
	// values look like
	// Charactername=1
	while (*p)
	{
		length = strlen(p);
		// split entries on =
		pch = strtok_s(p,"=",&Next_Token1);
		while (pch != nullptr)
		{
			// Odd entries are the names. Add it to the list
			vGlobalNames.push_back(pch);

			// next is value. Don't use it so skip it
			pch = strtok_s(nullptr, "=",&Next_Token1);

			// next name
			pch = strtok_s(nullptr, "=",&Next_Token1);
			i++;
		}
		p += length;
		p++;
	}

	// if we have entries show them
	CombineNames();
	if (vNames.size())
		ListUsers();

	// get all anchors
	std::string strAnchors = Prefix + "Anchors";
	GetPrivateProfileSection(strAnchors.c_str(), szList, MAX_STRING, INIFileName);

	// clear list
	vAnchors.clear();

	p = (char*)szList;
	i = 0;
	// loop through all entries under _Names
	// values are terminated by \0, final value is teminated with \0\0
	// values look like
	// Anchor0=anchorname
	while (*p)
	{
		length = strlen(p);
		// split entries on =
		pch = strtok_s(p,"=",&Next_Token1);
		while (pch != nullptr)
		{
			// Odd values are the numbered entries. Don't use it so skip it
			pch = strtok_s(nullptr, "=",&Next_Token1);

			// Even entries are the anchor values. Add it to the list
			if(pch)
				vAnchors.push_back(pch);

			// next anchor
			pch = strtok_s(nullptr, "=",&Next_Token1);
			i++;
		}
		p += length;
		p++;
	}
	if (!vAnchors.empty())
		ListAnchors();
	// flag first load init as done

	bInitDone = true;
}

PLUGIN_API void SetGameState(int GameState) {
	if(GameState==GAMESTATE_INGAME) {
		if (!bInitDone)
			LoadINI();
	} else if(GameState!=GAMESTATE_LOGGINGIN) {
		if (bInitDone)
			bInitDone=false;
	}
}

void ShowHelp() {
	WriteChatf("\atMQ2AutoAccept :: v%1.2f :: by Sym for RedGuides.com\ax", MQ2Version);
	WriteChatf("/autoaccept :: Lists command syntax");
	WriteChatf("/autoaccept on|off :: Main accept toggle. Nothing else will accept if this is off. Default \ag*ON*\ax");
	WriteChatf("/autoaccept translocate on|off :: Toggle acceptance of translocate or zephyr port.  Default \ar*OFF*\ax");
	WriteChatf("/autoaccept anchor on|off :: Toggle acceptance of primary/secondary real estate anchor port.  Default \ar*OFF*\ax");
	WriteChatf("/autoaccept selfanchor on|off :: Toggle acceptance of primary/secondary real estate anchor port when you cast it.  Default \ar*OFF*\ax");
	WriteChatf("/autoaccept trade on|off :: Toggle acceptance of trades by people on the auto accept list. Default \ag*ON*\ax");
	WriteChatf("/autoaccept trade always on|off :: Toggles always accept all trades. Default \ar*OFF*\ax");
	WriteChatf("/autoaccept trade reject on|off :: Reject trades for people not on auto accept list after 5 seconds. Default \ar*OFF*\ax");
	WriteChatf("/autoaccept group on|off :: Toggles accept group invites. Default \ag*ON*\ax");
	WriteChatf("/autoaccept fellowship on|off :: Toggles accept fellowship invites. Default \ag*ON*\ax");
	WriteChatf("/autoaccept raid on|off :: Toggles accept raid invites. Default \ag*ON*\ax");
	WriteChatf("/autoaccept status :: Lists status of toggles.");
	WriteChatf("/autoaccept list :: Lists users on your auto accept list.");
	WriteChatf("/autoaccept save :: Saves settings to ini. Changes \arDO NOT\ax auto save.");
	WriteChatf("/autoaccept load :: Loads settings from ini");
	WriteChatf("/autoaccept add NAME :: Add NAME to the auto accept list.");
	WriteChatf("/autoaccept del NAME :: Delete NAME from your auto accept list.");
	WriteChatf("/autoaccept addanchor VALUE :: Add VALUE as a valid anchor target. Put the entire address inside quotes as it shows in the portal dialog box such as \ag\"Willow Circle Bay, 100 Vanward Heights\"\ax");
	WriteChatf("/autoaccept delanchor VALUE :: Delete VALUE from your valid anchor target list. Put the entire address inside quotes as it shows in the portal dialog box such as \ag\"Willow Circle Bay, 100 Vanward Heights\"\ax");
}

void AutoAcceptCommand(PSPAWNINFO pCHAR, PCHAR zLine) {
	char szTemp[MAX_STRING] = { 0 };
	GetArg(szTemp,zLine,1);

	if(!_strnicmp(szTemp,"load",4)) {
		LoadINI();
		return;
	}
	if(!_strnicmp(szTemp,"save",4)) {
		SaveINI();
		return;
	}

	if(!_strnicmp(szTemp,"status",6)) {
		WriteChatf("MQ2AutoAccept :: %s", bAutoAccept ? "\agENABLED\ax" : "\arDISABLED\ax");
		WriteChatf("MQ2AutoAccept :: Anchor portal accept is %s", bAnchor ? "\agON\ax" : "\arOFF\ax");
		WriteChatf("MQ2AutoAccept :: Self anchor portal accept is %s", bSelfAnchor ? "\agON\ax" : "\arOFF\ax");
		WriteChatf("MQ2AutoAccept :: Group accept is %s", bGroup ? "\agON\ax" : "\arOFF\ax");
		WriteChatf("MQ2AutoAccept :: Fellowship accept is %s", bFellowship ? "\agON\ax" : "\arOFF\ax");
		WriteChatf("MQ2AutoAccept :: Raid accept is %s", bRaid ? "\agON\ax" : "\arOFF\ax");
		WriteChatf("MQ2AutoAccept :: Trade accept is %s", bTrade ? "\agON\ax" : "\arOFF\ax");
		WriteChatf("MQ2AutoAccept :: Trade reject is %s", bTradeReject ? "\agON\ax" : "\arOFF\ax");
		WriteChatf("MQ2AutoAccept :: Trade always accept is %s", bTradeAlways ? "\agON\ax" : "\arOFF\ax");
		WriteChatf("MQ2AutoAccept :: Translocate accept is %s", bTranslocate ? "\agON\ax" : "\arOFF\ax");
		return;
	}

	if(!_strnicmp(szTemp,"on",2)) {
		bAutoAccept = true;
		WriteChatf("MQ2AutoAccept :: \agEnabled\ax");
	}
	else if(!_strnicmp(szTemp,"off",3)) {
		bAutoAccept = false;
		WriteChatf("MQ2AutoAccept :: \arDisabled\ax");
	}
	else if(!_strnicmp(szTemp,"list",4)) {
		ListUsers();
		ListAnchors();
	}
	else if(!_strnicmp(szTemp,"addanchor",9)) {
		GetArg(szTemp,zLine,2);
		if(!_strcmpi(szTemp, "")) {
			WriteChatf("Usage: /autoaccept addanchor \"VALUE\"");
			return;
		}
		for (unsigned int a = 0; a < vAnchors.size(); a++) {
			std::string& vRef = vAnchors[a];
			if (!_strcmpi(szTemp,vRef.c_str())) {
				WriteChatf("MQ2AutoAccept :: Anchor \ay%s\ax already exists", szTemp);
				return;
			}
		}
		vAnchors.push_back(szTemp);
		WriteChatf("MQ2AutoAccept :: Added \ay%s\ax to anchor list", szTemp);
	}
	else if(!_strnicmp(szTemp,"add",3)) {
		GetArg(szTemp,zLine,2);
		if(!_strcmpi(szTemp, "")) {
			WriteChatf("Usage: /autoaccept add NAME");
			return;
		}
		for (unsigned int a = 0; a < vIniNames.size(); a++) {
			std::string& vRef = vIniNames[a];
			if (!_strcmpi(szTemp,vRef.c_str())) {
				WriteChatf("MQ2AutoAccept :: User \ay%s\ax already exists", szTemp);
				return;
			}
		}
		vIniNames.push_back(szTemp);
		CombineNames();
		WriteChatf("MQ2AutoAccept :: Added \ay%s\ax to name list", szTemp);
	}
	else if(!_strnicmp(szTemp,"delanchor",9)) {
		int delIndex = -1;
		GetArg(szTemp,zLine,2);
		for (unsigned int a = 0; a < vAnchors.size(); a++) {
			std::string& vRef = vAnchors[a];
			if (!_strcmpi(szTemp,vRef.c_str())) {
				delIndex = a;
			}
		}
		if (delIndex >= 0) {
			vAnchors.erase(vAnchors.begin() + delIndex);
			WriteChatf("MQ2AutoAccept :: Deleted anchor \ay%s\ax", szTemp);
		} else {
			WriteChatf("MQ2AutoAccept :: Anchor \ay%s\ax not found", szTemp);
		}
	}
	else if(!_strnicmp(szTemp,"del",3)) {
		int delIndex = -1;
		GetArg(szTemp,zLine,2);
		for (unsigned int a = 0; a < vIniNames.size(); a++) {
			std::string& vRef = vIniNames[a];
			if (!_strcmpi(szTemp,vRef.c_str())) {
				delIndex = a;
			}
		}
		if (delIndex >= 0) {
			CombineNames();
			vIniNames.erase(vIniNames.begin() + delIndex);
			WriteChatf("MQ2AutoAccept :: Deleted user \ay%s\ax", szTemp);
		} else {
			WriteChatf("MQ2AutoAccept :: User \ay%s\ax not found", szTemp);
		}
	}
	else if(!_strnicmp(szTemp,"selfanchor",10)) {
		GetArg(szTemp,zLine,2);
		if(!_strnicmp(szTemp,"on",2)) {
			bSelfAnchor = true;
		} else if(!_strnicmp(szTemp,"off",3)) {
			bSelfAnchor = false;
		}
		WriteChatf("MQ2AutoAccept :: Self anchor portal accept is %s", bSelfAnchor ? "\agON\ax" : "\arOFF\ax");
	}
	else if(!_strnicmp(szTemp,"anchor",6)) {
		GetArg(szTemp,zLine,2);
		if(!_strnicmp(szTemp,"on",2)) {
			bAnchor = true;
		} else if(!_strnicmp(szTemp,"off",3)) {
			bAnchor = false;
		}
		WriteChatf("MQ2AutoAccept :: Anchor portal accept is %s", bAnchor ? "\agON\ax" : "\arOFF\ax");
	}
	else if(!_strnicmp(szTemp,"group",5)) {
		GetArg(szTemp,zLine,2);
		if(!_strnicmp(szTemp,"on",2)) {
			bGroup = true;
		} else if(!_strnicmp(szTemp,"off",3)) {
			bGroup = false;
		}
		WriteChatf("MQ2AutoAccept :: Group accept is %s", bGroup ? "\agON\ax" : "\arOFF\ax");
	}
	else if(!_strnicmp(szTemp,"fellowship",10)) {
		GetArg(szTemp,zLine,2);
		if(!_strnicmp(szTemp,"on",2)) {
			bFellowship = true;
		} else if(!_strnicmp(szTemp,"off",3)) {
			bFellowship = false;
		}
		WriteChatf("MQ2AutoAccept :: Fellowship accept is %s", bFellowship ? "\agON\ax" : "\arOFF\ax");
	}
	else if(!_strnicmp(szTemp,"raid",4)) {
		GetArg(szTemp,zLine,2);
		if(!_strnicmp(szTemp,"on",2)) {
			bRaid = true;
		} else if(!_strnicmp(szTemp,"off",3)) {
			bRaid = false;
		}
		WriteChatf("MQ2AutoAccept :: Raid accept is %s", bRaid ? "\agON\ax" : "\arOFF\ax");
	}
	else if(!_strnicmp(szTemp,"trade",5)) {
		GetArg(szTemp,zLine,2);
		if(!_strnicmp(szTemp,"on",2)) {
			bTrade = true;
		} else if(!_strnicmp(szTemp,"off",3)) {
			bTrade = false;
			bTradeAlways = false;
		}
		else if(!_strnicmp(szTemp,"reject",6)) {
			GetArg(szTemp,zLine,3);
			if(!_strcmpi(szTemp, "")) {
				WriteChatf("Usage: /autoaccept trade reject on|off");
				return;
			}
			if(!_strnicmp(szTemp,"on",2)) {
				bTrade = true;
				bTradeReject = true;
			} else if(!_strnicmp(szTemp,"off",3)) {
				bTradeReject = false;
			}
			WriteChatf("MQ2AutoAccept :: Trade reject is %s", bTradeReject ? "\agON\ax" : "\arOFF\ax");
			return;
		}
		else if(!_strnicmp(szTemp,"always",6)) {
			GetArg(szTemp,zLine,3);
			if(!_strcmpi(szTemp, "")) {
				WriteChatf("Usage: /autoaccept trade always on|off");
				return;
			}
			if(!_strnicmp(szTemp,"on",2)) {
				bTrade = true;
				bTradeAlways = true;
			} else if(!_strnicmp(szTemp,"off",3)) {
				bTradeAlways = false;
			}
			WriteChatf("MQ2AutoAccept :: Trade always accept is %s", bTradeAlways ? "\agON\ax" : "\arOFF\ax");
			return;
		}
		WriteChatf("MQ2AutoAccept :: Trade accept is %s", bTrade ? "\agON\ax" : "\arOFF\ax");
	}
	else if(!_strnicmp(szTemp,"translocate",11)) {
		GetArg(szTemp,zLine,2);
		if(!_strnicmp(szTemp,"on",2)) {
			bTranslocate = true;
		} else if(!_strnicmp(szTemp,"off",3)) {
			bTranslocate = false;
		}
		WriteChatf("MQ2AutoAccept :: Translocate accept is %s", bTranslocate ? "\agON\ax" : "\arOFF\ax");
	}
	else {
		ShowHelp();
	}
}


// Called once, when the plugin is to initialize
PLUGIN_API void InitializePlugin() {
	DebugSpewAlways("Initializing MQ2AutoAccept");
	AddCommand("/autoaccept",AutoAcceptCommand);
}

// Called once, when the plugin is to shutdown
PLUGIN_API void ShutdownPlugin() {
	DebugSpewAlways("Shutting down MQ2AutoAccept");
	RemoveCommand("/autoaccept");
}

// TODO: This signature should be const char* but need to fix use of GetArg below.
PLUGIN_API bool OnIncomingChat(PCHAR Line, DWORD Color)
{
	// No users, abort
	if (vNames.empty())
		return false;

	PCHARINFO pChar = GetCharInfo();
	if (!pChar)
		return false;

	if (bAutoAccept) {
		char szName[MAX_STRING] = { 0 };
		if (strstr(Line, "invites you to join a group.") && bGroup) {
			GetArg(szName, Line, 1);
			// loop through user list and find a match for inviter. If found join group
			for (auto& vRef : vNames)
			{
				if (!_strcmpi(szName, vRef.c_str())) {
					DoCommand(pChar->pSpawn, "/timed 3s /invite");
					WriteChatf("\agMQ2AutoAccept :: Joining group with %s\ax", szName);
				}
			}
		}
		else if (strstr(Line, "invites you to join a fellowship.") && bFellowship) {
			GetArg(szName, Line, 1);
			// loop through user list and find a match for inviter. If found join group
			for (auto& vRef : vNames)
			{
				if (!_strcmpi(szName, vRef.c_str())) {
					DoCommand(pChar->pSpawn, "/timed 3s /invite");
					WriteChatf("\agMQ2AutoAccept :: Joining fellowship with %s\ax", szName);
				}
			}
		}
		else if (strstr(Line, "invites you to join a raid") && bRaid) {
			GetArg(szName, Line, 1);
			// loop through user list and find a match for inviter. If found join raid
			for (auto& vRef : vNames)
			{
				if (!_strcmpi(szName, vRef.c_str())) {
					DoCommand(pChar->pSpawn, "/timed 3s /raidaccept");
					WriteChatf("\agMQ2AutoAccept :: Joining raid with %s\ax", szName);
				}
			}
		}
	}
	return false;
}

void WinClick(CXWnd* Wnd, const char* ScreenID, const char* ClickNotification, DWORD KeyState) {
	if (Wnd && pWndMgr) {
		if (CXWnd* Child = Wnd->GetChildItem(ScreenID)) {
			bool KeyboardFlags[4];
			static_assert(sizeof(KeyboardFlags) == sizeof(pWndMgr->KeyboardFlags));

			memcpy(&KeyboardFlags, &pWndMgr->KeyboardFlags, sizeof(KeyboardFlags));
			memcpy(&pWndMgr->KeyboardFlags, &KeyState, sizeof(KeyboardFlags));

			SendWndClick2(Child, ClickNotification);

			memcpy(&pWndMgr->KeyboardFlags, &KeyboardFlags, sizeof(KeyboardFlags));
		}
	}
}

// This is called every time MQ pulses
PLUGIN_API void OnPulse()
{
	static int Pulse = 0;

	if (GetGameState() != GAMESTATE_INGAME)
		return;

	if (!bInitDone)
		return;

	if (!bAutoAccept)
		return;

	// Process every 40 pulses
	if (++Pulse < 40)
		return;

	Pulse = 0;

	bool clickTrade = false;
	bool givingItem = false;

	// if we've clicked trade no need to check anything, let other person accept or reject
	if (pTradeWnd && pTradeWnd->IsVisible() && !pTradeWnd->bMyReadyTrade) {
		if (pTradeWnd->bHisReadyTrade) {
			if (!pTradeWnd->HisNameLabel) {
				WriteChatf("\arTrade window is not initialized properly! New UI may be enabled or Trade window UI XML might be bad. Temporarily disabling MQ2AutoAccept.");
				bAutoAccept = false;
				return;
			}
			CXStr theirName = pTradeWnd->HisNameLabel->GetText();
			if (bTradeAlways) {
				clickTrade = true;
			} else {
				if (!theirName.empty()) {
					for (auto& vRef : vNames)
					{
						if (ci_equals(theirName, vRef)) {
							clickTrade = true;
							break;
						}
					}
				}
			}
			// Check only the first half of the trade slots (ours)
			for (int n = 0; n < MAX_TRADE_SLOTS / 2; ++n) {
				std::string strSlotName = "TRDW_TradeSlot" + std::to_string(n);
				if (CXWnd* pTRDW_TradeSlotWnd = pTradeWnd->GetChildItem(&strSlotName[0])) {
					const auto toolTip = pTRDW_TradeSlotWnd->GetTooltip();
					if (!toolTip.empty()) {
						//DebugSpew("Giving %s in slot %d",szTemp, n);
						givingItem = true;
						break;
					}
				}
			}
			bool givingMoney = false;
			for (int n = 0; n < MAX_TRADE_COIN_SLOTS; ++n) {
				std::string strSlotName = "TRDW_MyMoney" + std::to_string(n);
				if (CXWnd* pTRDW_MyMoneyWnd = pTradeWnd->GetChildItem(&strSlotName[0])) {
					const auto windowText = pTRDW_MyMoneyWnd->GetWindowText();
					if (!windowText.empty() && GetIntFromString(windowText, 0) > 0) {
						//DebugSpew("Giving %s in slot %d",szTemp, n);
						givingMoney = true;
						break;
					}
				}
			}
			if (givingItem || givingMoney) {
				// We're giving item or coin, don't auto do anything
				//DebugSpew("We're giving item or coin, don't do anything");
			} else {
				if (clickTrade) {
					if (!theirName.empty()) {
						if (CXWnd* pTRDW_Trade_Button = pTradeWnd->GetChildItem("TRDW_Trade_Button")) {
							WriteChatf("\agMQ2AutoAccept :: Accepting trade from %s\ax", theirName.c_str());
							SendWndClick2(pTRDW_Trade_Button,"leftmouseup");
							pTarget = nullptr;
						}
					}
				} else {
					if (bTradeReject) {
						if (rejectTimer == 0)
							rejectTimer = GetTickCount64() + 5000;
						if (rejectTimer < GetTickCount64()) {
							rejectTimer = 0;
							if (CXWnd* pTRDW_Cancel_Button = pTradeWnd->GetChildItem("TRDW_Cancel_Button")) {
								WriteChatf("\arMQ2AutoAccept :: Canceling Trade\ax");
								SendWndClick2(pTRDW_Cancel_Button,"leftmouseup");
							}
						}
					}
				}
			}
		}
	}

	CXWnd* pWnd=(CXWnd *)FindMQ2Window("ConfirmationDialogBox");
	if(pWnd && pWnd->IsVisible()) {
		if(CXWnd* Child=pWnd->GetChildItem("CD_TextOutput")) {
			CStmlWnd* cstm = (CStmlWnd*)Child;
			CXStr windowText = cstm->STMLText;

			if (ci_find_substr(windowText, "percent") != -1 || ci_find_substr(windowText, "return you to your corpse") != -1) {
				// rez request
				//DebugSpew("\agMQ2AutoAccept :: Ignoring rez\ax");
			}
			else if (bTranslocate && (ci_find_substr(windowText, "translocated to your bind point") != -1 || ci_find_substr(windowText, "wish to be translocated by") != -1)) {
				// Translocate request
				WriteChatf("\agMQ2AutoAccept :: Accepting translocate\ax");
				WinClick(FindMQ2Window("ConfirmationDialogBox"),"Yes_Button","leftmouseup",1);
			}
			else if (bAnchor && ci_find_substr(windowText, "to the real estate anchor in") != -1) {
				// Anchor portal request
				for (auto& vRef : vAnchors)
				{
					if (ci_find_substr(windowText, vRef) != -1) {
						WriteChatf("\agMQ2AutoAccept :: Accepting anchor portal to \ax\at%s\ax", vRef.c_str());
						WinClick(FindMQ2Window("ConfirmationDialogBox"),"Yes_Button","leftmouseup",1);
					}
				}
			}
			else if (bSelfAnchor && ci_find_substr(windowText, "transport yourself to the real estate") != -1) {
				// we cast the portal, accept it
				WriteChatf("\agMQ2AutoAccept :: Accepting self anchor portal cast\ax");
				WinClick(FindMQ2Window("ConfirmationDialogBox"), "Yes_Button", "leftmouseup", 1);
			}
			else if (ci_find_substr(windowText, "from the fellowship") != -1 && ci_find_substr(windowText, "Remove") != -1) {
				// Remove someone from fellowship.
				// DebugSpew("\agMQ2AutoAccept :: Ignoring removal from fellowship\ax");
			}
			//none of the above? do we have any names
			else if (!vNames.empty()) {
				// All other confirmation boxes
				for (auto& vRef : vNames)
				{
					if(ci_find_substr(windowText,vRef + " ") != -1 || ci_find_substr(windowText,vRef + "'s") != -1) {
						if (pWnd->GetChildItem("Yes_Button")) {
							WriteChatf("\agMQ2AutoAccept :: Clicking Yes\ax");
							WinClick(FindMQ2Window("ConfirmationDialogBox"),"Yes_Button","leftmouseup",1);
						}
						else if (pWnd->GetChildItem("OK_Button")) {
							WriteChatf("\agMQ2AutoAccept :: Clicking OK\ax");
							WinClick(FindMQ2Window("ConfirmationDialogBox"),"OK_Button","leftmouseup",1);
						}
						return;
					}
				}
			}
		}
	}
}
