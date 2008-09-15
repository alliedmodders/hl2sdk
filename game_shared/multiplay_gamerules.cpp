//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains the implementation of game rules for multiplayer.
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "multiplay_gamerules.h"
#include "viewport_panel_names.h"
#include "gameeventdefs.h"
#include <KeyValues.h>

#ifdef CLIENT_DLL

#else

	#include "eventqueue.h"
	#include "player.h"
	#include "basecombatweapon.h"
	#include "gamerules.h"
	#include "game.h"
	#include "items.h"
	#include "entitylist.h"
	#include "in_buttons.h"
	#include <ctype.h>
	#include "voice_gamemgr.h"
	#include "iscorer.h"
	#include "hltvdirector.h"
	
#endif

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


REGISTER_GAMERULES_CLASS( CMultiplayRules );

ConVar mp_chattime(
		"mp_chattime", 
		"10", 
		FCVAR_REPLICATED,
		"amount of time players can chat after the game is over",
		true, 1,
		true, 120 );

ConVar	mp_timelimit( "mp_timelimit",
					  "0",
					  FCVAR_NOTIFY|FCVAR_REPLICATED,
					  "game time per map in minutes" );

#ifdef GAME_DLL

ConVar	tv_delaymapchange( "tv_delaymapchange",
					 "0",
					 0,
					 "Delays map change until broadcast is complete" );
					  					  
#endif


//=========================================================
//=========================================================
bool CMultiplayRules::IsMultiplayer( void )
{
	return true;
}


#ifdef CLIENT_DLL


