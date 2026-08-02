// MQ2RemoteCamp.cpp : Defines the entry point for the DLL application.
//

// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time
// are shown below. Remove the ones your plugin does not use.  Always use Initialize
// and Shutdown for setup and cleanup, do NOT do it in DllMain.



#include "../MQ2Plugin.h"
#include <time.h>

PreSetup("MQ2RemoteCamp");

#define CLOCKS(x) (x * CLOCKS_PER_SEC)

char password[MAX_STRING];
int camping, countTime;
double startTime;
PSPAWNINFO ps;
PCHARINFO pc;
bool active = false;
int lastPrinted = -1;

bool VerifyPassword(PCHAR s);
void InitiateCountdown(PCHAR szName);
void DoCmdRemoteCamp(PSPAWNINFO pChar, PCHAR szLine);
void PrintHelp(void);
void PrintOptions(void);
void PrintAbout(void);

// Called once, when the plugin is to initialize
PLUGIN_API VOID InitializePlugin(VOID)
{
	DebugSpewAlways("Initializing MQ2RemoteCamp");

	// Add the command
	AddCommand("/remcamp", DoCmdRemoteCamp);

	// Load options from .ini
	GetPrivateProfileString("MQ2RemoteCamp", "Password", "ChangeMe", password, MAX_STRING, INIFileName);
	countTime = GetPrivateProfileInt("MQ2RemoteCamp", "Countdown", 60, INIFileName);
	// Disable the warning for the int to bool cast
#pragma warning(push)
#pragma warning(disable: 4800)
	active = (bool) GetPrivateProfileInt("MQ2RemoteCamp", "Active", 0, INIFileName);
#pragma warning(pop)
}

// Called once, when the plugin is to shutdown
PLUGIN_API VOID ShutdownPlugin(VOID)
{
	DebugSpewAlways("Shutting down MQ2RemoteCamp");
	
	// Remove the command
	RemoveCommand("/remcamp");
}

// Called once directly after initialization, and then every time the gamestate changes
PLUGIN_API VOID SetGameState(DWORD GameState)
{
	DebugSpewAlways("MQ2RemoteCamp::SetGameState()");
	if (GameState==GAMESTATE_INGAME){
		if (active) {
			// If the plugin is active display a reminder when the user enters game
			WriteChatColor("\atMQ2RemoteCamp\ax: Remote camping is currently \ayACTIVE\ax.");
			if (!stricmp(password, "ChangeMe")){
				// If the user didn't change the password and the plugin is active remind them to do so
				WriteChatColor("\arWARNING: \axYou are currently using \atMQ2RemoteCamp\ax with the default password.  For your protection please use \ay/remcamp set password <newpassword>\ax to change it immediately.");
			}
		}
	}
}


// This is called every time MQ pulses
PLUGIN_API VOID OnPulse(VOID)
{
	// Nothing to do if plugin isn't active
	if (!active)
		return;
	// Nothing to do if we're not in game
	if (MQ2Globals::gGameState != GAMESTATE_INGAME)
		return;
	
	char szTemp[MAX_STRING] = {0};
	// Are we on stage 1 of the countdown?
	if (camping == 1) {
		// Check how long since the countdown started
		double elapsed = clock() - startTime;
		if (elapsed > CLOCKS(countTime)){
			// If countdown expired, let the user know and move to stage 2
			WriteChatColor("Transferring countdown to EQ's /camp. You may still abort camping by standing up in the next 30 seconds! (\ay/sit off\ax)");
            camping = 2;
			if (gMacroBlock)
				DoCommand(ps, "/endmacro");
			DoCommand(ps, "/sit on");
			DoCommand(ps, "/camp desktop");
		} else {
			// Decide if we need to print a countdown update and print one if needed
			unsigned long el;
			el = (int)(elapsed / 1000);
			if ((countTime - el) % 5 == 0){
				if (el != lastPrinted){
					lastPrinted = el;
					sprintf(szTemp, "Remote camping countdown: \ay%d\axs", countTime - lastPrinted);
					WriteChatColor(szTemp);
				}
			}
		}
	}
}

// This is called every time EQ shows a line of chat with CEverQuest::dsp_chat,
// but after MQ filters and chat events are taken care of.
PLUGIN_API DWORD OnIncomingChat(PCHAR Line, DWORD Color)
{
	// Nothing to do if we're not in game
	if (MQ2Globals::gGameState != GAMESTATE_INGAME)
		return 0;
	// Nothing to do if plugin isn't active
	if (!active)
		return 0;

	char *s;
	char szName[MAX_STRING];
	int namelen;
	
	// If no countdown process is active check if someone might have requirested one
	if (camping == 0) {
		s = strstr(Line, "tells you, 'camp");
		if (s){
			namelen = (int) (s - Line - 1);
			strncpy(szName, Line, namelen);

			s += 17;
			// Check the remote character supplied a correct password
			if (VerifyPassword(s)){
				// Obtain a PSPAWNINFO for use with DoCommand()
				pc = GetCharInfo();
				if (!pc) {
					WriteChatColor("NULL CharInfo pointer.");
					return 0;
				}

				ps = pc->pSpawn;
				if (!ps) {
					WriteChatColor("NULL SpawnInfo pointer.");
					return 0;
				}

				// Start countdown
				InitiateCountdown(szName);
			}
		}
	} else if (camping == 2) {
		// Reset to stage 0 if the user stands up while in stage 2
		s = strstr(Line, "You abandon your preparations to camp.");
		if (s == Line)
			camping = 0;
	}
	
	return 0;
}


