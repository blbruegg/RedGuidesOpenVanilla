|  V1.0 - Gnits 11/28/05 (initial prior to submission to gimp)
|  V2.0 - Gnits 11/01/08 (lots of changes including a revamped Automage)
|
|  gimp addon for mage specific functions
|
|  Commands
|  /Automage - Toggle automage functions
|    /Pet(water, air, fire, earth) - Set the type of pet to be used.
|    /Nukelight - Automage nukes every 33% of the mob (ie max of 3)
|    /Nukemedium - Automage nukes every 25% of the mob (ie max of 4)
|    /Nukeheavy - Automage nukes every 20% of the mob (ie max of 5)
|    /Nukechain - Automage nukes every 1% of the mob (ie chain casting)
|    /nukefire - set the automage nuke spell to Ancient: Nova Strik
|    /nukemagic - set the automage nuke spell to Blade Strik
|    /nukebolt - set the automage nuke spell to Bolt of Jerikor
|    /nukeficle - set the automage nuke spell to Ficle Fire
|  /Siniamob - Autocasts Malosinia from mask until landed or canceled
|  /Malodone - give up on malosinia (cancels the previous chain cast of malosinia)
|  /newpet - Makes a fresh pet.  Uses the value set by /pet(type)
|
|  TODO List:
|	- Cleaning up of pet aliasias as they are not needed (better
|         understanding of how aliases work
|	- Add INI based settings where appropriate
|	_ cleanup handling of swapping items since the new spellroutines take this
|	  into account
|	- Need handling of when one of these events occur and not a mage
|	- Need to lookup the other wear off messages and spells for pet haste
|	  include the shield for saryrn

#include spell_routines.inc
#include swapcast.inc

#Event RezNewPet	"[MQ2] Summon A Fresh Pet!"
#Event MaloTarget	"[MQ2] Malosinia type #1#"
#Event exp 		"#*#experience!#*#" 
#Event rageon 		"#*#has become ENRAGED#*#" 
#Event rageoff 		"#*#is no longer enraged#*#" 
#event NukeType 	"[MQ2] Automage set to nuke #1#"
#event Fickle			"Small flames dance over your hands."
#Event DummyBuff	"[MQ2] Cast Dummy Buff"
#event PauseMage  	"[MQ2] PauseMage"
/g g
#Event NukeSpell 	"[MQ2] Automage spelltype set to #1#"
#Event SetPet 		"[MQ2] Set pet to type #1#"
#Event CancelMalo 	"[MQ2] Canceling Malosinia"
#Event Automage		"[MQ2] Toggle Automage"
#Event RodTell		"#*#rod#*#"
#Event RodRequests	"[MQ2] Toggle Rod Requests"
#Event AMCheck 		"[MQ2] Check current AM Settings"
#Event FBurn 		"[MQ2] FullBurning"
#Event FPetBurn		"[MQ2] FullPetBurning"
#Event FRaidBurn	"[MQ2] FullRaidBurning"
#Event CancelFB		"[MQ2] Canceling FB"

#event PetStun		"#1# is stunned by a gust of air#*#"
#Event QueenMalAttack	"#*# and takes careful aim at #1#!"
#Event Twincasted	"You twincast #1#"



Sub SetMageSpells
	/declare MSFireQuick1 string	outer	Torrid Sands Rk. III
	/declare MSFireQuick2 string	outer	Blazing Sands Rk. II
	/declare MSFireBolt1 string	outer	Spear of Magma Rk. II
	/declare MSFireBolt2 string	outer	Bolt of Rhyolite Rk. II
	/declare MSFireFickle1 string	outer	Fickle Blaze Rk. III
	/declare MSFireFickle2 string	outer	Fickle Pyroclasm Rk. III
	/declare MSFireShock1 string	outer	Shock of Discordant Steel Rk. III
	/declare MSFireShock2 string	outer	Shock of Ethereal Steel Rk. II
	/declare MSFireSumm1 string	outer	Annihilate the Anomalous Rk. III
	/declare MSFireSumm2 string	outer	Annihilate the Divergent Rk. II

	/declare MSGroupDS string	outer	Circle of Dreamfire Rk. II
	/declare MSPetMana string	outer	Thaumatize Pet Rk. III
	/declare MSDefPet string	outer	Tellurian Bodyguard Rk. II
	/declare MSRSPet string		outer	Ruinous Servant Rk. II
	/declare MSPetRune string	outer	Iceflame Ward Rk. II
	/declare MSManaRegen string	outer	Cognitive Guardian Rk. II
	/declare MSPetDmgAura string	outer	Arcane Distillect Rk. II
	/declare MSPetHaste string	outer	Burnout IX Rk. II
	/declare MSMalo string		outer	Malosene Rk. II
	/declare MSCombat string	outer	Burning Veil
	/declare MSHighDS string	outer	Surge of Shadowflares Rk. II
	/declare MSTwin string		outer	Twincast Rk. III
	/declare MSShield string	outer	Shield of Dreams Rk. II
	/declare MSMageCvt string	outer	Spectral Symbiosis Rk. II
	/declare MSMageRec string	outer	Spectral Symbiosis Recourse Rk. II
	/declare MSHarvest string	outer	Gather Potential Rk. II




	/return

Sub Event_Twincasted(Junk,TCSpell)
	/echo You have twincasted ${TCSpell}
	/popup You have twincasted ${TCSpell}
	/return

Sub Event_FBurn
	/declare iTarget int inner
	/echo Startung full burn
	:FBStart
	/if (${FBActive}==0) {
		/varset FBActive 1
		/return
		}
	/if (${Me.AltAbilityReady[Force of Elements]}) /call Cast "Force of Elements"
	/if ((${Me.SpellReady[Thaumatize Pet Rk. IIi]}) && (${Me.Pet.ID}))  {
		/varset iTarget ${Target.ID}
		/call Cast "Thaumatize Pet Rk. iII"
		/target id ${iTarget}
		/delay 20
		}
	/if (${Cast.Ready[1]}) /call Cast "${Me.Gem[1]}"
	/if (${Cast.Ready[2]}) /call Cast "${Me.Gem[2]}"
	/if (${Macro.Return.Equal[CAST_NOTARGET]}) {
		/echo Target gone, canceling full burn!
		/return
		}
	/if (${Macro.Return.Equal[CAST_OUTOFMANA]}) {
		/echo Tank empty, canceling full burn!
		/return
		}
	/doevents
	/goto :FBStart
	/return

Sub Event_FPetBurn
	/declare iTarget int inner
	/echo Startung full burn with jolt
	:FPBStart
	/if (${FBActive}==0) {
		/varset FBActive 1
		/return
		}
	/if (${Me.AltAbilityReady[Force of Elements]}) /call Cast "Force of Elements"
	/if ((${Me.SpellReady[Thaumatize Pet Rk. IiI]}) && (${Me.Pet.ID}))  {
		/varset iTarget ${Target.ID}
		/call Cast "Thaumatize Pet Rk. iII"
		/target id ${iTarget}
		/delay 20
		}
	/if (${Cast.Ready[3]}) /call Cast "${Me.Gem[3]}"
	/delay 1
	/if (${Cast.Ready[1]}) /call Cast "${Me.Gem[1]}"
	/delay 1
	/if (${Cast.Ready[2]}) /call Cast "${Me.Gem[2]}"
	/doevents
	/if (${Macro.Return.Equal[CAST_NOTARGET]}) {
		/echo Target gone, canceling full burn!
		/return
		}
	/if (${Macro.Return.Equal[CAST_OUTOFMANA]}) {
		/echo Tank empty, canceling full burn!
		/return
		}
	/goto :FPBStart
	/return

Sub Event_FRaidBurn
	/declare iTarget int inner
	/echo Startung full burn with jolt, RS and Twin
	:FBRStart
	/if (${FBActive}==0) {
		/varset FBActive 1
		/return
		}
	/if (${Cast.Ready[4]}) /call Cast "${Me.Gem[4]}"
	/if ((${Me.SpellReady[Thaumatize Pet Rk. iII]}) && (${Me.Pet.ID}))  {
		/varset iTarget ${Target.ID}
		/call Cast "Thaumatize Pet Rk. IiI"
		/target id ${iTarget}
		/delay 20
		}
	/if (${Me.AltAbilityReady[Force of Elements]}) /call Cast "Force of Elements"
	/delay 1
	/if (${Cast.Ready[3]}) /call Cast "${Me.Gem[3]}"
	/delay 1
	/if (${Cast.Ready[2]}) /call Cast "${Me.Gem[2]}"
	/delay 1
	/if (${Cast.Ready[1]}) /call Cast "${Me.Gem[1]}"
	/delay 1
	/if (${Cast.Ready[8]}) /call Cast "${Me.Gem[8]}"
	/doevents
	/if (${Macro.Return.Equal[CAST_NOTARGET]}) {
		/echo Target gone, canceling full burn!
		/return
		}
	/if (${Macro.Return.Equal[CAST_OUTOFMANA]}) {
		/echo Tank empty, canceling full burn!
		/return
		}
	/goto :FBRStart
	/return

Sub Event_CancelFB
	/varset FBActive 0
	/echo Shutting down full burn!
	/return

sub Init_Mage

  /if (${Me.Class.ShortName.NotEqual[MAG]}) {
	/echo gimp_mage.inc not initialized
	/echo   You are not a powerful mage!
	/return
	}

  /declare AMSiniaType int Outer
  /declare AMPetType string Outer
  /declare AMNukePctSize int Outer
  /declare AMLastNuke int Outer
  /declare AMCancelMalo int Outer
  /declare AMNukeType string Outer
  /declare AMNukeTypeAlt string Outer
  /declare AMNukeTypeC string Outer
  /declare AMNukeTypeAltC string Outer
  /declare AMActive int Outer
  /declare AMPause int outer 0
  /declare AMAutoMageMed int Outer
  /declare AMRodRequests int Outer
  /varset AMRodRequests 0
  /declare sentpet int Outer
  /declare MaloCast int Outer
  /declare AMNukesLeft int outer
  /declare AMPetWeap string outer
  /declare FBActive int outer 1

  /squelch /alias /Automage /echo Toggle Automage
  /squelch /alias /MaloDone /echo Canceling Malosinia
  /squelch /alias /Newpet /echo Summon A Fresh Pet!
  /squelch /alias /SiniaMob /echo Malosinia type ${AMSiniaType}
  /squelch /alias /NukeSpeed /echo Automage set to nuke 
  /squelch /alias /NukeType /echo Automage spelltype set to 
  /squelch /alias /setpet /echo Set pet to type 
  /squelch /alias /rodrequests /echo Toggle Rod Requests
  /squelch /alias /castdummy /echo Cast Dummy Buff
  /squelch /alias /amcheck /echo Check current AM Settings
  /squelch /alias /fullBurn    /echo FullBurning
  /squelch /alias /fullpetburn /echo FullPetBurning
  /squelch /alias /fullraidburn /echo FullRaidBurning
  /squelch /alias /Cancelfb /echo Canceling FB

|
|
|	Setting defaults (will work on INI based defaults in version 2)
|
|
|


  /varset AMActive 0
  /nukespeed medium
  /doevents  
  /nuketype fickle
  /doevents  
  /setpet water
  /doevents
  /call manacheck
  /call SetMageSpells
  /echo gimp_mage.inc has been initialized  
  /return


|
|  This summons a new pet if it dies (or if you do)
|
| ------------- Startup new pet after rez --------------
| 1) Click rod (checks rod for existance, if not casts it and inventories it)
| 2) Summon Pet
| 3) Click Mana regen buffs
| 4) wait 30seconds
| 5) Cast Pet Toys
| 6) Give toys to pet


