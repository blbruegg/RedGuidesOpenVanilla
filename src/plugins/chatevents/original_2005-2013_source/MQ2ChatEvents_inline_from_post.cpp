/* 
MQ2ChatEvents.cpp
2006-08-05	Created by Persnickety/BustedPretext
			Credit to Kuv for his MQ2CustomSound plugin on which some of this was based
2013-05-07	Updated to allow multiple commands to execute on a match			
2013-05-15	Updated to allow in-game variables to be used in Match strings and Commands
2013-05-24	Moved commands into a command queue for asyncronous execution from the chat matching

-----------------
Usage: /ce <Sound|Popup|MissedChat|MissedChatEcho|Commands|verbose|on|off|reload|help>
Sound			: Toggles playing of custom sounds
Popup			: Toggles displaying of custom popups
MissedChat		: Toggles popup notification after missing chat while zoning
MissedChatEcho	: Toggles echoing of missed chat to MQ window
Commands		: Toggles executing custom commands
On				: Turns plugin On
Off				: Turns plugin Off
Reload			: Reload INI file
Help            : Displays the in game commands and show the current settings

Usage /playsound <EventKey>
	Plays sound file associated with given key
*/

#include "../MQ2Plugin.h"
#include <mmsystem.h>
#include <vector>
#include <queue>

#pragma comment(lib,"winmm.lib")

void ChatEventsHelp(void);
DWORD TextToColor(char []);
void ToggleOption( char [], bool * );
string GetOnOffLabel( bool );
void HandleChat( PCHAR, DWORD );
void AddEvents( char [] ) ;

#define MAX_KEYLINES 100		// not max keys, but max number of EventKey# lines, each capable of containing multiple keys
#define MATCHABLE_STRINGS 100
#define MATCHABLE_COLORS 10
#define MAX_COMMANDS 100
#define SKIP_PULSES 5

typedef struct {
	char key[MAX_STRING];
	char matchStrings[MATCHABLE_STRINGS][MAX_STRING];
	char matchColors[MATCHABLE_COLORS][MAX_STRING];
	char soundFile[MAX_STRING];
	char popupText[MAX_STRING];
	char popupColor[MAX_STRING];
	char popupDuration[MAX_STRING];
	char command[MAX_COMMANDS][MAX_STRING];
	int	 commandCount;
	int  matchCount;
	int	 matchColorCount;
} ChatEvent;
vector<ChatEvent> eventVector;

char commandBuffer[MAX_STRING];
queue<string> commandQueue;

char CESection[MAX_STRING]="MQ2ChatEvents";
bool popupsEnabled = true;
bool soundsEnabled = true;
bool missedChatPopup = true;
bool missedChatEcho = true;
bool commandsEnabled = true;
bool verboseCommands = false;
bool pluginEnabled = true;
bool tellFlag = false;
int  pulseCounter = 0;
bool processFlag = true; 

PreSetup("MQ2ChatEvents");

