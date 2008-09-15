//========= Copyright © 1996-2003, Valve LLC, All rights reserved. ============
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================

#ifndef IVGUI_V06_H
#define IVGUI_V06_H

#ifdef _WIN32
#pragma once
#endif

#include "interface.h"
#include <vgui/VGUI.h>

#include "appframework/IAppSystem.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class KeyValues;

namespace vgui
{
// safe handle to a panel - can be converted to and from a VPANEL
typedef unsigned long HPanel;
}

namespace VGuiV06
{
typedef int HContext;

enum
{
	DEFAULT_VGUI_CONTEXT = ((vgui::HContext)~0)
};

//-----------------------------------------------------------------------------
// Purpose: This is the interface at the time of ship, 10/15/04
//-----------------------------------------------------------------------------
class IVGui : public IBaseInterface, public IAppSystemV0
{
public:
	// must be called first - provides interfaces for vgui to access
	virtual bool Init( CreateInterfaceFn *factoryList, int numFactories ) = 0;

	// call to free memory on shutdown
	virtual void Shutdown() = 0;

	// activates vgui message pump
	virtual void Start() = 0;

	// signals vgui to Stop running
	virtual void Stop() = 0;

	// returns true if vgui is current active
	virtual bool IsRunning() = 0;

	// runs a single frame of vgui
	virtual void RunFrame() = 0;

	// broadcasts "ShutdownRequest" "id" message to all top-level panels in the app
	virtual void ShutdownMessage(unsigned int shutdownID) = 0;

	// panel allocation
	virtual vgui::VPANEL AllocPanel() = 0;
	virtual void FreePanel(vgui::VPANEL panel) = 0;
	
	// debugging prints
	virtual void DPrintf(const char *format, ...) = 0;
	virtual void DPrintf2(const char *format, ...) = 0;
	virtual void SpewAllActivePanelNames() = 0;
	
	// safe-pointer handle methods
	virtual vgui::HPanel PanelToHandle(vgui::VPANEL panel) = 0;
	virtual vgui::VPANEL HandleToPanel(vgui::HPanel index) = 0;
	virtual void MarkPanelForDeletion(vgui::VPANEL panel) = 0;

	// makes panel receive a 'Tick' message every frame (~50ms, depending on sleep times/framerate)
	// panel is automatically removed from tick signal list when it's deleted
	virtual void AddTickSignal(vgui::VPANEL panel, int intervalMilliseconds = 0 ) = 0;
	virtual void RemoveTickSignal(vgui::VPANEL panel) = 0;

	// message sending
	virtual void PostMessage(vgui::VPANEL target, KeyValues *params, vgui::VPANEL from, float delaySeconds = 0.0f) = 0;

	// Creates/ destroys vgui contexts, which contains information
	// about which controls have mouse + key focus, for example.
	virtual HContext CreateContext() = 0;
	virtual void DestroyContext( HContext context ) = 0; 

	// Associates a particular panel with a vgui context
	// Associating NULL is valid; it disconnects the panel from the context
	virtual void AssociatePanelWithContext( HContext context, vgui::VPANEL pRoot ) = 0;

	// Activates a particular context, use DEFAULT_VGUI_CONTEXT
	// to get the one normally used by VGUI
	virtual void ActivateContext( HContext context ) = 0;

	// whether to sleep each frame or not, true = sleep
	virtual void SetSleep( bool state) = 0; 

	// data accessor for above
	virtual bool GetShouldVGuiControlSleep() = 0;
};

} // end namespace

#define VGUI_IVGUI_INTERFACE_VERSION_6 "VGUI_ivgui006"

//-----------------------------------------------------------------------------
// FIXME: This works around using scoped interfaces w/ EXPOSE_SINGLE_INTERFACE
//-----------------------------------------------------------------------------
class IVGuiV06 : public VGuiV06::IVGui
{
public:
};


#endif // IVGUI_H
