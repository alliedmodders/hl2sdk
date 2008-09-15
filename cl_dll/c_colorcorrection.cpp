//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Color correction entity with simple radial falloff
//
// $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "cbase.h"
#include "filesystem.h"
#include "cdll_client_int.h"

#include "materialsystem/materialsystemutil.h"
#include "materialsystem/icolorcorrection.h"

#include "utlvector.h"

#include "generichash.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


//------------------------------------------------------------------------------
// Purpose : Color correction entity with radial falloff
//------------------------------------------------------------------------------
class C_ColorCorrection : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_ColorCorrection, C_BaseEntity );

	DECLARE_CLIENTCLASS();

	void OnDataChanged(DataUpdateType_t updateType);
	bool ShouldDraw();

	void ClientThink();

private:
	Vector	m_vecOrigin;

	float	m_minFalloff;
	float	m_maxFalloff;
	float	m_maxWeight;
	char	m_netLookupFilename[MAX_PATH];

	bool	m_bEnabled;

	ColorCorrectionHandle_t m_CCHandle;
};

IMPLEMENT_CLIENTCLASS_DT(C_ColorCorrection, DT_ColorCorrection, CColorCorrection)
	RecvPropVector( RECVINFO(m_vecOrigin) ),
	RecvPropFloat(  RECVINFO(m_minFalloff) ),
	RecvPropFloat(  RECVINFO(m_maxFalloff) ),
	RecvPropFloat(  RECVINFO(m_maxWeight) ),
	RecvPropString( RECVINFO(m_netLookupFilename) ),
	RecvPropBool(   RECVINFO(m_bEnabled) ),

END_RECV_TABLE()

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_ColorCorrection::OnDataChanged(DataUpdateType_t updateType)
{
	BaseClass::OnDataChanged( updateType );

	// We're releasing the CS:S client before the engine with this interface, so we need to fail gracefully
	if ( !colorcorrection )
	{
		return;
	}

	if ( updateType == DATA_UPDATE_CREATED )
	{
		SetNextClientThink( CLIENT_THINK_ALWAYS );

		char filename[MAX_PATH];
		Q_strncpy( filename, m_netLookupFilename, MAX_PATH );

		m_CCHandle = colorcorrection->AddLookup( filename );

		colorcorrection->LockLookup( m_CCHandle );
		colorcorrection->LoadLookup( m_CCHandle, filename );
		colorcorrection->UnlockLookup( m_CCHandle );
	}
}

//------------------------------------------------------------------------------
// We don't draw...
//------------------------------------------------------------------------------
bool C_ColorCorrection::ShouldDraw()
{
	return false;
}

void C_ColorCorrection::ClientThink()
{
	// We're releasing the CS:S client before the engine with this interface, so we need to fail gracefully
	if ( !colorcorrection )
	{
		return;
	}

	if( !m_bEnabled )
	{
		colorcorrection->SetLookupWeight( m_CCHandle, 0.0f );
		return;
	}

	CBaseEntity *pPlayer = UTIL_PlayerByIndex(1);
	if( !pPlayer )
	{
		return;
	}

	Vector playerOrigin = pPlayer->GetAbsOrigin();

	float dist = (playerOrigin - m_vecOrigin).Length();
	float weight = (dist-m_minFalloff) / (m_maxFalloff-m_minFalloff);
	if( weight<0.0f ) weight = 0.0f;	
	if( weight>1.0f ) weight = 1.0f;	
	
	colorcorrection->SetLookupWeight( m_CCHandle, m_maxWeight * (1.0f - weight) );

	BaseClass::ClientThink();
}













