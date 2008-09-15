//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_VGUISCREEN_H
#define C_VGUISCREEN_H

#ifdef _WIN32
#pragma once
#endif


#include <vgui_controls/EditablePanel.h>
#include "C_BaseEntity.h"
#include "PanelMetaClassMgr.h"

class KeyValues;


//-----------------------------------------------------------------------------
// Helper macro to make overlay factories one line of code. Use like this:
//	DECLARE_VGUI_SCREEN_FACTORY( CVguiScreenPanel, "image" );
//-----------------------------------------------------------------------------
struct VGuiScreenInitData_t
{
	C_BaseEntity *m_pEntity;

	VGuiScreenInitData_t() : m_pEntity(NULL) {}
	VGuiScreenInitData_t( C_BaseEntity *pEntity ) : m_pEntity(pEntity) {}
};

#define DECLARE_VGUI_SCREEN_FACTORY( _PanelClass, _nameString )	\
	DECLARE_PANEL_FACTORY( _PanelClass, VGuiScreenInitData_t, _nameString )


//-----------------------------------------------------------------------------
// Base class for vgui screen panels
//-----------------------------------------------------------------------------
class CVGuiScreenPanel : public vgui::EditablePanel
{
	DECLARE_CLASS_GAMEROOT( CVGuiScreenPanel, vgui::EditablePanel );

public:
	CVGuiScreenPanel( vgui::Panel *parent, const char *panelName );
	CVGuiScreenPanel( vgui::Panel *parent, const char *panelName, vgui::HScheme hScheme );
	virtual bool Init( KeyValues* pKeyValues, VGuiScreenInitData_t* pInitData );
	vgui::Panel *CreateControlByName(const char *controlName);

protected:
	C_BaseEntity *GetEntity() const { return m_hEntity.Get(); }

private:
	EHANDLE	m_hEntity;
};


//-----------------------------------------------------------------------------
// Returns an entity that is the nearby vgui screen; NULL if there isn't one
//-----------------------------------------------------------------------------
C_BaseEntity *FindNearbyVguiScreen( const Vector &viewPosition, const QAngle &viewAngle, int nTeam = -1 );


//-----------------------------------------------------------------------------
// Activates/Deactivates vgui screen
//-----------------------------------------------------------------------------
void ActivateVguiScreen( C_BaseEntity *pVguiScreen );
void DeactivateVguiScreen( C_BaseEntity *pVguiScreen );


//-----------------------------------------------------------------------------
// Updates vgui screen button state
//-----------------------------------------------------------------------------
void SetVGuiScreenButtonState( C_BaseEntity *pVguiScreen, int nButtonState );


// Called at shutdown.
void ClearKeyValuesCache();


#endif // C_VGUISCREEN_H
  
