//======== (C) Copyright 1999, 2000 Valve, L.L.C. All rights reserved. ========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: An extra interface implemented by the material system 
// implementation of vgui::ISurface
//
// $Revision: $
// $NoKeywords: $
//=============================================================================

#ifndef IMATSYSTEMSURFACE_V4_H
#define IMATSYSTEMSURFACE_V4_H
#ifdef _WIN32
#pragma once
#endif

class VMatrix;
#include <vgui/VGUI.h>
#include "vgui/isurfacev29.h"

namespace MatSystemSurfaceV4
{
	//-----------------------------------------------------------------------------
	// Callbacks for mouse getting + setting
	//-----------------------------------------------------------------------------
	typedef void (*GetMouseCallback_t)(int &x, int &y);
	typedef void (*SetMouseCallback_t)(int x, int y);

	//-----------------------------------------------------------------------------
	// Callbacks for sound playing
	//-----------------------------------------------------------------------------
	typedef void (*PlaySoundFunc_t)(const char *pFileName);


	//-----------------------------------------------------------------------------
	//
	// An extra interface implemented by the material system implementation of vgui::ISurface
	//
	//-----------------------------------------------------------------------------
#define MAT_SYSTEM_SURFACE_INTERFACE_VERSION_4 "MatSystemSurface004"

	class IMatSystemSurface : public SurfaceV29::ISurface
	{
	public:
		// Hook needed to get input to work.
		// If the app drives the input (like the engine needs to do for VCR mode), 
		// it can set bLetAppDriveInput to true and call HandleWindowMessage for the Windows messages.
		virtual void AttachToWindow( void *hwnd, bool bLetAppDriveInput=false ) = 0;

		// If you specified true for bLetAppDriveInput, then call this for each window message that comes in.
		virtual void HandleWindowMessage( void *hwnd, unsigned int uMsg, unsigned int wParam, long lParam ) = 0;

		// Tells the surface to ignore windows messages
		virtual void EnableWindowsMessages( bool bEnable ) = 0;

		// Starts, ends 3D painting
		// NOTE: These methods should only be called from within the paint()
		// method of a panel.
		virtual void Begin3DPaint( int iLeft, int iTop, int iRight, int iBottom ) = 0;
		virtual void End3DPaint() = 0;

		// NOTE: This also should only be called from within the paint()
		// method of a panel. Use it to disable clipping for the rendering
		// of this panel.
		virtual void DisableClipping( bool bDisable ) = 0;

		// Prevents vgui from changing the cursor
		virtual bool IsCursorLocked() const = 0;

		// Sets the mouse get + set callbacks
		virtual void SetMouseCallbacks( GetMouseCallback_t getFunc, SetMouseCallback_t setFunc ) = 0;

		// Installs a function to play sounds
		virtual void InstallPlaySoundFunc( PlaySoundFunc_t soundFunc ) = 0;

		// Some drawing methods that cannot be accomplished under Win32
		virtual void DrawColoredCircle( int centerx, int centery, float radius, int r, int g, int b, int a ) = 0;
		virtual int DrawColoredText( vgui::HFont font, int x, int y, int r, int g, int b, int a, char *fmt, ... ) = 0;

		// Draws text with current font at position and wordwrapped to the rect using color values specified
		virtual void DrawColoredTextRect( vgui::HFont font, int x, int y, int w, int h, int r, int g, int b, int a, char *fmt, ... ) = 0;
		virtual void DrawTextHeight( vgui::HFont font, int w, int& h, char *fmt, ... ) = 0;

		// Returns the length of the text string in pixels
		virtual int	DrawTextLen( vgui::HFont font, char *fmt, ... ) = 0;

		// Draws a panel in 3D space. Assumes view + projection are already set up
		// Also assumes the (x,y) coordinates of the panels are defined in 640xN coords
		// (N isn't necessary 480 because the panel may not be 4x3)
		// The width + height specified are the size of the panel in world coordinates
		virtual void DrawPanelIn3DSpace( vgui::VPANEL pRootPanel, const VMatrix &panelCenterToWorld, int nPixelWidth, int nPixelHeight, float flWorldWidth, float flWorldHeight ) = 0; 
	};

} // namespace MatSystemSurfaceV4


//-----------------------------------------------------------------------------
// FIXME: This works around using scoped interfaces w/ EXPOSE_SINGLE_INTERFACE
//-----------------------------------------------------------------------------
class IMatSystemSurfaceV4 : public MatSystemSurfaceV4::IMatSystemSurface
{
public:
};


#endif // IMATSYSTEMSURFACE_V4_H

