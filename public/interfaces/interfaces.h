//===== Copyright � 2005-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose: A higher level link library for general use in the game and tools.
//
//===========================================================================//


#ifndef INTERFACES_H
#define INTERFACES_H

#if defined( COMPILER_MSVC )
#pragma once
#endif

#include "tier0/interface.h"

// All interfaces derive from this.
class IBaseInterface
{
public:
	virtual	~IBaseInterface() {}
};

typedef void* (*InstantiateInterfaceFn)();

// Used internally to register classes.
class InterfaceReg
{
public:
	InterfaceReg(InstantiateInterfaceFn fn, const char *pName);

public:
	InstantiateInterfaceFn	m_CreateFn;
	const char				*m_pName;

	InterfaceReg			*m_pNext; // For the global list.
};

// Use this to expose an interface that can have multiple instances.
// e.g.:
// EXPOSE_INTERFACE( CInterfaceImp, IInterface, "MyInterface001" )
// This will expose a class called CInterfaceImp that implements IInterface (a pure class)
// clients can receive a pointer to this class by calling CreateInterface( "MyInterface001" )
//
// In practice, the shared header file defines the interface (IInterface) and version name ("MyInterface001")
// so that each component can use these names/vtables to communicate
//
// A single class can support multiple interfaces through multiple inheritance
//
// Use this if you want to write the factory function.
#if !defined(_STATIC_LINKED) || !defined(_SUBSYSTEM)
#define EXPOSE_INTERFACE_FN(functionName, interfaceName, versionName) \
	static InterfaceReg __g_Create##interfaceName##_reg(functionName, versionName);
#else
#define EXPOSE_INTERFACE_FN(functionName, interfaceName, versionName) \
	namespace _SUBSYSTEM \
	{	\
		static InterfaceReg __g_Create##interfaceName##_reg(functionName, versionName); \
	}
#endif

#if !defined(_STATIC_LINKED) || !defined(_SUBSYSTEM)
#define EXPOSE_INTERFACE(className, interfaceName, versionName) \
	static void* __Create##className##_interface() {return static_cast<interfaceName *>( new className );} \
	static InterfaceReg __g_Create##className##_reg(__Create##className##_interface, versionName );
#else
#define EXPOSE_INTERFACE(className, interfaceName, versionName) \
	namespace _SUBSYSTEM \
	{	\
		static void* __Create##className##_interface() {return static_cast<interfaceName *>( new className );} \
		static InterfaceReg __g_Create##className##_reg(__Create##className##_interface, versionName ); \
	}
#endif

// Use this to expose a singleton interface with a global variable you've created.
#if !defined(_STATIC_LINKED) || !defined(_SUBSYSTEM)
#define EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, globalVarName) \
	static void* __Create##className##interfaceName##_interface() {return static_cast<interfaceName *>( &globalVarName );} \
	static InterfaceReg __g_Create##className##interfaceName##_reg(__Create##className##interfaceName##_interface, versionName);
#else
#define EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, globalVarName) \
	namespace _SUBSYSTEM \
	{ \
		static void* __Create##className##interfaceName##_interface() {return static_cast<interfaceName *>( &globalVarName );} \
		static InterfaceReg __g_Create##className##interfaceName##_reg(__Create##className##interfaceName##_interface, versionName); \
	}
#endif

// Use this to expose a singleton interface. This creates the global variable for you automatically.
#if !defined(_STATIC_LINKED) || !defined(_SUBSYSTEM)
#define EXPOSE_SINGLE_INTERFACE(className, interfaceName, versionName) \
	static className __g_##className##_singleton; \
	EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, __g_##className##_singleton)
