//* MQ2ScreenShot.cpp : Defines the entry point for the DLL application.
	A simple plugin that will run commands and then take a screenshot.
	After the screenshot is taken it will run another set of commands.
	The idea is to clean your screen so that the screenshot will not
	reflect any of the MQ2 Custom UI elements.
*/


#include "../MQ2Plugin.h"

PreSetup("MQ2ScreenShot");
//NewB Start
#define MAX_COMMANDS  15
typedef char mqCMD[80];

mqCMD preSSCmds[MAX_COMMANDS];
mqCMD postSSCmds[MAX_COMMANDS];

PLUGIN_API VOID ScreenShot(PSPAWNINFO pChar, PCHAR Cmd){
	//Run the commands in our command array. FIXME: use a linked list or some sort of dynamic data type
	//Commands to run prior to taking the Screenshot
	for(int n=0;n < MAX_COMMANDS;n++){
		if(strlen(preSSCmds[n]) > 0)//Make sure we have a command in there.
			DoCommand(GetCharInfo()->pSpawn,preSSCmds[n]);
		//Seperate the command calls
		Sleep(10);
	}
	//Press our screenshot key.
	Sleep(100);
	DoCommand(GetCharInfo()->pSpawn,"/keypress SCREENCAP");
	//Commands to run after to taking the Screenshot
	for(int n=0;n < MAX_COMMANDS;n++){
		if(strlen(postSSCmds[n]) > 0) //Make sure we have a command in there.
			DoCommand(GetCharInfo()->pSpawn,postSSCmds[n]);
		//Seperate the command calls
		Sleep(10);
	}
}
PLUGIN_API VOID ListCMDS(PSPAWNINFO pChar, PCHAR Cmd){
	WriteChatf("Pre-Screenshot Commands");
	for(int n=0;n < MAX_COMMANDS;n++){
		if(strlen(preSSCmds[n]) > 0) //Make sure we have a command in there.
			WriteChatf("%d) %s",n+1, preSSCmds[n]);
	}
	WriteChatf("Post-Screenshot Commands");
	for(int n=0;n < MAX_COMMANDS;n++){
		if(strlen(postSSCmds[n]) > 0) //Make sure we have a command in there.
			WriteChatf("%d) %s",n+1,postSSCmds[n]);
	}
}
PLUGIN_API VOID PreCMD(PSPAWNINFO pChar, PCHAR args){
	bool added = false;
	if(strlen(args) == 0){ // No arguments provided.
		WriteChatf("Please provide a command to add!");
		return;
	}
	//Find the first empty spot and place the command there.
	for(int n=0;n < MAX_COMMANDS;n++){
		if(added) break;//If we have added it then break out of the for.
		if(strlen(preSSCmds[n]) == 0){ //Found an empty one!
			strcpy(preSSCmds[n],args);
			added = true;
		}
	}
	if(added){
		WriteChatf("Added \"%s\" to the pre-screenshot command list!",args);
	}else{
		WriteChatf("Pre-screenshot command list is full! Unable to add \"%s\" to the command list", args);
	}
}
PLUGIN_API VOID RemPreCMD(PSPAWNINFO pChar, PCHAR args){
	bool removed = false;
	if(strlen(args) == 0){ // No arguments provided.
		WriteChatf("Please provide a command to remove!");
		return;
	}
	//Find the first empty spot and place the command there.
	for(int n=0;n < MAX_COMMANDS;n++){
		if(removed) break;//If we have added it then break out of the for.
		WriteChatf("%d, %s, %s, %d",n,preSSCmds[n],args,strcmp(preSSCmds[n],args));
		if(strcmp(preSSCmds[n],args) == 0){ //They are the same!
			strcpy(preSSCmds[n],"");
			removed = true;
		}
	}
	if(removed){
		WriteChatf("Removed \"%s\" from the pre-screenshot command list!",args);
	}else{
		WriteChatf("Unable to locate \"%s\" in the command list", args);
	}
}
PLUGIN_API VOID PostCMD(PSPAWNINFO pChar, PCHAR args){
	bool added = false;
	if(strlen(args) == 0){ // No arguments provided.
		WriteChatf("Please provide a command to add!");
		return;
	}
	//Find the first empty spot and place the command there.
	for(int n=0;n < MAX_COMMANDS;n++){
		if(added) break;//If we have added it then break out of the for.
		if(strlen(postSSCmds[n]) == 0){ //Found an empty one!
			strcpy(postSSCmds[n],args);
			added = true;
		}
	}
	if(added){
		WriteChatf("Added \"%s\" to the post-screenshot command list!",args);
	}else{
		WriteChatf("Post-screenshot command list is full! Unable to add \"%s\" to the command list", args);
	}
}
PLUGIN_API VOID RemPostCMD(PSPAWNINFO pChar, PCHAR args){
	bool removed = false;
	if(strlen(args) == 0){ // No arguments provided.
		WriteChatf("Please provide a command to remove!");
		return;
	}
	//Find the first empty spot and place the command there.
	for(int n=0;n < MAX_COMMANDS;n++){
		if(removed) break;//If we have added it then break out of the for.
		if(strcmp(postSSCmds[n],args) == 0){ //They are the same!
			strcpy(postSSCmds[n],"");
			removed = true;
		}
	}
	if(removed){
		WriteChatf("Removed \"%s\" from the post-screenshot command list!",args);
	}else{
		WriteChatf("Unable to locate \"%s\" in the command list", args);
	}
}
PLUGIN_API VOID SSHelp(PSPAWNINFO pChar, PCHAR args){
	WriteChatColor("MQ2ScreenShot Command list:",USERCOLOR_SYSTEM);
	WriteChatColor("/SS - Take a screenshot.",USERCOLOR_GROUP);
	WriteChatColor("/SSHelp - Command list.",USERCOLOR_GROUP);
	WriteChatColor("/SSList - Lists all the commands that will execute when /SS is called.", USERCOLOR_GROUP);
	WriteChatColor("/SSPre arg - Adds the provided argument to the list of commands that will be run prior to taking a screenshot.", USERCOLOR_GROUP);
	WriteChatColor("/SSRemPre arg - Removes the provided argument from the list of commands that will be run prior to taking a screenshot.",USERCOLOR_GROUP);
	WriteChatColor("/SSPost arg - Adds the provided argument to the list of commands that will be run after to taking a screenshot.", USERCOLOR_GROUP);
	WriteChatColor("/SSRemPost arg - Removes the provided argument from the list of commands that will be run after to taking a screenshot.",USERCOLOR_GROUP);
}
//NewB End

