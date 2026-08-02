|just include & call CheckModRod in main loop. ${mcHUDmodrod} will contain modrod status

| Changes by Gnits
| Added the ability for the macro to determine if both 
| mcHUDmodrodCharges = Current number of charges (First from V2 Rods then from V1 Rods)
| mcHUDmodAvail = Used to show if a modrod is available now for use (check = 1)
| mcHUDmodV1(or V2) = Shows which types of rods are in the inventory


|Item ID of a mod rod

#Event UsedModRod			"#*#You experience a mystical transvergance.#*#"
Sub Event_UsedModRod
	/echo [${Time}] Mod rod used (+360 mana)
	/call CheckModRod
/return

#Event UsedElemModRod			"#*#You experience an elemental transvergance.#*#"
Sub Event_UsedElemModRod
	/echo [${Time}] Mod rod used (+656 mana)
	/call CheckModRod
/return

#define ROD1 6346
#define ROD2 3426
#define ROD3 52674
#define ROD4 52709
#define ROD5 52817
#define ROD6 52803
#define ROD7 52888

sub ChooseModRod
	/declare sCurrent string local
	/declare sNext string local

|  
|  Choose the modrod in order of mana
|  - Modulation Rod (150 @ 1 )
|  - Rod of Mystical Transvergence (360 @ 3 )
|  - Wand of Elemental Transvergence (656 @ 3)
|  - Rod of Prime Transvergence ( 800 @ 3 )
|  - Rod of Ethereal Transvergence ( 976 @ 3 )
|  - Rod of Mechamagical Mastery ( 1040 @ 3 ) (Mage only)
|  - Wand of Temporal Mastery ( 1269 @ 3 ) (Mage only)
|
|  Function will show all rods in decending order, so if you have been given 2 different flavors of mod rod
|  it will use the highest one first.


|  - Modulation Rod (150 @ 1 )
	/varset sCurrent ${FindItem[=Summoned: Modulating Rod]}

|  - Rod of Mystical Transvergence (360 @ 3 )
	/varset sNext ${FindItem[=Rod of Mystical Transvergence]}
	/if (${FindItem[=Rod of Mystical Transvergence].InvSlot}) /varset sCurrent ${sNext}

|  - Wand of Elemental Transvergence (656 @ 3)
	/varset sNext ${FindItem[=Wand of Elemental Transvergence]}
	/if (${FindItem[=Wand of Elemental Transvergence].InvSlot}) /varset sCurrent ${sNext}

|  - Rod of Prime Transvergence ( 800 @ 3 )
	/varset sNext ${FindItem[=Prime Transvergence]}
	/if (${FindItem[=Prime Transvergence].InvSlot}) /varset sCurrent ${sNext}

|  - Rod of Ethereal Transvergence ( 976 @ 3 )
	/varset sNext ${FindItem[=Rod of Ethereal Transvergence]}
	/if (${FindItem[=Rod of Ethereal Transvergence].InvSlot}) /varset sCurrent ${sNext}

|  - Rod of Mechamagical Mastery ( 1040 @ 3 ) (Mage only)
	/varset sNext ${FindItem[=Rod of Mechamagical Mastery]}
	/if (${FindItem[=Rod of Mechamagical Mastery].InvSlot}) /varset sCurrent ${sNext}

|  - Wand of Temporal Mastery ( 1269 @ 3 ) (Mage only)
	/varset sNext ${FindItem[=Wand of Temporal Mastery]}
	/if (${FindItem[=Wand of Temporal Mastery].InvSlot}) /varset sCurrent ${sNext}

|  - Rod of Spectral Transvergence ( 1350 @ 3 )
	/varset sNext ${FindItem[=Rod of Spectral Transvergence]}
	/if (${FindItem[=Rod of Spectral Transvergence].InvSlot}) /varset sCurrent ${sNext}


	/return ${sCurrent}