#else
#define EXPOSE_SINGLE_INTERFACE(className, interfaceName, versionName) \
	namespace _SUBSYSTEM \
	{	\
		static className __g_##className##_singleton; \
	}	\
	EXPOSE_SINGLE_INTERFACE_GLOBALVAR(className, interfaceName, versionName, __g_##className##_singleton)
#endif

// interface return status
enum
{
	IFACE_OK = 0,
	IFACE_FAILED
};

//-----------------------------------------------------------------------------
// This function is automatically exported and allows you to access any interfaces exposed with the above macros.
// if pReturnCode is set, it will return one of the following values (IFACE_OK, IFACE_FAILED)
// extend this for other error conditions/code
//-----------------------------------------------------------------------------
DLL_EXPORT void* CreateInterface(const char *pName, int *pReturnCode);

//-----------------------------------------------------------------------------
// Macros to declare interfaces appropriate for various tiers
//-----------------------------------------------------------------------------
#if 1 || defined( TIER1_LIBRARY ) || defined( TIER2_LIBRARY ) || defined( TIER3_LIBRARY ) || defined( TIER4_LIBRARY ) || defined( APPLICATION )
#define DECLARE_TIER1_INTERFACE( _Interface, _Global )	extern _Interface * _Global;
#else
#define DECLARE_TIER1_INTERFACE( _Interface, _Global )
#endif

#if 1 || defined( TIER2_LIBRARY ) || defined( TIER3_LIBRARY ) || defined( TIER4_LIBRARY ) || defined( APPLICATION )
#define DECLARE_TIER2_INTERFACE( _Interface, _Global )	extern _Interface * _Global;
#else
#define DECLARE_TIER2_INTERFACE( _Interface, _Global )
#endif

#if 1 || defined( TIER3_LIBRARY ) || defined( TIER4_LIBRARY ) || defined( APPLICATION )
#define DECLARE_TIER3_INTERFACE( _Interface, _Global )	extern _Interface * _Global;
#else
#define DECLARE_TIER3_INTERFACE( _Interface, _Global )
#endif


//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class IApplication;
class ICvar;
class IUtlStringTokenSystem;
class ITestScriptMgr;
class IProcessUtils;
class ILocalize;
class IMediaFoundation;
class IVPhysics2;
class VPhys2HandleInterface;
class IModelDocUtils;
class IAnimGraphEditorUtils;
class IExportSystem;
class IServerToolsInfo;
class IClientToolsInfo;
class INavSystem;
class INavGameTest;
class IAsyncFileSystem;
class IFileSystem;
class IRenderHardwareConfig;
class IInputSystem;
class IInputStackSystem;
class IMaterialSystem2;
class INetworkSystem;
class IP4;
class IRenderDevice;
class IRenderDeviceSetup;
class IRenderDeviceMgr;
class IRenderUtils;
class IResourceSystem;
class IMatchFramework;
class ISource2V8System;
class ISoundSystem;
class IAvi;
class IBik;
class IVRAD3;
class IMeshSystem;
class IMeshUtils;
class IWorldRendererMgr;
class ISceneSystem;
class IPulseSystem;
class ISceneUtils;
class IResourceManifestRegistry;
class IResourceHandleUtils;
class ISchemaSystem;
class IResourceCompilerSystem;
class IPostProcessingSystem;
class ISoundOpSystemEdit;
class ISoundOpSystem;
class ISteamAudio;
class IAssetSystem;
class IAssetSystemTest;
class IParticleSystemMgr;
class IScriptManager;
class IPropertyEditorSystem;
class IToolFramework2;
class IMapBuilderMgr;
class IHelpSystem;
class IToolSceneNodeFactory;
class IEconItemToolModel;
class ISchemaTestExternal_Two;
class ISchemaTestExternal_One;
class IAnimationSystem;
class IAnimationSystemUtils;
class IHammerMapLoader;
class IMaterialSystem2Utils;
class IFontManager;
class ITextLayout;
class IAssetPreviewSystem;
class IAssetBrowserSystem;
class IAssetRenameSystem;
class IVConComm;
class IModelProcessingServices;
class INetworkMessages;
class IFlattenedSerializers;
class ISerializedEntities;
class IDemoUpconverter;
class ISource2Client;
class IClientUI;
class IPrediction2;
class ISource2Server;
class ISource2ServerConfig;
class ISource2Host;
class ISource2GameClients;
class ISource2GameEntities;
class IEngineServiceMgr;
class IHostStateMgr;
class INetworkService;
class INetworkClientService;
class INetworkServerService;
class INetworkP2PService;
class IToolService;
class IRenderService;
class IStatsService;
class IVProfService;
class IInputService;
class IMapListService;
class IGameUIService;
class ISoundService;
class IBenchmarkService;
class IKeyValueCache;
class IGameResourceService;
class IVEngineClient2;
class IVEngineServer2;
class INetworkStringTableContainer;

class IPanoramaUIEngine;
class IPanoramaUIClient;

namespace panorama
{
	class IUITextServices;
};

//-----------------------------------------------------------------------------
// Fills out global DLL exported interface pointers
//-----------------------------------------------------------------------------
#define APPLICATION_INTERFACE_VERSION			"VApplication001"
DECLARE_TIER1_INTERFACE( IApplication, g_pApplication );

#define CVAR_INTERFACE_VERSION					"VEngineCvar007"
DECLARE_TIER1_INTERFACE( ICvar, cvar );
DECLARE_TIER1_INTERFACE( ICvar, g_pCVar );

#define STRINGTOKENSYSTEM_INTERFACE_VERSION		"VStringTokenSystem001"
DECLARE_TIER1_INTERFACE( IUtlStringTokenSystem, g_pStringTokenSystem );

#define TESTSCRIPTMANAGER_INTERFACE_VERSION		"TestScriptMgr001"
DECLARE_TIER1_INTERFACE( ITestScriptMgr, g_pTestScriptMgr );

#define PROCESSUTILS_INTERFACE_VERSION			"VProcessUtils002"
DECLARE_TIER1_INTERFACE( IProcessUtils, g_pProcessUtils );

#define FILESYSTEM_INTERFACE_VERSION			"VFileSystem017"
DECLARE_TIER2_INTERFACE( IFileSystem, g_pFullFileSystem );

#define ASYNCFILESYSTEM_INTERFACE_VERSION		"VAsyncFileSystem2_001"
DECLARE_TIER2_INTERFACE( IAsyncFileSystem, g_pAsyncFileSystem );

#define RESOURCESYSTEM_INTERFACE_VERSION		"ResourceSystem013"
DECLARE_TIER2_INTERFACE( IResourceSystem, g_pResourceSystem );

#define RESOURCEMANIFESTREGISTRY_INTERFACE_VERSION	"ResourceManifestRegistry001"
DECLARE_TIER2_INTERFACE( IResourceManifestRegistry, g_pResourceManifestRegistry );

#define RESOURCEHANDLEUTILS_INTERFACE_VERSION		"ResourceHandleUtils001"
DECLARE_TIER2_INTERFACE( IResourceHandleUtils, g_pResourceHandleUtils );

#define SCHEMASYSTEM_INTERFACE_VERSION				"SchemaSystem_001"
DECLARE_TIER2_INTERFACE( ISchemaSystem, g_pSchemaSystem );

#define RESOURCECOMPILERSYSTEM_INTERFACE_VERSION		"ResourceCompilerSystem001"
DECLARE_TIER2_INTERFACE( IResourceCompilerSystem, g_pResourceCompilerSystem );

#define MATERIAL_SYSTEM2_INTERFACE_VERSION		"VMaterialSystem2_001"
DECLARE_TIER2_INTERFACE( IMaterialSystem2, g_pMaterialSystem2 );

#define POSTPROCESSINGSYSTEM_INTERFACE_VERSION		"PostProcessingSystem_001"
DECLARE_TIER2_INTERFACE( IPostProcessingSystem, g_pPostProcessingSystem );

#define INPUTSYSTEM_INTERFACE_VERSION			"InputSystemVersion001"
DECLARE_TIER2_INTERFACE( IInputSystem, g_pInputSystem );

#define INPUTSTACKSYSTEM_INTERFACE_VERSION		"InputStackSystemVersion001"
DECLARE_TIER2_INTERFACE( IInputStackSystem, g_pInputStackSystem );

#define RENDER_DEVICE_MGR_INTERFACE_VERSION		"RenderDeviceMgr001"
DECLARE_TIER2_INTERFACE( IRenderDeviceMgr, g_pRenderDeviceMgr );

#define RENDER_UTILS_INTERFACE_VERSION		"RenderUtils_001"
DECLARE_TIER2_INTERFACE( IRenderUtils, g_pRenderUtils );

#define SOUNDSYSTEM_INTERFACE_VERSION		"SoundSystem001"
DECLARE_TIER2_INTERFACE( ISoundSystem, g_pSoundSystem );

#define SOUNDOPSYSTEMEDIT_INTERFACE_VERSION		"SoundOpSystemEdit001"
DECLARE_TIER2_INTERFACE( ISoundOpSystemEdit, g_pSoundOpSystemEdit );

#define SOUNDOPSYSTEM_INTERFACE_VERSION		"SoundOpSystem001"
DECLARE_TIER2_INTERFACE( ISoundOpSystem, g_pSoundOpSystem );

#define STEAMAUDIO_INTERFACE_VERSION		"SteamAudio001"
DECLARE_TIER2_INTERFACE( ISteamAudio, g_pSteamAudio );

#define P4_INTERFACE_VERSION					"VP4003"
DECLARE_TIER2_INTERFACE( IP4, g_pP4 );

#define LOCALIZE_INTERFACE_VERSION 			"Localize_001"
DECLARE_TIER2_INTERFACE( ILocalize, g_pLocalize );

#define MEDIA_FOUNDATION_INTERFACE_VERSION			"VMediaFoundation001"
DECLARE_TIER2_INTERFACE( IMediaFoundation, g_pMediaFoundation );

#define AVI_INTERFACE_VERSION					"VAvi001"
DECLARE_TIER3_INTERFACE( IAvi, g_pAVI );

#define BIK_INTERFACE_VERSION					"VBik001"
DECLARE_TIER3_INTERFACE( IBik, g_pBIK );

#define MESHSYSTEM_INTERFACE_VERSION			"MeshSystem001"
DECLARE_TIER3_INTERFACE( IMeshSystem, g_pMeshSystem );

#define MESHUTILS_INTERFACE_VERSION			"MeshUtils001"
DECLARE_TIER3_INTERFACE( IMeshUtils, g_pMeshUtils );

#define RENDER_DEVICE_INTERFACE_VERSION			"RenderDevice003"
DECLARE_TIER3_INTERFACE( IRenderDevice, g_pRenderDevice );

#define RENDER_DEVICE_SETUP_INTERFACE_VERSION			"VRenderDeviceSetupV001"
DECLARE_TIER3_INTERFACE( IRenderDeviceSetup, g_pRenderDeviceSetup );

#define RENDER_HARDWARECONFIG_INTERFACE_VERSION		"RenderHardwareConfig002"
DECLARE_TIER3_INTERFACE( IRenderHardwareConfig, g_pRenderHardwareConfig );

#define SCENESYSTEM_INTERFACE_VERSION			"SceneSystem_002"
DECLARE_TIER3_INTERFACE( ISceneSystem, g_pSceneSystem );

#define PULSESYSTEM_INTERFACE_VERSION			"IPulseSystem_001"
DECLARE_TIER3_INTERFACE( IPulseSystem, g_pPulseSystem );

#define SCENEUTILS_INTERFACE_VERSION			"SceneUtils_001"
DECLARE_TIER3_INTERFACE( ISceneUtils, g_pSceneUtils );

#define WORLD_RENDERER_MGR_INTERFACE_VERSION	"WorldRendererMgr001"
DECLARE_TIER3_INTERFACE( IWorldRendererMgr, g_pWorldRendererMgr );

#define ASSETSYSTEM_INTERFACE_VERSION	"AssetSystem001"
DECLARE_TIER3_INTERFACE( IAssetSystem, g_pAssetSystem );

#define ASSETSYSTEMTEST_INTERFACE_VERSION	"AssetSystemTest001"
DECLARE_TIER3_INTERFACE( IAssetSystemTest, g_pAssetSystemTest );

#define PARTICLESYSTEMMGR_INTERFACE_VERSION	"ParticleSystemMgr003"
DECLARE_TIER3_INTERFACE( IParticleSystemMgr, g_pParticleSystemMgr );

#define SCRIPTMANAGER_INTERFACE_VERSION		"VScriptManager010"
DECLARE_TIER3_INTERFACE( IScriptManager, g_pScriptManager );

#define PROPERTYEDITORSYSTEM_INTERFACE_VERSION		"PropertyEditorSystem_001"
DECLARE_TIER3_INTERFACE( IPropertyEditorSystem, g_pPropertyEditorSystem );

#define MATCHFRAMEWORK_INTERFACE_VERSION		"MATCHFRAMEWORK_001"
DECLARE_TIER3_INTERFACE( IMatchFramework, g_pMatchFramework );

#define V8SYSTEM_INTERFACE_VERSION		"Source2V8System001"
DECLARE_TIER3_INTERFACE( ISource2V8System, g_pV8System );

#define PANORAMAUIENGINE_INTERFACE_VERSION		"PanoramaUIEngine001"
DECLARE_TIER3_INTERFACE( IPanoramaUIEngine, g_pPanoramaUIEngine );

#define PANORAMAUICLIENT_INTERFACE_VERSION		"PanoramaUIClient001"
DECLARE_TIER3_INTERFACE( IPanoramaUIClient, g_pPanoramaUIClient );

#define PANORAMATEXTSERVICES_INTERFACE_VERSION		"PanoramaTextServices001"
DECLARE_TIER3_INTERFACE( panorama::IUITextServices, g_IUITextServices );

#define TOOLFRAMEWORK2_INTERFACE_VERSION		"ToolFramework2_002"
DECLARE_TIER3_INTERFACE( IToolFramework2, g_pToolFramework2 );

#define PHYSICSBUILDER_INTERFACE_VERSION		"PhysicsBuilderMgr001"
DECLARE_TIER3_INTERFACE( IMapBuilderMgr, g_pPhysicsBuilderMgr );

#define VISBUILDER_INTERFACE_VERSION		"VisBuilder_001"
DECLARE_TIER3_INTERFACE( IMapBuilderMgr, g_pVisBuilderMgr );

#define BAKEDLODBUILDER_INTERFACE_VERSION		"BakedLODBuilderMgr001"
DECLARE_TIER3_INTERFACE( IMapBuilderMgr, g_pBakedLODBuilderMgr );

#define HELPSYSTEM_INTERFACE_VERSION		"HelpSystem_001"
DECLARE_TIER3_INTERFACE( IHelpSystem, g_pHelpSystem );

#define TOOLSCENENODEFACTORY_INTERFACE_VERSION		"ToolSceneNodeFactory_001"
DECLARE_TIER3_INTERFACE( IToolSceneNodeFactory, g_pToolSceneNodeFactory );

#define ECONITEMTOOLMODEL_INTERFACE_VERSION		"EconItemToolModel_001"
DECLARE_TIER3_INTERFACE( IEconItemToolModel, g_pEconItemToolModel );

#define SCHEMATESTEXTERNALTWO_INTERFACE_VERSION		"SchemaTestExternal_Two_001"
DECLARE_TIER3_INTERFACE( ISchemaTestExternal_Two, g_pSchemaTestExternal_Two );

#define SCHEMATESTEXTERNALONE_INTERFACE_VERSION		"SchemaTestExternal_One_001"
DECLARE_TIER3_INTERFACE( ISchemaTestExternal_One, g_pSchemaTestExternal_One );

#define ANIMATIONSYSTEM_INTERFACE_VERSION		"AnimationSystem_001"
DECLARE_TIER3_INTERFACE( IAnimationSystem, g_pAnimationSystem );

#define ANIMATIONSYSTEMUTILS_INTERFACE_VERSION		"AnimationSystemUtils_001"
DECLARE_TIER3_INTERFACE( IAnimationSystemUtils, g_pAnimationSystemUtils );

#define HAMMERMAPLOADER_INTERFACE_VERSION		"HammerMapLoader001"
DECLARE_TIER3_INTERFACE( IHammerMapLoader, g_pHammerMapLoader );

#define MATERIALUTILS_INTERFACE_VERSION		"MaterialUtils_001"
DECLARE_TIER3_INTERFACE( IMaterialSystem2Utils, g_pMaterialSystem2Utils );

#define FONTMANAGER_INTERFACE_VERSION		"FontManager_001"
DECLARE_TIER3_INTERFACE( IFontManager, g_pFontManager );

#define TEXTLAYOUT_INTERFACE_VERSION		"TextLayout_001"
DECLARE_TIER3_INTERFACE( ITextLayout, g_pTextLayout );

#define ASSETPREVIEWSYSTEM_INTERFACE_VERSION		"AssetPreviewSystem_001"
DECLARE_TIER3_INTERFACE( IAssetPreviewSystem, g_pAssetPreviewSystem );

#define ASSETBROWSERSYSTEM_INTERFACE_VERSION		"AssetBrowserSystem_001"
DECLARE_TIER3_INTERFACE( IAssetBrowserSystem, g_pAssetBrowserSystem );

#define ASSETRENAMESYSTEM_INTERFACE_VERSION		"AssetRenameSystem_001"
DECLARE_TIER3_INTERFACE( IAssetRenameSystem, g_pAssetRenameSystem );

#define VCONCOMM_INTERFACE_VERSION		"VConComm001"
DECLARE_TIER3_INTERFACE( IVConComm, g_pVConComm );

#define MODELPROCESSINGSERVICES_INTERFACE_VERSION		"MODEL_PROCESSING_SERVICES_INTERFACE_001"
DECLARE_TIER3_INTERFACE( IModelProcessingServices, g_pModelProcessingServices );

#define NETWORKSYSTEM_INTERFACE_VERSION			"NetworkSystemVersion001"
DECLARE_TIER3_INTERFACE( INetworkSystem, g_pNetworkSystem );

#define NETWORKMESSAGES_INTERFACE_VERSION			"NetworkMessagesVersion001"
DECLARE_TIER3_INTERFACE( INetworkMessages, g_pNetworkMessages );

#define FLATTENEDSERIALIZERS_INTERFACE_VERSION		"FlattenedSerializersVersion001"
DECLARE_TIER3_INTERFACE( IFlattenedSerializers, g_pFlattenedSerializers );

#define SERIALIZEDENTITIES_INTERFACE_VERSION		"SerializedEntitiesVersion001"
DECLARE_TIER3_INTERFACE( ISerializedEntities, g_pSerializedEntities );

#define DEMOUPCONVERTER_INTERFACE_VERSION		"DemoUpconverterVersion001"
DECLARE_TIER3_INTERFACE( IDemoUpconverter, g_pDemoUpconverter );

#define SOURCE2CLIENT_INTERFACE_VERSION		"Source2Client002"
DECLARE_TIER3_INTERFACE( ISource2Client, g_pSource2Client );

#define SOURCE2CLIENTUI_INTERFACE_VERSION		"Source2ClientUI001"
DECLARE_TIER3_INTERFACE( IClientUI, g_pIClientUI );

#define SOURCE2CLIENTPREDICTION_INTERFACE_VERSION		"Source2ClientPrediction001"
DECLARE_TIER3_INTERFACE( IPrediction2, g_pClientSidePrediction );

#define SOURCE2SERVER_INTERFACE_VERSION		"Source2Server001"
DECLARE_TIER3_INTERFACE( ISource2Server, g_pSource2Server );

#define SOURCE2SERVERCONFIG_INTERFACE_VERSION		"Source2ServerConfig001"
DECLARE_TIER3_INTERFACE( ISource2ServerConfig, g_pSource2ServerConfig );

#define SOURCE2HOST_INTERFACE_VERSION		"Source2Host001"
DECLARE_TIER3_INTERFACE( ISource2Host, g_pSource2Host );

#define SOURCE2GAMECLIENTS_INTERFACE_VERSION		"Source2GameClients001"
DECLARE_TIER3_INTERFACE( ISource2GameClients, g_pSource2GameClients );

#define SOURCE2GAMEENTITIES_INTERFACE_VERSION		"Source2GameEntities001"
DECLARE_TIER3_INTERFACE( ISource2GameEntities, g_pSource2GameEntities );

#define ENGINESERVICEMGR_INTERFACE_VERSION		"EngineServiceMgr001"
DECLARE_TIER3_INTERFACE( IEngineServiceMgr, g_pEngineServiceMgr );

#define HOSTSTATEMGR_INTERFACE_VERSION		"HostStateMgr001"
DECLARE_TIER3_INTERFACE( IHostStateMgr, g_pHostStateMgr );

#define NETWORKSERVICE_INTERFACE_VERSION		"NetworkService_001"
DECLARE_TIER3_INTERFACE( INetworkService, g_pNetworkService );

#define NETWORKCLIENTSERVICE_INTERFACE_VERSION		"NetworkClientService_001"
DECLARE_TIER3_INTERFACE( INetworkClientService, g_pNetworkClientService );

#define NETWORKP2PSERVICE_INTERFACE_VERSION		"NetworkP2PService_001"
DECLARE_TIER3_INTERFACE( INetworkP2PService, g_pNetworkP2PService );

#define NETWORKSERVERSERVICE_INTERFACE_VERSION		"NetworkServerService_001"
DECLARE_TIER3_INTERFACE( INetworkServerService, g_pNetworkServerService );

#define TOOLSERVICE_INTERFACE_VERSION		"ToolService_001"
DECLARE_TIER3_INTERFACE( IToolService, g_pToolService );

#define RENDERSERVICE_INTERFACE_VERSION		"RenderService_001"
DECLARE_TIER3_INTERFACE( IRenderService, g_pRenderService );

#define STATSSERVICE_INTERFACE_VERSION		"StatsService_001"
DECLARE_TIER3_INTERFACE( IStatsService, g_pStatsService );

#define VPROFSERVICE_INTERFACE_VERSION		"VProfService_001"
DECLARE_TIER3_INTERFACE( IVProfService, g_pVProfService );

#define INPUTSERVICE_INTERFACE_VERSION		"InputService_001"
DECLARE_TIER3_INTERFACE( IInputService, g_pInputService );

#define MAPLISTSERVICE_INTERFACE_VERSION		"MapListService_001"
DECLARE_TIER3_INTERFACE( IMapListService, g_pMapListService );

#define GAMEUISERVICE_INTERFACE_VERSION		"GameUIService_001"
DECLARE_TIER3_INTERFACE( IGameUIService, g_pGameUIService );

#define SOUNDSERVICE_INTERFACE_VERSION		"SoundService_001"
DECLARE_TIER3_INTERFACE( ISoundService, g_pSoundService );

#define BENCHMARKSERVICE_INTERFACE_VERSION		"BenchmarkService001"
DECLARE_TIER3_INTERFACE( IBenchmarkService, g_pBenchmarkService );

#define KEYVALUECACHE_INTERFACE_VERSION		"KeyValueCache001"
DECLARE_TIER3_INTERFACE( IKeyValueCache, g_pKeyValueCache );

#define GAMERESOURCESERVICECLIENT_INTERFACE_VERSION		"GameResourceServiceClientV001"
DECLARE_TIER3_INTERFACE( IGameResourceService, g_pGameResourceServiceClient );

#define GAMERESOURCESERVICESERVER_INTERFACE_VERSION		"GameResourceServiceServerV001"
DECLARE_TIER3_INTERFACE( IGameResourceService, g_pGameResourceServiceServer );

#define SOURCE2ENGINETOCLIENT_INTERFACE_VERSION		"Source2EngineToClient001"
DECLARE_TIER3_INTERFACE( IVEngineClient2, g_pEngineClient );

#define SOURCE2ENGINETOSERVER_INTERFACE_VERSION		"Source2EngineToServer001"
DECLARE_TIER3_INTERFACE( IVEngineServer2, g_pEngineServer );

#define SOURCE2ENGINETOSERVERSTRINGTABLE_INTERFACE_VERSION		"Source2EngineToServerStringTable001"
DECLARE_TIER3_INTERFACE( INetworkStringTableContainer, g_pNetworkStringTableServer );

#define SOURCE2ENGINETOCLIENTSTRINGTABLE_INTERFACE_VERSION		"Source2EngineToClientStringTable001"
DECLARE_TIER3_INTERFACE( INetworkStringTableContainer, g_pNetworkStringTableClient );

#define VPHYSICS2_INTERFACE_VERSION				"VPhysics2_Interface_001"
DECLARE_TIER3_INTERFACE( IVPhysics2, g_pVPhysics2 );

#define VPHYSICS2HANDLE_INTERFACE_VERSION				"VPhysics2_Handle_Interface_001"
DECLARE_TIER3_INTERFACE( VPhys2HandleInterface, g_pVPhys2HandleInterface );

#define MODELDOCUTILS_INTERFACE_VERSION				"ModelDocUtils001"
DECLARE_TIER3_INTERFACE( IModelDocUtils, g_pModelDocUtils );

#define ANIMGRAPHEDITORUTILS_INTERFACE_VERSION				"AnimGraphEditorUtils001"
DECLARE_TIER3_INTERFACE( IAnimGraphEditorUtils, g_pAnimGraphEditorUtils );

#define EXPORTSYSTEM_INTERFACE_VERSION				"EXPORTSYSTEM_INTERFACE_VERSION_001"
DECLARE_TIER3_INTERFACE( IExportSystem, g_pExportSystem );

#define SERVERTOOLSINFO_INTERFACE_VERSION				"ServerToolsInfo_001"
DECLARE_TIER3_INTERFACE( IServerToolsInfo, g_pServerToolsInfo );

#define CLIENTTOOLSINFO_INTERFACE_VERSION				"ClientToolsInfo_001"
DECLARE_TIER3_INTERFACE( IClientToolsInfo, g_pClientToolsInfo );

#define VRAD3_INTERFACE_VERSION				"Vrad3_001"
DECLARE_TIER3_INTERFACE( IVRAD3, g_pVRAD3 );

#define NAVSYSTEM_INTERFACE_VERSION				"NavSystem001"
DECLARE_TIER3_INTERFACE( INavSystem, g_pNavSystem );

#define NAVGAMETEST_INTERFACE_VERSION				"NavGameTest001"
DECLARE_TIER3_INTERFACE( INavGameTest, g_pNavGameTest );

//-----------------------------------------------------------------------------
// Fills out global DLL exported interface pointers
//-----------------------------------------------------------------------------
void ConnectInterfaces( CreateInterfaceFn *pFactoryList, int nFactoryCount );
void DisconnectInterfaces();


//-----------------------------------------------------------------------------
// Reconnects an interface
//-----------------------------------------------------------------------------
void ReconnectInterface( CreateInterfaceFn factory, const char *pInterfaceName );


#endif // INTERFACES_H