PLUGIN_API VOID InitializePlugin(VOID)
{
	DebugSpewAlways("Initializing MQ2ScreenShot");
	//NewB Start
	//Add Screenshot command
	AddCommand("/SS",			ScreenShot);
	//Help me!
	AddCommand("/SSHelp",		SSHelp);
	//List commands that are executed
	AddCommand("/SSList",		ListCMDS); 
	//Add or command to the pre screenshot batch
	AddCommand("/SSPRE",		PreCMD);
	AddCommand("/SSREMPRE",		RemPreCMD);
	//Add or command to the post screenshot batch
	AddCommand("/SSPOST",		PostCMD);
	AddCommand("/SSREMPOST",	RemPostCMD);
	//Add default commands to run when screenshot is called
	strcpy(preSSCmds[0],		"/caption MQCaptions off");
	strcpy(preSSCmds[1],		"/keypress NETSTAT");
	strcpy(preSSCmds[2],		"/keypress FULLSCREEN");
	strcpy(postSSCmds[0],		"/caption MQCaptions on");
	strcpy(postSSCmds[1],		"/keypress NETSTAT");
	strcpy(postSSCmds[2],		"/keypress FULLSCREEN");
	//NewB End
}

PLUGIN_API VOID ShutdownPlugin(VOID)
{
	DebugSpewAlways("Shutting down MQ2ScreenShot");
	RemoveCommand("/MQ2SS");
	RemoveCommand("/MQ2SSList");
}

PLUGIN_API VOID OnZoned(VOID)
{
	DebugSpewAlways("MQ2ScreenShot::OnZoned()");
}

PLUGIN_API VOID OnCleanUI(VOID)
{
	DebugSpewAlways("MQ2ScreenShot::OnCleanUI()");
}

PLUGIN_API VOID OnReloadUI(VOID)
{
	DebugSpewAlways("MQ2ScreenShot::OnReloadUI()");
}
PLUGIN_API VOID OnDrawHUD(VOID)
{
}

PLUGIN_API VOID SetGameState(DWORD GameState)
{
	DebugSpewAlways("MQ2ScreenShot::SetGameState()");
}


PLUGIN_API VOID OnPulse(VOID)
{
}

PLUGIN_API DWORD OnWriteChatColor(PCHAR Line, DWORD Color, DWORD Filter)
{
	DebugSpewAlways("MQ2ScreenShot::OnWriteChatColor(%s)",Line);
	return 0;
}

PLUGIN_API DWORD OnIncomingChat(PCHAR Line, DWORD Color)
{
	DebugSpewAlways("MQ2ScreenShot::OnIncomingChat(%s)",Line);
	return 0;
}

PLUGIN_API VOID OnAddSpawn(PSPAWNINFO pNewSpawn)
{
	DebugSpewAlways("MQ2ScreenShot::OnAddSpawn(%s)",pNewSpawn->Name);
}

PLUGIN_API VOID OnRemoveSpawn(PSPAWNINFO pSpawn)
{
	DebugSpewAlways("MQ2ScreenShot::OnRemoveSpawn(%s)",pSpawn->Name);
}

PLUGIN_API VOID OnAddGroundItem(PGROUNDITEM pNewGroundItem)
{
	DebugSpewAlways("MQ2ScreenShot::OnAddGroundItem(%d)",pNewGroundItem->DropID);
}

PLUGIN_API VOID OnRemoveGroundItem(PGROUNDITEM pGroundItem)
{
	DebugSpewAlways("MQ2ScreenShot::OnRemoveGroundItem(%d)",pGroundItem->DropID);
}
