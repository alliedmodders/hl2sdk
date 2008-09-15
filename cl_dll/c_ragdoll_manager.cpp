//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"
#include "ragdoll_shared.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

class C_RagdollManager : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_RagdollManager, C_BaseEntity );
	DECLARE_CLIENTCLASS();

	C_RagdollManager();

// C_BaseEntity overrides.
public:

	virtual void	OnDataChanged( DataUpdateType_t updateType );

public:

	int		m_iMaxRagdollCount;
};

IMPLEMENT_CLIENTCLASS_DT_NOBASE( C_RagdollManager, DT_RagdollManager, CRagdollManager )
	RecvPropInt( RECVINFO( m_iMaxRagdollCount ) ),
END_RECV_TABLE()

//-----------------------------------------------------------------------------
// Constructor 
//-----------------------------------------------------------------------------
C_RagdollManager::C_RagdollManager()
{
	m_iMaxRagdollCount = -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : updateType - 
//-----------------------------------------------------------------------------
void C_RagdollManager::OnDataChanged( DataUpdateType_t updateType )
{
	BaseClass::OnDataChanged( updateType );

	s_RagdollLRU.SetMaxRagdollCount( m_iMaxRagdollCount );
}
