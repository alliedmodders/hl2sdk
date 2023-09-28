//===== Copyright © 2005-2005, Valve Corporation, All rights reserved. ======//
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
class ICvar;
class IProcessUtils;
class ILocalize;
class IVPhysics2;
class IPhysics2ResourceManager;
class IEventSystem;

class IAsyncFileSystem;
class IColorCorrectionSystem;
class IDebugTextureInfo;
class IBaseFileSystem;
class IFileSystem;
class IRenderHardwareConfig;
class IInputSystem;
class IInputStackSystem;
class IMaterialSystem;
class IMdlLib;
class INetworkSystem;
class INetworkSystemUtils;
class IP4;
class IQueuedLoader;
class IRenderDevice;
class IRenderDeviceSetup;
class IRenderDeviceMgr;
class IRenderUtils;
class IResourceSystem;
class IResourceSystemTools;
class IVBAllocTracker;
class IXboxInstaller;
class IMatchFramework;
class ISoundSystem;
class IMatSystemSurface;
class IGameUISystemMgr;
class IDataCache;
class IAvi;
class IBik;
class IQuickTime;
class IDmeMakefileUtils;
class ISoundEmitterSystemBase;
class ISoundEmitterSystemBaseS1;
class IMeshSystem;
class IMeshUtils;
class IWorldRendererMgr;
class ISceneSystem;
class ISceneUtils;
class IVGuiRenderSurface;
class IResourceManifestRegistry;
class IResourceHandleUtils;
class ISchemaSystem;
class IResourceCompilerSystem;
class IPostProcessingSystem;
class ISoundMixGroupSystem;
class ISoundOpSystemEdit;
class ISoundOpSystem;
class IAssetSystem;
class IAssetSystemTest;
class IParticleSystemMgr;
class IVScriptManager;
class IToolScriptManager;
class IPropertyEditorSystem;
class IModelProcessingSystem;
class IPanoramaUI;
class IToolFramework2;
class IMapBuilderMgr;
class IHelpSystem;
class IToolSceneNodeFactory;
class IToolGameSimulationSystem;
class IToolGameSimulationDispatcher;
class ISchemaTestExternal_Two;
class ISchemaTestExternal_One;
class IAnimationSystem;
class IAnimationSystemUtils;
class IHammerMapLoader;
class IMaterialUtils;
class IFontManager;
class ITextLayout;
class IAssetPreviewSystem;
class IAssetBrowserSystem;
class IVConComm;
class IConfigurationSystem;
class INetworkMessages;
class IFlattenedSerializers;
class ISource2Client;
class ISource2ClientPrediction;
class ISource2Server;
class ISource2ServerConfig;
class ISource2ServerSerializers;
class ISource2Host;
class ISource2GameClients;
class ISource2GameEntities;
class IEngineServiceMgr;
class IHostStateMgr;
class INetworkService;
class INetworkClientService;
class INetworkServerService;
class ITextMessageMgr;
class IToolService;
class IRenderService;
class IStatsService;
class IUserInfoChangeService;
class IVProfService;
class IInputService;
class IMapListService;
class IGameUIService;
class ISoundService;
class IBenchmarkService;
class IDebugService;
class IKeyValueCache;
class IGameResourceServiceClient;
class IGameResourceServiceServer;
class ISource2EngineToClient;
class ISource2EngineToServer;
class ISource2EngineToServerStringTable;
class ISource2EngineToClientStringTable;
class ISource2EngineSoundServer;
class ISource2EngineSoundClient;

class IServerUploadGameStats;
class IScaleformUI;
class IVR;

namespace vgui
{
	class ISurface;
	class IVGui;
	class IInput;
	class IPanel;
	class ILocalize;
	class ISchemeManager;
	class ISystem;
}



//-----------------------------------------------------------------------------
// Fills out global DLL exported interface pointers
//-----------------------------------------------------------------------------
#define CVAR_INTERFACE_VERSION					"VEngineCvar007"
DECLARE_TIER1_INTERFACE( ICvar, cvar );
DECLARE_TIER1_INTERFACE( ICvar, g_pCVar )

#define PROCESS_UTILS_INTERFACE_VERSION			"VProcessUtils002"
DECLARE_TIER1_INTERFACE( IProcessUtils, g_pProcessUtils );

#define VPHYSICS2_INTERFACE_VERSION				"VPhysics2_Interface_001"
DECLARE_TIER1_INTERFACE( IVPhysics2, g_pVPhysics2 );

