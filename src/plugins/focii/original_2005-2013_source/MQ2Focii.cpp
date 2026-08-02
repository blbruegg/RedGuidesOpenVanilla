// MQ2Focii.cpp : Defines the entry point for the DLL application. 
// 
// Modified by Dragonbutt on September 6, 2009 to version 1.32 
// Added Mods from Augs for Skill Damage, Consolidated mod and hero display format 
// Modified by Dragonbutt on August 21, 2009 to version 1.31 
// Added Backstab, Bash, Flying Kick, Frenzy, and Kick 
// Modified by Dragonbutt on August 13, 2009, Thanks Gnatch for Attack Rating stuff 
// Added Clairvoyance, SpellDamage, HealAmount and Atk 
// PLUGIN_API is only to be used for callbacks.  All existing callbacks at this time 
// are shown below. Remove the ones your plugin does not use.  Always use Initialize 
// and Shutdown for setup and cleanup, do NOT do it in DllMain. 

#include "../MQ2Plugin.h" 
#include <list> 

PreSetup("MQ2Focii"); 

#define MQ2Focii_Version   "MQ2Focii \ar1.32\ax by \agDragonButt\ax Original by \agDrunkDwarf\ax" 
#define MQ2FociiTitle      "ShowFocii" 

BOOL InGame(); 
VOID Handler(PSPAWNINFO pChar, PCHAR szLine); 
VOID ShowFocii(PSPAWNINFO pChar, PCHAR szLine); 
static void ChatTell(PSPAWNINFO pChar, char *cLine); 
VOID Usage(); 

int Attack = 0; 
int Avoidance = 0; 
int Accuracy = 0; 
int Spellshield = 0; 
int Shielding = 0; 
int StunResist = 0; 
int StrikeThrough = 0; 
int CombatEffects = 0; 
int DoTShielding = 0; 
int HeroicSTR = 0; 
int HeroicINT = 0; 
int HeroicWIS = 0; 
int HeroicAGI = 0; 
int HeroicDEX = 0; 
int HeroicSTA = 0; 
int HeroicCHA = 0; 
int Clairvoyance = 0; 
int SpellDamage = 0; 
int HealAmount = 0; 
int KickDmg = 0; 
int BackstabDmg = 0; 
int Frenzy = 0; 
int FlyKick = 0; 
int Bash = 0; 

// Called once, when the plugin is to initialize 
PLUGIN_API VOID InitializePlugin(VOID) 
{ 
   DebugSpewAlways("Initializing MQ2Focii"); 

   // Add commands, MQ2Data items, hooks, etc. 
   AddCommand("/showfocii", Handler); 
   // AddXMLFile("MQUI_MyXMLFile.xml"); 
   // bmMyBenchmark=AddMQ2Benchmark("My Benchmark Name"); 
} 

// Called once, when the plugin is to shutdown 
PLUGIN_API VOID ShutdownPlugin(VOID) 
{ 
   DebugSpewAlways("Shutting down MQ2Focii"); 

   // Remove commands, MQ2Data items, hooks, etc. 
   // RemoveMQ2Benchmark(bmMyBenchmark); 
   RemoveCommand("/showfocii"); 
   // RemoveXMLFile("MQUI_MyXMLFile.xml"); 
} 

// Called every frame that the "HUD" is drawn -- e.g. net status / packet loss bar 
PLUGIN_API VOID OnDrawHUD(VOID) 
{ 
   if(FALSE == InGame()) 
      return; 
} 

// return true if im ingame and have access to all information. 
BOOL InGame() 
{ 
   return(!MQ2Globals::gZoning && MQ2Globals::gGameState == GAMESTATE_INGAME && GetCharInfo2() && GetCharInfo() && GetCharInfo()->pSpawn); 
} 

VOID Handler(PSPAWNINFO pChar, PCHAR szLine) 
{ 
   ShowFocii(pChar, szLine); 
} 

