/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_CLIENT_H
#define ENGINE_CLIENT_CLIENT_H

#include "db_sqlite3.h"
#include "curlwrapper.h"

class CGraph
{
public:
	enum
	{
		// restrictions: Must be power of two
		MAX_VALUES=128,
	};

	float m_Min, m_Max;
	float m_aValues[MAX_VALUES];
	float m_aColors[MAX_VALUES][3];
	int m_Index;

	void Init(float Min, float Max);

	void ScaleMax();
	void ScaleMin();

	void Add(float v, float r, float g, float b);
	void Render(IGraphics *pGraphics, int Font, float x, float y, float w, float h, const char *pDescription);
};


class CSmoothTime
{
	int64 m_Snap;
	int64 m_Current;
	int64 m_Target;

	CGraph m_Graph;

	int m_SpikeCounter;

	float m_aAdjustSpeed[2]; // 0 = down, 1 = up
public:
	void Init(int64 Target);
	void SetAdjustSpeed(int Direction, float Value);

	int64 Get(int64 Now);

	void UpdateInt(int64 Target);
	void Update(CGraph *pGraph, int64 Target, int TimeLeft, int AdjustDirection);
};


class CClient : public IClient, public CDemoPlayer::IListener
{
	MACRO_ALLOC_HEAP()

	// needed interfaces
	IEngine *m_pEngine;
	IEditor *m_pEditor;
	IEngineInput *m_pInput;
	IEngineGraphics *m_pGraphics;
	IEngineSound *m_pSound;
	IGameClient *m_pGameClient;
	IEngineMap *m_pMap;
	IConsole *m_pConsole;
	IStorageTW *m_pStorage;
	IFetcher *m_pFetcher;
	ICurlWrapper *m_pCurlWrapper;
	IUpdater *m_pUpdater;
	IEngineMasterServer *m_pMasterServer;
	IIRC *m_pIRC;

	enum
	{
		NUM_SNAPSHOT_TYPES=2,
		PREDICTION_MARGIN=1000/50/2, // magic network prediction value
	};

	class CNetClient m_NetClient[3];
	class CDemoPlayer m_DemoPlayer;
	class CDemoRecorder m_DemoRecorder[RECORDER_MAX];
	class CDemoEditor m_DemoEditor;
	class CServerBrowser m_ServerBrowser;
	class CFetcher m_Fetcher;
	class CCurlWrapper m_CurlWrapper;
	class CUpdater m_Updater;
	class CFriends m_Friends;
	class CFriends m_Foes;
	class CMapChecker m_MapChecker;
	class CIRC m_IRC;

	char m_aServerAddressStr[256];

	unsigned m_SnapshotParts;
	int64 m_LocalStartTime;
	int64 m_TimerStartTime;

	int m_DebugFont;

	int64 m_LastRenderTime;
	float m_RenderFrameTimeLow;
	float m_RenderFrameTimeHigh;
	int m_RenderFrames;

	NETADDR m_ServerAddress;
	int m_SnapCrcErrors;
	bool m_AutoScreenshotRecycle;
	bool m_AutoStatScreenshotRecycle;
	bool m_AutoCSVRecycle;
	bool m_EditorActive;
	bool m_SoundInitFailed;
	bool m_ResortServerBrowser;

	int m_AckGameTick[2];
	int m_CurrentRecvTick[2];
	int m_RconAuthed[2];
	char m_RconPassword[32];
	int m_UseTempRconCommands;

	// version-checking
	//char m_aVersionStr[10];

	// pinging
	int64 m_PingStartTime;

	char m_aCurrentMap[256];
	char m_aCurrentMapPath[CEditor::MAX_PATH_LENGTH];

	char m_aTimeoutCodes[2][32];
	bool m_aTimeoutCodeSent[2];
	bool m_GenerateTimeoutSeed;

	//
	char m_aCmdConnect[256];

