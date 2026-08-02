|must call Init_DPS before main loop of program
|exports data to HUD:
|	 mcHUDlastDS - last ammount of DS a mob was hit for

|DPS counter events:
#Event DamageNuke		"#1# hit #2# for #3# points of non-melee damage."
#Event DamageUnknown "#1# was hit by non-melee for #2# points of damage."
#Event DamageDoT		"#*#has taken #1# damage from your#*#"
#Event Heal				"You have healed #1# for #2# points of damage."
#Event DamageDS			"#1# was hit by non-melee for #2# points of damage."
#Event HealedMe			"#1# has healed you for #2# points of damage."
#event CritBlast		"You deliver a critical blast! (#1#)"

|you make melee damage (addverbated form):
#Event DamageMelee		"#*#crush #*# for #1#"
#Event DamageMelee		"#*#punch #*# for #1#"
#Event DamageMelee		"#*#pierce #*# for #1#"
#Event DamageMelee		"#*#slash #*# for #1#"
#Event DamageMelee		"#*#backstab #*# for #1#"
#Event DamageMelee		"#*#bash #*# for #1#"
#Event DamageMelee		"#*#kick #*# for #1#"
#Event DamageMelee		"#*#strike #*# for #1#"

|Uncomment the below lines to count arrow damage, to make dps numbers more fair for pullers, these are commented out
|#Event DamageMelee		"hit #*# for #1#"
|#Event DamageMelee		"You hit #*# for #1# point of damage."
|#Event DamageMelee		"You hit #*# for #1# points of damage."


|you take damage (addverbated form):
|"mob hits for xx"
#Event DamageMeleeTaken "#1# hits for #2#"
#Event DamageMeleeTaken "#1# bashes for #2#"
#Event DamageMeleeTaken "#1# slashes for #2#"
#Event DamageMeleeTaken "#1# backstabs for #2#"
#event DamageMeleeTaken "#1# smashes for #2#"
#Event DamageMeleeTaken "#1# kicks for #2#"
#Event DamageMeleeTaken "#1# pierces for #2#"
#Event DamageMeleeTaken "#1# bites for #2#"
#Event DamageMeleeTaken "#1# claws for #2#"
#Event DamageMeleeTaken "#1# crushes for #2#"

#event DamageByOthers "#1# crushes #*# for #2#"
#event DamageByOthers "#1# bashes #*# for #2#"
#event DamageByOthers "#1# kicks #*# for #2#"
#event DamageByOthers "#1# backstabs #*# for #2#"
#event DamageByOthers "#1# slashes #*# for #2#"
#event DamageByOthers "#1# pierces #*# for #2#"
#event DamageByOthers "#1# hits #*# for #2#"

Sub Event_DamageMeleeTaken(string Line, string Name, int Value)
		|/echo # Was hit for ${Value} by ${Name}
		/varcalc totDmgTaken ${totDmgTaken}+${Value}
		/return

#Event DPS_XP			"You gain experience#*#"
#Event DPS_XP			"You gain party experience#*#"
#Event DPS_XP			"You gained raid experience#*#"

Sub Init_DPS
	/declare topNuke	int outer ${Ini[INI_FILE, ${Me.Name}, TopNuke, NOT_FOUND]}
	/declare topDoT		int outer ${Ini[INI_FILE, ${Me.Name}, TopDoT, NOT_FOUND]}
	/declare topMelee	int outer ${Ini[INI_FILE, ${Me.Name}, TopMelee, NOT_FOUND]}
	/declare topHeal	int outer ${Ini[INI_FILE, ${Me.Name}, TopHeal, NOT_FOUND]}
	/declare topDPS		float outer ${Ini[INI_FILE, ${Me.Name}, TopDPS, NOT_FOUND]}
	/declare mcHUDlastDS	int outer	0

	/declare gimp_dps_fightstarted int outer 0
	/declare gimp_dps_fight_time int outer 0
	/declare gimp_dps_current_dps float outer 0
	/declare gimp_dps_hits	int outer 0
	/declare gimp_dps_last_hit int outer 0

	|Will count how much i healed until mob is dead
	/declare totHeal		int outer 0
	/declare totHealedMe	int outer 0
	/declare totDmg			int outer 0
	/declare totDmgUnknown int outer 0
	/declare totDmgTaken int outer 0

	/echo gimp_dps.inc initialized
	/echo * Best nuke: ${topNuke}, dot: ${topDoT}, melee: ${topMelee}, heal: ${topHeal}, dps: ${topDPS}

