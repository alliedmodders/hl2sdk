//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef IENGINESERVICE_H
#define IENGINESERVICE_H
#ifdef _WIN32
#pragma once
#endif

#include <appframework/IAppSystem.h>
#include <inputsystem/InputEnums.h>

class ISwitchLoopModeStatusNotify;
class IAddonListChangeNotify;
class ISceneView;
class ISource2WorldSession;
class IPrerequisite;

DECLARE_POINTER_HANDLE(PlatWindow_t);
DECLARE_POINTER_HANDLE(InputContextHandle_t);
DECLARE_POINTER_HANDLE(SwapChainHandle_t);
DECLARE_POINTER_HANDLE(ActiveLoop_t);
DECLARE_POINTER_HANDLE(HSceneViewRenderTarget);

struct RenderViewport_t
{
	int m_nVersion;
	int m_nTopLeftX;
	int m_nTopLeftY;
	int m_nWidth;
	int m_nHeight;
	float m_flMinZ;
	float m_flMaxZ;
};

enum ClientServerMode_t : int32
{
	CLIENTSERVERMODE_NONE = 0,
	CLIENTSERVERMODE_SERVER,
	CLIENTSERVERMODE_CLIENT,
	CLIENTSERVERMODE_LISTENSERVER,
};

struct RHBackColorBuffer_t
{
	ISceneView *m_pView;
	HSceneViewRenderTarget m_hBackColorBuffer;
};

enum EventMapRegistrationType_t : int32
{
	EVENT_MAP_REGISTER = 0,
	EVENT_MAP_UNREGISTER,
};

struct EngineLoopState_t
{
	PlatWindow_t m_hWnd;
	SwapChainHandle_t m_hSwapChain;
	InputContextHandle_t m_hInputContext;
	int m_nPlatWindowWidth;
	int m_nPlatWindowHeight;
	int m_nRenderWidth;
	int m_nRenderHeight;
};

struct EventClientOutput_t
{
	EngineLoopState_t m_LoopState;
	float m_flRenderTime;
	float m_flRealTime;
};

enum LoopModeType_t : int32
{
	LOOP_MODE_TYPE_INVALID = -1,
	LOOP_MODE_TYPE_SIMPLE = 0,
	LOOP_MODE_TYPE_CLIENT,
	LOOP_MODE_TYPE_SERVER,
	LOOP_MODE_TYPE_HOST,

	LOOP_MODE_TYPE_COUNT,
};

enum InputHandlerResult_t : int32
{
  	IHR_INPUT_IGNORED = 0,
  	IHR_INPUT_CONSUMED,
};

abstract_class ILoopType
{
public:
	virtual void AddEngineService(const char* pszName) = 0;
};

template <typename T>
struct CBaseCmdKeyValues : T
{
	KeyValues* m_pKeyValues;
};


class GameSessionConfiguration_t;

/*class GameSessionConfiguration_t : CBaseCmdKeyValues<CSVCMsg_GameSessionConfiguration>
{
};*/

abstract_class ILoopMode
{
public:
	virtual bool LoopInit( KeyValues *pKeyValues, ILoopModePrerequisiteRegistry *pRegistry ) = 0;
	virtual void LoopShutdown( void ) = 0;
	virtual void OnLoopActivate( const EngineLoopState_t &state, /*CEventDispatcher<CEventIDManager_Default> **/void* pEventDispatcher) = 0;
	virtual void OnLoopDeactivate( const EngineLoopState_t &state, /*CEventDispatcher<CEventIDManager_Default> **/void *pEventDispatcher ) = 0;
	virtual void RegisterEventMap( /*CEventDispatcher<CEventIDManager_Default> **/void* pEventDispatcher, EventMapRegistrationType_t nRegistrationType) = 0;
	virtual InputHandlerResult_t HandleInputEvent( const InputEvent_t &event, CSplitScreenSlot nSplitScreenPlayerSlot ) = 0;
	virtual ISceneView* AddViewsToSceneSystem( const EngineLoopState_t &state, double flRenderTime, double flRealTime,
		const RenderViewport_t &viewport, const RHBackColorBuffer_t &backColorBuffer ) = 0;
	virtual ISource2WorldSession* GetWorldSession( void ) = 0;
	virtual ClientServerMode_t GetClientServerMode( void ) = 0;
	virtual bool ReceivedServerInfo( const GameSessionConfiguration_t &config, ILoopModePrerequisiteRegistry *pRegistry ) = 0;
	virtual bool IsInHeadlessMode( void ) = 0;
	virtual void SetFadeColor( Vector4D vColorNormalized ) = 0;
	virtual bool IsBackgroundMap( void ) = 0;
	virtual bool IsLevelTransition( void ) = 0;
};

abstract_class ILoopModeFactory
{
public:
	virtual bool Init( ILoopType *pLoop ) =0;
	virtual void Shutdown( void ) =0;
	virtual ILoopMode *CreateLoopMode( void ) =0;
	virtual void DestroyLoopMode( ILoopMode *pLoopMode ) =0;
	virtual LoopModeType_t GetLoopModeType( void ) const =0;
};