Sub Event_RezNewPet
  /declare PetType int local
  /call ManaCheck
	
	:RezPetManaCheck
	/if (${Me.PctMana}<4) {
		/if (!${Me.Sitting}) /sit
		/goto :RezPetManaCheck
		}	
	/if (${InvSlot[30].Item.ID}) {
		/if (${InvSlot[30].Item.Container}) {
			/echo Pack Slot 8 Contains A Container.  Please deal with Container before summoning Pet.
			/return
		} else {
			/echo Putting stray Item in top Slot 8 into A bag.
  		/shift /itemnotify ${InvSlot[30]} leftmouseup
  		/delay 12
			/call FindEmptyBagSlot
		  /shift /itemnotify ${InvSlot[${Macro.Return}]} leftmouseup
		  /delay 12
		}
	}
	/if (${Me.Pet.ID}) {
		/echo Already have A Pet.
		/return
	}
  
  /varcalc PetType ${Select[${AMPetType},Water,Air,Earth,Fire]}

  	/if (${PetType}==1) {
	  /call Cast "${MSCurPetWater}" gem10
	  | /delay 110
	  }

  	/if (${PetType}==2) {
	  /call Cast "${MSCurPetAir}" gem10
	  | /delay 110
	  }

  	/if (${PetType}==3) {
	  /call Cast "${MSCurPetEarth}" gem10
	  | /delay 110
	  }

  	/if (${PetType}==4) {
	  /call Cast "${MSCurPetFire}" gem10
	  | /delay 110
	  }
	  
	  /delay 15
	  /if (!${Me.Pet.ID}) /goto :RezPetManaCheck
	
  /pet ghold    
  /pet focus
  /call Cast ${MageConvert} gem10
  /delay 55
  /doEvents
  /call EquipPet
  /call Cast "Draconic Servant's Wand" item
	/echo Putting stray Item in top Slot 8 into A bag.
	/shift /itemnotify ${InvSlot[30]} leftmouseup
	/delay 12
	/call FindEmptyBagSlot
  /shift /itemnotify ${InvSlot[${Macro.Return}]} leftmouseup
  /delay 12  
  /echo A fresh pet of type ${AMPetType} is complete.
	/return

