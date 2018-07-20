/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_GAMECLIENT_H
#define GAME_CLIENT_GAMECLIENT_H

#include <base/vmath.h>
#include <base/color.h> // this doesn't really belong here; it's for all the components
#include <engine/client.h>
#include <engine/console.h>
#include <engine/serverbrowser.h>
#include <game/layers.h>
#include <game/gamecore.h>
#include "render.h"

#include <game/teamscore.h>

class CGameClient;

class CWeaponData
{
public:
	int m_Tick;
	vec2 m_Pos;
	vec2 m_Direction;
	vec2 StartPos() { return m_Pos + m_Direction * 28.0f * 0.75f; }
};

class CLocalProjectile
{
public:
	int m_Active;
	CGameClient *m_pGameClient;
	CWorldCore *m_pWorld;
	CCollision *m_pCollision;

	vec2 m_Direction;
	vec2 m_Pos;
	int m_StartTick;
	int m_Type;

	int m_Owner;
	int m_Weapon;
	bool m_Explosive;
	int m_Bouncing;
	bool m_Freeze;
	bool m_ExtraInfo;

	vec2 GetPos(float Time);
	void CreateExplosion(vec2 Pos, int LocalClientID);
	void Tick(int CurrentTick, int GameTickSpeed, int LocalClientID);
	void Init(CGameClient *pGameClient, CWorldCore *pWorld, CCollision *pCollision, const CNetObj_Projectile *pProj);
	void Init(CGameClient *pGameClient, CWorldCore *pWorld, CCollision *pCollision, vec2 Vel, vec2 Pos, int StartTick, int Type, int Owner, int Weapon, bool Explosive, int Bouncing, bool Freeze, bool ExtraInfo);
	bool GameLayerClipped(vec2 CheckPos);
	void Deactivate() { m_Active = 0; }
};

class CGameClient : public IGameClient
{
	friend class CLuaFile;
	class CStack
	{
	public:
		enum
		{
			MAX_COMPONENTS = 128,
		};

		CStack();
		void Add(class CComponent *pComponent);

		class CComponent *m_paComponents[MAX_COMPONENTS];
		int m_Num;
	};

	CStack m_All;
	CStack m_Input;
	CNetObjHandler m_NetObjHandler;

	struct IRCMessage
	{
		std::string m_From;
		std::string m_User;
		std::string m_Text;

		IRCMessage() { }

		IRCMessage(const std::string& From, const std::string& User, const std::string& Text) :
				m_From(From), m_User(User), m_Text(Text)
		{
		}
	};

	std::vector<IRCMessage> m_IRCMessageEventQueue;
	std::mutex m_IRCMessageEventQueueMutex;


protected:
	class IEngine *m_pEngine;
	class IInput *m_pInput;
	class IGraphics *m_pGraphics;
	class ITextRender *m_pTextRender;
	class IClient *m_pClient;
	class ISound *m_pSound;
	class IConsole *m_pConsole;
	class IStorageTW *m_pStorage;
	class IDemoPlayer *m_pDemoPlayer;
	class IServerBrowser *m_pServerBrowser;
	class IEditor *m_pEditor;
	class IFriends *m_pFriends;
	class IFriends *m_pFoes;
	class IUpdater *m_pUpdater;
	class ICurlWrapper *m_pCurlWrapper;

	CLayers m_Layers;
	class CCollision m_Collision;
	CUI m_UI;
//<<<! HEAD
//private:
//	void DispatchInput();
//=======
//>>>>>>> ddnet/master

	void ProcessEvents();
	void UpdatePositions();

	int m_PredictedTick;
	int m_LastNewPredictedTick[2];

	int m_LastRoundStartTick;

	int m_LastFlagCarrierRed;
	int m_LastFlagCarrierBlue;

	int m_CheckInfo[2];

	static void ConTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConKill(IConsole::IResult *pResult, void *pUserData);
	static void ConKillDummy(IConsole::IResult *pResult, void *pUserData);
	static void ConLuafile(IConsole::IResult *pResult, void *pUserData);