#define VPHYSICS2_RESOURCE_MGR_INTERFACE_VERSION "VPhysX Interface ResourceMgr v0.1"
DECLARE_TIER1_INTERFACE( IPhysics2ResourceManager, g_pPhysics2ResourceManager );

#define EVENTSYSTEM_INTERFACE_VERSION "EventSystem001"
DECLARE_TIER1_INTERFACE( IEventSystem, g_pEventSystem );

#define LOCALIZE_INTERFACE_VERSION 			"Localize_001"
DECLARE_TIER2_INTERFACE( ILocalize, g_pLocalize );
DECLARE_TIER3_INTERFACE( vgui::ILocalize, g_pVGuiLocalize );

#define RENDER_DEVICE_MGR_INTERFACE_VERSION		"RenderDeviceMgr001"
DECLARE_TIER2_INTERFACE( IRenderDeviceMgr, g_pRenderDeviceMgr );

#define RENDER_UTILS_INTERFACE_VERSION		"RenderUtils_001"
DECLARE_TIER2_INTERFACE( IRenderUtils, g_pRenderUtils );

#define BASEFILESYSTEM_INTERFACE_VERSION		"VBaseFileSystem011"
DECLARE_TIER2_INTERFACE( IBaseFileSystem, g_pBaseFileSystem );

#define FILESYSTEM_INTERFACE_VERSION			"VFileSystem017"
DECLARE_TIER2_INTERFACE( IFileSystem, g_pFullFileSystem );

#define ASYNCFILESYSTEM_INTERFACE_VERSION		"VNewAsyncFileSystem001"
DECLARE_TIER2_INTERFACE( IAsyncFileSystem, g_pAsyncFileSystem );

#define RESOURCESYSTEM_INTERFACE_VERSION		"ResourceSystem009"
DECLARE_TIER2_INTERFACE( IResourceSystem, g_pResourceSystem );

#define RESOURCESYSTEMTOOLS_INTERFACE_VERSION		"ResourceSystemTools001"
DECLARE_TIER2_INTERFACE( IResourceSystemTools, g_pResourceSystemTools );

#define RESOURCEMANIFESTREGISTRY_INTERFACE_VERSION	"ResourceManifestRegistry001"
DECLARE_TIER2_INTERFACE( IResourceManifestRegistry, g_pResourceManifestRegistry );

#define RESOURCEHANDLEUTILS_INTERFACE_VERSION		"ResourceHandleUtils001"
DECLARE_TIER2_INTERFACE( IResourceHandleUtils, g_pResourceHandleUtils );

#define SCHEMASYSTEM_INTERFACE_VERSION				"SchemaSystem_001"
DECLARE_TIER2_INTERFACE( ISchemaSystem, g_pSchemaSystem );

#define RESOURCECOMPILERSYSTEM_INTERFACE_VERSION		"ResourceCompilerSystem001"
DECLARE_TIER2_INTERFACE( IResourceCompilerSystem, g_pResourceCompilerSystem );

#define POSTPROCESSINGSYSTEM_INTERFACE_VERSION		"PostProcessingSystem_001"
DECLARE_TIER2_INTERFACE( IPostProcessingSystem, g_pPostProcessingSystem );

#define MATERIAL_SYSTEM2_INTERFACE_VERSION		"VMaterialSystem2_001"
DECLARE_TIER2_INTERFACE( IMaterialSystem, g_pMaterialSystem );

#define INPUTSYSTEM_INTERFACE_VERSION			"InputSystemVersion001"
DECLARE_TIER2_INTERFACE( IInputSystem, g_pInputSystem );

#define INPUTSTACKSYSTEM_INTERFACE_VERSION		"InputStackSystemVersion001"
DECLARE_TIER2_INTERFACE( IInputStackSystem, g_pInputStackSystem );

#define NETWORKSYSTEM_INTERFACE_VERSION			"NetworkSystemVersion001"
DECLARE_TIER2_INTERFACE( INetworkSystem, g_pNetworkSystem );

#define NETWORKSYSTEMUTILS_INTERFACE_VERSION			"NetworkSystemUtilsVersion001"
DECLARE_TIER2_INTERFACE( INetworkSystemUtils, g_pNetworkSystemUtils );

#define NETWORKMESSAGES_INTERFACE_VERSION			"NetworkMessagesVersion001"
DECLARE_TIER2_INTERFACE( INetworkMessages, g_pNetworkMessages );