	// map download
	void LoadMapDatabaseUrls();
	struct MapDbUrl
	{
		int prior;
		std::string url;
		bool operator<(const MapDbUrl& other) { return this->prior < other.prior; }
	};
	sorted_array<MapDbUrl> m_MapDbUrls;
	CFetchTask *m_pMapdownloadTask;
	int m_CurrentMapServer;

	char m_aMapdownloadFilename[256];
	char m_aMapdownloadName[256];
	const char *m_pMapdownloadSource;
	IOHANDLE m_MapdownloadFile;
	int m_MapdownloadChunk;
	int m_MapdownloadCrc;
	int m_MapdownloadAmount;
	int m_MapdownloadTotalsize;

	// time
	CSmoothTime m_GameTime[2];
	CSmoothTime m_PredictedTime;

	// input
	struct // TODO: handle input better
	{
		int m_aData[MAX_INPUT_SIZE]; // the input data
		int m_Tick; // the tick that the input is for
		int64 m_PredictedTime; // prediction latency when we sent this input
		int64 m_Time;
	} m_aInputs[2][200];

	int m_CurrentInput[2];
	bool m_LastDummy;
	bool m_LastDummy2;
	CNetObj_PlayerInput HammerInput;

	// graphs
	CGraph m_InputtimeMarginGraph;
	CGraph m_GametimeMarginGraph;
	CGraph m_FpsGraph;

	// the game snapshots are modifiable by the game
	class CSnapshotStorage m_SnapshotStorage[2];
	CSnapshotStorage::CHolder *m_aSnapshots[2][NUM_SNAPSHOT_TYPES];

	int m_ReceivedSnapshots[2];
	char m_aSnapshotIncomingData[CSnapshot::MAX_SIZE];

	class CSnapshotStorage::CHolder m_aDemorecSnapshotHolders[NUM_SNAPSHOT_TYPES];
	char *m_aDemorecSnapshotData[NUM_SNAPSHOT_TYPES][2][CSnapshot::MAX_SIZE];

	class CSnapshotDelta m_SnapshotDelta;

	//
	class CServerInfo m_CurrentServerInfo;
	int64 m_CurrentServerInfoRequestTime; // >= 0 has requested
	bool m_GotServerInfo;

	int64 TickStartTime(int Tick);

	void ConnectImpl(); // hide this

	char m_aDDNetSrvListToken[4];
	bool m_DDNetSrvListTokenSet;

	// - SQL
	CSql *m_pDatabase;

#if defined(CONF_FAMILY_UNIX)
	CFifo m_Fifo;
#endif

public:
	IClient *Client() { return this; }
	IEngine *Engine() { return m_pEngine; }
	IEngineGraphics *Graphics() { return m_pGraphics; }
	IEngineInput *Input() { return m_pInput; }
	IEngineSound *Sound() { return m_pSound; }
	IGameClient *GameClient() { return m_pGameClient; }
	IEngineMasterServer *MasterServer() { return m_pMasterServer; }
	IStorageTW *Storage() { return m_pStorage; }
	IFetcher *Fetcher() { return m_pFetcher; }
	IUpdater *Updater() { return m_pUpdater; }
	IIRC *IRC() { return m_pIRC; }

	CClient();
	~CClient();

	// ----- send functions -----
	virtual int SendMsg(CMsgPacker *pMsg, int Flags);
	virtual int SendMsgExY(CMsgPacker *pMsg, int Flags, bool System=true, int NetClient=1);

	int SendMsgEx(CMsgPacker *pMsg, int Flags, bool System=true);
	void SendInfo();
	void SendEnterGame();
	void SendReady();
	void SendMapRequest();

	virtual void SendPlayerInfo(bool Start);

	virtual bool RconAuthed() { return m_RconAuthed[g_Config.m_ClDummy] != 0; }
	virtual bool UseTempRconCommands() { return m_UseTempRconCommands != 0; }
	void RconAuth(const char *pName, const char *pPassword);
	virtual void Rcon(const char *pCmd);

