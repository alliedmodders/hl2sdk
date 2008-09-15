//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================
#include "cbase.h"
#include "const.h"
#include "toolframework/itoolentity.h"
#include "entitylist.h"
#include "toolframework/itoolsystem.h"
#include "KeyValues.h"
#include "icliententity.h"
#include "iserverentity.h"
#include "sceneentity.h"


// Interface from engine to tools for manipulating entities
class CServerTools : public IServerTools
{
public:
	virtual IServerEntity *GetIServerEntity( IClientEntity *pClientEntity )
	{
		if ( pClientEntity == NULL )
			return NULL;

		CBaseHandle ehandle = pClientEntity->GetRefEHandle();
		if ( ehandle.GetEntryIndex() >= MAX_EDICTS )
			return NULL; // the first MAX_EDICTS entities are networked, the rest are client or server only

#if 0
		// this fails, since the server entities have extra bits in their serial numbers,
		// since 20 bits are reserved for serial numbers, except for networked entities, which are restricted to 10

		// Brian believes that everything should just restrict itself to 10 to make things simpler,
		// so if/when he changes NUM_SERIAL_NUM_BITS to 10, we can switch back to this simpler code

		IServerNetworkable *pNet = gEntList.GetServerNetworkable( ehandle );
		if ( pNet == NULL )
			return NULL;

		CBaseEntity *pServerEnt = pNet->GetBaseEntity();
		return pServerEnt;
#else
		IHandleEntity *pEnt = gEntList.LookupEntityByNetworkIndex( ehandle.GetEntryIndex() );
		if ( pEnt == NULL )
			return NULL;

		CBaseHandle h = gEntList.GetNetworkableHandle( ehandle.GetEntryIndex() );
		const int mask = ( 1 << NUM_NETWORKED_EHANDLE_SERIAL_NUMBER_BITS ) - 1;
		if ( !h.IsValid() || ( ( h.GetSerialNumber() & mask ) != ( ehandle.GetSerialNumber() & mask ) ) )
			return NULL;

		IServerUnknown *pUnk = static_cast< IServerUnknown* >( pEnt );
		return pUnk->GetBaseEntity();
#endif
	}

	bool GetPlayerPosition( Vector &org, QAngle &ang, IClientEntity *pClientPlayer = NULL )
	{
		IServerEntity *pServerPlayer = GetIServerEntity( pClientPlayer );
		CBasePlayer *pPlayer = pServerPlayer ? ( CBasePlayer* )pServerPlayer : UTIL_GetLocalPlayer();
		if ( pPlayer == NULL )
			return false;

		org = pPlayer->EyePosition();
		ang = pPlayer->EyeAngles();
		return true;
	}

	bool SnapPlayerToPosition( const Vector &org, const QAngle &ang, IClientEntity *pClientPlayer = NULL )
	{
		IServerEntity *pServerPlayer = GetIServerEntity( pClientPlayer );
		CBasePlayer *pPlayer = pServerPlayer ? ( CBasePlayer* )pServerPlayer : UTIL_GetLocalPlayer();
		if ( pPlayer == NULL )
			return false;

		pPlayer->SetAbsOrigin( org - pPlayer->GetViewOffset() );
		pPlayer->SnapEyeAngles( ang );

		// Disengage from hierarchy
		pPlayer->SetParent( NULL );

		return true;
	}

	int GetPlayerFOV( IClientEntity *pClientPlayer = NULL )
	{
		IServerEntity *pServerPlayer = GetIServerEntity( pClientPlayer );
		CBasePlayer *pPlayer = pServerPlayer ? ( CBasePlayer* )pServerPlayer : UTIL_GetLocalPlayer();
		if ( pPlayer == NULL )
			return 0;

		return pPlayer->GetFOV();
	}

	bool SetPlayerFOV( int fov, IClientEntity *pClientPlayer = NULL )
	{
		IServerEntity *pServerPlayer = GetIServerEntity( pClientPlayer );
		CBasePlayer *pPlayer = pServerPlayer ? ( CBasePlayer* )pServerPlayer : UTIL_GetLocalPlayer();
		if ( pPlayer == NULL )
			return false;

		pPlayer->SetDefaultFOV( fov );
		CBaseEntity *pFOVOwner = pPlayer->GetFOVOwner();
		return pPlayer->SetFOV( pFOVOwner ? pFOVOwner : pPlayer, fov );
	}
};


static CServerTools g_ServerTools;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CServerTools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION, g_ServerTools );


// Interface from engine to tools for manipulating entities
class CServerChoreoTools : public IServerChoreoTools
{
public:
	// Iterates through ALL entities (separate list for client vs. server)
	virtual EntitySearchResult	NextChoreoEntity( EntitySearchResult currentEnt )
	{
		CBaseEntity *ent = reinterpret_cast< CBaseEntity* >( currentEnt );
		ent = gEntList.FindEntityByClassname( ent, "logic_choreographed_scene" );
		return reinterpret_cast< EntitySearchResult >( ent );
	}

	virtual const char			*GetSceneFile( EntitySearchResult sr )
	{
		CBaseEntity *ent = reinterpret_cast< CBaseEntity* >( sr );
		if ( !sr )
			return "";

		if ( Q_stricmp( ent->GetClassname(), "logic_choreographed_scene" ) )
			return "";

		return GetSceneFilename( ent );
	}

	// For interactive editing
	virtual int	GetEntIndex( EntitySearchResult sr )
	{
		CBaseEntity *ent = reinterpret_cast< CBaseEntity* >( sr );
		if ( !ent )
			return -1;

		return ent->entindex();
	}

	virtual void ReloadSceneFromDisk( int entindex )
	{
		CBaseEntity *ent = CBaseEntity::Instance( entindex );
		if ( !ent )
			return;

		::ReloadSceneFromDisk( ent );
	}
};


static CServerChoreoTools g_ServerChoreoTools;
EXPOSE_SINGLE_INTERFACE_GLOBALVAR( CServerChoreoTools, IServerChoreoTools, VSERVERCHOREOTOOLS_INTERFACE_VERSION, g_ServerChoreoTools );