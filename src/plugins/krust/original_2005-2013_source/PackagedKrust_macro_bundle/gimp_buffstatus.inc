| exposes a number of HUD variables, call CheckBuffs
|
|
|fixme: om man har paladin i gruppen ska symbol of jeran listas som missing


#Event JoinedGroup "You have joined the group."
#Event JoinedGroup "You have been removed from the group."
#Event JoinedGroup "Your raid was disbanded."
Sub Event_JoinedGroup(string Line)
	/if (${Raid.Members}) {
		/varset mcHUDbuffmode RL ${Raid.Leader}, GL${Raid.Member[${Me}].Group} ${Group.Leader} (raid:${Raid.Members}, zone:${SpawnCount[pc]})
	} else {
		/varset mcHUDbuffmode UNGROUPED (zone:${SpawnCount[pc]})
		/if (${Group.Members}) /varset mcHUDbuffmode GL: ${Group.Leader.Name} (group:${Math.Calc[${Group.Members}+1].Int}, zone:${SpawnCount[pc]})
	}
/return

Sub CheckBuffs

	/if (!${gimp_buffstatus_loaded}) {
		/declare mcHUDbuffmode				string	outer
		/declare mcHUDmeleebuffs			string	outer
		/declare mcHUDcasterbuffs			string	outer
		/declare mcHUDresistbuffs			string	outer
		/declare gimp_buffstatus_loaded		int		outer	1
		/declare gimp_buffstatus_timer	timer outer 0
	}

	/if (${gimp_buffstatus_timer}) /return
	/varset gimp_buffstatus_timer 100

	/call Event_JoinedGroup

	/declare str	string local
	/declare i int local 0

	/declare ZoneAllowsLevitate int local ${Ini[ZONEINFO_FILE, ZoneInfo, ${Zone.ShortName}_Levitate, 0]}

	/declare HasBST int local 0
	/declare HasCLR int local 0
	/declare HasDRU int local 0
	/declare HasSHD int local 0
	/declare HasSHM int local 0
	/declare HasENC int local 0
	/declare HasPAL int local 0
	/declare HasMAG int local 0
	/declare HasNEC int local 0
	/declare HasRNG int local 0
	
	/declare CasterClass int local 0
	/declare MeleeClass int local 0
	/declare HybridClass int local 0
	/declare TankClass int local 0

	/if (${Raid.Members}) {
		/for i 0 to ${Raid.Members}
			/if (${Raid.Member[${i}].Class.Name.Equal["Beastlord"]})				/varset HasBST 1
			/if (${Raid.Member[${i}].Class.Name.Equal["Cleric"]})						/varset HasCLR 1
			/if (${Raid.Member[${i}].Class.Name.Equal["Druid"]})						/varset HasDRU 1
			/if (${Raid.Member[${i}].Class.Name.Equal["Enchanter"]})				/varset HasENC 1
			/if (${Raid.Member[${i}].Class.Name.Equal["Magician"]})					/varset HasMAG 1
			/if (${Raid.Member[${i}].Class.Name.Equal["Necromancer"]})			/varset HasNEC 1
			/if (${Raid.Member[${i}].Class.Name.Equal["Paladin"]})					/varset HasPAL 1
			/if (${Raid.Member[${i}].Class.Name.Equal["Ranger"]})						/varset HasRNG 1
			/if (${Raid.Member[${i}].Class.Name.Equal["Shadow Knight"]})		/varset HasSHD 1
			/if (${Raid.Member[${i}].Class.Name.Equal["Shaman"]})						/varset HasSHM 1
			/doevents
		/next i
	} else {
		|0=myself
		/for i 0 to 5
			/if (${Group.Member[${i}].Class.Name.Equal["Beastlord"]})				/varset HasBST 1
			/if (${Group.Member[${i}].Class.Name.Equal["Cleric"]})					/varset HasCLR 1
			/if (${Group.Member[${i}].Class.Name.Equal["Druid"]})						/varset HasDRU 1
			/if (${Group.Member[${i}].Class.Name.Equal["Enchanter"]})				/varset HasENC 1
			/if (${Group.Member[${i}].Class.Name.Equal["Magician"]})				/varset HasMAG 1
			/if (${Group.Member[${i}].Class.Name.Equal["Necromancer"]})			/varset HasNEC 1
			/if (${Group.Member[${i}].Class.Name.Equal["Paladin"]})					/varset HasPAL 1
			/if (${Group.Member[${i}].Class.Name.Equal["Ranger"]})					/varset HasRNG 1
			/if (${Group.Member[${i}].Class.Name.Equal["Shadow Knight"]})		/varset HasSHD 1
			/if (${Group.Member[${i}].Class.Name.Equal["Shaman"]})					/varset HasSHM 1
			/doevents
		/next i
	}

	/if (${Me.Class.Name.Equal["Druid"]} || ${Me.Class.Name.Equal["Cleric"]} || ${Me.Class.Name.Equal["Shaman"]} || ${Me.Class.Name.Equal["Enchanter"]} || ${Me.Class.Name.Equal["Magician"]} || ${Me.Class.Name.Equal["Wizard"]} || ${Me.Class.Name.Equal["Necromancer"]}) /varset CasterClass 1
	/if (${Me.Class.Name.Equal["Warrior"]} || ${Me.Class.Name.Equal["Shadow Knight"]} || ${Me.Class.Name.Equal["Paladin"]}) /varset TankClass 1
	/if (${Me.Class.Name.Equal["Ranger"]} || ${Me.Class.Name.Equal["Beastlord"]} || ${Me.Class.Name.Equal["Paladin"]} || ${Me.Class.Name.Equal["Shadow Knight"]} || ${Me.Class.Name.Equal["Bard"]}) /varset HybridClass 1
	/if (${Me.Class.Name.Equal["Monk"]} || ${Me.Class.Name.Equal["Rogue"]} || ${Me.Class.Name.Equal["Warrior"]} || ${Me.Class.Name.Equal["Berserker"]}) /varset MeleeClass 1

	|XXXXXXXXXXXXXXXXXXXXXXXXXXXX
	|HP & MELEE BUFFS
	|XXXXXXXXXXXXXXXXXXXXXXXXXXXX
	/varset str
	/if (${HasDRU} && !${Me.Buff["Steeloak Skin"].ID} && !${Me.Buff["Blessing of Steeloak"].ID} && !${Me.Buff["Blessing of the Nine"].ID} && !${Me.Buff["Protection of the Nine"].ID} && !${Me.Buff["Direwild Skin"].ID} && !${Me.Buff["Direwild Skin Rk. II"].ID} && !${Me.Buff["Direwild Skin Rk. III"].ID} && !${Me.Buff["Blessing of the Direwild"].ID} && !${Me.Buff["Blessing of the Direwild Rk. II"].ID} && !${Me.Buff["Blessing of the Direwild Rk. III"].ID} && !${Me.Buff["Conviction"].ID} && !${Me.Buff["Hand of Conviction"].ID} && !${Me.Buff["Virtue"].ID} && !${Me.Buff["Hand of Virtue"].ID} && !${Me.Buff["Tenacity"].ID} && !${Me.Buff["Tenacity Rk. II"].ID} && !${Me.Buff["Tenacity Rk. III"].ID} && !${Me.Buff["Hand of Tenacity"].ID} && !${Me.Buff["Hand of Tenacity Rk. II"].ID} && !${Me.Buff["Hand of Tenacity Rk. III"].ID}) /varset str ${str} DIRSKN
	/if ((${HasCLR} && ${HasDRU}) && !${Me.Buff["Symbol of Balikor"].ID} && !${Me.Buff["Balikor's Mark"].ID} && !${Me.Buff["Symbol of Kazad"].ID} && !${Me.Buff["Kazad`s Mark"].ID} && !${Me.Buff["Jeron's Mark"].ID} && !${Me.Buff["Symbol of Elushar"].ID} && !${Me.Buff["Symbol of Elushar Rk. II"].ID} && !${Me.Buff["Symbol of Elushar Rk. III"].ID} && !${Me.Buff["Elushar's Mark"].ID} && !${Me.Buff["Elushar's Mark Rk. II"].ID} && !${Me.Buff["Elushar's Mark Rk. III"].ID} && !${Me.Buff["Conviction"].ID} && !${Me.Buff["Hand of Conviction"].ID} && !${Me.Buff["Virtue"].ID} && !${Me.Buff["Hand of Virtue"].ID} && !${Me.Buff["Tenacity"].ID} && !${Me.Buff["Tenacity Rk. II"].ID} && !${Me.Buff["Tenacity Rk. III"].ID} && !${Me.Buff["Hand of Tenacity"].ID} && !${Me.Buff["Hand of Tenacity Rk. II"].ID} && !${Me.Buff["Hand of Tenacity Rk. III"].ID}) /varset str ${str} SYMBOL
	/if ((!${HasDRU} && ${HasCLR}) && !${Me.Buff["Blessing of Steeloak"].ID} && !${Me.Buff["Steeloak Skin"].ID} && !${Me.Buff["Blessing of the Nine"].ID} && !${Me.Buff["Protection of the Nine"].ID} && !${Me.Buff["Direwild Skin"].ID} && !${Me.Buff["Direwild Skin Rk. II"].ID} && !${Me.Buff["Direwild Skin Rk. III"].ID} && !${Me.Buff["Blessing of the Direwild"].ID} && !${Me.Buff["Blessing of the Direwild Rk. II"].ID} && !${Me.Buff["Blessing of the Direwild Rk. III"].ID} && !${Me.Buff["Symbol of Balikor"].ID} && !${Me.Buff["Balikor's Mark"].ID} && !${Me.Buff["Symbol of Kazad"].ID} && !${Me.Buff["Kazad`s Mark"].ID} && !${Me.Buff["Jeron's Mark"].ID} && !${Me.Buff["Symbol of Elushar"].ID} && !${Me.Buff["Symbol of Elushar Rk. II"].ID} && !${Me.Buff["Symbol of Elushar Rk. III"].ID} && !${Me.Buff["Elushar's Mark"].ID} && !${Me.Buff["Elushar's Mark Rk. II"].ID} && !${Me.Buff["Elushar's Mark Rk. III"].ID} && !${Me.Buff["Conviction"].ID} && !${Me.Buff["Hand of Conviction"].ID} && !${Me.Buff["Temperance"].ID} && !${Me.Buff["Blessing of Temperance"].ID} && !${Me.Buff["Virtue"].ID} && !${Me.Buff["Hand of Virtue"].ID} && !${Me.Buff["Tenacity"].ID} && !${Me.Buff["Tenacity Rk. II"].ID} && !${Me.Buff["Tenacity Rk. III"].ID} && !${Me.Buff["Hand of Tenacity"].ID} && !${Me.Buff["Hand of Tenacity Rk. II"].ID} && !${Me.Buff["Hand of Tenacity Rk. III"].ID}) /varset str ${str} Tena
	/if (${Me.Class.Name.Equal["Ranger"]} && !${Me.Buff["Strength of the Hunter"].ID} && !${Me.Buff["Strength of the Forest Stalker"].ID} && !${Me.Buff["Strength of the Forest Stalker Rk. II"].ID} && !${Me.Buff["Strength of the Forest Stalker Rk. III"].ID} && !${Me.Buff["Aura of Rage"].ID} && !${Me.Buff["Nature's Precision"].ID}) /varset str ${str} ATK

	/if ((${HasPAL} || ${HasBST} || ${HasRNG}) && !${Me.Buff["Brell's Stony Guard"].ID} && !${Me.Buff["Brell's Stony Guard Rk. II"].ID} && !${Me.Buff["Brell's Stony Guard Rk. III"].ID} && !${Me.Buff["Brell's Brawny Bulwark"].ID} && !${Me.Buff["Brell's Stalwart Shield"].ID} && !${Me.Buff["Brell's Mountainous Barrier"].ID} && !${Me.Buff["Spiritual Vim"].ID} && !${Me.Buff["Spiritual Vim Rk. II"].ID} && !${Me.Buff["Spiritual Vim Rk. III"].ID} && !${Me.Buff["Spiritual Vitality"].ID} && !${Me.Buff["Spiritual Vigor"].ID} && !${Me.Buff["Spiritual Strength"].ID} && !${Me.Buff["Strength of the Hunter"].ID} && !${Me.Buff["Strength of the Forest Stalker"].ID} && !${Me.Buff["Strength of the Forest Stalker Rk. II"].ID} && !${Me.Buff["Strength of the Forest Stalker Rk. III"].ID} && !${Me.Buff["Protection of the Minohten"].ID} && !${Me.Buff["Protection of the Minohten Rk. II"].ID} && !${Me.Buff["Protection of the Minohten Rk. III"].ID} && !${Me.Buff["Ward of the Hunter"].ID} && !${Me.Buff["Strength of Tunare"].ID}) {
		/if (${HasPAL} && ${Me.Class.Name.NotEqual["Ranger"]}) {
			/varset str ${str} BRELLS
		} else {
			|Clerics Yaulp dont stack with SV
			/if (${HasBST} && ${Me.Class.Name.NotEqual["Cleric"]} && ${Me.Class.Name.NotEqual["Ranger"]}) {
				/varset str ${str} SV
			} else {
				/if (${HasRNG} && ${Me.Class.Name.NotEqual["Ranger"]}) {
					/varset str ${str} SotFS
				} else {
					/if (${Me.Class.Name.Equal["Ranger"]}) /varset str ${str} PotM
						
				}			
			}
		}
	}
	
	/if (${Me.Class.Name.Equal["Ranger"]} && !${Me.Buff["Eagle Eye"].ID} && !${Me.Buff["Eyes of the Owl"].ID}) /varset str ${str} EYE
	/if (${Me.Class.Name.Equal["Ranger"]} && !${Me.Buff["Call of Lightning"].ID} && !${Me.Buff["Jolting Blades"].ID} && !${Me.Buff["Sylvan Call"].ID} && !${Me.Buff["Cry of Thunder"].ID}) /varset str ${str} PROC
	/if (${Me.Class.Name.Equal["Ranger"]} && !${Me.Buff["Consumed by the Hunt"].ID}) /varset str ${str} CbtH

	/if ((${HasSHM} || ${HasBST}) && !${Me.Buff["Dire Focusing"].ID} && !${Me.Buff["Dire Focusing Rk. II"].ID} && !${Me.Buff["Dire Focusing Rk. III"].ID} && !${Me.Buff["Talisman of the Dire"].ID} && !${Me.Buff["Talisman of the Dire Rk. II"].ID} && !${Me.Buff["Talisman of the Dire Rk. III"].ID} && !${Me.Buff["Wunshi's Focusing"].ID} && !${Me.Buff["Talisman of Wunshi"].ID} && !${Me.Buff["Focus of the Seventh"].ID} && !${Me.Buff["Focus of Soul"].ID} && !${Me.Buff["Focus of Amilan"].ID} && !${Me.Buff["Focus of Amilan Rk. II"].ID} && !${Me.Buff["Focus of Amilan Rk. III"].ID} && !${Me.Buff["Focus of Alladnu"].ID} && !${Me.Buff["Talisman of Kragg"].ID} && !${Me.Buff["Shield of the Arcane"].ID} && !${Me.Buff["Armor of the Pious"].ID} && !${Me.Buff["Armor of the Zealot"].ID}) /varset str ${str} FOCUS
	/if ((${HasENC} || ${HasSHM} || ${HasBST}) && !${CasterClass} && !${Me.Buff["Speed of Ellowind"].ID} && !${Me.Buff["Speed of Ellowind Rk. II"].ID} && !${Me.Buff["Speed of Ellowind Rk. III"].ID} && !${Me.Buff["Hastening of Ellowind"].ID} && !${Me.Buff["Hastening of Ellowind Rk. II"].ID} && !${Me.Buff["Hastening of Ellowind Rk. III"].ID} && !${Me.Buff["Speed of Salik"].ID} && !${Me.Buff["Hastening of Salik"].ID} && !${Me.Buff["Vallon's Quickening"].ID} && !${Me.Buff["Speed of Vallon"].ID} && !${Me.Buff["Celerity"].ID} && !${Me.Buff["Alacrity"].ID} && !${Me.Buff["Talisman of Alacrity"].ID} && !${Me.Buff["Talisman of Celerity"].ID} && !${Me.Buff["Swift like the Wind"].ID} && !${Me.Buff["Wonderous Rapidity"].ID} && !${Me.Buff["Twitching Speed"].ID} && !${Me.Buff["Elixir of Speed IX"].ID} && !${Me.Buff["Elixir of Speed X"].ID}) /varset str ${str} HASTE
	/if ((${HasMAG} || ${HasDRU}) && (${TankClass} || (!${CasterClass} && !${Raid.Members})) && ${Me.Class.Name.NotEqual["Ranger"]} && !${Me.Buff["Fireskin"].ID} && !${Me.Buff["Circle of Fireskin"].ID} && !${Me.Buff["Magmaskin"].ID} && !${Me.Buff["Magmaskin Rk. II"].ID} && !${Me.Buff["Magmaskin Rk. III"].ID} && !${Me.Buff["Circle of Magmaskin"].ID} && !${Me.Buff["Circle of Magmaskin Rk. II"].ID} && !${Me.Buff["Circle of Magmaskin Rk. III"].ID} && !${Me.Buff["Aspect of Elemental Agony"].ID} && !${Me.Buff["Aspect of Elemental Agony Rk. II"].ID} && !${Me.Buff["Aspect of Elemental Agony Rk. III"].ID} && !${Me.Buff["Viridifloral Shield"].ID} && !${Me.Buff["Viridifloral Shield Rk. II"].ID} && !${Me.Buff["Viridifloral Shield Rk. III"].ID} && !${Me.Buff["Legacy of Viridiflora"].ID} && !${Me.Buff["Legacy of Viridiflora Rk. III"].ID} && !${Me.Buff["Legacy of Viridiflora Rk. III"].ID} && !${Me.Buff["Nettle Shield"].ID} && !${Me.Buff["Circle of Nettles"].ID}) /varset str ${str} DS
	/if (${HasRNG} && ${TankClass} && !${Me.Buff["Guard of the Earth"].ID}) /varset str ${str} GotE
	/if (${HasRNG} && ${Me.Class.Name.NotEqual["Ranger"]} && !${CasterClass} && !${Me.Buff["Snarl of the Predator"].ID} && !${Me.Buff["Snarl of the Predator Rk. II"].ID} && !${Me.Buff["Snarl of the Predator Rk. III"].ID} && !${Me.Buff["Howl of the Predator"].ID} && !${Me.Buff["Call of the Predator"].ID} && !${Me.Buff["Spirit of the Predator"].ID}) /varset str ${str} PREDATOR

	/if ((${HasSHM} || ${HasDRU}) && !${CasterClass} && !${Me.Buff["Champion"].ID} && !${Me.Buff["Lion's Strength"].ID} && !${Me.Buff["Mammoth's Strength"].ID} && !${Me.Buff["Mammoth's Strength Rk. II"].ID} && !${Me.Buff["Mammoth's Strength Rk. III"].ID} && !${Me.Buff["Talisman of Might"].ID} && !${Me.Buff["Spirit of Might"].ID}) {
		/if (${HasDRU}) {
			/varset str ${str} MAMMOTH
		} else {
			/varset str ${str} MIGHT
		}
	}

	/if (${HasSHM} && !${Me.Buff["Preternatural Foresight"].ID} && !${Me.Buff["Preternatural Foresight Rk. II"].ID} && !${Me.Buff["Preternatural Foresight Rk. III"].ID} && !${Me.Buff["Talisman of Foresight"].ID} && !${Me.Buff["Talisman of Foresight Rk. II"].ID} && !${Me.Buff["Talisman of Foresight Rk. III"].ID} && !${Me.Buff["Talisman of Sense"].ID} && !${Me.Buff["Spirit of Sense"].ID}) /varset str ${str} SENSE
	/if (${HasSHM} && !${Me.Buff["Talisman of Fortitude"].ID} && !${Me.Buff["Spirit of Fortitude"].ID}) /varset str ${str} FORTITUDE
	/if (${HasCLR} && (!${Me.Buff["Ward of Valiance"].ID} && !${Me.Buff["Ward of Gallantry"].ID}) && (${Me.Buff["Symbol of Balikor"].ID} || ${Me.Buff["Balikor's Mark"].ID} || ${Me.Buff["Symbol of Kazad"].ID} || ${Me.Buff["Kazad`s Mark"].ID}) && (${Me.Buff["Steeloak Skin"].ID} || ${Me.Buff["Blessing of Steeloak"].ID} || ${Me.Buff["Blessing of the Nine"].ID} || ${Me.Buff["Protection of the Nine"].ID} || ${Me.Buff["Direwild Skin"].ID} || ${Me.Buff["Direwild Skin Rk. II"].ID} || ${Me.Buff["Direwild Skin Rk. III"].ID} || ${Me.Buff["Blessing of the Direwild"].ID} || ${Me.Buff["Blessing of the Direwild Rk. II"].ID} || ${Me.Buff["Blessing of the Direwild Rk. III"].ID})) /varset str ${str} WOV
	/if (${Me.Class.Name.Equal["Cleric"]} && !${Me.Buff["Armor of the Sacred"].ID} && !${Me.Buff["Armor of the Sacred Rk. II"].ID} && !${Me.Buff["Armor of the Sacred Rk. III"].ID} && !${Me.Buff["Armor of the Pious"].ID} && !${Me.Buff["Armor of the Zealot"].ID} && ${Me.Buff["Tenacity"].ID} || ${Me.Buff["Tenacity Rk. II"].ID} || ${Me.Buff["Tenacity Rk. III"].ID} || ${Me.Buff["Hand of Tenacity"].ID} || ${Me.Buff["Hand of Tenacity Rk. II"].ID} || ${Me.Buff["Hand of Tenacity Rk. III"].ID} || (${Me.Buff["Conviction"].ID} || ${Me.Buff["Hand of Conviction"].ID}) ) /varset str ${str} ARMOR
	/if (${Me.Class.Name.Equal["Druid"]} || ${Me.Class.Name.Equal["Ranger"]} && ${Zone.ShortName.NotEqual[Tacvi]} && !${Me.Buff["Briarcoat"].ID} && !${Me.Buff["Nettlecoat"].ID}) /varset str ${str} COAT

	/if (!${str.Length}) /varset str OK
	/varset mcHUDmeleebuffs ${str}

	|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
	|caster buffs (C6+SA+spell haste) + class specific self buffs
	|XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
	/varset str
	/if (${HasCLR} && !${MeleeClass} && !${Me.Buff["Aura of Purpose"].ID} && !${Me.Buff["Aura of Purpose Rk. II"].ID} && !${Me.Buff["Aura of Purpose Rk. III"].ID} && !${Me.Buff["Aura of Devotion"].ID} && !${Me.Buff["Blessing of Devotion"].ID} && !${Me.Buff["Aura of Reverence"].ID} && !${Me.Buff["Blessing of Reverence"].ID}) /varset str ${str} SPELLHASTE
	/if (${HasBST} && !${MeleeClass} && !${Me.Buff["Spiritual Enlightenment"].ID} && !${Me.Buff["Spiritual Enlightenment Rk. II"].ID} && !${Me.Buff["Spiritual Enlightenment Rk. III"].ID} && !${Me.Buff["Spiritual Ascendance"].ID} && !${Me.Buff["Spiritual Dominion"].ID}) /varset str ${str} SE
	/if (${HasENC} && !${MeleeClass} && !${Me.Buff["Seer's Intuition"].ID} && !${Me.Buff["Seer's Intuition Rk. II"].ID} && !${Me.Buff["Seer's Intuition Rk. III"].ID} && !${Me.Buff["Voice of Intuition"].ID} && !${Me.Buff["Voice of Intuition Rk. II"].ID} && !${Me.Buff["Voice of Intuition Rk. III"].ID} && !${Me.Buff["Clairvoyance"].ID} && !${Me.Buff["Voice of Clairvoyance"].ID} && !${Me.Buff["Voice of Quellious"].ID} && !${Me.Buff["Tranquility"].ID} && !${Me.Buff["Koadic's Endless Intellect"].ID} && !${Me.Buff["Elixir of Clarity X"].ID}) /varset str ${str} CRACK
	/if (${Me.Class.Name.Equal["Druid"]} || ${Me.Class.Name.Equal["Ranger"]} && !${Me.Buff["Mask of the Forest"].ID} && !${Me.Buff["Mask of the Stalker"].ID} && !${Me.Buff["Mask of the Wild"].ID} && !${Me.Buff["Eyes of the Owl"].ID}) /varset str ${str} MASK
		
	/if (!${str.Length}) /varset str OK
	/varset mcHUDcasterbuffs ${str}


	|XXXXXXXXXXXXXXXXXXXXXXXXXXX
	|resist buffs
	|XXXXXXXXXXXXXXXXXXXXXXXXXXX
	/varset str
	/if (${HasNEC} && ${ZoneAllowsLevitate} && !${Me.Buff["Dead Men Floating"].ID} && !${Me.Buff["Dead Man Floating"].ID}) /varset str ${str} DMF
	
	/if ((${HasSHM} || ${HasBST}) && !${Me.Buff["Protection of Wishka"].ID} && !${Me.Buff["Protection of Wishka Rk. II"].ID} && !${Me.Buff["Protection of Wishka Rk. III"].ID} && !${Me.Buff["Talisman of the Tribunal"].ID} && !${Me.Buff["Talisman of Jasinth"].ID}) {
		/if (${HasSHM}) {
			/varset str ${str} DR/PR
		} else {
			/varset str ${str} Jasinth
		}
	}

	/if (${HasENC} && !${Me.Buff["Guard of Druzzil"].ID}) /varset str ${str} GoD
	/if (${HasDRU} && !${Me.Buff["Protection of Seasons"].ID}) /varset str ${str} PoS
	/if ((${HasDRU} || ${HasCLR}) && !${Me.Buff["Shared Purity"].ID} && !${Me.Buff["Shared Purity Rk. II"].ID} && !${Me.Buff["Shared Purity Rk. III"].ID} && !${Me.Buff["Resist Corruption"].ID} && !${Me.Buff["Resist Corruption Rk. II"].ID} && !${Me.Buff["Resist Corruption Rk. III"].ID}) /varset str ${str} CORRUPT
	/if ((${HasDRU} || ${HasRNG}) && ${Raid.Members} && !${Me.Buff["Circle of Summer"].ID}) /varset str ${str} SUMMER
	
	/if (!${str.Length}) /varset str OK
	/varset mcHUDresistbuffs ${str}

/return
