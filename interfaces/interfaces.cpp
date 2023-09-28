//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: A higher level link library for general use in the game and tools.
//
//=============================================================================//

/* This is totally reverse-engineered code and may be wrong */

#include "interfaces/interfaces.h"
#include "tier0/dbg.h"

ICvar *cvar, *g_pCVar;
IEventSystem *g_pEventSystem;
IProcessUtils *g_pProcessUtils;
IVPhysics2 *g_pVPhysics2;
IPhysics2ResourceManager *g_pPhysics2ResourceManager;
IBaseFileSystem *g_pBaseFileSystem;
IFileSystem *g_pFullFileSystem;
IAsyncFileSystem *g_pAsyncFileSystem;
IResourceSystem *g_pResourceSystem;
IResourceSystemTools *g_pResourceSystemTools;
IMaterialSystem *g_pMaterialSystem;
IInputSystem *g_pInputSystem;
IInputStackSystem *g_pInputStackSystem;
INetworkSystem *g_pNetworkSystem;
INetworkSystemUtils *g_pNetworkSystemUtils;
IRenderDeviceMgr *g_pRenderDeviceMgr;
IRenderUtils *g_pRenderUtils;
ISoundSystem *g_pSoundSystem;
IDebugTextureInfo *g_pMaterialSystemDebugTextureInfo;
IVBAllocTracker *g_VBAllocTracker;
IColorCorrectionSystem *colorcorrection;
IP4 *p4;
IMdlLib *mdllib;
IQueuedLoader *g_pQueuedLoader;
vgui::IVGui *g_pVGui;
vgui::IInput *g_pVGuiInput;
vgui::IPanel *g_pVGuiPanel;
vgui::ISurface *g_pVGuiSurface;
vgui::ISchemeManager *g_pVGuiSchemeManager;
vgui::ISystem *g_pVGuiSystem;
ILocalize *g_pLocalize;
vgui::ILocalize *g_pVGuiLocalize;
IDataCache *g_pDataCache;
IAvi *g_pAVI;
IBik *g_pBIK;
IQuickTime *g_pQuickTime;
IDmeMakefileUtils *g_pDmeMakefileUtils;
ISoundEmitterSystemBase *g_pSoundEmitterSystem;
ISoundEmitterSystemBaseS1 *g_pSoundEmitterSystemS1;
IMeshSystem *g_pMeshSystem;
IMeshUtils *g_pMeshUtils;
IRenderDevice *g_pRenderDevice;
IRenderDeviceSetup *g_pRenderDeviceSetup;
IRenderHardwareConfig *g_pRenderHardwareConfig;
ISceneSystem *g_pSceneSystem;
ISceneUtils *g_pSceneUtils;
IWorldRendererMgr *g_pWorldRendererMgr;
IVGuiRenderSurface *g_pVGuiRenderSurface;
IMatchFramework *g_pMatchFramework;
IResourceManifestRegistry *g_pResourceManifestRegistry;
IResourceHandleUtils *g_pResourceHandleUtils;
ISchemaSystem *g_pSchemaSystem;
IResourceCompilerSystem *g_pResourceCompilerSystem;
IPostProcessingSystem *g_pPostProcessingSystem;
ISoundMixGroupSystem *g_pSoundMixGroupSystem;
ISoundOpSystemEdit *g_pSoundOpSystemEdit;
ISoundOpSystem *g_pSoundOpSystem;
IAssetSystem *g_pAssetSystem;
IAssetSystemTest *g_pAssetSystemTest;
IParticleSystemMgr *g_pParticleSystemMgr;
IVScriptManager *g_pVScriptService;
IToolScriptManager *g_pToolScriptManager;
IPropertyEditorSystem *g_pPropertyEditorSystem;
IModelProcessingSystem *g_pModelProcessingSystem;
IPanoramaUI *g_pPanoramaUI;
IToolFramework2 *g_pToolFramework2;
IMapBuilderMgr *g_pWorldRendererBuilderMgr;
IMapBuilderMgr *g_pLightingBuilderMgr;
IMapBuilderMgr *g_pPhysicsBuilderMgr;
IMapBuilderMgr *g_pVisBuilderMgr;
IMapBuilderMgr *g_pEnvironmentBuilderMgr;
IMapBuilderMgr *g_pBakedLODBuilderMgr;
IHelpSystem *g_pHelpSystem;
IToolSceneNodeFactory *g_pToolSceneNodeFactory;
IToolGameSimulationSystem *g_pToolGameSimulationSystem;
IToolGameSimulationDispatcher *g_pToolGameSimulationDispatcher;
ISchemaTestExternal_Two *g_pSchemaTestExternal_Two;
ISchemaTestExternal_One *g_pSchemaTestExternal_One;
IAnimationSystem *g_pAnimationSystem;
IAnimationSystemUtils *g_pAnimationSystemUtils;
IHammerMapLoader *g_pHammerMapLoader;
IMaterialUtils *g_pMaterialUtils;
IFontManager *g_pFontManager;
ITextLayout *g_pTextLayout;
IAssetPreviewSystem *g_pAssetPreviewSystem;
IAssetBrowserSystem *g_pAssetBrowserSystem;
IVConComm *g_pVConComm;
IConfigurationSystem *g_pConfigurationSystem;
INetworkMessages *g_pNetworkMessages;
IFlattenedSerializers *g_pFlattenedSerializers;
ISource2Client *g_pSource2Client;
ISource2ClientPrediction *g_pSource2ClientPrediction;
ISource2Server *g_pSource2Server;
ISource2ServerConfig *g_pSource2ServerConfig;
ISource2ServerSerializers *g_pSource2ServerSerializers;
ISource2Host *g_pSource2Host;
ISource2GameClients *g_pSource2GameClients;
ISource2GameEntities *g_pSource2GameEntities;
IEngineServiceMgr *g_pEngineServiceMgr;
IHostStateMgr *g_pHostStateMgr;
INetworkService *g_pNetworkService;
INetworkClientService *g_pNetworkClientService;
INetworkServerService *g_pNetworkServerService;
ITextMessageMgr *g_pTextMessageMgr;
IToolService *g_pToolService;
IRenderService *g_pRenderService;
IStatsService *g_pStatsService;
IUserInfoChangeService *g_pUserInfoChangeService;
IVProfService *g_pVProfService;
IInputService *g_pInputService;
IMapListService *g_pMapListService;
IGameUIService *g_pGameUIService;
ISoundService *g_pSoundService;
IBenchmarkService *g_pBenchmarkService;
IDebugService *g_pDebugService;
IKeyValueCache *g_pKeyValueCache;
IGameResourceServiceClient *g_pGameResourceServiceClient;
IGameResourceServiceServer *g_pGameResourceServiceServer;
ISource2EngineToClient *g_pSource2EngineToClient;
ISource2EngineToServer *g_pSource2EngineToServer;
ISource2EngineToServerStringTable *g_pSource2EngineToServerStringTable;
ISource2EngineToClientStringTable *g_pSource2EngineToClientStringTable;
ISource2EngineSoundServer *g_pSource2EngineSoundServer;
ISource2EngineSoundClient *g_pSource2EngineSoundClient;