#define DEBUG_TEXTURE_INFO_VERSION				"DebugTextureInfo001"
DECLARE_TIER2_INTERFACE( IDebugTextureInfo, g_pMaterialSystemDebugTextureInfo );

#define VB_ALLOC_TRACKER_INTERFACE_VERSION		"VBAllocTracker001"
DECLARE_TIER2_INTERFACE( IVBAllocTracker, g_VBAllocTracker );

#define COLORCORRECTION_INTERFACE_VERSION		"COLORCORRECTION_VERSION_1"
DECLARE_TIER2_INTERFACE( IColorCorrectionSystem, colorcorrection );

#define P4_INTERFACE_VERSION					"VP4003"
DECLARE_TIER2_INTERFACE( IP4, p4 );

#define MDLLIB_INTERFACE_VERSION				"VMDLLIB001"
DECLARE_TIER2_INTERFACE( IMdlLib, mdllib );

#define QUEUEDLOADER_INTERFACE_VERSION			"QueuedLoaderVersion001"
DECLARE_TIER2_INTERFACE( IQueuedLoader, g_pQueuedLoader );

#if defined( _X360 )
#define XBOXINSTALLER_INTERFACE_VERSION			"XboxInstallerVersion001"
DECLARE_TIER2_INTERFACE( IXboxInstaller, g_pXboxInstaller );
#endif

#define MATCHFRAMEWORK_INTERFACE_VERSION		"MATCHFRAMEWORK_001"
DECLARE_TIER2_INTERFACE( IMatchFramework, g_pMatchFramework );


//-----------------------------------------------------------------------------
// Not exactly a global, but we're going to keep track of these here anyways
// NOTE: Appframework deals with connecting these bad boys. See materialsystem2app.cpp
//-----------------------------------------------------------------------------
#define RENDER_DEVICE_INTERFACE_VERSION			"RenderDevice002"
DECLARE_TIER2_INTERFACE(IRenderDevice, g_pRenderDevice);

#define RENDER_DEVICE_SETUP_INTERFACE_VERSION			"VRenderDeviceSetupV001"
DECLARE_TIER2_INTERFACE( IRenderDeviceSetup, g_pRenderDeviceSetup );

#define RENDER_HARDWARECONFIG_INTERFACE_VERSION		"RenderHardwareConfig002"
DECLARE_TIER2_INTERFACE( IRenderHardwareConfig, g_pRenderHardwareConfig );

#define SOUNDSYSTEM_INTERFACE_VERSION		"SoundSystem001"
DECLARE_TIER2_INTERFACE( ISoundSystem, g_pSoundSystem );

#define SOUNDMIXGROUPSYSTEM_INTERFACE_VERSION		"SoundMixGroupSystem001"
DECLARE_TIER2_INTERFACE( ISoundMixGroupSystem, g_pSoundMixGroupSystem);

#define SOUNDOPSYSTEMEDIT_INTERFACE_VERSION		"SoundOpSystemEdit001"
DECLARE_TIER2_INTERFACE( ISoundOpSystemEdit, g_pSoundOpSystemEdit );

#define SOUNDOPSYSTEM_INTERFACE_VERSION		"SoundOpSystem001"
DECLARE_TIER2_INTERFACE( ISoundOpSystem, g_pSoundOpSystem );

#define MESHSYSTEM_INTERFACE_VERSION			"MeshSystem001"
DECLARE_TIER3_INTERFACE( IMeshSystem, g_pMeshSystem );

#define MESHUTILS_INTERFACE_VERSION			"MeshUtils001"
DECLARE_TIER3_INTERFACE( IMeshUtils, g_pMeshUtils );

#define RENDER_SYSTEM_SURFACE_INTERFACE_VERSION	"RenderSystemSurface001"
DECLARE_TIER3_INTERFACE( IVGuiRenderSurface, g_pVGuiRenderSurface );

#define ASSETSYSTEM_INTERFACE_VERSION	"AssetSystem001"
DECLARE_TIER3_INTERFACE( IAssetSystem, g_pAssetSystem );

#define ASSETSYSTEMTEST_INTERFACE_VERSION	"AssetSystemTest001"
DECLARE_TIER3_INTERFACE( IAssetSystemTest, g_pAssetSystemTest );

