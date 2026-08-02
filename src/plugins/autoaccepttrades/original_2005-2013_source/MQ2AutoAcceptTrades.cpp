/* MQ2AutoAcceptTrades.cpp v2.0000
	Plugin created by Sadge (with help from Thez)
	Please do not distribute without my consent.

*************************************************************************************
| This is a simple plugin created to auto-accept every trade.						|
| In the MQ2AutoAcceptTrades.ini file, you can change the option to accept trades	|
| from everyone or just the toons you list in the ini file.							|
|																					|
| WARNING!!!! This plugin will *not* accept trades where you are giving an item or	|
| plat away.  If you have added an item or money to a trade, you'll have to 		|
| manually accept.																	|
*************************************************************************************

*****Version 2.0000*****
-- Added inventory checks.  Will no longer accept items if there is no inventory space available.
-- Added ability to "ReportTradeReject" which will send a /tell to the person trading and tell them why it was rejected
-- Added ability to "EchoTradeInfo" which will echo just about everything about the trade to the MQ2 window
-- Added ability to "AllowGoldandLowerDump" which will allow trades which have a quantity of gold, silver, or copper
-- Added commands /autotrade report, /autotrade echo, and /autotrade gold which toggle the settings for those abilities 
*****Version 1.2000*****
-- Fixed bug that would change the name of the person trading with you if you clicked someone else while waiting to accept trade
-- Fixed bad coding
*****Version 1.1000*****
-- Fixed plugin so you will not autoaccept if you hand something (items or money) to a player and they hit accept. Thanks to Thez for help with the syntax.
*****Version 1.0000*****
-- Plugin creation.  Created based on suggestion by Norrathian

*/

#include "../MQ2Plugin.h"

#define   PLUGIN_NAME  "MQ2AutoAcceptTrades"	// Plugin Name
#define   PLUGIN_DATE				20080130	// Plugin Date
#define   PLUGIN_VERS				  2.0000	// Plugin Version

char PersonTrading[MAX_STRING];					//Defines who is trading with you
bool AutoAcceptEveryone;						//Check if you want to accept all trades (true) or just from ini list (false)
bool ReportTradeReject;							//Send tells telling why trade was rejected (if true)
bool EchoTradeInfo;								//Echos trade information to the MQ2 window if true
bool AllowGoldandLowerDump;						//Allows character to reject any trade that includes any coin besides copper (to avoid Copper dumping)
bool TradeDone;									//Flag to stop the plugin from "looping" due to the lag involved
int MAXLIST = 30;								//Max names allowed in ini list
int TotalNames = 0;								//Total names in ini list
char Name[100][64];								//Array to contain each name from ini list
char ItemReceived[100][64];						//Array that contains the names of all items received
char StackedItems[100][64];						//Array that contains the names of the stackable items
char tradecommand[MAX_STRING];					//The actual trade command (used to accepting trade or rejecting)
int StackedItemsCount[8];						//The total number of a stackable item recevied
int StacksCount[8];								//The total number of stacks
int RemainCount[8];								//The number of a stackable item that is less than a complete stack
int MaxStackedItemsCount[8];					//Holds the max stack value of each stackable item
int CoinCount[4];								//Holds the value of each coin received (Plat, Gold, Silver, Copper)
int totalitemstaken;							//The total amount of items received (as inventory space)
int bagsreceived;								//The number of containers received (used to checked top level inventory)
int openTLinv;									//The amount of top level inventory slots available
bool TradeRejected;								//Tells if the trade was rejected (true) or not (false)
int numstackable;								//The number of stackable items received
char ReportReject[MAX_STRING];					//Holds the tell information for why trade was rejected
int InventoryCheck;								//Holds the available inventory (top level+freespace in bags)
char reason[MAX_STRING];						//Holds why trade was rejected
int totalitems;									//Holds the total items received (as absolute)
int AvailableRoom[8];							//How much stack room is available for a stackable item

void DoAutoTrade(PSPAWNINFO pChar, PCHAR szLine);

PreSetup("MQ2AutoAcceptTrades");
PLUGIN_VERSION(PLUGIN_VERS);

