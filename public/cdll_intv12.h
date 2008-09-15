//===== Copyright © 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: Interfaces between the client.dll and engine
//
//===========================================================================//

#ifndef CDLL_INTV12_H
#define CDLL_INTV12_H
#ifdef _WIN32
#pragma once
#endif

#include "basetypes.h"
#include "interface.h"
#include "mathlib.h"
#include "const.h"
#include "checksum_crc.h"

//-----------------------------------------------------------------------------
// forward declarations
//-----------------------------------------------------------------------------
class ClientClass;
struct model_t;
class CSentence;
struct vrect_t;
struct cmodel_t;
class IMaterial;
class CAudioSource;
class CMeasureSection;
class SurfInfo;
class ISpatialQuery;
struct cache_user_t;
class IMaterialSystem;
class VMatrix;
class bf_write;
class bf_read;
struct ScreenFade_t;
struct ScreenShake_t;
class CViewSetupV1;
class CTerrainModParams;
class CSpeculativeTerrainModVert;
class CEngineSprite;
class CGlobalVarsBase;
class CPhysCollide;
class CSaveRestoreData;
class INetChannelInfo;
struct datamap_t;
struct typedescription_t;
class CStandardRecvProxies;
struct client_textmessage_t;


namespace BaseClientDLLV12
{

#define CLIENT_DLL_INTERFACE_VERSION_12		"VClient012"

//-----------------------------------------------------------------------------
// Purpose: Interface exposed from the client .dll back to the engine
//-----------------------------------------------------------------------------
abstract_class IBaseClientDLL
{
public:
	// Called once when the client DLL is loaded
	virtual int				Init( CreateInterfaceFn appSystemFactory, 
									CreateInterfaceFn physicsFactory,
									CGlobalVarsBase *pGlobals ) = 0;

	// Called once when the client DLL is being unloaded
	virtual void			Shutdown( void ) = 0;
	
	// Called at the start of each level change
	virtual void			LevelInitPreEntity( char const* pMapName ) = 0;
	// Called at the start of a new level, after the entities have been received and created
	virtual void			LevelInitPostEntity( ) = 0;
	// Called at the end of a level
	virtual void			LevelShutdown( void ) = 0;

	// Request a pointer to the list of client datatable classes
	virtual ClientClass		*GetAllClasses( void ) = 0;

	// Called once per level to re-initialize any hud element drawing stuff
	virtual int				HudVidInit( void ) = 0;
	// Called by the engine when gathering user input
	virtual void			HudProcessInput( bool bActive ) = 0;
	// Called oncer per frame to allow the hud elements to think
	virtual void			HudUpdate( bool bActive ) = 0;
	// Reset the hud elements to their initial states
	virtual void			HudReset( void ) = 0;
	// Display a hud text message
	virtual void			HudText( const char * message ) = 0;

	// Mouse Input Interfaces
	// Activate the mouse (hides the cursor and locks it to the center of the screen)
	virtual void			IN_ActivateMouse( void ) = 0;
	// Deactivates the mouse (shows the cursor and unlocks it)
	virtual void			IN_DeactivateMouse( void ) = 0;
	// The user pressed or released a mouse button
	virtual void			IN_MouseEvent (int mstate, bool down) = 0;
	// This is only called during extra sound updates and just accumulates mouse x, y offets and recenters the mouse.
	//  This call is used to try to prevent the mouse from appearing out of the side of a windowed version of the engine if 
	//  rendering or other processing is taking too long
	virtual void			IN_Accumulate (void) = 0;
	// Reset all key and mouse states to their initial, unpressed state
	virtual void			IN_ClearStates (void) = 0;
	// If key is found by name, returns whether it's being held down in isdown, otherwise function returns false
	virtual bool			IN_IsKeyDown( const char *name, bool& isdown ) = 0;
	// Raw keyboard signal, if the client .dll returns 1, the engine processes the key as usual, otherwise,
	//  if the client .dll returns 0, the key is swallowed.
	virtual int				IN_KeyEvent( int eventcode, int keynum, const char *pszCurrentBinding ) = 0;

	// This function is called once per tick to create the player CUserCmd (used for prediction/physics simulation of the player)
	// Because the mouse can be sampled at greater than the tick interval, there is a separate input_sample_frametime, which
	//  specifies how much additional mouse / keyboard simulation to perform.
	virtual void			CreateMove ( 
								int sequence_number,			// sequence_number of this cmd
								float input_sample_frametime,	// Frametime for mouse input sampling
								bool active ) = 0;				// True if the player is active (not paused)
								 		
	// If the game is running faster than the tick_interval framerate, then we do extra mouse sampling to avoid jittery input
	//  This code path is much like the normal move creation code, except no move is created
	virtual void			ExtraMouseSample( float frametime, bool active ) = 0;

	// Encode the delta (changes) between the CUserCmd in slot from vs the one in slot to.  The game code will have
	//  matching logic to read the delta.
	virtual bool			WriteUsercmdDeltaToBuffer( bf_write *buf, int from, int to, bool isnewcommand ) = 0;
	// Demos need to be able to encode/decode CUserCmds to memory buffers, so these functions wrap that
	virtual void			EncodeUserCmdToBuffer( bf_write& buf, int slot ) = 0;
	virtual void			DecodeUserCmdFromBuffer( bf_read& buf, int slot ) = 0;

	// Set up and render one or more views (e.g., rear view window, etc.).  This called into RenderView below
	virtual void			View_Render( vrect_t *rect ) = 0;

	// Allow engine to expressly render a view (e.g., during timerefresh)
	virtual void			RenderView( const CViewSetupV1 &view, bool drawViewmodel ) = 0;

	// Apply screen fade directly from engine
	virtual void			View_Fade( ScreenFade_t *pSF ) = 0;

	// The engine has parsed a crosshair angle message, this function is called to dispatch the new crosshair angle
	virtual void			SetCrosshairAngle( const QAngle& angle ) = 0;

	// Sprite (.spr) model handling code
	// Load a .spr file by name
	virtual void			InitSprite( CEngineSprite *pSprite, const char *loadname ) = 0;
	// Shutdown a .spr file
	virtual void			ShutdownSprite( CEngineSprite *pSprite ) = 0;
	// Returns sizeof( CEngineSprite ) so the engine can allocate appropriate memory
	virtual int				GetSpriteSize( void ) const = 0;

	// Called when a player starts or stops talking.
	// entindex is -1 to represent the local client talking (before the data comes back from the server). 
	// entindex is -2 to represent the local client's voice being acked by the server.
	// entindex is GetPlayer() when the server acknowledges that the local client is talking.
	virtual void			VoiceStatus( int entindex, qboolean bTalking ) = 0;

	// Networked string table definitions have arrived, allow client .dll to 
	//  hook string changes with a callback function ( see INetworkStringTableClient.h )
	virtual void			InstallStringTableCallback( char const *tableName ) = 0;

	// Notification that we're moving into another stage during the frame.
	virtual void			FrameStageNotify( ClientFrameStage_t curStage ) = 0;

	// The engine has received the specified user message, this code is used to dispatch the message handler
	virtual bool			DispatchUserMessage( int msg_type, bf_read &msg_data ) = 0;

	// Save/restore system hooks
	virtual CSaveRestoreData  *SaveInit( int size ) = 0;
	virtual void			SaveWriteFields( CSaveRestoreData *, const char *, void *, datamap_t *, typedescription_t *, int ) = 0;
	virtual void			SaveReadFields( CSaveRestoreData *, const char *, void *, datamap_t *, typedescription_t *, int ) = 0;
	virtual void			PreSave( CSaveRestoreData * ) = 0;
	virtual void			Save( CSaveRestoreData * ) = 0;
	virtual void			WriteSaveHeaders( CSaveRestoreData * ) = 0;
	virtual void			ReadRestoreHeaders( CSaveRestoreData * ) = 0;
	virtual void			Restore( CSaveRestoreData *, bool ) = 0;
	virtual void			DispatchOnRestore() = 0;

	// Hand over the StandardRecvProxies in the client DLL's module.
	virtual CStandardRecvProxies* GetStandardRecvProxies() = 0;

	// save game screenshot writing
	virtual void			WriteSaveGameScreenshot( const char *pFilename ) = 0;

	// Given a list of "S(wavname) S(wavname2)" tokens, look up the localized text and emit
	//  the appropriate close caption if running with closecaption = 1
	virtual void			EmitSentenceCloseCaption( char const *tokenstream ) = 0;
	// Emits a regular close caption by token name
	virtual void			EmitCloseCaption( char const *captionname, float duration ) = 0;

	// Returns true if the client can start recording a demo now.  Added 7/25/2005, to existing VClient011 version.
	// If the client returns false, an error message of up to length bytes should be returned in errorMsg.
	virtual bool			CanRecordDemo( char *errorMsg, int length ) const = 0;
};

} // end namespace

#endif // CDLL_INTV12_H