|
|   Set the approriate pet type (only works 65 to 70 now, will need revamp for more pet selections)
|


Sub Event_SetPet(junk, PetType)
	/varcalc PetType ${Select[${PetType},Water,Air,Earth,Fire]}

  	/if (${PetType}==1) {
	  /varset AMPetType Water
	  /varset AMPetWeap Summoned: Winterbane
	  /return
	  }

  	/if (${PetType}==2) {
	  /varset AMPetType Air
	  /varset AMPetWeap Summoned: Tonfa of the North Wind
	  /return
	  }

  	/if (${PetType}==3) {
	  /varset AMPetType Earth
	  /varset AMPetWeap Summoned: Jagged Ragesword
	  /return
	  }

  	/if (${PetType}==4) {
	  /varset AMPetType Fire
	  /varset AMPetWeap Summoned: Blazing Brand
	  /return
	  }

	/echo Invalid Pet type, not changing pet.
	/return 

|
|    Use Modrod
|


Sub ClickRodWithCheck	
  /if (${mcHUDmodAvail}==0) { 
  	/target myself
  	/delay 8
    /call Replenishing Rod!
    /call Cast "Wand of Ethereal Transvergence" 
    /delay 2 
    /autoinv 
    } 
  /call Cast "${sCurrentRod}" item
 
 
/return 

Sub EquipPet
  /declare sumBelt int local 0
  /declare sumWeap int local 0
  /declare sumMuzzle int local 0
  /declare summonedStep int local 0
  /declare itemname string local ""
  /Goto :EquipWeapStart
|
|  Summon the toys and inventory
|

  :EquipCycleArmor
	/call Cast "Summon Plate of the Prime" gem10
	/if (${Macro.Return.Equal[CAST_FIZZLE]}) /goto :EquipCycleArmor
	/delay 10
	/autoinv
	
	:InvArmorNext
	/autoinv
	/doevents
	/if (${FindItemCount[Prime Plate Helm]}==1) /goto :EquipArmorDone
	/goto :InvArmorNext

  :EquipArmorDone
|
|   Give toys to pet
|

  /target ${Me.Pet}
  /delay 4
  /varset itemname Prime Plate Helm
  /echo Giving '${itemname}' from Slot '${FindItem[=${itemname}].InvSlot}'
  /ctrl /itemnotify ${FindItem[=${itemname}].InvSlot} leftmouseup

| Open Trade window with pet 
:opentrade1
  /click left target
  /delay 4
  /if (!${Window[GiveWnd].Open}) /goto :opentrade1

  /varset itemname Prime Plate Vambraces
  /echo Giving '${itemname}' from Slot '${FindItem[=${itemname}].InvSlot}'
  /ctrl /itemnotify ${FindItem[=${itemname}].InvSlot} leftmouseup
  /notify GiveWnd GVW_MyItemSlot1 leftmouseup
  /delay 8 

  /varset itemname Prime Plate Gauntlets
  /echo Giving '${itemname}' from Slot '${FindItem[=${itemname}].InvSlot}'
  /ctrl /itemnotify ${FindItem[=${itemname}].InvSlot} leftmouseup
  /notify GiveWnd GVW_MyItemSlot2 leftmouseup
  /delay 8 

  /varset itemname Prime Plate Boots
  /echo Giving '${itemname}' from Slot '${FindItem[=${itemname}].InvSlot}'
  /ctrl /itemnotify ${FindItem[=${itemname}].InvSlot} leftmouseup
  /notify GiveWnd GVW_MyItemSlot3 leftmouseup

  /delay 8 
  /notify GiveWnd GVW_Give_Button leftmouseup
  /delay 20

|
|   Give toys to pet
|

  /target ${Me.Pet}
  /delay 4
  /varset itemname Prime Plate Bracers
  /echo Giving '${itemname}' from Slot '${FindItem[=${itemname}].InvSlot}'
  /ctrl /itemnotify ${FindItem[=${itemname}].InvSlot} leftmouseup