VOID ShowHelp(VOID) {	// Will display the current settings
    WriteChatf("%s::Version [\ag%1.4f\ax] Loaded! Created by Sadge",PLUGIN_NAME,PLUGIN_VERS);
	WriteChatf("\ay====================\ax");
    WriteChatf("Status AutoAcceptEveryone is Currently: %s", AutoAcceptEveryone?"\agTRUE":"\arFALSE");
    WriteChatf("Status ReportTradeReject is Currently: %s", ReportTradeReject?"\agTRUE":"\arFALSE");
    WriteChatf("Status EchoTradeInfo is Currently: %s", EchoTradeInfo?"\agTRUE":"\arFALSE");
    WriteChatf("Status AllowGoldandLowerDump is Currently: %s", AllowGoldandLowerDump?"\agTRUE":"\arFALSE");
    WriteChatf("There are <%d> name(s) in your list of people to always trade with", TotalNames);
	WriteChatf("\ay====================\ax");
    
    return;
}	// end of ShowHelp function
VOID Load_INI(VOID)	{	// Loads the ini settings or creates them if not available

	char szTemp[MAX_STRING];		// Local variable to receive and write settings from the ini
	TotalNames = 0;

	GetPrivateProfileString("Settings","AutoAcceptEveryone","TRUE",szTemp,MAX_STRING,INIFileName);
	if (!strnicmp(szTemp,"TRUE",4)) {
		WritePrivateProfileString("Settings","AutoAcceptEveryone","TRUE",INIFileName);
		AutoAcceptEveryone = true;
	}
	else {
		WritePrivateProfileString("Settings","AutoAcceptEveryone","FALSE",INIFileName);
		AutoAcceptEveryone = false;
	}
	GetPrivateProfileString("Settings","ReportTradeReject","TRUE",szTemp,MAX_STRING,INIFileName);
	if (!strnicmp(szTemp,"TRUE",4)) {
		WritePrivateProfileString("Settings","ReportTradeReject","TRUE",INIFileName);
		ReportTradeReject = true;
	}
	else {
		WritePrivateProfileString("Settings","ReportTradeReject","FALSE",INIFileName);
		ReportTradeReject = false;
	}
	GetPrivateProfileString("Settings","EchoTradeInfo","TRUE",szTemp,MAX_STRING,INIFileName);
	if (!strnicmp(szTemp,"TRUE",4)) {
		WritePrivateProfileString("Settings","EchoTradeInfo","TRUE",INIFileName);
		EchoTradeInfo = true;
	}
	else {
		WritePrivateProfileString("Settings","EchoTradeInfo","FALSE",INIFileName);
		EchoTradeInfo = false;
	}
	GetPrivateProfileString("Settings","AllowGoldandLowerDump","TRUE",szTemp,MAX_STRING,INIFileName);
	if (!strnicmp(szTemp,"TRUE",4)) {
		WritePrivateProfileString("Settings","AllowGoldandLowerDump","TRUE",INIFileName);
		AllowGoldandLowerDump = true;
	}
	else {
		WritePrivateProfileString("Settings","AllowGoldandLowerDump","FALSE",INIFileName);
		AllowGoldandLowerDump = false;
	}

	for (int i = 0; i<(MAXLIST+1); i++)			// Loop to read names
	{
		sprintf(szTemp,"Name%i",i);
		if(!GetPrivateProfileString("Names",szTemp,"",Name[i],64,INIFileName))
			return;
		else
			TotalNames++;
	}
}	// end Load_INI function

PLUGIN_API VOID InitializePlugin(VOID) {	// Initializes the Plugin & spews out plugin information

	DebugSpewAlways("Initializing MQ2AutoAcceptTrades");
	Load_INI();
	AddCommand("/autotrade", DoAutoTrade);

	WriteChatf("%s::Version [\ag%1.4f\ax] Loaded! Created by Sadge",PLUGIN_NAME,PLUGIN_VERS);
	WriteChatf("\ay====================\ax");
	WriteChatColor("You can view the current settings by typing /autotrade", CONCOLOR_GREEN);
	WriteChatColor("You can toggle auto-accepting with everyone by typing /autotrade TRUE (on) or FALSE (off)", CONCOLOR_GREEN);
	WriteChatColor("You can toggle reporting trade reject to person trading by typing /autotrade report", CONCOLOR_GREEN);
	WriteChatColor("You can toggle echoing trade information in the MQ2 window by typing /autotrade echo", CONCOLOR_GREEN);
	WriteChatColor("You can toggle allowing receiving gold and lower (Gold, Silver, or Copper Dump) by typing /autotrade gold", CONCOLOR_GREEN);
	WriteChatColor("You can add someone to your list of people to always trade with by typing /autotrade <CharName>", CONCOLOR_GREEN);
	WriteChatf("\ay====================\ax");
    WriteChatf("Status AutoAcceptEveryone is Currently: %s", AutoAcceptEveryone?"\agTRUE":"\arFALSE");
    WriteChatf("Status ReportTradeReject is Currently: %s", ReportTradeReject?"\agTRUE":"\arFALSE");
    WriteChatf("Status EchoTradeInfo is Currently: %s", EchoTradeInfo?"\agTRUE":"\arFALSE");
    WriteChatf("Status AllowGoldandLowerDump is Currently: %s", AllowGoldandLowerDump?"\agTRUE":"\arFALSE");
	WriteChatf("\ay====================\ax");
}	// end InitializePlugin function

