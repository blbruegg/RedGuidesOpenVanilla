| gimp_clickies.inc

#Event ToggleClickies		"[MQ2] clickies"
Sub Event_ToggleClickies
	/if (!${gimp_clickies_loaded}) /call Init_Clickies
	/varset gimp_clickies_activated ${If[${gimp_clickies_activated},0,1]}
	/echo gimp_clickies.inc is ${If[${gimp_clickies_activated},ACTIVATED,DEACTIVATED]}
/return

|Casts all important clickies to be ready for a boss fight

#Event Charge		"[MQ2] charge"
Sub Event_Charge
	/echo # Casting charge clickies	
	/if (${ManaregenBuffItem.Length}) /call Cast "${ManaregenBuffItem}" item
	/if (${ManapoolBuffItem.Length}) /call Cast "${ManapoolBuffItem}" item
	/if (${useClickyAttackBuff} && ${ATK1Item.Length} && (${Spell[${ATK1Item}].Stacks})) /call Cast "${ATK1Item}" item
	/if (${useClickyAttackBuff} && ${ATK3Item.Length} && (${Spell[${ATK3Item}].Stacks})) /call Cast "${ATK3Item}" item
	/if (${useClickyAttackBuff} && ${ATK5Item.Length} && (${Spell[${ATK5Item}].Stacks})) /call Cast "${ATK1Item}" item
	/if (${useClickyResists} && ${Resist1Item.Length} && (${Spell[${Resist1}].Stacks})) /call Cast "${Resist1Item}" item
	/if (${useClickyResists} && ${Resist3Item.Length} && (${Spell[${Resist3}].Stacks})) /call Cast "${Resist3Item}" item
	/if (${useClickyHaste} && ${HV1BuffItem.Length} && (${Spell[${HV1Buff}].Stacks})) /call Cast "${HV1BuffItem}" item
	/if (${useClickyHaste} && ${HV3BuffItem.Length} && (${Spell[${HV3Buff}].Stacks})) /call Cast "${HV3BuffItem}" item
	/if (${useClickyHPBuff} && ${HPBuffItem.Length} && (${Spell[${HPBuff}].Stacks})) /call Cast "${HPBuffItem}" item
	/if (${useClickyAC} && ${ACBuffItem.Length} && (${Spell[${ACBuff}].Stacks})) /call Cast "${ACBuffItem}" item
/return

#Event ShowClickies "[MQ2] showclickies"
sub Event_ShowClickies
	/varset iShow 1
	/varset ManapoolBuffVal		0
	/varset ManaregenBuffVal	0
	/varset DSBuff2Val		0
	/varset DSBuff4Val		0
	/varset Resist1Val		0
	/varset Resist3Val		0
	/varset HPBuffVal		0
	/varset ATK1Val			0
	/varset ATK3Val			0
	/varset ATK5Val			0
	/varset ACBuffVal		0	
	/varset HV1BuffVal		0	
	/varset HV3BuffVal		0			

	/call Init_Clickies
	/return

Sub Init_Clickies
	/declare iLoop						int inner 0
	/declare iLoop2						int inner 0
	/if (${iShow}==1) /goto :ReInit
	/declare gimp_clickies_loaded				int outer	1
	/declare gimp_clickies_activated			int outer	1
	/declare gimp_clickies_timer				timer outer	0
		
	|let user turn on the use of the following clickies from INI:
	/declare useClickyJboots				int	outer	${Ini[INI_FILE, ${Me.Name}, ClickyJboots, NOT_FOUND]}
	/declare useClickyBracerOfTheHidden			int	outer	${Ini[INI_FILE, ${Me.Name}, ClickyBracerOfTheHidden, NOT_FOUND]}
	/declare useClickyVoiceOfTheSerpent			int	outer	${Ini[INI_FILE, ${Me.Name}, ClickyVoiceOfTheSerpent, NOT_FOUND]}
	/declare useClickyRapierOfSomberNotes			int	outer	${Ini[INI_FILE, ${Me.Name}, ClickyRapierOfSomberNotes, NOT_FOUND]}
	/declare ClickyIllusionName				string	outer	${Ini[INI_FILE, ${Me.Name}, ClickyIllusion, NOT_FOUND]}
	/declare ClickyIllusionActive				int	outer	${Ini[INI_FILE, ${Me.Name}, ClickyIllusionActive, 1]}

	|new type of clicky handling, by type of clicky instead of by item name:
	/declare useClickyAttackBuff				int	outer	${Ini[INI_FILE, ${Me.Name}, useClickyAttack, NOT_FOUND]}
	/declare useClickyDS					int	outer	${Ini[INI_FILE, ${Me.Name}, useClickyDS, NOT_FOUND]}
	/declare useClickyResists				int	outer	${Ini[INI_FILE, ${Me.Name}, useClickyResists, NOT_FOUND]}
	/declare useClickyHPBuff				int	outer	${Ini[INI_FILE, ${Me.Name}, useClickyHP, NOT_FOUND]}
	/declare useClickyHaste					int	outer	${Ini[INI_FILE, ${Me.Name}, useClickyHaste, NOT_FOUND]}
	/declare useClickyAC					int	outer	${Ini[INI_FILE, ${Me.Name}, useClickyAC, NOT_FOUND]}
	

	/declare mcHUDmount					string	outer
	/declare mcHUDshrinker					string	outer

	/declare mcHUDsummonPullItem				string	outer


	/declare ManapoolBuff					string outer
	/declare ManapoolBuffVal				int outer 0
	/declare ManapoolBuffItem				string outer
	/declare ManaregenBuff					string outer
	/declare ManaregenBuffVal				int outer 0
	/declare ManaregenBuffItem				string outer
	/declare DSBuff2					string outer
	/declare DSBuff2Val					int outer 0
	/declare DSBuff2Item					string outer
	/declare DSBuff4					string outer
	/declare DSBuff4Val					int outer 0
	/declare DSBuff4Item					string outer
	/declare Resist1					string outer
	/declare Resist1Val					int outer 0
	/declare Resist1Item					string outer
	/declare Resist3					string outer
	/declare Resist3Val					int outer 0
	/declare Resist3Item					string outer
	/declare HPBuff						string outer
	/declare HPBuffVal					int outer 0
	/declare HPBuffItem					string outer
	/declare ATK1						string outer
	/declare ATK1Val					int outer 0
	/declare ATK1Item					string outer
	/declare ATK3						string outer
	/declare ATK3Val					int outer 0
	/declare ATK3Item					string outer
	/declare ATK5						string outer
	/declare ATK5Val					int outer 0
	/declare ATK5Item					string outer
	/declare ACBuff						string outer
	/declare ACBuffVal					int outer 0
	/declare ACBuffItem					string outer
	/declare HV1Buff					string outer
	/declare HV1BuffVal					int outer 0
	/declare HV1BuffItem					string outer
	/declare HV3Buff					string outer
	/declare HV3BuffVal					int outer 0
	/declare HV3BuffItem					string outer
	/declare iDone 						int outer 0
	/declare iShow 						int outer 0

	:ReInit
	/call FindMountClicky
	/varset mcHUDmount ${Macro.Return}
		
	/call FindShrinkClicky
	/varset mcHUDshrinker ${Macro.Return}

	/for iLoop 1 to 32
		| /echo Slot ${iLoop} : ${InvSlot[${iLoop}].Item} ${InvSlot[${iLoop}].Item.Spell} ${InvSlot[${iLoop}].Item.Spell.CastTime}
		/if (${Bool[${InvSlot[${iLoop}].Item.Spell}]})		/call FindClickies 0 ${iLoop} 
	/next iLoop

	/for iLoop 23 to 32
		/if (${Me.Inventory[${iLoop}].Container} >0) {
			/for iLoop2 1 to ${Me.Inventory[${iLoop}].Container}
				| Me.Inventory[packX].Item[N].Name
				| /echo Slot ${iLoop} : ${InvSlot[${iLoop}].Item} ${InvSlot[${iLoop}].Item.Spell} ${InvSlot[${iLoop}].Item.Spell.CastTime}
				/if (${Bool[${Me.Inventory[${iLoop}].Item[${iLoop2}].Spell}]})		/call FindClickies ${iLoop} ${iLoop2}

			/next iLoop2
			}
	/next iLoop
	/varset iShow 0


	|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
	|XXXX Adds aliases to the script
	|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

	/squelch /alias /shrinkgroup /echo shrinkgroup
	/squelch /alias /shrinkself /echo shrinkself
	/squelch /alias /makepulleritem /echo makepulleritem
	/squelch /alias /mount /echo clickmount
	/squelch /alias /illusion /echo illusion
	/squelch /alias /charge /echo charge

	/squelch /alias /unmount /dismount

	|Installs class specific aliases
	/squelch /alias /rez /echo rez

	|Installs command to toggle module ON/OFF
	/squelch /alias /clickies /echo clickies

	/echo gimp_clickies.inc initialized

	/if (${mcHUDshrinker.Length}) 			/echo * Shrinker: ${mcHUDshrinker}
	/if (${mcHUDmount.Length}) 			/echo * Mount: ${mcHUDmount}

	/if (${ManaregenBuffItem.Length})		/echo * Manaregen clicky: ${ManaregenBuffItem}
	/if (${ManapoolBuffItem.Length})			/echo * Mana pool clicky: ${ManapoolBuffItem}

	/if (${mcHUDsummonPullItem.Length})		/echo * Summon puller item: ${mcHUDsummonPullItem}
	/if (${ClickyIllusionName.Length}>1)		/echo * Illusion clicky: ${ClickyIllusionName} (spell ${FindItem[=${ClickyIllusionName}].Spell.Name})

	/if (${useClickyAttackBuff} && ${ATK1Item.Length}) /echo * Attack1 clicky: ${ATK1Item}
	/if (${useClickyAttackBuff} && ${ATK3Item.Length}) /echo * Attack3 clicky: ${ATK3Item}
	/if (${useClickyAttackBuff} && ${ATK5Item.Length}) /echo * Attack5 clicky: ${ATK5Item}

	/if (${useClickyDS} && ${DSBuff2Item.Length}) 	/echo * DS2 clicky: ${DSBuff2Item}
	/if (${useClickyDS} && ${DSBuff4Item.Length}) 	/echo * DS4 clicky: ${DSBuff4Item}

	/if (${useClickyResists} && ${Resist1Item.Length}) /echo * Resist clicky: ${Resist1Item}
	/if (${useClickyResists} && ${Resist3Item.Length}) /echo * Resist clicky: ${Resist3Item}

	/if (${useClickyHPBuff} && ${HPBuffItem.Length}) /echo * HP clicky: ${HPBuffItem}

	/if (${useClickyHaste} && ${H1VBuffItem.Length}) /echo * Haste clicky: ${H1VBuffItem}
	/if (${useClickyHaste} && ${H3VBuffItem.Length}) /echo * Overhaste clicky: ${H3VBuffItem}

	/if (${useClickyAC} && ${ACBuffItem.Length}) /echo * AC clicky: ${ACBuffItem}