#define PARTICLESYSTEMMGR_INTERFACE_VERSION	"ParticleSystemMgr002"
DECLARE_TIER3_INTERFACE( IParticleSystemMgr, g_pParticleSystemMgr );

#define VSCRIPT_INTERFACE_VERSION		"VScriptManager010"
DECLARE_TIER3_INTERFACE( IVScriptManager, g_pVScriptService );

#define TOOLSCRIPTMANAGER_INTERFACE_VERSION		"ToolScriptManager001"
DECLARE_TIER3_INTERFACE( IToolScriptManager, g_pToolScriptManager );

#define PROPERTYEDITORSYSTEM_INTERFACE_VERSION		"PropertyEditorSystem_001"
DECLARE_TIER3_INTERFACE( IPropertyEditorSystem, g_pPropertyEditorSystem );

#define MODELPROCESSINGSYSTEM_INTERFACE_VERSION		"ModelProcessingSystem001"
DECLARE_TIER3_INTERFACE( IModelProcessingSystem, g_pModelProcessingSystem );

#define PANORAMAUI_INTERFACE_VERSION		"PanoramaUI001"
DECLARE_TIER3_INTERFACE( IPanoramaUI, g_pPanoramaUI );

#define TOOLFRAMEWORK2_INTERFACE_VERSION		"ToolFramework2_001"
DECLARE_TIER3_INTERFACE( IToolFramework2, g_pToolFramework2 );

#define WORLDRENDERERBUILDER_INTERFACE_VERSION		"WorldRendererBuilderMgr001"
DECLARE_TIER3_INTERFACE( IMapBuilderMgr, g_pWorldRendererBuilderMgr );

#define LIGHTINGBUILDER_INTERFACE_VERSION		"LightingBuilderMgr001"
DECLARE_TIER3_INTERFACE( IMapBuilderMgr, g_pLightingBuilderMgr );

#define PHYSICSBUILDER_INTERFACE_VERSION		"PhysicsBuilderMgr001"
DECLARE_TIER3_INTERFACE( IMapBuilderMgr, g_pPhysicsBuilderMgr );

#define VISBUILDER_INTERFACE_VERSION		"VisBuilder_001"
DECLARE_TIER3_INTERFACE( IMapBuilderMgr, g_pVisBuilderMgr );

#define ENVIRONMENTBUILDER_INTERFACE_VERSION		"EnvironmentMapBuilder_001"
DECLARE_TIER3_INTERFACE( IMapBuilderMgr, g_pEnvironmentBuilderMgr );

#define BAKEDLODBUILDER_INTERFACE_VERSION		"BakedLODBuilderMgr001"
DECLARE_TIER3_INTERFACE( IMapBuilderMgr, g_pBakedLODBuilderMgr );

#define HELPSYSTEM_INTERFACE_VERSION		"HelpSystem_001"
DECLARE_TIER3_INTERFACE( IHelpSystem, g_pHelpSystem );

#define TOOLSCENENODEFACTORY_INTERFACE_VERSION		"ToolSceneNodeFactory_001"
DECLARE_TIER3_INTERFACE( IToolSceneNodeFactory, g_pToolSceneNodeFactory );

#define TOOLGAMESIMULATIONSYSTEM_INTERFACE_VERSION		"ToolGameSimulationSystem_001"
DECLARE_TIER3_INTERFACE( IToolGameSimulationSystem, g_pToolGameSimulationSystem );

#define TOOLGAMESIMULATIONDISPATCHER_INTERFACE_VERSION		"ToolGameSimulationDispatcher_001"
DECLARE_TIER3_INTERFACE( IToolGameSimulationDispatcher, g_pToolGameSimulationDispatcher );

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
DECLARE_TIER3_INTERFACE( IMaterialUtils, g_pMaterialUtils );

#define FONTMANAGER_INTERFACE_VERSION		"FontManager_001"
DECLARE_TIER3_INTERFACE( IFontManager, g_pFontManager );

#define TEXTLAYOUT_INTERFACE_VERSION		"TextLayout_001"
DECLARE_TIER3_INTERFACE( ITextLayout, g_pTextLayout );

#define ASSETPREVIEWSYSTEM_INTERFACE_VERSION		"AssetPreviewSystem_001"
DECLARE_TIER3_INTERFACE( IAssetPreviewSystem, g_pAssetPreviewSystem );

#define ASSETBROWSERSYSTEM_INTERFACE_VERSION		"AssetBrowserSystem_001"
DECLARE_TIER3_INTERFACE( IAssetBrowserSystem, g_pAssetBrowserSystem );