PLUGIN_API VOID ShutdownPlugin(VOID) {	// Contains shutdown information
	DebugSpewAlways("Shutting down MQ2AutoAcceptTrades");
	RemoveCommand("/autotrade");
}	// end ShutdownPlugin function
bool CheckWindow(PCHAR windowinfo) {	// This will check the window information that is passed to it and return true/false
   ParseMacroData(windowinfo);
   if (strcmp(windowinfo,"TRUE") == 0) return true;
   else if (strcmp(windowinfo,"NULL") != 0) return false;
   else if (strcmp(windowinfo,"NULL") == 0) return true;
   else return false;
}	// end CheckWindow function
void AcceptTrade(VOID) {	// simply contains the command to accept the trade
	sprintf(tradecommand,"/nomodkey /notify tradewnd TRDW_Trade_Button leftmouseup ");
}	// end AcceptTrade function
void RejectTrade(VOID) {	// simply contains the command to reject the trade
	sprintf(tradecommand,"/nomodkey /notify tradewnd TRDW_Cancel_Button leftmouseup ");
}	// end RejectTrade function
void EchoTrade(VOID) {	// contains the trade information to echo in MQ2 window
	char timetrade[10];
	sprintf(timetrade, "${Time}");
	ParseMacroData(timetrade);
	WriteChatf("\ay====================\ax");
	WriteChatf("\agTrade Information\ax");
	WriteChatf("\ay====================\ax");
	WriteChatf("Trade from: <%s> at %s", PersonTrading, timetrade);
	WriteChatf("Inventory Slots Available: %i", InventoryCheck);
	WriteChatf("Top Level Inventory Slots Available: %i", openTLinv);
	WriteChatf("\ay====================\ax");
	WriteChatf("TotalInventorySpaceNeeded = %i", totalitemstaken);
	WriteChatf("TotalBagsGivenToMe = %i", bagsreceived);
	WriteChatf("TotalPlatGivenToMe = %i", CoinCount[1]);
	WriteChatf("TotalGoldGivenToMe = %i", CoinCount[2]);
	WriteChatf("TotalSilverGivenToMe = %i", CoinCount[3]);
	WriteChatf("TotalCopperGivenToMe = %i", CoinCount[4]);
	if (totalitems>0) {
		WriteChatf("Items Given to Me:");
		for (int a=1; a<9; a++) {
			if (stricmp(ItemReceived[a],"NULL")) WriteChatf(" %i) %s", a, ItemReceived[a]);
		}
		if (numstackable>=1) {
			WriteChatf("Stackable Items:");
			for (int a=1; a<numstackable+1; a++){
				WriteChatf(" %i) %s", a, StackedItems[a]);
				WriteChatf("  Received: %i", StackedItemsCount[a]);
				WriteChatf("  Available Stack Room: %i", AvailableRoom[a]);
			}
		}
	}
	WriteChatf("\ay====================\ax");
	if (TradeRejected) {
		WriteChatf("\arTRADE REJECTED\ax");
		WriteChatf("Reason: %s", reason);
	}
	else WriteChatf("\agTRADE ACCEPTED\ax");
	WriteChatf("\ay====================\ax");
}	// end EchoTrade function
void DoTrade(VOID) {	// This will read all the items and coin received and determine if the trade is acceptable or not
	char TLInventoryCheck[MAX_STRING];						//Command to check top level inventory
	char itemstakencheck[MAX_STRING];						//Command to check for items received
	char stackablecheck[MAX_STRING];						//Command to check if item received is stackable
	char bagcheck[MAX_STRING];								//Command to check if item is a bag
	int tradeslots;											//# of tradeslot (9-16)
	char FreeInventoryCheck[MAX_STRING];					//Command to check for how much inventory space is available (total)
	char CoinCheck[MAX_STRING];								//Checks for coins
	char NumberinStack[MAX_STRING];							//Checks number of items for each stack received
	char MaxNumberinStack[MAX_STRING];						//Checks what the max stackable value is of each stack received
	char AvailableStackRoom[MAX_STRING];					//Checks how many items you can receive before you make a new stack
	bool matchfound;										//Checks for multiple stacks of 1 item
	int addstackable;										//Adds stackable items to total items received

	for (int a=1; a<5; a++) {								//initalizes the CoinCount array
		CoinCount[a] = 0;
	}	// end for loop (CoinCount)

	for (int a=1; a<9; a++) {								//initalizes the various other arrays
		sprintf(ItemReceived[a], "NULL");
		sprintf(StackedItems[a], "NULL");
		StackedItemsCount[a] = 0;
		StacksCount[a] = 0;
		RemainCount[a] = 0;
		AvailableRoom[a] = 0;
	}	// end for loop (initialize arrays)

	for (int a=0; a<4; a++) {								//Gets the coin received values (plat, gold, silver, copper)
		sprintf(CoinCheck, "${Window[Tradewnd].Child[TRDW_HisMoney%i].Text}", a);
		ParseMacroData(CoinCheck);
		CoinCount[a+1]=atoi(CoinCheck);
	}	//end for loop (coins received)

	openTLinv=0;
	matchfound=false;
	sprintf(FreeInventoryCheck, "${Me.FreeInventory}");
	ParseMacroData(FreeInventoryCheck);
	InventoryCheck=atoi(FreeInventoryCheck);
	for (int TLInv=23; TLInv<31; TLInv++) {					//Loops to check how many free top level invetory slots are available
		sprintf(TLInventoryCheck,"${Me.Inventory[%i]}", TLInv);
		ParseMacroData(TLInventoryCheck);
		if (!stricmp(TLInventoryCheck,"NULL"))openTLinv++;
	}	//end for loop (top level inventory)
	
	totalitemstaken=0;
	bagsreceived=0;
	numstackable=0;
	totalitems=0;
	
	// Ok - let's start the loop that will look at each trade slot to find what's being given to the receiver
	for (tradeslots=9; tradeslots<17; tradeslots++) {		//Loops to check each trade received slot (9-16)
		sprintf(itemstakencheck, "${InvSlot[trade%i].Item}", tradeslots);
		if (!CheckWindow(itemstakencheck)) {											// Is there an item in that slot?
			totalitems++;																// Total items increases
			sprintf(bagcheck, "${InvSlot[trade%i].Item.Container}", tradeslots);		// Set value to perform check to see if you've recevied a bag
			ParseMacroData(bagcheck);													// Checks if item is a bag
			if (stricmp(bagcheck,"0")) bagsreceived++;									// If item is a bag, number of bags received goes up by 1
			sprintf(stackablecheck, "${InvSlot[trade%i].Item.Stackable}", tradeslots);	// Sets value to see if item in trade slot is stackable
			ParseMacroData(stackablecheck);												// Performs the acutal stackable check
			if (stricmp(stackablecheck,"FALSE")) {										// If item is stackable, then we need to do some stuff to see if it'll take up an inventory slot.
				matchfound=false;														// Sets the value of matchfound to false since we haven't found a match yet
				sprintf(NumberinStack,"${InvSlot[trade%i].Item.Stack}",tradeslots);
				ParseMacroData(NumberinStack);											// Gets the number of stacked items
				sprintf(MaxNumberinStack,"${InvSlot[trade%i].Item.StackSize}",tradeslots);
				ParseMacroData(MaxNumberinStack);										// Gets the max stack value for item

				int n=1;																// Initialized loop variable
				while (n<9 && !matchfound) {											// While looping through the StackedItems array (at value n) and we haven't found a match yet ...
					if (!stricmp(itemstakencheck,StackedItems[n])) {					// Compares the item to the StackedItem array at posistion n
						StackedItemsCount[n]=atoi(NumberinStack)+StackedItemsCount[n];	// If we've already received some of this stackable item, add the amount to the total amount
						matchfound=true;												// We've found a match!
					}	// end if statement (if we've already stored the stackable item)
					else n++;
				}	// end while loop (to check the staked items we've already stored)
				if (!matchfound) {														// if we didn't find a match for the stackable item previously...
					numstackable++;														// we have +1 stackable item count
					sprintf(StackedItems[numstackable], "%s", itemstakencheck);
					StackedItemsCount[numstackable]=atoi(NumberinStack);				// we store the total number in the stack
					MaxStackedItemsCount[numstackable]=atoi(MaxNumberinStack);			// we store the max stack value
				} // end if statement (no matchfound for stacked item)
			} // end if statement (there was a stackable item found)
			else {																		// if the item was not stackable
				totalitemstaken++;														// total items received +1
				sprintf(ItemReceived[totalitemstaken], "%s", itemstakencheck);			// Set Value for the name of the item received
			}	//end else statement (not a stackable item)
		} // end if statement (item found)
	} // end tradeslots loop (checked all tradeslots)
	addstackable=totalitemstaken;														// Gets proper array value to add stacked items to the items received array
	if (numstackable>=1){																// If we had at least one stackable item ...
		for (int a=1; a<numstackable+1; a++){											// Loop until we reach the end of the amount of stackable items we received
			sprintf(ItemReceived[addstackable+a], "%s", StackedItems[a]);				// Adds the stackable item to the total items received array
			sprintf(AvailableStackRoom,"${FindItem[=%s].FreeStack}", StackedItems[a]);
			ParseMacroData(AvailableStackRoom);											// Finds how much of stackable item we can recieve before it makes a new stack
			AvailableRoom[a]=atoi(AvailableStackRoom);
			StacksCount[a]=int(StackedItemsCount[a] / MaxStackedItemsCount[a]);			// Finds how many total stacks of the item we received
			RemainCount[a]=StackedItemsCount[a] % MaxStackedItemsCount[a];				// Finds how many of items there are that don't make a complete stack
			if (AvailableRoom[a]<StackedItemsCount[a]) {								// if we don't have enough room to take the items without taking up an inventory space...
				totalitemstaken=totalitemstaken+StacksCount[a];							// totalitems taken increases by the number of stacks we have
				if (RemainCount[a]>AvailableRoom[a]) totalitemstaken++;					// if there's a remainder left, that counts as inventory space as well
			}	//end if statement (if we don't have enough room for the stacked items)
		} // end for loop (stackable items)
	}	// end if statement (we found a stackable item)

	// Now that we have all the information we need, we need to run it through some checks to see if we will accept the trade
	if ((!AllowGoldandLowerDump) && (CoinCount[2]>0 || CoinCount[3]>0 || CoinCount[4]>0)) {	// If we're not accepting any coin lower than plat & were given gold, silver, or copper ...
		sprintf(ReportReject,"/tell %s Keep your chump change -- I only accept plat!", PersonTrading);	// Sets the tell message to report why trade is rejected
		RejectTrade();																		// Set the tradecommand to reject
		TradeDone=true;																		// Trade check complete
		TradeRejected=true;																	// We rejected the trade
		sprintf(reason, "Given money other than plat while I'm not accepting that.");		// Sets the reason rejected (if echo trade info is on)
		return;																				// returns us so we can complete the trade rejection
	}	// end if statement (coin dumping check)
	if (bagsreceived>openTLinv) {															// Checks if we received more bags than we have top level inventory slots
		if (openTLinv<1) sprintf(ReportReject,"/tell %s I don't have any room for bags", PersonTrading);			// sets reject /tell for 0 inventory available
		if (openTLinv>0) sprintf(ReportReject,"/tell %s I only have room for %i bag", PersonTrading, openTLinv);	// sets reject /tell for 1 inventory available
		if (openTLinv>1) sprintf(ReportReject,"/tell %s I only have room for %i bags", PersonTrading, openTLinv);	// sets reject /tell for >1 inventory available
		RejectTrade();																		// Set the tradecommand to reject
		TradeDone=true;																		// Trade check complete
		TradeRejected=true;																	// We rejected the trade
		sprintf(reason, "Given more bags than I have top level inventory room for");		// sets the reason rejected (if echo trade info is on)
		return;																				// returns us so we complete the trade rejection 
	}	// end if statement (top level inventory check for bags received)

	if (InventoryCheck<totalitemstaken){													// if our total inventory space is less then items received ...
		if (InventoryCheck<1) sprintf(ReportReject,"/tell %s I don't have any room in my inventory", PersonTrading);							// sets reject /tell for 0 inventory available
		if (InventoryCheck>0) sprintf(ReportReject,"/tell %s I only have room for %i item in my inventory", PersonTrading, InventoryCheck);		// sets reject /tell for 1 inventory available
		if (InventoryCheck>1) sprintf(ReportReject,"/tell %s I only have room for %i items in my inventory", PersonTrading, InventoryCheck);	// sets reject /tell for >1 inventory available
		RejectTrade();																		// Set the tradecommand to reject
		TradeDone=true;																		// Trade check complete
		TradeRejected=true;																	// We rejected the trade
		sprintf(reason, "Given more items than I have room for.");							// sets the reason rejected (if echo trade info is on)
		return;																				// returns us so we complete the trade rejection 
	} // end if statement (total inventory check)

	AcceptTrade();																			// all checks passed so sets tradecommand to accept trade
	TradeDone=true;																			// Trade check complete
}	//end DoTrade function