	virtual bool ConnectionProblems();

	virtual bool SoundInitFailed() { return m_SoundInitFailed; }

	virtual int GetDebugFont() { return m_DebugFont; }

	void DirectInput(int *pInput, int Size);
	void SendInput();

	// TODO: OPT: do this alot smarter!
	virtual int *GetInput(int Tick);
	virtual bool InputExists(int Tick);

	const char *News() { return m_Updater.GetNews(); }
	const char *LatestVersion();
	void CheckVersionUpdate();

	// ------ state handling -----
	void SetState(int NewState);

	// called when the map is loaded and we should init for a new round
	void OnEnterGame();
	virtual void EnterGame();

	virtual void Connect(const char *pAddress); // synchron
	void DisconnectWithReason(const char *pReason);
	virtual void Disconnect();
	virtual void TimeMeOut();

	virtual void DummyDisconnect(const char *pReason);
	virtual void DummyConnect();
	virtual bool DummyConnected();
	virtual bool DummyConnecting();
	void DummyInfo();
	int m_DummyConnected;
	int m_LastDummyConnectTime;
	int m_Fire;

	const CServerInfo *GetServerInfo(CServerInfo *pServerInfo = 0) const;
	void ServerInfoRequest();

	int LoadData();

	// ---

	int GetPredictionTime();
	void *SnapGetItem(int SnapID, int Index, CSnapItem *pItem);
	void SnapInvalidateItem(int SnapID, int Index);
	void *SnapFindItem(int SnapID, int Type, int ID);
	int SnapNumItems(int SnapID);
	void SnapSetStaticsize(int ItemType, int Size);

	void Render();
	void DebugRender();

	virtual void Restart();
	virtual void Quit();

	virtual const char *ErrorString();

	bool MapLoaded();
	bool LoadBackgroundMap();

	const char *LoadMap(const char *pName, const char *pFilename, unsigned WantedCrc);
	const char *LoadMapSearch(const char *pMapName, int WantedCrc);
	bool UnloadCurrentMap();

	static int PlayerScoreNameComp(const void *a, const void *b);

	void ProcessConnlessPacket(CNetChunk *pPacket);
	void ProcessServerInfo(int Type, NETADDR *pFrom, const void *pData, int DataSize);
	void ProcessServerPacket(CNetChunk *pPacket);
	void ProcessServerPacketDummy(CNetChunk *pPacket);

	void MapFetcherStart(const char *pMap, int MapCrc);
	void ResetMapDownload();
	void FinishMapDownload();

	virtual const char *MapDownloadName() { return m_aMapdownloadName; }
	virtual const char *MapDownloadSource() { return m_pMapdownloadSource; }
	virtual int MapDownloadSourceID() { return m_CurrentMapServer; }
	virtual int MapDownloadAmount() { return !m_pMapdownloadTask ? m_MapdownloadAmount : (int)m_pMapdownloadTask->Current(); }
	virtual int MapDownloadTotalsize() { return !m_pMapdownloadTask ? m_MapdownloadTotalsize : (int)m_pMapdownloadTask->Size(); }
	virtual int NumMapDBServers() { return m_MapDbUrls.size(); }

	void PumpNetwork();

	virtual void OnDemoPlayerSnapshot(void *pData, int Size);
	virtual void OnDemoPlayerMessage(void *pData, int Size);

	void Update();

	void RegisterInterfaces();
	void InitInterfaces();

	void Run();
private:
	void RunMainloop();
	void RunTeardown();
public:

	bool CtrlShiftKey(int Key, bool &Last);

	static void Con_Connect(IConsole::IResult *pResult, void *pUserData);
	static void Con_Disconnect(IConsole::IResult *pResult, void *pUserData);
	static void Con_Timeout(IConsole::IResult *pResult, void *pUserData);