	static void ConchainSpecialInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainSpecialDummyInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainSpecialDummy(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainIRCNickUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);


public:
	IKernel *Kernel() { return IInterface::Kernel(); }
	IEngine *Engine() const { return m_pEngine; }
	class IGraphics *Graphics() const { return m_pGraphics; }
	class IClient *Client() const { return m_pClient; }
	class CUI *UI() { return &m_UI; }
	class ISound *Sound() const { return m_pSound; }
	class IInput *Input() const { return m_pInput; }
	class IStorageTW *Storage() const { return m_pStorage; }
	class IConsole *Console() { return m_pConsole; }
	class ITextRender *TextRender() const { return m_pTextRender; }
	class IDemoPlayer *DemoPlayer() const { return m_pDemoPlayer; }
	class IDemoRecorder *DemoRecorder(int Recorder) const { return Client()->DemoRecorder(Recorder); }
	class IServerBrowser *ServerBrowser() const { return m_pServerBrowser; }
	class CRenderTools *RenderTools() { return &m_RenderTools; }
	class CLayers *Layers() { return &m_Layers; };
	class CCollision *Collision() { return &m_Collision; };
	class IEditor *Editor() { return m_pEditor; }
	class IFriends *Friends() { return m_pFriends; }
	class IFriends *Foes() { return m_pFoes; }
	class IUpdater *Updater() { return m_pUpdater; }
	class IIRC *IRC() const { return m_pIRC; }

	int NetobjNumCorrections() { return m_NetObjHandler.NumObjCorrections(); }
	const char *NetobjCorrectedOn() { return m_NetObjHandler.CorrectedObjOn(); }

	bool m_SuppressEvents;
	bool m_NewTick;
	bool m_NewPredictedTick;
	int m_FlagDropTick[2];

	// TODO: move this
	CTuningParams m_Tuning[2];

	enum
	{
		SERVERMODE_PURE=0,
		SERVERMODE_MOD,
		SERVERMODE_PUREMOD,
	};
	int m_ServerMode;

	int m_DemoSpecID;

	vec2 m_LocalCharacterPos;

	// predicted players
	CCharacterCore m_PredictedPrevChar;
	CCharacterCore m_PredictedChar;

	// snap pointers
	struct CSnapState
	{
		const CNetObj_Character *m_pLocalCharacter;
		const CNetObj_Character *m_pLocalPrevCharacter;
		const CNetObj_PlayerInfo *m_pLocalInfo;
		const CNetObj_SpectatorInfo *m_pSpectatorInfo;
		const CNetObj_SpectatorInfo *m_pPrevSpectatorInfo;
		const CNetObj_Flag *m_paFlags[2];
		const CNetObj_GameInfo *m_pGameInfoObj;
		const CNetObj_GameData *m_pGameDataObj;
		int m_GameDataSnapID;

		const CNetObj_PlayerInfo *m_paPlayerInfos[MAX_CLIENTS];
		const CNetObj_PlayerInfo *m_paInfoByScore[MAX_CLIENTS];
		const CNetObj_PlayerInfo *m_paInfoByName[MAX_CLIENTS];
		//const CNetObj_PlayerInfo *m_paInfoByTeam[MAX_CLIENTS];
		const CNetObj_PlayerInfo *m_paInfoByDDTeam[MAX_CLIENTS];

		const CNetObj_PlayerInfo * LuaGetPlayerInfos(int ID) const { return m_paPlayerInfos[ID]; }
		const CNetObj_PlayerInfo * LuaGetInfoByScore(int ID) const { return m_paInfoByScore[ID]; }
		const CNetObj_PlayerInfo * LuaGetInfoByName(int ID) const { return m_paInfoByName[ID]; }
		const CNetObj_PlayerInfo * LuaGetInfoByDDTeam(int ID) const { return m_paInfoByDDTeam[ID]; }

		int m_LocalClientID;
		int m_NumPlayers;
		int m_aTeamSize[2];

		// spectate data
		struct CSpectateInfo
		{
			bool m_Active;
			int m_SpectatorID;
			bool m_UsePosition;
			vec2 m_Position;
		} m_SpecInfo;

		//
		struct CCharacterInfo
		{
			bool m_Active;

			// snapshots
			CNetObj_Character m_Prev;
			CNetObj_Character m_Cur;

			// interpolated position
			vec2 m_Position;
		};

		CCharacterInfo m_aCharacters[MAX_CLIENTS];
	};

	CSnapState m_Snap;