/return

Sub Event_DamageByOthers(string line, string Name, int value)

	/if (${Me.Pet.CleanName.Equal[${Name}]}) {
 		|/echo melee: ${Name} ${value}
		/call CalcDPS ${value}

		/if (${value} > ${topMelee}) {

			/echo Melee highschore: ${value} (old was ${topMelee})
			/varset topMelee ${value}
			/ini "INI_FILE" "${Me.Name}" "TopMelee" "${topMelee}"
			}
		}
  |/echo ${Name.Token[1, ]}
	/if (${Name.Token[1, ].Equal[${Me.Name}`s]}) {
 		|/echo melee: ${Name} ${value}
		/call CalcDPS ${value}

		/if (${value} > ${topMelee}) {

			/echo Melee highschore: ${value} (old was ${topMelee})
			/varset topMelee ${value}
			/ini "INI_FILE" "${Me.Name}" "TopMelee" "${topMelee}"
			}
		}
/return

Sub CalcDPS(int value)

	/if (${totDmg} == 0) {
		/echo [${Time}] Fight started
		/varset gimp_dps_fightstarted ${Macro.RunTime}
		/varset gimp_dps_hits 0
	}
	/varcalc totDmg ${totDmg}+${value}
	/varcalc gimp_dps_hits ${gimp_dps_hits}+1

	/varcalc gimp_dps_fight_time ${Macro.RunTime}-${gimp_dps_fightstarted}
	/varset gimp_dps_last_hit ${Macro.RunTime}

	/if (${gimp_dps_fight_time}) {
		/varcalc gimp_dps_current_dps ${totDmg}/${gimp_dps_fight_time}
	}

/return

|todo: skriv om dps calc: spara tid vid varje "senaste" hit, räkna dps från första till sissta hit

Sub Event_DPS_XP

	/if (${totDmg} && ${gimp_dps_fight_time}) {
		/declare gimp_dps_hits_per_sec float local ${Math.Calc[${gimp_dps_hits}/${gimp_dps_fight_time}]}
		/echo - Dealt ${totDmg} damage over ${gimp_dps_fight_time}s in ${gimp_dps_hits} hits (${gimp_dps_current_dps} DPS, ${gimp_dps_hits_per_sec} hits per sec)
		
		/if (${gimp_dps_current_dps} > ${topDPS}) {
			/if (${gimp_dps_fight_time} > 20 && (${gimp_dps_hits} > 5)) {
				|Only log dps highscores that is taken over some time (20 sec), to make result more accurate
				/echo # New DPS highscore: ${gimp_dps_current_dps} (old was ${topDPS})
				/varset topDPS ${gimp_dps_current_dps}
				/ini "INI_FILE" "${Me.Name}" "TopDPS" "${topDPS}"
			}
		}
	}
	/if (${totDmgUnknown}) {
		/echo - ${totDmgUnknown} unknown damage was made (DS & such)
	}
	/if (${totHeal} && ${totHealedMe}) {
		/echo - Healed ${totHeal} HP, was healed for ${totHealedMe} HP
	} else {
		/if (${totHeal}) {
			/echo - Healed ${totHeal} HP
		}
		/if (${totHealedMe}) {
			/echo - Was healed for ${totHealedMe} HP
		}
	}
	/if (${totDmgTaken}) {
		/echo - Took ${totDmgTaken} points of melee damage (fixme: spell dmg)
	}
	/echo -----------------------------------

	/varset totHeal 0
	/varset totHealedMe 0
	/varset totDmg 0
	/varset totDmgUnknown 0
	/varset totDmgTaken 0

/return

Sub Event_Heal(string line, string name, int value)

	/if (${Me.Name.Equal[${name}]}) {
		/varcalc totHealedMe ${totHealedMe}+${value}
	}

	/if (${value} > 1000) {
		/echo Heal: ${name} -> ${value} HP
	}
	
	/varcalc totHeal ${totHeal}+${value}

	/if (${value} > ${topHeal}) {
		/echo Heal highschore: ${value} (old was ${topHeal})
		/varset topHeal ${value}
		/ini "INI_FILE" "${Me.Name}" "TopHeal" "${topHeal}"
	}
/return

Sub Event_DamageMelee(string line, int value)

	|/echo melee: ${value}
	
	/call CalcDPS ${value}

	/if (${value} > ${topMelee}) {

		/echo Melee highschore: ${value} (old was ${topMelee})
		/varset topMelee ${value}
		/ini "INI_FILE" "${Me.Name}" "TopMelee" "${topMelee}"
	}
/return

Sub Event_DamageNuke(string line, string Name, string mob, int value)
  /declare intIsPet int local 0
  |/echo ${Name} is nuking for ${value}
	/if ((${Me.Name.Equal[${Name}]} && ${Me.Name.NotEqual[${mob}]}) || (${Me.Pet.CleanName.Equal[${Name}]} || ${Name.Token[1, ].Equal[${Me.Name}`s]})) {
		/if ((${Me.Pet.CleanName.Equal[${Name}]} || ${Name.Token[1, ].Equal[${Me.Name}`s]})) /varset intIsPet 1

		/call CalcDPS ${value}

		/if ((${value} > ${topNuke}) && (${value} != 20000) && ((${value} < 32000) || (${value} > 32100))) {
			/echo Nuke highschore: ${value} (old was ${topNuke})
			/popup New High Nuke: ${value} (old was ${topNuke})
			/varset topNuke ${value}
			/ini "INI_FILE" "${Me.Name}" "TopNuke" "${topNuke}"
			}
		/if (${value} >= ${Math.Calc[50*${Me.Level}]}) {
			/if (${intIsPet}) {
				/echo Nuke (Pet): ${value}
				} else {
				/if ((${value} < 32000) || (${value} > 32200)) {
					/echo Nuke: ${value}
				} else {
					/echo Nuke: BAM! ${value}
					/popup BAM (${value})
				}
				}
			}
		}

/return

Sub Event_DamageDoT(string line, int value)

	|/echo dot: ${value}
	/call CalcDPS ${value}
	
	/if (${value} > ${topDoT}) {
		/echo DoT highschore: ${value} (old was ${topDoT})
		/varset topDoT ${value}
		/ini "INI_FILE" "${Me.Name}" "TopDoT" "${topDoT}"
	}
	
	/if ((${value} > 100) && !${Raid.Members}) {
		/echo DoT: ${value}
	}

/return


Sub Event_DamageUnknown(string line, string name, int value)

	|This event triggers from Damage Shield, probably more
	/if (${Spawn[${name}].Type.Equal["NPC"]}) {

		/varcalc totDmgUnknown ${totDmgUnknown}+${value}
		|/echo # Unknown dmg: ${name}: ${value}

	}
	
/return


Sub Event_HealedMe(string line, string name, int value)
	|Ignore spam from smaller heals
	/if (${value} > 300) {
		/echo ${name} healed ME for ${value}
	}
	
	/varcalc totHealedMe ${totHealedMe}+${value}
/return

Sub Event_DamageDS(string line, string name, int value)

	|/echo ${name} hit by DS for ${value}

	|mark of the righteous 34
	|This will work for most ppl, can't really make a better fix
	/if ((${value} != 34) && (${value} != 45)) {
		|ignore cleric reverse DS, this is to report my own DS
		/varset mcHUDlastDS ${value}
	}

/return

Sub Event_CritBlast(string line, int blast)
	/echo Critical Spell Hit for ${blast}.
/return