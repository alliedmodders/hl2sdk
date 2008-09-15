//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =====//
//
// Purpose: 
//
//===========================================================================//

#ifndef TOOLFRAMEWORK_CLIENT_H
#define TOOLFRAMEWORK_CLIENT_H

#ifdef _WIN32
#pragma once
#endif

#include "toolframework/itoolentity.h"


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class KeyValues;


//-----------------------------------------------------------------------------
// Posts a message to all tools
//-----------------------------------------------------------------------------
void ToolFramework_PostToolMessage( HTOOLHANDLE hEntity, KeyValues *msg );


//-----------------------------------------------------------------------------
// Should we render with a 3rd person camera?
//-----------------------------------------------------------------------------
bool ToolFramework_IsThirdPersonCamera( );


//-----------------------------------------------------------------------------
// View manipulation
//-----------------------------------------------------------------------------
void ToolFramework_AdjustEngineViewport( int& x, int& y, int& width, int& height );
bool ToolFramework_SetupEngineView( Vector &origin, QAngle &angles, float &fov );
bool ToolFramework_SetupEngineMicrophone( Vector &origin, QAngle &angles );


//-----------------------------------------------------------------------------
// Recorded temp entity structures
//-----------------------------------------------------------------------------
enum TERecordingType_t
{
	TE_DYNAMIC_LIGHT = 0,
	TE_WORLD_DECAL,
	TE_DISPATCH_EFFECT,
	TE_MUZZLE_FLASH,
	TE_ARMOR_RICOCHET,
	TE_METAL_SPARKS,
	TE_SMOKE,
	TE_SPARKS,
	TE_BLOOD_SPRITE,
	TE_BREAK_MODEL,
	TE_GLOW_SPRITE,
	TE_PHYSICS_PROP,
	TE_SPRITE_SINGLE,
	TE_SPRITE_SPRAY,
	TE_CONCUSSIVE_EXPLOSION,
	TE_BLOOD_STREAM,
	TE_SHATTER_SURFACE,
	TE_DECAL,
	TE_PROJECT_DECAL,
	TE_EXPLOSION,

	TE_RECORDING_TYPE_COUNT,
};


#endif // TOOLFRAMEWORK_CLIENT_H