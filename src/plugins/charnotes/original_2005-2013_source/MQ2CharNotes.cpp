// MQ2CharNotes.cpp : Add Character / NPC Comments, Viewable in Chat and HUD
//
// version 0.9 (by Psycotic)
//
//////////////////////////////////////////////////////////////////////////////
// Usage:
//   This plugin lets you add a PC or NPC comment to a character.  When
//   active, It scan any new spawns, and all spawns upon zoning in, for
//   any comments you have saved in MQCharNotes.ini.  It will display these
//   comments in the MQChat window upon spawn or zone if anyone zones in
//   or is currently in zone when you enter.  It will display the current
//   targeted player or NPC comment in the HUD.
// 
//
// Commands:
//  /charnote <comment>   - Currently targeted PC or NPC will have the comment 
//                          assigned to them, and be displayed as requested
//                          If the target is a corpse, it will assign the 
//                          comment to the live  PC or NPC it belongs to.
//                          If the comment already exists, it will update it.
//
//  /charnote -delete     - Will delete the comment from the currently targeted
//                          PC or NPC.  Currently will only 'clear' the comment
//                          and not actually remove the name from the INI file.
//
//  /charnotepos <x> <y>  - Assigns the position of the comment in the HUD
//
//
//  Examples -
//
//  (With "a rabbit" currently targeted)
//   /charnote This guy's foot isn't so lucky anymore!
//    - Would add the comment "This guy's foot isn't so lucky anymore!"
//      to any "a rabbit" NPC's in zone.
//
//  (With "Psycotic" currently targeted)
//   /charnote This guy rocks!
//    - Would add the comment "This guy rocks!" to PC Psycotic
//
//   /charnote -delete
//    - Would clear the comment from Psycotic, leaving Psycotic's INI entry
//
//   /charnotepos 200 500
//    - Would change the current HUD location of the comment to x200 y500
//
//
// INI File Config Settings - Current possible settings
//
// CharNotesPosX=200       - Current HUD X-Axis Location
// CharNotesPosY=170       - Current HUD Y-Axis Location
// ShowInChat=on           - Toggle Showing Comments in Chat
// ShowInHUD=on            - Toggle Showing Comments on HUD
// MaxNotes=200            - Maximum Number of Possible Comments
//
//
// Known Issues/ToDo:
//  - Currently, there's no way to remove a comment once it's been added
//    without editing the INI file and reordering the numbers.  It will
//    clear the comment, but will not remove the target from the INI file.
//
//  - The code could use a bit of optimization and commenting
//
//  - Add code to automatically create the INI file if it doesn't exist 
//    with default settings.
//
//  - Add the ability to show the comments in a pop-up, movable window
//
//////////////////////////////////////////////////////////////////////////////



#include "../MQ2Plugin.h"

PreSetup("MQ2CharNotes");

#define CHAT 0
#define HUD 1
char szTemp[MAX_STRING];
bool ShowInChat;
bool ShowInHUD;
int MAX_NOTES;
int CharNotesX;
int CharNotesY;
int CurNote=0;

struct CharNoteList 
{
    char Name[35];
    char Note[90];
}; 

CharNoteList CharNote[MAX_STRING];
PSPAWNINFO psTarget;


VOID ShowHelp(VOID)
{
    WriteChatColor("CharNotes - Usage", CONCOLOR_YELLOW);
    WriteChatColor("'/charnote This note will be displayed for the target'", CONCOLOR_YELLOW);
    WriteChatColor("'/charnote -delete'      (Will 'clear' the comment for current target)", CONCOLOR_YELLOW);
    WriteChatColor("'/charnotepos <x> <y>'   (X,Y display location on HUD)", CONCOLOR_YELLOW);
    return;
}

VOID ClearArray(VOID)
{
CurNote=0;
    for (int i = 1; i <= MAX_NOTES; i++)
    {
        strcpy(CharNote[i].Name, "");
        strcpy(CharNote[i].Note, "");
    }
}

VOID SaveINIArray(VOID)
{
char NameArray[MAX_STRING];
char NoteArray[MAX_STRING];

    for (int i = 1;  i <= CurNote; i++)
    {
        sprintf(NameArray, "Name%i",i);
        sprintf(NoteArray, "Note%i",i);
        WritePrivateProfileString("SPAWNS",NameArray,CharNote[i].Name,INIFileName);
        WritePrivateProfileString("SPAWNS",NoteArray,CharNote[i].Note,INIFileName);
    }    
}

VOID LoadINIArray(VOID)
{
char NameArray[MAX_STRING];
char NoteArray[MAX_STRING];
char Name[MAX_STRING];
char Note[MAX_STRING];

    ClearArray();
    for (int i = 1; i <= MAX_NOTES; i++)
    {
        sprintf(NameArray, "Name%i",i);
        sprintf(NoteArray, "Note%i",i);
        GetPrivateProfileString("SPAWNS",NameArray,NULL,Name,MAX_STRING,INIFileName);
        GetPrivateProfileString("SPAWNS",NoteArray,NULL,Note,MAX_STRING,INIFileName);
        if (strcmp(Name,""))
        {
            strcpy(CharNote[i].Name,Name);
            strcpy(CharNote[i].Note,Note);
            CurNote=i;
        }
        else
        {
            return;
        }

    }
}

