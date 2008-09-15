//====== Copyright © 1996-2005, Valve Corporation, All rights reserved. =======
//
// Purpose: 
//
//=============================================================================

#ifndef ITOOLFRAMEWORK_H
#define ITOOLFRAMEWORK_H
#ifdef _WIN32
#pragma once
#endif

#include "appframework/IAppSystem.h"
#include "materialsystem/imaterialproxy.h"
#include "toolframework/itoolentity.h"
#include "vector.h"
#include "Color.h"
#include "toolframework/itoolentity.h" // HTOOLHANDLE defn

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IToolSystem;
struct SpatializationInfo_t;
class KeyValues;
class CBoneList;


//-----------------------------------------------------------------------------
// Standard messages
//-----------------------------------------------------------------------------
struct BaseEntityRecordingState_t
{
	float m_flTime;
	const char *m_pModelName;
	int m_nOwner;
	int m_nEffects;
	bool m_bVisible;
	Vector m_vecRenderOrigin;
	QAngle m_vecRenderAngles;
};

struct SpriteRecordingState_t
{
	float m_flRenderScale;
	float m_flFrame;
	int m_nRenderMode;
	bool m_nRenderFX;
	Color m_Color;
};

struct BaseAnimatingRecordingState_t
{
	int m_nSkin;
	int m_nBody;
	int m_nSequence;
	CBoneList *m_pBoneList;
};

struct BaseFlexRecordingState_t
{
	int m_nFlexCount;
	float *m_pDestWeight;
	Vector m_vecViewTarget;
};

struct CameraRecordingState_t
{
	float m_flFOV;
	Vector m_vecEyePosition;
	QAngle m_vecEyeAngles;
};


//-----------------------------------------------------------------------------
// Purpose: This interface lives in the engine and handles loading up/unloading all 
//  available tools
//-----------------------------------------------------------------------------
class IToolFrameworkInternal : public IAppSystem
{
public: // Client Hooks
	virtual bool	ClientInit( CreateInterfaceFn clientFactory ) = 0; 
	virtual void	ClientShutdown() = 0;

	// Level init, shutdown
	virtual void	ClientLevelInitPreEntityAllTools() = 0;
	// entities are created / spawned / precached here
	virtual void	ClientLevelInitPostEntityAllTools() = 0;

	virtual void	ClientLevelShutdownPreEntityAllTools() = 0;
	// Entities are deleted / released here...
	virtual void	ClientLevelShutdownPostEntityAllTools() = 0;

	virtual void	ClientPreRenderAllTools() = 0;
	virtual void	ClientPostRenderAllTools() = 0;

	// Should we render with a thirdperson camera?
	virtual bool	IsThirdPersonCamera() = 0;

	// is the current tool recording?
	virtual bool	IsToolRecording() = 0;

public:  // Server Hooks
	// Level init, shutdown
	virtual bool	ServerInit( CreateInterfaceFn serverFactory ) = 0; 
	virtual void	ServerShutdown() = 0;

	virtual void	ServerLevelInitPreEntityAllTools() = 0;
	// entities are created / spawned / precached here
	virtual void	ServerLevelInitPostEntityAllTools() = 0;

	virtual void	ServerLevelShutdownPreEntityAllTools() = 0;
	// Entities are deleted / released here...
	virtual void	ServerLevelShutdownPostEntityAllTools() = 0;
	// end of level shutdown

	// Called each frame before entities think
	virtual void	ServerFrameUpdatePreEntityThinkAllTools() = 0;
	// called after entities think
	virtual void	ServerFrameUpdatePostEntityThinkAllTools() = 0;
	virtual void	ServerPreClientUpdateAllTools() = 0;

public:  // Other Hooks
	// If any tool returns false, the engine will not actually quit
	// FIXME:  Not implemented yet
	virtual bool	CanQuit() = 0;

	// Called at end of Host_Init
	virtual bool	PostInit() = 0;

	virtual void	Think( bool finalTick ) = 0;

	virtual void	PostMessage( KeyValues *msg ) = 0;

	virtual bool	GetSoundSpatialization( int iUserData, int guid, SpatializationInfo_t& info ) = 0;

	virtual void	HostRunFrameBegin() = 0;
	virtual void	HostRunFrameEnd() = 0;

	virtual void	RenderFrameBegin() = 0;
	virtual void	RenderFrameEnd() = 0;

	// Paintmode is an enum declared in enginevgui.h
	virtual void	VGui_PreRenderAllTools( int paintMode ) = 0;
	virtual void	VGui_PostRenderAllTools( int paintMode ) = 0;

	virtual void	VGui_PreSimulateAllTools() = 0;
	virtual void	VGui_PostSimulateAllTools() = 0;

	// Are we using tools?
	virtual bool	InToolMode() = 0;

	// Should the game be allowed to render the world?
	virtual bool	ShouldGameRenderView() = 0;

	virtual IMaterialProxy *LookupProxy( const char *proxyName ) = 0;

public:  // general framework hooks
	virtual int			GetToolCount() = 0;
	virtual char const	*GetToolName( int index ) = 0;
	virtual void		SwitchToTool( int index ) = 0;
	virtual IToolSystem *SwitchToTool( const char *pToolName ) = 0;
	virtual bool		IsTopmostTool( const IToolSystem *sys ) = 0;
	virtual const IToolSystem *GetToolSystem( int index ) const = 0;
	virtual IToolSystem *GetTopmostTool() = 0;
};

#ifndef _XBOX
// Expose to rest of engine as a singleton
extern IToolFrameworkInternal *toolframework;
#endif

// Exposed to launcher to automatically add AppSystemGroup hooks
#define VTOOLFRAMEWORK_INTERFACE_VERSION  "VTOOLFRAMEWORKVERSION002"

#endif // ITOOLFRAMEWORK_H