IServerUploadGameStats *g_pServerUploadGameStats;
IScaleformUI *g_pScaleformUI;
IVR *vr;

struct InterfaceGlobals_t
{
	const char *m_pInterfaceName;
	void *m_ppGlobal;
};

struct ConnectionRegistration_t
{
	void *m_ppGlobalStorage;
	int m_nConnectionPhase;
};

static const InterfaceGlobals_t g_pInterfaceGlobals[] =
{
	{ CVAR_INTERFACE_VERSION, &cvar },
	{ CVAR_INTERFACE_VERSION, &g_pCVar },
	{ EVENTSYSTEM_INTERFACE_VERSION, &g_pEventSystem },
	{ PROCESS_UTILS_INTERFACE_VERSION, &g_pProcessUtils },
	{ VPHYSICS2_INTERFACE_VERSION, &g_pVPhysics2 },
	{ VPHYSICS2_RESOURCE_MGR_INTERFACE_VERSION, &g_pPhysics2ResourceManager },
	{ BASEFILESYSTEM_INTERFACE_VERSION, &g_pBaseFileSystem },
	{ FILESYSTEM_INTERFACE_VERSION, &g_pFullFileSystem },
	{ ASYNCFILESYSTEM_INTERFACE_VERSION, &g_pAsyncFileSystem },
	{ RESOURCESYSTEM_INTERFACE_VERSION, &g_pResourceSystem },
	{ RESOURCESYSTEMTOOLS_INTERFACE_VERSION, &g_pResourceSystemTools },
	{ RESOURCEMANIFESTREGISTRY_INTERFACE_VERSION, &g_pResourceManifestRegistry },
	{ RESOURCEHANDLEUTILS_INTERFACE_VERSION, &g_pResourceHandleUtils },
	{ SCHEMASYSTEM_INTERFACE_VERSION, &g_pSchemaSystem },
	{ RESOURCECOMPILERSYSTEM_INTERFACE_VERSION, &g_pResourceCompilerSystem },
	{ MATERIAL_SYSTEM2_INTERFACE_VERSION, &g_pMaterialSystem },
	{ POSTPROCESSINGSYSTEM_INTERFACE_VERSION, &g_pPostProcessingSystem },
	{ INPUTSYSTEM_INTERFACE_VERSION, &g_pInputSystem },
	{ INPUTSTACKSYSTEM_INTERFACE_VERSION, &g_pInputStackSystem },
	{ RENDER_DEVICE_MGR_INTERFACE_VERSION, &g_pRenderDeviceMgr },
	{ RENDER_UTILS_INTERFACE_VERSION, &g_pRenderUtils },
	{ SOUNDSYSTEM_INTERFACE_VERSION, &g_pSoundSystem },
	{ SOUNDMIXGROUPSYSTEM_INTERFACE_VERSION, &g_pSoundMixGroupSystem },
	{ SOUNDOPSYSTEMEDIT_INTERFACE_VERSION, &g_pSoundOpSystemEdit },
	{ SOUNDOPSYSTEM_INTERFACE_VERSION, &g_pSoundOpSystem },
	{ DEBUG_TEXTURE_INFO_VERSION, &g_pMaterialSystemDebugTextureInfo },
	{ VB_ALLOC_TRACKER_INTERFACE_VERSION, &g_VBAllocTracker },
	{ COLORCORRECTION_INTERFACE_VERSION, &colorcorrection },
	{ P4_INTERFACE_VERSION, &p4 },
	{ MDLLIB_INTERFACE_VERSION, &mdllib },
	{ QUEUEDLOADER_INTERFACE_VERSION, &g_pQueuedLoader },
	{ VGUI_IVGUI_INTERFACE_VERSION, &g_pVGui },
	{ VGUI_INPUT_INTERFACE_VERSION, &g_pVGuiInput },
	{ VGUI_PANEL_INTERFACE_VERSION, &g_pVGuiPanel },
	{ VGUI_SURFACE_INTERFACE_VERSION, &g_pVGuiSurface },
	{ VGUI_SCHEME_INTERFACE_VERSION, &g_pVGuiSchemeManager },
	{ VGUI_SYSTEM_INTERFACE_VERSION, &g_pVGuiSystem },
	{ LOCALIZE_INTERFACE_VERSION, &g_pLocalize },
	{ LOCALIZE_INTERFACE_VERSION, &g_pVGuiLocalize },
	{ DATACACHE_INTERFACE_VERSION, &g_pDataCache },
	{ AVI_INTERFACE_VERSION, &g_pAVI },
	{ BIK_INTERFACE_VERSION, &g_pBIK },
	{ QUICKTIME_INTERFACE_VERSION, &g_pQuickTime },
	{ DMEMAKEFILE_UTILS_INTERFACE_VERSION, &g_pDmeMakefileUtils },
	{ SOUNDEMITTERSYSTEM_INTERFACE_VERSION, &g_pSoundEmitterSystem },
	{ SOUNDEMITTERSYSTEMS1_INTERFACE_VERSION, &g_pSoundEmitterSystemS1 },
	{ MESHSYSTEM_INTERFACE_VERSION, &g_pMeshSystem },
	{ MESHUTILS_INTERFACE_VERSION, &g_pMeshUtils },
	{ RENDER_DEVICE_INTERFACE_VERSION, &g_pRenderDevice },
	{ RENDER_DEVICE_SETUP_INTERFACE_VERSION, &g_pRenderDeviceSetup },
	{ RENDER_HARDWARECONFIG_INTERFACE_VERSION, &g_pRenderHardwareConfig },
	{ SCENESYSTEM_INTERFACE_VERSION, &g_pSceneSystem },
	{ SCENEUTILS_INTERFACE_VERSION, &g_pSceneUtils },
	{ WORLD_RENDERER_MGR_INTERFACE_VERSION, &g_pWorldRendererMgr },
	{ RENDER_SYSTEM_SURFACE_INTERFACE_VERSION, &g_pVGuiRenderSurface },
	{ ASSETSYSTEM_INTERFACE_VERSION, &g_pAssetSystem },
	{ ASSETSYSTEMTEST_INTERFACE_VERSION, &g_pAssetSystemTest },
	{ PARTICLESYSTEMMGR_INTERFACE_VERSION, &g_pParticleSystemMgr },
	{ VSCRIPT_INTERFACE_VERSION, &g_pVScriptService },
	{ TOOLSCRIPTMANAGER_INTERFACE_VERSION, &g_pToolScriptManager },
	{ PROPERTYEDITORSYSTEM_INTERFACE_VERSION, &g_pPropertyEditorSystem },
	{ MODELPROCESSINGSYSTEM_INTERFACE_VERSION, &g_pModelProcessingSystem },
	{ MATCHFRAMEWORK_INTERFACE_VERSION, &g_pMatchFramework },
	{ PANORAMAUI_INTERFACE_VERSION, &g_pPanoramaUI },
	{ TOOLFRAMEWORK2_INTERFACE_VERSION, &g_pToolFramework2 },
	{ WORLDRENDERERBUILDER_INTERFACE_VERSION, &g_pWorldRendererBuilderMgr },
	{ LIGHTINGBUILDER_INTERFACE_VERSION, &g_pLightingBuilderMgr },
	{ PHYSICSBUILDER_INTERFACE_VERSION, &g_pPhysicsBuilderMgr },
	{ VISBUILDER_INTERFACE_VERSION, &g_pVisBuilderMgr },
	{ ENVIRONMENTBUILDER_INTERFACE_VERSION, &g_pEnvironmentBuilderMgr },
	{ BAKEDLODBUILDER_INTERFACE_VERSION, &g_pBakedLODBuilderMgr },
	{ HELPSYSTEM_INTERFACE_VERSION, &g_pHelpSystem },
	{ TOOLSCENENODEFACTORY_INTERFACE_VERSION, &g_pToolSceneNodeFactory },
	{ TOOLGAMESIMULATIONSYSTEM_INTERFACE_VERSION, &g_pToolGameSimulationSystem },
	{ TOOLGAMESIMULATIONDISPATCHER_INTERFACE_VERSION, &g_pToolGameSimulationDispatcher },
	{ SCHEMATESTEXTERNALTWO_INTERFACE_VERSION, &g_pSchemaTestExternal_Two },
	{ SCHEMATESTEXTERNALONE_INTERFACE_VERSION, &g_pSchemaTestExternal_One },
	{ ANIMATIONSYSTEM_INTERFACE_VERSION, &g_pAnimationSystem },
	{ ANIMATIONSYSTEMUTILS_INTERFACE_VERSION, &g_pAnimationSystemUtils },
	{ HAMMERMAPLOADER_INTERFACE_VERSION, &g_pHammerMapLoader },
	{ MATERIALUTILS_INTERFACE_VERSION, &g_pMaterialUtils },
	{ FONTMANAGER_INTERFACE_VERSION, &g_pFontManager },
	{ TEXTLAYOUT_INTERFACE_VERSION, &g_pTextLayout },
	{ ASSETPREVIEWSYSTEM_INTERFACE_VERSION, &g_pAssetPreviewSystem },
	{ ASSETBROWSERSYSTEM_INTERFACE_VERSION, &g_pAssetBrowserSystem },
	{ VCONCOMM_INTERFACE_VERSION, &g_pVConComm },
	{ CONFIGURATIONSYSTEM_INTERFACE_VERSION, &g_pConfigurationSystem },
	{ NETWORKSYSTEM_INTERFACE_VERSION, &g_pNetworkSystem },
	{ NETWORKSYSTEMUTILS_INTERFACE_VERSION, &g_pNetworkSystemUtils },
	{ NETWORKMESSAGES_INTERFACE_VERSION, &g_pNetworkMessages },
	{ FLATTENEDSERIALIZERS_INTERFACE_VERSION, &g_pFlattenedSerializers },
	{ SOURCE2CLIENT_INTERFACE_VERSION, &g_pSource2Client },
	{ SOURCE2CLIENTPREDICTION_INTERFACE_VERSION, &g_pSource2ClientPrediction },
	{ SOURCE2SERVER_INTERFACE_VERSION, &g_pSource2Server },
	{ SOURCE2SERVERCONFIG_INTERFACE_VERSION, &g_pSource2ServerConfig },
	{ SOURCE2SERVERSERIALIZERS_INTERFACE_VERSION, &g_pSource2ServerSerializers },
	{ SOURCE2HOST_INTERFACE_VERSION, &g_pSource2Host },
	{ SOURCE2GAMECLIENTS_INTERFACE_VERSION, &g_pSource2GameClients },
	{ SOURCE2GAMEENTITIES_INTERFACE_VERSION, &g_pSource2GameEntities },
	{ ENGINESERVICEMGR_INTERFACE_VERSION, &g_pEngineServiceMgr },
	{ HOSTSTATEMGR_INTERFACE_VERSION, &g_pHostStateMgr },
	{ NETWORKSERVICE_INTERFACE_VERSION, &g_pNetworkService },
	{ NETWORKCLIENTSERVICE_INTERFACE_VERSION, &g_pNetworkClientService },
	{ NETWORKSERVERSERVICE_INTERFACE_VERSION, &g_pNetworkServerService },
	{ TEXTMESSAGEMGR_INTERFACE_VERSION, &g_pTextMessageMgr },
	{ TOOLSERVICE_INTERFACE_VERSION, &g_pToolService },
	{ RENDERSERVICE_INTERFACE_VERSION, &g_pRenderService },
	{ STATSSERVICE_INTERFACE_VERSION, &g_pStatsService },
	{ USERINFOCHANGESERVICE_INTERFACE_VERSION, &g_pUserInfoChangeService },
	{ VPROFSERVICE_INTERFACE_VERSION, &g_pVProfService },
	{ INPUTSERVICE_INTERFACE_VERSION, &g_pInputService },
	{ MAPLISTSERVICE_INTERFACE_VERSION, &g_pMapListService },
	{ GAMEUISERVICE_INTERFACE_VERSION, &g_pGameUIService },
	{ SOUNDSERVICE_INTERFACE_VERSION, &g_pSoundService },
	{ BENCHMARKSERVICE_INTERFACE_VERSION, &g_pBenchmarkService },
	{ DEBUGSERVICE_INTERFACE_VERSION, &g_pDebugService },
	{ KEYVALUECACHE_INTERFACE_VERSION, &g_pKeyValueCache },
	{ GAMERESOURCESERVICECLIENT_INTERFACE_VERSION, &g_pGameResourceServiceClient },
	{ GAMERESOURCESERVICESERVER_INTERFACE_VERSION, &g_pGameResourceServiceServer },
	{ SOURCE2ENGINETOCLIENT_INTERFACE_VERSION, &g_pSource2EngineToClient },
	{ SOURCE2ENGINETOSERVER_INTERFACE_VERSION, &g_pSource2EngineToServer },
	{ SOURCE2ENGINETOSERVERSTRINGTABLE_INTERFACE_VERSION, &g_pSource2EngineToServerStringTable },
	{ SOURCE2ENGINETOCLIENTSTRINGTABLE_INTERFACE_VERSION, &g_pSource2EngineToClientStringTable },
	{ SOURCE2ENGINESOUNDSERVER_INTERFACE_VERSION, &g_pSource2EngineSoundServer },
	{ SOURCE2ENGINESOUNDCLIENT_INTERFACE_VERSION, &g_pSource2EngineSoundClient },
	/*
	.data:4C0E39A8                 dd offset aVphysics2_inte ; "VPhysics2_Interface_001"
	.data:4C0E39B0                 dd offset aVphysics2_hand ; "VPhysics2_Handle_Interface_001"
	*/
	{ SERVERUPLOADGAMESTATS_INTERFACE_VERSION, &g_pServerUploadGameStats },
	{ SCALEFORMUI_INTERFACE_VERSION, &g_pScaleformUI },
	{ VR_INTERFACE_VERSION,	vr }
};