	// client data
	struct CClientData
	{
		int m_UseCustomColor;
		int m_ColorBody;
		int m_ColorFeet;

		char m_aName[MAX_NAME_LENGTH];
		char m_aClan[MAX_CLAN_LENGTH];
		int m_Country;
		char m_aSkinName[64];
		int m_SkinID;
		int m_SkinColor;
		int m_Team;
		int m_Emoticon;
		int m_EmoticonStart;
		CCharacterCore m_Predicted;
		CCharacterCore m_PrevPredicted;

		CTeeRenderInfo m_SkinInfo; // this is what the server reports
		CTeeRenderInfo m_RenderInfo; // this is what we use

		float m_Angle;
		bool m_Active;
		bool m_ChatIgnore;
		bool m_Friend;
		bool m_Foe;
		bool m_ATH;

		void UpdateRenderInfo();
		void Reset();

		// DDRace
		int m_Score;

		// haxx
		bool m_Spoofable;
		char m_Addr[NETADDR_MAXSTRSIZE];
		
		//lua
		std::string GetName() const { return std::string(m_aName); }
		std::string GetClan() const { return std::string(m_aClan); }
		std::string GetSkinName() const { return std::string(m_aSkinName); }
		const CTeeRenderInfo& GetTeeRenderInfo() const { return m_RenderInfo; }
	};

	CClientData m_aClients[MAX_CLIENTS];

	class CClientStats
	{
		int m_IngameTicks;
		int m_JoinTick;
		bool m_Active;
		
	public:
		CClientStats();

		int m_aFragsWith[NUM_WEAPONS];
		int m_aDeathsFrom[NUM_WEAPONS];
		int m_Frags;
		int m_Deaths;
		int m_Suicides;
		int m_BestSpree;
		int m_CurrentSpree;

		int m_FlagGrabs;
		int m_FlagCaptures;

		void Reset();
		
		bool IsActive() const { return m_Active; }
		void JoinGame(int Tick) { m_Active = true; m_JoinTick = Tick; };
		void JoinSpec(int Tick) { m_Active = false; m_IngameTicks += Tick - m_JoinTick; };
		int GetIngameTicks(int Tick) const { return m_IngameTicks + Tick - m_JoinTick; };
		float GetFPM(int Tick, int TickSpeed) const { return (float)(m_Frags * TickSpeed * 60) / GetIngameTicks(Tick); };
	};

	CClientStats m_aStats[MAX_CLIENTS];

	CRenderTools m_RenderTools;

	void OnReset();

	// hooks
	virtual void OnConnected();
	virtual void OnRender();
	virtual void OnUpdate();
	virtual void OnDummyDisconnect();
	virtual void OnRelease();
	virtual void OnInit();
	virtual void OnConsoleInit();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnMessage(int MsgId, CUnpacker *pUnpacker, bool IsDummy = 0);
	virtual void OnMessageIRC(const std::string& From, const std::string& User, const std::string& Text);
	virtual void OnNewSnapshot();
	virtual void OnPredict();
	virtual void OnActivateEditor();
	virtual int OnSnapInput(int *pData);
	virtual void OnShutdown();
	virtual void OnEnterGame();
	virtual void OnRconLine(const char *pLine);
	virtual void OnGameOver();
	virtual void OnStartGame();
	virtual void OnFlagGrab(int TeamID);

	virtual void OnLuaScriptLoaded(class CLuaFile *pLF);
	virtual void OnLuaScriptUnload(class CLuaFile *pLF);

	virtual void ResetDummyInput();
	virtual const char *GetItemName(int Type);
	virtual const char *Version();
	virtual const char *NetVersion();

	virtual const CNetObj_PlayerInput &getPlayerInput(int dummy);


	// actions
	// TODO: move these
	void SendSwitchTeam(int Team);
	void SendInfo(bool Start);
	virtual void SendDummyInfo(bool Start);
	void SendKill();
	void SendKillDummy();

