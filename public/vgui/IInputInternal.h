//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef IINPUTINTERNAL_H
#define IINPUTINTERNAL_H
#ifdef _WIN32
#pragma once
#endif

#include <vgui/IInput.h>

namespace vgui
{

typedef int HInputContext;

#define DEFAULT_INPUT_CONTEXT ((vgui::HInputContext)~0)

class IInputInternal : public IInput
{
public:
	// processes input for a frame
	virtual void RunFrame() = 0;

	virtual void UpdateMouseFocus(int x, int y) = 0;

	// called when a panel becomes invalid
	virtual void PanelDeleted(VPANEL panel) = 0;

	// inputs into vgui input handling 
	virtual void InternalCursorMoved(int x,int y) = 0; //expects input in surface space
	virtual void InternalMousePressed(MouseCode code) = 0;
	virtual void InternalMouseDoublePressed(MouseCode code) = 0;
	virtual void InternalMouseReleased(MouseCode code) = 0;
	virtual void InternalMouseWheeled(int delta) = 0;
	virtual void InternalKeyCodePressed(KeyCode code) = 0;
	virtual void InternalKeyCodeTyped(KeyCode code) = 0;
	virtual void InternalKeyTyped(wchar_t unichar) = 0;
	virtual void InternalKeyCodeReleased(KeyCode code) = 0;

	// Creates/ destroys "input" contexts, which contains information
	// about which controls have mouse + key focus, for example.
	virtual HInputContext CreateInputContext() = 0;
	virtual void DestroyInputContext( HInputContext context ) = 0; 

	// Associates a particular panel with an input context
	// Associating NULL is valid; it disconnects the panel from the context
	virtual void AssociatePanelWithInputContext( HInputContext context, VPANEL pRoot ) = 0;

	// Activates a particular input context, use DEFAULT_INPUT_CONTEXT
	// to get the one normally used by VGUI
	virtual void ActivateInputContext( HInputContext context ) = 0;
};

} // namespace vgui

#define VGUI_INPUTINTERNAL_INTERFACE_VERSION "VGUI_InputInternal001"

#endif // IINPUTINTERNAL_H