static const int NUM_INTERFACES = sizeof(g_pInterfaceGlobals) / sizeof(InterfaceGlobals_t);

static int s_nConnectionCount;
static int s_nRegistrationCount;

static ConnectionRegistration_t s_pConnectionRegistration[NUM_INTERFACES + 1];

void ReconnectInterface(CreateInterfaceFn factory, char const *pInterfaceName, void **ppGlobal);

void ConnectInterfaces(CreateInterfaceFn *pFactoryList, int nFactoryCount)
{
	if (s_nRegistrationCount < 0)
	{
		//Error("APPSYSTEM: In ConnectInterfaces(), s_nRegistrationCount is %d!\n", s_nRegistrationCount);
		Plat_ExitProcess(1);
		s_nConnectionCount++;
		return;
	}

	if (s_nRegistrationCount)
	{
		for (int i = 0; i < nFactoryCount; i++)
		{
			for (int j = 0; j < NUM_INTERFACES; j++)
			{
				ReconnectInterface(pFactoryList[i],  g_pInterfaceGlobals[j].m_pInterfaceName, (void **)g_pInterfaceGlobals[j].m_ppGlobal);
			}
		}

		s_nConnectionCount++;
		return;
	}

	for (int i = 0; i < nFactoryCount; i++)
	{
		for (int j = 0; j < NUM_INTERFACES; j++)
		{
			const InterfaceGlobals_t &iface = g_pInterfaceGlobals[j];

			if (!(*(void **)iface.m_ppGlobal))
			{
				void *ptr = pFactoryList[i](iface.m_pInterfaceName, NULL);
				*(void **)iface.m_ppGlobal = ptr;
				if (ptr)
				{
					ConnectionRegistration_t &reg = s_pConnectionRegistration[s_nRegistrationCount++];
					reg.m_ppGlobalStorage = iface.m_ppGlobal;
					reg.m_nConnectionPhase = s_nConnectionCount;
				}
			}
		}
	}

	s_nConnectionCount++;
}

