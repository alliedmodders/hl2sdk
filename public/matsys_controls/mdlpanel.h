//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef MDLPANEL_H
#define MDLPANEL_H

#ifdef _WIN32
#pragma once
#endif


#include "vgui_controls/Panel.h"
#include "datacache/imdlcache.h"
#include "materialsystem/materialsystemutil.h"
#include "matsys_controls/potterywheelpanel.h"
#include "tier3/mdlutils.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
namespace vgui
{
	class IScheme;
}


//-----------------------------------------------------------------------------
// MDL Viewer Panel
//-----------------------------------------------------------------------------
class CMDLPanel : public CPotteryWheelPanel
{
	DECLARE_CLASS_SIMPLE( CMDLPanel, CPotteryWheelPanel );

public:
	// constructor, destructor
	CMDLPanel( vgui::Panel *pParent, const char *pName );
	virtual ~CMDLPanel();

	// Overriden methods of vgui::Panel
	virtual void ApplySchemeSettings( vgui::IScheme *pScheme );

	virtual void OnTick();

	// Sets the current mdl
	virtual void SetMDL( MDLHandle_t handle );
	virtual void SetMDL( const char *pMDLName );

	// Sets the camera to look at the model
	void LookAtMDL( );

	// Sets the current sequence
	void SetSequence( int nSequence );

	void SetCollsionModel( bool bVisible );
	void SetGroundGrid( bool bVisible );
	void SetWireFrame( bool bVisible );
	void SetLockView( bool bLocked );
	void SetSkin( int nSkin );
	void SetLookAtCamera( bool bLookAtCamera );

	// Bounds.
	bool GetBoundingBox( Vector &vecBoundsMin, Vector &vecBoundsMax );
	bool GetBoundingSphere( Vector &vecCenter, float &flRadius );

	void SetModelAnglesAndPosition( const QAngle &angRot, const Vector &vecPos );

	// Attached models.
	void	SetMergeMDL( MDLHandle_t handle );
	void	SetMergeMDL( const char *pMDLName );
	int		GetMergeMDLIndex( MDLHandle_t handle );
	void	ClearMergeMDLs( void );

protected:

	struct MDLData_t
	{
		CMDL		m_MDL;
		matrix3x4_t	m_MDLToWorld;
	};

	MDLData_t				m_RootMDL;
	CUtlVector<MDLData_t>	m_aMergeMDLs;

private:
	// paint it!
	void OnPaint3D();
	void OnMouseDoublePressed( vgui::MouseCode code );

	void DrawCollisionModel();
	void UpdateStudioRenderConfig( void );

	CTextureReference m_DefaultEnvCubemap;
	CTextureReference m_DefaultHDREnvCubemap;

	bool	m_bDrawCollisionModel : 1;
	bool	m_bGroundGrid : 1;
	bool	m_bLockView : 1;
	bool	m_bWireFrame : 1;
	bool	m_bLookAtCamera : 1;
};


#endif // MDLPANEL_H