/return

Sub DoClickies
	|Declarations of clickies:
	/if (!${gimp_clickies_loaded}) /call Init_Clickies
	/if (!${gimp_clickies_activated}) /return
	/varset iDone 0

	|Time to check if the player is busy with something, then cancel (chest item unequipped = assume you are naked)
	/if (${Me.Invis} || ${Me.Feigning} || ${Me.Moving} || ${Me.AFK} || ${Cursor.ID} || ${Me.Casting.ID} || ${Window[RespawnWnd].Open} || ${Window[SpellBookWnd].Open} || ${Window[BigBankWnd].Open} || ${Window[LootWnd].Open} || ${Window[TradeWnd].Open} || ${Window[MerchantWnd].Open} || ${Window[TradeskillWnd].Open} || ${Window[BarterWnd].Open} || !${InvSlot[chest].Item.ID}) /return

	/declare wasISitting	int local	0
	/if (${Me.Sitting}) /varset wasISitting 1

	| ********************************
	|
	|	Begin pre-revamp clickies
	|
	| ********************************



	|duration 100 = 10 minutes
	|fixme: make this a ini section?
	|/if (${Zone.Name.Equal["The Plane of Knowledge"]} || ${Zone.Name.Equal["The Plane of Tranquility"]} || ${Zone.Name.Equal["The Nexus"]} || ${Zone.Name.Equal["The Bazaar"]} || ${Zone.Name.Equal["Guild Lobby"]} || ${Zone.Name.Equal["Guild Hall"]}) /return

	/declare ZoneOutdoors int local ${Ini[ZONEINFO_FILE, ZoneInfo, ${Zone.ShortName}_Outdoors, -1]}
	/if (${ZoneOutdoors} == -1) {
		/echo # Zone ${Zone.ShortName} data missing in INI, saving new data
		/ini "ZONEINFO_FILE" "ZoneInfo" "${Zone.ShortName}_Outdoors" "???"

		|If i have a levitate buff on, zone allows levitate, right :)
		/if (${Me.Buff["Spirit of Eagle"].ID} || ${Me.Buff["Flight of Eagles"].ID} || ${Me.Buff["Dead Men Floating"].ID} || ${Me.Buff["Levitation"].ID}) {
			/ini "ZONEINFO_FILE" "ZoneInfo" "${Zone.ShortName}_Levitate" "1"
		} else {
			/ini "ZONEINFO_FILE" "ZoneInfo" "${Zone.ShortName}_Levitate" "???"
		}
	}

	|Only refreshes if zone is marked as outdoor zone in INI_FILE under ZoneInfo section
	/if (${useClickyJboots} && ${ZoneOutdoors} && (${Me.FreeBuffSlots} > 2)) {
		/call BuffedRunSpeed
		/if (${Macro.Return} == 0) {
			/if (${FindItem[=Journeyman's Boots].InvSlot} && ${Me.Buff["JourneymanBoots"].Duration} < 40) {
				/echo Refreshing jboots
				/call Cast "Journeyman's Boots" item
				/goto :DoneRefreshingClickies
			}
			/if (${FindItem[=Fabled Journeyman's Boots].InvSlot} && ${Me.Buff["Bootstrutter's Blessing"].Duration} < 40) {
				/echo Refreshing jboots
				/call Cast "Fabled Journeyman's Boots" item
				/goto :DoneRefreshingClickies
			}
		}
	}

	|XXXXXXXXXXXXXXXXXXXXXXXXX
	|XX below is more special clickies handling
	/if (${useClickyResists} && !${Me.Combat} && (!${Target.ID} || ${Target.ID} == ${Me.ID}) && (${Me.FreeBuffSlots} > 1)) {
		/if (${FindItem[=Bangle of Disease Warding].InvSlot} && !${Me.Buff["Talisman of the Tribunal"].ID} && !${Me.Buff["Talisman of Jasinth"].ID} && (${Me.Buff["Resist Disease"].Duration} < 20)) {
			/echo Refreshing Resist Disease
			/target myself
			/call Cast "Bangle of Disease Warding" item
			/goto :DoneRefreshingClickies
		}
	}

	/if (${useClickyBracerOfTheHidden} && !${Raid.Members} && (!${Target.ID} || ${Target.ID} == ${Me.ID})) {
		/if (${FindItem[=Bracer of the Hidden].InvSlot} && ${Me.Buff["See Invisible"].Duration} < 20) {
			/echo Refreshing See Invisible
			/target myself
			/call Cast "Bracer of the Hidden" item
			/goto :DoneRefreshingClickies
		}
	}

	/if ((${ClickyIllusionName.Length}>1) && ${ClickyIllusionActive} && !${Raid.Members} && (${Me.FreeBuffSlots} > 1)) {
		/if (${FindItem[=${ClickyIllusionName}].InvSlot} && ${Me.Buff["${FindItem[=${ClickyIllusionName}].Spell.Name}"].Duration} < 20) {
			/echo Refreshing illusion
			/call Cast "${ClickyIllusionName}" item
			/goto :DoneRefreshingClickies
		}
	}

	/if ((${ClickyIllusionName.Length}>1) && ${ClickyIllusionActive} && ${Raid.Members} && (${Me.FreeBuffSlots} > 1)) {
		/if (${FindItem[=${ClickyIllusionName}].InvSlot} && ${Me.Buff["${FindItem[=${ClickyIllusionName}].Spell.Name}"].Duration} < 20) {
			/echo Refreshing illusion
			/call Cast "${ClickyIllusionName}" item
			/goto :DoneRefreshingClickies
		}
	}

	/if (${useClickyVoiceOfTheSerpent}) {
		/if (${FindItem[=Voice of the Serpent].InvSlot} && (${Me.Buff["Resonance"].Duration} < 5)) {
			/echo Refreshing Voice of the Serpent clicky
			/call Cast "Voice of the Serpent" item
			/goto :DoneRefreshingClickies
		}
	}
	
	/if (${useClickyRapierOfSomberNotes}) {
		/if (${FindItem[=Rapier of Somber Notes].InvSlot} && (${Me.Buff["Symphony of Battle"].Duration} < 5)) {
			/echo Refreshing Rapier of Somber Notes clicky
			/call Cast "Rapier of Somber Notes" item
			/goto :DoneRefreshingClickies
		}
	}
	
	|Druid AA DS refresh
	/if (!${Raid.Members} && !${Me.Moving} && !${Target.ID} && (${Me.FreeBuffSlots} > 2) && ${Me.AltAbilityReady["Wrath of the Wild"]} && !${Me.Buff["Wrath of the Wild"].ID}) {
		/echo # Refreshing druid AA DS
		/target myself
		/if (${Me.Sitting}) {
			/stand
			/delay 5
		}
		/docommand /alt activate 170
		/delay 4s
	}

	| ********************************
	|
	|	Begin post-revamp clickies
	|
	| ********************************


	/if (!(${Me.FreeBuffSlots} > 1)) /goto :DoneRefreshingClickies
	| Manapool Buff needed
	/if ((!${Me.Buff[${ManapoolBuff}].ID}) && (${Spell[${ManapoolBuff}].Stacks}) )			/multiline ; /echo Casting ManapoolBuff:${ManapoolBuff} ; /call Cast "${ManapoolBuffItem}" item ; /varset iDone 1
	/if (${iDone}==1) /goto :DoneRefreshingClickies
	/if ((!${Me.Buff[${ManaregenBuff}].ID}) && (${Spell[${ManaregenBuff}].Stacks})) 		/multiline ; /echo Casting ManaregenBuff:${ManaregenBuff} ; /call Cast "${ManaregenBuffItem}" item ; /varset iDone 1
	/if (${iDone}==1) /goto :DoneRefreshingClickies
	/if ((!${Me.Buff[${DSBuff2}].ID}) && (${Spell[${DSBuff2}].Stacks}) && (${useClickyDS}))		/multiline ; /echo Casting DS Slot 2:${DSBuff2} ; /call Cast "${DSBuff2Item}" item ; /varset iDone 1
	/if (${iDone}==1) /goto :DoneRefreshingClickies
	/if ((!${Me.Buff[${DSBuff4}].ID}) && (${Spell[${DSBuff4}].Stacks}) && (${useClickyDS}))		/multiline ; /echo Casting DS Slot 4:${DSBuff4} ; /call Cast "${DSBuff4Item}" item ; /varset iDone 1
	/if (${iDone}==1) /goto :DoneRefreshingClickies
	/if ((!${Me.Buff[${Resist1}].ID}) && (${Spell[${Resist1}].Stacks}) && (${useClickyResists}))	/multiline ; /echo Casting Resist Slot 1:${Resist1} ; /call Cast "${Resist1Item}" item ; /varset iDone 1
	/if (${iDone}==1) /goto :DoneRefreshingClickies
	/if ((!${Me.Buff[${Resist3}].ID}) && (${Spell[${Resist3}].Stacks}) && (${useClickyResists}))	/multiline ; /echo Casting Resist Slot 3:${Resist3} ; /call Cast "${Resist3Item}" item ; /varset iDone 1
	/if (${iDone}==1) /goto :DoneRefreshingClickies
	/if ((!${Me.Buff[${HPBuff}].ID}) && (${Spell[${HPBuff}].Stacks}) && (${useClickyHPBuff})) 	/multiline ; /echo Casting HP Buff:${HPBuff} ; /call Cast "${HPBuffItem}" item ; /varset iDone 1
	/if (${iDone}==1) /goto :DoneRefreshingClickies
	/if ((!${Me.Buff[${ATK1}].ID}) && (${Spell[${ATK1}].Stacks}) && (${useClickyAttackBuff}))	/multiline ; /echo Casting ATK Slot 1:${ATK1} ; /call Cast "${ATK1Item}" item ; /varset iDone 1
	/if (${iDone}==1) /goto :DoneRefreshingClickies
	/if ((!${Me.Buff[${ATK3}].ID}) && (${Spell[${ATK3}].Stacks}) && (${useClickyAttackBuff}))	/multiline ; /echo Casting ATK Slot 3:${ATK3} ; /call Cast "${ATK3Item}" item ; /varset iDone 1
	/if (${iDone}==1) /goto :DoneRefreshingClickies
	/if ((!${Me.Buff[${ATK5}].ID}) && (${Spell[${ATK5}].Stacks}) && (${useClickyAttackBuff}))	/multiline ; /echo Casting ATK Slot 5:${ATK5} ; /call Cast "${ATK5Item}" item ; /varset iDone 1
	/if (${iDone}==1) /goto :DoneRefreshingClickies
	/if ((!${Me.Buff[${ACBuff}].ID}) && (${Spell[${ACBuff}].Stacks}) && (${useClickyAC})) 		/multiline ; /echo Casting AC Buff:${ACBuff} ; /call Cast "${ACBuffItem}" item ; /varset iDone 1
	/if (${iDone}==1) /goto :DoneRefreshingClickies
	/if ((!${Me.Buff[${HV1Buff}].ID}) && (${Spell[${HV1Buff}].Stacks}) && (${useClickyHaste})) 	/multiline ; /echo Casting Haste BUff:${HV1Buff} ; /call Cast "${HV1BuffItem}" item ; /varset iDone 1
	/if (${iDone}==1) /goto :DoneRefreshingClickies
	/if ((!${Me.Buff[${HV3Buff}].ID}) && (${Spell[${HV3Buff}].Stacks}) && (${useClickyHaste})) 	/multiline ; /echo Casting Overhaste BUff:${HV3Buff} ; /call Cast "${HV3BuffItem}" item ; /varset iDone 1
	

	|/echo Done refreshing clickies
	:DoneRefreshingClickies
	/varset gimp_clickies_timer 60s
	/if (${wasISitting} && !${Me.Sitting} && !${Me.Moving} && !${Me.Casting.ID}) /sit
/return


Sub FindMountClicky
	/declare itemfind string local
	
	/if (${FindItem[=Collapsable Roboboar].InvSlot})		/varset itemfind Collapsable Roboboar
	/if (${FindItem[=Giant Red Drum].InvSlot})		/varset itemfind Giant Red Drum
	/if (${FindItem[=Large Red Drum].InvSlot})		/varset itemfind Large Red Drum
	/if (${FindItem[=Red Drum].InvSlot})			/varset itemfind Red Drum
	/if (${FindItem[=Small Red Drum].InvSlot})		/varset itemfind Small Red Drum
	/if (${FindItem[=Giant Green Drum].InvSlot})	/varset itemfind Giant Green Drum
	/if (${FindItem[=Large Green Drum].InvSlot})	/varset itemfind Large Green Drum
	/if (${FindItem[=Green Drum].InvSlot})			/varset itemfind Green Drum
	/if (${FindItem[=Small Green Drum].InvSlot})	/varset itemfind Small Green Drum
	/if (${FindItem[=Glowing Black Drum].InvSlot})	/varset itemfind Glowing Black Drum
	/if (${FindItem[=Giant Black Drum].InvSlot})	/varset itemfind Giant Black Drum
	/if (${FindItem[=Large Black Drum].InvSlot})	/varset itemfind Large Black Drum
	/if (${FindItem[=Black Drum].InvSlot})			/varset itemfind Black Drum
	/if (${FindItem[=Small Black Drum].InvSlot})	/varset itemfind Small Black Drum
	/if (${FindItem[=Glowing White Drum].InvSlot})	/varset itemfind Glowing White Drum
	/if (${FindItem[=Giant White Drum].InvSlot})	/varset itemfind Giant White Drum
	/if (${FindItem[=Large White Drum].InvSlot})	/varset itemfind Large White Drum
	/if (${FindItem[=White Drum].InvSlot})			/varset itemfind White Drum
	/if (${FindItem[=Small White Drum].InvSlot})	/varset itemfind Small White Drum
	/if (${FindItem[=Black Chain Bridle].InvSlot})			/varset itemfind Black Chain Bridle
	/if (${FindItem[=Black Leather Bridle].InvSlot})		/varset itemfind Black Leather Bridle
	/if (${FindItem[=Black Ornate Chain Bridle].InvSlot})	/varset itemfind Black Ornate Chain Bridle
	/if (${FindItem[=Black Rope Bridle].InvSlot})			/varset itemfind Black Rope Bridle
	/if (${FindItem[=Black Silken Bridle].InvSlot})			/varset itemfind Black Silken Bridle
	/if (${FindItem[=Brown Chain Bridle].InvSlot})			/varset itemfind Brown Chain Bridle
	/if (${FindItem[=Brown Leather Bridle].InvSlot})		/varset itemfind Brown Leather Bridle
	/if (${FindItem[=Brown Ornate Chain Bridle].InvSlot})	/varset itemfind Brown Ornate Chain Bridle
	/if (${FindItem[=Brown Rope Bridle].InvSlot})			/varset itemfind Brown Rope Bridle
	/if (${FindItem[=Brown Silken Bridle].InvSlot})			/varset itemfind Brown Silken Bridle
	/if (${FindItem[=Tan Chain Bridle].InvSlot})			/varset itemfind Tan Chain Bridle
	/if (${FindItem[=Tan Leather Bridle].InvSlot})			/varset itemfind Tan Leather Bridle
	/if (${FindItem[=Tan Rope Bridle].InvSlot})				/varset itemfind Tan Rope Bridle
	/if (${FindItem[=Tan Silken Bridle].InvSlot})			/varset itemfind Tan Silken Bridle
	/if (${FindItem[=White Chain Bridle].InvSlot})			/varset itemfind  White Chain Bridle
	/if (${FindItem[=White Leather Bridle].InvSlot})		/varset itemfind  White Leather Bridle
	/if (${FindItem[=White Ornate Chain Bridle].InvSlot})	/varset itemfind  White Ornate Chain Bridle
	/if (${FindItem[=White Rope Bridle].InvSlot})			/varset itemfind  White Rope Bridle
	/if (${FindItem[=White Silken Bridle].InvSlot})			/varset itemfind  White Silken Bridle
	/if (${FindItem[=Black Chain Bridle and Barding].InvSlot})			/varset itemfind Black Chain Bridle and Barding
	/if (${FindItem[=Black Leather Bridle and Barding].InvSlot})		/varset itemfind Black Leather Bridle and Barding
	/if (${FindItem[=Black Ornate Chain Bridle and Barding].InvSlot})	/varset itemfind Black Ornate Chain Bridle and Barding
	/if (${FindItem[=Black Rope Bridle and Barding].InvSlot})			/varset itemfind Black Rope Bridle and Barding
	/if (${FindItem[=Black Silken Bridle and Barding].InvSlot})			/varset itemfind Black Silken Bridle and Barding
	/if (${FindItem[=Brown Chain Bridle and Barding].InvSlot})			/varset itemfind Brown Chain Bridle and Barding
	/if (${FindItem[=Brown Leather Bridle and Barding].InvSlot})		/varset itemfind Brown Leather Bridle and Barding
	/if (${FindItem[=Brown Ornate Chain Bridle and Barding].InvSlot})	/varset itemfind Brown Ornate Chain Bridle and Barding
	/if (${FindItem[=Brown Rope Bridle and Barding].InvSlot})			/varset itemfind Brown Rope Bridle and Barding
	/if (${FindItem[=Brown Silken Bridle and Barding].InvSlot})			/varset itemfind Brown Silken Bridle and Barding
	/if (${FindItem[=Tan Chain Bridle and Barding].InvSlot})			/varset itemfind Tan Chain Bridle and Barding
	/if (${FindItem[=Tan Leather Bridle and Barding].InvSlot})			/varset itemfind Tan Leather Bridle and Barding
	/if (${FindItem[=Tan Rope Bridle and Barding].InvSlot})				/varset itemfind Tan Rope Bridle and Barding
	/if (${FindItem[=Tan Silken Bridle and Barding].InvSlot})			/varset itemfind Tan Silken Bridle and Barding
	/if (${FindItem[=White Chain Bridle and Barding].InvSlot})			/varset itemfind  White Chain Bridle and Barding
	/if (${FindItem[=White Leather Bridle and Barding].InvSlot})		/varset itemfind  White Leather Bridle and Barding
	/if (${FindItem[=White Ornate Chain Bridle and Barding].InvSlot})	/varset itemfind  White Ornate Chain Bridle and Barding
	/if (${FindItem[=White Rope Bridle and Barding].InvSlot})			/varset itemfind  White Rope Bridle and Barding
	/if (${FindItem[=White Silken Bridle and Barding].InvSlot})			/varset itemfind  White Silken Bridle and Barding
/return ${itemfind}

Sub FindSummonPullItem
	/declare itemfind  string local
	/if (${FindItem[=A Dusty Soriz Worker Pouch].InvSlot})	/varset itemfind A Dusty Soriz Worker Pouch
	/if (${FindItem[=Bloody Ancille's Pouch].InvSlot})		/varset itemfind Bloody Ancille's Pouch
	/if (${FindItem[=Ton Po's Mystical Pouch].InvSlot})		/varset itemfind Ton Po's Mystical Pouch
/return ${itemfind}


Sub FindShrinkClicky
	/declare itemfind string local
	/if (${FindItem[=Ant's Potion].InvSlot})				/varset itemfind Ant's Potion
	/if (${FindItem[=5 Dose Ant's Potion].InvSlot})			/varset itemfind 5 Dose Ant's Potion
	/if (${FindItem[=10 Dose Ant's Potion].InvSlot})		/varset itemfind 10 Dose Ant's Potion
	/if (${FindItem[=Anizok's Minimizing Device].InvSlot})	/varset itemfind Anizok's Minimizing Device
	/if (${FindItem[=Bestial Sandals].InvSlot})				/varset itemfind Bestial Sandals
	/if (${FindItem[=Cobalt Bracer].InvSlot})				/varset itemfind Cobalt Bracer
	/if (${FindItem[=Wand of Imperceptibility].InvSlot})	/varset itemfind Wand of Imperceptibility
	/if (${FindItem[=Bracelet of the Shadow Hive].InvSlot})	/varset itemfind Bracelet of the Shadow Hive
	/if (${FindItem[=Earring of Diminutiveness].InvSlot})	/varset itemfind Earring of Diminutiveness
	/if (${item.Equal[NULL]}) /echo WARNING: Couldn't find a shrink clicky in inventory
/return ${itemfind}


#Event ClickMount				"[MQ2] clickmount"
Sub Event_ClickMount
	/if (!${mcHUDmount.Length}) /return
	/call Cast "${mcHUDmount}" item
/return

#Event RezClicky				"[MQ2] rez"
Sub Event_RezClicky
	/if (${Me.Class.Name.NotEqual["Cleric"]} || !${Target.ID} || ${Target.Type.NotEqual[Corpse]}) /return
	/echo Ressurecting ${Target.Name}
	/call Cast "Water Sprinkler of Nem Ankh" item
/return

#Event ShrinkSelf            "[MQ2] shrinkself"
Sub Event_ShrinkSelf
	/if (!${mcHUDshrinker.Length}) /return
	|ShrinkSelf code by Harden
	|Lets you Shrink yourself without trying to shrink everyone in group.
	|useage == /shrinkself
	
	/declare i int local 0
	/declare oldId int local 0
	
	/if (${Target.ID} && (${Target.ID} != ${Me.ID})) /varset oldId $/{Target.ID}

	/echo # Shrinking Self

	|Note: When you are max shrinked, your Height == 2.00
	/target myself
	:Shrink
	/if ((${Me.Height} > 1.50)) {
		/call Cast "${mcHUDshrinker}" item
		/goto :Shrink
	}
	/echo # Done Shrinking

	/if (${oldId}) /target id ${oldId}
/return

#Event ShrinkGroup			"[MQ2] shrinkgroup"
Sub Event_ShrinkGroup
	/if (!${mcHUDshrinker.Length}) /return
	|Note: Not all shrink items work on targets
	|Bracelet of Shadow hive does. Wand of Impercibility doesnt.
	/declare i int local 0

	/echo Shrinking group
	/for i 0 to 5
		:Shrink
		/if ((${Group.Member[${i}].Height} > 2.00) && ${Group.Member[${i}].Type.Equal["PC"]}) {
			/if ((${Group.Member[${i}].Distance} < 100) && ${Group.Member[${i}].LineOfSight}) {
				|Skip MNK/BRD/SHD for pullage, and RNG for LoS problems
				/if (${Group.Member[${i}].Class.Name.NotEqual["Monk"]} && ${Group.Member[${i}].Class.Name.NotEqual["Bard"]} && ${Group.Member[${i}].Class.Name.NotEqual["Ranger"]}) {
					/target pc ${Group.Member[${i}]}
					/echo Shrinking ${Group.Member[${i}]} from ${Group.Member[${i}].Height}
					/call Cast "${mcHUDshrinker}" item
					/goto :Shrink
				} else {
					/echo Skipping ${Group.Member[${i}]} (${Group.Member[${i}].Class.ShortName})
				}
			} else {
				/echo Skipping ${Group.Member[${i}]}, OOR, size ${Group.Member[${i}].Height}
			}
		}
	/next i

	/echo Done shrinking
/return

#Event ClickyIllusion "[MQ2] illusion#1#"
Sub Event_ClickyIllusion(string Line, string name)
	/if (${name.Left[1].Equal[" "]}) /varset name ${name.Right[-1]}
	/if (${name.Equal[aon]}) /varset name Amulet of Necropotence
	/if (${name.Equal[woodelf]}) /varset name Crown of Deceit

	/if (${name.Length}) {
		|If i have item in inventory, and item has a spell click effect, then set it

		/if (${FindItem[=${name}].Spell.ID}) {
			/echo # Saving illusion item  ${FindItem[=${name}].Name} (spell ${FindItem[=${name}].Spell.Name}) to INI
			/ini "INI_FILE" "${Me.Name}" "ClickyIllusion" "${FindItem[=${name}].Name}"
			/varset ClickyIllusionName ${FindItem[=${name}].Name}
			/call Cast "${ClickyIllusionName}" item
		} else {
			/if (${FindItem[=${name}].ID} {
				/echo # /illusion error: The item ${name} is not a illusion clicky
			} else {
				/echo # /illusion error: The item ${name} was not found in your inventory
			}
		}
	} else {
		|Toggles casting of illusion clicky ON/OFF

		/if (${ClickyIllusionActive}) {
			/echo # Disabling illusion clicky
			/varset ClickyIllusionActive 0
			/ini "INI_FILE" "${Me.Name}" "ClickyIllusionActive" "0"
		} else {
			/echo # Enabling illusion clicky
			/varset ClickyIllusionActive 1
			/ini "INI_FILE" "${Me.Name}" "ClickyIllusionActive" "1"
			/if (!${Me.Buff["${FindItem[=${ClickyIllusionName}].Spell.Name}"].ID}) /call Cast "${ClickyIllusionName}" item
		}
	}
/return

|Casts the item clicky 5 times and auto inventories the summoned items
#Event MakePullerItem		"[MQ2] makepulleritem#1#"
Sub Event_MakePullerItem(string Line, int n)
	/if (!${mcHUDsummonPullItem.Length}) /return
	/declare i			int local 0
	/declare ammount	int local 5
	/if (${n}) /varset ammount ${n}

	/echo Clicking ${mcHUDsummonPullItem} ${ammount} times
	
	/for i 1 to ${ammount}
		/echo ${i}/${ammount}
		/call Cast "${mcHUDsummonPullItem}" item
		/delay 1s
		/autoinventory
	/next i
	/autoinventory
/return

Sub BuffedRunSpeed
	/declare SpeedBuff int local 0
	/if (${Me.Buff["Spirit of the Shrew"].ID})			/varset SpeedBuff 30
	/if (${Me.Buff["Pack Shrew"].ID})					/varset SpeedBuff 30
	/if (${Me.Buff["Feral Pack"].ID})					/varset SpeedBuff 30
	/if (${Me.Buff["Spirit of Bih`Li"].ID})				/varset SpeedBuff 55
	/if (${Me.Buff["Pack Spirit"].ID})					/varset SpeedBuff 55
	/if (${Me.Buff["Share Wolf Form"].ID})				/varset SpeedBuff 60
	/if (${Me.Buff["Spirit of Eagle"].ID})				/varset SpeedBuff 60
	/if (${Me.Buff["Share Form of the Great Wolf"].ID})	/varset SpeedBuff 62
	/if (${Me.Buff["Flight of Eagles"].ID})				/varset SpeedBuff 65
/return ${SpeedBuff}


sub FindClickies(iPack,iSlot)
	/declare tItem		string
	/declare tItemVal	int	inner 0
	/declare tItemBuff	string
	/declare sType		string
	/declare iSelect	int

	| Me.Inventory[packX].Item[N].Name

	/if (${iPack} == 0 ) {
		/varset tItem 		${InvSlot[${iSlot}].Item}
		/varset tItemBuff	${InvSlot[${iSlot}].Item.Spell}
		} else {
		/varset tItem 		${Me.Inventory[${iPack}].Item[${iSlot}]}
		/varset tItemBuff	${Me.Inventory[${iPack}].Item[${iSlot}].Spell}
		}

	| Checking for mana buffs
	/if (${tItemBuff.Equal[Lunar Whispers]}) 		/multiline ; /varset sType MANABUFF ; /varset tItemVal 2 
	/if (${tItemBuff.Equal[Soul Clasp]}) 			/multiline ; /varset sType MANABUFF ; /varset tItemVal 2
	/if (${tItemBuff.Equal[Aura of Eternity]}) 		/multiline ; /varset sType MANABUFF ; /varset tItemVal 5
	/if (${tItemBuff.Equal[Koadic's Heightened Focus]})	/multiline ; /varset sType MANABUFF ; /varset tItemVal 5
	/if (${tItemBuff.Equal[Aura of Taelosia]}) 		/multiline ; /varset sType MANABUFF ; /varset tItemVal 7
	/if (${tItemBuff.Equal[Maelin's Methodical Mind]}) 	/multiline ; /varset sType MANABUFF ; /varset tItemVal 8
	/if (${tItemBuff.Equal[Reyfin's Random Musings]}) 	/multiline ; /varset sType MANABUFF ; /varset tItemVal 9
	/if (${tItemBuff.Equal[Breath of Selay]}) 		/multiline ; /varset sType MANABUFF ; /varset tItemVal 9
	/if (${tItemBuff.Equal[Chaotic Enlightenment]}) 	/multiline ; /varset sType MANABUFF ; /varset tItemVal 10
	/if (${tItemBuff.Equal[Knowledge of the Past I]}) 	/multiline ; /varset sType MANABUFF ; /varset tItemVal 11
	/if (${tItemBuff.Equal[Knowledge of the Past II]}) 	/multiline ; /varset sType MANABUFF ; /varset tItemVal 14
	/if (${tItemBuff.Equal[Knowledge of the Past III]}) 	/multiline ; /varset sType MANABUFF ; /varset tItemVal 17
	/if (${tItemBuff.Equal[Knowledge of the Past IV]}) 	/multiline ; /varset sType MANABUFF ; /varset tItemVal 19
	/if (${tItemBuff.Equal[Knowledge of the Past V]}) 	/multiline ; /varset sType MANABUFF ; /varset tItemVal 27
	/if (${tItemBuff.Equal[Knowledge of the Past VI]}) 	/multiline ; /varset sType MANABUFF ; /varset tItemVal 32
	/if (${tItemBuff.Equal[Knowledge of the Past VII]}) 	/multiline ; /varset sType MANABUFF ; /varset tItemVal 37
	/if (${tItemBuff.Equal[Knowledge of the Past VIII]}) 	/multiline ; /varset sType MANABUFF ; /varset tItemVal 42
	/if (${tItemBuff.Equal[Knowledge of the Past IX]}) 	/multiline ; /varset sType MANABUFF ; /varset tItemVal 47
	/if (${tItemBuff.Equal[Knowledge of the Past X]}) 	/multiline ; /varset sType MANABUFF ; /varset tItemVal 52
	/if (${tItemBuff.Equal[Soothing Breath I]})	 	/multiline ; /varset sType MANABUFF ; /varset tItemVal 17
	/if (${tItemBuff.Equal[Soothing Breath II]}) 		/multiline ; /varset sType MANABUFF ; /varset tItemVal 25
	/if (${tItemBuff.Equal[Soothing Breath III]}) 		/multiline ; /varset sType MANABUFF ; /varset tItemVal 25
	/if (${tItemBuff.Equal[Soothing Breath IV]}) 		/multiline ; /varset sType MANABUFF ; /varset tItemVal 25
	/if (${tItemBuff.Equal[Soothing Breath V]})	 	/multiline ; /varset sType MANABUFF ; /varset tItemVal 25


	| Checking for mana pool buffs
	/if (${tItemBuff.Equal[Koadic's Expansive Mind]}) 	/multiline ; /varset sType MANAPOOL ; /varset tItemVal 250
	/if (${tItemBuff.Equal[Maelin's Meditation]}) 		/multiline ; /varset sType MANAPOOL ; /varset tItemVal 400
	/if (${tItemBuff.Equal[Reyfin's Racing Thoughts]}) 	/multiline ; /varset sType MANAPOOL ; /varset tItemVal 450
	/if (${tItemBuff.Equal[Salik's Expansive Mind]}) 	/multiline ; /varset sType MANAPOOL ; /varset tItemVal 500
	/if (${tItemBuff.Equal[Ellowind's Expansive Mind]}) 	/multiline ; /varset sType MANAPOOL ; /varset tItemVal 550
	/if (${tItemBuff.Equal[Expanding Mind I]}) 		/multiline ; /varset sType MANAPOOL ; /varset tItemVal 600
	/if (${tItemBuff.Equal[Expanding Mind II]}) 		/multiline ; /varset sType MANAPOOL ; /varset tItemVal 650
	/if (${tItemBuff.Equal[Expanding Mind III]}) 		/multiline ; /varset sType MANAPOOL ; /varset tItemVal 700
	/if (${tItemBuff.Equal[Expanding Mind IV]}) 		/multiline ; /varset sType MANAPOOL ; /varset tItemVal 750
	/if (${tItemBuff.Equal[Expanding Mind V]}) 		/multiline ; /varset sType MANAPOOL ; /varset tItemVal 800

	| Checking for DS2 Buffs
	/if (${tItemBuff.Equal[Riftwind's Protection]}) 	/multiline ; /varset sType DS2 ; /varset tItemVal 2
	/if (${tItemBuff.Equal[Force of Nature]}) 		/multiline ; /varset sType DS2 ; /varset tItemVal 3
	/if (${tItemBuff.Equal[Call of Earth]})	 		/multiline ; /varset sType DS2 ; /varset tItemVal 4
	/if (${tItemBuff.Equal[Chromatic Chaos]}) 		/multiline ; /varset sType DS2 ; /varset tItemVal 5
	/if (${tItemBuff.Equal[Tempest Guard]}) 		/multiline ; /varset sType DS2 ; /varset tItemVal 5
	/if (${tItemBuff.Equal[Fiery Might]})			/multiline ; /varset sType DS2 ; /varset tItemVal 5
	/if (${tItemBuff.Equal[O'Keils Flickering Flame]}) 	/multiline ; /varset sType DS2 ; /varset tItemVal 5
	/if (${tItemBuff.Equal[Fiery Might]}) 			/multiline ; /varset sType DS2 ; /varset tItemVal 5
	/if (${tItemBuff.Equal[Shield of the Eighth]}) 		/multiline ; /varset sType DS2 ; /varset tItemVal 8
	/if (${tItemBuff.Equal[Illusion Fire Elemental]}) 	/multiline ; /varset sType DS2 ; /varset tItemVal 9
	/if (${tItemBuff.Equal[Call of the Rathe]}) 		/multiline ; /varset sType DS2 ; /varset tItemVal 10
	/if (${tItemBuff.Equal[Frostreaver's Blessing]})	/multiline ; /varset sType DS2 ; /varset tItemVal 10
	/if (${tItemBuff.Equal[Guard of the Earth]}) 		/multiline ; /varset sType DS2 ; /varset tItemVal 13
	/if (${tItemBuff.Equal[Vengeful Guard]}) 		/multiline ; /varset sType DS2 ; /varset tItemVal 20
	/if (${tItemBuff.Equal[Hateful Guard]})	 		/multiline ; /varset sType DS2 ; /varset tItemVal 25
	/if (${tItemBuff.Equal[Illusionary Spikes I]})		/multiline ; /varset sType DS2 ; /varset tItemVal 30
	/if (${tItemBuff.Equal[Illusionary Spikes II]})		/multiline ; /varset sType DS2 ; /varset tItemVal 35
	/if (${tItemBuff.Equal[Illusionary Spikes III]})	/multiline ; /varset sType DS2 ; /varset tItemVal 40
	/if (${tItemBuff.Equal[Illusionary Spikes IV]})		/multiline ; /varset sType DS2 ; /varset tItemVal 45
	/if (${tItemBuff.Equal[Illusionary Spikes V]})		/multiline ; /varset sType DS2 ; /varset tItemVal 50


	| Checking for DS4 Buffs
	/if (${tItemBuff.Equal[Thorny Shield]})	 		/multiline ; /varset sType DS4 ; /varset tItemVal 4
	/if (${tItemBuff.Equal[Icicle Aura]})	 		/multiline ; /varset sType DS4 ; /varset tItemVal 6
	/if (${tItemBuff.Equal[Shield of Pain]}) 		/multiline ; /varset sType DS4 ; /varset tItemVal 10
	/if (${tItemBuff.Equal[Shield of Torment]}) 		/multiline ; /varset sType DS4 ; /varset tItemVal 15
	/if (${tItemBuff.Equal[Icicle Shield]})	 		/multiline ; /varset sType DS4 ; /varset tItemVal 18
	/if (${tItemBuff.Equal[Frost Shield]}) 			/multiline ; /varset sType DS4 ; /varset tItemVal 21
	/if (${tItemBuff.Equal[Frost Guard]})	 		/multiline ; /varset sType DS4 ; /varset tItemVal 24
	/if (${tItemBuff.Equal[Storm Guard]})	 		/multiline ; /varset sType DS4 ; /varset tItemVal 27

	| Checking for Resist1 Clicky
	/if (${tItemBuff.Equal[Soul Energy]}) 			/multiline ; /varset sType RESIST1 ; /varset tItemVal 10
	/if (${tItemBuff.Equal[Eternal Ward]}) 			/multiline ; /varset sType RESIST1 ; /varset tItemVal 15
	/if (${tItemBuff.Equal[Chaotic Ward]}) 			/multiline ; /varset sType RESIST1 ; /varset tItemVal 20
	/if (${tItemBuff.Equal[Prismatic Ward I]}) 		/multiline ; /varset sType RESIST1 ; /varset tItemVal 35
	/if (${tItemBuff.Equal[Prismatic Ward II]}) 		/multiline ; /varset sType RESIST1 ; /varset tItemVal 40
	/if (${tItemBuff.Equal[Prismatic Ward III]}) 		/multiline ; /varset sType RESIST1 ; /varset tItemVal 45
	/if (${tItemBuff.Equal[Prismatic Ward IV]}) 		/multiline ; /varset sType RESIST1 ; /varset tItemVal 50
	/if (${tItemBuff.Equal[Prismatic Ward V]}) 		/multiline ; /varset sType RESIST1 ; /varset tItemVal 55
	/if (${tItemBuff.Equal[Prismatic Ward VI]}) 		/multiline ; /varset sType RESIST1 ; /varset tItemVal 60

	| Checking for Resist3 Clicky
	/if (${tItemBuff.Equal[Aura of Pure Arcanum]})		/multiline ; /varset sType RESIST3 ; /varset tItemVal 50
	/if (${tItemBuff.Equal[Form of Protection I]})		/multiline ; /varset sType RESIST3 ; /varset tItemVal 10
	/if (${tItemBuff.Equal[Form of Protection II]})		/multiline ; /varset sType RESIST3 ; /varset tItemVal 20
	/if (${tItemBuff.Equal[Form of Protection III]})	/multiline ; /varset sType RESIST3 ; /varset tItemVal 30
	/if (${tItemBuff.Equal[Form of Protection IV]})		/multiline ; /varset sType RESIST3 ; /varset tItemVal 40
	/if (${tItemBuff.Equal[Form of Protection V]})		/multiline ; /varset sType RESIST3 ; /varset tItemVal 55
	/if (${tItemBuff.Equal[Form of Protection VI]})		/multiline ; /varset sType RESIST3 ; /varset tItemVal 60
	/if (${tItemBuff.Equal[Form of Protection VII]})	/multiline ; /varset sType RESIST3 ; /varset tItemVal 70

	| Checking for HP Clicky
	/if (${tItemBuff.Equal[Form of Endurance I]}) 		/multiline ; /varset sType HP ; /varset tItemVal 90
	/if (${tItemBuff.Equal[Form of Endurance II]}) 		/multiline ; /varset sType HP ; /varset tItemVal 180
	/if (${tItemBuff.Equal[Form of Endurance III]}) 	/multiline ; /varset sType HP ; /varset tItemVal 270
	/if (${tItemBuff.Equal[Form of Endurance IV]}) 		/multiline ; /varset sType HP ; /varset tItemVal 360
	/if (${tItemBuff.Equal[Form of Endurance V]}) 		/multiline ; /varset sType HP ; /varset tItemVal 450
	/if (${tItemBuff.Equal[Form of Endurance VI]}) 		/multiline ; /varset sType HP ; /varset tItemVal 540
	/if (${tItemBuff.Equal[Form of Endurance VII]}) 	/multiline ; /varset sType HP ; /varset tItemVal 630
	/if (${tItemBuff.Equal[Form of Endurance VIII]})	/multiline ; /varset sType HP ; /varset tItemVal 720
	/if (${tItemBuff.Equal[Form of Endurance IX]})		/multiline ; /varset sType HP ; /varset tItemVal 810
	/if (${tItemBuff.Equal[Form of Endurance X]})		/multiline ; /varset sType HP ; /varset tItemVal 900

	| Checking for Attack5 Buffs
	/if (${tItemBuff.Equal[Chromatic Chaos]}) 		/multiline ; /varset sType ATK5 ; /varset tItemVal 5
	/if (${tItemBuff.Equal[Frostreaver's Blessing]}) 	/multiline ; /varset sType ATK5 ; /varset tItemval 10
	/if (${tItemBuff.Equal[Savage Guard]}) 			/multiline ; /varset sType ATK5 ; /varset tItemVal 25
	/if (${tItemBuff.Equal[Furious Might]}) 		/multiline ; /varset sType ATK5 ; /varset tItemVal 40
	/if (${tItemBuff.Equal[Might of the Giants]})		/multiline ; /varset sType ATK5 ; /varset tItemVal 55
	/if (${tItemBuff.Equal[Might of Stone I]})		/multiline ; /varset sType ATK5 ; /varset tItemVal 60
	/if (${tItemBuff.Equal[Might of Stone II]})		/multiline ; /varset sType ATK5 ; /varset tItemVal 65
	/if (${tItemBuff.Equal[Might of Stone III]})		/multiline ; /varset sType ATK5 ; /varset tItemVal 70
	/if (${tItemBuff.Equal[Might of Stone IV]})		/multiline ; /varset sType ATK5 ; /varset tItemVal 75
	/if (${tItemBuff.Equal[Might of Stone V]})		/multiline ; /varset sType ATK5 ; /varset tItemVal 80

	| Checking for Attack3 Buffs
	/if (${tItemBuff.Equal[Aura of Battle]})		/multiline ; /varset sType ATK3 ; /varset tItemVal 10
	/if (${tItemBuff.Equal[Aura of Daring]})		/multiline ; /varset sType ATK3 ; /varset tItemVal 20
	/if (${tItemBuff.Equal[Carnage]}) 			/multiline ; /varset sType ATK3 ; /varset tItemVal 40
	/if (${tItemBuff.Equal[Avatar]}) 			/multiline ; /varset sType ATK3 ; /varset tItemVal 100
	/if (${tItemBuff.Equal[Ferine Avatar]})			/multiline ; /varset sType ATK3 ; /varset tItemVal 140

	| Checking for Attack1 Buffs
	/if (${tItemBuff.Equal[Spiritual Brawn]}) 		/multiline ; /varset sType ATK1 ; /varset tItemVal 10
	/if (${tItemBuff.Equal[Firefist]}) 			/multiline ; /varset sType ATK1 ; /varset tItemVal 10
	/if (${tItemBuff.Equal[Grim Aura]}) 			/multiline ; /varset sType ATK1 ; /varset tItemVal 10
	/if (${tItemBuff.Equal[Aura of Rage]}) 			/multiline ; /varset sType ATK1 ; /varset tItemVal 20
	/if (${tItemBuff.Equal[Nortlav's Vengeance]}) 		/multiline ; /varset sType ATK1 ; /varset tItemVal 20
	/if (${tItemBuff.Equal[Fury of the Vah Shir]}) 		/multiline ; /varset sType ATK1 ; /varset tItemVal 30
	/if (${tItemBuff.Equal[Celestial Tranquility]}) 	/multiline ; /varset sType ATK1 ; /varset tItemVal 40

	| Checking for AC Clicky
	/if (${tItemBuff.Equal[Form of Defense I]}) 		/multiline ; /varset sType AC ; /varset tItemVal 27
	/if (${tItemBuff.Equal[Form of Defense II]}) 		/multiline ; /varset sType AC ; /varset tItemVal 54
	/if (${tItemBuff.Equal[Form of Defense III]}) 		/multiline ; /varset sType AC ; /varset tItemVal 81
	/if (${tItemBuff.Equal[Form of Defense IV]}) 		/multiline ; /varset sType AC ; /varset tItemVal 108
	/if (${tItemBuff.Equal[Form of Defense V]}) 		/multiline ; /varset sType AC ; /varset tItemVal 135
	/if (${tItemBuff.Equal[Form of Defense VI]}) 		/multiline ; /varset sType AC ; /varset tItemVal 162
	/if (${tItemBuff.Equal[Form of Defense VII]}) 		/multiline ; /varset sType AC ; /varset tItemVal 189
	/if (${tItemBuff.Equal[Form of Defense VIII]}) 		/multiline ; /varset sType AC ; /varset tItemVal 216
	/if (${tItemBuff.Equal[Form of Defense IX]}) 		/multiline ; /varset sType AC ; /varset tItemVal 243
	/if (${tItemBuff.Equal[Form of Defense X]}) 		/multiline ; /varset sType AC ; /varset tItemVal 270

	| Checking for Overhaste Clicky
	/if (${tItemBuff.Equal[Primal Guard]}) 			/multiline ; /varset sType HV3 ; /varset tItemVal 5
	/if (${tItemBuff.Equal[Taelosian Guard]}) 		/multiline ; /varset sType HV3 ; /varset tItemVal 8
	/if (${tItemBuff.Equal[Ancient Power]}) 		/multiline ; /varset sType HV3 ; /varset tItemVal 10
	/if (${tItemBuff.Equal[Ancestral Memories I]})		/multiline ; /varset sType HV3 ; /varset tItemVal 12
	/if (${tItemBuff.Equal[Ancestral Memories II]})		/multiline ; /varset sType HV3 ; /varset tItemVal 14
	/if (${tItemBuff.Equal[Ancestral Memories III]})	/multiline ; /varset sType HV3 ; /varset tItemVal 16

	| Checking for Haste Clicky
	/if (${tItemBuff.Equal[Miraculous Visions]}) 		/multiline ; /varset sType HV1 ; /varset tItemVal 30
	/if (${tItemBuff.Equal[Marvelous Visions]}) 		/multiline ; /varset sType HV1 ; /varset tItemVal 30
	/if (${tItemBuff.Equal[Twitching Speed]}) 		/multiline ; /varset sType HV1 ; /varset tItemVal 50



	/varset iSelect ${Select[${sType},MANABUFF,MANAPOOL,DS2,DS4,RESIST1,RESIST3,HP,ATK1,ATK3,ATK5,AC,HV1,HV3]}  | 13 types

	|determine if the current item clicky is better than the old one

	|Mana Regen buff
	/if (${iSelect}==1) {
		/if (${tItemVal}>${ManaregenBuffVal}) {
			/varset ManaregenBuff		${tItemBuff}
			/varset ManaregenBuffVal	${tItemVal}
			/varset ManaregenBuffItem	${tItem}
			/if (${iShow}==1) /echo Found mana regen item:${tItem} @ ${tItemVal}
			}
		}

	|Mana pool buff
	/if (${iSelect}==2) {
		/if (${tItemVal}>${ManapoolBuffVal}) {
			/varset ManapoolBuff		${tItemBuff}
			/varset ManapoolBuffVal		${tItemVal}
			/varset ManapoolBuffItem	${tItem}
			/if (${iShow}==1) /echo Found mana pool item:${tItem} @ ${tItemVal} 
			}
		}

	|DS Slot 2 buff
	/if (${iSelect}==3) {
		/if (${tItemVal}>${DSBuff2Val}) {
			/varset DSBuff2			${tItemBuff}
			/varset DSBuff2Val		${tItemVal}
			/varset DSBuff2Item		${tItem}
			/if (${iShow}==1) /echo Found DS2 item:${tItem} @ ${tItemVal}
			}
		}


	|DS Slot 4 buff
	/if (${iSelect}==4) {
		/if (${tItemVal}>${DSBuff4Val}) {
			/varset DSBuff4			${tItemBuff}
			/varset DSBuff4Val		${tItemVal}
			/varset DSBuff4Item		${tItem}
			/if (${iShow}==1) /echo Found DS4 item:${tItem} @ ${tItemVal}
			}
		}

	|Resist V1 buff
	/if (${iSelect}==5) {
		/if (${tItemVal}>${Resist1Val}) {
			/varset Resist1			${tItemBuff}
			/varset Resist1Val		${tItemVal}
			/varset Resist1Item		${tItem}
			/if (${iShow}==1) /echo Found ResistV1 item:${tItem} @ ${tItemVal}
			}
		}

	|Resist V3 buff
	/if (${iSelect}==6) {
		/if (${tItemVal}>${Resist3Val}) {
			/varset Resist3			${tItemBuff}
			/varset Resist3Val		${tItemVal}
			/varset Resist3Item		${tItem}
			/if (${iShow}==1) /echo Found ResistV3 item:${tItem} @ ${tItemVal}
			}
		}

	|HP buff
	/if (${iSelect}==7) {
		/if (${tItemVal}>${HPBuffVal}) {
			/varset HPBuff			${tItemBuff}
			/varset HPBuffVal		${tItemVal}
			/varset HPBuffItem		${tItem}
			/if (${iShow}==1) /echo Found HP item:${tItem} @ ${tItemVal}
			}
		}

	|ATK Slot 1 buff
	/if (${iSelect}==8) {
		/if (${tItemVal}>${ATK1Val}) {
			/varset ATK1			${tItemBuff}
			/varset ATK1Val			${tItemVal}
			/varset ATK1Item		${tItem}
			/if (${iShow}==1) /echo Found ATK1 item:${tItem} @ ${tItemVal}
			}
		}

	|ATK Slot 3 buff
	/if (${iSelect}==9) {
		/if (${tItemVal}>${ATK3Val}) {
			/varset ATK3			${tItemBuff}
			/varset ATK3Val			${tItemVal}
			/varset ATK3Item		${tItem}
			/if (${iShow}==1) /echo Found ATK3 item:${tItem} @ ${tItemVal}
			}
		}


	|ATK Slot 5 buff
	/if (${iSelect}==10) {
		/if (${tItemVal}>${ATK5Val}) {
			/varset ATK5			${tItemBuff}
			/varset ATK5Val			${tItemVal}
			/varset ATK5Item		${tItem}
			/if (${iShow}==1) /echo Found ATK5 item:${tItem} @ ${tItemVal}
			}
		}

	|AC buff
	/if (${iSelect}==11) {
		/if (${tItemVal}>${ACBuffVal}) {
			/varset ACBuff			${tItemBuff}
			/varset ACBuffVal		${tItemVal}
			/varset ACBuffItem		${tItem}
			/if (${iShow}==1) /echo Found AC item:${tItem} @ ${tItemVal}
			}
		}

	|Haste V1 buff
	/if (${iSelect}==12) {
		/if (${tItemVal}>${HV1BuffVal}) {
			/varset HV1Buff			${tItemBuff}
			/varset HV1BuffVal		${tItemVal}
			/varset HV1VBuffItem		${tItem}
			/if (${iShow}==1) /echo Found HV1 item:${tItem} @ ${tItemVal}
			}
		}

	|Haste v3 buff
	/if (${iSelect}==13) {
		/if (${tItemVal}>${HV3BuffVal}) {
			/varset HV3Buff			${tItemBuff}
			/varset HV3BuffVal		${tItemVal}
			/varset HV3BuffItem		${tItem}
			/if (${iShow}==1) /echo Found HV3 item:${tItem} @ ${tItemVal}
			}
		}
	/return


