/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_H
#define ENGINE_CLIENT_H
#include "kernel.h"

#include "message.h"
#include <engine/friends.h>
#include <engine/shared/config.h>
#include <versionsrv/versionsrv.h>
#include <game/generated/protocol.h>
#include <engine/client/lua.h>
#include "client/debug.h"
#include "curlwrapper.h"


enum
{
	RECORDER_MANUAL=0,
	RECORDER_AUTO=1,
	RECORDER_RACE=2,
	RECORDER_MAX=3,
};

typedef bool (*CLIENTFUNC_FILTER)(const void *pData, int DataSize, void *pUser);

extern CCallstack gDebugInfo;
class IClient : public IInterface
{
	MACRO_INTERFACE("client", 0)
protected:
	// quick access to state of the client
	int m_State;

	// quick access to time variables
	int m_PrevGameTick[2];
	int m_CurGameTick[2];
	float m_GameIntraTick[2];
	float m_GameTickTime[2];

	int m_CurMenuTick;
	int64 m_MenuStartTime;

	int m_PredTick[2];
	float m_PredIntraTick[2];

	float m_LocalTime;
	float m_SteadyTimer;
	float m_RenderFrameTime;

	int m_GameTickSpeed;

	CLua m_Lua;

public:
	bool m_Restarting; // set this to make the client restart on the next quit

	int m_LocalIDs[2];
	//char m_aNews[NEWS_SIZE]; // ATH NEWS ARE IN CUpdater!!
	char m_aNewsDDNet[NEWS_SIZE];
	int64 m_ReconnectTime;

	CNetObj_PlayerInput m_DummyInput;

	bool m_DummySendConnInfo;

	class CSnapItem
	{
	public:
		int m_Type;
		int m_ID;
		int m_DataSize;
	};

	/* Constants: Client States
		STATE_OFFLINE - The client is offline.
		STATE_CONNECTING - The client is trying to connect to a server.
		STATE_LOADING - The client has connected to a server and is loading resources.
		STATE_ONLINE - The client is connected to a server and running the game.
		STATE_DEMOPLAYBACK - The client is playing a demo
		STATE_QUITING - The client is quiting.
	*/

	enum
	{
		STATE_OFFLINE=0,
		STATE_CONNECTING,
		STATE_LOADING,
		STATE_ONLINE,
		STATE_DEMOPLAYBACK,
		STATE_QUITING,
	};

	//
	inline int State() const { return m_State; }
	inline bool IsOnline() const { return m_State == STATE_ONLINE; }
	inline bool IsIngame() const { return m_State == STATE_ONLINE || m_State == STATE_DEMOPLAYBACK; }

	// tick time access
	inline int PrevGameTick() const { return m_PrevGameTick[g_Config.m_ClDummy]; }
	inline int GameTick() const { return m_CurGameTick[g_Config.m_ClDummy]; }
	inline int MenuTick() const { return m_CurMenuTick; }
	inline int PredGameTick() const { return m_PredTick[g_Config.m_ClDummy]; }
	inline float IntraGameTick() const { return m_GameIntraTick[g_Config.m_ClDummy]; }
	inline float PredIntraGameTick() const { return m_PredIntraTick[g_Config.m_ClDummy]; }
	inline float GameTickTime() const { return m_GameTickTime[g_Config.m_ClDummy]; }
	inline int GameTickSpeed() const { return m_GameTickSpeed; }

	// other time access
	inline int   GetFPS() const { return (int)(1.f/m_RenderFrameTime); }
	inline float RenderFrameTime() const { return m_RenderFrameTime; }
	inline float LocalTime() const { return m_LocalTime; }
	inline float SteadyTimer() const { return m_SteadyTimer; }

	// actions
	virtual void Connect(const char *pAddress) = 0;
	virtual void Disconnect() = 0;

	// dummy
	virtual void DummyDisconnect(const char *pReason) = 0;
	virtual void DummyConnect() = 0;
	virtual bool DummyConnected() = 0;
	virtual bool DummyConnecting() = 0;

	virtual void Restart() = 0;
	virtual void Quit() = 0;
	virtual const char *DemoPlayer_Play(const char *pFilename, int StorageType) = 0;
	virtual void DemoRecorder_Start(const char *pFilename, bool WithTimestamp, int Recorder) = 0;
	virtual void DemoRecorder_HandleAutoStart() = 0;
	virtual void DemoRecorder_Stop(int Recorder) = 0;
	virtual void DemoRecorder_AddDemoMarker(int Recorder) = 0;
	virtual class IDemoRecorder *DemoRecorder(int Recorder) = 0;
	virtual void AutoScreenshot_Start() = 0;
	virtual void AutoStatScreenshot_Start() = 0;
	virtual void AutoCSV_Start() = 0;
	virtual void ServerBrowserUpdate() = 0;

	// gfx
	virtual void GfxUpdateWindowMode() = 0;
	virtual void GfxUpdateVSync() = 0;
	virtual void SwitchWindowScreen(int Index) = 0;
	virtual void LuaCheckDrawingState(lua_State *L, const char *pFuncName, bool NoThrow=false) = 0;