BOOL CheckNames(PCHAR szName) {						// Checks to see if person trading is on your list

	for (int i = 0; i<(TotalNames+1); i++) {		// Loops until the total amount of names on list is reached
		if (!stricmp(szName,Name[i])) return true;	// if name is found on the list, then returns true
	}	// end for loop (names check)
	return false;									// if no name was found on list, then returns false
}	// end CheckNames function
VOID DoAutoTrade(PSPAWNINFO pChar, PCHAR szLine)	// This allows you to change settings
{
	char szTemp[MAX_STRING];

	if (gGameState == GAMESTATE_INGAME) {			// Checks to make sure we're in game
		if (szLine[0]==0) {							// if no parameter was passed then show the help (settings)
			ShowHelp();
			return;
		}	// end if statement (no parameters)
		if (!stricmp(szLine,"TRUE")) {				// if received "TRUE" turns autoaccepteveryone on
			AutoAcceptEveryone = true;
			WritePrivateProfileString("Settings","AutoAcceptEveryone",szLine,INIFileName);
			WriteChatf("AutoAcceptEveryone is now: %s", AutoAcceptEveryone?"\agTRUE":"\arFALSE");
		}	// end if statement (autoaccepteveryone true)
		else if (!stricmp(szLine,"FALSE")) {		// if received "FALSE" turns autoaccepteveryone off
			AutoAcceptEveryone = false;
			WritePrivateProfileString("Settings","AutoAcceptEveryone",szLine,INIFileName);
			WriteChatf("AutoAcceptEveryone is now: %s", AutoAcceptEveryone?"\agTRUE":"\arFALSE");
		}	// end if statement (autoaccepteveryone false)
		else if (!stricmp(szLine,"report")) {		// if received "report" toggles reporting the trade reject
			if (ReportTradeReject) {				// if it's on turn it off
				ReportTradeReject = false;
				sprintf(szLine,"FALSE");
			}	// end if statment (if it's on turn it off)
			else {									// turn it on
				ReportTradeReject = true;
				sprintf(szLine,"TRUE");
			}	// end else statement (it's off so turn it on)
			WritePrivateProfileString("Settings","ReportTradeReject",szLine,INIFileName);
			WriteChatf("ReportTradeReject is now: %s", ReportTradeReject?"\agTRUE":"\arFALSE");
		}	// end if statement (toggle reporttradereject)
		else if (!stricmp(szLine,"echo")) {		// if received "echo" toggles echoing the trade info
			if (EchoTradeInfo) {				// if it's on turn it off
				EchoTradeInfo = false;
				sprintf(szLine,"FALSE");
			}	// end if statment (if it's on turn it off)
			else {									// turn it on
				EchoTradeInfo = true;
				sprintf(szLine,"TRUE");
			}	// end else statement (it's off so turn it on)
			WritePrivateProfileString("Settings","EchoTradeInfo",szLine,INIFileName);
			WriteChatf("EchoTradeInfo is now: %s", EchoTradeInfo?"\agTRUE":"\arFALSE");
		}	// end if statement (toggle echotradeinfo)
		else if (!stricmp(szLine,"gold")) {		// if received "gold" toggles accepting gold and lower coins
			if (AllowGoldandLowerDump) {		// if it's on turn it off
				AllowGoldandLowerDump = false;
				sprintf(szLine,"FALSE");
			}	// end if statment (if it's on turn it off)
			else {									// turn it on
				AllowGoldandLowerDump = true;
				sprintf(szLine,"TRUE");
			}	// end else statement (it's off so turn it on)
			WritePrivateProfileString("Settings","AllowGoldandLowerDump",szLine,INIFileName);
			WriteChatf("AllowGoldandLowerDump is now: %s", AllowGoldandLowerDump?"\agTRUE":"\arFALSE");
		}
		else {									// if anything else received, it's treated as a name to add to the list of acceptable trade names
			WriteChatf("Adding \ay< %s >\ax to list of people you'll always trade with",szLine);
			sprintf(szTemp,"Name%i",TotalNames);
			WritePrivateProfileString("Names",szTemp,szLine,INIFileName);
			TotalNames++;						// increases the total amount of names on our list
			Load_INI();
			return;
		}	// end else statement (it must be a name)
		Load_INI();								// reloads the ini
	}	// end if we're in game
}	// end function DoAutoTrade
PLUGIN_API VOID OnPulse(VOID) {									// Checks to see if someone's trading with us

	char tradeacceptcheck[MAX_STRING];							// have they hit accept?
	char platcheck[MAX_STRING];									// did we give plat?
	char windowcheck[MAX_STRING];								// is trade window up?
	char goldcheck[MAX_STRING];									// did we give gold?
	char silvercheck[MAX_STRING];								// did we give silver
	char coppercheck[MAX_STRING];								// did we give copper?
	char itemsgivencheck[MAX_STRING];							// did we give any items?

	if (gGameState==GAMESTATE_INGAME) {							// are we in game?
		sprintf(windowcheck, "${Window[tradewnd].Open}");
		sprintf(itemsgivencheck, "${InvSlot[trade1].Item}");
		sprintf(platcheck, "${Window[Tradewnd].Child[TRDW_MyMoney0].Text.Equal[0]}");
		sprintf(goldcheck, "${Window[Tradewnd].Child[TRDW_MyMoney1].Text.Equal[0]}");
		sprintf(silvercheck, "${Window[Tradewnd].Child[TRDW_MyMoney2].Text.Equal[0]}");
		sprintf(coppercheck, "${Window[Tradewnd].Child[TRDW_MyMoney3].Text.Equal[0]}");
		sprintf(tradeacceptcheck, "${Window[Tradewnd].HisTradeReady}");

		if (!CheckWindow(windowcheck)) {						// is the trade window closed?
			TradeDone=false;
			TradeRejected=false;
		}	// end if statement (trade window closed)
		else if ((CheckWindow(itemsgivencheck)) && (CheckWindow(platcheck)) && (CheckWindow(goldcheck))
			&& (CheckWindow(silvercheck)) && (CheckWindow(coppercheck)) && (CheckWindow(tradeacceptcheck))
			&& !TradeDone) {									// checks to make sure we haven't given any items or coin and the trade check hasn't been completed
				sprintf(PersonTrading, "${Window[Tradewnd].Child[TRDW_HisName].Text}");
				ParseMacroData(PersonTrading);
				DoTrade();
				if ((!AutoAcceptEveryone) && !(CheckNames(PersonTrading))) {	// if you're not accepting all trades and the person trading is not on the always trade list...
					RejectTrade();												// sets tradecommand to reject
					sprintf(ReportReject,"/tell %s I'm not interested in anything you have", PersonTrading);
					TradeRejected=true;											// we rejected the trade
					TradeDone=true;												// trade check complete
					sprintf(reason, "Not accepting all trades and person not on always accept list.");	// sets the reason rejected (if echo trade info is on)
				}	// end if statement (not accepting all trades and name is not on list)
				DoCommand(NULL,tradecommand);									// do the tradecommand (accept or reject)
				if ((ReportTradeReject) && (TradeRejected)) DoCommand(NULL,ReportReject);	// if we rejected trade and are reporting, report reject
				if (EchoTradeInfo) EchoTrade();									// if we're echoing trade info to MQ2 window, echo trade info
		}	// end if statement (do trade if nothing given and trade not done)
	}	// end if statement (in game)
}	// end OnPulse function