abstract_class IEngineService : public IAppSystem
{
public:
	virtual void		*GetServiceDependencies( void ) = 0;
	virtual const char	*GetName( void ) const = 0;
	virtual bool		ShouldActivate( const char * ) = 0;
	virtual void		OnLoopActivate( const EngineLoopState_t &loopState, /*CEventDispatcher<CEventIDManager_Default> * */void * pEventDispatcher) = 0;
	virtual void		OnLoopDeactivate( const EngineLoopState_t &loopState, /*CEventDispatcher<CEventIDManager_Default> * */ void *pEventDispatcher) = 0;
	virtual bool		IsActive( void ) const = 0;
	virtual void		SetActive( bool ) = 0;
	virtual void		SetName( const char *pszName ) = 0;
	virtual void		RegisterEventMap( /*CEventDispatcher<CEventIDManager_Default> * */ void *pEventDispatcher, EventMapRegistrationType_t nRegistrationType ) = 0;
	virtual uint16		GetServiceIndex( void ) = 0;
	virtual void		SetServiceIndex( uint16 index ) = 0;
};

abstract_class IPrerequisiteRegistry
{
public:
	virtual void RegisterPrerequisite( IPrerequisite * ) = 0;
};

abstract_class ILoopModePrerequisiteRegistry : public IPrerequisiteRegistry
{
public:
	virtual void LookupLocalizationToken( const char * ) =0;
	virtual void UnregisterPrerequisite( IPrerequisite * ) =0;
};

abstract_class IEngineServiceMgr : public IAppSystem, public ILoopModePrerequisiteRegistry
{
public:
	virtual void		RegisterEngineService( const char *psServiceName, IEngineService *pService ) = 0;
	virtual void		UnregisterEngineService( const char *pszServiceName, IEngineService *pService ) = 0;
	virtual void		RegisterLoopMode( const char *pLoopModeName, ILoopModeFactory *pLoopModeFactory, void **ppGlobalPointer ) = 0;
	virtual void		UnregisterLoopMode( const char *pLoopModeName, ILoopModeFactory *pLoopModeFactory, void **ppGlobalPointer ) = 0;
	virtual void		SwitchToLoop( const char *pszLoopModeName, KeyValues *pkvLoopOptions, uint32 nId, const char *pszAddonName, bool ) = 0;
	virtual const char		*GetActiveLoopName( void ) const = 0;
	virtual IEngineService	*FindService( const char * ) = 0;
	virtual PlatWindow_t	GetEngineWindow( void ) const = 0;
	virtual SwapChainHandle_t	GetEngineSwapChain(void) const = 0;
	virtual void		*GetEngineInputContext( void ) const = 0;
	virtual void		*GetEngineDeviceInfo( void ) const = 0;
	virtual int			GetEngineDeviceWidth( void ) const = 0;
	virtual int			GetEngineDeviceHeight( void ) const = 0;
	virtual int			GetEngineSwapChainSize( void ) const = 0;
	virtual bool		IsLoopSwitchQueued( void ) const = 0;
	virtual bool		IsLoopSwitchRequested( void ) const = 0;
	virtual /*CEventDispatcher<CEventIDManager_Default> * */void * GetEventDispatcher(void) = 0;
	virtual void		*GetDebugVisualizerMgr( void ) = 0;
	virtual int			GetActiveLoopClientServerMode( void ) const = 0;
	virtual void		EnableMaxFramerate( bool ) = 0;
	virtual void		OverrideMaxFramerate( float ) = 0;
	virtual void		PrintStatus( void ) = 0;
	virtual ActiveLoop_t	GetActiveLoop( void ) = 0;
	virtual bool		IsLoadingLevel( void ) const = 0;
	virtual bool		IsInGameLoop( void ) const = 0;
	virtual void		OnFrameRenderingFinished( bool, const EventClientOutput_t & ) = 0;
	virtual void		ChangeVideoMode( RenderDeviceInfo_t & ) = 0;
	virtual void		GetVideoModeChange( void ) = 0;
	virtual int			GetAddonCount( void ) const = 0;
	virtual void		GetAddon( int ) const = 0;
	virtual bool		IsAddonMounted( const char * ) const = 0;
	virtual const char	*GetAddonsString( void ) const = 0;
	virtual void		InstallSwitchLoopModeStatusNotify( ISwitchLoopModeStatusNotify * ) = 0;
	virtual void		UninstallSwitchLoopModeStatusNotify( ISwitchLoopModeStatusNotify * ) = 0;
	virtual void		InstallAddonListChangeNotify( IAddonListChangeNotify * ) = 0;
	virtual void		UninstallAddonListChangeNotify( IAddonListChangeNotify * ) = 0;
	virtual void		ExitMainLoop( void ) = 0;
};

#endif // IENGINESERVICE_H