| Open Trade window with pet 
:opentrade2
  /click left target
  /delay 4
  /if (!${Window[GiveWnd].Open}) /goto :opentrade2

  /varset itemname Prime Plate Breastplate
  /echo Giving '${itemname}' from Slot '${FindItem[=${itemname}].InvSlot}'
  /ctrl /itemnotify ${FindItem[=${itemname}].InvSlot} leftmouseup
  /notify GiveWnd GVW_MyItemSlot1 leftmouseup
  /delay 8 

  /varset itemname Prime Plate Greaves
  /echo Giving '${itemname}' from Slot '${FindItem[=${itemname}].InvSlot}'
  /ctrl /itemnotify ${FindItem[=${itemname}].InvSlot} leftmouseup
  /notify GiveWnd GVW_MyItemSlot2 leftmouseup
  /delay 8 

  /varset itemname Summoned: Prime Belt
  /echo Giving '${itemname}' from Slot '${FindItem[=${itemname}].InvSlot}'
  /ctrl /itemnotify ${FindItem[=${itemname}].InvSlot} leftmouseup
  /notify GiveWnd GVW_MyItemSlot3 leftmouseup
  /delay 8 
  /notify GiveWnd GVW_Give_Button leftmouseup
	
	/delay 20
	
	  
  /varset itemname Phantom Satchel
  /echo Destroying '${itemname}' from Slot '${FindItem[=${itemname}].InvSlot}'
  /ctrl /itemnotify ${FindItem[=${itemname}].InvSlot} leftmouseup 
	/destroy
	
	
	:EquipWeapStart	
	/call Cast "Summon Ethereal Armaments" gem10
	/if (${Macro.Return.Equal[CAST_FIZZLE]}) /goto :EquipWeapStart
	/delay 10
	/autoinv
	
	:InvWeapNext
	/autoinv
	/doevents
	/if (${FindItemCount[Summoned: Winterbane]}>0) /goto :EquipWeapDone
	/goto :InvWeapNext
	:EquipWeapDone

| 
| End of Armaments Summoning
| 
	
	:EquipMuzzleStart	
	/call Cast "Summon Muzzle of Mowcha" gem10
	/if (${Macro.Return.Equal[CAST_FIZZLE]}) /goto :EquipMuzzleStart
	/delay 10
	/autoinv
	
	:InvMuzzleNext
	/autoinv
	/doevents
	/if (${FindItemCount[Summoned: Muzzle of Mowcha]}>0) /goto :EquipMuzzleDone
	/goto :InvMuzzleNext
	:EquipMuzzleDone

| 
| End of Summon Muzzle
| 	

  /keypress OPEN_INV_BAGS
  /target ${Me.Pet}
  /delay 4
  /varset itemname Summoned: Muzzle of Mowcha
  /echo Giving '${itemname}' from Slot '${FindItem[=${itemname}].InvSlot}'
  /ctrl /itemnotify ${FindItem[=${itemname}].InvSlot} leftmouseup

| Open Trade window with pet 
:opentrade3
  /keypress OPEN_INV_BAGS
  /click left target
  /delay 4
  /if (!${Window[GiveWnd].Open}) /goto :opentrade3
  
  /varset itemname ${AMPetWeap}
  /echo Giving '${itemname}' from Slot '${FindItem[=${itemname}].InvSlot}'
  /ctrl /itemnotify ${FindItem[=${itemname}].InvSlot} leftmouseup
  /notify GiveWnd GVW_MyItemSlot1 leftmouseup
  /delay 8 

  /varset itemname ${AMPetWeap}
  /echo Giving '${itemname}' from Slot '${FindItem[=${itemname}].InvSlot}'
  /ctrl /itemnotify ${FindItem[=${itemname}].InvSlot} leftmouseup
  /notify GiveWnd GVW_MyItemSlot2 leftmouseup
  /delay 8 
  /notify GiveWnd GVW_Give_Button leftmouseup
	
	/delay 20
	  
  /varset itemname Phantom Satchel
  /echo Destroying '${itemname}' from Slot '${FindItem[=${itemname}].InvSlot}'
  /ctrl /itemnotify ${FindItem[=${itemname}].InvSlot} leftmouseup 
	/destroy

  /keypress CLOSE_INV_BAGS
  /return




|Sub WaitCursor2(slot, pack) 
|   /declare item local 
|   /declare timeout timer 5 
|   /varset item ${Cursor.ID} 
|   /if (${Defined[pack]}) /goto :waitloop2
|   :waitloop1
|      /echo Waitloop1 ${slot} ${pack}
|      /nomod /itemnotify ${slot} leftmouseup 
|      /if ((${item} == ${Cursor.ID}) && (${timeout} > 0)) /goto :waitloop1 
|      /goto :donewaiting 
|   :waitloop2
|      |/Echo Waitloop2 ${slot} ${pack}
|      /nomod /itemnotify in ${pack} ${slot} leftmouseup 
|      /if ((${item} == ${Cursor.ID}) && (${timeout} > 0)) /goto :waitloop2 
|   :donewaiting 
|/return 

| Sub GetItem(item, equipslot) 
|    /declare inventoryslot int local 
|    /declare pack local 
|    /declare packslot int local 
|    /declare leaveopen int local 0 
|    /declare packnumber int local 0 
|    /declare leavetwisting int local 0 
| 
|    /varset inventoryslot ${FindItem[${item}].InvSlot} 
|    /if (!${inventoryslot}) /goto :notfound
|  	 /if (${inventoryslot}==29) /echo Found IT '${FindItem[${item}].Name}' in toplevel Slot
|  
|    :packcountloop
|    /if (${inventoryslot}>250 && ${inventoryslot}<331) { 
|       /varset packnumber ${Math.Calc[${packnumber}+1]} 
|       /varset inventoryslot ${Math.Calc[${inventoryslot}-10]} 
|       /goto :packcountloop 
|       } 
|    /if (${packnumber} > 0) { 
|       /varset pack pack${packnumber} 
|       /varset packslot ${Math.Calc[${inventoryslot}-240]} 
|       /goto :foundit 
|       } 
| 
|    :notinpack 
|       /goto :end 
| 
|    :foundit 
|       /echo ${pack} ${packslot} 
|       /echo Found IT '${FindItem[${item}].Name}'
|       /if (${inventoryslot}>21 && ${inventoryslot}<30) {
| 	      /call WaitCursor2 ${inventoryslot}
|          } else {
|       	/if (!${Window[${pack}].Open}) { 
|         	 /nomod /itemnotify ${pack} rightmouseup 
|          	/varset leaveopen 0 
|          	}         	
| 	      /call WaitCursor2 ${packslot} ${pack} 
|         } 
| 
|       /goto :end 
|    :notfound 
|       /echo ${item} not found in inventory. 
|    :end
|    /if (${leaveopen} == 0) /nomod /itemnotify ${pack} rightmouseup
| /return