Sub CheckModRod
	|  Revamped logic on autoinventory and usage notification.
	|  ToDo:  Usage notification.


	/if (!${gimp_modrod_loaded}) {
		/declare gimp_modrod_loaded		int	outer	1
		/declare mcHUDmodrod			string	outer
		/declare mcHUDmodrodCharges		int	outer	0
		/declare mcHUDmodAvail			int	outer	0
		/declare sCurrentRod			string  outer
		/echo gimp_modrod.inc initialized
		}

	/declare str		string	local
	/declare i		int	local

	/call ChooseModRod
	/varset sCurrentRod ${Macro.Return}
	
	
	/if (((${Cursor.ID} == ROD1) || (${Cursor.ID} == ROD2) || (${Cursor.ID} == ROD3) || (${Cursor.ID} == ROD4) || (${Cursor.ID} == ROD5) || (${Cursor.ID} == ROD6) || (${Cursor.ID} == ROD7)) && !${Window[ConfirmationDialogBox].Open}) {
		/if (${Me.MaxMana}) {
			/echo Autoinventorying mod rod
			/delay 3s
			/if ((${Cursor.ID} == ROD1) || (${Cursor.ID} == ROD2) || (${Cursor.ID} == ROD3) || (${Cursor.ID} == ROD4) || (${Cursor.ID} == ROD5) || (${Cursor.ID} == ROD6) || (${Cursor.ID} == ROD7)) /autoinventory
		} else {
			/if ((${Cursor.ID} == ROD1) || (${Cursor.ID} == ROD2) || (${Cursor.ID} == ROD3) || (${Cursor.ID} == ROD4) || (${Cursor.ID} == ROD5) || (${Cursor.ID} == ROD6) || (${Cursor.ID} == ROD7)) /destroy
		}
	}	

|
|	This routine will do the following:
|	No mod rod = No, X Mages in zone
|	Original Mod rod has no charges, hard set it to 1.
|	Other Mod rod as follows:
|		If 1 charge, change text from charges to charge, otherwise say charges
|		If timer is active, Mod Rod is not available and mcHUDmodAvail = 1 (Good for integrations scripts to test for rod availability)
|		If timer is not active, then then it is available (as it would not test for a timer otherwise).
|
|&& and (${FindItem[${sCurrentRod}].ID != 6346)

	/if (${Bool[${sCurrentRod}]}) {


		/if (${FindItem[${sCurrentRod}].ID} == 6346) {
			/varset str 1 Charge
			/varset mcHUDmodrodCharges 1
			/if (${FindItem[${sCurrent}].Timer}) {
				/varcalc i ${FindItem[${sCurrentRod}].Timer}*6
				/varset mcHUDmodrod ${str}, ${Int[${Math.Calc[${i}/60]}]}m${Int[${Math.Calc[${i}%60]}]}s Left
				/varset mcHUDmodAvail 0
				} else {
				/varset mcHUDmodAvail 1
				}
			} else {



			/if (${FindItem[${sCurrentRod}].Charges} == 1 ) {
				/varset str ${FindItem[${sCurrentRod}].Charges} Charge
				/varset mcHUDmodrodCharges ${FindItem[${sCurrentRod}].Charges}
				} else {
				/varset str ${FindItem[${sCurrentRod}].Charges} Charges
				/varset mcHUDmodrodCharges ${FindItem[${sCurrentRod}].Charges}
				}


			/if (${FindItem[${sCurrentRod}].Timer}) {
				/varcalc i ${FindItem[${sCurrentRod}].Timer}*6
				/varset mcHUDmodrod ${str}, ${Int[${Math.Calc[${i}/60]}]}m${Int[${Math.Calc[${i}%60]}]}s Left
				/varset mcHUDmodAvail 0
				} else {
				/varset mcHUDmodrod ${str}
				/varset mcHUDmodAvail 1
				}
			}
		} else {
			/varset mcHUDmodrod NO, ${SpawnCount[pc mage]} mages in zone
			/varset mcHUDmodAvail 0
			/varset mcHUDmodrodCharges 0
		}



	/return

