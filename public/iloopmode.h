//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef ILOOPMODE_H
#define ILOOPMODE_H
#ifdef _WIN32
#pragma once
#endif

#include <appframework/IAppSystem.h>
#include <KeyValues.h>

class ISwitchLoopModeStatusNotify;
class ISource2WorldSession;
class ISceneView;
class IPrerequisite;

DECLARE_POINTER_HANDLE(PlatWindow_t);
DECLARE_POINTER_HANDLE(InputContextHandle_t);
DECLARE_POINTER_HANDLE(SwapChainHandle_t);
DECLARE_POINTER_HANDLE(HSceneViewRenderTarget);
DECLARE_POINTER_HANDLE(ActiveLoop_t);

enum ClientServerMode_t : int32
{
	CLIENTSERVERMODE_NONE = 0,
	CLIENTSERVERMODE_SERVER,
	CLIENTSERVERMODE_CLIENT,
	CLIENTSERVERMODE_LISTENSERVER,
};

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

enum EventMapRegistrationType_t : int32
{
	EVENT_MAP_REGISTER = 0,
	EVENT_MAP_UNREGISTER,
};

struct RHBackColorBuffer_t
{
	ISceneView *m_pView;
	HSceneViewRenderTarget m_hBackColorBuffer;
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

#endif // ILOOPMODE_H
