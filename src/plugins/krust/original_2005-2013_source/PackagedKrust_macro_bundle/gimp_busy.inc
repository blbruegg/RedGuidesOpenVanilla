| just a sub that does various checks to make sure the player isnt busy doing something atm. used by buffrequests & clickies modules

Sub AmIBusy

	/declare busy int local 0
	
	|sitting not considered being busy

	/if (${Me.Invis} || ${Me.Feigning} || ${Me.Moving} || ${Me.AFK} || ${Cursor.ID} || ${Me.Casting.ID} || ${Window[SpellBookWnd].Open} || ${Window[BigBankWnd].Open} || ${Window[LootWnd].Open} || ${Window[TradeWnd].Open} || ${Window[MerchantWnd].Open} || ${Window[TradeskillWnd].Open} || ${Window[BarterWnd].Open}) {
		/varset busy 1
	}

/return ${busy}