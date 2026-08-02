// MQ2Pop.cpp : a simple, barebones plugin by notadruid.  All it does
// is spam the MQ2 Chat window when an NPC pops, and if the NPC is
// named it will spam NAMED instead of pop
// Version 3

#include "../MQ2Plugin.h"

PreSetup("MQ2Pop");

bool AreWeZoning = false;
time_t seconds;

bool NamedStatus(int NewSpawnID)
{
   SEARCHSPAWN MySearchSpawn = {0};
   ClearSearchSpawn(&MySearchSpawn);
   MySearchSpawn.SpawnID = NewSpawnID;
   MySearchSpawn.bNamed = TRUE;
   if (CountMatchingSpawns(&MySearchSpawn, GetCharInfo()->pSpawn))
      return true;
   else
      return false;
}

PLUGIN_API VOID OnBeginZone(VOID) { AreWeZoning = true; }

PLUGIN_API VOID OnEndZone(VOID) {
   AreWeZoning = false;
   seconds = time (NULL);
}

PLUGIN_API VOID InitializePlugin(VOID) { DebugSpewAlways("Initializing MQ2Pop"); seconds = time (NULL); }
PLUGIN_API VOID ShutdownPlugin(VOID) { DebugSpewAlways("Shutting down MQ2Pop"); }

PLUGIN_API VOID OnAddSpawn(PSPAWNINFO pNewSpawn) {
   DebugSpewAlways("MQ2Pop::OnAddSpawn()");
   if ( AreWeZoning || time(NULL) < seconds + 3 || pNewSpawn->Type != SPAWN_NPC || pNewSpawn->MasterID || strstr(pNewSpawn->Name, "s_Mount") )
      return;
   if(NamedStatus(pNewSpawn->pSpawn->SpawnID)) {
      WriteChatf("\arNAMED\ax > %s < \arNAMED\ax",pNewSpawn->DisplayedName);
   } else {
      WriteChatf("Pop > %s < Pop",pNewSpawn->DisplayedName);
   }
}