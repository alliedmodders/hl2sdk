//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "ServerNetworkProperty.h"
#include "tier0/dbg.h"
#include "gameinterface.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CTimedEventMgr g_NetworkPropertyEventMgr;


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CServerNetworkProperty::CServerNetworkProperty()
{
	Init( NULL );
}


CServerNetworkProperty::~CServerNetworkProperty()
{
	/* Free our transmit proxy.
	if ( m_pTransmitProxy )
	{
		m_pTransmitProxy->Release();
	}*/

	// remove the attached edict if it exists
	DetachEdict();
}


//-----------------------------------------------------------------------------
// Initialization
//-----------------------------------------------------------------------------
void CServerNetworkProperty::Init( CBaseEntity *pEntity )
{
	m_pPev = NULL;
	m_pOuter = pEntity;
//	m_pTransmitProxy = NULL;
	m_bPendingStateChange = false;
	m_TimerEvent.Init( &g_NetworkPropertyEventMgr, this );
}


//-----------------------------------------------------------------------------
// Connects, disconnects edicts
//-----------------------------------------------------------------------------
void CServerNetworkProperty::AttachEdict( edict_t *pRequiredEdict )
{
	Assert ( !m_pPev );

	// see if there is an edict allocated for it, otherwise get one from the engine
	if ( !pRequiredEdict )
	{
		pRequiredEdict = engine->CreateEdict();
	}

	m_pPev = pRequiredEdict;
	m_pPev->SetEdict( GetBaseEntity(), true );
}

void CServerNetworkProperty::DetachEdict()
{
	if ( m_pPev )
	{
		m_pPev->SetEdict( NULL, false );
		engine->RemoveEdict( m_pPev );
		m_pPev = NULL;
	}
}


//-----------------------------------------------------------------------------
// Entity handles
//-----------------------------------------------------------------------------
IHandleEntity *CServerNetworkProperty::GetEntityHandle( )
{
	return m_pOuter;
}

void CServerNetworkProperty::Release()
{
	delete m_pOuter;
}


//-----------------------------------------------------------------------------
// EFlags access
// FIXME: Move flags related to networking solely into this class
//-----------------------------------------------------------------------------
int CServerNetworkProperty::GetEFlags() const
{
	return m_pOuter->GetEFlags();
}


void CServerNetworkProperty::AddEFlags( int iEFlags )
{
	m_pOuter->AddEFlags( iEFlags );
}

//-----------------------------------------------------------------------------
// PVS information
//-----------------------------------------------------------------------------
void CServerNetworkProperty::RecomputePVSInformation()
{
	if ( m_pOuter->GetEFlags() & EFL_DIRTY_PVS_INFORMATION )
	{
		m_pOuter->RemoveEFlags( EFL_DIRTY_PVS_INFORMATION );
		engine->BuildEntityClusterList( edict(), &m_PVSInfo );
	}
}


//-----------------------------------------------------------------------------
// Serverclass
//-----------------------------------------------------------------------------
ServerClass* CServerNetworkProperty::GetServerClass()
{
	return m_pOuter->GetServerClass();
}

const char* CServerNetworkProperty::GetClassName() const
{
	return STRING(m_pOuter->m_iClassname);
}


//-----------------------------------------------------------------------------
// Transmit proxies
/*-----------------------------------------------------------------------------
void CServerNetworkProperty::SetTransmitProxy( CBaseTransmitProxy *pProxy )
{
	if ( m_pTransmitProxy )
	{
		m_pTransmitProxy->Release();
	}

	m_pTransmitProxy = pProxy;
	
	if ( m_pTransmitProxy )
	{
		m_pTransmitProxy->AddRef();
	}
}*/

//-----------------------------------------------------------------------------
// PVS rules
//-----------------------------------------------------------------------------
bool CServerNetworkProperty::IsInPVS( const edict_t *pRecipient, const void *pvs, int pvssize )
{
	RecomputePVSInformation();

	// ignore if not touching a PV leaf
	// negative leaf count is a node number
	// If no pvs, add any entity

	Assert( pvs && ( edict() != pRecipient ) );

	unsigned char *pPVS = ( unsigned char * )pvs;
	
	if ( m_PVSInfo.m_nClusterCount < 0 )   // too many clusters, use headnode
	{
		return (engine->CheckHeadnodeVisible( m_PVSInfo.m_nHeadNode, pPVS, pvssize ) != 0);
	}
	
	for ( int i = m_PVSInfo.m_nClusterCount; --i >= 0; )
	{
		if (pPVS[m_PVSInfo.m_pClusters[i] >> 3] & (1 << (m_PVSInfo.m_pClusters[i] & 7) ))
			return true;
	}

	return false;		// not visible
}

//-----------------------------------------------------------------------------
// PVS: this function is called alot, so it avoids function calls
//-----------------------------------------------------------------------------
 
bool CServerNetworkProperty::IsInPVS( const CCheckTransmitInfo *pInfo )
{
	// PVS data must be up to date
	Assert( !( m_pOuter->GetEFlags() & EFL_DIRTY_PVS_INFORMATION ) );
	
	int i;

	for ( i=0; i< pInfo->m_AreasNetworked; i++ )
	{
		int clientArea = pInfo->m_Areas[i];

		// Early out if the areas are connected
		if ( engine->CheckAreasConnected( clientArea, m_PVSInfo.m_nAreaNum) )
		{	
			break;
		}
		// doors can legally straddle two areas, so
		// we may need to check another one

		if ( m_PVSInfo.m_nAreaNum2 && engine->CheckAreasConnected( clientArea, m_PVSInfo.m_nAreaNum2 ) )
		{
			break;
		}
	}

	if ( i == pInfo->m_AreasNetworked )
	{
		// areas not connected
		return false;
	}

	// ignore if not touching a PV leaf
	// negative leaf count is a node number
	// If no pvs, add any entity

	Assert( edict() != pInfo->m_pClientEnt );

	unsigned char *pPVS = ( unsigned char * )pInfo->m_PVS;
	
	if ( m_PVSInfo.m_nClusterCount < 0 )   // too many clusters, use headnode
	{
		return (engine->CheckHeadnodeVisible( m_PVSInfo.m_nHeadNode, pPVS, pInfo->m_nPVSSize ) != 0);
	}
	
	for ( i = m_PVSInfo.m_nClusterCount; --i >= 0; )
	{
		if (pPVS[m_PVSInfo.m_pClusters[i] >> 3] & (1 << (m_PVSInfo.m_pClusters[i] & 7) ))
			return true;
	}

	return false;		// not visible

}


void CServerNetworkProperty::SetUpdateInterval( float val )
{
	if ( val == 0 )
		m_TimerEvent.StopUpdates();
	else
		m_TimerEvent.SetUpdateInterval( val );
}


void CServerNetworkProperty::FireEvent()
{
	// Our timer went off. If our state has changed in the background, then 
	// trigger a state change in the edict.
	if ( m_bPendingStateChange )
	{
		m_pPev->StateChanged();
		m_bPendingStateChange = false;
	}
}