void InitiateCountdown(PCHAR szName){
	char szTemp[MAX_STRING] = {0};
	sprintf(szTemp, "Remote camping command received from \ar%s\ax.", szName);
	WriteChatColor(szTemp);
	sprintf(szTemp, "Starting \ay%u\axs countdown.", countTime);
	WriteChatColor(szTemp);
	WriteChatColor("To stop countdown type: \ay/remcamp abort");

	camping = 1;
	// Save time we started countdown at
	startTime = clock();
	lastPrinted = countTime;
}

bool VerifyPassword(PCHAR s){
	// Check password and account for the ' present at the end of a tell
	char szTemp[MAX_STRING];
	strcpy(szTemp, password);
	strcat(szTemp, "'");
	return (!stricmp(s, szTemp));
}

void DoCmdRemoteCamp(PSPAWNINFO pChar, PCHAR szLine){
	// Parse local command
	char szTemp[MAX_STRING] = {0};
	CHAR Arg[3][MAX_STRING] = {0};
	for (int i=0; i<3; i++)
		GetArg(Arg[i], szLine, i+1);

	if (Arg[0][0]==0) {
		// /remcamp
		PrintHelp();
        return;
	}
	if (!stricmp(Arg[0], "on")){
		// /remcamp on
		// Activate plugin and go to stage 0
		active = true;
		WritePrivateProfileString("MQ2RemoteCamp", "Active", "1", INIFileName);
		WriteChatColor("MQ2RemoteCamp now active.");
		camping = 0;
	} else if (!stricmp(Arg[0], "off")){
		// /remcamp off
		// Deactivate plugin and go to stage 0
		active = false;
		WritePrivateProfileString("MQ2RemoteCamp", "Active", "0", INIFileName);
		WriteChatColor("MQ2RemoteCamp now inactive.");
		camping = 0;
	} else if (!stricmp(Arg[0], "options")){
		// /remcamp options
		PrintOptions();
	} else if (!stricmp(Arg[0], "set")){
		// /remcamp set
		// Exit if 2 values are not given to set
		if (Arg[1][0]==0 || Arg[2][0]==0){
			PrintHelp();
			return;
		}
		
		if (!stricmp(Arg[1], "countdown")){
			// /remcamp set countdown
			countTime = atoi(Arg[2]);
			WritePrivateProfileString("MQ2RemoteCamp", "Countdown", Arg[2], INIFileName);
			sprintf(szTemp, "Countdown time set to \ay%d\axs", countTime);
			WriteChatColor(szTemp);
		} else if (!stricmp(Arg[1], "password")){
			// /remcamp set password
			strcpy(password, Arg[2]);
			WritePrivateProfileString("MQ2RemoteCamp", "Password", Arg[2], INIFileName);
			sprintf(szTemp, "Password set to \ay%s\ax", password);
			WriteChatColor(szTemp);
		} else {
			// Print instructions of invalid options are given
			PrintHelp();
			return;
		}
	} else if (!stricmp(Arg[0], "about")){
		// /remcamp about
		PrintAbout();
	} else if (!stricmp(Arg[0], "abort")){
		// /remcamp abort
		camping = 0;
		WriteChatColor("\agCountdown aborted.\ax");
	} else {
		// Print instructions if an invalid command is given.
		PrintHelp();
		return;
	}
}

void PrintHelp(void){
	// Print instructions
	WriteChatColor("\atMQ2RemoteCamp\ax by anOrcPawn00",USERCOLOR_DEFAULT);
    WriteChatColor("Syntax: /remcamp <command> <parameters>",USERCOLOR_DEFAULT);
	WriteChatColor("Commands:",USERCOLOR_DEFAULT);
	WriteChatColor("   \ayon\ax - enable remote camp",USERCOLOR_DEFAULT);
	WriteChatColor("   \ayoff\ax - disable remote camp",USERCOLOR_DEFAULT);
	WriteChatColor("   \ayabort\ax - aborts countdown in progress",USERCOLOR_DEFAULT);
	WriteChatColor("   \ayoptions\ax - displays current options",USERCOLOR_DEFAULT);
	WriteChatColor("   \ayset <option> <value>\ax - changes the value of an option",USERCOLOR_DEFAULT);
	WriteChatColor("   \ayabout\ax - credits & version",USERCOLOR_DEFAULT);
}

void PrintOptions(void){
	// Print options
	char szTemp[MAX_STRING] = {0};
	WriteChatColor("MQ2RemoteCamp options:");
	sprintf(szTemp, "Countdown = \ay%d\axs", countTime);
	WriteChatColor(szTemp);
	sprintf(szTemp, "Password = \ay%s\ax", password);
	WriteChatColor(szTemp);
}

void PrintAbout(void){
	// Print author & version
	WriteChatColor("MQ2RemoteCamp");
	WriteChatColor("  Author: \ayanOrcPawn00\ax");
	WriteChatColor("  Version: \ay1.1\ax");
}