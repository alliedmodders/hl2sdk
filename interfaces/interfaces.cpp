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
IPhysics2 *g_pPhysics2;
IPhysics2ActorManager *g_pPhysics2ActorManager;
IPhysics2ResourceManager *g_pPhysics2ResourceManager;
IFileSystem *g_pFullFileSystem;
IAsyncFileSystem *g_pAsyncFileSystem;
IResourceSystem *g_pResourceSystem;
IMaterialSystem *materials, *g_pMaterialSystem;
IMaterialSystem2 *g_pMaterialSystem2;
IInputSystem *g_pInputSystem;
IInputStackSystem *g_pInputStackSystem;
INetworkSystem *g_pNetworkSystem;
IRenderDeviceMgr *g_pRenderDeviceMgr;
IMaterialSystemHardwareConfig *g_pMaterialSystemHardwareConfig;
ISoundSystem *g_pSoundSystem;
IDebugTextureInfo *g_pMaterialSystemDebugTextureInfo;
IVBAllocTracker *g_VBAllocTracker;
IColorCorrectionSystem *colorcorrection;
IP4 *p4;
IMdlLib *mdllib;
IQueuedLoader *g_pQueuedLoader;
IResourceAccessControl *g_pResourceAccessControl;
IPrecacheSystem *g_pPrecacheSystem;
IStudioRender *g_pStudioRender, *studiorender;
vgui::IVGui *g_pVGui;
vgui::IInput *g_pVGuiInput;
vgui::IPanel *g_pVGuiPanel;
vgui::ISurface *g_pVGuiSurface;
vgui::ISchemeManager *g_pVGuiSchemeManager;
vgui::ISystem *g_pVGuiSystem;
ILocalize *g_pLocalize;
vgui::ILocalize *g_pVGuiLocalize;
IMatSystemSurface *g_pMatSystemSurface;
IDataCache *g_pDataCache;
IMDLCache *g_pMDLCache, *mdlcache;
IAvi *g_pAVI;
IBik *g_pBIK;
IDmeMakefileUtils *g_pDmeMakefileUtils;
IPhysicsCollision *g_pPhysicsCollision;
ISoundEmitterSystemBase *g_pSoundEmitterSystem;
IMeshSystem *g_pMeshSystem;
IRenderDevice *g_pRenderDevice;
IRenderHardwareConfig *g_pRenderHardwareConfig;
ISceneSystem *g_pSceneSystem;
IWorldRendererMgr *g_pWorldRendererMgr;
IVGuiRenderSurface *g_pVGuiRenderSurface;
IMatchFramework *g_pMatchFramework;
IGameUISystemMgr *g_pGameUISystemMgr;

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
	{ VPHYSICS2_INTERFACE_VERSION, &g_pPhysics2 },
	{ VPHYSICS2_ACTOR_MGR_INTERFACE_VERSION, &g_pPhysics2ActorManager },
	{ VPHYSICS2_RESOURCE_MGR_INTERFACE_VERSION, &g_pPhysics2ResourceManager },
	{ FILESYSTEM_INTERFACE_VERSION, &g_pFullFileSystem },
	{ ASYNCFILESYSTEM_INTERFACE_VERSION, &g_pAsyncFileSystem },
	{ RESOURCESYSTEM_INTERFACE_VERSION, &g_pResourceSystem },
	{ MATERIAL_SYSTEM_INTERFACE_VERSION, &materials },
	{ MATERIAL_SYSTEM_INTERFACE_VERSION, &g_pMaterialSystem },
	{ MATERIAL_SYSTEM2_INTERFACE_VERSION, &g_pMaterialSystem2 },
	{ INPUTSYSTEM_INTERFACE_VERSION, &g_pInputSystem },
	{ INPUTSTACKSYSTEM_INTERFACE_VERSION, &g_pInputStackSystem },
	{ NETWORKSYSTEM_INTERFACE_VERSION, &g_pNetworkSystem },
	{ RENDER_DEVICE_MGR_INTERFACE_VERSION, &g_pRenderDeviceMgr },
	{ MATERIALSYSTEM_HARDWARECONFIG_INTERFACE_VERSION, &g_pMaterialSystemHardwareConfig },
	{ SOUNDSYSTEM_INTERFACE_VERSION, &g_pSoundSystem },
	{ DEBUG_TEXTURE_INFO_VERSION, &g_pMaterialSystemDebugTextureInfo },
	{ VB_ALLOC_TRACKER_INTERFACE_VERSION, &g_VBAllocTracker },
	{ COLORCORRECTION_INTERFACE_VERSION, &colorcorrection },
	{ P4_INTERFACE_VERSION, &p4 },
	{ MDLLIB_INTERFACE_VERSION, &mdllib },
	{ QUEUEDLOADER_INTERFACE_VERSION, &g_pQueuedLoader },
	{ RESOURCE_ACCESS_CONTROL_INTERFACE_VERSION, &g_pResourceAccessControl },
	{ PRECACHE_SYSTEM_INTERFACE_VERSION, &g_pPrecacheSystem },
	{ STUDIO_RENDER_INTERFACE_VERSION, &g_pStudioRender },
	{ STUDIO_RENDER_INTERFACE_VERSION, &studiorender },
	{ VGUI_IVGUI_INTERFACE_VERSION, &g_pVGui },
	{ VGUI_INPUT_INTERFACE_VERSION, &g_pVGuiInput },
	{ VGUI_PANEL_INTERFACE_VERSION, &g_pVGuiPanel },
	{ VGUI_SURFACE_INTERFACE_VERSION, &g_pVGuiSurface },
	{ VGUI_SCHEME_INTERFACE_VERSION, &g_pVGuiSchemeManager },
	{ VGUI_SYSTEM_INTERFACE_VERSION, &g_pVGuiSystem },
	{ LOCALIZE_INTERFACE_VERSION, &g_pLocalize },
	{ LOCALIZE_INTERFACE_VERSION, &g_pVGuiLocalize },
	{ MAT_SYSTEM_SURFACE_INTERFACE_VERSION, &g_pMatSystemSurface },
	{ DATACACHE_INTERFACE_VERSION, &g_pDataCache },
	{ MDLCACHE_INTERFACE_VERSION, &g_pMDLCache },
	{ MDLCACHE_INTERFACE_VERSION, &mdlcache },
	{ AVI_INTERFACE_VERSION, &g_pAVI },
	{ BIK_INTERFACE_VERSION, &g_pBIK },
	{ DMEMAKEFILE_UTILS_INTERFACE_VERSION, &g_pDmeMakefileUtils },
	{ VPHYSICS_COLLISION_INTERFACE_VERSION, &g_pPhysicsCollision },
	{ SOUNDEMITTERSYSTEM_INTERFACE_VERSION, &g_pSoundEmitterSystem },
	{ MESHSYSTEM_INTERFACE_VERSION, &g_pMeshSystem },
	{ RENDER_DEVICE_INTERFACE_VERSION, &g_pRenderDevice },
	{ RENDER_HARDWARECONFIG_INTERFACE_VERSION, &g_pRenderHardwareConfig },
	{ SCENESYSTEM_INTERFACE_VERSION, &g_pSceneSystem },
	{ WORLD_RENDERER_MGR_INTERFACE_VERSION, &g_pWorldRendererMgr },
	{ RENDER_SYSTEM_SURFACE_INTERFACE_VERSION, &g_pVGuiRenderSurface },
	{ MATCHFRAMEWORK_INTERFACE_VERSION, &g_pMatchFramework },
	{ GAMEUISYSTEMMGR_INTERFACE_VERSION, &g_pGameUISystemMgr }
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
		Error("APPSYSTEM: In ConnectInterfaces(), s_nRegistrationCount is %d!\n", s_nRegistrationCount);
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
