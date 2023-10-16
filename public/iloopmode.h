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
#include <inputsystem/InputEnums.h>
#include <KeyValues.h>
#include <engine/eventdispatcher.h>

class ISource2WorldSession;
class ISceneView;
class IPrerequisite;

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

enum HostStateLoopModeType_t
{
	HOST_STATE_LOOP_MODE_IDLE = 0,
	HOST_STATE_LOOP_MODE_GAME,
	HOST_STATE_LOOP_MODE_SOURCETV_RELAY,

	HOST_STATE_LOOP_MODE_COUNT
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

abstract_class IPrerequisiteRegistry
{
public:
	virtual void RegisterPrerequisite( IPrerequisite * ) = 0;
};

abstract_class ILoopModePrerequisiteRegistry : public IPrerequisiteRegistry
{
public:
	virtual void LookupLocalizationToken( const char * ) = 0;
	virtual void UnregisterPrerequisite( IPrerequisite * ) = 0;
};

abstract_class ILoopMode
{
public:
	virtual bool LoopInit( KeyValues *pKeyValues, ILoopModePrerequisiteRegistry *pRegistry ) = 0;
	virtual void LoopShutdown( void ) = 0;
	virtual void OnLoopActivate( const EngineLoopState_t &state, CEventDispatcher<CEventIDManager_Default> *pEventDispatcher) = 0;
	virtual void OnLoopDeactivate( const EngineLoopState_t &state, CEventDispatcher<CEventIDManager_Default> *pEventDispatcher ) = 0;
	virtual void RegisterEventMap( CEventDispatcher<CEventIDManager_Default> *pEventDispatcher, EventMapRegistrationType_t nRegistrationType) = 0;
	virtual InputHandlerResult_t HandleInputEvent( const InputEvent_t &event, CSplitScreenSlot nSplitScreenPlayerSlot ) = 0;
	virtual ISceneView* AddViewsToSceneSystem( const EngineLoopState_t &state, double flRenderTime, double flRealTime,
		const RenderViewport_t &viewport, const RHBackColorBuffer_t &backColorBuffer ) = 0;
	virtual bool unk001( void ) = 0;
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

#endif // ILOOPMODE_H