#define VCONCOMM_INTERFACE_VERSION		"VConComm001"
DECLARE_TIER3_INTERFACE( IVConComm, g_pVConComm );

#define CONFIGURATIONSYSTEM_INTERFACE_VERSION		"ConfigurationSystem_001"
DECLARE_TIER3_INTERFACE( IConfigurationSystem, g_pConfigurationSystem );

#define FLATTENEDSERIALIZERS_INTERFACE_VERSION		"FlattenedSerializersVersion001"
DECLARE_TIER3_INTERFACE( IFlattenedSerializers, g_pFlattenedSerializers );

#define SOURCE2CLIENT_INTERFACE_VERSION		"Source2Client001"
DECLARE_TIER3_INTERFACE( ISource2Client, g_pSource2Client );

#define SOURCE2CLIENTPREDICTION_INTERFACE_VERSION		"Source2ClientPrediction001"
DECLARE_TIER3_INTERFACE( ISource2ClientPrediction, g_pSource2ClientPrediction );

#define SOURCE2SERVER_INTERFACE_VERSION		"Source2Server001"
DECLARE_TIER3_INTERFACE( ISource2Server, g_pSource2Server );

#define SOURCE2SERVERCONFIG_INTERFACE_VERSION		"Source2ServerConfig001"
DECLARE_TIER3_INTERFACE( ISource2ServerConfig, g_pSource2ServerConfig );

#define SOURCE2SERVERSERIALIZERS_INTERFACE_VERSION		"Source2ServerSerializers001"
DECLARE_TIER3_INTERFACE( ISource2ServerSerializers, g_pSource2ServerSerializers );

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

#define NETWORKSERVERSERVICE_INTERFACE_VERSION		"NetworkServerService_001"
DECLARE_TIER3_INTERFACE( INetworkServerService, g_pNetworkServerService );

#define TEXTMESSAGEMGR_INTERFACE_VERSION		"TextMessageMgr_001"
DECLARE_TIER3_INTERFACE( ITextMessageMgr, g_pTextMessageMgr );

#define TOOLSERVICE_INTERFACE_VERSION		"ToolService_001"
DECLARE_TIER3_INTERFACE(IToolService, g_pToolService);

#define RENDERSERVICE_INTERFACE_VERSION		"RenderService_001"
DECLARE_TIER3_INTERFACE( IRenderService, g_pRenderService );

#define STATSSERVICE_INTERFACE_VERSION		"StatsService_001"
DECLARE_TIER3_INTERFACE( IStatsService, g_pStatsService );

#define USERINFOCHANGESERVICE_INTERFACE_VERSION		"UserInfoChangeService_001"
DECLARE_TIER3_INTERFACE( IUserInfoChangeService, g_pUserInfoChangeService );

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

#define DEBUGSERVICE_INTERFACE_VERSION		"VDebugService_001"
DECLARE_TIER3_INTERFACE( IDebugService, g_pDebugService );

#define KEYVALUECACHE_INTERFACE_VERSION		"KeyValueCache001"
DECLARE_TIER3_INTERFACE( IKeyValueCache, g_pKeyValueCache );

#define GAMERESOURCESERVICECLIENT_INTERFACE_VERSION		"GameResourceServiceClientV001"
DECLARE_TIER3_INTERFACE( IGameResourceServiceClient, g_pGameResourceServiceClient );

#define GAMERESOURCESERVICESERVER_INTERFACE_VERSION		"GameResourceServiceServerV001"
DECLARE_TIER3_INTERFACE( IGameResourceServiceServer, g_pGameResourceServiceServer );

#define SOURCE2ENGINETOCLIENT_INTERFACE_VERSION		"Source2EngineToClient001"
DECLARE_TIER3_INTERFACE( ISource2EngineToClient, g_pSource2EngineToClient );

#define SOURCE2ENGINETOSERVER_INTERFACE_VERSION		"Source2EngineToServer001"
DECLARE_TIER3_INTERFACE( ISource2EngineToServer, g_pSource2EngineToServer );

#define SOURCE2ENGINETOSERVERSTRINGTABLE_INTERFACE_VERSION		"Source2EngineToServerStringTable001"
DECLARE_TIER3_INTERFACE( ISource2EngineToServerStringTable, g_pSource2EngineToServerStringTable );

