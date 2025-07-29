#include "tier1/utlstring.h"
#include "tier1/KeyValues.h"
#include "appframework/IAppSystem.h"
#include "tier2/tier2.h"

struct ResourceManifestDesc_t;

enum HostStateRequestType_t
{
	HSR_IDLE = 1,
	HSR_GAME,
	HSR_SOURCETV_RELAY,
	HSR_QUIT
};

enum HostStateRequestMode_t
{
	HM_LEVEL_LOAD_SERVER = 1,
	HM_CONNECT,
	HM_CHANGE_LEVEL,
	HM_LEVEL_LOAD_LISTEN,
	HM_LOAD_SAVE,
	HM_PLAY_DEMO,
	HM_SOURCETV_RELAY,
	HM_ADDON_DOWNLOAD
};

struct CHostStateRequest
{
	HostStateRequestType_t m_iType;
	CUtlString m_LoopModeType;
	CUtlString m_Desc;
	bool m_bActive;
	unsigned int m_ID;
	HostStateRequestMode_t m_iMode;
	CUtlString m_LevelName;
	bool m_bChangelevel;
	CUtlString m_SaveGame;
	CUtlString m_Address;
	CUtlString m_DemoFile;
	bool m_bLoadMap;
	CUtlString m_Addons;
	KeyValues *m_pKV;
};

class ISwitchLoopModeStatusNotify
{
	virtual void OnSwitchLoopModeFinished(const char *, uint32, bool) = 0;
};

class IHostStateMgr : public IAppSystem
{
	virtual void RequestHS_Quit(void) = 0;
	virtual void RequestHS_Idle(KeyValues *) = 0;
	virtual void RequestHS_Connect(const char *, KeyValues *) = 0;
	virtual void RequestHS_ChangelevelReconnect(const char *, KeyValues *) = 0;
	virtual void RequestHS_LoadSpawnGroup(const char *, const char *, bool, KeyValues *) = 0;
	virtual void RequestHS_LoadSaveGame(const char *, const char *, KeyValues *) = 0;
	virtual void RequestHS_PlayDemo(const char *, const char *, bool, KeyValues *) = 0;
	virtual void RequestHS_PlayBroadcast(const char *, KeyValues *) = 0;
	virtual void RequestHS_DownloadAddons(KeyValues *) = 0;
	virtual void RequestHS_SourceTVRelay(const char *, KeyValues *) = 0;
	virtual void RequestHS_ReloadLastSaveGame(void) = 0;
	virtual void RequestHS_RestartSpawnGroups(void) = 0;
};

class CHostStateMgr : public CTier2AppSystem<IHostStateMgr>, public ISwitchLoopModeStatusNotify
{
public:
	CHostStateRequest *m_PendingRequest;              // 0x30
	CHostStateRequest *m_CurrentRequest;              // 0x38
	CUtlString m_LoopModeType;                        // 0x40 console/game
	KeyValues *m_pGameConfigurationKV;                // 0x48
	CUtlString m_LoopMode;                            // 0x50
	KeyValues *m_pConnectKV;                          // 0x58
	CUtlString m_LoopModeName;                        // 0x60
	CUtlString m_Address;                             // 0x68
	CUtlString m_SaveGame;                            // 0x70
	CUtlString m_LevelName;                           // 0x78
	CUtlString m_Addons;                              // 0x80
	KeyValues *m_pKV;                                 // 0x88
	int m_ID;                                         // 0x90
	CUtlVector<CHostStateRequest *> m_QueuedRequests; // 0x98
};