	// networking
	virtual void EnterGame() = 0;

	virtual bool LoadBackgroundMap() = 0;
	virtual bool MapLoaded() = 0;

	//
	virtual const char *MapDownloadName() = 0;
	virtual const char *MapDownloadSource() = 0;
	virtual int MapDownloadSourceID() = 0;
	virtual int MapDownloadAmount() = 0;
	virtual int MapDownloadTotalsize() = 0;
	virtual int NumMapDBServers() = 0;

	// input
	virtual int *GetInput(int Tick) = 0;
	virtual bool InputExists(int Tick) = 0;

	virtual void SendPlayerInfo(bool Start) = 0;

	// remote console
	virtual void RconAuth(const char *pUsername, const char *pPassword) = 0;
	virtual bool RconAuthed() = 0;
	virtual bool UseTempRconCommands() = 0;
	virtual void Rcon(const char *pLine) = 0;

	// server info
	virtual const class CServerInfo *GetServerInfo(class CServerInfo *pServerInfo = 0) const = 0;

	virtual void CheckVersionUpdate() = 0;

	virtual int GetPredictionTime() = 0;

	// snapshot interface

	enum
	{
		SNAP_CURRENT=0,
		SNAP_PREV=1
	};

	// TODO: Refactor: should redo this a bit i think, too many virtual calls
	virtual int SnapNumItems(int SnapID) = 0;
	virtual void *SnapFindItem(int SnapID, int Type, int ID) = 0;
	virtual void *SnapGetItem(int SnapID, int Index, CSnapItem *pItem) = 0;
	virtual void SnapInvalidateItem(int SnapID, int Index) = 0;

	virtual void SnapSetStaticsize(int ItemType, int Size) = 0;

	virtual int SendMsg(CMsgPacker *pMsg, int Flags) = 0;
	virtual int SendMsgExY(CMsgPacker *pMsg, int Flags, bool System=true, int NetClient=1) = 0;

	template<class T>
	int SendPackMsg(T *pMsg, int Flags)
	{
		CMsgPacker Packer(pMsg->MsgID());
		if(pMsg->Pack(&Packer))
			return -1;
		return SendMsg(&Packer, Flags);
	}

	//
	virtual const char *ErrorString() = 0;
	virtual const char *News() = 0;
	virtual const char *LatestVersion() = 0;
	virtual bool ConnectionProblems() = 0;

	virtual bool SoundInitFailed() = 0;

	virtual int GetDebugFont() = 0;

	//DDRace

	virtual const char* GetCurrentMap() = 0;
	virtual int GetCurrentMapCrc() = 0;
	virtual const char* GetCurrentServerAddress() const = 0;
	virtual const char* GetCurrentMapPath() = 0;
	virtual const char* RaceRecordStart(const char *pFilename) = 0;
	virtual void RaceRecordStop() = 0;
	virtual bool RaceRecordIsRecording() = 0;

	virtual void DemoSliceBegin() = 0;
	virtual void DemoSliceEnd() = 0;
	virtual void DemoSlice(const char *pDstPath, CLIENTFUNC_FILTER pfnFilter, void *pUser) = 0;

	virtual bool EditorHasUnsavedData() = 0;

	virtual void GenerateTimeoutSeed() = 0;

	virtual IFriends* Foes() = 0;

	CLua *Lua() { return &m_Lua; }
};

class IGameClient : public IInterface
{
	MACRO_INTERFACE("gameclient", 0)
protected:
public:
	virtual void OnConsoleInit() = 0;

	virtual void OnRconLine(const char *pLine) = 0;
	virtual void OnInit() = 0;
	virtual void OnNewSnapshot() = 0;
	virtual void OnEnterGame() = 0;
	virtual void OnShutdown() = 0;
	virtual void OnRender() = 0;
	virtual void OnUpdate() = 0;
	virtual void OnStateChange(int NewState, int OldState) = 0;
	virtual void OnConnected() = 0;
	virtual void OnMessage(int MsgID, CUnpacker *pUnpacker, bool IsDummy = 0) = 0;
	virtual void OnMessageIRC(const std::string& From, const std::string& User, const std::string& Text) = 0;
	virtual void OnPredict() = 0;
	virtual void OnActivateEditor() = 0;

	virtual void OnLuaScriptLoaded(class CLuaFile *pLF) = 0;
	virtual void OnLuaScriptUnload(class CLuaFile *pLF) = 0;


	virtual int OnSnapInput(int *pData) = 0;
	virtual void SendDummyInfo(bool Start) = 0;
	virtual void ResetDummyInput() = 0;
	virtual const CNetObj_PlayerInput &getPlayerInput(int dummy) = 0;

	virtual const char *GetItemName(int Type) = 0;
	virtual const char *Version() = 0;
	virtual const char *NetVersion() = 0;

	virtual void OnDummyDisconnect() = 0;
};

extern IGameClient *CreateGameClient();
#endif
