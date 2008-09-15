//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "baseentity.h"
#include "sendproxy.h"
#include "ragdoll_shared.h"
#include "ai_basenpc.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class CRagdollManager : public CBaseEntity
{
public:
	DECLARE_CLASS( CRagdollManager, CBaseEntity );
	DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CRagdollManager();

	virtual void	Activate();
	virtual int		UpdateTransmitState();

	void InputMaxRagdollCount(inputdata_t &data);

public:

	CNetworkVar( int,  m_iMaxRagdollCount );

	bool	m_bSaveImportant;
};


IMPLEMENT_SERVERCLASS_ST_NOBASE( CRagdollManager, DT_RagdollManager )
	SendPropInt( SENDINFO( m_iMaxRagdollCount ), 6 ),
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( game_ragdoll_manager, CRagdollManager );

BEGIN_DATADESC( CRagdollManager )

	DEFINE_KEYFIELD( m_iMaxRagdollCount, FIELD_INTEGER,	"MaxRagdollCount" ),
	DEFINE_KEYFIELD( m_bSaveImportant, FIELD_BOOLEAN, "SaveImportant" ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMaxRagdollCount",  InputMaxRagdollCount ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Constructor 
//-----------------------------------------------------------------------------
CRagdollManager::CRagdollManager( void )
{
	m_iMaxRagdollCount = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInfo - 
// Output : int
//-----------------------------------------------------------------------------
int CRagdollManager::UpdateTransmitState()
{
	return SetTransmitState( FL_EDICT_ALWAYS );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRagdollManager::Activate()
{
	BaseClass::Activate();

	s_RagdollLRU.SetMaxRagdollCount( m_iMaxRagdollCount );
}

void CRagdollManager::InputMaxRagdollCount(inputdata_t &inputdata)
{
	m_iMaxRagdollCount = inputdata.value.Int();
	s_RagdollLRU.SetMaxRagdollCount( m_iMaxRagdollCount );
}

bool RagdollManager_SaveImportant( CAI_BaseNPC *pNPC )
{
#ifdef HL2_DLL
	CRagdollManager *pEnt =	(CRagdollManager *)gEntList.FindEntityByClassname( NULL, "game_ragdoll_manager" );

	if ( pEnt == NULL )
		return false;

	if ( pEnt->m_bSaveImportant )
	{
		if ( pNPC->Classify() == CLASS_PLAYER_ALLY || pNPC->Classify() == CLASS_PLAYER_ALLY_VITAL )
		{
			return true;
		}
	}
#endif

	return false;
}
