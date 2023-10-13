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
#include <iloopmode.h>

class ISwitchLoopModeStatusNotify;
class IAddonListChangeNotify;

struct EventClientOutput_t
{
	EngineLoopState_t m_LoopState;
	float m_flRenderTime;
	float m_flRealTime;
};

abstract_class IEngineService : public IAppSystem
{
public:
	virtual void		*GetServiceDependencies( void ) = 0;
	virtual const char	*GetName( void ) const = 0;
	virtual bool		ShouldActivate( const char * ) = 0;
	virtual void		OnLoopActivate( const EngineLoopState_t &loopState, CEventDispatcher<CEventIDManager_Default> *pEventDispatcher) = 0;
	virtual void		OnLoopDeactivate( const EngineLoopState_t &loopState, CEventDispatcher<CEventIDManager_Default> *pEventDispatcher) = 0;
	virtual bool		IsActive( void ) const = 0;
	virtual void		SetActive( bool ) = 0;
	virtual void		SetName( const char *pszName ) = 0;
	virtual void		RegisterEventMap( CEventDispatcher<CEventIDManager_Default> *pEventDispatcher, EventMapRegistrationType_t nRegistrationType ) = 0;
	virtual uint16		GetServiceIndex( void ) = 0;
	virtual void		SetServiceIndex( uint16 index ) = 0;
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
	virtual SwapChainHandle_t	GetEngineSwapChain( void ) const = 0;
	virtual InputContextHandle_t GetEngineInputContext( void ) const = 0;
	virtual void		*GetEngineDeviceInfo( void ) const = 0;
	virtual int			GetEngineDeviceWidth( void ) const = 0;
	virtual int			GetEngineDeviceHeight( void ) const = 0;
	virtual int			GetEngineSwapChainSize( void ) const = 0;
	virtual bool		IsLoopSwitchQueued( void ) const = 0;
	virtual bool		IsLoopSwitchRequested( void ) const = 0;
	virtual CEventDispatcher<CEventIDManager_Default> *GetEventDispatcher( void ) = 0;
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