VOID LoadConfig(VOID)
{
    GetPrivateProfileString("Config","ShowInChat","on",szTemp,MAX_STRING,INIFileName);
    if (!strcmp(szTemp, "on"))
        ShowInChat = true;
    else
        ShowInChat = false;
    GetPrivateProfileString("Config","ShowInHUD","on",szTemp,MAX_STRING,INIFileName);
    if (!strcmp(szTemp, "on"))
        ShowInHUD = true;
    else
        ShowInHUD = false;
    GetPrivateProfileString("Config","CharNotesPosX",NULL,szTemp,MAX_STRING,INIFileName);
    CharNotesX = atoi(szTemp);
    GetPrivateProfileString("Config","CharNotesPosY",NULL,szTemp,MAX_STRING,INIFileName);
    CharNotesY = atoi(szTemp);
    GetPrivateProfileString("Config","MaxNotes",NULL,szTemp,MAX_STRING,INIFileName);
    MAX_NOTES = atoi(szTemp);
    
}

VOID DisplayCharNote(BOOL chat, CHAR szLine[MAX_STRING])
{
    for (int i = 1; i <= MAX_NOTES; i++)
    {
        if ((!strcmp(szLine, CharNote[i].Name)) && (strcmp(CharNote[i].Note, "")))
        {
            sprintf(szTemp,"%s - %s", CharNote[i].Name, CharNote[i].Note);
            if (!chat)
            {
                WriteChatColor(szTemp, CONCOLOR_YELLOW);
            } 
            else
            {
                DrawHUDText(szTemp, CharNotesX,CharNotesY,0xFFFFFFFF);
            }
        }
    }
}

VOID DoCharNotePos(PSPAWNINFO pChar, PCHAR szLine)
{
char szArg1[MAX_STRING]={0};
char szArg2[MAX_STRING]={0};

GetArg(szArg1,szLine,1); 
GetArg(szArg2,szLine,2); 

    if (strlen(szArg2))
    {
        WritePrivateProfileString("Config","CharNotesPosX",szArg1,INIFileName);
        WritePrivateProfileString("Config","CharNotesPosY",szArg2,INIFileName);
        CharNotesX = atoi(szArg1);
        CharNotesY = atoi(szArg2);
    }
    else
    {
        WriteChatColor("You must enter Xpos and Ypos in the format \"/charnotepos 500 200\"", CONCOLOR_YELLOW);
    }
}


VOID DoCharNote(PSPAWNINFO pChar, PCHAR szLine)
{
char CharType[MAX_STRING]={0};
char CharName[MAX_STRING]={0};
char NameArray[MAX_STRING];
char NoteArray[MAX_STRING];

    if (CurNote >= MAX_NOTES)
    {
        sprintf(szTemp, "The maximum number of notes (%i) have been added.  Please increase MAX_NOTES to increase", MAX_NOTES);
        WriteChatColor(szTemp);
        return;
    }
    if (( pTarget && ppTarget )  && (strlen(szLine)))
    {
        psTarget = (PSPAWNINFO)pTarget;
        for (int i = 1; i <= CurNote; i++)
        {
            if (!strcmp(CharNote[i].Name, psTarget->DisplayedName))
            {
                if (!strcmp(szLine, "-delete"))
                {
                    strcpy(CharNote[i].Note,"");
                    WritePrivateProfileString("SPAWNS",NoteArray,CharNote[i].Note,INIFileName);
                    return;
                }

                strcpy(CharNote[i].Note,szLine);
                WritePrivateProfileString("SPAWNS",NoteArray,CharNote[i].Note,INIFileName);
                return;
            }
        }
        CurNote++;
        sprintf(NameArray, "Name%i",CurNote);
        sprintf(NoteArray, "Note%i",CurNote);
        strcpy(CharNote[CurNote].Name,psTarget->DisplayedName);
        strcpy(CharNote[CurNote].Note,szLine);
        WritePrivateProfileString("SPAWNS",NameArray,CharNote[CurNote].Name,INIFileName);
        WritePrivateProfileString("SPAWNS",NoteArray,CharNote[CurNote].Note,INIFileName);
        DisplayCharNote(CHAT, CharNote[CurNote].Name);
    }
    else
    {
        ShowHelp();
    }

}


PLUGIN_API VOID InitializePlugin(VOID)
{
    DebugSpewAlways("Initializing MQ2CharNotes");
    AddCommand("/charnote",DoCharNote);
    AddCommand("/charnotepos",DoCharNotePos);
    LoadConfig();
    LoadINIArray();
}

PLUGIN_API VOID ShutdownPlugin(VOID)
{
    DebugSpewAlways("Shutting down MQ2CharNotes");
    RemoveCommand("/charnote");
    RemoveCommand("/charnotepos");
    SaveINIArray();
}

PLUGIN_API VOID OnDrawHUD(VOID)
{
    if (ShowInHUD)
    {
        if (pTarget && ppTarget) 
        {
            psTarget = (PSPAWNINFO)pTarget;
//            if (psTarget->Type != SPAWN_CORPSE)
            DisplayCharNote(HUD, psTarget->DisplayedName);
        }
    }
}


PLUGIN_API VOID OnAddSpawn(PSPAWNINFO pNewSpawn)
{
    if (ShowInChat)
    {
        if (pNewSpawn->Type != SPAWN_CORPSE)
        DisplayCharNote(CHAT, pNewSpawn->DisplayedName);
    }
}

PLUGIN_API VOID OnZoned(VOID)
{
    SaveINIArray();
}

PLUGIN_API VOID SetGameState(DWORD GameState)
{
    SaveINIArray();
}
