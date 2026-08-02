// MQ2Notify.cpp : Defines the entry point for the DLL application.
//
// Written by jimbob for MacroQuest VIPs and RedGuides Level2 users.
//
// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup, do NOT do it in DllMain.
#include "../MQ2Plugin.h"
#define		PLUGIN_DATE     20160516
#define		PLUGIN_VERS         0.01
#define		PLUGIN_NAME "MQ2Notify"

PreSetup(PLUGIN_NAME);
PLUGIN_VERSION(PLUGIN_VERS);

#include "CSmtp.h"
#include <iostream>

using namespace std;

string g_Server;
string g_Username;
string g_Password;
string g_FromDisplayName;
string g_FromEmailAddress;
string g_DefaultSubject;
string g_ToEmailAddress;
string g_TextEmailAddress;
int g_Port = 25;
bool Initialized = false;
bool TextInitialized = true;
bool g_Use_TLS = false;
bool g_Use_SSL = false;

void LoadIni();

void SendMailMessage(string server, unsigned short port, string username, string password, string fromname, string fromemail, string toemail, string subject, string body)
{
	bool bError = false;
	try
	{
		CSmtp mail;

		if(g_Use_SSL)
			mail.SetSecurityType(USE_SSL);
		else if (g_Use_TLS)
			mail.SetSecurityType(USE_TLS);
		
		mail.SetSMTPServer(server.c_str(), port);
		mail.SetLogin(username.c_str());
		mail.SetPassword(password.c_str());
		mail.SetSenderName(fromname.c_str());
		mail.SetSenderMail(fromemail.c_str());
		mail.SetReplyTo(fromemail.c_str());
		mail.SetSubject(subject.c_str());
		mail.AddRecipient(toemail.c_str());
		mail.SetXPriority(XPRIORITY_NORMAL);
		mail.SetXMailer("MQ2Notify");
		mail.AddMsgLine(body.c_str());
		mail.Send();
	}
	catch (ECSmtp e)
	{
		cout << "Error: " << e.GetErrorText().c_str() << ".\n";
		bError = true;
	}
	if (!bError)
		cout << "Mail was send successfully.\n";
}

void SendEmail(PSPAWNINFO pCHAR, PCHAR zLine)
{
	if(!Initialized)
		LoadIni();
	if(Initialized)
		SendMailMessage(g_Server, g_Port, g_Username, g_Password, g_FromDisplayName, g_FromEmailAddress, g_ToEmailAddress, g_DefaultSubject, zLine);
}

void SendText(PSPAWNINFO pCHAR, PCHAR zLine)
{
	if(!TextInitialized)
		LoadIni();
	if(TextInitialized)
		SendMailMessage(g_Server, g_Port, g_Username, g_Password, g_FromDisplayName, g_FromEmailAddress, g_TextEmailAddress, g_DefaultSubject, zLine);
}

void SendPushbullet(PSPAWNINFO pCHAR, PCHAR zLine)
{

}

// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializePlugin(VOID)
{
    DebugSpewAlways("Initializing MQ2Notify");

    //Add commands, MQ2Data items, hooks, etc.
    AddCommand("/email", SendEmail);
	AddCommand("/text", SendText);
	//AddCommand("/push", SendPushbullet);
	//AddXMLFile("MQUI_MyXMLFile.xml");
    //bmMyBenchmark=AddMQ2Benchmark("My Benchmark Name");
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownPlugin(VOID)
{
    DebugSpewAlways("Shutting down MQ2Notify");

    //Remove commands, MQ2Data items, hooks, etc.
    //RemoveMQ2Benchmark(bmMyBenchmark);
    RemoveCommand("/email");
	RemoveCommand("/text");
	//RemoveCommand("/push");
	//RemoveXMLFile("MQUI_MyXMLFile.xml");
}

void LoadIni()
{
	PCHAR buf[4096] = { 0 };
	DWORD ret = 0;
	_snprintf(INIFileName, 260, "%s\\%s.ini", gszINIPath, PLUGIN_NAME);
	ret = GetPrivateProfileString(PLUGIN_NAME, "Username", NULL, (LPSTR)buf, 4095, INIFileName);
	if (ret) g_Username.assign((const char *)buf);
	ret = GetPrivateProfileString(PLUGIN_NAME, "Password", NULL, (LPSTR)buf, 4095, INIFileName);
	if (ret) g_Password.assign((const char *)buf);
	ret = GetPrivateProfileString(PLUGIN_NAME, "Server", NULL, (LPSTR)buf, 4095, INIFileName);
	if (ret) g_Server.assign((const char *)buf);
	g_Port = GetPrivateProfileInt(PLUGIN_NAME, "Port", 25, INIFileName);

	ret = GetPrivateProfileString(PLUGIN_NAME, "FromDisplayName", "MQ2Notify", (LPSTR)buf, 4095, INIFileName);
	if (ret) g_FromDisplayName.assign((const char *)buf);
	ret = GetPrivateProfileString(PLUGIN_NAME, "FromEmail", "MQ2Notify@macroquest.com", (LPSTR)buf, 4095, INIFileName);
	if (ret) g_FromEmailAddress.assign((const char *)buf);
	ret = GetPrivateProfileString(PLUGIN_NAME, "FromDisplayName", "MQ2Notify", (LPSTR)buf, 4095, INIFileName);
	if (ret) g_FromDisplayName.assign((const char *)buf);
	ret = GetPrivateProfileString(PLUGIN_NAME, "ToEmail", NULL, (LPSTR)buf, 4095, INIFileName);
	if (ret) g_ToEmailAddress.assign((const char *)buf);
	ret = GetPrivateProfileString(PLUGIN_NAME, "TextEmail", NULL, (LPSTR)buf, 4095, INIFileName);
	if (ret) g_TextEmailAddress.assign((const char *)buf);
	ret = GetPrivateProfileString(PLUGIN_NAME, "Subject", "MQ2Notify Notification", (LPSTR)buf, 4095, INIFileName);
	if (ret) g_DefaultSubject.assign((const char *)buf);

	ret = GetPrivateProfileString(PLUGIN_NAME, "Security", "NONE", (LPSTR)buf, 4095, INIFileName);
	if (ret)
	{
		if (strnicmp((const char *)buf, "SSL", 3) == 0)
		{
			g_Use_SSL = true;
			g_Use_TLS = false;
		}
		else if (strnicmp((const char *)buf, "TLS", 3) == 0)
		{
			g_Use_TLS = true;
			g_Use_SSL = false;
		}
		else
		{
			g_Use_SSL = false;
			g_Use_TLS = false;
		}
	}

	if (g_Username.size() && g_Password.size() && g_Server.size() && g_ToEmailAddress.size())
	{
		if (g_TextEmailAddress.size())
			TextInitialized = true;
		Initialized = true;
	}
}

/*
// Called after entering a new zone
PLUGIN_API VOID OnZoned(VOID)
{
    DebugSpewAlways("MQ2Notify::OnZoned()");
}

// Called once directly before shutdown of the new ui system, and also
// every time the game calls CDisplay::CleanGameUI()
PLUGIN_API VOID OnCleanUI(VOID)
{
    DebugSpewAlways("MQ2Notify::OnCleanUI()");
    // destroy custom windows, etc
}

// Called once directly after the game ui is reloaded, after issuing /loadskin
PLUGIN_API VOID OnReloadUI(VOID)
{
    DebugSpewAlways("MQ2Notify::OnReloadUI()");
    // recreate custom windows, etc
}

// Called every frame that the "HUD" is drawn -- e.g. net status / packet loss bar
PLUGIN_API VOID OnDrawHUD(VOID)
{
    // DONT leave in this debugspew, even if you leave in all the others
    //DebugSpewAlways("MQ2Notify::OnDrawHUD()");
}

// Called once directly after initialization, and then every time the gamestate changes
PLUGIN_API VOID SetGameState(DWORD GameState)
{
    DebugSpewAlways("MQ2Notify::SetGameState()");
    //if (GameState==GAMESTATE_INGAME)
    // create custom windows if theyre not set up, etc
}


// This is called every time MQ pulses
PLUGIN_API VOID OnPulse(VOID)
{
    // DONT leave in this debugspew, even if you leave in all the others
    //DebugSpewAlways("MQ2Notify::OnPulse()");
}

// This is called every time WriteChatColor is called by MQ2Main or any plugin,
// IGNORING FILTERS, IF YOU NEED THEM MAKE SURE TO IMPLEMENT THEM. IF YOU DONT
// CALL CEverQuest::dsp_chat MAKE SURE TO IMPLEMENT EVENTS HERE (for chat plugins)
PLUGIN_API DWORD OnWriteChatColor(PCHAR Line, DWORD Color, DWORD Filter)
{
    DebugSpewAlways("MQ2Notify::OnWriteChatColor(%s)",Line);
    return 0;
}

// This is called every time EQ shows a line of chat with CEverQuest::dsp_chat,
// but after MQ filters and chat events are taken care of.
PLUGIN_API DWORD OnIncomingChat(PCHAR Line, DWORD Color)
{
    DebugSpewAlways("MQ2Notify::OnIncomingChat(%s)",Line);
    return 0;
}

// This is called each time a spawn is added to a zone (inserted into EQ's list of spawns),
// or for each existing spawn when a plugin first initializes
// NOTE: When you zone, these will come BEFORE OnZoned
PLUGIN_API VOID OnAddSpawn(PSPAWNINFO pNewSpawn)
{
    DebugSpewAlways("MQ2Notify::OnAddSpawn(%s)",pNewSpawn->Name);
}

// This is called each time a spawn is removed from a zone (removed from EQ's list of spawns).
// It is NOT called for each existing spawn when a plugin shuts down.
PLUGIN_API VOID OnRemoveSpawn(PSPAWNINFO pSpawn)
{
    DebugSpewAlways("MQ2Notify::OnRemoveSpawn(%s)",pSpawn->Name);
}

// This is called each time a ground item is added to a zone
// or for each existing ground item when a plugin first initializes
// NOTE: When you zone, these will come BEFORE OnZoned
PLUGIN_API VOID OnAddGroundItem(PGROUNDITEM pNewGroundItem)
{
    DebugSpewAlways("MQ2Notify::OnAddGroundItem(%d)",pNewGroundItem->DropID);
}

// This is called each time a ground item is removed from a zone
// It is NOT called for each existing ground item when a plugin shuts down.
PLUGIN_API VOID OnRemoveGroundItem(PGROUNDITEM pGroundItem)
{
    DebugSpewAlways("MQ2Notify::OnRemoveGroundItem(%d)",pGroundItem->DropID);
}

// This is called when we receive the EQ_BEGIN_ZONE packet is received
PLUGIN_API VOID BeginZone(VOID)
{
    DebugSpewAlways("MQ2Notify::BeginZone");
}

// This is called when we receive the EQ_END_ZONE packet is received
PLUGIN_API VOID EndZone(VOID)
{
    DebugSpewAlways("MQ2Notify::EndZone");
}
// This is called when pChar!=pCharOld && We are NOT zoning
// honestly I have no idea if its better to use this one or EndZone (above)
PLUGIN_API VOID Zoned(VOID)
{
    DebugSpewAlways("MQ2Notify::Zoned");
}
*/