VOID ShowFocii(PSPAWNINFO pChar, PCHAR szLine) 
{ 
   Attack =0; 
   Avoidance = 0; 
   Accuracy = 0; 
   Spellshield = 0; 
   Shielding = 0; 
   StunResist = 0; 
   StrikeThrough = 0; 
   CombatEffects = 0; 
   DoTShielding = 0; 
   HeroicSTR = 0; 
   HeroicINT = 0; 
   HeroicWIS = 0; 
   HeroicAGI = 0; 
   HeroicDEX = 0; 
   HeroicSTA = 0; 
   HeroicCHA = 0; 
   Clairvoyance = 0; 
   SpellDamage = 0; 
   HealAmount = 0; 
   KickDmg = 0; 
   BackstabDmg = 0; 
   Frenzy = 0; 
   FlyKick = 0; 
   Bash = 0; 

//   PITEMINFO Item, int nInBank; 

//   DWORD ID = Item->ItemNumber; 
//   DWORD AugType = Item->AugType; 

//   int nHowMany = 0; 
//   nInBank = 0; 

   list<string>   Spells; 

   // inventory 
   for(int iSlot=0; iSlot<23;iSlot++) 
   { 
      if(PCONTENTS cSlot=GetCharInfo2()->InventoryArray[iSlot]) 
      { 
        Attack += cSlot->Item->Attack; 
         Avoidance += cSlot->Item->Avoidance; 
         Accuracy += cSlot->Item->Accuracy; 
         Spellshield += cSlot->Item->SpellShield; 
         Shielding += cSlot->Item->Shielding; 
         StunResist += cSlot->Item->StunResist; 
         StrikeThrough += cSlot->Item->StrikeThrough; 
         CombatEffects += cSlot->Item->CombatEffects; 
         DoTShielding += cSlot->Item->DoTShielding; 
         Clairvoyance += cSlot->Item->Clairvoyance; 
         SpellDamage += cSlot->Item->SpellDamage; 
         HealAmount += cSlot->Item->HealAmount; 
         HeroicSTR += cSlot->Item->HeroicSTR; 
         HeroicINT += cSlot->Item->HeroicINT; 
         HeroicWIS += cSlot->Item->HeroicWIS; 
         HeroicAGI += cSlot->Item->HeroicAGI; 
         HeroicDEX += cSlot->Item->HeroicDEX; 
         HeroicSTA += cSlot->Item->HeroicSTA; 
         HeroicCHA += cSlot->Item->HeroicCHA; 

       if(szSkills[cSlot->Item->DmgBonusSkill] && cSlot->Item->DmgBonusValue) 
       { 
         if(!stricmp(szSkills[cSlot->Item->DmgBonusSkill], "backstab")) 
         {            
            BackstabDmg += cSlot->Item->DmgBonusValue; 
         } 
           if(!stricmp(szSkills[cSlot->Item->DmgBonusSkill], "bash")) 
         { 
            Bash += cSlot->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Item->DmgBonusSkill], "flying kick")) 
         { 
            FlyKick += cSlot->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Item->DmgBonusSkill], "frenzy")) 
         { 
            Frenzy += cSlot->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Item->DmgBonusSkill], "kick")) 
         { 
            KickDmg += cSlot->Item->DmgBonusValue; 
         } 
       } 

       
         if(cSlot->Item->Worn.SpellID != 0 && cSlot->Item->Worn.SpellID != 0xFFFFFFFF) 
         { 
            string text = GetSpellNameByID(cSlot->Item->Worn.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
            Spells.push_back(text); 
         } 
         if(cSlot->Item->Focus.SpellID != 0 && cSlot->Item->Focus.SpellID != 0xFFFFFFFF) 
         { 
            string text = GetSpellNameByID(cSlot->Item->Focus.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
            Spells.push_back(text); 
         } 
         if(cSlot->Item->Proc.SpellID != 0 && cSlot->Item->Proc.SpellID!= 0xFFFFFFFF) 
         { 
            string text = GetSpellNameByID(cSlot->Item->Proc.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
            Spells.push_back(text); 
         } 

//         if(cSlot->Item->AugType) 
         { 
            if (cSlot->Item->AugSlot1 && cSlot->Contents[0]) 
            { 
            Attack += cSlot->Contents[0]->Item->Attack; 
               Avoidance += cSlot->Contents[0]->Item->Avoidance; 
               Accuracy += cSlot->Contents[0]->Item->Accuracy; 
               Spellshield += cSlot->Contents[0]->Item->SpellShield; 
               Shielding += cSlot->Contents[0]->Item->Shielding; 
               StunResist += cSlot->Contents[0]->Item->StunResist; 
               StrikeThrough += cSlot->Contents[0]->Item->StrikeThrough; 
               CombatEffects += cSlot->Contents[0]->Item->CombatEffects; 
               DoTShielding += cSlot->Contents[0]->Item->DoTShielding; 
               Clairvoyance += cSlot->Contents[0]->Item->Clairvoyance; 
               SpellDamage += cSlot->Contents[0]->Item->SpellDamage; 
               HealAmount += cSlot->Contents[0]->Item->HealAmount; 
               HeroicSTR += cSlot->Contents[0]->Item->HeroicSTR; 
               HeroicINT += cSlot->Contents[0]->Item->HeroicINT; 
               HeroicWIS += cSlot->Contents[0]->Item->HeroicWIS; 
               HeroicAGI += cSlot->Contents[0]->Item->HeroicAGI; 
               HeroicDEX += cSlot->Contents[0]->Item->HeroicDEX; 
               HeroicSTA += cSlot->Contents[0]->Item->HeroicSTA; 
               HeroicCHA += cSlot->Contents[0]->Item->HeroicCHA; 

       if(szSkills[cSlot->Contents[0]->Item->DmgBonusSkill] && cSlot->Contents[0]->Item->DmgBonusValue) 
       { 
         if(!stricmp(szSkills[cSlot->Contents[0]->Item->DmgBonusSkill], "backstab")) 
         {            
            BackstabDmg += cSlot->Contents[0]->Item->DmgBonusValue; 
         } 
           if(!stricmp(szSkills[cSlot->Contents[0]->Item->DmgBonusSkill], "bash")) 
         { 
            Bash += cSlot->Contents[0]->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Contents[0]->Item->DmgBonusSkill], "flying kick")) 
         { 
            FlyKick += cSlot->Contents[0]->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Contents[0]->Item->DmgBonusSkill], "frenzy")) 
         { 
            Frenzy += cSlot->Contents[0]->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Contents[0]->Item->DmgBonusSkill], "kick")) 
         { 
            KickDmg += cSlot->Contents[0]->Item->DmgBonusValue; 
         } 
       } 

               if(cSlot->Contents[0]->Item->Worn.SpellID != 0 && cSlot->Contents[0]->Item->Worn.SpellID != 0xFFFFFFFF) 
               { 
                  string text = GetSpellNameByID(cSlot->Contents[0]->Item->Worn.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
                  Spells.push_back(text); 
               } 
               if(cSlot->Contents[0]->Item->Focus.SpellID != 0 && cSlot->Contents[0]->Item->Focus.SpellID != 0xFFFFFFFF) 
               { 
                  string text = GetSpellNameByID(cSlot->Contents[0]->Item->Focus.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
                  Spells.push_back(text); 
               } 
               if(cSlot->Contents[0]->Item->Proc.SpellID != 0 && cSlot->Contents[0]->Item->Proc.SpellID!= 0xFFFFFFFF) 
               { 
                  string text = GetSpellNameByID(cSlot->Contents[0]->Item->Proc.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
                  Spells.push_back(text); 
               } 
            } 
            if (cSlot->Item->AugSlot2 && cSlot->Contents[1]) 
            { 
            Attack += cSlot->Contents[1]->Item->Attack; 
               Avoidance += cSlot->Contents[1]->Item->Avoidance; 
               Accuracy += cSlot->Contents[1]->Item->Accuracy; 
               Spellshield += cSlot->Contents[1]->Item->SpellShield; 
               Shielding += cSlot->Contents[1]->Item->Shielding; 
               StunResist += cSlot->Contents[1]->Item->StunResist; 
               StrikeThrough += cSlot->Contents[1]->Item->StrikeThrough; 
               CombatEffects += cSlot->Contents[1]->Item->CombatEffects; 
               DoTShielding += cSlot->Contents[1]->Item->DoTShielding; 
               Clairvoyance += cSlot->Contents[1]->Item->Clairvoyance; 
               SpellDamage += cSlot->Contents[1]->Item->SpellDamage; 
               HealAmount += cSlot->Contents[1]->Item->HealAmount; 
               HeroicSTR += cSlot->Contents[1]->Item->HeroicSTR; 
               HeroicINT += cSlot->Contents[1]->Item->HeroicINT; 
               HeroicWIS += cSlot->Contents[1]->Item->HeroicWIS; 
               HeroicAGI += cSlot->Contents[1]->Item->HeroicAGI; 
               HeroicDEX += cSlot->Contents[1]->Item->HeroicDEX; 
               HeroicSTA += cSlot->Contents[1]->Item->HeroicSTA; 
               HeroicCHA += cSlot->Contents[1]->Item->HeroicCHA; 

       if(szSkills[cSlot->Contents[1]->Item->DmgBonusSkill] && cSlot->Contents[1]->Item->DmgBonusValue) 
       { 
         if(!stricmp(szSkills[cSlot->Contents[1]->Item->DmgBonusSkill], "backstab")) 
         {            
            BackstabDmg += cSlot->Contents[1]->Item->DmgBonusValue; 
         } 
           if(!stricmp(szSkills[cSlot->Contents[1]->Item->DmgBonusSkill], "bash")) 
         { 
            Bash += cSlot->Contents[1]->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Contents[1]->Item->DmgBonusSkill], "flying kick")) 
         { 
            FlyKick += cSlot->Contents[1]->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Contents[1]->Item->DmgBonusSkill], "frenzy")) 
         { 
            Frenzy += cSlot->Contents[1]->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Contents[1]->Item->DmgBonusSkill], "kick")) 
         { 
            KickDmg += cSlot->Contents[1]->Item->DmgBonusValue; 
         } 
       } 

               if(cSlot->Contents[1]->Item->Worn.SpellID != 0 && cSlot->Contents[1]->Item->Worn.SpellID != 0xFFFFFFFF) 
               { 
                  string text = GetSpellNameByID(cSlot->Contents[1]->Item->Worn.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
                  Spells.push_back(text); 
               } 
               if(cSlot->Contents[1]->Item->Focus.SpellID != 0 && cSlot->Contents[1]->Item->Focus.SpellID != 0xFFFFFFFF) 
               { 
                  string text = GetSpellNameByID(cSlot->Contents[1]->Item->Focus.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
                  Spells.push_back(text); 
               } 
               if(cSlot->Contents[1]->Item->Proc.SpellID != 0 && cSlot->Contents[1]->Item->Proc.SpellID!= 0xFFFFFFFF) 
               { 
                  string text = GetSpellNameByID(cSlot->Contents[1]->Item->Proc.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
                  Spells.push_back(text); 
               } 
            } 
            if (cSlot->Item->AugSlot3 && cSlot->Contents[2]) 
            { 
            Attack += cSlot->Contents[2]->Item->Attack; 
               Avoidance += cSlot->Contents[2]->Item->Avoidance; 
               Accuracy += cSlot->Contents[2]->Item->Accuracy; 
               Spellshield += cSlot->Contents[2]->Item->SpellShield; 
               Shielding += cSlot->Contents[2]->Item->Shielding; 
               StunResist += cSlot->Contents[2]->Item->StunResist; 
               StrikeThrough += cSlot->Contents[2]->Item->StrikeThrough; 
               CombatEffects += cSlot->Contents[2]->Item->CombatEffects; 
               DoTShielding += cSlot->Contents[2]->Item->DoTShielding; 
               Clairvoyance += cSlot->Contents[2]->Item->Clairvoyance; 
               SpellDamage += cSlot->Contents[2]->Item->SpellDamage; 
               HealAmount += cSlot->Contents[2]->Item->HealAmount; 
               HeroicSTR += cSlot->Contents[2]->Item->HeroicSTR; 
               HeroicINT += cSlot->Contents[2]->Item->HeroicINT; 
               HeroicWIS += cSlot->Contents[2]->Item->HeroicWIS; 
               HeroicAGI += cSlot->Contents[2]->Item->HeroicAGI; 
               HeroicDEX += cSlot->Contents[2]->Item->HeroicDEX; 
               HeroicSTA += cSlot->Contents[2]->Item->HeroicSTA; 
               HeroicCHA += cSlot->Contents[2]->Item->HeroicCHA; 

       if(szSkills[cSlot->Contents[2]->Item->DmgBonusSkill] && cSlot->Contents[2]->Item->DmgBonusValue) 
       { 
         if(!stricmp(szSkills[cSlot->Contents[2]->Item->DmgBonusSkill], "backstab")) 
         {            
            BackstabDmg += cSlot->Contents[2]->Item->DmgBonusValue; 
         } 
           if(!stricmp(szSkills[cSlot->Contents[2]->Item->DmgBonusSkill], "bash")) 
         { 
            Bash += cSlot->Contents[2]->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Contents[2]->Item->DmgBonusSkill], "flying kick")) 
         { 
            FlyKick += cSlot->Contents[2]->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Contents[2]->Item->DmgBonusSkill], "frenzy")) 
         { 
            Frenzy += cSlot->Contents[2]->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Contents[2]->Item->DmgBonusSkill], "kick")) 
         { 
            KickDmg += cSlot->Contents[2]->Item->DmgBonusValue; 
         } 
       } 

               if(cSlot->Contents[2]->Item->Worn.SpellID != 0 && cSlot->Contents[2]->Item->Worn.SpellID != 0xFFFFFFFF) 
               { 
                  string text = GetSpellNameByID(cSlot->Contents[2]->Item->Worn.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
                  Spells.push_back(text); 
               } 
               if(cSlot->Contents[2]->Item->Focus.SpellID != 0 && cSlot->Contents[2]->Item->Focus.SpellID != 0xFFFFFFFF) 
               { 
                  string text = GetSpellNameByID(cSlot->Contents[2]->Item->Focus.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
                  Spells.push_back(text); 
               } 
               if(cSlot->Contents[2]->Item->Proc.SpellID != 0 && cSlot->Contents[2]->Item->Proc.SpellID!= 0xFFFFFFFF) 
               { 
                  string text = GetSpellNameByID(cSlot->Contents[2]->Item->Proc.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
                  Spells.push_back(text); 
               } 
            } 
            if (cSlot->Item->AugSlot4 && cSlot->Contents[3]) 
            { 
            Attack += cSlot->Contents[3]->Item->Attack; 
               Avoidance += cSlot->Contents[3]->Item->Avoidance; 
               Accuracy += cSlot->Contents[3]->Item->Accuracy; 
               Spellshield += cSlot->Contents[3]->Item->SpellShield; 
               Shielding += cSlot->Contents[3]->Item->Shielding; 
               StunResist += cSlot->Contents[3]->Item->StunResist; 
               StrikeThrough += cSlot->Contents[3]->Item->StrikeThrough; 
               CombatEffects += cSlot->Contents[3]->Item->CombatEffects; 
               DoTShielding += cSlot->Contents[3]->Item->DoTShielding; 
               Clairvoyance += cSlot->Contents[3]->Item->Clairvoyance; 
               SpellDamage += cSlot->Contents[3]->Item->SpellDamage; 
               HealAmount += cSlot->Contents[3]->Item->HealAmount; 
               HeroicSTR += cSlot->Contents[3]->Item->HeroicSTR; 
               HeroicINT += cSlot->Contents[3]->Item->HeroicINT; 
               HeroicWIS += cSlot->Contents[3]->Item->HeroicWIS; 
               HeroicAGI += cSlot->Contents[3]->Item->HeroicAGI; 
               HeroicDEX += cSlot->Contents[3]->Item->HeroicDEX; 
               HeroicSTA += cSlot->Contents[3]->Item->HeroicSTA; 
               HeroicCHA += cSlot->Contents[3]->Item->HeroicCHA; 

       if(szSkills[cSlot->Contents[3]->Item->DmgBonusSkill] && cSlot->Contents[3]->Item->DmgBonusValue) 
       { 
         if(!stricmp(szSkills[cSlot->Contents[3]->Item->DmgBonusSkill], "backstab")) 
         {            
            BackstabDmg += cSlot->Contents[3]->Item->DmgBonusValue; 
         } 
           if(!stricmp(szSkills[cSlot->Contents[3]->Item->DmgBonusSkill], "bash")) 
         { 
            Bash += cSlot->Contents[3]->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Contents[3]->Item->DmgBonusSkill], "flying kick")) 
         { 
            FlyKick += cSlot->Contents[3]->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Contents[3]->Item->DmgBonusSkill], "frenzy")) 
         { 
            Frenzy += cSlot->Contents[3]->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Contents[3]->Item->DmgBonusSkill], "kick")) 
         { 
            KickDmg += cSlot->Contents[3]->Item->DmgBonusValue; 
         } 
       } 

            if(cSlot->Contents[3]->Item->Worn.SpellID != 0 && cSlot->Contents[3]->Item->Worn.SpellID != 0xFFFFFFFF) 
               { 
                  string text = GetSpellNameByID(cSlot->Contents[3]->Item->Worn.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
                  Spells.push_back(text); 
               } 
               if(cSlot->Contents[3]->Item->Focus.SpellID != 0 && cSlot->Contents[3]->Item->Focus.SpellID != 0xFFFFFFFF) 
               { 
                  string text = GetSpellNameByID(cSlot->Contents[3]->Item->Focus.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
                  Spells.push_back(text); 
               } 
               if(cSlot->Contents[3]->Item->Proc.SpellID != 0 && cSlot->Contents[3]->Item->Proc.SpellID!= 0xFFFFFFFF) 
               { 
                  string text = GetSpellNameByID(cSlot->Contents[3]->Item->Proc.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
                  Spells.push_back(text); 
               } 
            } 
            if (cSlot->Item->AugSlot5 && cSlot->Contents[4]) 
            { 
            Attack += cSlot->Contents[4]->Item->Attack; 
               Avoidance += cSlot->Contents[4]->Item->Avoidance; 
               Accuracy += cSlot->Contents[4]->Item->Accuracy; 
               Spellshield += cSlot->Contents[4]->Item->SpellShield; 
               Shielding += cSlot->Contents[4]->Item->Shielding; 
               StunResist += cSlot->Contents[4]->Item->StunResist; 
               StrikeThrough += cSlot->Contents[4]->Item->StrikeThrough; 
               CombatEffects += cSlot->Contents[4]->Item->CombatEffects; 
               DoTShielding += cSlot->Contents[4]->Item->DoTShielding; 
               Clairvoyance += cSlot->Contents[4]->Item->Clairvoyance; 
               SpellDamage += cSlot->Contents[4]->Item->SpellDamage; 
               HealAmount += cSlot->Contents[4]->Item->HealAmount; 
               HeroicSTR += cSlot->Contents[4]->Item->HeroicSTR; 
               HeroicINT += cSlot->Contents[4]->Item->HeroicINT; 
               HeroicWIS += cSlot->Contents[4]->Item->HeroicWIS; 
               HeroicAGI += cSlot->Contents[4]->Item->HeroicAGI; 
               HeroicDEX += cSlot->Contents[4]->Item->HeroicDEX; 
               HeroicSTA += cSlot->Contents[4]->Item->HeroicSTA; 
               HeroicCHA += cSlot->Contents[4]->Item->HeroicCHA; 

       if(szSkills[cSlot->Contents[4]->Item->DmgBonusSkill] && cSlot->Contents[4]->Item->DmgBonusValue) 
       { 
         if(!stricmp(szSkills[cSlot->Contents[4]->Item->DmgBonusSkill], "backstab")) 
         {            
            BackstabDmg += cSlot->Contents[4]->Item->DmgBonusValue; 
         } 
           if(!stricmp(szSkills[cSlot->Contents[4]->Item->DmgBonusSkill], "bash")) 
         { 
            Bash += cSlot->Contents[4]->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Contents[4]->Item->DmgBonusSkill], "flying kick")) 
         { 
            FlyKick += cSlot->Contents[4]->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Contents[4]->Item->DmgBonusSkill], "frenzy")) 
         { 
            Frenzy += cSlot->Contents[4]->Item->DmgBonusValue; 
         } 
         if(!stricmp(szSkills[cSlot->Contents[4]->Item->DmgBonusSkill], "kick")) 
         { 
            KickDmg += cSlot->Contents[4]->Item->DmgBonusValue; 
         } 
       } 

               if(cSlot->Contents[4]->Item->Worn.SpellID != 0 && cSlot->Contents[4]->Item->Worn.SpellID != 0xFFFFFFFF) 
               { 
                  string text = GetSpellNameByID(cSlot->Contents[4]->Item->Worn.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
                  Spells.push_back(text); 
               } 
               if(cSlot->Contents[4]->Item->Focus.SpellID != 0 && cSlot->Contents[4]->Item->Focus.SpellID != 0xFFFFFFFF) 
               { 
                  string text = GetSpellNameByID(cSlot->Contents[4]->Item->Focus.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
                  Spells.push_back(text); 
               } 
               if(cSlot->Contents[4]->Item->Proc.SpellID != 0 && cSlot->Contents[4]->Item->Proc.SpellID!= 0xFFFFFFFF) 
               { 
                  string text = GetSpellNameByID(cSlot->Contents[4]->Item->Proc.SpellID) + string(" (") + szItemSlot[iSlot] + ")"; 
                  Spells.push_back(text); 
               } 
            } 
         } 
      } 
   } 

    ChatTell(pChar, ""); 

   bool bShowMod2 = true; 
   bool bShowHero = true; 
   bool bShowEffects = true; 
   CHAR szArg1[MAX_STRING] = {0}; 
   GetArg(szArg1, szLine, 1); 
   // if there's no arguments, show everything 
   if(strlen(szArg1) != 0) 
   { 
      bShowMod2 = false; 
      bShowHero = false; 
      bShowEffects = false; 
      // lowercase 
      string lcArg1 = szArg1; 
      MakeLower(lcArg1); 

      if(lcArg1 == "mod") 
      { 
         bShowMod2 = true; 
      } 
      else if(lcArg1 == "eff") 
      { 
         bShowEffects = true; 
      } 
      else if(lcArg1 == "hero") 
      { 
         bShowHero = true; 
      } 
      else 
      { 
         Usage(); 

         bShowMod2 = true; 
         bShowHero = true; 
         bShowEffects = true; 
      } 
   } 

   char cTemp[256]; 
   if(true == bShowMod2) 
  { 
      ChatTell(pChar, "-->>FOCII<<--"); 
     int EARank  =0,spent = 0; 
      //getting ranks of Enhanced Aggression 
   if ( PALTABILITY pAbility=pAltAdvManager->GetAltAbility(GetAAIndexByName("Enhanced Aggression")) ) { 
     spent = pAbility->PointsSpent; 
      for(int i=0; i<pAbility->MaxRank ;i++ ){ 
          switch(i){ 
          case 0: case 1: case 2: case 3: case 4: case 5: case 6: case 7: case 8: case 9: //ranks 1-10 
              spent -= 5; 
              break; 
          case 10: case 11: case 12: case 13: //ranks 11-14 
             spent -= (i-3); 
             break; 
         case 14: case 19: //ranks 15 and 20 
             spent -= 12; 
             break; 
         case 15: case 16: case 17: case 18: //ranks 16-19 
             spent -= (i-8); 
             break; 
         default: 
              break; 
         } 
        if(spent <0){ 
             EARank =i; 
            break; 
        } 
       } 
   }          
   sprintf(cTemp, "Attack: %d (%d) | Accuracy: %d (150) | Avoidance: %d (100)", Attack, 250+(10*EARank), Accuracy, Avoidance); 
     ChatTell(pChar, cTemp); 
     sprintf(cTemp, "Shielding: %d (35) | Spellshield: %d (35) | DotShield: %d (35)", Shielding, Spellshield, DoTShielding); 
      ChatTell(pChar, cTemp); 
     sprintf(cTemp, "StunResist: %d (35) | StrikeThrough: %d (35) | CombatEffects: %d (100)", StunResist, StrikeThrough, CombatEffects); 
      ChatTell(pChar, cTemp); 
     sprintf(cTemp, "Clairvoyance: %d | SpellDamage: %d | HealAmount: %d", Clairvoyance, SpellDamage, HealAmount); 
      ChatTell(pChar, cTemp); 
     sprintf(cTemp, "Backstab: %d | Bash: %d | Flying Kick: %d", BackstabDmg, Bash, FlyKick); 
      ChatTell(pChar, cTemp); 
     sprintf(cTemp, "Frenzy: %d | Kick: %d", Frenzy, KickDmg); 
      ChatTell(pChar, cTemp); 
  } 

   if(true == bShowHero) 
   { 
      ChatTell(pChar, "-->>HEROICS<<--"); 
     sprintf(cTemp, "hSTR: %d | hDEX: %d | hAGI: %d | hSTA: %d", HeroicSTR, HeroicDEX, HeroicAGI, HeroicSTA); 
      ChatTell(pChar, cTemp); 
     sprintf(cTemp, "hWIS: %d | hINT: %d | hCHA: %d ", HeroicWIS, HeroicINT, HeroicCHA); 
      ChatTell(pChar, cTemp); 
   } 

   if(true == bShowEffects) 
   { 
      ChatTell(pChar, "-->>EFFECTS<<--"); 
      Spells.sort(); 
      list<string>::iterator pSpells = Spells.begin(); 
      while(pSpells != Spells.end()) 
      { 
         char tmp[256]; 
         strcpy(tmp, pSpells->c_str()); 
         ChatTell(pChar, tmp); 
         pSpells++; 
      } 
   } 
} 

// 
// static void ChatTell(PSPAWNINFO pChar, char *cLine) 
// 
static void ChatTell(PSPAWNINFO pChar, char *cLine) 
{ 
   DebugSpew("MQ2Focii::ChatTell(%s)",cLine); 

   bool bReplyMode = false; 

   char cTemp[1024]; 
   if (!bReplyMode) { 
       sprintf(cTemp, "Focii told you, '%s'", cLine); 
       dsp_chat_no_events(cTemp, USERCOLOR_TELL, false); 
   } else { 
       sprintf(cTemp, ";tell %s %s", pChar->Name, cLine); 
      DoCommand(GetCharInfo()->pSpawn,cTemp); 
       //dsp_chat_no_events(cTemp, USERCOLOR_TELL, false); 
    } 
} 

VOID Usage() 
{ 
   WriteChatf("--------------------------------------------------------------------------------------------------------"); 
   WriteChatf(MQ2Focii_Version); 
   WriteChatf("\ay/showfocii\ax"); 
   WriteChatf("\axShows All Mods, Hero Stats, Effects"); 
   WriteChatf("\ay/showfocii [mod|hero|eff]\ax"); 
   WriteChatf("\axShows either mod2 stats, hero stats, or effects only\ax"); 
} 