Sub Event_MaloTarget(Junk, maloType)
| 1 - group
| 2 - raid (non boss)
| 3 - raid (boss)
| 4 - SWAP (non boss)
| 5 - SWAP (boss)
|
| Need to change to use /select logic.  Should be much cleaner then.  
| Will add a 3 strike and your out for automage portion
|
|
  
/declare MaloCount int local 0
/declare MaloReturn str local
/declare MaloSelect int local
/varset AMCancelMalo 0

:beginmalo

  /doevents

|  Exit clause just in case its getting ridiculous

  /if (${AMCancelMalo}==1) {
	/echo Unable to Malosinia Mob and shutting down attempt
	/return
	}

  /if (${LineOfSight[${Me.Y},${Me.X},${Me.Z}:${Target.Y},${Target.X},${Target.Z}]} != TRUE) /goto :beginmalo

  /echo Try ${MaloCount} for type ${AMSiniaType} on ${Target.CleanName}
  /varcalc MaloCount ${MaloCount}+1

|
|	Spell Handling
|
:OutOfRange
  /if (${AMCancelMalo}==1) {
	/echo Unable to Malosinia Mob and shutting down attempt
	/return
	}

	|  No notifications
	/if (${AMSiniaType}==0) {
		/call Cast "${MSMalo}" 
		/varset MaloReturn ${Macro.Return}
		/if (${MaloReturn.Equal[CAST_OUTOFRANGE]}) goto :OutOfRange
		}

	|  Group only
	/if (${AMSiniaType}==1) {
		/call Cast "Veil of Lost Hopes" item
		/varset MaloReturn ${Macro.Return}
		}

	|  Raid (Debuff Channel only)
	/if (${AMSiniaType}==2) {
		/call Cast "Veil of Lost Hopes" item
		/varset MaloReturn ${Macro.Return}
		}

	|  Raid (Raid Channel only)
	/if (${AMSiniaType}==3) {
		/call Cast "Veil of Lost Hopes" item
		/varset MaloReturn ${Macro.Return}
		}

	|  Raid (Debuff Channel only)
	/if (${AMSiniaType}==4) {
		/call swapcast "Veil of Lost Hopes" face
		/varset MaloReturn ${Macro.Return}
		}

	|  Raid (Raid Channel only)
	/if (${AMSiniaType}==5) {
		/call swapcast "Veil of Lost Hopes" face
		/varset MaloReturn ${Macro.Return}
		}

	/if (${AMSiniaType}==6) {
		/call Cast "Malosinatia" 
		/varset MaloReturn ${Macro.Return}
		}

|----------------------+----------------------------------------------------------------------+ 
| CAST_CANCELLED       | Spell was cancelled by ducking (either manually or because mob died) | 
| CAST_CANNOTSEE       | You can't see your target                                            | 
| CAST_IMMUNE          | Target is immune to this spell                                       | 
| CAST_INTERRUPTED     | Casting was interrupted and exceeded the given time limit            | 
| CAST_INVIS           | You were invis, and noInvis is set to true                           | 
| CAST_NOTARGET        | You don't have a target selected for this spell                      | 
| CAST_NOTMEMMED       | Spell is not memmed and you gem to mem was not specified             | 
| CAST_NOTREADY        | AA ability or spell is not ready yet                                 | 
| CAST_OUTOFMANA       | You don't have enough mana for this spell!                           | 
| CAST_OUTOFRANGE      | Target is out of range                                               | 
| CAST_RESISTED        | Your spell was resisted!                                             | 
| CAST_SUCCESS         | Your spell was cast successfully! (yay)                              | 
| CAST_UNKNOWNSPELL    | Spell/Item/Ability was not found                                     | 
|----------------------+----------------------------------------------------------------------+ 

	/varcalc MaloSelect ${Select[${MaloReturn},CAST_SUCCESS,CAST_RESISTED,CAST_CANCELLED,CAST_INTERRUPTED]}
	/if (${MaloSelect}==1) /goto :malosuccess
	/if (${MaloSelect}==2) /goto :beginmalo
	/if (${MaloSelect}==3) /varset AMCancelMalo 1
	/if (${MaloSelect}==4) /goto :beginmalo
	/echo Cast of Malosinia returned ${MaloReturn}
	/goto :beginmalo

|
|	 Notifications for Automage Malosinia
|

:malosuccess

  /if (${AMSiniaType}==1) /g Malosinia succeeded on ${Target.CleanName} after ${MaloCount} trie(s)
  /if (${AMSiniaType}==2) /g Malosinia succeeded on ${Target.CleanName} after ${MaloCount} trie(s)
  /if (${AMSiniaType}==2) /5 Malosinia succeeded on ${Target.CleanName} after ${MaloCount} trie(s)
  /if (${AMSiniaType}==3) /rs Malosinia succeeded on ${Target.CleanName} after ${MaloCount} trie(s)
  |/if (${AMSiniaType}==4) /g Malosinia succeeded on ${Target.CleanName} after ${MaloCount} trie(s)
  |/if (${AMSiniaType}==4) /5 Malosinia succeeded on ${Target.CleanName} after ${MaloCount} trie(s)
  /if (${AMSiniaType}==5) /rs Malosinia succeeded on ${Target.CleanName} after ${MaloCount} trie(s)
  /echo Malosinia succeeded on ${Target.CleanName} after ${MaloCount} trie(s)
  /return

Sub Event_CancelMalo
  /varset AMCancelMalo 1
  /return


|  Automage functions
|  This is not intended to run 100% of the time,  Please watch your mage and use the appropriate control commands
|
|
|
|
|
|

Sub Event_AutoMage
	/echo AMActive is ${AMActive}
	/if (${AMActive}==1) {
		/varset AMActive 0
		} else {
		/varset AMActive 1
		}
	/if (${AMActive}==1) /call StartMage
	/echo Automage is shut down now.
	/return