void DisconnectInterfaces()
{
	if (--s_nConnectionCount >= 0)
	{
		for (int i = 0; i < s_nRegistrationCount; i++)
		{
			ConnectionRegistration_t &reg = s_pConnectionRegistration[i];
			if (reg.m_nConnectionPhase == s_nConnectionCount)
				reg.m_ppGlobalStorage = NULL;
		}
	}
}

void ReconnectInterface(CreateInterfaceFn factory, char const *pInterfaceName, void **ppGlobal)
{
	bool got = false;

	*ppGlobal = factory(pInterfaceName, NULL);

	for (int i = 0; i < s_nRegistrationCount; i++)
	{
		if (s_pConnectionRegistration[i].m_ppGlobalStorage == ppGlobal)
		{
			got = true;
			break;
		}
	}

	if ((s_nRegistrationCount <= 0 || !got) && *ppGlobal)
	{
		ConnectionRegistration_t &reg = s_pConnectionRegistration[s_nRegistrationCount++];
		reg.m_ppGlobalStorage = ppGlobal;
		reg.m_nConnectionPhase = s_nConnectionCount;
	}
}

void ReconnectInterface(CreateInterfaceFn factory, const char *pInterfaceName)
{
	for (int i = 0; i < NUM_INTERFACES; i++)
	{
		const InterfaceGlobals_t &iface = g_pInterfaceGlobals[i];

		if (strcmp(iface.m_pInterfaceName, pInterfaceName) == 0)
			ReconnectInterface(factory, iface.m_pInterfaceName, (void **)iface.m_ppGlobal);
	}
}

// ------------------------------------------------------------------------------------ //
// InterfaceReg.
// ------------------------------------------------------------------------------------ //
InterfaceReg *s_pInterfaceRegs = NULL;

InterfaceReg::InterfaceReg(InstantiateInterfaceFn fn, const char *pName) :
	m_pName(pName)
{
	m_CreateFn = fn;
	m_pNext = s_pInterfaceRegs;
	s_pInterfaceRegs = this;
}

// ------------------------------------------------------------------------------------ //
// CreateInterface.
// This is the primary exported function by a dll, referenced by name via dynamic binding
// that exposes an opqaue function pointer to the interface.
// ------------------------------------------------------------------------------------ //
void* CreateInterface(const char *pName, int *pReturnCode)
{
	InterfaceReg *pCur;

	for (pCur = s_pInterfaceRegs; pCur; pCur = pCur->m_pNext)
	{
		if (strcmp(pCur->m_pName, pName) == 0)
		{
			if (pReturnCode)
			{
				*pReturnCode = IFACE_OK;
			}
			return pCur->m_CreateFn();
		}
	}

	if (pReturnCode)
	{
		*pReturnCode = IFACE_FAILED;
	}
	return NULL;
}
