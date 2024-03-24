//========= Copyright � 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: A higher level link library for general use in the game and tools.
//
//=============================================================================//

/* This is totally reverse-engineered code and may be wrong */

#include "interfaces/interfaces.h"
#include "tier0/dbg.h"

IApplication *g_pApplication;
ICvar *cvar, *g_pCVar;
IUtlStringTokenSystem *g_pStringTokenSystem;
ITestScriptMgr *g_pTestScriptMgr;
IProcessUtils *g_pProcessUtils;
IFileSystem *g_pFullFileSystem;
IAsyncFileSystem *g_pAsyncFileSystem;
IResourceSystem *g_pResourceSystem;
IResourceManifestRegistry *g_pResourceManifestRegistry;
IResourceHandleUtils *g_pResourceHandleUtils;
ISchemaSystem *g_pSchemaSystem;
IResourceCompilerSystem *g_pResourceCompilerSystem;
IMaterialSystem2 *g_pMaterialSystem2;
IPostProcessingSystem *g_pPostProcessingSystem;
IInputSystem *g_pInputSystem;
IInputStackSystem *g_pInputStackSystem;
IRenderDeviceMgr *g_pRenderDeviceMgr;
IRenderUtils *g_pRenderUtils;
ISoundSystem *g_pSoundSystem;
ISoundOpSystemEdit *g_pSoundOpSystemEdit;
ISoundOpSystem *g_pSoundOpSystem;
ISteamAudio *g_pSteamAudio;
IP4 *g_pP4;
ILocalize *g_pLocalize;
IMediaFoundation *g_pMediaFoundation;
IAvi *g_pAVI;
IBik *g_pBIK;
IMeshSystem *g_pMeshSystem;
IMeshUtils *g_pMeshUtils;
IRenderDevice *g_pRenderDevice;
IRenderDeviceSetup *g_pRenderDeviceSetup;
IRenderHardwareConfig *g_pRenderHardwareConfig;
ISceneSystem *g_pSceneSystem;
IPulseSystem *g_pPulseSystem;
ISceneUtils *g_pSceneUtils;
IWorldRendererMgr *g_pWorldRendererMgr;
IAssetSystem *g_pAssetSystem;
IAssetSystemTest *g_pAssetSystemTest;
IParticleSystemMgr *g_pParticleSystemMgr;
IScriptManager *g_pScriptManager;
IPropertyEditorSystem *g_pPropertyEditorSystem;
IMatchFramework *g_pMatchFramework;
ISource2V8System *g_pV8System;
IPanoramaUIEngine *g_pPanoramaUIEngine;
IPanoramaUIClient *g_pPanoramaUIClient;
panorama::IUITextServices *g_IUITextServices;
IToolFramework2 *g_pToolFramework2;
IMapBuilderMgr *g_pPhysicsBuilderMgr;
IMapBuilderMgr *g_pVisBuilderMgr;
IMapBuilderMgr *g_pBakedLODBuilderMgr;
IHelpSystem *g_pHelpSystem;
IToolSceneNodeFactory *g_pToolSceneNodeFactory;
IEconItemToolModel *g_pEconItemToolModel;
ISchemaTestExternal_Two *g_pSchemaTestExternal_Two;
ISchemaTestExternal_One *g_pSchemaTestExternal_One;
IAnimationSystem *g_pAnimationSystem;
IAnimationSystemUtils *g_pAnimationSystemUtils;
IHammerMapLoader *g_pHammerMapLoader;
IMaterialSystem2Utils *g_pMaterialSystem2Utils;
IFontManager *g_pFontManager;
ITextLayout *g_pTextLayout;
IAssetPreviewSystem *g_pAssetPreviewSystem;
IAssetBrowserSystem *g_pAssetBrowserSystem;
IAssetRenameSystem *g_pAssetRenameSystem;
IVConComm *g_pVConComm;
IModelProcessingServices *g_pModelProcessingServices;
INetworkSystem *g_pNetworkSystem;
INetworkMessages *g_pNetworkMessages;
IFlattenedSerializers *g_pFlattenedSerializers;
ISerializedEntities *g_pSerializedEntities;
IDemoUpconverter *g_pDemoUpconverter;
ISource2Client *g_pSource2Client;
IClientUI *g_pIClientUI;
IPrediction2 *g_pClientSidePrediction;
ISource2Server *g_pSource2Server;
ISource2ServerConfig *g_pSource2ServerConfig;
ISource2Host *g_pSource2Host;
ISource2GameClients *g_pSource2GameClients;
ISource2GameEntities *g_pSource2GameEntities;
IEngineServiceMgr *g_pEngineServiceMgr;
IHostStateMgr *g_pHostStateMgr;
INetworkService *g_pNetworkService;
INetworkClientService *g_pNetworkClientService;
INetworkP2PService *g_pNetworkP2PService;
INetworkServerService *g_pNetworkServerService;
IToolService *g_pToolService;
IRenderService *g_pRenderService;
IStatsService *g_pStatsService;
IVProfService *g_pVProfService;
IInputService *g_pInputService;
IMapListService *g_pMapListService;
IGameUIService *g_pGameUIService;
ISoundService *g_pSoundService;
IBenchmarkService *g_pBenchmarkService;
IKeyValueCache *g_pKeyValueCache;
IGameResourceService *g_pGameResourceServiceClient;
IGameResourceService *g_pGameResourceServiceServer;
IVEngineClient2 *g_pEngineClient;
IVEngineServer2 *g_pEngineServer;
INetworkStringTableContainer *g_pNetworkStringTableServer;
INetworkStringTableContainer *g_pNetworkStringTableClient;
IVPhysics2 *g_pVPhysics2;
VPhys2HandleInterface *g_pVPhys2HandleInterface;
IModelDocUtils *g_pModelDocUtils;
IAnimGraphEditorUtils *g_pAnimGraphEditorUtils;
IExportSystem *g_pExportSystem;
IServerToolsInfo *g_pServerToolsInfo;
IClientToolsInfo *g_pClientToolsInfo;
IVRAD3 *g_pVRAD3;
INavSystem *g_pNavSystem;
INavGameTest *g_pNavGameTest;

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
	{ APPLICATION_INTERFACE_VERSION, &g_pApplication },
	{ CVAR_INTERFACE_VERSION, &cvar },
	{ CVAR_INTERFACE_VERSION, &g_pCVar },
	{ STRINGTOKENSYSTEM_INTERFACE_VERSION, &g_pStringTokenSystem },
	{ TESTSCRIPTMANAGER_INTERFACE_VERSION, &g_pTestScriptMgr },
	{ PROCESSUTILS_INTERFACE_VERSION, &g_pProcessUtils },
	{ FILESYSTEM_INTERFACE_VERSION, &g_pFullFileSystem },
	{ ASYNCFILESYSTEM_INTERFACE_VERSION, &g_pAsyncFileSystem },
	{ RESOURCESYSTEM_INTERFACE_VERSION, &g_pResourceSystem },
	{ RESOURCEMANIFESTREGISTRY_INTERFACE_VERSION, &g_pResourceManifestRegistry },
	{ RESOURCEHANDLEUTILS_INTERFACE_VERSION, &g_pResourceHandleUtils },
	{ SCHEMASYSTEM_INTERFACE_VERSION, &g_pSchemaSystem },
	{ RESOURCECOMPILERSYSTEM_INTERFACE_VERSION, &g_pResourceCompilerSystem },
	{ MATERIAL_SYSTEM2_INTERFACE_VERSION, &g_pMaterialSystem2 },
	{ POSTPROCESSINGSYSTEM_INTERFACE_VERSION, &g_pPostProcessingSystem },
	{ INPUTSYSTEM_INTERFACE_VERSION, &g_pInputSystem },
	{ INPUTSTACKSYSTEM_INTERFACE_VERSION, &g_pInputStackSystem },
	{ RENDER_DEVICE_MGR_INTERFACE_VERSION, &g_pRenderDeviceMgr },
	{ RENDER_UTILS_INTERFACE_VERSION, &g_pRenderUtils },
	{ SOUNDSYSTEM_INTERFACE_VERSION, &g_pSoundSystem },
	{ SOUNDOPSYSTEMEDIT_INTERFACE_VERSION, &g_pSoundOpSystemEdit },
	{ SOUNDOPSYSTEM_INTERFACE_VERSION, &g_pSoundOpSystem },
	{ STEAMAUDIO_INTERFACE_VERSION, &g_pSteamAudio },
	{ P4_INTERFACE_VERSION, &g_pP4 },
	{ LOCALIZE_INTERFACE_VERSION, &g_pLocalize },
	{ MEDIA_FOUNDATION_INTERFACE_VERSION, &g_pMediaFoundation },
	{ AVI_INTERFACE_VERSION, &g_pAVI },
	{ BIK_INTERFACE_VERSION, &g_pBIK },
	{ MESHSYSTEM_INTERFACE_VERSION, &g_pMeshSystem },
	{ MESHUTILS_INTERFACE_VERSION, &g_pMeshUtils },
	{ RENDER_DEVICE_INTERFACE_VERSION, &g_pRenderDevice },
	{ RENDER_DEVICE_SETUP_INTERFACE_VERSION, &g_pRenderDeviceSetup },
	{ RENDER_HARDWARECONFIG_INTERFACE_VERSION, &g_pRenderHardwareConfig },
	{ SCENESYSTEM_INTERFACE_VERSION, &g_pSceneSystem },
	{ PULSESYSTEM_INTERFACE_VERSION, &g_pPulseSystem },
	{ SCENEUTILS_INTERFACE_VERSION, &g_pSceneUtils },
	{ WORLD_RENDERER_MGR_INTERFACE_VERSION, &g_pWorldRendererMgr },
	{ ASSETSYSTEM_INTERFACE_VERSION, &g_pAssetSystem },
	{ ASSETSYSTEMTEST_INTERFACE_VERSION, &g_pAssetSystemTest },
	{ PARTICLESYSTEMMGR_INTERFACE_VERSION, &g_pParticleSystemMgr },
	{ SCRIPTMANAGER_INTERFACE_VERSION, &g_pScriptManager },
	{ PROPERTYEDITORSYSTEM_INTERFACE_VERSION, &g_pPropertyEditorSystem },
	{ MATCHFRAMEWORK_INTERFACE_VERSION, &g_pMatchFramework },
	{ V8SYSTEM_INTERFACE_VERSION, &g_pV8System },
	{ PANORAMAUIENGINE_INTERFACE_VERSION, &g_pPanoramaUIEngine },
	{ PANORAMAUICLIENT_INTERFACE_VERSION, &g_pPanoramaUIClient },
	{ PANORAMATEXTSERVICES_INTERFACE_VERSION, &g_IUITextServices },
	{ TOOLFRAMEWORK2_INTERFACE_VERSION, &g_pToolFramework2 },
	{ PHYSICSBUILDER_INTERFACE_VERSION, &g_pPhysicsBuilderMgr },
	{ VISBUILDER_INTERFACE_VERSION, &g_pVisBuilderMgr },
	{ BAKEDLODBUILDER_INTERFACE_VERSION, &g_pBakedLODBuilderMgr },
	{ HELPSYSTEM_INTERFACE_VERSION, &g_pHelpSystem },
	{ TOOLSCENENODEFACTORY_INTERFACE_VERSION, &g_pToolSceneNodeFactory },
	{ ECONITEMTOOLMODEL_INTERFACE_VERSION, &g_pEconItemToolModel },
	{ SCHEMATESTEXTERNALTWO_INTERFACE_VERSION, &g_pSchemaTestExternal_Two },
	{ SCHEMATESTEXTERNALONE_INTERFACE_VERSION, &g_pSchemaTestExternal_One },
	{ ANIMATIONSYSTEM_INTERFACE_VERSION, &g_pAnimationSystem },
	{ ANIMATIONSYSTEMUTILS_INTERFACE_VERSION, &g_pAnimationSystemUtils },
	{ HAMMERMAPLOADER_INTERFACE_VERSION, &g_pHammerMapLoader },
	{ MATERIALUTILS_INTERFACE_VERSION, &g_pMaterialSystem2Utils },
	{ FONTMANAGER_INTERFACE_VERSION, &g_pFontManager },
	{ TEXTLAYOUT_INTERFACE_VERSION, &g_pTextLayout },
	{ ASSETPREVIEWSYSTEM_INTERFACE_VERSION, &g_pAssetPreviewSystem },
	{ ASSETBROWSERSYSTEM_INTERFACE_VERSION, &g_pAssetBrowserSystem },
	{ ASSETRENAMESYSTEM_INTERFACE_VERSION, &g_pAssetRenameSystem },
	{ VCONCOMM_INTERFACE_VERSION, &g_pVConComm },
	{ MODELPROCESSINGSERVICES_INTERFACE_VERSION, &g_pModelProcessingServices },
	{ NETWORKSYSTEM_INTERFACE_VERSION, &g_pNetworkSystem },
	{ NETWORKMESSAGES_INTERFACE_VERSION, &g_pNetworkMessages },
	{ FLATTENEDSERIALIZERS_INTERFACE_VERSION, &g_pFlattenedSerializers },
	{ SERIALIZEDENTITIES_INTERFACE_VERSION, &g_pSerializedEntities },
	{ DEMOUPCONVERTER_INTERFACE_VERSION, &g_pDemoUpconverter },
	{ SOURCE2CLIENT_INTERFACE_VERSION, &g_pSource2Client },
	{ SOURCE2CLIENTUI_INTERFACE_VERSION, &g_pIClientUI },
	{ SOURCE2CLIENTPREDICTION_INTERFACE_VERSION, &g_pClientSidePrediction },
	{ SOURCE2SERVER_INTERFACE_VERSION, &g_pSource2Server },
	{ SOURCE2HOST_INTERFACE_VERSION, &g_pSource2Host },
	{ SOURCE2GAMECLIENTS_INTERFACE_VERSION, &g_pSource2GameClients },
	{ SOURCE2GAMEENTITIES_INTERFACE_VERSION, &g_pSource2GameEntities },
	{ ENGINESERVICEMGR_INTERFACE_VERSION, &g_pEngineServiceMgr },
	{ HOSTSTATEMGR_INTERFACE_VERSION, &g_pHostStateMgr },
	{ NETWORKSERVICE_INTERFACE_VERSION, &g_pNetworkService },
	{ NETWORKCLIENTSERVICE_INTERFACE_VERSION, &g_pNetworkClientService },
	{ NETWORKP2PSERVICE_INTERFACE_VERSION, &g_pNetworkP2PService },
	{ NETWORKSERVERSERVICE_INTERFACE_VERSION, &g_pNetworkServerService },
	{ TOOLSERVICE_INTERFACE_VERSION, &g_pToolService },
	{ RENDERSERVICE_INTERFACE_VERSION, &g_pRenderService },
	{ STATSSERVICE_INTERFACE_VERSION, &g_pStatsService },
	{ VPROFSERVICE_INTERFACE_VERSION, &g_pVProfService },
	{ INPUTSERVICE_INTERFACE_VERSION, &g_pInputService },
	{ MAPLISTSERVICE_INTERFACE_VERSION, &g_pMapListService },
	{ GAMEUISERVICE_INTERFACE_VERSION, &g_pGameUIService },
	{ SOUNDSERVICE_INTERFACE_VERSION, &g_pSoundService },
	{ BENCHMARKSERVICE_INTERFACE_VERSION, &g_pBenchmarkService },
	{ KEYVALUECACHE_INTERFACE_VERSION, &g_pKeyValueCache },
	{ GAMERESOURCESERVICECLIENT_INTERFACE_VERSION, &g_pGameResourceServiceClient },
	{ GAMERESOURCESERVICESERVER_INTERFACE_VERSION, &g_pGameResourceServiceServer },
	{ SOURCE2ENGINETOCLIENT_INTERFACE_VERSION, &g_pEngineClient },
	{ SOURCE2ENGINETOSERVER_INTERFACE_VERSION, &g_pEngineServer },
	{ SOURCE2ENGINETOSERVERSTRINGTABLE_INTERFACE_VERSION, &g_pNetworkStringTableServer },
	{ SOURCE2ENGINETOCLIENTSTRINGTABLE_INTERFACE_VERSION, &g_pNetworkStringTableClient },
	{ VPHYSICS2_INTERFACE_VERSION, &g_pVPhysics2 },
	{ VPHYSICS2HANDLE_INTERFACE_VERSION, &g_pVPhys2HandleInterface },
	{ MODELDOCUTILS_INTERFACE_VERSION, &g_pModelDocUtils },
	{ ANIMGRAPHEDITORUTILS_INTERFACE_VERSION, &g_pAnimGraphEditorUtils },
	{ EXPORTSYSTEM_INTERFACE_VERSION, &g_pExportSystem },
	{ SERVERTOOLSINFO_INTERFACE_VERSION, &g_pServerToolsInfo },
	{ CLIENTTOOLSINFO_INTERFACE_VERSION, &g_pClientToolsInfo },
	{ VRAD3_INTERFACE_VERSION, &g_pVRAD3 },
	{ NAVSYSTEM_INTERFACE_VERSION, &g_pNavSystem },
	{ NAVGAMETEST_INTERFACE_VERSION, &g_pNavGameTest },
};

static const int NUM_INTERFACES = sizeof(g_pInterfaceGlobals) / sizeof(InterfaceGlobals_t);

static int s_nConnectionCount = 0;
static int s_nRegistrationCount = 0;

static ConnectionRegistration_t s_pConnectionRegistration[NUM_INTERFACES + 1] = {};

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
