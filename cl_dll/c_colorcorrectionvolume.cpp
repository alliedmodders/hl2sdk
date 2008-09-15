//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Color correction entity.
//
 // $NoKeywords: $
//=============================================================================//
#include "cbase.h"

#include "cbase.h"
#include "filesystem.h"
//#include "triggers.h"
#include "cdll_client_int.h"

#include "materialsystem/materialsystemutil.h"
#include "materialsystem/icolorcorrection.h"

#include "utlvector.h"

#include "generichash.h"

//#include "engine/conprint.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//------------------------------------------------------------------------------
// FIXME: This really should inherit from something	more lightweight
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------
// Purpose : Shadow control entity
//------------------------------------------------------------------------------
class C_ColorCorrectionVolume : public C_BaseEntity
{
public:
	DECLARE_CLASS( C_ColorCorrectionVolume, C_BaseEntity );

	DECLARE_CLIENTCLASS();
	DECLARE_PREDICTABLE();

	void OnDataChanged(DataUpdateType_t updateType);
	bool ShouldDraw();

	void ClientThink();

private:

	float	m_Weight;
	char	m_lookupFilename[MAX_PATH];

	ColorCorrectionHandle_t m_CCHandle;
};

IMPLEMENT_CLIENTCLASS_DT(C_ColorCorrectionVolume, DT_ColorCorrectionVolume, CColorCorrectionVolume)
	RecvPropFloat( RECVINFO(m_Weight) ),
	RecvPropString( RECVINFO(m_lookupFilename) ),
END_RECV_TABLE()

BEGIN_PREDICTION_DATA( C_ColorCorrectionVolume )
	DEFINE_PRED_FIELD( m_Weight, FIELD_FLOAT, FTYPEDESC_INSENDTABLE ),
END_PREDICTION_DATA()

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void C_ColorCorrectionVolume::OnDataChanged(DataUpdateType_t updateType)
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
		Q_strncpy( filename, m_lookupFilename, MAX_PATH );

		m_CCHandle = colorcorrection->AddLookup( filename );

		colorcorrection->LockLookup( m_CCHandle );
		colorcorrection->LoadLookup( m_CCHandle, filename );
		colorcorrection->UnlockLookup( m_CCHandle );
	}
}

//------------------------------------------------------------------------------
// We don't draw...
//------------------------------------------------------------------------------
bool C_ColorCorrectionVolume::ShouldDraw()
{
	return false;
}

void C_ColorCorrectionVolume::ClientThink()
{
	// We're releasing the CS:S client before the engine with this interface, so we need to fail gracefully
	if ( !colorcorrection )
	{
		return;
	}

	Vector entityPosition = GetAbsOrigin();

	colorcorrection->SetLookupWeight( m_CCHandle, m_Weight );
}