	static void Con_DummyConnect(IConsole::IResult *pResult, void *pUserData);
	static void Con_DummyDisconnect(IConsole::IResult *pResult, void *pUserData);

	static void Con_Quit(IConsole::IResult *pResult, void *pUserData);
	static void Con_DemoPlay(IConsole::IResult *pResult, void *pUserData);
	static void Con_DemoSpeed(IConsole::IResult *pResult, void *pUserData);
	static void Con_Minimize(IConsole::IResult *pResult, void *pUserData);
	static void Con_Restart(IConsole::IResult *pResult, void *pUserData);
	static void Con_Ping(IConsole::IResult *pResult, void *pUserData);
	static void Con_SaveConfig(IConsole::IResult *pResult, void *pUserData);
	static void Con_Screenshot(IConsole::IResult *pResult, void *pUserData);
	static void Con_Rcon(IConsole::IResult *pResult, void *pUserData);
	static void Con_RconAuth(IConsole::IResult *pResult, void *pUserData);
	static void Con_RconLogin(IConsole::IResult *pResult, void *pUserData);
	static void Con_AddFavorite(IConsole::IResult *pResult, void *pUserData);
	static void Con_RemoveFavorite(IConsole::IResult *pResult, void *pUserData);
	static void Con_Play(IConsole::IResult *pResult, void *pUserData);
	static void Con_Record(IConsole::IResult *pResult, void *pUserData);
	static void Con_StopRecord(IConsole::IResult *pResult, void *pUserData);
	static void Con_AddDemoMarker(IConsole::IResult *pResult, void *pUserData);
	static void ConchainServerBrowserUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainWindowMode(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainWindowScreen(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainWindowVSync(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainLuaEnable(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainTimeoutSeed(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	static void Con_Panic(IConsole::IResult *pResult, void *pUserData);

	static void Con_DemoSlice(IConsole::IResult *pResult, void *pUserData);
	static void Con_DemoSliceBegin(IConsole::IResult *pResult, void *pUserData);
	static void Con_DemoSliceEnd(IConsole::IResult *pResult, void *pUserData);

	void RegisterCommands();

	const char *DemoPlayer_Play(const char *pFilename, int StorageType);
	void DemoRecorder_Start(const char *pFilename, bool WithTimestamp, int Recorder);
	void DemoRecorder_HandleAutoStart();
	void DemoRecorder_Stop(int Recorder);
	void DemoRecorder_AddDemoMarker(int Recorder);
	class IDemoRecorder *DemoRecorder(int Recorder);

	void AutoScreenshot_Start();
	void AutoStatScreenshot_Start();
	void AutoScreenshot_Cleanup();
	void AutoStatScreenshot_Cleanup();

	void AutoCSV_Start();
	void AutoCSV_Cleanup();

	void ServerBrowserUpdate();

	// gfx
	void GfxUpdateWindowMode();
	void GfxUpdateVSync();
	void SwitchWindowScreen(int Index);
	void LuaCheckDrawingState(lua_State *L, const char *pFuncName, bool NoThrow=false);


	// DDRace

	void GenerateTimeoutSeed();
	void GenerateTimeoutCodes();

	virtual const char* GetCurrentMap();
	virtual int GetCurrentMapCrc();
	virtual const char* GetCurrentServerAddress() const { return m_aServerAddressStr; }
	virtual const char* GetCurrentMapPath();
	virtual const char* RaceRecordStart(const char *pFilename);
	virtual void RaceRecordStop();
	virtual bool RaceRecordIsRecording();

	virtual void DemoSliceBegin();
	virtual void DemoSliceEnd();
	virtual void DemoSlice(const char *pDstPath, CLIENTFUNC_FILTER pfnFilter, void *pUser);

	bool EditorHasUnsavedData() { return m_pEditor->HasUnsavedData(); }

	static void InputThread(void *pUser);
	void *m_pInputThread;

	virtual IFriends* Foes() { return &m_Foes; }
};
#endif
