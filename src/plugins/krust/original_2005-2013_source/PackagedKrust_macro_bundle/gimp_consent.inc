|auto memorize 1 spell set upon death
#define SPELL_SET_ON_DEATH selfbuffs

#Event YouHaveDied		"You have been slain by#*#"
#Event YouHaveDied		"You died.#*#"

|fixme: move out dps count stuff from this file

Sub Event_YouHaveDied
	/echo [${Time}] You died at ${Me.Y}, ${Me.X}, ${Me.Z}!

	/if (${totHeal}) /echo - You healed for ${totHeal} HP before you died.
	/if (${totHealedMe}) /echo - Was healed for ${totHealedMe} HP before you died.
	/if (${totDmg}) /echo - You made ${totDmg} damage before you died.
	/if (${totDmgTaken}) /echo - Took ${totDmgTaken} points of melee damage (fixme: spell dmg)

	|Reset dps counters
	/varset totHeal 0
	/varset totHealedMe 0
	/varset totDmg 0
	/varset totDmgTaken 0
	/delay 15s ${Me.ID}

	|auto loads a spell set after death
	/delay 5s
	/if (${Me.Class.Name.NotEqual[Warrior]} && ${Me.Class.Name.NotEqual[Monk]} && ${Me.Class.Name.NotEqual[Rogue]} && ${Me.Class.Name.NotEqual[Berserker]}) /memspellset SPELL_SET_ON_DEATH
/return


|orginally borrowed lootmycorpse code from ARCH.MAC: http://www.macroquest2.com/phpBB2/viewtopic.php?t=9356
|modified by gimp
#Event LootMyCorpse        "[MQ2] lootmycorpse" 
Sub Event_LootMyCorpse(string Line)
	/declare lootTotal int local 0
	/declare lootSlot int local 0
	/declare skippedCount int local 0
	/declare wornSlotNumber int local 0
	
	/if (!${SpawnCount[${Me.Name}'s corpse]}) {
		/echo # You have no corpses in the zone!
		/return	
	}

	/echo # You have ${SpawnCount[${Me.Name}'s corpse]} corpses in zone, looting nearest...

	/target mycorpse
	/delay 2s
	

	/if (${Target.Type.NotEqual[CORPSE]}) {
		/echo * Error: Can't target my corpse.
		/return
	}

	/echo # Looting my corpse

	/corpse
	/delay 1s
	/loot
	/delay 1s

	/if (${Me.State.NotEqual[BIND]}) {
		/echo * Debug: Error in getting onto corpse! Aborting.
		/return
	}

	:lootLag
	/varset lootTotal ${Corpse.Items}   
	/delay 2
	/if (${lootTotal} != ${Corpse.Items}) /goto :lootLag

	/delay 1s
	:doAgain
	/if (${Corpse.Items} > ${lootTotal}) /goto :lootLag
	/echo # Looting ${lootTotal} items. (corpse has ${Corpse.Items} items)
	/for lootSlot 1 to ${lootTotal}
		:lootItem
		/if (${Corpse.Item[${lootSlot}].ID}) {
			/nomodkey /itemnotify loot${lootSlot} rightmouseup
			/delay 2
			/goto :lootItem
		}
		/delay 2
	/next lootSlot

	/delay 5
	/echo * Done looting my corpse.
	/notify LootWnd DoneButton leftmouseup
	
	/delay 20
	
	|Refresh clickies
	/if (${gimp_clickies_activated}) /call DoClickies

	|Uses mod rod if possible
	/if (${Me.MaxMana} && (${Me.PctMana} <= 10) && (${Me.CurrentHPs} >= 1500) && ${FindItem["Rod of Mystical Transvergance"].InvSlot} && !${FindItem["Rod of Mystical Transvergance"].Timer}) {
		/call Cast "Rod of Mystical Transvergance" item
		/echo # Clicking modrod
	}
	
	/if (${Me.MaxMana} && (${Me.PctMana} <= 10) && (${Me.CurrentHPs} >= 1500) && ${FindItem["Wand of Elemental Transvergance "].InvSlot} && !${FindItem["Wand of Elemental Transvergance "].Timer}) {
		/call Cast "Wand of Elemental Transvergance " item
		/echo # Clicking modrod
	}

	/if (${Me.Standing} && ${Me.MaxMana} && !${Me.Moving} && !${Me.Casting.ID}) /sit
/return