#else 

	extern bool			g_fGameOver;

	#define ITEM_RESPAWN_TIME	30
	#define WEAPON_RESPAWN_TIME	20
	#define AMMO_RESPAWN_TIME	20

	//*********************************************************
	// Rules for the half-life multiplayer game.
	//*********************************************************

	CMultiplayRules::CMultiplayRules()
	{
		RefreshSkillData( true );
		
		// 11/8/98
		// Modified by YWB:  Server .cfg file is now a cvar, so that 
		//  server ops can run multiple game servers, with different server .cfg files,
		//  from a single installed directory.
		// Mapcyclefile is already a cvar.

		// 3/31/99
		// Added lservercfg file cvar, since listen and dedicated servers should not
		// share a single config file. (sjb)
		if ( engine->IsDedicatedServer() )
		{
			// dedicated server
			const char *cfgfile = servercfgfile.GetString();

			if ( cfgfile && cfgfile[0] )
			{
				char szCommand[256];
				
				Msg( "Executing dedicated server config file\n" );
				Q_snprintf( szCommand,sizeof(szCommand), "exec %s\n", cfgfile );
				engine->ServerCommand( szCommand );
			}
		}
		else
		{
			// listen server
			const char *cfgfile = lservercfgfile.GetString();

			if ( cfgfile && cfgfile[0] )
			{
				char szCommand[256];
				
				Msg( "Executing listen server config file\n" );
				Q_snprintf( szCommand,sizeof(szCommand), "exec %s\n", cfgfile );
				engine->ServerCommand( szCommand );
			}
		}
	}

	//=========================================================
	//=========================================================
	void CMultiplayRules::RefreshSkillData( bool forceUpdate )
	{
	// load all default values
		BaseClass::RefreshSkillData( forceUpdate );

	// override some values for multiplay.

		// suitcharger
		ConVar *suitcharger = ( ConVar * )cvar->FindVar( "sk_suitcharger" );
		if ( suitcharger )
		{
			suitcharger->SetValue( 30 );
		}
	}


	//=========================================================
	//=========================================================
	void CMultiplayRules::Think ( void )
	{
		BaseClass::Think();
		
		///// Check game rules /////

		if ( g_fGameOver )   // someone else quit the game already
		{
			ChangeLevel(); // intermission is over
			return;
		}

		float flTimeLimit = mp_timelimit.GetFloat() * 60;
		float flFragLimit = fraglimit.GetFloat();
		
		if ( flTimeLimit != 0 && gpGlobals->curtime >= flTimeLimit )
		{
			GoToIntermission();
			return;
		}

		if ( flFragLimit )
		{
			// check if any player is over the frag limit
			for ( int i = 1; i <= gpGlobals->maxClients; i++ )
			{
				CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

				if ( pPlayer && pPlayer->FragCount() >= flFragLimit )
				{
					GoToIntermission();
					return;
				}
			}
		}
	}


	//=========================================================
	//=========================================================
	bool CMultiplayRules::IsDeathmatch( void )
	{
		return true;
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::IsCoOp( void )
	{
		return false;
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::FShouldSwitchWeapon( CBasePlayer *pPlayer, CBaseCombatWeapon *pWeapon )
	{
		if ( !pPlayer->Weapon_CanSwitchTo( pWeapon ) )
		{
			// Can't switch weapons for some reason.
			return false;
		}

		if ( !pPlayer->GetActiveWeapon() )
		{
			// Player doesn't have an active item, might as well switch.
			return true;
		}

		if ( !pWeapon->AllowsAutoSwitchTo() )
		{
			// The given weapon should not be auto switched to from another weapon.
			return false;
		}

		if ( !pPlayer->GetActiveWeapon()->AllowsAutoSwitchFrom() )
		{
			// The active weapon does not allow autoswitching away from it.
			return false;
		}

		if ( pWeapon->GetWeight() > pPlayer->GetActiveWeapon()->GetWeight() )
		{
			return true;
		}

		return false;
	}

	//-----------------------------------------------------------------------------
	// Purpose: Returns the weapon in the player's inventory that would be better than
	//			the given weapon.
	//-----------------------------------------------------------------------------
	CBaseCombatWeapon *CMultiplayRules::GetNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
	{
		CBaseCombatWeapon *pCheck;
		CBaseCombatWeapon *pBest;// this will be used in the event that we don't find a weapon in the same category.

		int iCurrentWeight = -1;
		int iBestWeight = -1;// no weapon lower than -1 can be autoswitched to
		pBest = NULL;

		// If I have a weapon, make sure I'm allowed to holster it
		if ( pCurrentWeapon )
		{
			if ( !pCurrentWeapon->AllowsAutoSwitchFrom() || !pCurrentWeapon->CanHolster() )
			{
				// Either this weapon doesn't allow autoswitching away from it or I
				// can't put this weapon away right now, so I can't switch.
				return NULL;
			}

			iCurrentWeight = pCurrentWeapon->GetWeight();
		}

		for ( int i = 0 ; i < pPlayer->WeaponCount(); ++i )
		{
			pCheck = pPlayer->GetWeapon( i );
			if ( !pCheck )
				continue;

			// If we have an active weapon and this weapon doesn't allow autoswitching away
			// from another weapon, skip it.
			if ( pCurrentWeapon && !pCheck->AllowsAutoSwitchTo() )
				continue;

			if ( pCheck->GetWeight() > -1 && pCheck->GetWeight() == iCurrentWeight && pCheck != pCurrentWeapon )
			{
				// this weapon is from the same category. 
				if ( pCheck->HasAnyAmmo() )
				{
					if ( pPlayer->Weapon_CanSwitchTo( pCheck ) )
					{
						return pCheck;
					}
				}
			}
			else if ( pCheck->GetWeight() > iBestWeight && pCheck != pCurrentWeapon )// don't reselect the weapon we're trying to get rid of
			{
				//Msg( "Considering %s\n", STRING( pCheck->GetClassname() );
				// we keep updating the 'best' weapon just in case we can't find a weapon of the same weight
				// that the player was using. This will end up leaving the player with his heaviest-weighted 
				// weapon. 
				if ( pCheck->HasAnyAmmo() )
				{
					// if this weapon is useable, flag it as the best
					iBestWeight = pCheck->GetWeight();
					pBest = pCheck;
				}
			}
		}

		// if we make it here, we've checked all the weapons and found no useable 
		// weapon in the same catagory as the current weapon. 
		
		// if pBest is null, we didn't find ANYTHING. Shouldn't be possible- should always 
		// at least get the crowbar, but ya never know.
		return pBest;
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Output : Returns true on success, false on failure.
	//-----------------------------------------------------------------------------
	bool CMultiplayRules::SwitchToNextBestWeapon( CBaseCombatCharacter *pPlayer, CBaseCombatWeapon *pCurrentWeapon )
	{
		CBaseCombatWeapon *pWeapon = GetNextBestWeapon( pPlayer, pCurrentWeapon );

		if ( pWeapon != NULL )
			return pPlayer->Weapon_Switch( pWeapon );
		
		return false;
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::ClientConnected( edict_t *pEntity, const char *pszName, const char *pszAddress, char *reject, int maxrejectlen )
	{
		GetVoiceGameMgr()->ClientConnected( pEntity );
		return true;
	}

	void CMultiplayRules::InitHUD( CBasePlayer *pl )
	{
	} 

	//=========================================================
	//=========================================================
	void CMultiplayRules::ClientDisconnected( edict_t *pClient )
	{
		if ( pClient )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)CBaseEntity::Instance( pClient );

			if ( pPlayer )
			{
				FireTargets( "game_playerleave", pPlayer, pPlayer, USE_TOGGLE, 0 );

				pPlayer->RemoveAllItems( true );// destroy all of the players weapons and items

				// Kill off view model entities
				pPlayer->DestroyViewModels();

				pPlayer->SetConnected( PlayerDisconnected );
			}
		}
	}

	//=========================================================
	//=========================================================
	float CMultiplayRules::FlPlayerFallDamage( CBasePlayer *pPlayer )
	{
		int iFallDamage = (int)falldamage.GetFloat();

		switch ( iFallDamage )
		{
		case 1://progressive
			pPlayer->m_Local.m_flFallVelocity -= PLAYER_MAX_SAFE_FALL_SPEED;
			return pPlayer->m_Local.m_flFallVelocity * DAMAGE_FOR_FALL_SPEED;
			break;
		default:
		case 0:// fixed
			return 10;
			break;
		}
	} 

	//=========================================================
	//=========================================================
	bool CMultiplayRules::AllowDamage( CBaseEntity *pVictim, const CTakeDamageInfo &info )
	{
		return true;
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::FPlayerCanTakeDamage( CBasePlayer *pPlayer, CBaseEntity *pAttacker )
	{
		return true;
	}

	//=========================================================
	//=========================================================
	void CMultiplayRules::PlayerThink( CBasePlayer *pPlayer )
	{
		if ( g_fGameOver )
		{
			// clear attack/use commands from player
			pPlayer->m_afButtonPressed = 0;
			pPlayer->m_nButtons = 0;
			pPlayer->m_afButtonReleased = 0;
		}
	}

	//=========================================================
	//=========================================================
	void CMultiplayRules::PlayerSpawn( CBasePlayer *pPlayer )
	{
		bool		addDefault;
		CBaseEntity	*pWeaponEntity = NULL;

		pPlayer->EquipSuit();
		
		addDefault = true;

		while ( (pWeaponEntity = gEntList.FindEntityByClassname( pWeaponEntity, "game_player_equip" )) != NULL)
		{
			pWeaponEntity->Touch( pPlayer );
			addDefault = false;
		}
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::FPlayerCanRespawn( CBasePlayer *pPlayer )
	{
		return true;
	}

	//=========================================================
	//=========================================================
	float CMultiplayRules::FlPlayerSpawnTime( CBasePlayer *pPlayer )
	{
		return gpGlobals->curtime;//now!
	}

	bool CMultiplayRules::AllowAutoTargetCrosshair( void )
	{
		return ( aimcrosshair.GetInt() != 0 );
	}

	//=========================================================
	// IPointsForKill - how many points awarded to anyone
	// that kills this player?
	//=========================================================
	int CMultiplayRules::IPointsForKill( CBasePlayer *pAttacker, CBasePlayer *pKilled )
	{
		return 1;
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	//-----------------------------------------------------------------------------
	CBasePlayer *CMultiplayRules::GetDeathScorer( CBaseEntity *pKiller, CBaseEntity *pInflictor )
	{
		if ( pKiller)
		{
			if ( pKiller->Classify() == CLASS_PLAYER )
				return (CBasePlayer*)pKiller;

			// Killing entity might be specifying a scorer player
			IScorer *pScorerInterface = dynamic_cast<IScorer*>( pKiller );
			if ( pScorerInterface )
			{
				CBasePlayer *pPlayer = pScorerInterface->GetScorer();
				if ( pPlayer )
					return pPlayer;
			}

			// Inflicting entity might be specifying a scoring player
			pScorerInterface = dynamic_cast<IScorer*>( pInflictor );
			if ( pScorerInterface )
			{
				CBasePlayer *pPlayer = pScorerInterface->GetScorer();
				if ( pPlayer )
					return pPlayer;
			}
		}

		return NULL;
	}

	//=========================================================
	// PlayerKilled - someone/something killed this player
	//=========================================================
	void CMultiplayRules::PlayerKilled( CBasePlayer *pVictim, const CTakeDamageInfo &info )
	{
		DeathNotice( pVictim, info );

		// Find the killer & the scorer
		CBaseEntity *pInflictor = info.GetInflictor();
		CBaseEntity *pKiller = info.GetAttacker();
		CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor );
		
		pVictim->IncrementDeathCount( 1 );

		// dvsents2: uncomment when removing all FireTargets
		// variant_t value;
		// g_EventQueue.AddEvent( "game_playerdie", "Use", value, 0, pVictim, pVictim );
		FireTargets( "game_playerdie", pVictim, pVictim, USE_TOGGLE, 0 );

		// Did the player kill himself?
		if ( pVictim == pScorer )  
		{
			// Players lose a frag for killing themselves
			pVictim->IncrementFragCount( -1 );
		}
		else if ( pScorer )
		{
			// if a player dies in a deathmatch game and the killer is a client, award the killer some points
			pScorer->IncrementFragCount( IPointsForKill( pScorer, pVictim ) );
			
			// Allow the scorer to immediately paint a decal
			pScorer->AllowImmediateDecalPainting();

			// dvsents2: uncomment when removing all FireTargets
			//variant_t value;
			//g_EventQueue.AddEvent( "game_playerkill", "Use", value, 0, pScorer, pScorer );
			FireTargets( "game_playerkill", pScorer, pScorer, USE_TOGGLE, 0 );
		}
		else
		{  
			// Players lose a frag for letting the world kill them
			pVictim->IncrementFragCount( -1 );
		}
	}

	//=========================================================
	// Deathnotice. 
	//=========================================================
	void CMultiplayRules::DeathNotice( CBasePlayer *pVictim, const CTakeDamageInfo &info )
	{
		// Work out what killed the player, and send a message to all clients about it
		const char *killer_weapon_name = "world";		// by default, the player is killed by the world
		int killer_ID = 0;

		// Find the killer & the scorer
		CBaseEntity *pInflictor = info.GetInflictor();
		CBaseEntity *pKiller = info.GetAttacker();
		CBasePlayer *pScorer = GetDeathScorer( pKiller, pInflictor );

		// Custom kill type?
		if ( info.GetCustomKill() )
		{
			killer_weapon_name = GetCustomKillString( info );
			if ( pScorer )
			{
				killer_ID = pScorer->GetUserID();
			}
		}
		else
		{
			// Is the killer a client?
			if ( pScorer )
			{
				killer_ID = pScorer->GetUserID();
				
				if ( pInflictor )
				{
					if ( pInflictor == pScorer )
					{
						// If the inflictor is the killer,  then it must be their current weapon doing the damage
						if ( pScorer->GetActiveWeapon() )
						{
							killer_weapon_name = pScorer->GetActiveWeapon()->GetDeathNoticeName();
						}
					}
					else
					{
						killer_weapon_name = STRING( pInflictor->m_iClassname );  // it's just that easy
					}
				}
			}
			else
			{
				killer_weapon_name = STRING( pInflictor->m_iClassname );
			}

			// strip the NPC_* or weapon_* from the inflictor's classname
			if ( strncmp( killer_weapon_name, "weapon_", 7 ) == 0 )
			{
				killer_weapon_name += 7;
			}
			else if ( strncmp( killer_weapon_name, "NPC_", 8 ) == 0 )
			{
				killer_weapon_name += 8;
			}
			else if ( strncmp( killer_weapon_name, "func_", 5 ) == 0 )
			{
				killer_weapon_name += 5;
			}
		}

		IGameEvent * event = gameeventmanager->CreateEvent( "player_death" );
		if ( event )
		{
			event->SetInt("userid", pVictim->GetUserID() );
			event->SetInt("attacker", killer_ID );
			event->SetInt("priority", 7 );	// HLTV event priority, not transmitted
			
			gameeventmanager->FireEvent( event );
		}

	}

	//=========================================================
	// FlWeaponRespawnTime - what is the time in the future
	// at which this weapon may spawn?
	//=========================================================
	float CMultiplayRules::FlWeaponRespawnTime( CBaseCombatWeapon *pWeapon )
	{
		if ( weaponstay.GetInt() > 0 )
		{
			// make sure it's only certain weapons
			if ( !(pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
			{
				return gpGlobals->curtime + 0;		// weapon respawns almost instantly
			}
		}

		return gpGlobals->curtime + WEAPON_RESPAWN_TIME;
	}

	// when we are within this close to running out of entities,  items 
	// marked with the ITEM_FLAG_LIMITINWORLD will delay their respawn
	#define ENTITY_INTOLERANCE	100

	//=========================================================
	// FlWeaponRespawnTime - Returns 0 if the weapon can respawn 
	// now,  otherwise it returns the time at which it can try
	// to spawn again.
	//=========================================================
	float CMultiplayRules::FlWeaponTryRespawn( CBaseCombatWeapon *pWeapon )
	{
		if ( pWeapon && (pWeapon->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD) )
		{
			if ( gEntList.NumberOfEntities() < (gpGlobals->maxEntities - ENTITY_INTOLERANCE) )
				return 0;

			// we're past the entity tolerance level,  so delay the respawn
			return FlWeaponRespawnTime( pWeapon );
		}

		return 0;
	}

	//=========================================================
	// VecWeaponRespawnSpot - where should this weapon spawn?
	// Some game variations may choose to randomize spawn locations
	//=========================================================
	Vector CMultiplayRules::VecWeaponRespawnSpot( CBaseCombatWeapon *pWeapon )
	{
		return pWeapon->GetAbsOrigin();
	}

	//=========================================================
	// WeaponShouldRespawn - any conditions inhibiting the
	// respawning of this weapon?
	//=========================================================
	int CMultiplayRules::WeaponShouldRespawn( CBaseCombatWeapon *pWeapon )
	{
		if ( pWeapon->HasSpawnFlags( SF_NORESPAWN ) )
		{
			return GR_WEAPON_RESPAWN_NO;
		}

		return GR_WEAPON_RESPAWN_YES;
	}

	//=========================================================
	// CanHaveWeapon - returns false if the player is not allowed
	// to pick up this weapon
	//=========================================================
	bool CMultiplayRules::CanHavePlayerItem( CBasePlayer *pPlayer, CBaseCombatWeapon *pItem )
	{
		if ( weaponstay.GetInt() > 0 )
		{
			if ( pItem->GetWeaponFlags() & ITEM_FLAG_LIMITINWORLD )
				return BaseClass::CanHavePlayerItem( pPlayer, pItem );

			// check if the player already has this weapon
			for ( int i = 0 ; i < pPlayer->WeaponCount() ; i++ )
			{
				if ( pPlayer->GetWeapon(i) == pItem )
				{
					return false;
				}
			}
		}

		return BaseClass::CanHavePlayerItem( pPlayer, pItem );
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::CanHaveItem( CBasePlayer *pPlayer, CItem *pItem )
	{
		return true;
	}

	//=========================================================
	//=========================================================
	void CMultiplayRules::PlayerGotItem( CBasePlayer *pPlayer, CItem *pItem )
	{
	}

	//=========================================================
	//=========================================================
	int CMultiplayRules::ItemShouldRespawn( CItem *pItem )
	{
		if ( pItem->HasSpawnFlags( SF_NORESPAWN ) )
		{
			return GR_ITEM_RESPAWN_NO;
		}

		return GR_ITEM_RESPAWN_YES;
	}


	//=========================================================
	// At what time in the future may this Item respawn?
	//=========================================================
	float CMultiplayRules::FlItemRespawnTime( CItem *pItem )
	{
		return gpGlobals->curtime + ITEM_RESPAWN_TIME;
	}

	//=========================================================
	// Where should this item respawn?
	// Some game variations may choose to randomize spawn locations
	//=========================================================
	Vector CMultiplayRules::VecItemRespawnSpot( CItem *pItem )
	{
		return pItem->GetAbsOrigin();
	}

	//=========================================================
	// What angles should this item use to respawn?
	//=========================================================
	QAngle CMultiplayRules::VecItemRespawnAngles( CItem *pItem )
	{
		return pItem->GetAbsAngles();
	}

	//=========================================================
	//=========================================================
	void CMultiplayRules::PlayerGotAmmo( CBaseCombatCharacter *pPlayer, char *szName, int iCount )
	{
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::IsAllowedToSpawn( CBaseEntity *pEntity )
	{
	//	if ( pEntity->GetFlags() & FL_NPC )
	//		return false;

		return true;
	}


	//=========================================================
	//=========================================================
	float CMultiplayRules::FlHealthChargerRechargeTime( void )
	{
		return 60;
	}


	float CMultiplayRules::FlHEVChargerRechargeTime( void )
	{
		return 30;
	}

	//=========================================================
	//=========================================================
	int CMultiplayRules::DeadPlayerWeapons( CBasePlayer *pPlayer )
	{
		return GR_PLR_DROP_GUN_ACTIVE;
	}

	//=========================================================
	//=========================================================
	int CMultiplayRules::DeadPlayerAmmo( CBasePlayer *pPlayer )
	{
		return GR_PLR_DROP_AMMO_ACTIVE;
	}

	CBaseEntity *CMultiplayRules::GetPlayerSpawnSpot( CBasePlayer *pPlayer )
	{
		CBaseEntity *pentSpawnSpot = BaseClass::GetPlayerSpawnSpot( pPlayer );	

	//!! replace this with an Event
	/*
		if ( IsMultiplayer() && pentSpawnSpot->m_target )
		{
			FireTargets( STRING(pentSpawnSpot->m_target), pPlayer, pPlayer, USE_TOGGLE, 0 ); // dvsents2: what is this code supposed to do?
		}
	*/

		return pentSpawnSpot;
	}


	//=========================================================
	//=========================================================
	bool CMultiplayRules::PlayerCanHearChat( CBasePlayer *pListener, CBasePlayer *pSpeaker )
	{
		return ( PlayerRelationship( pListener, pSpeaker ) == GR_TEAMMATE );
	}

	int CMultiplayRules::PlayerRelationship( CBaseEntity *pPlayer, CBaseEntity *pTarget )
	{
		// half life deathmatch has only enemies
		return GR_NOTTEAMMATE;
	}

	bool CMultiplayRules::PlayFootstepSounds( CBasePlayer *pl )
	{
		if ( footsteps.GetInt() == 0 )
			return false;

		if ( pl->IsOnLadder() || pl->GetAbsVelocity().Length2D() > 220 )
			return true;  // only make step sounds in multiplayer if the player is moving fast enough

		return false;
	}

	bool CMultiplayRules::FAllowFlashlight( void ) 
	{ 
		return flashlight.GetInt() != 0; 
	}

	//=========================================================
	//=========================================================
	bool CMultiplayRules::FAllowNPCs( void )
	{
		return true; // E3 hack
		return ( allowNPCs.GetInt() != 0 );
	}

	//=========================================================
	//======== CMultiplayRules private functions ===========

	void CMultiplayRules::GoToIntermission( void )
	{
		if ( g_fGameOver )
			return;

		g_fGameOver = true;

		float flWaitTime = mp_chattime.GetInt();

		if ( tv_delaymapchange.GetBool() && HLTVDirector()->IsActive() )	
		{
			flWaitTime = max ( flWaitTime, HLTVDirector()->GetDelay() );
		}
				
		m_flIntermissionEndTime = gpGlobals->curtime + flWaitTime;

		for ( int i = 0; i < MAX_PLAYERS; i++ )
		{
			CBasePlayer *pPlayer = UTIL_PlayerByIndex( i );

			if ( !pPlayer )
				continue;

			pPlayer->ShowViewPortPanel( PANEL_SCOREBOARD );
		}
	}
	
	void CMultiplayRules::GetNextLevelName( char *pszNextMap, int bufsize )
	{
		char szFirstMapInList[32];
		Q_strncpy( szFirstMapInList, "hldm1" ,sizeof(szFirstMapInList));  // the absolute default level is hldm1

		// find the map to change to

		const char *mapcfile = mapcyclefile.GetString();
		Assert( mapcfile != NULL );
		Q_strncpy( pszNextMap, STRING(gpGlobals->mapname) ,bufsize);
		Q_strncpy( szFirstMapInList, STRING(gpGlobals->mapname) ,sizeof(szFirstMapInList));

		int length;
		char *pFileList;
		char *aFileList = pFileList = (char*)UTIL_LoadFileForMe( mapcfile, &length );
		if ( pFileList && length )
		{
			// the first map name in the file becomes the default
			sscanf( pFileList, " %31s", pszNextMap );
			if ( engine->IsMapValid( pszNextMap ) )
				Q_strncpy( szFirstMapInList, pszNextMap ,sizeof(szFirstMapInList));

			// keep pulling mapnames out of the list until the current mapname
			// if the current mapname isn't found,  load the first map in the list
			bool next_map_is_it = false;
			while ( 1 )
			{
				while ( *pFileList && isspace( *pFileList ) ) pFileList++; // skip over any whitespace
				if ( !(*pFileList) )
					break;

				char cBuf[32];
				int ret = sscanf( pFileList, " %31s", cBuf );
				// Check the map name is valid
				if ( ret != 1 || *cBuf < 13 )
					break;

				if ( next_map_is_it )
				{
					// check that it is a valid map file
					if ( engine->IsMapValid( cBuf ) )
					{
						Q_strncpy( pszNextMap, cBuf, bufsize);
						break;
					}
				}

				if ( FStrEq( cBuf, STRING(gpGlobals->mapname) ) )
				{  // we've found our map;  next map is the one to change to
					next_map_is_it = true;
				}

				pFileList += strlen( cBuf );
			}

			UTIL_FreeFile( (byte *)aFileList );
		}

		if ( !engine->IsMapValid(pszNextMap) )
			Q_strncpy( pszNextMap, szFirstMapInList, bufsize);
	}

	void CMultiplayRules::ChangeLevel( void )
	{
		char szNextMap[32];
		GetNextLevelName( szNextMap, sizeof(szNextMap) );

		g_fGameOver = true;

		Msg( "CHANGE LEVEL: %s\n", szNextMap );
		
		engine->ChangeLevel( szNextMap, NULL );
	}

#endif		