#define SOURCE2ENGINETOCLIENTSTRINGTABLE_INTERFACE_VERSION		"Source2EngineToClientStringTable001"
DECLARE_TIER3_INTERFACE( ISource2EngineToClientStringTable, g_pSource2EngineToClientStringTable );

#define SOURCE2ENGINESOUNDSERVER_INTERFACE_VERSION		"Source2EngineSoundServer001"
DECLARE_TIER3_INTERFACE( ISource2EngineSoundServer, g_pSource2EngineSoundServer );

#define SOURCE2ENGINESOUNDCLIENT_INTERFACE_VERSION		"Source2EngineSoundClient001"
DECLARE_TIER3_INTERFACE( ISource2EngineSoundClient, g_pSource2EngineSoundClient );

#define SCALEFORMUI_INTERFACE_VERSION		"ScaleformUI001"
DECLARE_TIER3_INTERFACE( IScaleformUI, g_pScaleformUI );

#define SERVERUPLOADGAMESTATS_INTERFACE_VERSION		"ServerUploadGameStats001"
DECLARE_TIER3_INTERFACE( IServerUploadGameStats, g_pServerUploadGameStats );

#define SCENESYSTEM_INTERFACE_VERSION			"SceneSystem_002"
DECLARE_TIER3_INTERFACE( ISceneSystem, g_pSceneSystem );

#define SCENEUTILS_INTERFACE_VERSION			"SceneUtils_001"
DECLARE_TIER3_INTERFACE( ISceneUtils, g_pSceneUtils );

#define VGUI_SURFACE_INTERFACE_VERSION			"VGUI_Surface032"
DECLARE_TIER3_INTERFACE( vgui::ISurface, g_pVGuiSurface );

#define SCHEME_SURFACE_INTERFACE_VERSION		"SchemeSurface001"

#define VGUI_INPUT_INTERFACE_VERSION			"VGUI_Input005"
DECLARE_TIER3_INTERFACE( vgui::IInput, g_pVGuiInput );

#define VGUI_IVGUI_INTERFACE_VERSION			"VGUI_ivgui008"
DECLARE_TIER3_INTERFACE( vgui::IVGui, g_pVGui );

#define VGUI_PANEL_INTERFACE_VERSION			"VGUI_Panel010"
DECLARE_TIER3_INTERFACE( vgui::IPanel, g_pVGuiPanel );

#define VGUI_SCHEME_INTERFACE_VERSION			"VGUI_Scheme010"
DECLARE_TIER3_INTERFACE( vgui::ISchemeManager, g_pVGuiSchemeManager );

#define VGUI_SYSTEM_INTERFACE_VERSION			"VGUI_System010"
DECLARE_TIER3_INTERFACE( vgui::ISystem, g_pVGuiSystem );

#define DATACACHE_INTERFACE_VERSION				"VDataCache003"
DECLARE_TIER3_INTERFACE( IDataCache, g_pDataCache );	// FIXME: Should IDataCache be in tier2?

#define AVI_INTERFACE_VERSION					"VAvi001"
DECLARE_TIER3_INTERFACE( IAvi, g_pAVI );

#define BIK_INTERFACE_VERSION					"VBik001"
DECLARE_TIER3_INTERFACE( IBik, g_pBIK );

#define QUICKTIME_INTERFACE_VERSION		"IQuickTime001"
DECLARE_TIER3_INTERFACE( IQuickTime, g_pQuickTime );

#define DMEMAKEFILE_UTILS_INTERFACE_VERSION		"VDmeMakeFileUtils001"
DECLARE_TIER3_INTERFACE( IDmeMakefileUtils, g_pDmeMakefileUtils );

#define SOUNDEMITTERSYSTEM_INTERFACE_VERSION	"VSoundEmitter003"
DECLARE_TIER3_INTERFACE( ISoundEmitterSystemBase, g_pSoundEmitterSystem );

#define SOUNDEMITTERSYSTEMS1_INTERFACE_VERSION	"VSoundEmitterS1_001"
DECLARE_TIER3_INTERFACE( ISoundEmitterSystemBaseS1, g_pSoundEmitterSystemS1 );

#define WORLD_RENDERER_MGR_INTERFACE_VERSION	"WorldRendererMgr001"
DECLARE_TIER3_INTERFACE( IWorldRendererMgr, g_pWorldRendererMgr );

#define VR_INTERFACE_VERSION					"VR_001"
DECLARE_TIER3_INTERFACE(IVR, vr);

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