	// pointers to all systems
	class CGameConsole *m_pGameConsole;
	class CBinds *m_pBinds;
	class CParticles *m_pParticles;
	class CMenus *m_pMenus;
	class CMenusTooltip *m_pTooltip;
	class CHud *m_pHud;
	class CSkins *m_pSkins;
	class CSkinDownload *m_pSkinDownload;
	class CCountryFlags *m_pCountryFlags;
	class CFlow *m_pFlow;
	class CChat *m_pChat;
	class CDamageInd *m_pDamageind;
	class CCamera *m_pCamera;
	class CControls *m_pControls;
	class CEffects *m_pEffects;
	class CEmoticon *m_pEmoticon;
	class CSounds *m_pSounds;
	class CSpoofRemote *m_pSpoofRemote;
	class CMotd *m_pMotd;
	class CMapImages *m_pMapimages;
	class CVoting *m_pVoting;
	class CScoreboard *m_pScoreboard;
	class CStatboard *m_pStatboard;
	class CItems *m_pItems;
	class CMapLayers *m_pMapLayersBackGround;
	class CMapLayers *m_pMapLayersForeGround;
	class CNamePlates *m_pNamePlates;
	class CBackground *m_pBackGround;

	class CMapSounds *m_pMapSounds;

	class CAStar *m_pAStar;
	class CIRCBind *m_pIRCBind;
	class CIdentity *m_pIdentity;
	class CCollision *m_pCollision; // for lua
	class CGameTextureManager *m_pGameTextureManager;
	class CFontMgr *m_pFontMgrBasic;
	class CFontMgr *m_pFontMgrMono;
	class CUI *m_pUi;

	// fck this hack for lua
	class IIRC *m_pIRC;

	// DDRace

	class CRaceDemo *m_pRaceDemo;
	class CGhost *m_pGhost;
	class CTeamsCore m_Teams;

	int IntersectCharacterLua(const vec2& Pos0, const vec2& Pos1, vec2 *pOutNewPos, int OwnID) { return IntersectCharacter(Pos0, Pos1, pOutNewPos, OwnID); }
	int IntersectCharacter(const vec2& Pos0, const vec2& Pos1, vec2 *pOutNewPos, int OwnID);
	int IntersectCharacter(const vec2& OldPos, const vec2& NewPos, float Radius, vec2* NewPos2, int OwnID, CWorldCore *pWorld);

	CWeaponData m_aWeaponData[150];
	CWeaponData *GetWeaponData(int Tick) { return &m_aWeaponData[((Tick%150)+150)%150]; }
	CWeaponData *FindWeaponData(int TargetTick);

	void FindWeaker(bool IsWeaker[2][MAX_CLIENTS]);

	bool AntiPingPlayers() { return g_Config.m_ClAntiPing && g_Config.m_ClAntiPingPlayers && !m_Snap.m_SpecInfo.m_Active && Client()->State() != IClient::STATE_DEMOPLAYBACK && (m_Tuning[g_Config.m_ClDummy].m_PlayerCollision || m_Tuning[g_Config.m_ClDummy].m_PlayerHooking); }
	bool AntiPingGrenade() { return g_Config.m_ClAntiPing && g_Config.m_ClAntiPingGrenade && !m_Snap.m_SpecInfo.m_Active && Client()->State() != IClient::STATE_DEMOPLAYBACK; }
	bool AntiPingWeapons() { return g_Config.m_ClAntiPing && g_Config.m_ClAntiPingWeapons && !m_Snap.m_SpecInfo.m_Active && Client()->State() != IClient::STATE_DEMOPLAYBACK; }

	CServerInfo m_CurrentServerInfo;

	static CClientData * LuaGetClientData(int ID) { return &CLua::m_pCGameClient->m_aClients[ID]; }
	static CSnapState::CCharacterInfo * LuaGetCharacterInfo(int ID) { return &CLua::m_pCGameClient->m_Snap.m_aCharacters[ID]; }
	static const CSnapState& LuaGetFullSnap() { return CLua::m_pCGameClient->m_Snap; }
	static CTuningParams * LuaGetTuning() { return &CLua::m_pCGameClient->m_Tuning[g_Config.m_ClDummy]; }
	
private:
	bool m_DDRaceMsgSent[2];
	int m_ShowOthers[2];

	bool m_ResetConfig;
	
	std::string m_HiddenMessages[MAX_CLIENTS];
	unsigned int m_aHiddenMsgSpamScores[MAX_CLIENTS];
	int64 m_LastHiddenCountsUpdate;
};


extern const char *Localize(const char *Str);


#endif