// InitEvents - loads custom events and match strings from INI file
void InitEvents()
{
	//DebugSpewAlways("MQ2ChatEvents - Enter InitEvents");
	gbInZone = true;
	CHAR tempSetting[MAX_STRING];	
	eventVector.clear();
	
	// return if character not yet in game
	if(strcmp(CESection, "MQ2ChatEvents") == 0 ) return ;	

	sprintf(CESection,"%s_%s",GetCharInfo()->Name,EQADDR_SERVERNAME);

	GetPrivateProfileString(CESection,"PluginEnabled","MQ2ChatEvents_Error",tempSetting,MAX_STRING,INIFileName);
	//WriteChatColor(tempSetting);
	if(strcmp( tempSetting, "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection,"PluginEnabled","TRUE",INIFileName);
		sprintf(tempSetting,"TRUE");
	}
	if(!strnicmp(tempSetting,"true",4) || !strnicmp(tempSetting,"on",2) || !strnicmp(tempSetting,"1",1)) pluginEnabled = true;
	else pluginEnabled = false;
	if( !pluginEnabled ) return;

	GetPrivateProfileString(CESection,"PopupsEnabled","MQ2ChatEvents_Error",tempSetting,MAX_STRING,INIFileName);
	if(strcmp( tempSetting, "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection,"PopupsEnabled","TRUE",INIFileName);
		sprintf(tempSetting,"TRUE");
	}
	if(!strnicmp(tempSetting,"true",4) || !strnicmp(tempSetting,"on",2) || !strnicmp(tempSetting,"1",1)) popupsEnabled = true;
	else popupsEnabled = false;
	
	GetPrivateProfileString(CESection,"SoundsEnabled","MQ2ChatEvents_Error",tempSetting,MAX_STRING,INIFileName);
	if(strcmp( tempSetting, "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection,"SoundsEnabled","TRUE",INIFileName);
		sprintf(tempSetting,"TRUE");
	}
	if(!strnicmp(tempSetting,"true",4) || !strnicmp(tempSetting,"on",2) || !strnicmp(tempSetting,"1",1)) soundsEnabled = true;
	else soundsEnabled = false;
	
	GetPrivateProfileString(CESection,"MissedChatEcho","MQ2ChatEvents_Error",tempSetting,MAX_STRING,INIFileName);
	if(strcmp( tempSetting, "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection,"MissedChatEcho","FALSE",INIFileName);
		sprintf(tempSetting,"FALSE");
	}
	if(!strnicmp(tempSetting,"true",4) || !strnicmp(tempSetting,"on",2) || !strnicmp(tempSetting,"1",1)) missedChatEcho = true;
	else missedChatEcho = false;
	
	GetPrivateProfileString(CESection,"MissedChatPopup","MQ2ChatEvents_Error",tempSetting,MAX_STRING,INIFileName);
	if(strcmp( tempSetting, "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection,"MissedChatPopup","FALSE",INIFileName);
		sprintf(tempSetting,"FALSE");
	}
	if(!strnicmp(tempSetting,"true",4) || !strnicmp(tempSetting,"on",2) || !strnicmp(tempSetting,"1",1)) missedChatPopup = true;
	else missedChatPopup = false;
	
	GetPrivateProfileString(CESection,"CommandsEnabled","MQ2ChatEvents_Error",tempSetting,MAX_STRING,INIFileName);
	if(strcmp( tempSetting, "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection,"CommandsEnabled","TRUE",INIFileName);
		sprintf(tempSetting,"TRUE");
	}
	if(!strnicmp(tempSetting,"true",4) || !strnicmp(tempSetting,"on",2) || !strnicmp(tempSetting,"1",1)) commandsEnabled = true;
	else commandsEnabled = false;
	
	GetPrivateProfileString(CESection,"VerboseCommands","MQ2ChatEvents_Error",tempSetting,MAX_STRING,INIFileName);
	if(strcmp( tempSetting, "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection,"VerboseCommands","FALSE",INIFileName);
		sprintf(tempSetting,"FALSE");
	}
	if(!strnicmp(tempSetting,"true",4) || !strnicmp(tempSetting,"on",2) || !strnicmp(tempSetting,"1",1)) verboseCommands = true;
	else verboseCommands = false;

	CHAR szAllKeys[MAX_KEYLINES][MAX_STRING] = {0};
	
	// Get event keys : EventKeys=
	GetPrivateProfileString(CESection,"EventKeys","MQ2ChatEvents_Error",szAllKeys[0],MAX_STRING,INIFileName);
	if(strcmp( szAllKeys[0], "MQ2ChatEvents_Error") == 0) {
		WritePrivateProfileString(CESection,"EventKeys","test123|",INIFileName);
	}
	AddEvents( szAllKeys[0] );

	// Add event keys : EventKeys#=
	char tmpEventKeysLine[MAX_STRING];
	char tmpEventKeysKey[MAX_STRING];
	for( int i=0; i<MAX_KEYLINES; i++ ) {			
		sprintf( tmpEventKeysKey, "EventKeys%d", i );
		GetPrivateProfileString(CESection,tmpEventKeysKey,"MQ2ChatEvents_Error",tmpEventKeysLine,MAX_STRING,INIFileName);		    
		//DebugSpewAlways( "\tReading INI: [%s]=%s", tmpEventKeysKey, tmpEventKeysLine );
		if (strcmp( tmpEventKeysLine, "MQ2ChatEvents_Error" ) == 0 ) continue;
		strcpy(szAllKeys[i], tmpEventKeysLine);
		//DebugSpewAlways("\tAdding Valid EventKeys String: [%s]=%s", tmpEventKeysKey, tmpEventKeysLine );		
	}
	
	for( int keyIndex=0; keyIndex<MAX_KEYLINES; keyIndex++ ) {			
		AddEvents( szAllKeys[keyIndex] );
	}

	// Validate events
	/*
	for( unsigned int eventIndex = 0; eventIndex < eventVector.size(); eventIndex++ ) {
		DebugSpewAlways("Event: %s", eventVector[eventIndex].key );
		for( int matchStringIndex = 0; matchStringIndex < eventVector[eventIndex].matchCount; matchStringIndex++ ){
			DebugSpewAlways("[%s]MatchString%d=%s", eventVector[eventIndex].key, matchStringIndex, eventVector[eventIndex].matchStrings[matchStringIndex] );
		}
		//for( int matchColorIndex = 0; matchColorIndex < eventVector[eventIndex].matchColorCount; matchColorIndex++ ){					
		//	DebugSpewAlways("[%s]MatchColor%d=%d", eventVector[eventIndex].key, matchColorIndex, eventVector[eventIndex].matchColors[matchColorIndex] );
		//}
		for( int cmdIndex = 0; cmdIndex < eventVector[eventIndex].commandCount; cmdIndex++ ){
			DebugSpewAlways("[%s]Command%d=%s", eventVector[eventIndex].key, cmdIndex, eventVector[eventIndex].command[cmdIndex] );
		}
		DebugSpewAlways("[%s]SoundFile=%s", eventVector[eventIndex].key, eventVector[eventIndex].soundFile );
		DebugSpewAlways("[%s]PopupText=%s", eventVector[eventIndex].key, eventVector[eventIndex].popupText );
		//DebugSpewAlways("[%s]PopupColor=%d", eventVector[eventIndex].key, eventVector[eventIndex].popupColor );
		DebugSpewAlways("[%s]PopupDuration=%d", eventVector[eventIndex].key, eventVector[eventIndex].popupDuration );
		DebugSpewAlways("[%s]CommandCount=%d", eventVector[eventIndex].key, eventVector[eventIndex].commandCount );
		DebugSpewAlways("[%s]MatchCount=%d", eventVector[eventIndex].key, eventVector[eventIndex].matchCount );
		DebugSpewAlways("[%s]matchColorCount=%d", eventVector[eventIndex].key, eventVector[eventIndex].matchColorCount );
	}
	*/
}

void AddEvents( char KeyLine[MAX_STRING] ) 
{
	string strAllKeys( KeyLine );	
	string::size_type pos = strAllKeys.find("|",0);
	while (pos != string::npos )
	{		
		if (strlen(strAllKeys.substr(0,pos).c_str()) == 0 ) {
			// Erase key from AllKeys variable (so we process the next one on next loop)
			strAllKeys.erase(0, pos + 1);
			pos = strAllKeys.find("|");
			continue;
		}

		//Create new event structure
		ChatEvent *newEvent = new ChatEvent;		
		//Add event name to events structure
		sprintf(newEvent->key, "%s", strAllKeys.substr(0,pos).c_str() );

		// Get event match strings from INI
		char tmpMatchString[MAX_STRING];
		char tmpMatchStringKey[MAX_STRING];
		newEvent->matchCount = 0;		
		for( int i=0; i<MATCHABLE_STRINGS; i++ ) {			
			sprintf( tmpMatchStringKey, "MatchString%d", i );
			GetPrivateProfileString(newEvent->key,tmpMatchStringKey,"MQ2ChatEvents_Error",tmpMatchString,MAX_STRING,INIFileName);		    
			//DebugSpewAlways( "\tReading INI: [%s]matchString[%d]=%s", newEvent->key, i, tmpMatchString );
			if (strcmp( tmpMatchString, "MQ2ChatEvents_Error" ) == 0 ) continue;
			strcpy(newEvent->matchStrings[newEvent->matchCount], tmpMatchString);
			//DebugSpewAlways("\tAdding Valid Match String: [%d] = %s", newEvent->matchCount, newEvent->matchStrings[newEvent->matchCount] );
			newEvent->matchCount++;				
		}
		
		// Get matchColors from INI
		char tmpMatchColor[MAX_STRING];
		char tmpMatchColorKey[MAX_STRING];		
		newEvent->matchColorCount = 0;
		for( int i=0; i<MATCHABLE_COLORS; i++ ) {			
			sprintf( tmpMatchColorKey, "MatchColor%d", i );
			GetPrivateProfileString(newEvent->key,tmpMatchColorKey,"MQ2ChatEvents_Error",tmpMatchColor,MAX_STRING,INIFileName);		    
			//DebugSpewAlways( "\tReading INI: [%s]matchColor[%d]=%s", newEvent->key, i, tmpMatchColor );
			if (strcmp( tmpMatchColor, "MQ2ChatEvents_Error" ) == 0 ) continue;
			strcpy(newEvent->matchColors[newEvent->matchColorCount], tmpMatchColor);
			//DebugSpewAlways("\tAdding Valid Match Color: [%d] = %s", newEvent->matchColorCount, newEvent->matchColors[newEvent->matchColorCount] );
			newEvent->matchColorCount++;				
		}
		
		// Get sound file from INI
		GetPrivateProfileString(newEvent->key,"SoundFile","MQ2ChatEvents_Error",newEvent->soundFile ,MAX_STRING,INIFileName);
		if (strlen(newEvent->soundFile) == 0) strcpy(newEvent->soundFile, "MQ2ChatEvents_Error");
		
		// Get popup message from INI
		GetPrivateProfileString(newEvent->key,"PopupText","MQ2ChatEvents_Error",newEvent->popupText,MAX_STRING,INIFileName);
		if (strlen(newEvent->popupText) == 0) strcpy(newEvent->popupText, "MQ2ChatEvents_Error");
		
		// Get popup text color from INI
		CHAR szPopupColor[MAX_STRING] = {0};
		GetPrivateProfileString(newEvent->key,"PopupColor", "CONCOLOR_YELLOW",newEvent->popupColor,MAX_STRING,INIFileName);
		if (strlen(newEvent->popupColor) == 0) strcpy(newEvent->popupColor, "CONCOLOR_YELLOW");
		
		// Get popup display duration from INI
		GetPrivateProfileString(newEvent->key,"PopupDuration","3000",newEvent->popupDuration,MAX_STRING,INIFileName);
		if (strlen(newEvent->popupDuration) == 0 ) strcpy(newEvent->popupDuration, "3000");
		
		// Get commands from INI
		char tmpCommandKey[MAX_STRING];
		char tmpCommandString[MAX_STRING];
		newEvent->commandCount = 0;	// Used as both a count and an index for commands found		
		for( int i = 0; i < MAX_COMMANDS; i++ ) {			
			sprintf( tmpCommandKey, "Command%d", i );
			GetPrivateProfileString(newEvent->key,tmpCommandKey,"MQ2ChatEvents_Error",tmpCommandString,MAX_STRING,INIFileName);
			//DebugSpewAlways( "\tReading INI: [%s]command[%d]=%s", newEvent->key, i, tmpCommandString );
			if (strcmp(tmpCommandString, "MQ2ChatEvents_Error" ) == 0 ) continue;
			strcpy( newEvent->command[newEvent->commandCount], tmpCommandString );
			//DebugSpewAlways("\tAdding Valid Command: [%d] = %s", newEvent->commandCount, newEvent->command[newEvent->commandCount] );
			newEvent->commandCount++;			
		}
		
		// Add the event to the vector 
		eventVector.push_back ( *newEvent );
		delete newEvent;

		// Erase current key from AllKeys variable (so we process the next one on next loop)
		strAllKeys.erase(0, pos + 1);
		pos = strAllKeys.find("|");		
	}
}
// Cycle through events looking for a string (and color) match
// Returns the index of the event where the match is found
// Returns -1 if no match found
int FindMatch( PCHAR Line, DWORD Color ){
	string strLine ( Line );
	string::size_type pos;	  
	bool skip = false;
	bool matchFound = false;
	bool NoColorCheck = false;
	char expandedString[MAX_STRING] = {0};

	//DebugSpewAlways("Entering FindMatch: %s", Line );
	if( Color == NULL ) NoColorCheck = true ;

	for( unsigned int eventIndex = 0; eventIndex < eventVector.size(); eventIndex++ )
	{
		//for each MatchString# entry in the event	
		for( int matchStringIndex = 0; matchStringIndex < eventVector[eventIndex].matchCount; matchStringIndex++ ){
			//DebugSpewAlways("\t\tMQ2ChatEvents Inside for matchStringIndex = 0; %d < eventVector[eventIndex].matchCount (%d); matchStringIndex++ ", matchStringIndex, eventVector[eventIndex].matchCount);
			//DebugSpewAlways("Comparing strings: %s  <==>  %s", Line, eventVector[eventIndex].matchStrings[matchStringIndex]);
			
			strcpy( expandedString, eventVector[eventIndex].matchStrings[matchStringIndex] );
			// Expand any variables in the match string			
			ParseMacroData(expandedString);
			
			pos = strLine.find(expandedString,0);
			if ( pos != string::npos ){
				//DebugSpewAlways("\tFound Match, checking color");				
				//string match found, scan for matched colors if specified
				
				//DebugSpewAlways("Color is null: %s",(NoColorCheck)?"true":"false");
				// If no color check, successful match
				if ( NoColorCheck ) return eventIndex;
				
				//If no colors specified, successful match for all colors
				if ( eventVector[eventIndex].matchColorCount == 0 ) return eventIndex;  
				
				// for each MatchColor# entry in the event
				for( int matchColorIndex = 0; matchColorIndex < eventVector[eventIndex].matchColorCount; matchColorIndex++ ){					
					//DebugSpewAlways("\t\t\tMQ2ChatEvents Inside for matchColorIndex = 0; %d < eventVector[eventIndex].matchColorCount (%d); matchColorIndex++", matchColorIndex, eventVector[eventIndex].matchColorCount);
					//DebugSpewAlways("\tComparing colors: Index %d ", matchColorIndex );
					if( Color == TextToColor(eventVector[eventIndex].matchColors[matchColorIndex])){
						// Color specified and found, match successful
						//DebugSpewAlways("\tColor specified and found, match successful");
						return eventIndex;						
					}	
				}				
			}			
		}
	}
	// No match found
	//DebugSpewAlways("Leaving FindMatch, no match found for %s", Line);
	return -1;
}

void ProcessPopups( PCHAR Line, unsigned int i )
{
	DWORD popupColor = 0;
	DWORD popupTransparency = 100;
	DWORD popupFadeIn = 500;
	DWORD popupFadeOut = 500;
	DWORD popupHold = 3000;
	string::size_type pos;
	char popupText[MAX_STRING];

	if( popupsEnabled && strcmp( eventVector[i].popupText, "MQ2ChatEvents_Error") != 0 ){
		popupColor = TextToColor(eventVector[i].popupColor ) ;
		popupHold = (DWORD) atoi( eventVector[i].popupDuration ); //convert string to int, cast as dword

		// Replace placeholder #FULLTEXT# with the entire line that was matched
		string popupString (eventVector[i].popupText);
		pos = popupString.find("#FULLTEXT#");		
		if ( pos != string::npos ) { 
			//found a match on FULLTEXT
			popupString.replace(pos,strlen("#FULLTEXT#"),Line);
		} 
		sprintf(popupText, "%s", popupString.c_str() );

		DisplayOverlayText(popupText, popupColor, popupTransparency, popupFadeIn, popupFadeOut,popupHold);   
	}
}

void ProcessSounds( unsigned int i )
{
	if( soundsEnabled && strcmp( eventVector[i].soundFile, "MQ2ChatEvents_Error") != 0 ){
		PlaySound(eventVector[i].soundFile,NULL,SND_FILENAME | SND_ASYNC);        
	}
}

void ProcessCommands( PCHAR Line, unsigned int eventIndex, unsigned int commandIndex )
{
	CHAR commandText[MAX_STRING] ;
	string::size_type pos;
	string strLine ( Line );	
		
	//DebugSpewAlways("Entering ProcessCommands = Line:%s |eventIndex:%d |commandIndex:%d |processFlag:%s |command:%s", Line, eventIndex, commandIndex, (processFlag)?"true":"false", eventVector[eventIndex].command[commandIndex] );
	//DebugSpewAlways("\tCommand %d : %s", commandIndex, eventVector[eventIndex].command[commandIndex] );
	if ( !commandsEnabled ) return ;
	
	strcpy(commandText, eventVector[eventIndex].command[commandIndex]);

	// Replace placeholder #FULLTEXT# with the entire line that was matched
	string commandString (commandText);
	pos = commandString.find("#FULLTEXT#");		
	if ( pos != string::npos ) { 
		//found a match on FULLTEXT
		commandString.replace(pos,strlen("#FULLTEXT#"),strLine);
	} 
	sprintf(commandText, "%s", commandString.c_str() );
		
	if( strcmp( commandText, "MQ2ChatEvents_Error") != 0 ){
		commandQueue.push( commandText );		
	}
	//DebugSpewAlways("Leaving ProcessCommands = Line:%s |eventIndex:%d |commandIndex:%d |processFlag:%s |command:%s", Line, eventIndex, commandIndex, (processFlag)?"true":"false", eventVector[eventIndex].command[commandIndex] );
	return;
}

PLUGIN_API DWORD OnWriteChatColor(PCHAR Line, DWORD Color, DWORD Filter)
{
	//DebugSpewAlways("Entering OnWriteChatColor: Line:%s |processFlag:%s", Line, (processFlag)?"true":"false" );
	if( processFlag && pluginEnabled) HandleChat( Line, Color );
	return 0;
}
PLUGIN_API DWORD OnIncomingChat(PCHAR Line, DWORD Color)
{
	//DebugSpewAlways("Entering OnIncomingChat: Line:%s |processFlag:%s", Line, (processFlag)?"true":"false" );
	if( processFlag && pluginEnabled) HandleChat( Line, Color );
	return 0;
}

void CheckMissedChat(PCHAR Line, DWORD Color){
	// Check if you received any tells or group chat while zoning
	string strLine ( Line );
	string::size_type pos;	  

	//DebugSpewAlways("gbInZone: %s  --- missedChatEcho: %s --- %s", (gbInZone)?"true":"false", (missedChatEcho)?"true":"false", Line );
	if (!gbInZone && missedChatEcho ) {		
		pos = strLine.find(" tells ",0);
		if ( pos != string::npos ) {
			processFlag = false;
			DebugSpewAlways("Missed chat: %s", Line);
			WriteChatColor( Line,Color);
			processFlag = true;
			tellFlag = true;
		}
		pos = strLine.find(" told ",0);
		if ( pos != string::npos ) {
			processFlag = false;
			DebugSpewAlways("Missed chat: %s", Line);
			WriteChatColor( Line,Color);
			processFlag = true;
			tellFlag = true;			
		}		
	} 

	if(tellFlag && missedChatPopup) {
		DisplayOverlayText(	"You missed some chat while zoning!  Check MQ window.", 
							CONCOLOR_YELLOW, 100, 500, 500, 10000);
		tellFlag = false;		
	}
}

void HandleChat(PCHAR Line, DWORD Color){
	/* ****************************************************************************************
	Be very careful writing anything to the EQ or MQ chat windows inside this function!
	It can create an infinite recursive loop by re-calling HandleChat for any line that is written!
	**************************************************************************************** */
	//DebugSpewAlways("MQ2ChatEvents entering HandleChat (proccessFlag=%s) = %s", (processFlag)?"true":"false", Line);   
	int eventIndex = -1;	// Index of the event vector where the match was found
	
	if( !pluginEnabled || !processFlag ) return;	

	CheckMissedChat( Line, Color );
	// Cycle through events looking for a match.  If match is found, that event's index is returned
	eventIndex = FindMatch( Line, Color );
	if ( eventIndex < 0 || eventIndex >= eventVector.size()  ) return ;
	//DebugSpewAlways("\tHandleChat Match Found [%s]: Index=%d", eventVector[eventIndex].key, matchFound );

	// Found a matching chat line.  Now process all defined events for that match
	ProcessPopups( Line, eventIndex );
	ProcessSounds( eventIndex );

	for( int cmdIndex = 0; cmdIndex < eventVector[eventIndex].commandCount; cmdIndex++ ){
		//DebugSpewAlways("\t\tMQ2ChatEvents Inside for cmdIndex = 0; %d < eventVector[eventIndex].commandCount %d; cmdIndex++", cmdIndex, eventVector[eventIndex].commandCount );
		//DebugSpewAlways("\t\tLine:%s |eventIndex:%d |cmdIndex:%d", Line, eventIndex, cmdIndex);
		ProcessCommands( Line, eventIndex, cmdIndex );		
	}

	//DebugSpewAlways("MQ2ChatEvents exiting HandleChat");
   return;
} 

// Handle commands passed through /ce
void ChatEventsCmd(PSPAWNINFO pChar, PCHAR szLine)
{
	char Arg[MAX_STRING];
	CHAR tempString[MAX_STRING];

	GetArg(Arg,szLine,1);
	
	if( !strlen(Arg) ||
		!strnicmp(Arg,"help",strlen(Arg)) ||
		!strnicmp(Arg,"?",strlen(Arg)) ){
		ChatEventsHelp();
		return;
	}
	if (!strnicmp(Arg,"sound",strlen(Arg)) ||
		!strnicmp(Arg,"sounds",strlen(Arg)) ||
		!strnicmp(Arg,"togglesound",strlen(Arg)) ||
		!strnicmp(Arg,"togglesounds",strlen(Arg))  ){
		ToggleOption( "SoundsEnabled", &soundsEnabled );	
		return;
	}
	if (!strnicmp(Arg,"popup",strlen(Arg)) ||
		!strnicmp(Arg,"popups",strlen(Arg)) ||
		!strnicmp(Arg,"togglepopup",strlen(Arg)) ||
		!strnicmp(Arg,"togglepopups",strlen(Arg))  ){
		ToggleOption( "PopupsEnabled", &popupsEnabled );	
		return;
	}
	if (!strnicmp(Arg,"missedchat",strlen(Arg)) ||
		!strnicmp(Arg,"missedchatecho",strlen(Arg)) ||
		!strnicmp(Arg,"togglemissedchat",strlen(Arg)) ||
		!strnicmp(Arg,"togglemissedchatecho",strlen(Arg))  ){
		ToggleOption( "MissedChatEcho", &missedChatEcho );	
		return;
	}
	if (!strnicmp(Arg,"missedchatpopup",strlen(Arg)) ||
		!strnicmp(Arg,"togglemissedchatpopup",strlen(Arg)) ){
		ToggleOption( "MissedChatPopup", &missedChatPopup );
		return;
	}
	if (!strnicmp(Arg,"command",strlen(Arg)) ||
		!strnicmp(Arg,"commands",strlen(Arg)) ||
		!strnicmp(Arg,"togglecommand",strlen(Arg)) ||
		!strnicmp(Arg,"togglecommands",strlen(Arg))  ){
		ToggleOption( "CommandsEnabled", &commandsEnabled );
		return;
	}
	if (!strnicmp(Arg,"verbose",strlen(Arg)) ||
		!strnicmp(Arg,"verbosecommands",strlen(Arg)) ||
		!strnicmp(Arg,"toggleverbose",strlen(Arg)) ||
		!strnicmp(Arg,"togglerverbosecommands",strlen(Arg))  ){
		ToggleOption( "VerboseCommands", &verboseCommands );
		return;
	}
	if (!strnicmp(Arg,"on",strlen(Arg)) ){
		WritePrivateProfileString(CESection,"PluginEnabled","TRUE",INIFileName);	
		pluginEnabled = true;
		string enabledTxt = GetOnOffLabel(pluginEnabled);
		sprintf(tempString,"ChatEvents: Plugin = %s", enabledTxt.c_str());
		WriteChatColor(tempString);
		InitEvents();
		return;
	}
	if (!strnicmp(Arg,"off",strlen(Arg))  ){
		WritePrivateProfileString(CESection,"PluginEnabled","FALSE",INIFileName);	
		pluginEnabled = false;
		string enabledTxt = GetOnOffLabel(pluginEnabled);
		sprintf(tempString,"ChatEvents: Plugin = %s", enabledTxt.c_str());
		WriteChatColor(tempString);	
		return;
	}
	if (!strnicmp(Arg,"reload",strlen(Arg)) ||
		!strnicmp(Arg,"reloadini",strlen(Arg)) ){
		InitEvents();
		WriteChatColor("ChatEvents INI reloaded");
		return;
	}
	else {
		ChatEventsHelp();
		return;
	}

}
void ToggleOption( char optionName[MAX_STRING], bool *optionVal )
{   
	CHAR tempString[MAX_STRING];
	
	*optionVal = !(*optionVal);
	if(*optionVal) sprintf(tempString,"TRUE");
	else sprintf(tempString, "FALSE");
	WritePrivateProfileString(CESection,optionName,tempString,INIFileName);
   
	string enabledTxt = GetOnOffLabel(*optionVal);
	sprintf(tempString,"ChatEvents: %s = %s", optionName, enabledTxt.c_str());
	WriteChatColor(tempString);

	InitEvents();
}

void ChatEventsHelp()
{
	char szTemp[MAX_STRING];
	string tempStr;

	WriteChatColor(" --------------------------------", CONCOLOR_LIGHTBLUE);
	WriteChatColor("MQ2ChatEvents    ", CONCOLOR_LIGHTBLUE);	
	WriteChatColor(" --------------------------------", CONCOLOR_LIGHTBLUE);
	tempStr = GetOnOffLabel(pluginEnabled);
	sprintf(szTemp,"[%s] Plugin", tempStr.c_str() );
	WriteChatColor(szTemp);
	tempStr = GetOnOffLabel(popupsEnabled);
	sprintf(szTemp,"[%s] Popups", tempStr.c_str() );
	WriteChatColor(szTemp);
	tempStr = GetOnOffLabel(soundsEnabled);
	sprintf(szTemp,"[%s] Sounds", tempStr.c_str() );
	WriteChatColor(szTemp);	
	tempStr = GetOnOffLabel(missedChatPopup);
	sprintf(szTemp,"[%s] MissedChatPopup", tempStr.c_str() );
	WriteChatColor(szTemp);
	tempStr = GetOnOffLabel(missedChatEcho);
	sprintf(szTemp,"[%s] MissedChatEcho", tempStr.c_str() );
	WriteChatColor(szTemp);
	tempStr = GetOnOffLabel(commandsEnabled);
	sprintf(szTemp,"[%s] Commands", tempStr.c_str() );
	WriteChatColor(szTemp);
	tempStr = GetOnOffLabel(verboseCommands);
	sprintf(szTemp,"[%s] Verbose Commands", tempStr.c_str() );
	WriteChatColor(szTemp);	
	WriteChatColor(" --------------------------------", CONCOLOR_LIGHTBLUE);
	SyntaxError("Usage: /ce <Sound|Popup|MissedChat|MissedChatEcho|Commands|verbose|on|off|reload>" );	
}

string GetOnOffLabel( bool val )
{
   string s;

   if( val ) s = "\agON\ax";
   else s = "\auOFF\ax";

   return s;
}


DWORD TextToColor( char colorText[MAX_STRING] )
{	
		if( !strcmp(colorText,  "COLOR_DEFAULT" ))                  return COLOR_DEFAULT;
		if( !strcmp(colorText,  "COLOR_DARKGREY" ))                 return COLOR_DARKGREY;
		if( !strcmp(colorText,  "COLOR_DARKGREEN" ))                return COLOR_DARKGREEN;
		if( !strcmp(colorText,  "COLOR_DARKBLUE"	))				return COLOR_DARKBLUE;
		if( !strcmp(colorText,  "COLOR_PURPLE" ))                   return COLOR_PURPLE;
		if( !strcmp(colorText,  "COLOR_LIGHTGREY" ))                return COLOR_LIGHTGREY;

		if( !strcmp(colorText,  "CONCOLOR_GREEN" ))                 return CONCOLOR_GREEN;
		if( !strcmp(colorText,  "CONCOLOR_LIGHTBLUE" ))             return CONCOLOR_LIGHTBLUE;
		if( !strcmp(colorText,  "CONCOLOR_BLUE" ))                  return CONCOLOR_BLUE;
		if( !strcmp(colorText,  "CONCOLOR_BLACK" ))                 return CONCOLOR_BLACK;
		if( !strcmp(colorText,  "CONCOLOR_YELLOW" ))                return CONCOLOR_YELLOW;
		if( !strcmp(colorText,  "CONCOLOR_RED" ))                   return CONCOLOR_RED;

		if( !strcmp(colorText,  "USERCOLOR_SAY" ))                  return USERCOLOR_SAY;					//  1  - Say
		if( !strcmp(colorText,  "USERCOLOR_TELL" ))                 return USERCOLOR_TELL;					//  2  - Tell
		if( !strcmp(colorText,  "USERCOLOR_GROUP" ))                return USERCOLOR_GROUP;					//  3  - Group
		if( !strcmp(colorText,  "USERCOLOR_GUILD" ))                return USERCOLOR_GUILD;					//  4  - Guild
		if( !strcmp(colorText,  "USERCOLOR_OOC" ))                  return USERCOLOR_OOC;					//  5  - OOC
		if( !strcmp(colorText,  "USERCOLOR_AUCTION" ))              return USERCOLOR_AUCTION;				//  6  - Auction
		if( !strcmp(colorText,  "USERCOLOR_SHOUT" ))                return USERCOLOR_SHOUT;					//  7  - Shout
		if( !strcmp(colorText,  "USERCOLOR_EMOTE" ))                return USERCOLOR_EMOTE;					//  8  - Emote
		if( !strcmp(colorText,  "USERCOLOR_SPELLS" ))               return USERCOLOR_SPELLS;				//  9  - Spells (meming, scribing, casting, etc.)
		if( !strcmp(colorText,  "USERCOLOR_YOU_HIT_OTHER" ))        return USERCOLOR_YOU_HIT_OTHER;			//  10 - You hit other
		if( !strcmp(colorText,  "USERCOLOR_OTHER_HIT_YOU" ))        return USERCOLOR_OTHER_HIT_YOU;			//  11 - Other hits you
		if( !strcmp(colorText,  "USERCOLOR_YOU_MISS_OTHER" ))       return USERCOLOR_YOU_MISS_OTHER;		//  12 - You miss other
		if( !strcmp(colorText,  "USERCOLOR_OTHER_MISS_YOU" ))       return USERCOLOR_OTHER_MISS_YOU;		//  13 - Other misses you
		if( !strcmp(colorText,  "USERCOLOR_DUELS" ))                return USERCOLOR_DUELS;					//  14 - Some broadcasts (duels)
		if( !strcmp(colorText,  "USERCOLOR_SKILLS" ))               return USERCOLOR_SKILLS;				//  15 - Skills (ups, non-combat use, etc.)
		if( !strcmp(colorText,  "USERCOLOR_DISCIPLINES" ))          return USERCOLOR_DISCIPLINES;			//  16 - Disciplines or special abilities
		if( !strcmp(colorText,  "USERCOLOR_UNUSED001" ))            return USERCOLOR_UNUSED001;				//  17 - Unused at this time
		if( !strcmp(colorText,  "USERCOLOR_DEFAULT" ))              return USERCOLOR_DEFAULT;				//  18 - Default text and stuff you type
		if( !strcmp(colorText,  "USERCOLOR_UNUSED002" ))            return USERCOLOR_UNUSED002;				//  19 - Unused at this time
		if( !strcmp(colorText,  "USERCOLOR_MERCHANT_OFFER" ))       return USERCOLOR_MERCHANT_OFFER;		//  20 - Merchant Offer Price
		if( !strcmp(colorText,  "USERCOLOR_MERCHANT_EXCHANGE" ))    return USERCOLOR_MERCHANT_EXCHANGE;		//  21 - Merchant Buy/Sell
		if( !strcmp(colorText,  "USERCOLOR_YOUR_DEATH" ))           return USERCOLOR_YOUR_DEATH;			//  22 - Your death message
		if( !strcmp(colorText,  "USERCOLOR_OTHER_DEATH" ))          return USERCOLOR_OTHER_DEATH;			//  23 - Others death message
		if( !strcmp(colorText,  "USERCOLOR_OTHER_HIT_OTHER" ))      return USERCOLOR_OTHER_HIT_OTHER;		//  24 - Other damage other
		if( !strcmp(colorText,  "USERCOLOR_OTHER_MISS_OTHER" ))     return USERCOLOR_OTHER_MISS_OTHER;		//  25 - Other miss other
		if( !strcmp(colorText,  "USERCOLOR_WHO" ))                  return USERCOLOR_WHO;					//  26 - /who command
		if( !strcmp(colorText,  "USERCOLOR_YELL" ))                 return USERCOLOR_YELL;					//  27 - yell for help
		if( !strcmp(colorText,  "USERCOLOR_NON_MELEE" ))            return USERCOLOR_NON_MELEE;				//  28 - Hit for non-melee
		if( !strcmp(colorText,  "USERCOLOR_SPELL_WORN_OFF" ))       return USERCOLOR_SPELL_WORN_OFF;		//  29 - Spell worn off
		if( !strcmp(colorText,  "USERCOLOR_MONEY_SPLIT" ))          return USERCOLOR_MONEY_SPLIT;			//  30 - Money splits
		if( !strcmp(colorText,  "USERCOLOR_LOOT" ))                 return USERCOLOR_LOOT;					//  31 - Loot message
		if( !strcmp(colorText,  "USERCOLOR_RANDOM" ))               return USERCOLOR_RANDOM;				//  32 - Dice Roll (/random)
		if( !strcmp(colorText,  "USERCOLOR_OTHERS_SPELLS" ))        return USERCOLOR_OTHERS_SPELLS;			//  33 - Others spells
		if( !strcmp(colorText,  "USERCOLOR_SPELL_FAILURE" ))        return USERCOLOR_SPELL_FAILURE;			//  34 - Spell Failures (resists, fizzles, missing component, bad target, etc.)
		if( !strcmp(colorText,  "USERCOLOR_CHAT_CHANNEL" ))         return USERCOLOR_CHAT_CHANNEL;			//  35 - Chat Channel Messages
		if( !strcmp(colorText,  "USERCOLOR_CHAT_1" ))               return USERCOLOR_CHAT_1;;				//  36 - Chat Channel 1
		if( !strcmp(colorText,  "USERCOLOR_CHAT_2" ))               return USERCOLOR_CHAT_2;				//  37 - Chat Channel 2
		if( !strcmp(colorText,  "USERCOLOR_CHAT_3" ))               return USERCOLOR_CHAT_3;				//  38 - Chat Channel 3
		if( !strcmp(colorText,  "USERCOLOR_CHAT_4" ))               return USERCOLOR_CHAT_4;				//  39 - Chat Channel 4
		if( !strcmp(colorText,  "USERCOLOR_CHAT_5" ))               return USERCOLOR_CHAT_5;				//  40 - Chat Channel 5
		if( !strcmp(colorText,  "USERCOLOR_CHAT_6" ))               return USERCOLOR_CHAT_6;				//  41 - Chat Channel 6
		if( !strcmp(colorText,  "USERCOLOR_CHAT_7" ))               return USERCOLOR_CHAT_7;				//  42 - Chat Channel 7
		if( !strcmp(colorText,  "USERCOLOR_CHAT_8" ))               return USERCOLOR_CHAT_8;				//  43 - Chat Channel 8
		if( !strcmp(colorText,  "USERCOLOR_CHAT_9" ))               return USERCOLOR_CHAT_9;				//  44 - Chat Channel 9
		if( !strcmp(colorText,  "USERCOLOR_CHAT_10" ))              return USERCOLOR_CHAT_10;				//  45 - Chat Channel 10
		if( !strcmp(colorText,  "USERCOLOR_MELEE_CRIT" ))           return USERCOLOR_MELEE_CRIT;			//  46 - Melee Crits
		if( !strcmp(colorText,  "USERCOLOR_SPELL_CRIT" ))           return USERCOLOR_SPELL_CRIT;			//  47 - Spell Crits
		if( !strcmp(colorText,  "USERCOLOR_TOO_FAR_AWAY" ))         return USERCOLOR_TOO_FAR_AWAY;			//  48 - Too far away (melee)
		if( !strcmp(colorText,  "USERCOLOR_NPC_RAMPAGE" ))          return USERCOLOR_NPC_RAMPAGE;			//  49 - NPC Rampage
		if( !strcmp(colorText,  "USERCOLOR_NPC_FLURRY" ))           return USERCOLOR_NPC_FLURRY;			//  50 - NPC Furry
		if( !strcmp(colorText,  "USERCOLOR_NPC_ENRAGE" ))           return USERCOLOR_NPC_ENRAGE;			//  51 - NPC Enrage
		if( !strcmp(colorText,  "USERCOLOR_ECHO_SAY" ))             return USERCOLOR_ECHO_SAY;				//  52 - say echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_TELL" ))            return USERCOLOR_ECHO_TELL;				//  53 - tell echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_GROUP" ))           return USERCOLOR_ECHO_GROUP;			//  54 - group echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_GUILD" ))           return USERCOLOR_ECHO_GUILD;			//  55 - guild echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_OOC" ))             return USERCOLOR_ECHO_OOC;				//  56 - group echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_AUCTION" ))         return USERCOLOR_ECHO_AUCTION;			//  57 - auction echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_SHOUT" ))           return USERCOLOR_ECHO_SHOUT;			//  58 - shout echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_EMOTE" ))           return USERCOLOR_ECHO_EMOTE;			//  59 - emote echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_CHAT_1" ))          return USERCOLOR_ECHO_CHAT_1;			//  60 - chat 1 echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_CHAT_2" ))          return USERCOLOR_ECHO_CHAT_2;			//  61 - chat 2 echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_CHAT_3" ))          return USERCOLOR_ECHO_CHAT_3;			//  62 - chat 3 echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_CHAT_4" ))          return USERCOLOR_ECHO_CHAT_4;			//  63 - chat 4 echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_CHAT_5" ))          return USERCOLOR_ECHO_CHAT_5;			//  64 - chat 5 echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_CHAT_6" ))          return USERCOLOR_ECHO_CHAT_6;			//  65 - chat 6 echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_CHAT_7" ))          return USERCOLOR_ECHO_CHAT_7;			//  66 - chat 7 echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_CHAT_8" ))          return USERCOLOR_ECHO_CHAT_8;			//  67 - chat 8 echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_CHAT_9" ))          return USERCOLOR_ECHO_CHAT_9;			//  68 - chat 9 echo
		if( !strcmp(colorText,  "USERCOLOR_ECHO_CHAT_10" ))         return USERCOLOR_ECHO_CHAT_10;			//  69 - chat 10 echo
		if( !strcmp(colorText,  "USERCOLOR_RESERVED" ))             return USERCOLOR_RESERVED;				//  70 - "unused at this time" ))
		if( !strcmp(colorText,  "USERCOLOR_LINK" ))                 return USERCOLOR_LINK;					//  71 - item links 
		if( !strcmp(colorText,  "USERCOLOR_RAID" ))                 return USERCOLOR_RAID;					//  72 - raid 
		if( !strcmp(colorText,  "USERCOLOR_PET" ))                  return USERCOLOR_PET;					//  73 - my pet 
		if( !strcmp(colorText,  "USERCOLOR_DAMAGESHIELD" ))         return USERCOLOR_DAMAGESHIELD;			//  74 - damage shields 
		if( !strcmp(colorText,  "USERCOLOR_LEADER" ))               return USERCOLOR_LEADER;				//  75 - LAA-related messages 
		if( !strcmp(colorText,  "USERCOLOR_PETRAMPFLURRY" ))        return USERCOLOR_PETRAMPFLURRY;			//  76 - pet rampage/flurry 
		if( !strcmp(colorText,  "USERCOLOR_PETCRITS" ))             return USERCOLOR_PETCRITS;				//  77 - pet's critical hits 
		if( !strcmp(colorText,  "USERCOLOR_FOCUS" ))                return USERCOLOR_FOCUS;					//  78 - focus item activation 
		if( !strcmp(colorText,  "USERCOLOR_XP" ))                   return USERCOLOR_XP;					//  79 - xp gain/loss 
		if( !strcmp(colorText,  "USERCOLOR_SYSTEM" ))               return USERCOLOR_SYSTEM;				//  80 - system broadcasts etc 

}
// PlaySound - plays sound events when using the /playsound command
// Inspired by the MQ2PlaySound plug-in by Digitalxero
void PlaySoundCmd(PSPAWNINFO pChar, PCHAR szLine)
{
	CHAR Arg[MAX_STRING] = {0};
	CHAR szSoundFile[MAX_STRING] = {0};
	bool matchFound = false;
	unsigned int i = 0;

	// Get sound file from INI
	GetArg(Arg,szLine,1);
	for( i = 0; i < eventVector.size(); i++ ){
		if( !strnicmp( Arg, eventVector[i].key, strlen(Arg)) ){
			matchFound = true;
			break;
		}
		else matchFound = false;
	}
	
	GetPrivateProfileString(eventVector[i].key,"SoundFile","MQ2ChatEvents_Error",szSoundFile,MAX_STRING,INIFileName);
	if ( !stricmp( szSoundFile, "MQ2ChatEvents_Error" )) return;
	if ( !stricmp( Arg, "stop" ) ) {
		PlaySound(NULL,NULL,SND_ASYNC);
	} else {
		PlaySound(eventVector[i].soundFile,NULL,SND_FILENAME | SND_ASYNC);      
	}
}

PLUGIN_API VOID SetGameState(DWORD GameState)
{
	if (GameState==GAMESTATE_INGAME)
        sprintf(CESection,"%s_%s",GetCharInfo()->Name,EQADDR_SERVERNAME);
    else 
        strcpy(CESection,"MQ2ChatEvents");   

	InitEvents();
}

PLUGIN_API VOID OnPulse(VOID)
{   
	/* 
	pulseCounter++;
	if ( pulseCounter <= SKIP_PULSES ) {
		//DebugSpewAlways("\tpulseCounter <= SKIP_PULSES, returning : %d <= %d", pulseCounter, SKIP_PULSES);	
		return ;
	}
	else {
		pulseCounter = 0 ;
	}
	*/
	
	// Execute one command per pulse
	// If there are commands in the queue, turn off chat line processing until the queue is empty again 
	if ( !commandQueue.empty() ) {
		processFlag = false;
		char commandText[MAX_STRING];
		// Expand any variables in the command string			
		sprintf(commandText, "%s", commandQueue.front().c_str() );
		ParseMacroData(commandText);
		
		if(verboseCommands) {
			char verboseMessage[MAX_STRING];
			sprintf(verboseMessage, "MQ2ChatEvents::Executing command=\ag%s\ax", commandText );
			WriteChatColor( verboseMessage );
		}
		//DebugSpewAlways("Executing command: %s", commandQueue.front().c_str() );
		//DebugSpewAlways("Executing command: %s", commandText );
		DoCommand(((PCHARINFO)pCharData)->pSpawn, commandText );					
		commandQueue.pop();		
	}
	else {
		processFlag = true;
	}
	return;
}

PLUGIN_API VOID InitializePlugin(VOID)
{
   DebugSpewAlways("Initializing MQ2ChatEvents");
   AddCommand("/chatevents",ChatEventsCmd);
   AddCommand("/ce", ChatEventsCmd);
   AddCommand("/playsound", PlaySoundCmd);
   InitEvents();
}

PLUGIN_API VOID ShutdownPlugin(VOID)
{
   DebugSpewAlways("Shutting down MQ2ChatEvents");
   RemoveCommand("/chatevents");
   RemoveCommand("/ce");
   RemoveCommand("/playsound");
}