Sub Event_PauseMage
	/echo AMPause is ${AMPause}
	/if (${AMPause}==1) {
		/varset AMPause 0
		/echo Automage no longer paused.
		} else {
		/varset AMPause 1
		/echo Automage is paused.
		}
	/return

Sub StartMage

| Variables
	/echo Assisting ${mcGlobalMA} 
	/declare strSpellTemp str
	/varset sentpet 0 
	/varset MaloCast 0 


	/echo ${mcGlobalMA} is the main tank and I'll be casting ${AMNukeType} 
	

	:MainLoop
		/call DoRegularUpdates
		/call doclickies
		/call BuffCheck 
	
		/if (${Bool[${Me.Gem[1]}]}) {
			/if (!${AMNukeType.Equal[${Me.Gem[1]}]}) {
				/memspell 1 ${AMNukeType}
				/delay 2s
				}
			} else if {
				/memspell 1 ${AMNukeType}
				/delay 2s
			}
		/if (${Bool[${Me.Gem[2]}]}) {
			/if (!${AMNukeTypeAlt.Equal[${Me.Gem[2]}]}) {
				/memspell 2 ${AMNukeTypeAlt}
				/delay 2s
				}
			} else if {
				/memspell 2 ${AMNukeTypeAlt}
				/delay 2s
			}

		/if (${AMPause}==1) /goto :MainLoop
		/call ManaCheck 		
		/delay 5
		/doevents 
		/assist ${mcGlobalMA}
		/delay 5
		/if (${AMActive}==0) {
			/echo Shutting down Automage functions, Returning to user control
			/call Event_exp
			/return
			}
		/if (!${Me.Pet.ID}) {
			/automage
			}


		/if (!${Target.Type.Equal[NPC]}) /goto :MainLoop

		/if (${Target.ID} && ${Target.PctHPs}<=97 && ${Target.Distance} <=65) { 
			/if (${Target.Type.Equal[NPC]}) { 
				/if (${sentpet}==0) { 
					/pet attack
					/if (${Me.AltAbilityReady[Frenzied Burnout]}) {
						/call Cast "Frenzied Burnout" alt
						} else /if (${Me.AltAbilityReady[Servant of Ro]}) {
						/call Cast "Servant of Ro" alt
						} else /if (${Me.AltAbilityReady[Host of the Elements]}) {
						/call Cast "Host of the Elements" alt
						} else /if (${Me.AltAbilityReady[Focus of Arcanum]}) {
						/call Cast "Focus of Arcanum" alt
						} else /if (${Me.AltAbilityReady[Fire Core]}) {
						/call Cast "Fire Core" alt
						} else /if (${FindItem[Glyphwielder's Tunic of the Summoner].Timer}==0) {
						/call Cast "Glyphwielder's Tunic of the Summoner" Item
						} else /if (${Me.AltAbilityReady[Fundament: First Spire of the Elements]}) {
						/call Cast "Fundament: First Spire of the Elements" alt
						} else /if (${FindItem[Tainted Forcestrike Silk Robe].Timer}==0) {
						/call Cast "Tainted Forcestrike Silk Robe" Item
						} else /if (${FindItem[Summoned: Elemental Ice Fragment].Timer}==0) {
						/call Cast "Summoned: Elemental Ice Fragment" Item
						
						/delay 8
						/assistma
						}
					/varset sentpet 1
					/1 Mage is sending %m at %t
					/echo Pet Sent
					/varcalc AMLastNuke 100\${AMNukePctSize}
					/stand off
					/face
					/delay 5
					/call Cast "Companion's Relocation" alt
					} 

		/if (${MaloCast}==0) {
			/varset AMSiniaType 0 
			/Siniamob
			/doevents
			/varset MaloCast 1
			/varcalc AMLastNuke 100\${AMNukePctSize}
			} 
		} 
		} 

|	Begin Nuking procedure
|	Nukes are done in equal paces.
|	Light=33%
|	Medium=25%
|	Heavy=20%
|	Chain=1% (or any time you can)
|
|	Mage will auto med at 15% and then med to 90% before nuking again
|	
		/if (${AMAutoMageMed}==1) {
			|
			| Medding mage (Sends pet, debuffs, thats it)
			|
			/if (${Me.Sitting} == FALSE) /sit
			/if (${Me.SpellReady[${MSPetMana}]})  /call Cast "${MSPetMana}"
			/doevents

			} else {
			|
			| Non Medding mage (Sends pet, debuffs and nukes)
			|
			/if (${Target.PctHPs}<98 && ${MaloCast}==1) {
				|
				|	Cast damage clickies
				|
				/if (${Me.AltAbilityReady[Force of Elements]}) /call Cast "Force of Elements" alt
				/if (${FindItem[Mark of the Last Wing].Timer}==0) /call Cast "Mark of the Last Wing" Item
				/if (${FindItem[Dagger of Evil Summons].Timer}==0) /call Cast "Dagger of Evil Summons" Item
				/if (${Me.SpellReady[${MSPetMana}]})  /call Cast "${MSPetMana}"
				|
				|	Twincast if its up
				|
|				/if (${Cast.Ready[${MSTwin}]}) /call Cast "${MSTwin}"


				/varcalc AMNukesLeft ${Target.PctHPs}\${AMNukePctSize}
				/if (${AMNukesLeft}<${AMLastNuke}) {
					/varcalc AMLastNuke ${Target.PctHPs}\${AMNukePctSize}
					/if (${Cast.Ready[${AMNukeType}]}) {
						/call Cast "${AMNukeType}" 
						} else {
						/call Cast "${AMNukeTypeAlt}" 
						}
					}
				}

			}
			/doevents 

	/goto :MainLoop 

/return 


Sub Event_exp
	|
	|  Will find that in multiple kill situations, mage will cast less.  (have not
	|  thought through the logic for this yet.  Likely will need to use hidden target
	|  Name to verify that my target is actually dead.

	/if (${AMActive}==0) /return

	/varset sentpet 0 
	/varset MaloCast 0
	/varset AMCancelMalo 1
	/varcalc AMLastNuke 100\${AMNukePctSize}

	/echo Mob is dead!!! Resetting Variables!! 

	/if (${Me.Sitting} == FALSE) {
		/delay ${Math.Rand[4]}s
		/call cast "Perfected Invisibilty" alt
		/sit
		}
	/return 

|----------------------------------------------------------------------------------- 
|VERY IMPORTANT INFORMATION!! READ SLOWLY AND CAREFULLY!! 
| 
|I have this portion of the macro set to cast malosinia from 
|my face slot item.. NOTE: if you do NOT have the time veil 
|you will need to change the line /call Cast "Veil of Lost Hopes" item to 
|/call Cast "Malosinia" 
|----------------------------------------------------------------------------------- 


Sub ManaCheck 
	/if (${Me.PctMana}<=86 && ${mcHUDmodAvail}==1) { 
		/call ClickRodWithCheck 
		}
	/if (${Me.PctMana}<=60 && (${Cast.Ready[${MSHarvest}]})) { 
		/call cast "${MSHarvest}"
		}
	/IF (${Me.PctMana}<15) /varset AMAutoMageMed 1
	/if (${Me.PctMana}>90 && ${AMAutoMageMed}==1) /varset AMAutoMageMed 0
	/return 

|
|	Avoiding Enrage
|

Sub Event_rageon 
|	/if (${AMActive}==0) /return
	/echo ${Target.CleanName} is enraged. Backing pet off. 
	/pet back off 
	/delay 2 

	/return 
|
|	Re-engaging pet after enrage
|

Sub Event_rageoff 
	/if (${AMActive}==0) /return
	/doevents flush 
	/echo No longer worried about rage from ${Target.CleanName}
	/pet attack 
	/return 

|----------------------------------------------------------------------------------- 
|VERY IMPORTANT INFORMATION!! READ SLOWLY AND CAREFULLY!! 
|
|NOTE: if you do NOT have elemental legs 
|you will need to change the line /call Cast "Magi`kot's Pants of Convergence" item to 
|/call Cast "Rod of Mystical Transvergence" 
|this is located at the BOTTEM of Sub Buffcheck just below 
|----------------------------------------------------------------------------------- 


Sub BuffCheck
	/declare iLp int inner 0
	/declare iNec int inner 0

	/for iLp 0 to ${Group}
		/if (${Group.Member[${iLp}].Class.ShortName.Equal[NEC]}) /varset iNec 1
	/next iLp

	|/call AmIBusy
	/if (${Macro.Return}==0) /return

	/if ((!${Krust.Buff[mageconvert]}) && (${iNec}==0)) { 
		/echo Casting Ethereal Symbiosis 
	 	/call Cast "${MSMageCvt}" gem10
		/delay 5 
		/doevents
		} 
		
	/if ((!${Krust.Buff[armor]}) && (!${Krust.Buff[focus]})) { 
		/echo Casting Shield of the Void 
		/call Cast "${MSShield}" gem10
		} 
	/doevents 
	/if (!${Krust.Buff[skin]} && !${Krust.Buff[mageregen]}) { 
		/echo Casting Empyrean Guardian
		/call Cast "${MSManaRegen}" gem10
		/delay 5 
		/doevents I
		} 
	/if (!${Me.Buff[${MSDefPet}].ID}) { 
		/call Cast "${MSDefPet}"  gem10
		} 
	/if (!${Krust.Buff[DS]}) { 
		/call Cast "${MSGroupDS}"  gem10
		} 
	/if (${mcHUDmodrodCharges}==0) { 
		/echo Summoning a new Mod Rod!
		/target myself
		/call Cast "Wand of Spectral Transvergence" gem10
		/delay 3s
		/autoinv
		/delay 3s
		/autoinv
		} 
	/if (${FindItem[=Summoned: Elemental Ice Shard].InvSlot}<1) { 
		/echo Summoning a new ice fragment
		/call Cast "Summon Frigid Paradox Rk. III" gem3
		/delay 3s
		/autoinv
		/delay 3s
		/autoinv
		} 
	/return
 
|
|	Setting the spell for nuking for the automage
|


Sub Event_NukeSpell(Junk,NukeType)
	/declare NType int local
	/varcalc NType ${Select[${NukeType},fire,magic,bolt,fickle,summoned]}
	/if (${NType}==1) {
		/varset AMNukeType     ${MSFireQuick1}
		/varset AMNukeTypeAlt  ${MSFireQuick2}
		}
	/if (${NType}==2) {
		/varset AMNukeType	${MSFireShock1}
		/varset AMNukeTypeAlt	${MSFireShock2}
		}
	/if (${NType}==3) {
		/varset AMNukeType	${MSFireBolt1}
		/varset AMNukeTypeAlt	${MSFireBolt2}
		}
	/if (${NType}==4) {
		/varset AMNukeType	${MSFireFickle1}
		/varset AMNukeTypeAlt	${MSFireFickle2}
		}
	/if (${NType}==5) {
		/varset AMNukeType	${MSFireSumm1}
		/varset AMNukeTypeAlt	${MSFireSumm2}
		}
	/return 

|
|	Setting the amount of nuking for the automage
|

sub Event_NukeType(Junk,NukeType)
	/declare NType int local
	/varcalc NType ${Select[${NukeType},light,medium,heavy,super,chain]}

	/if (${NType}==1) {
		/varset AMNukePctSize 33
		/varcalc AMLastNuke 100\${AMNukePctSize}
		}
	/if (${NType}==2) {
		/varset AMNukePctSize 25
		/varcalc AMLastNuke 100\${AMNukePctSize}
		}
	/if (${NType}==3) {
		/varset AMNukePctSize 20
		/varcalc AMLastNuke 100\${AMNukePctSize}
		}
	/if (${NType}==4) {
		/varset AMNukePctSize 10
		/varcalc AMLastNuke 100\${AMNukePctSize}
		}
	/if (${NType}==5) {
		/varset AMNukePctSize 4
		/varcalc AMLastNuke 100\${AMNukePctSize}
		}

	/return



sub Event_RodTell(strMessage)
	/if (${AMRodRequests}==0) /return
        /if (${strMessage.Token[1, ].Equal[[MQ2]]}) 		/return		|dont need A recursive trigger
        /if (${strMessage.Token[1, ].Equal[You]}) 			/return			|dont need A recursive trigger
        /if (${strMessage.Token[4, ].Equal[[guild,]]})	/return		|dont need A recursive trigger
        /if (${strMessage.Token[2, ].Equal[begins]}) 		/return  |dont need to trigger when another mage is in the Group and Casting with know spells Leader AA
        /echo ${strMessage.Token[1, ]}


	/declare strCurrentTarget string local
	/declare strRequestor string local


	/varset strCurrentTarget ${Target.Name}
        /varset strRequestor ${strMessage.Token[1, ]}

	/echo Old target was ${strCurrentTarget} and requestor is ${strRequestor}
	/target ${strRequestor}
	/delay 5
	/call Cast "Magi`kot's Pants of Convergence" item
	/tt Sending you a mod rod
	/delay 24
	/target ${strCurrentTarget}
	/return

Sub Event_RodRequests
	/if (${AMRodRequests}==1) {
		/varset AMRodRequests 0
		/echo ModRod requests are off
		} else {
		/varset AMRodRequests 1
		/echo ModRod requests are on
		}
	/return

Sub Event_DummyBuff
	/call Cast "Amulet of Necropotence" Item
	/call Cast "Fabled Shrunken Goblin Skull Earring" Item
	/return

Sub Event_AMCheck
	/echo MA is ${mcGlobalMA}
	/echo Nuke Type is ${AMNukeType}
	/echo Nuke Speed is ${AMNukePctSize}
	/return

Sub CheckMageBuffs
	  /if (${Me.Class.ShortName.NotEqual[MAG]}) {
		/return
		}

	/call AmIBusy

	/if (${Macro.Return}==0) {
		/if (${Me.Pet.Name.NotEqual[NULL]}) {
			/if (${Me.PetBuff[${MSPetRune}]}== ${NULL}) {
				/echo Refreshing Iceflame on ${Me.Pet.CleanName}
				/call Cast "${MSPetRune}" Gem10
				}
			/if (${Me.PetBuff[Primal Fusion]} == ${NULL}) {
				/if (${FindItem[=Focus of Primal Elements].Timer}==0) {
					/call Cast "Focus of Primal Elements" item
					/echo Refreshing Primal Fusion on ${Me.Pet.CleanName}
					}
				}
			/if (${Me.PetBuff[${MSPetHaste}]} == ${NULL}) {
				/echo Refreshing Burnout on ${Me.Pet.CleanName}
				/call Cast "${MSPetHaste}" Gem10
				}
			}
		}
	/return
	
Sub FindEmptyBagSlot
	/declare iSlot int local
	/declare iLastSlot int local
	

| Free Slot in Pack 1
	/echo Check Bag 1
	/if (${InvSlot[23].Item.Items} < ${InvSlot[23].Item.Container}) {
		/varcalc iLastSlot 261+${InvSlot[23].Item.Container}
		/for iSlot 262 to ${iLastSlot}
			/if (!${InvSlot[${iSlot}].Item.ID}) /return ${iSlot}
		/next iSlot
		}

| Free Slot in Pack 2
	/echo Check Bag 2
	/if (${InvSlot[24].Item.Items} < ${InvSlot[24].Item.Container}) {
		/varcalc iLastSlot 271+${InvSlot[24].Item.Container}
		/for iSlot 272 to ${iLastSlot}
			/if (!${InvSlot[${iSlot}].Item.ID}) /return ${iSlot}
		/next iSlot
		}

| Free Slot in Pack 3
	/echo Check Bag 3
	/if (${InvSlot[25].Item.Items} < ${InvSlot[25].Item.Container}) {
		/for iSlot 282 to ${iLastSlot}
			/echo Checking ${iSlot} for ${InvSlot[${iSlot}].Item.Name}
			/if (!${InvSlot[${iSlot}].Item.ID}) /return ${iSlot}
		/next iSlot
		}

| Free Slot in Pack 4
	/echo Check Bag 4
	/if (${InvSlot[26].Item.Items} < ${InvSlot[26].Item.Container}) {
		/varcalc iLastSlot 291+${InvSlot[26].Item.Container}
		/for iSlot 292 to ${iLastSlot}
			/if (!${InvSlot[${iSlot}].Item.ID}) /return ${iSlot}
		/next iSlot
		}

| Free Slot in Pack 5
	/echo Check Bag 5
	/if (${InvSlot[27].Item.Items} < ${InvSlot[27].Item.Container}) {
		/varcalc iLastSlot 301+${InvSlot[27].Item.Container}
		/for iSlot 302 to ${iLastSlot}
			/if (!${InvSlot[${iSlot}].Item.ID}) /return ${iSlot}
		/next iSlot
		}

| Free Slot in Pack 6
	/echo Check Bag 6
	/if (${InvSlot[28].Item.Items} < ${InvSlot[28].Item.Container}) {
		/varcalc iLastSlot 311+${InvSlot[28].Item.Container}
		/for iSlot 312 to ${iLastSlot}
			/if (!${InvSlot[${iSlot}].Item.ID}) /return ${iSlot}
		/next iSlot
		}

| Free Slot in Pack 7
	/echo Check Bag 7
	/if (${InvSlot[29].Item.Items} < ${InvSlot[29].Item.Container}) {
		/varcalc iLastSlot 321+${InvSlot[29].Item.Container}
		/for iSlot 322 to ${iLastSlot}
			/if (!${InvSlot[${iSlot}].Item.ID}) /return ${iSlot}
		/next iSlot
		}

	/return -1
		
| Audio text: You should duck
| Audio text: between his legs
| Audio text: far away as possible
| Audio text: directly at you
	
Sub Event_Fickle
	/popup  Double Fire Nuke BONUS!
	/return

Sub Event_PetStun(Junk,StunnedMob)
	/echo ${StunnedMob} STUNNED (pet)
	/return

Sub Event_QueenMalAttack(Junk,DiseasedPC)
	/echo ${DiseasedPC} is going to get diseased.
	| /RS Disease incoming to ${DiseasedPC}
	/return

#Event  QueenMalInfested "You are infested by hundreds of slithering maggots#*#"
Sub Event_QueenMalInfested
	/echo I am diseased.
	/popup I am diseased
	/return









