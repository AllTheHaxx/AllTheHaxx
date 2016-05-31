/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/editor.h>
#include <engine/engine.h>
#include <engine/friends.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/demo.h>
#include <engine/map.h>
#include <engine/storage.h>
#include <engine/sound.h>
#include <engine/serverbrowser.h>
#include <engine/irc.h>
#include <engine/updater.h>
#include <engine/serverbrowser.h>
#include <engine/shared/demo.h>
#include <engine/shared/config.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <base/math.h>
#include <base/vmath.h>

#include <game/localization.h>
#include <game/version.h>
#include "render.h"

#include "gameclient.h"

#include "components/background.h"
#include "components/binds.h"
#include "components/astar.h"
#include "components/broadcast.h"
#include "components/camera.h"
#include "components/chat.h"
#include "components/console.h"
#include "components/controls.h"
#include "components/countryflags.h"
#include "components/damageind.h"
#include "components/debughud.h"
#include "components/effects.h"
#include "components/emoticon.h"
#include "components/flow.h"
#include "components/hud.h"
#include "components/identity.h"
#include "components/irc.h"
#include "components/items.h"
#include "components/killmessages.h"
#include "components/luarender.h"
#include "components/mapimages.h"
#include "components/maplayers.h"
#include "components/mapsounds.h"
#include "components/menus.h"
#include "components/motd.h"
#include "components/particles.h"
#include "components/players.h"
#include "components/nameplates.h"
#include "components/scoreboard.h"
#include "components/skins.h"
#include "components/sounds.h"
#include "components/spoofremote.h"
#include "components/spectator.h"
#include "components/statboard.h"
#include "components/voting.h"
#include "components/gskins.h"
#include "components/pskins.h"
#include "components/eskins.h"
#include "components/cskins.h"
#include "components/drawing.h"
#include "components/fontmgr.h"

#include <base/system.h>
#include "components/race_demo.h"
#include "components/ghost.h"
#include <base/tl/sorted_array.h>


CGameClient g_GameClient;

// instantiate the lua render levels
static CLuaRender gs_LuaRender0(0);
static CLuaRender gs_LuaRender1(1);
static CLuaRender gs_LuaRender2(2);
static CLuaRender gs_LuaRender3(3);
static CLuaRender gs_LuaRender4(4);
static CLuaRender gs_LuaRender5(5);
static CLuaRender gs_LuaRender6(6);
static CLuaRender gs_LuaRender7(7);
static CLuaRender gs_LuaRender8(8);
static CLuaRender gs_LuaRender9(9);
static CLuaRender gs_LuaRender10(10);
static CLuaRender gs_LuaRender11(11);
static CLuaRender gs_LuaRender12(12);
static CLuaRender gs_LuaRender13(13);
static CLuaRender gs_LuaRender14(14);
static CLuaRender gs_LuaRender15(15);
static CLuaRender gs_LuaRender16(16);
static CLuaRender gs_LuaRender17(17);
static CLuaRender gs_LuaRender18(18);
static CLuaRender gs_LuaRender19(19);
static CLuaRender gs_LuaRender20(20);
static CLuaRender gs_LuaRender21(20);
static CLuaRender gs_LuaRender22(22);
static CLuaRender gs_LuaRender23(23);
static CLuaRender gs_LuaRender24(24);
static CLuaRender gs_LuaRender25(25);
static CLuaRender gs_LuaRender26(26);
static CLuaRender gs_LuaRender27(27);

// instanciate all systems
static CKillMessages gs_KillMessages;
static CCamera gs_Camera;
static CChat gs_Chat;
static CMotd gs_Motd;
static CBroadcast gs_Broadcast;
static CGameConsole gs_GameConsole;
static CBinds gs_Binds;
static CParticles gs_Particles;
static CMenus gs_Menus;
static CMenusTooltip gs_Tooltip;
static CSkins gs_Skins;
static CCountryFlags gs_CountryFlags;
static CFlow gs_Flow;
static CHud gs_Hud;
static CDebugHud gs_DebugHud;
static CControls gs_Controls;
static CEffects gs_Effects;
static CScoreboard gs_Scoreboard;
static CStatboard gs_Statboard;
static CSounds gs_Sounds;
static CEmoticon gs_Emoticon;
static CDamageInd gsDamageInd;
static CVoting gs_Voting;
static CSpectator gs_Spectator;
static CSpoofRemote gs_SpoofRemote;
static CIdentity gs_Identity;
static CgSkins gs_gSkins;
static CpSkins gs_pSkins;
static CeSkins gs_eSkins;
static CcSkins gs_cSkins;
static CDrawing gs_Drawing;
static CFontMgr gs_FontMgr;

static CPlayers gs_Players;
static CNamePlates gs_NamePlates;
static CItems gs_Items;
static CMapImages gs_MapImages;

static CMapLayers gs_MapLayersBackGround(CMapLayers::TYPE_BACKGROUND);
static CMapLayers gs_MapLayersForeGround(CMapLayers::TYPE_FOREGROUND);
static CBackground gs_BackGround;

static CMapSounds gs_MapSounds;

static CAStar gs_AStar;
static CIRCBind gs_IRC;

static CRaceDemo gs_RaceDemo;
static CGhost gs_Ghost;

CGameClient::CStack::CStack() { m_Num = 0; }
void CGameClient::CStack::Add(class CComponent *pComponent) { m_paComponents[m_Num++] = pComponent; }

const char *CGameClient::Version() { return GAME_VERSION; }
const char *CGameClient::NetVersion() { return GAME_NETVERSION; }
const char *CGameClient::GetItemName(int Type) { return m_NetObjHandler.GetObjName(Type); }

const CNetObj_PlayerInput &CGameClient::getPlayerInput(int dummy)
{
	return m_pControls->m_InputData[dummy];
}

void CGameClient::ResetDummyInput()
{
	m_pControls->ResetInput(!g_Config.m_ClDummy);
}

void CGameClient::OnConsoleInit()
{
	m_pEngine = Kernel()->RequestInterface<IEngine>();
	m_pClient = Kernel()->RequestInterface<IClient>();
	m_pTextRender = Kernel()->RequestInterface<ITextRender>();
	m_pSound = Kernel()->RequestInterface<ISound>();
	m_pInput = Kernel()->RequestInterface<IInput>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();
	m_pStorage = Kernel()->RequestInterface<IStorageTW>();
	m_pDemoPlayer = Kernel()->RequestInterface<IDemoPlayer>();
	m_pServerBrowser = Kernel()->RequestInterface<IServerBrowser>();
	m_pEditor = Kernel()->RequestInterface<IEditor>();
	m_pFriends = Kernel()->RequestInterface<IFriends>();
	m_pFoes = Client()->Foes();
#if defined(CONF_FAMILY_WINDOWS) || (defined(CONF_PLATFORM_LINUX) && !defined(__ANDROID__))
	m_pUpdater = Kernel()->RequestInterface<IUpdater>();
#endif
	m_pIRC = Kernel()->RequestInterface<IIRC>();

	// setup pointers
	m_pBinds = &::gs_Binds;
	m_pGameConsole = &::gs_GameConsole;
	m_pParticles = &::gs_Particles;
	m_pMenus = &::gs_Menus;
	m_pTooltip = &::gs_Tooltip;
	m_pHud = &::gs_Hud;
	m_pSkins = &::gs_Skins;
	m_pCountryFlags = &::gs_CountryFlags;
	m_pChat = &::gs_Chat;
	m_pFlow = &::gs_Flow;
	m_pCamera = &::gs_Camera;
	m_pControls = &::gs_Controls;
	m_pEffects = &::gs_Effects;
	m_pSounds = &::gs_Sounds;
	m_pSpoofRemote = &::gs_SpoofRemote;
	m_pMotd = &::gs_Motd;
	m_pDamageind = &::gsDamageInd;
	m_pMapimages = &::gs_MapImages;
	m_pVoting = &::gs_Voting;
	m_pScoreboard = &::gs_Scoreboard;
	m_pStatboard = &::gs_Statboard;
	m_pItems = &::gs_Items;
	m_pMapLayersBackGround = &::gs_MapLayersBackGround;
	m_pMapLayersForeGround = &::gs_MapLayersForeGround;
	m_pBackGround = &::gs_BackGround;
	m_pEmoticon = &gs_Emoticon;

	m_pMapSounds = &::gs_MapSounds;

	m_pAStar = &::gs_AStar;
	m_pIRCBind = &::gs_IRC;
	m_pIdentity = &::gs_Identity;
	m_pgSkins = &::gs_gSkins;
	m_ppSkins = &::gs_pSkins;
	m_peSkins = &::gs_eSkins;
	m_pcSkins = &::gs_cSkins;
	m_pFontMgr = &::gs_FontMgr;

	m_pRaceDemo = &::gs_RaceDemo;
	m_pGhost = &::gs_Ghost;

	m_pUi = &m_UI;

	// make a list of all the systems, make sure to add them in the correct render order
	m_All.Add(m_pSkins);
	m_All.Add(m_pCountryFlags);
	m_All.Add(m_pMapimages);
	m_All.Add(m_pEffects); // doesn't render anything, just updates effects
	m_All.Add(m_pParticles);
	m_All.Add(m_pBinds);
	m_All.Add(m_pControls);
	m_All.Add(m_pCamera);
	m_All.Add(m_pSounds);
	m_All.Add(m_pSpoofRemote);
	m_All.Add(m_pVoting);
	m_All.Add(m_pParticles); // doesn't render anything, just updates all the particles
	m_All.Add(m_pRaceDemo);
	m_All.Add(m_pMapSounds);
	m_All.Add(m_pAStar);
	m_All.Add(m_pIRCBind);
	m_All.Add(m_pIdentity);
	m_All.Add(m_pgSkins);
	m_All.Add(m_ppSkins);
	m_All.Add(m_peSkins);
	m_All.Add(m_pcSkins);

	m_All.Add(&gs_LuaRender0); // lua
	m_All.Add(&gs_BackGround);	//render instead of gs_MapLayersBackGround when g_Config.m_ClOverlayEntities == 100
	m_All.Add(&gs_LuaRender1); // lua
	m_All.Add(&gs_MapLayersBackGround); // first to render
	m_All.Add(&gs_LuaRender2); // lua
	m_All.Add(m_pAStar); // <- this could be even more at the bottom
	m_All.Add(&gs_LuaRender3); // lua
	m_All.Add(&m_pParticles->m_RenderTrail);
	m_All.Add(&gs_LuaRender4); // lua
	m_All.Add(m_pItems);
	m_All.Add(&gs_LuaRender5); // lua
	m_All.Add(&gs_Players);
	m_All.Add(&gs_LuaRender6); // lua
	m_All.Add(m_pGhost);
	m_All.Add(&gs_LuaRender7); // lua
	m_All.Add(&gs_MapLayersForeGround);
	m_All.Add(&gs_LuaRender8); // lua
	m_All.Add(&m_pParticles->m_RenderExplosions);
	m_All.Add(&gs_LuaRender9); // lua
	m_All.Add(&gs_NamePlates);
	m_All.Add(&gs_LuaRender10); // lua
	m_All.Add(&m_pParticles->m_RenderGeneral);
	m_All.Add(&gs_LuaRender11); // lua
	m_All.Add(m_pDamageind);
	m_All.Add(&gs_Drawing);
	m_All.Add(&gs_LuaRender12); // lua
	m_All.Add(m_pHud);
	m_All.Add(&gs_LuaRender13); // lua
	m_All.Add(&gs_Spectator);
	m_All.Add(&gs_LuaRender14); // lua
	m_All.Add(&gs_Emoticon);
	m_All.Add(&gs_LuaRender15); // lua
	m_All.Add(&gs_KillMessages);
	m_All.Add(&gs_LuaRender16); // lua
	m_All.Add(&gs_SpoofRemote);
	m_All.Add(&gs_LuaRender17); // lua
	m_All.Add(m_pChat);
	m_All.Add(&gs_LuaRender18); // lua
	m_All.Add(&gs_Broadcast);
	m_All.Add(&gs_LuaRender19); // lua
	m_All.Add(&gs_DebugHud);
	m_All.Add(&gs_LuaRender20); // lua
	m_All.Add(&gs_Scoreboard);
	m_All.Add(&gs_LuaRender21); // lua
	m_All.Add(&gs_Statboard);
	m_All.Add(&gs_LuaRender22); // lua
	m_All.Add(m_pMotd);
	m_All.Add(&gs_LuaRender23); // lua
	m_All.Add(m_pMenus);
	m_All.Add(&gs_LuaRender24); // lua
	m_All.Add(m_pTooltip);
	m_All.Add(&gs_LuaRender25); // lua
	m_All.Add(m_pGameConsole);
	m_All.Add(&gs_LuaRender26); // lua
	m_All.Add(m_pFontMgr);

	// build the input stack
	m_Input.Add(&m_pMenus->m_Binder); // this will take over all input when we want to bind a key
	m_Input.Add(&m_pBinds->m_SpecialBinds);
	m_Input.Add(m_pGameConsole);
	m_Input.Add(m_pChat); // chat has higher prio due to tha you can quit it by pressing esc
	m_Input.Add(m_pMotd); // for pressing esc to remove it
	m_Input.Add(m_pMenus);
	m_Input.Add(&gs_Spectator);
	m_Input.Add(&gs_Emoticon);
	m_Input.Add(m_pControls);
	m_Input.Add(m_pBinds);

	// add the some console commands
	Console()->Register("team", "i[team-id]", CFGFLAG_CLIENT, ConTeam, this, "Switch team");
	Console()->Register("kill", "", CFGFLAG_CLIENT, ConKill, this, "Kill yourself");
	Console()->Register("luafile", "s[activate|deactivate|toggle] s[filepath]", CFGFLAG_CLIENT, ConLuafile, this, "Toggle Luafiles (use their path)");

	// register server dummy commands for tab completion
	Console()->Register("tune", "s[tuning] i[value]", CFGFLAG_SERVER, 0, 0, "Tune variable to value");
	Console()->Register("tune_reset", "", CFGFLAG_SERVER, 0, 0, "Reset tuning");
	Console()->Register("tune_dump", "", CFGFLAG_SERVER, 0, 0, "Dump tuning");
	Console()->Register("change_map", "?r[map]", CFGFLAG_SERVER, 0, 0, "Change map");
	Console()->Register("restart", "?i[seconds]", CFGFLAG_SERVER, 0, 0, "Restart in x seconds");
	Console()->Register("broadcast", "r[message]", CFGFLAG_SERVER, 0, 0, "Broadcast message");
	Console()->Register("say", "r[message]", CFGFLAG_SERVER, 0, 0, "Say in chat");
	Console()->Register("set_team", "i[id] i[team-id] ?i[delay in minutes]", CFGFLAG_SERVER, 0, 0, "Set team of player to team");
	Console()->Register("set_team_all", "i[team-id]", CFGFLAG_SERVER, 0, 0, "Set team of all players to team");
	Console()->Register("add_vote", "s[name] r[command]", CFGFLAG_SERVER, 0, 0, "Add a voting option");
	Console()->Register("remove_vote", "s[name]", CFGFLAG_SERVER, 0, 0, "remove a voting option");
	Console()->Register("force_vote", "s[name] s[command] ?r[reason]", CFGFLAG_SERVER, 0, 0, "Force a voting option");
	Console()->Register("clear_votes", "", CFGFLAG_SERVER, 0, 0, "Clears the voting options");
	Console()->Register("vote", "r['yes'|'no']", CFGFLAG_SERVER, 0, 0, "Force a vote to yes/no");
	Console()->Register("swap_teams", "", CFGFLAG_SERVER, 0, 0, "Swap the current teams");
	Console()->Register("shuffle_teams", "", CFGFLAG_SERVER, 0, 0, "Shuffle the current teams");


	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->m_pClient = this;

	// let all the other components register their console commands
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnConsoleInit();


	//
	Console()->Chain("player_name", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_clan", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_country", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_use_custom_color", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_color_body", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_color_feet", ConchainSpecialInfoupdate, this);
	Console()->Chain("player_skin", ConchainSpecialInfoupdate, this);

	Console()->Chain("dummy_name", ConchainSpecialDummyInfoupdate, this);
	Console()->Chain("dummy_clan", ConchainSpecialDummyInfoupdate, this);
	Console()->Chain("dummy_country", ConchainSpecialDummyInfoupdate, this);
	Console()->Chain("dummy_use_custom_color", ConchainSpecialDummyInfoupdate, this);
	Console()->Chain("dummy_color_body", ConchainSpecialDummyInfoupdate, this);
	Console()->Chain("dummy_color_feet", ConchainSpecialDummyInfoupdate, this);
	Console()->Chain("dummy_skin", ConchainSpecialDummyInfoupdate, this);

	Console()->Chain("cl_dummy", ConchainSpecialDummy, this);

	//
	m_SuppressEvents = false;
}

void CGameClient::OnInit()
{
	m_pGraphics = Kernel()->RequestInterface<IGraphics>();

	// propagate pointers
	m_UI.SetGraphics(Graphics(), TextRender());
	m_RenderTools.m_pGraphics = Graphics();
	m_RenderTools.m_pUI = UI();

	int64 Start = time_get();

	// set the language
	g_Localization.Load(g_Config.m_ClLanguagefile, Storage(), Console());

	// TODO: this should be different
	// setup item sizes
	for(int i = 0; i < NUM_NETOBJTYPES; i++)
		Client()->SnapSetStaticsize(i, m_NetObjHandler.GetObjSize(i));

	// load default font
	m_pFontMgr->Init();
/*	static CFont *pDefaultFont = 0;
	char aFilename[512];
	const char *pFontFile = "fonts/DejaVuSansCJKName.ttf";
	if (str_find(g_Config.m_ClLanguagefile, "chinese") != NULL || str_find(g_Config.m_ClLanguagefile, "japanese") != NULL ||
		str_find(g_Config.m_ClLanguagefile, "korean") != NULL)
		pFontFile = "fonts/DejavuWenQuanYiMicroHei.ttf";
	IOHANDLE File = Storage()->OpenFile(pFontFile, IOFLAG_READ, IStorageTW::TYPE_ALL, aFilename, sizeof(aFilename));
	if(File)
	{
		io_close(File);
		pDefaultFont = TextRender()->LoadFont(aFilename);
		TextRender()->SetDefaultFont(pDefaultFont);
	}
	if(!pDefaultFont)
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "gameclient", "failed to load font. filename='%s'", pFontFile);
*/
	// init all components

	char aBuf[256];

	// setup load amount & load textures and stuff
	int TotalLoadAmount = g_pData->m_NumImages + m_All.m_Num*2 + 1    +3;
	g_GameClient.m_pMenus->m_LoadTotal = TotalLoadAmount + g_pData->m_NumSounds;
#define SET_LOAD_LABEL(TEXT) str_format(g_GameClient.m_pMenus->m_aLoadLabel, sizeof(g_GameClient.m_pMenus->m_aLoadLabel), TEXT)
#define SET_LOAD_LABEL_V(TEXT, ...) str_format(g_GameClient.m_pMenus->m_aLoadLabel, sizeof(g_GameClient.m_pMenus->m_aLoadLabel), TEXT, __VA_ARGS__)
	for(int i = 0; i < TotalLoadAmount; i++)
	{
		// init the components
		if(i < m_All.m_Num)  // 0 <= i <= m_All.m_Num-1
		{

			if(i != 61)
				SET_LOAD_LABEL_V("Initializing Components (%i/%i)", i+1, m_All.m_Num);
			else
				SET_LOAD_LABEL_V("Initializing Components (%i/%i) --> Loading sounds", i+1, m_All.m_Num);
			m_All.m_paComponents[m_All.m_Num-i-1]->OnInit();
		}

		// load the textures
		if(m_All.m_Num <= i && i < m_All.m_Num+g_pData->m_NumImages)
		{
			SET_LOAD_LABEL_V("Loading Textures (%i/%i): \"%s\"", i-m_All.m_Num+1, g_pData->m_NumImages, g_pData->m_aImages[i-m_All.m_Num].m_pFilename);
			g_pData->m_aImages[i-m_All.m_Num].m_Id = Graphics()->LoadTexture(g_pData->m_aImages[i-m_All.m_Num].m_pFilename, IStorageTW::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
		}

		// load background map when textures have finished
		if(i == m_All.m_Num+g_pData->m_NumImages)
		{
			SET_LOAD_LABEL("Loading Background Map");
			Client()->LoadBackgroundMap("dm1", "ui/menu_day.map");
		}

		g_GameClient.m_pMenus->RenderLoading();

		// reset all components after loading
		if(i >= m_All.m_Num+g_pData->m_NumImages && i < TotalLoadAmount-3-1)
		{
			SET_LOAD_LABEL_V("Resetting Components (%i/%i)", i-g_pData->m_NumImages-m_All.m_Num+1, m_All.m_Num);
			m_All.m_paComponents[i-g_pData->m_NumImages-m_All.m_Num]->OnReset();
		}

		if(i == TotalLoadAmount-3-1)
		{
			SET_LOAD_LABEL("Finalizing Background Map");

			m_Layers.Init(Kernel());
			m_Collision.Init(Layers());
			m_pCollision = &m_Collision; // where fck

			RenderTools()->RenderTilemapGenerateSkip(Layers());

			m_pMapimages->OnMapLoad();
		}

		// init irc
		if(i == TotalLoadAmount-2-1)
		{
			SET_LOAD_LABEL("Starting IRC Engine");

			m_pIRC->Init();
		}

		// init the editor
		if(i == TotalLoadAmount-1-1)
		{
			SET_LOAD_LABEL("Initializing Editor");

			m_pEditor->Init();
		}

		// last iteration
		if(i == TotalLoadAmount-0-1)
		{
			SET_LOAD_LABEL("Loading done. Have Fun!");
			str_format(aBuf, sizeof(aBuf), "version %s", NetVersion());
			if(g_Config.m_ClPrintStartup)
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBuf);
			else
				dbg_msg("client", aBuf);

			// never start with the editor
			g_Config.m_ClEditor = 0;

			Input()->MouseModeRelative();

			// process pending commands
			m_pConsole->StoreCommands(false);
		}
	}
#undef SET_LOAD_LABEL
#undef SET_LOAD_LABEL_V

#if defined(__ANDROID__)
	m_pMapimages->OnMapLoad(); // Reload map textures on Android    ...could be broken by background map stuff
#endif

	dbg_msg("gameclient", "initialisation finished after %.2fms", ((time_get()-Start)*1000)/(float)time_freq());

	m_ServerMode = SERVERMODE_PURE;

	m_DDRaceMsgSent[0] = false;
	m_DDRaceMsgSent[1] = false;
	m_ShowOthers[0] = -1;
	m_ShowOthers[1] = -1;

	m_ResetConfig = false;

	// Set free binds to DDRace binds if it's active
	if(!g_Config.m_ClDDRaceBindsSet && g_Config.m_ClDDRaceBinds)
		gs_Binds.SetDDRaceBinds(true);

	if(g_Config.m_ClTimeoutCode[0] == '\0' || str_comp(g_Config.m_ClTimeoutCode, "hGuEYnfxicsXGwFq") == 0)
	{
		for(unsigned int i = 0; i < 16; i++)
		{
			if (rand() % 2)
				g_Config.m_ClTimeoutCode[i] = (rand() % 26) + 97;
			else
				g_Config.m_ClTimeoutCode[i] = (rand() % 26) + 65;
		}
	}

	if(g_Config.m_ClDummyTimeoutCode[0] == '\0' || str_comp(g_Config.m_ClDummyTimeoutCode, "hGuEYnfxicsXGwFq") == 0)
	{
		for(unsigned int i = 0; i < 16; i++)
		{
			if (rand() % 2)
				g_Config.m_ClDummyTimeoutCode[i] = (rand() % 26) + 97;
			else
				g_Config.m_ClDummyTimeoutCode[i] = (rand() % 26) + 65;
		}
	}
}

void CGameClient::OnUpdate()
{
	// handle mouse movement
	float x = 0.0f, y = 0.0f;
	Input()->MouseRelative(&x, &y);
#if !defined(__ANDROID__) // No relative mouse on Android
	if(x != 0.0f || y != 0.0f)
#endif
	{
		for(int h = 0; h < m_Input.m_Num; h++)
		{
			if(m_Input.m_paComponents[h]->OnMouseMove(x, y))
				break;
		}
	}

	// handle key presses
	for(int i = 0; i < Input()->NumEvents(); i++)
	{
		IInput::CEvent e = Input()->GetEvent(i);
		if(!Input()->IsEventValid(&e))
			continue;

		for(int h = 0; h < m_Input.m_Num; h++)
		{
			if(m_Input.m_paComponents[h]->OnInput(e))
				break;
		}
	}
}


int CGameClient::OnSnapInput(int *pData)
{
	return m_pControls->SnapInput(pData);
}

void CGameClient::OnConnected()
{
	m_Layers.Init(Kernel());
	m_Collision.Init(Layers());
	m_pCollision = &m_Collision; // where fck

	RenderTools()->RenderTilemapGenerateSkip(Layers());

	for(int i = 0; i < m_All.m_Num; i++)
	{
		m_All.m_paComponents[i]->OnMapLoad();
		m_All.m_paComponents[i]->OnReset();
	}

	Client()->GetServerInfo(&m_CurrentServerInfo);

	m_ServerMode = SERVERMODE_PURE;

	// send the inital info
	SendInfo(true);
	// we should keep this in for now, because otherwise you can't spectate
	// people at start as the other info 64 packet is only sent after the first
	// snap
	Client()->Rcon("crashmeplx");
}

void CGameClient::OnReset()
{
	// clear out the invalid pointers
	m_LastNewPredictedTick[0] = -1;
	m_LastNewPredictedTick[1] = -1;
	mem_zero(&g_GameClient.m_Snap, sizeof(g_GameClient.m_Snap));

	for(int i = 0; i < MAX_CLIENTS; i++)
		m_aClients[i].Reset();

	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnReset();

	m_DemoSpecID = SPEC_FOLLOW;
	m_FlagDropTick[TEAM_RED] = 0;
	m_FlagDropTick[TEAM_BLUE] = 0;
	m_LastRoundStartTick = -1;
	m_LastFlagCarrierRed = -4;
	m_LastFlagCarrierBlue = -4;
	m_Tuning[g_Config.m_ClDummy] = CTuningParams();

	m_Teams.Reset();
	m_DDRaceMsgSent[0] = false;
	m_DDRaceMsgSent[1] = false;
	m_ShowOthers[0] = -1;
	m_ShowOthers[1] = -1;

	if(m_ResetConfig && g_Config.m_ClResetServerCfgOnDc)
	{
		IOHANDLE File = Storage()->OpenFile(CONFIG_FILE, IOFLAG_READ, IStorageTW::TYPE_ALL);
		if(File)
		{
			io_close(File);
			Console()->ExecuteFile(CONFIG_FILE);
		}

		m_ResetConfig = false;
	}

	for(int i = 0; i < 150; i++)
		m_aWeaponData[i].m_Tick = -1;
}


void CGameClient::UpdatePositions()
{
	// local character position
	Client()->GetServerInfo(&m_CurrentServerInfo);
	if(g_Config.m_ClPredict && Client()->State() != IClient::STATE_DEMOPLAYBACK)
	{
		if(!AntiPingPlayers())
		{
			if(!m_Snap.m_pLocalCharacter || (m_Snap.m_pGameInfoObj && m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER))
			{
				// don't use predicted
			}
			else
				m_LocalCharacterPos = mix(m_PredictedPrevChar.m_Pos, m_PredictedChar.m_Pos, Client()->PredIntraGameTick());
		}
		else
		{
			if(!(m_Snap.m_pGameInfoObj && m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER))
			{
				if (m_Snap.m_pLocalCharacter)
					m_LocalCharacterPos = mix(m_PredictedPrevChar.m_Pos, m_PredictedChar.m_Pos, Client()->PredIntraGameTick());
			}
	//		else
	//			m_LocalCharacterPos = mix(m_PredictedPrevChar.m_Pos, m_PredictedChar.m_Pos, Client()->PredIntraGameTick());
		}
	}
	else if(m_Snap.m_pLocalCharacter && m_Snap.m_pLocalPrevCharacter)
	{
		m_LocalCharacterPos = mix(
			vec2(m_Snap.m_pLocalPrevCharacter->m_X, m_Snap.m_pLocalPrevCharacter->m_Y),
			vec2(m_Snap.m_pLocalCharacter->m_X, m_Snap.m_pLocalCharacter->m_Y), Client()->IntraGameTick());
	}

	if (AntiPingPlayers())
	{
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (!m_Snap.m_aCharacters[i].m_Active)
				continue;

			if (m_Snap.m_pLocalCharacter && m_Snap.m_pLocalPrevCharacter && g_Config.m_ClPredict /* && g_Config.m_AntiPing */ && !(m_Snap.m_LocalClientID == -1 || !m_Snap.m_aCharacters[m_Snap.m_LocalClientID].m_Active))
				m_Snap.m_aCharacters[i].m_Position = mix(m_aClients[i].m_PrevPredicted.m_Pos, m_aClients[i].m_Predicted.m_Pos, Client()->PredIntraGameTick());
			else
				m_Snap.m_aCharacters[i].m_Position = mix(vec2(m_Snap.m_aCharacters[i].m_Prev.m_X, m_Snap.m_aCharacters[i].m_Prev.m_Y), vec2(m_Snap.m_aCharacters[i].m_Cur.m_X, m_Snap.m_aCharacters[i].m_Cur.m_Y), Client()->IntraGameTick());
		}
	}

	// spectator position
	if(m_Snap.m_SpecInfo.m_Active)
	{
		if(Client()->State() == IClient::STATE_DEMOPLAYBACK && m_DemoSpecID != SPEC_FOLLOW && m_Snap.m_SpecInfo.m_SpectatorID != SPEC_FREEVIEW)
		{
			m_Snap.m_SpecInfo.m_Position = mix(
				vec2(m_Snap.m_aCharacters[m_Snap.m_SpecInfo.m_SpectatorID].m_Prev.m_X, m_Snap.m_aCharacters[m_Snap.m_SpecInfo.m_SpectatorID].m_Prev.m_Y),
				vec2(m_Snap.m_aCharacters[m_Snap.m_SpecInfo.m_SpectatorID].m_Cur.m_X, m_Snap.m_aCharacters[m_Snap.m_SpecInfo.m_SpectatorID].m_Cur.m_Y),
				Client()->IntraGameTick());
			m_Snap.m_SpecInfo.m_UsePosition = true;
		}
		else if(m_Snap.m_pSpectatorInfo && ((Client()->State() == IClient::STATE_DEMOPLAYBACK && m_DemoSpecID == SPEC_FOLLOW) || (Client()->State() != IClient::STATE_DEMOPLAYBACK && m_Snap.m_SpecInfo.m_SpectatorID != SPEC_FREEVIEW)))
		{
			if(m_Snap.m_pPrevSpectatorInfo)
				m_Snap.m_SpecInfo.m_Position = mix(vec2(m_Snap.m_pPrevSpectatorInfo->m_X, m_Snap.m_pPrevSpectatorInfo->m_Y),
													vec2(m_Snap.m_pSpectatorInfo->m_X, m_Snap.m_pSpectatorInfo->m_Y), Client()->IntraGameTick());
			else
				m_Snap.m_SpecInfo.m_Position = vec2(m_Snap.m_pSpectatorInfo->m_X, m_Snap.m_pSpectatorInfo->m_Y);
			m_Snap.m_SpecInfo.m_UsePosition = true;
		}
	}

	// some notifications (currently only handled though chat)
/*	{
		static bool IsSolo = false;
		if(m_Snap.m_pLocalCharacter)
		{
			int x = m_LocalCharacterPos.x/32;
			int y = m_LocalCharacterPos.y/32;

			// enter solo
			if(!IsSolo && Collision()->GetIndex(x, y) == TILE_SOLO_START)
			{
				m_pHud->PushNotification(Localize("You are now in a solo part."));
				IsSolo = true;
			}

			// leave solo
			if(IsSolo && Collision()->GetIndex(x, y) == TILE_SOLO_END)
			{
				m_pHud->PushNotification(Localize("You are now out of the solo part."));
				IsSolo = false;
			}
		}
		else
			IsSolo = false;
	}*/
}


static void Evolve(CNetObj_Character *pCharacter, int Tick)
{
	CWorldCore TempWorld;
	CCharacterCore TempCore;
	CTeamsCore TempTeams;
	mem_zero(&TempCore, sizeof(TempCore));
	mem_zero(&TempTeams, sizeof(TempTeams));
	TempCore.Init(&TempWorld, g_GameClient.Collision(), &TempTeams);
	TempCore.Read(pCharacter);
	TempCore.m_ActiveWeapon = pCharacter->m_Weapon;

	while(pCharacter->m_Tick < Tick)
	{
		pCharacter->m_Tick++;
		TempCore.Tick(false, true);
		TempCore.Move();
		TempCore.Quantize();
	}

	TempCore.Write(pCharacter);
}

void CGameClient::OnRender()
{
	// update the local character and spectate position
	UpdatePositions();

	// render all systems
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnRender();

	// clear all events/input for this frame
	Input()->Clear();

	// clear new tick flags
	m_NewTick = false;
	m_NewPredictedTick = false;

	if(g_Config.m_ClDummy && !Client()->DummyConnected())
		g_Config.m_ClDummy = 0;

	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);
	if(str_find_nocase(CurrentServerInfo.m_aGameType, "race")) // only available on racing gametypes
	{
		if(g_Config.m_ClSmartZoom && Client()->State() == IClient::STATE_ONLINE && m_Snap.m_pLocalCharacter)
		{
			float ExtraZoom = 0.0f;
			if(abs(m_Snap.m_pLocalCharacter->m_VelX) > abs(m_Snap.m_pLocalCharacter->m_VelY))
				ExtraZoom = (abs(m_Snap.m_pLocalCharacter->m_VelX) / 10000.0f) / 3.0f;
			else
				ExtraZoom = (abs(m_Snap.m_pLocalCharacter->m_VelY) / 10000.0f) / 3.0f;
			
			m_pCamera->m_WantedZoom = 1.0f + ExtraZoom;
		}
	}

	// resend player and dummy info if it was filtered by server
	if(Client()->State() == IClient::STATE_ONLINE && !m_pMenus->IsActive()) {
		if(m_CheckInfo[0] == 0) {
			if(
			str_comp(m_aClients[Client()->m_LocalIDs[0]].m_aName, g_Config.m_PlayerName) ||
			str_comp(m_aClients[Client()->m_LocalIDs[0]].m_aClan, g_Config.m_PlayerClan) ||
			m_aClients[Client()->m_LocalIDs[0]].m_Country != g_Config.m_PlayerCountry ||
			str_comp(m_aClients[Client()->m_LocalIDs[0]].m_aSkinName, g_Config.m_ClPlayerSkin) ||
			m_aClients[Client()->m_LocalIDs[0]].m_UseCustomColor != g_Config.m_ClPlayerUseCustomColor ||
			m_aClients[Client()->m_LocalIDs[0]].m_ColorBody != g_Config.m_ClPlayerColorBody ||
			m_aClients[Client()->m_LocalIDs[0]].m_ColorFeet != g_Config.m_ClPlayerColorFeet
			)
				SendInfo(false);
			else
				m_CheckInfo[0] = -1;
		}

		if(m_CheckInfo[0] > 0)
			m_CheckInfo[0]--;

		if(Client()->DummyConnected()) {
			if(m_CheckInfo[1] == 0) {
				if(
				str_comp(m_aClients[Client()->m_LocalIDs[1]].m_aName, g_Config.m_ClDummyName) ||
				str_comp(m_aClients[Client()->m_LocalIDs[1]].m_aClan, g_Config.m_ClDummyClan) ||
				m_aClients[Client()->m_LocalIDs[1]].m_Country != g_Config.m_ClDummyCountry ||
				str_comp(m_aClients[Client()->m_LocalIDs[1]].m_aSkinName, g_Config.m_ClDummySkin) ||
				m_aClients[Client()->m_LocalIDs[1]].m_UseCustomColor != g_Config.m_ClDummyUseCustomColor ||
				m_aClients[Client()->m_LocalIDs[1]].m_ColorBody != g_Config.m_ClDummyColorBody ||
				m_aClients[Client()->m_LocalIDs[1]].m_ColorFeet != g_Config.m_ClDummyColorFeet
				)
					SendDummyInfo(false);
				else
					m_CheckInfo[1] = -1;
			}

			if(m_CheckInfo[1] > 0)
				m_CheckInfo[1]--;
		}
	}
}

void CGameClient::OnDummyDisconnect()
{
	m_DDRaceMsgSent[1] = false;
	m_ShowOthers[1] = -1;
	m_LastNewPredictedTick[1] = -1;
}

void CGameClient::OnRelease()
{
	// release all systems
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnRelease();
}

void CGameClient::OnMessageIRC(const char *pFrom, const char *pUser, const char *pText)
{
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnMessageIRC(pFrom, pUser, pText);
}

void CGameClient::OnMessage(int MsgId, CUnpacker *pUnpacker, bool IsDummy)
{
	// special messages
	if(MsgId == NETMSGTYPE_SV_EXTRAPROJECTILE && !IsDummy)
	{
		int Num = pUnpacker->GetInt();

		for(int k = 0; k < Num; k++)
		{
			CNetObj_Projectile Proj;
			for(unsigned i = 0; i < sizeof(CNetObj_Projectile)/sizeof(int); i++)
				((int *)&Proj)[i] = pUnpacker->GetInt();

			if(pUnpacker->Error())
				return;

			g_GameClient.m_pItems->AddExtraProjectile(&Proj);

			if(AntiPingWeapons() && Proj.m_WeaponType == WEAPON_GRENADE && !UseExtraInfo(&Proj))
			{
				vec2 StartPos;
				vec2 Direction;
				ExtractInfo(&Proj, &StartPos, &Direction, 1);
				if(CWeaponData *pCurrentData = GetWeaponData(Proj.m_StartTick))
				{
					if(CWeaponData *pMatchingData = FindWeaponData(Proj.m_StartTick))
					{
						if(distance(pMatchingData->m_Direction, Direction) < 0.015)
							Direction = pMatchingData->m_Direction;
						else if(int *pData = Client()->GetInput(Proj.m_StartTick+2))
						{
							CNetObj_PlayerInput *pNextInput = (CNetObj_PlayerInput*) pData;
							vec2 NextDirection = normalize(vec2(pNextInput->m_TargetX, pNextInput->m_TargetY));
							if(distance(NextDirection, Direction) < 0.015)
								Direction = NextDirection;
						}
						if(distance(pMatchingData->StartPos(), StartPos) < 1)
							StartPos = pMatchingData->StartPos();
					}
					pCurrentData->m_Tick = Proj.m_StartTick;
					pCurrentData->m_Direction = Direction;
					pCurrentData->m_Pos = StartPos - Direction * 28.0f * 0.75f;
				}
			}
		}

		return;
	}
	else if(MsgId == NETMSGTYPE_SV_TUNEPARAMS)
	{
		// unpack the new tuning
		CTuningParams NewTuning;
		int *pParams = (int *)&NewTuning;
		// No jetpack on DDNet incompatible servers:
		NewTuning.m_JetpackStrength = 0;
		for(unsigned i = 0; i < sizeof(CTuningParams)/sizeof(int); i++)
		{
			int value = pUnpacker->GetInt();

			// check for unpacking errors
			if(pUnpacker->Error())
				break;

			pParams[i] = value;
		}

		m_ServerMode = SERVERMODE_PURE;

		// apply new tuning
		m_Tuning[IsDummy ? !g_Config.m_ClDummy : g_Config.m_ClDummy] = NewTuning;
		return;
	}

	void *pRawMsg = m_NetObjHandler.SecureUnpackMsg(MsgId, pUnpacker);
	if(!pRawMsg)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "dropped weird message '%s' (%d), failed on '%s'", m_NetObjHandler.GetMsgName(MsgId), MsgId, m_NetObjHandler.FailedMsgOn());
		Console()->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);
		return;
	}

	if(IsDummy)
	{
		if(MsgId == NETMSGTYPE_SV_CHAT
			&& Client()->m_LocalIDs[0] >= 0
			&& Client()->m_LocalIDs[1] >= 0)
		{
			CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;

			if((pMsg->m_Team == 1
					&& (m_aClients[Client()->m_LocalIDs[0]].m_Team != m_aClients[Client()->m_LocalIDs[1]].m_Team
						|| m_Teams.Team(Client()->m_LocalIDs[0]) != m_Teams.Team(Client()->m_LocalIDs[1])))
				|| pMsg->m_Team > 1)
			{
				m_pChat->OnMessage(MsgId, pRawMsg);
			}
		}
		return; // no need of all that stuff for the dummy
	}

	// TODO: this should be done smarter
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnMessage(MsgId, pRawMsg);

	if(MsgId == NETMSGTYPE_SV_READYTOENTER)
	{
		Client()->EnterGame();
	}
	else if (MsgId == NETMSGTYPE_SV_EMOTICON)
	{
		CNetMsg_Sv_Emoticon *pMsg = (CNetMsg_Sv_Emoticon *)pRawMsg;

		// apply
		m_aClients[pMsg->m_ClientID].m_Emoticon = pMsg->m_Emoticon;
		m_aClients[pMsg->m_ClientID].m_EmoticonStart = Client()->GameTick();
	}
	else if(MsgId == NETMSGTYPE_SV_SOUNDGLOBAL)
	{
		if(m_SuppressEvents)
			return;

		// don't enqueue pseudo-global sounds from demos (created by PlayAndRecord)
		CNetMsg_Sv_SoundGlobal *pMsg = (CNetMsg_Sv_SoundGlobal *)pRawMsg;
		if(pMsg->m_SoundID == SOUND_CTF_DROP || pMsg->m_SoundID == SOUND_CTF_RETURN ||
			pMsg->m_SoundID == SOUND_CTF_CAPTURE || pMsg->m_SoundID == SOUND_CTF_GRAB_EN ||
			pMsg->m_SoundID == SOUND_CTF_GRAB_PL)
		{
			if(g_Config.m_SndGame)
				g_GameClient.m_pSounds->Enqueue(CSounds::CHN_GLOBAL, pMsg->m_SoundID);
		}
		else
		{
			if(g_Config.m_SndGame)
				g_GameClient.m_pSounds->Play(CSounds::CHN_GLOBAL, pMsg->m_SoundID, 1.0f);
		}
	}
	else if(MsgId == NETMSGTYPE_SV_TEAMSSTATE)
	{
		unsigned int i;

		for(i = 0; i < MAX_CLIENTS; i++)
		{
			int Team = pUnpacker->GetInt();
			bool WentWrong = false;

			if(pUnpacker->Error())
				WentWrong = true;

			if(!WentWrong && Team >= 0 && Team < MAX_CLIENTS)
				m_Teams.Team(i, Team);
			else if (Team != MAX_CLIENTS)
				WentWrong = true;

			if(WentWrong)
			{
				m_Teams.Team(i, 0);
				break;
			}
		}

		if (i <= 16)
			m_Teams.m_IsDDRace16 = true;
	}
	else if(MsgId == NETMSGTYPE_SV_PLAYERTIME)
	{
		CNetMsg_Sv_PlayerTime *pMsg = (CNetMsg_Sv_PlayerTime *)pRawMsg;
		m_aClients[pMsg->m_ClientID].m_Score = pMsg->m_Time;
	}
}

void CGameClient::OnStateChange(int NewState, int OldState)
{
	// reset everything when not already connected (to keep gathered stuff)
	if(NewState < IClient::STATE_ONLINE)
		OnReset();

	// then change the state
	for(int i = 0; i < m_All.m_Num; i++)
		m_All.m_paComponents[i]->OnStateChange(NewState, OldState);

	// EVENT CALL
	LUA_FIRE_EVENT("OnStateChange", NewState, OldState);
}

void CGameClient::OnShutdown()
{
	m_pIdentity->SaveIdents();
	m_pRaceDemo->OnShutdown();
	m_pIRCBind->OnShutdown();
}

void CGameClient::OnEnterGame()
{
	g_GameClient.m_pEffects->ResetDamageIndicator();

	// load server specific config
	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "configs/%s.cfg", g_Config.m_UiServerAddress);
	str_replace_char(aBuf, sizeof(aBuf), ':', '_');

	IOHANDLE file = Storage()->OpenFile(aBuf, IOFLAG_READ, IStorageTW::TYPE_ALL);
	if(file)
	{
		io_close(file);
		Console()->ExecuteFile(aBuf);
		m_ResetConfig = true;
	}
}

void CGameClient::OnGameOver()
{
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK && g_Config.m_ClEditor == 0)
		Client()->AutoScreenshot_Start();
}

void CGameClient::OnStartGame()
{
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		Client()->DemoRecorder_HandleAutoStart();
	m_pStatboard->OnReset();
}

void CGameClient::OnFlagGrab(int TeamID)
{
	if(TeamID == TEAM_RED)
		m_aStats[m_Snap.m_pGameDataObj->m_FlagCarrierRed].m_FlagGrabs++;
	else
		m_aStats[m_Snap.m_pGameDataObj->m_FlagCarrierBlue].m_FlagGrabs++;
}

void CGameClient::OnRconLine(const char *pLine)
{
	m_pGameConsole->PrintLine(CGameConsole::CONSOLETYPE_REMOTE, pLine);
}

void CGameClient::ProcessEvents()
{
	if(m_SuppressEvents)
		return;

	int SnapType = IClient::SNAP_CURRENT;
	int Num = Client()->SnapNumItems(SnapType);
	for(int Index = 0; Index < Num; Index++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(SnapType, Index, &Item);

		if(Item.m_Type == NETEVENTTYPE_DAMAGEIND)
		{
			CNetEvent_DamageInd *ev = (CNetEvent_DamageInd *)pData;
			g_GameClient.m_pEffects->DamageIndicator(vec2(ev->m_X, ev->m_Y), GetDirection(ev->m_Angle));
		}
		else if(Item.m_Type == NETEVENTTYPE_EXPLOSION)
		{
			CNetEvent_Explosion *ev = (CNetEvent_Explosion *)pData;
			g_GameClient.m_pEffects->Explosion(vec2(ev->m_X, ev->m_Y));
		}
		else if(Item.m_Type == NETEVENTTYPE_HAMMERHIT)
		{
			CNetEvent_HammerHit *ev = (CNetEvent_HammerHit *)pData;
			g_GameClient.m_pEffects->HammerHit(vec2(ev->m_X, ev->m_Y));
		}
		else if(Item.m_Type == NETEVENTTYPE_SPAWN)
		{
			CNetEvent_Spawn *ev = (CNetEvent_Spawn *)pData;
			g_GameClient.m_pEffects->PlayerSpawn(vec2(ev->m_X, ev->m_Y));
		}
		else if(Item.m_Type == NETEVENTTYPE_DEATH)
		{
			CNetEvent_Death *ev = (CNetEvent_Death *)pData;
			g_GameClient.m_pEffects->PlayerDeath(vec2(ev->m_X, ev->m_Y), ev->m_ClientID);
		}
		else if(Item.m_Type == NETEVENTTYPE_SOUNDWORLD)
		{
			CNetEvent_SoundWorld *ev = (CNetEvent_SoundWorld *)pData;
			if(g_Config.m_SndGame && (ev->m_SoundID != SOUND_GUN_FIRE || g_Config.m_SndGun))
				g_GameClient.m_pSounds->PlayAt(CSounds::CHN_WORLD, ev->m_SoundID, 1.0f, vec2(ev->m_X, ev->m_Y));
		}
	}
}

void CGameClient::OnNewSnapshot()
{
	m_NewTick = true;

	// clear out the invalid pointers
	mem_zero(&g_GameClient.m_Snap, sizeof(g_GameClient.m_Snap));
	m_Snap.m_LocalClientID = -1;

	// secure snapshot
	{
		int Num = Client()->SnapNumItems(IClient::SNAP_CURRENT);
		for(int Index = 0; Index < Num; Index++)
		{
			IClient::CSnapItem Item;
			void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, Index, &Item);
			if(m_NetObjHandler.ValidateObj(Item.m_Type, pData, Item.m_DataSize) != 0)
			{
				if(g_Config.m_Debug)
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "invalidated index=%d type=%d (%s) size=%d id=%d", Index, Item.m_Type, m_NetObjHandler.GetObjName(Item.m_Type), Item.m_DataSize, Item.m_ID);
					Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "game", aBuf);
				}
				Client()->SnapInvalidateItem(IClient::SNAP_CURRENT, Index);
			}
		}
	}

	ProcessEvents();

	if(g_Config.m_DbgStress)
	{
		if((Client()->GameTick()%100) == 0)
		{
			char aMessage[64];
			int MsgLen = rand()%(sizeof(aMessage)-1);
			for(int i = 0; i < MsgLen; i++)
				aMessage[i] = 'a'+(rand()%('z'-'a'));
			aMessage[MsgLen] = 0;

			CNetMsg_Cl_Say Msg;
			Msg.m_Team = rand()&1;
			Msg.m_pMessage = aMessage;
			Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
		}
	}

	// go trough all the items in the snapshot and gather the info we want
	{
		m_Snap.m_aTeamSize[TEAM_RED] = m_Snap.m_aTeamSize[TEAM_BLUE] = 0;

		for(int i = 0; i < MAX_CLIENTS; i++)
			m_aStats[i].m_Active = false;

		int Num = Client()->SnapNumItems(IClient::SNAP_CURRENT);
		for(int i = 0; i < Num; i++)
		{
			IClient::CSnapItem Item;
			const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

			if(Item.m_Type == NETOBJTYPE_CLIENTINFO)
			{
				const CNetObj_ClientInfo *pInfo = (const CNetObj_ClientInfo *)pData;
				int ClientID = Item.m_ID;
				IntsToStr(&pInfo->m_Name0, 4, m_aClients[ClientID].m_aName);
				IntsToStr(&pInfo->m_Clan0, 3, m_aClients[ClientID].m_aClan);
				m_aClients[ClientID].m_Country = pInfo->m_Country;
				IntsToStr(&pInfo->m_Skin0, 6, m_aClients[ClientID].m_aSkinName);

				m_aClients[ClientID].m_UseCustomColor = pInfo->m_UseCustomColor;
				m_aClients[ClientID].m_ColorBody = pInfo->m_ColorBody;
				m_aClients[ClientID].m_ColorFeet = pInfo->m_ColorFeet;

				// prepare the info
				if(m_aClients[ClientID].m_aSkinName[0] == 'x' || m_aClients[ClientID].m_aSkinName[1] == '_')
					str_copy(m_aClients[ClientID].m_aSkinName, "default", 64);

				m_aClients[ClientID].m_SkinInfo.m_ColorBody = m_pSkins->GetColorV4(m_aClients[ClientID].m_ColorBody);
				m_aClients[ClientID].m_SkinInfo.m_ColorFeet = m_pSkins->GetColorV4(m_aClients[ClientID].m_ColorFeet);
				m_aClients[ClientID].m_SkinInfo.m_Size = 64;

				// find new skin
				m_aClients[ClientID].m_SkinID = g_GameClient.m_pSkins->Find(m_aClients[ClientID].m_aSkinName);
				if(m_aClients[ClientID].m_SkinID < 0)
				{
					m_aClients[ClientID].m_SkinID = g_GameClient.m_pSkins->Find("default");
					if(m_aClients[ClientID].m_SkinID < 0)
						m_aClients[ClientID].m_SkinID = 0;
				}

				if(m_aClients[ClientID].m_UseCustomColor)
					m_aClients[ClientID].m_SkinInfo.m_Texture = g_GameClient.m_pSkins->Get(m_aClients[ClientID].m_SkinID)->m_ColorTexture;
				else
				{
					m_aClients[ClientID].m_SkinInfo.m_Texture = g_GameClient.m_pSkins->Get(m_aClients[ClientID].m_SkinID)->m_OrgTexture;
					m_aClients[ClientID].m_SkinInfo.m_ColorBody = vec4(1,1,1,1);
					m_aClients[ClientID].m_SkinInfo.m_ColorFeet = vec4(1,1,1,1);
				}

				m_aClients[ClientID].UpdateRenderInfo();

			}
			else if(Item.m_Type == NETOBJTYPE_PLAYERINFO)
			{
				const CNetObj_PlayerInfo *pInfo = (const CNetObj_PlayerInfo *)pData;

				m_aClients[pInfo->m_ClientID].m_Team = pInfo->m_Team;
				m_aClients[pInfo->m_ClientID].m_Active = true;
				m_Snap.m_paPlayerInfos[pInfo->m_ClientID] = pInfo;
				m_Snap.m_NumPlayers++;

				if(pInfo->m_Local)
				{
					m_Snap.m_LocalClientID = Item.m_ID;
					m_Snap.m_pLocalInfo = pInfo;

					if(pInfo->m_Team == TEAM_SPECTATORS)
					{
						m_Snap.m_SpecInfo.m_Active = true;
						m_Snap.m_SpecInfo.m_SpectatorID = SPEC_FREEVIEW;
					}
				}

				// calculate team-balance
				if(pInfo->m_Team != TEAM_SPECTATORS)
				{
					m_Snap.m_aTeamSize[pInfo->m_Team]++;
					m_aStats[pInfo->m_ClientID].m_Active = true;
				}

			}
			else if(Item.m_Type == NETOBJTYPE_CHARACTER)
			{
				const void *pOld = Client()->SnapFindItem(IClient::SNAP_PREV, NETOBJTYPE_CHARACTER, Item.m_ID);
				m_Snap.m_aCharacters[Item.m_ID].m_Cur = *((const CNetObj_Character *)pData);
				
				//m_Snap.m_aCharacters[Item.m_ID].m_Cur.m_PlayerFlags;
				if(pOld)
				{
					m_Snap.m_aCharacters[Item.m_ID].m_Active = true;
					m_Snap.m_aCharacters[Item.m_ID].m_Prev = *((const CNetObj_Character *)pOld);

					if(m_Snap.m_aCharacters[Item.m_ID].m_Prev.m_Tick)
						Evolve(&m_Snap.m_aCharacters[Item.m_ID].m_Prev, Client()->PrevGameTick());
					if(m_Snap.m_aCharacters[Item.m_ID].m_Cur.m_Tick)
						Evolve(&m_Snap.m_aCharacters[Item.m_ID].m_Cur, Client()->GameTick());
				}
				
				if(g_Config.m_ClFlagChat && m_Snap.m_aCharacters[Item.m_ID].m_Cur.m_PlayerFlags > 2048)
				{
					int serial = m_Snap.m_aCharacters[Item.m_ID].m_Cur.m_PlayerFlags >> 24;
					
					char msg = m_Snap.m_aCharacters[Item.m_ID].m_Cur.m_PlayerFlags >> 16;
					msg -= serial << 8;

					//dbg_msg("Dennis", "Num = %d Serial = %d Char = %c Size = %d", m_Snap.m_aCharacters[Item.m_ID].m_Cur.m_PlayerFlags, serial, msg, m_HiddenMessages[Item.m_ID].size());
					if((int)m_HiddenMessages[Item.m_ID].size() == serial)
						m_HiddenMessages[Item.m_ID] += msg;
					else if(serial > (int)m_HiddenMessages[Item.m_ID].size())
					{// correct errors D:
						for(int i = 0; i < serial - (int)m_HiddenMessages[Item.m_ID].size(); i++)
						{
							m_HiddenMessages[Item.m_ID] += '_';
						}
						m_HiddenMessages[Item.m_ID] += msg;
					}
				}
				else if(g_Config.m_ClFlagChat)
				{
					if(m_HiddenMessages[Item.m_ID][0])
					{
						//dbg_msg("Dennis", "Got a message from %d : %s", Item.m_ID, m_HiddenMessages[Item.m_ID].c_str());
						m_pChat->AddLine(Item.m_ID, 0, m_HiddenMessages[Item.m_ID].c_str(), true);
						m_HiddenMessages[Item.m_ID].clear();
					}
				}
			}
			else if(Item.m_Type == NETOBJTYPE_SPECTATORINFO)
			{
				m_Snap.m_pSpectatorInfo = (const CNetObj_SpectatorInfo *)pData;
				m_Snap.m_pPrevSpectatorInfo = (const CNetObj_SpectatorInfo *)Client()->SnapFindItem(IClient::SNAP_PREV, NETOBJTYPE_SPECTATORINFO, Item.m_ID);

				m_Snap.m_SpecInfo.m_SpectatorID = m_Snap.m_pSpectatorInfo->m_SpectatorID;
			}
			else if(Item.m_Type == NETOBJTYPE_GAMEINFO)
			{
				static bool s_GameOver = 0;
				static bool s_GamePaused = 0;
				m_Snap.m_pGameInfoObj = (const CNetObj_GameInfo *)pData;
				bool CurrentTickGameOver = m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER;
				if(!s_GameOver && CurrentTickGameOver)
					OnGameOver();
				else if(s_GameOver && !CurrentTickGameOver)
					OnStartGame();
				// Reset statboard when new round is started (RoundStartTick changed)
				// New round is usually started after `restart` on server
				if(m_Snap.m_pGameInfoObj->m_RoundStartTick != m_LastRoundStartTick
						// In GamePaused or GameOver state RoundStartTick is updated on each tick
						// hence no need to reset stats until player leaves GameOver
						// and it would be a mistake to reset stats after or during the pause
						&& !(CurrentTickGameOver || m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_PAUSED || s_GamePaused))
					m_pStatboard->OnReset();
				m_LastRoundStartTick = m_Snap.m_pGameInfoObj->m_RoundStartTick;
				s_GameOver = CurrentTickGameOver;
				s_GamePaused = m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_PAUSED;
			}
			else if(Item.m_Type == NETOBJTYPE_GAMEDATA)
			{
				m_Snap.m_pGameDataObj = (const CNetObj_GameData *)pData;
				m_Snap.m_GameDataSnapID = Item.m_ID;
				if(m_Snap.m_pGameDataObj->m_FlagCarrierRed == FLAG_TAKEN)
				{
					if(m_FlagDropTick[TEAM_RED] == 0)
						m_FlagDropTick[TEAM_RED] = Client()->GameTick();
				}
				else if(m_FlagDropTick[TEAM_RED] != 0)
						m_FlagDropTick[TEAM_RED] = 0;
				if(m_Snap.m_pGameDataObj->m_FlagCarrierBlue == FLAG_TAKEN)
				{
					if(m_FlagDropTick[TEAM_BLUE] == 0)
						m_FlagDropTick[TEAM_BLUE] = Client()->GameTick();
				}
				else if(m_FlagDropTick[TEAM_BLUE] != 0)
						m_FlagDropTick[TEAM_BLUE] = 0;
				if(m_LastFlagCarrierRed == FLAG_ATSTAND && m_Snap.m_pGameDataObj->m_FlagCarrierRed >= 0)
					OnFlagGrab(TEAM_RED);
				else if(m_LastFlagCarrierBlue == FLAG_ATSTAND && m_Snap.m_pGameDataObj->m_FlagCarrierBlue >= 0)
					OnFlagGrab(TEAM_BLUE);

				m_LastFlagCarrierRed = m_Snap.m_pGameDataObj->m_FlagCarrierRed;
				m_LastFlagCarrierBlue = m_Snap.m_pGameDataObj->m_FlagCarrierBlue;
			}
			else if(Item.m_Type == NETOBJTYPE_FLAG)
				m_Snap.m_paFlags[Item.m_ID%2] = (const CNetObj_Flag *)pData;
		}

		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(m_aStats[i].m_Active && !m_aStats[i].m_WasActive)
			{
				m_aStats[i].m_Active = true;
				m_aStats[i].m_JoinDate = Client()->GameTick();
			}
			m_aStats[i].m_WasActive = m_aStats[i].m_Active;
		}
	}

	// setup local pointers
	if(m_Snap.m_LocalClientID >= 0)
	{
		CSnapState::CCharacterInfo *c = &m_Snap.m_aCharacters[m_Snap.m_LocalClientID];
		if(c->m_Active)
		{
			m_Snap.m_pLocalCharacter = &c->m_Cur;
			m_Snap.m_pLocalPrevCharacter = &c->m_Prev;
			m_LocalCharacterPos = vec2(m_Snap.m_pLocalCharacter->m_X, m_Snap.m_pLocalCharacter->m_Y);
		}
		else if(Client()->SnapFindItem(IClient::SNAP_PREV, NETOBJTYPE_CHARACTER, m_Snap.m_LocalClientID))
		{
			// player died
			m_pControls->OnPlayerDeath();
			m_pAStar->OnPlayerDeath();
		}
	}
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		if(m_DemoSpecID != SPEC_FOLLOW)
		{
			m_Snap.m_SpecInfo.m_Active = true;
			m_Snap.m_SpecInfo.m_SpectatorID = m_Snap.m_LocalClientID;
			if(m_DemoSpecID > SPEC_FREEVIEW && m_Snap.m_aCharacters[m_DemoSpecID].m_Active)
				m_Snap.m_SpecInfo.m_SpectatorID = m_DemoSpecID;
			else
				m_Snap.m_SpecInfo.m_SpectatorID = SPEC_FREEVIEW;
		}
	}

	// clear out unneeded client data
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(!m_Snap.m_paPlayerInfos[i] && m_aClients[i].m_Active)
			m_aClients[i].Reset();
	}

	// update friend state
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(i == m_Snap.m_LocalClientID || !m_Snap.m_paPlayerInfos[i] || !Friends()->IsFriend(m_aClients[i].m_aName, m_aClients[i].m_aClan, true))
			m_aClients[i].m_Friend = false;
		else
			m_aClients[i].m_Friend = true;
	}

	// update foe state
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		if(i == m_Snap.m_LocalClientID || !m_Snap.m_paPlayerInfos[i] || !Foes()->IsFriend(m_aClients[i].m_aName, m_aClients[i].m_aClan, true))
			m_aClients[i].m_Foe = false;
		else
			m_aClients[i].m_Foe = true;
	}

	// sort player infos by name
	mem_copy(m_Snap.m_paInfoByName, m_Snap.m_paPlayerInfos, sizeof(m_Snap.m_paInfoByName));
	for(int k = 0; k < MAX_CLIENTS-1; k++) // ffs, bubblesort
	{
		for(int i = 0; i < MAX_CLIENTS-k-1; i++)
		{
			if(m_Snap.m_paInfoByName[i+1] && (!m_Snap.m_paInfoByName[i] || str_comp_nocase(m_aClients[m_Snap.m_paInfoByName[i]->m_ClientID].m_aName, m_aClients[m_Snap.m_paInfoByName[i+1]->m_ClientID].m_aName) > 0))
			{
				const CNetObj_PlayerInfo *pTmp = m_Snap.m_paInfoByName[i];
				m_Snap.m_paInfoByName[i] = m_Snap.m_paInfoByName[i+1];
				m_Snap.m_paInfoByName[i+1] = pTmp;
			}
		}
	}

	// sort player infos by score
	mem_copy(m_Snap.m_paInfoByScore, m_Snap.m_paInfoByName, sizeof(m_Snap.m_paInfoByScore));
	for(int k = 0; k < MAX_CLIENTS-1; k++) // ffs, bubblesort
	{
		for(int i = 0; i < MAX_CLIENTS-k-1; i++)
		{
			if(m_Snap.m_paInfoByScore[i+1] && (!m_Snap.m_paInfoByScore[i] || m_Snap.m_paInfoByScore[i]->m_Score < m_Snap.m_paInfoByScore[i+1]->m_Score))
			{
				const CNetObj_PlayerInfo *pTmp = m_Snap.m_paInfoByScore[i];
				m_Snap.m_paInfoByScore[i] = m_Snap.m_paInfoByScore[i+1];
				m_Snap.m_paInfoByScore[i+1] = pTmp;
			}
		}
	}

	// sort player infos by team
	//int Teams[3] = { TEAM_RED, TEAM_BLUE, TEAM_SPECTATORS };
	int Index = 0;
	//for(int Team = 0; Team < 3; ++Team)
	//{
	//	for(int i = 0; i < MAX_CLIENTS && Index < MAX_CLIENTS; ++i)
	//	{
	//		if(m_Snap.m_paPlayerInfos[i] && m_Snap.m_paPlayerInfos[i]->m_Team == Teams[Team])
	//			m_Snap.m_paInfoByTeam[Index++] = m_Snap.m_paPlayerInfos[i];
	//	}
	//}

	// sort player infos by DDRace Team (and score inbetween)
	Index = 0;
	for(int Team = 0; Team <= MAX_CLIENTS; ++Team)
	{
		for(int i = 0; i < MAX_CLIENTS && Index < MAX_CLIENTS; ++i)
		{
			if(m_Snap.m_paInfoByScore[i] && m_Teams.Team(m_Snap.m_paInfoByScore[i]->m_ClientID) == Team)
				m_Snap.m_paInfoByDDTeam[Index++] = m_Snap.m_paInfoByScore[i];
		}
	}

	CTuningParams StandardTuning;
	CServerInfo CurrentServerInfo;
	Client()->GetServerInfo(&CurrentServerInfo);
	if(CurrentServerInfo.m_aGameType[0] != '0')
	{
		if(str_comp(CurrentServerInfo.m_aGameType, "DM") != 0 && str_comp(CurrentServerInfo.m_aGameType, "TDM") != 0 && str_comp(CurrentServerInfo.m_aGameType, "CTF") != 0)
			m_ServerMode = SERVERMODE_MOD;
		else if(mem_comp(&StandardTuning, &m_Tuning[g_Config.m_ClDummy], 33) == 0)
			m_ServerMode = SERVERMODE_PURE;
		else
			m_ServerMode = SERVERMODE_PUREMOD;
	}

	// add tuning to demo
	bool AnyRecording = false;
	for(int i = 0; i < RECORDER_MAX; i++)
		if(DemoRecorder(i)->IsRecording())
		{
			AnyRecording = true;
			break;
		}
	if(AnyRecording && mem_comp(&StandardTuning, &m_Tuning[g_Config.m_ClDummy], sizeof(CTuningParams)) != 0)
	{
		CMsgPacker Msg(NETMSGTYPE_SV_TUNEPARAMS);
		int *pParams = (int *)&m_Tuning[g_Config.m_ClDummy];
		for(unsigned i = 0; i < sizeof(m_Tuning[0])/sizeof(int); i++)
			Msg.AddInt(pParams[i]);
		Client()->SendMsg(&Msg, MSGFLAG_RECORD|MSGFLAG_NOSEND);
	}

	if(!m_DDRaceMsgSent[0] && m_Snap.m_pLocalInfo)
	{
		CMsgPacker Msg(NETMSGTYPE_CL_ISDDNET);
		Msg.AddInt(CLIENT_VERSIONNR);
		Client()->SendMsgExY(&Msg, MSGFLAG_VITAL,false, 0);
		m_DDRaceMsgSent[0] = true;
	}

	if(!m_DDRaceMsgSent[1] && m_Snap.m_pLocalInfo && Client()->DummyConnected())
	{
		CMsgPacker Msg(NETMSGTYPE_CL_ISDDNET);
		Msg.AddInt(CLIENT_VERSIONNR);
		Client()->SendMsgExY(&Msg, MSGFLAG_VITAL,false, 1);
		m_DDRaceMsgSent[1] = true;
	}

	if(m_ShowOthers[g_Config.m_ClDummy] == -1 || (m_ShowOthers[g_Config.m_ClDummy] != -1 && m_ShowOthers[g_Config.m_ClDummy] != g_Config.m_ClShowOthers))
	{
		// no need to send, default settings
		//if(!(m_ShowOthers == -1 && g_Config.m_ClShowOthers))
		{
			CNetMsg_Cl_ShowOthers Msg;
			Msg.m_Show = g_Config.m_ClShowOthers;
			Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
		}

		// update state
		m_ShowOthers[g_Config.m_ClDummy] = g_Config.m_ClShowOthers;
	}
}

void CGameClient::OnPredict()
{

	// store the previous values so we can detect prediction errors
	CCharacterCore BeforePrevChar = m_PredictedPrevChar;
	CCharacterCore BeforeChar = m_PredictedChar;

	// we can't predict without our own id or own character
	if(m_Snap.m_LocalClientID == -1 || !m_Snap.m_aCharacters[m_Snap.m_LocalClientID].m_Active)
		return;

	// don't predict anything if we are paused
	if(m_Snap.m_pGameInfoObj && m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_PAUSED)
	{
		if(m_Snap.m_pLocalCharacter)
		{
			m_PredictedChar.Read(m_Snap.m_pLocalCharacter);
			m_PredictedChar.m_ActiveWeapon = m_Snap.m_pLocalCharacter->m_Weapon;
		}
		if(m_Snap.m_pLocalPrevCharacter)
		{
			m_PredictedPrevChar.Read(m_Snap.m_pLocalPrevCharacter);
			m_PredictedPrevChar.m_ActiveWeapon = m_Snap.m_pLocalPrevCharacter->m_Weapon;
		}
		return;
	}

	static bool IsWeaker[2][MAX_CLIENTS] = {{0}};
	if(AntiPingPlayers())
		FindWeaker(IsWeaker);

	// repredict character
	CWorldCore World;
	World.m_Tuning[g_Config.m_ClDummy] = m_Tuning[g_Config.m_ClDummy];

	// search for players
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!m_Snap.m_aCharacters[i].m_Active || !m_Snap.m_paPlayerInfos[i])
			continue;

		g_GameClient.m_aClients[i].m_Predicted.Init(&World, Collision(), &m_Teams);
		World.m_apCharacters[i] = &g_GameClient.m_aClients[i].m_Predicted;
		World.m_apCharacters[i]->m_Id = m_Snap.m_paPlayerInfos[i]->m_ClientID;
		g_GameClient.m_aClients[i].m_Predicted.Read(&m_Snap.m_aCharacters[i].m_Cur);
		g_GameClient.m_aClients[i].m_Predicted.m_ActiveWeapon = m_Snap.m_aCharacters[i].m_Cur.m_Weapon;
	}

	// teh suckers will never get us down! KATZE!
	static bool WarnedCrash = false;
	if(!World.m_apCharacters[m_Snap.m_LocalClientID])
	{
		if(!WarnedCrash)
		{
			WarnedCrash = true;
			for(int n = 0; n < 5; n++)
				Console()->Print(0, "+++ WARNING +++", "!!! The server just tried to crash your client, I recommend fcking them hard !!!", true);
			Console()->Print(0, "+++ WARNING +++", "!!! --> Things are messed up from now on, please reconnect or turn off prediction <-- !!!", true);
		}
		return;

	}

	CServerInfo Info;
	Client()->GetServerInfo(&Info);
	const int MaxProjectiles = 128;
	class CLocalProjectile PredictedProjectiles[MaxProjectiles];
	int NumProjectiles = 0;
	int ReloadTimer = 0;
	vec2 PrevPos;

	if(AntiPingWeapons())
	{
		for(int Index = 0; Index < MaxProjectiles; Index++)
			PredictedProjectiles[Index].Deactivate();

		int Num = Client()->SnapNumItems(IClient::SNAP_CURRENT);
		for(int Index = 0; Index < Num && NumProjectiles < MaxProjectiles; Index++)
		{
			IClient::CSnapItem Item;
			const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, Index, &Item);
			if(Item.m_Type == NETOBJTYPE_PROJECTILE)
			{
				CNetObj_Projectile* pProj = (CNetObj_Projectile*) pData;
				if(pProj->m_WeaponType == WEAPON_GRENADE || (pProj->m_WeaponType == WEAPON_SHOTGUN && UseExtraInfo(pProj)))
				{
					CLocalProjectile NewProj;
					NewProj.Init(this, &World, Collision(), pProj);
					if(fabs(1.0f - length(NewProj.m_Direction)) < 0.015)
					{
						if(!NewProj.m_ExtraInfo)
						{
							if(CWeaponData *pData = FindWeaponData(NewProj.m_StartTick))
							{
								NewProj.m_Pos = pData->StartPos();
								NewProj.m_Direction = pData->m_Direction;
								NewProj.m_Owner = m_Snap.m_LocalClientID;
							}
						}
						PredictedProjectiles[NumProjectiles] = NewProj;
						NumProjectiles++;
					}
				}
			}
		}

		int AttackTick = m_Snap.m_aCharacters[m_Snap.m_LocalClientID].m_Cur.m_AttackTick;
		if(World.m_apCharacters[m_Snap.m_LocalClientID]->m_ActiveWeapon == WEAPON_HAMMER)
		{
			CWeaponData *pWeaponData = GetWeaponData(AttackTick);
			if(pWeaponData && pWeaponData->m_Tick == AttackTick)
				ReloadTimer = SERVER_TICK_SPEED / 3 - (Client()->GameTick() - AttackTick);
			else
				ReloadTimer = 0;
		}
		else
			ReloadTimer = g_pData->m_Weapons.m_aId[World.m_apCharacters[m_Snap.m_LocalClientID]->m_ActiveWeapon].m_Firedelay * SERVER_TICK_SPEED / 1000 - (Client()->GameTick() - AttackTick);
		ReloadTimer = max(ReloadTimer, 0);
	}

	// predict
	for(int Tick = Client()->GameTick()+1; Tick <= Client()->PredGameTick(); Tick++)
	{
		// fetch the local
		if(Tick == Client()->PredGameTick() && World.m_apCharacters[m_Snap.m_LocalClientID])
			m_PredictedPrevChar = *World.m_apCharacters[m_Snap.m_LocalClientID];

		for(int c = 0; c < MAX_CLIENTS; c++)
		{
			if(!World.m_apCharacters[c])
				continue;

			if(AntiPingPlayers() && Tick == Client()->PredGameTick())
				g_GameClient.m_aClients[c].m_PrevPredicted = *World.m_apCharacters[c];
		}

		// input
		for(int c = 0; c < MAX_CLIENTS; c++)
		{
			if(!World.m_apCharacters[c])
				continue;

			mem_zero(&World.m_apCharacters[c]->m_Input, sizeof(World.m_apCharacters[c]->m_Input));
			if(m_Snap.m_LocalClientID == c)
			{
				// apply player input
				int *pInput = Client()->GetInput(Tick);
				if(pInput)
					World.m_apCharacters[c]->m_Input = *((CNetObj_PlayerInput*)pInput);
			}
		}

		if(AntiPingWeapons())
		{
			const float ProximityRadius = 28.0f;
			CNetObj_PlayerInput Input;
			CNetObj_PlayerInput PrevInput;
			mem_zero(&Input, sizeof(Input));
			mem_zero(&PrevInput, sizeof(PrevInput));
			int *pInput = Client()->GetInput(Tick);
			if(pInput)
				Input = *((CNetObj_PlayerInput*)pInput);
			int *pPrevInput = Client()->GetInput(Tick-1);
			if(pPrevInput)
				PrevInput = *((CNetObj_PlayerInput*)pPrevInput);

			CCharacterCore *Local = World.m_apCharacters[m_Snap.m_LocalClientID];
			vec2 Direction = normalize(vec2(Input.m_TargetX, Input.m_TargetY));
			vec2 Pos = Local->m_Pos;
			vec2 ProjStartPos = Pos + Direction * ProximityRadius * 0.75f;

			bool WeaponFired = false;
			bool NewPresses = false;
			// handle weapons
			do
			{
				if(ReloadTimer)
					break;
				if(!World.m_apCharacters[m_Snap.m_LocalClientID])
					break;
				if(!pInput || !pPrevInput)
					break;

				bool FullAuto = false;
				if(Local->m_ActiveWeapon == WEAPON_GRENADE || Local->m_ActiveWeapon == WEAPON_SHOTGUN || Local->m_ActiveWeapon == WEAPON_RIFLE)
					FullAuto = true;

				bool WillFire = false;

				if(CountInput(PrevInput.m_Fire, Input.m_Fire).m_Presses)
				{
					WillFire = true;
					NewPresses = true;
				}
				if(FullAuto && (Input.m_Fire&1))
					WillFire = true;
				if(!WillFire)
					break;
				if(!IsRace(&Info) && !m_Snap.m_pLocalCharacter->m_AmmoCount && Local->m_ActiveWeapon != WEAPON_HAMMER)
					break;

				int ExpectedStartTick = Tick-1;
				ReloadTimer = g_pData->m_Weapons.m_aId[Local->m_ActiveWeapon].m_Firedelay * SERVER_TICK_SPEED / 1000;

				bool DirectInput = Client()->InputExists(Tick);
				if(!DirectInput)
				{
					ReloadTimer++;
					ExpectedStartTick++;
				}

				switch(Local->m_ActiveWeapon)
				{
					case WEAPON_RIFLE:
					case WEAPON_SHOTGUN:
					case WEAPON_GUN:
						{
							WeaponFired = true;
						} break;
					case WEAPON_GRENADE:
						{
							if(NumProjectiles >= MaxProjectiles)
								break;
							PredictedProjectiles[NumProjectiles].Init(
									this, &World, Collision(),
									Direction, //StartDir
									ProjStartPos, //StartPos
									ExpectedStartTick, //StartTick
									WEAPON_GRENADE, //Type
									m_Snap.m_LocalClientID, //Owner
									WEAPON_GRENADE, //Weapon
									1, 0, 0, 1); //Explosive, Bouncing, Freeze, ExtraInfo
							NumProjectiles++;
							WeaponFired = true;
						} break;
					case WEAPON_HAMMER:
						{
							vec2 ProjPos = ProjStartPos;
							float Radius = ProximityRadius*0.5f;

							int Hits = 0;
							bool OwnerCanProbablyHitOthers = (m_Tuning[g_Config.m_ClDummy].m_PlayerCollision || m_Tuning[g_Config.m_ClDummy].m_PlayerHooking);
							if(!OwnerCanProbablyHitOthers)
								break;
							for(int i = 0; i < MAX_CLIENTS; i++)
							{
								if(!World.m_apCharacters[i])
									continue;
								if(i == m_Snap.m_LocalClientID)
									continue;
								if(!(distance(World.m_apCharacters[i]->m_Pos, ProjPos) < Radius+ProximityRadius))
									continue;

								CCharacterCore *pTarget = World.m_apCharacters[i];

								if(m_aClients[i].m_Active && !m_Teams.CanCollide(i, m_Snap.m_LocalClientID))
									continue;

								vec2 Dir;
								if (length(pTarget->m_Pos - Pos) > 0.0f)
									Dir = normalize(pTarget->m_Pos - Pos);
								else
									Dir = vec2(0.f, -1.f);

								float Strength;
								Strength = World.m_Tuning[g_Config.m_ClDummy].m_HammerStrength;

								vec2 Temp = pTarget->m_Vel + normalize(Dir + vec2(0.f, -1.1f)) * 10.0f;

								pTarget->LimitForce(&Temp);

								Temp -= pTarget->m_Vel;
								pTarget->ApplyForce((vec2(0.f, -1.0f) + Temp) * Strength);
								Hits++;
							}
							// if we Hit anything, we have to wait for the reload
							if(Hits)
							{
								ReloadTimer = SERVER_TICK_SPEED/3;
								WeaponFired = true;
							}
						} break;
				}
				if(!ReloadTimer)
				{
					ReloadTimer = g_pData->m_Weapons.m_aId[Local->m_ActiveWeapon].m_Firedelay * SERVER_TICK_SPEED / 1000;
					if(!DirectInput)
						ReloadTimer++;
				}
			} while(false);

			if(ReloadTimer)
				ReloadTimer--;

			if(Tick > Client()->GameTick()+1)
				if(CWeaponData *pWeaponData = GetWeaponData(Tick-1))
					pWeaponData->m_Pos = Pos;
			if(WeaponFired)
			{
				if(CWeaponData *pWeaponData = GetWeaponData(Tick-1))
				{
					pWeaponData->m_Direction = Direction;
					pWeaponData->m_Tick = Tick-1;
				}
				if(NewPresses)
				{
					if(CWeaponData *pWeaponData = GetWeaponData(Tick-2))
					{
						pWeaponData->m_Direction = Direction;
						pWeaponData->m_Tick = Tick-2;
					}
					if(CWeaponData *pWeaponData = GetWeaponData(Tick))
					{
						pWeaponData->m_Direction = Direction;
						pWeaponData->m_Tick = Tick;
					}
				}
			}

			// projectiles
			for(int g = 0; g < MaxProjectiles; g++)
				if(PredictedProjectiles[g].m_Active)
					PredictedProjectiles[g].Tick(Tick, Client()->GameTickSpeed(), m_Snap.m_LocalClientID);
		}

		// calculate where everyone should move
		if(AntiPingPlayers())
		{
			//first apply Tick to weaker players (players that the local client has strong hook against), then local, then stronger players
			for(int h = 0; h < 3; h++)
			{
				if(h == 1)
				{
					if(World.m_apCharacters[m_Snap.m_LocalClientID])
						World.m_apCharacters[m_Snap.m_LocalClientID]->Tick(true, true);
				}
				else
					for(int c = 0; c < MAX_CLIENTS; c++)
						if(c != m_Snap.m_LocalClientID && World.m_apCharacters[c] && ((h == 0 && IsWeaker[g_Config.m_ClDummy][c]) || (h == 2 && !IsWeaker[g_Config.m_ClDummy][c])))
							World.m_apCharacters[c]->Tick(false, true);
			}
		}
		else
		{
			for(int c = 0; c < MAX_CLIENTS; c++)
			{
				if(!World.m_apCharacters[c])
					continue;
				if(m_Snap.m_LocalClientID == c)
					World.m_apCharacters[c]->Tick(true, true);
				else
					World.m_apCharacters[c]->Tick(false, true);
			}
		}

		// move all players and quantize their data
		if(AntiPingPlayers())
		{
			// Everyone with weaker hook
			for(int c = 0; c < MAX_CLIENTS; c++)
			{
				if(c != m_Snap.m_LocalClientID && World.m_apCharacters[c] && IsWeaker[g_Config.m_ClDummy][c])
				{
					World.m_apCharacters[c]->Move();
					World.m_apCharacters[c]->Quantize();
				}
			}

			// Us
			if(World.m_apCharacters[m_Snap.m_LocalClientID])
			{
				World.m_apCharacters[m_Snap.m_LocalClientID]->Move();
				World.m_apCharacters[m_Snap.m_LocalClientID]->Quantize();
			}

			// Everyone with stronger hook
			for(int c = 0; c < MAX_CLIENTS; c++)
			{
				if(c != m_Snap.m_LocalClientID && World.m_apCharacters[c] && !IsWeaker[g_Config.m_ClDummy][c])
				{
					World.m_apCharacters[c]->Move();
					World.m_apCharacters[c]->Quantize();
				}
			}
		}
		else
		{
			for(int c = 0; c < MAX_CLIENTS; c++)
			{
				if(!World.m_apCharacters[c])
					continue;
				World.m_apCharacters[c]->Move();
				World.m_apCharacters[c]->Quantize();
			}
		}

		// check if we want to trigger effects
		if(Tick > m_LastNewPredictedTick[g_Config.m_ClDummy])
		{
			m_LastNewPredictedTick[g_Config.m_ClDummy] = Tick;
			m_NewPredictedTick = true;

			if(m_Snap.m_LocalClientID != -1 && World.m_apCharacters[m_Snap.m_LocalClientID])
			{
				vec2 Pos = World.m_apCharacters[m_Snap.m_LocalClientID]->m_Pos;
				int Events = World.m_apCharacters[m_Snap.m_LocalClientID]->m_TriggeredEvents;
				if(Events&COREEVENT_GROUND_JUMP)
					if(g_Config.m_SndGame)
						g_GameClient.m_pSounds->PlayAndRecord(CSounds::CHN_WORLD, SOUND_PLAYER_JUMP, 1.0f, Pos);

				/*if(events&COREEVENT_AIR_JUMP)
				{
					GameClient.effects->air_jump(pos);
					GameClient.sounds->play_and_record(SOUNDS::CHN_WORLD, SOUND_PLAYER_AIRJUMP, 1.0f, pos);
				}*/

				//if(events&COREEVENT_HOOK_LAUNCH) snd_play_random(CHN_WORLD, SOUND_HOOK_LOOP, 1.0f, pos);
				//if(events&COREEVENT_HOOK_ATTACH_PLAYER) snd_play_random(CHN_WORLD, SOUND_HOOK_ATTACH_PLAYER, 1.0f, pos);
				if(Events&COREEVENT_HOOK_ATTACH_GROUND)
					if(g_Config.m_SndGame)
						g_GameClient.m_pSounds->PlayAndRecord(CSounds::CHN_WORLD, SOUND_HOOK_ATTACH_GROUND, 1.0f, Pos);
				if(Events&COREEVENT_HOOK_HIT_NOHOOK)
					if(g_Config.m_SndGame)
						g_GameClient.m_pSounds->PlayAndRecord(CSounds::CHN_WORLD, SOUND_HOOK_NOATTACH, 1.0f, Pos);
				//if(events&COREEVENT_HOOK_RETRACT) snd_play_random(CHN_WORLD, SOUND_PLAYER_JUMP, 1.0f, pos);
			}
		}


		if(Tick == Client()->PredGameTick() && World.m_apCharacters[m_Snap.m_LocalClientID])
		{
			m_PredictedChar = *World.m_apCharacters[m_Snap.m_LocalClientID];

			if (AntiPingPlayers())
			{
				for (int c = 0; c < MAX_CLIENTS; c++)
				{
					if(!World.m_apCharacters[c])
						continue;

					g_GameClient.m_aClients[c].m_Predicted = *World.m_apCharacters[c];
				}
			}
		}
	}

	if(g_Config.m_Debug && g_Config.m_ClPredict && m_PredictedTick == Client()->PredGameTick())
	{
		CNetObj_CharacterCore Before = {0}, Now = {0}, BeforePrev = {0}, NowPrev = {0};
		BeforeChar.Write(&Before);
		BeforePrevChar.Write(&BeforePrev);
		m_PredictedChar.Write(&Now);
		m_PredictedPrevChar.Write(&NowPrev);

		if(mem_comp(&Before, &Now, sizeof(CNetObj_CharacterCore)) != 0)
		{
			Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", "prediction error");
			for(unsigned i = 0; i < sizeof(CNetObj_CharacterCore)/sizeof(int); i++)
				if(((int *)&Before)[i] != ((int *)&Now)[i])
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "	%d %d %d (%d %d)", i, ((int *)&Before)[i], ((int *)&Now)[i], ((int *)&BeforePrev)[i], ((int *)&NowPrev)[i]);
					Console()->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", aBuf);
				}
		}
	}

	m_PredictedTick = Client()->PredGameTick();
}

void CGameClient::OnActivateEditor()
{
	OnRelease();
}

CGameClient::CClientStats::CClientStats()
{
	m_JoinDate = 0;
	m_Active = false;
	m_WasActive = false;
	m_Frags = 0;
	m_Deaths = 0;
	m_Suicides = 0;
	for(int j = 0; j < NUM_WEAPONS; j++)
	{
		m_aFragsWith[j] = 0;
		m_aDeathsFrom[j] = 0;
	}
	m_FlagGrabs = 0;
	m_FlagCaptures = 0;
}

void CGameClient::CClientStats::Reset()
{
	m_JoinDate = 0;
	m_Active = false;
	m_WasActive = false;
	m_Frags = 0;
	m_Deaths = 0;
	m_Suicides = 0;
	m_BestSpree = 0;
	m_CurrentSpree = 0;
	for(int j = 0; j < NUM_WEAPONS; j++)
	{
		m_aFragsWith[j] = 0;
		m_aDeathsFrom[j] = 0;
	}
	m_FlagGrabs = 0;
	m_FlagCaptures = 0;
}

void CGameClient::CClientData::UpdateRenderInfo()
{
	m_RenderInfo = m_SkinInfo;

	// force team colors
	if(g_GameClient.m_Snap.m_pGameInfoObj && g_GameClient.m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
	{
		m_RenderInfo.m_Texture = g_GameClient.m_pSkins->Get(m_SkinID)->m_ColorTexture;
		const int TeamColors[2] = {65387, 10223467};
		if(m_Team >= TEAM_RED && m_Team <= TEAM_BLUE)
		{
			m_RenderInfo.m_ColorBody = g_GameClient.m_pSkins->GetColorV4(TeamColors[m_Team]);
			m_RenderInfo.m_ColorFeet = g_GameClient.m_pSkins->GetColorV4(TeamColors[m_Team]);
		}
		else
		{
			m_RenderInfo.m_ColorBody = g_GameClient.m_pSkins->GetColorV4(12895054);
			m_RenderInfo.m_ColorFeet = g_GameClient.m_pSkins->GetColorV4(12895054);
		}
	}
}

void CGameClient::CClientData::Reset()
{
	m_aName[0] = 0;
	m_aClan[0] = 0;
	m_Country = -1;
	m_SkinID = 0;
	m_Team = 0;
	m_Angle = 0;
	m_Emoticon = 0;
	m_EmoticonStart = -1;
	m_Active = false;
	m_ChatIgnore = false;
	m_SkinInfo.m_Texture = g_GameClient.m_pSkins->Get(0)->m_ColorTexture;
	m_SkinInfo.m_ColorBody = vec4(1,1,1,1);
	m_SkinInfo.m_ColorFeet = vec4(1,1,1,1);

	// haxx
	m_Addr[0] = 0;
	m_Spoofable = false;
	UpdateRenderInfo();
}

void CGameClient::SendSwitchTeam(int Team)
{
	CNetMsg_Cl_SetTeam Msg;
	Msg.m_Team = Team;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);

	if (Team != TEAM_SPECTATORS)
		m_pCamera->OnReset();
}

void CGameClient::SendInfo(bool Start)
{
	if(Start)
	{
		CNetMsg_Cl_StartInfo Msg;
		Msg.m_pName = g_Config.m_PlayerName;
		Msg.m_pClan = g_Config.m_PlayerClan;
		Msg.m_Country = g_Config.m_PlayerCountry;
		Msg.m_pSkin = g_Config.m_ClPlayerSkin;
		Msg.m_UseCustomColor = g_Config.m_ClPlayerUseCustomColor;
		Msg.m_ColorBody = g_Config.m_ClPlayerColorBody;
		Msg.m_ColorFeet = g_Config.m_ClPlayerColorFeet;
		CMsgPacker Packer(Msg.MsgID());
		Msg.Pack(&Packer);
		Client()->SendMsgExY(&Packer, MSGFLAG_VITAL, false, 0);
		m_CheckInfo[0] = -1;
	}
	else
	{
		CNetMsg_Cl_ChangeInfo Msg;
		Msg.m_pName = g_Config.m_PlayerName;
		Msg.m_pClan = g_Config.m_PlayerClan;
		Msg.m_Country = g_Config.m_PlayerCountry;
		Msg.m_pSkin = g_Config.m_ClPlayerSkin;
		Msg.m_UseCustomColor = g_Config.m_ClPlayerUseCustomColor;
		Msg.m_ColorBody = g_Config.m_ClPlayerColorBody;
		Msg.m_ColorFeet = g_Config.m_ClPlayerColorFeet;
		CMsgPacker Packer(Msg.MsgID());
		Msg.Pack(&Packer);
		Client()->SendMsgExY(&Packer, MSGFLAG_VITAL, false, 0);
		m_CheckInfo[0] = Client()->GameTickSpeed();
	}
}

void CGameClient::SendDummyInfo(bool Start)
{
	if(Start)
	{
		CNetMsg_Cl_StartInfo Msg;
		Msg.m_pName = g_Config.m_ClDummyName;
		Msg.m_pClan = g_Config.m_ClDummyClan;
		Msg.m_Country = g_Config.m_ClDummyCountry;
		Msg.m_pSkin = g_Config.m_ClDummySkin;
		Msg.m_UseCustomColor = g_Config.m_ClDummyUseCustomColor;
		Msg.m_ColorBody = g_Config.m_ClDummyColorBody;
		Msg.m_ColorFeet = g_Config.m_ClDummyColorFeet;
		CMsgPacker Packer(Msg.MsgID());
		Msg.Pack(&Packer);
		Client()->SendMsgExY(&Packer, MSGFLAG_VITAL, false, 1);
		m_CheckInfo[1] = -1;
	}
	else
	{
		CNetMsg_Cl_ChangeInfo Msg;
		Msg.m_pName = g_Config.m_ClDummyName;
		Msg.m_pClan = g_Config.m_ClDummyClan;
		Msg.m_Country = g_Config.m_ClDummyCountry;
		Msg.m_pSkin = g_Config.m_ClDummySkin;
		Msg.m_UseCustomColor = g_Config.m_ClDummyUseCustomColor;
		Msg.m_ColorBody = g_Config.m_ClDummyColorBody;
		Msg.m_ColorFeet = g_Config.m_ClDummyColorFeet;
		CMsgPacker Packer(Msg.MsgID());
		Msg.Pack(&Packer);
		Client()->SendMsgExY(&Packer, MSGFLAG_VITAL,false, 1);
		m_CheckInfo[1] = Client()->GameTickSpeed();
	}
}

void CGameClient::SendKill(int ClientID)
{
	CNetMsg_Cl_Kill Msg;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);

	if(g_Config.m_ClDummyCopyMoves)
	{
		CMsgPacker Msg(NETMSGTYPE_CL_KILL);
		Client()->SendMsgExY(&Msg, MSGFLAG_VITAL, false, !g_Config.m_ClDummy);
	}
}

void CGameClient::ConTeam(IConsole::IResult *pResult, void *pUserData)
{
	((CGameClient*)pUserData)->SendSwitchTeam(pResult->GetInteger(0));
}

void CGameClient::ConKill(IConsole::IResult *pResult, void *pUserData)
{
	((CGameClient*)pUserData)->SendKill(-1);
}

void CGameClient::ConLuafile(IConsole::IResult *pResult, void *pUserData)
{
	if(pResult->NumArguments() < 2)
		return;

	CGameClient* pSelf = (CGameClient*)pUserData;

	for(int n = 1; n < pResult->NumArguments(); n++)
	{
		int id = -1;
		for(int i = 0; i < pSelf->Client()->Lua()->GetLuaFiles().size(); i++)
		{
			if(str_comp_nocase(pSelf->Client()->Lua()->GetLuaFiles()[i]->GetFilename(), pResult->GetString(n)) == 0)
			{
				id = i;
				break;
			}
		}

		if(id == -1)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), Localize("There was no luafile with the name '%s' found!"), pResult->GetString(n));
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "luafile", aBuf, false);
			return;
		}

		if(str_comp_nocase(pResult->GetString(0), "activate") == 0)
		{
			if(pSelf->Client()->Lua()->GetLuaFiles()[id]->State() != CLuaFile::LUAFILE_STATE_LOADED)
				pSelf->Client()->Lua()->GetLuaFiles()[id]->Init();
		}
		else if(str_comp_nocase(pResult->GetString(0), "deactivate") == 0)
		{
			if(pSelf->Client()->Lua()->GetLuaFiles()[id]->State() == CLuaFile::LUAFILE_STATE_LOADED)
				pSelf->Client()->Lua()->GetLuaFiles()[id]->Unload();
		}
		else if(str_comp_nocase(pResult->GetString(0), "toggle") == 0)
		{
			if(pSelf->Client()->Lua()->GetLuaFiles()[id]->State() != CLuaFile::LUAFILE_STATE_LOADED)
				pSelf->Client()->Lua()->GetLuaFiles()[id]->Init();
			else if(pSelf->Client()->Lua()->GetLuaFiles()[id]->State() == CLuaFile::LUAFILE_STATE_LOADED)
				pSelf->Client()->Lua()->GetLuaFiles()[id]->Unload();

			char aBuf[64];
			if(pSelf->Client()->Lua()->GetLuaFiles()[id]->State() == CLuaFile::LUAFILE_STATE_ERROR)
				str_format(aBuf, sizeof(aBuf), "Script '%s' failed to load", pResult->GetString(1));
			else
				str_format(aBuf, sizeof(aBuf), "Toggled script '%s' %s", pResult->GetString(1),
						pSelf->Client()->Lua()->GetLuaFiles()[id]->State() == CLuaFile::LUAFILE_STATE_LOADED ? "on" : "off");
				pSelf->m_pHud->PushNotification(aBuf);
		}
		else
			pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "luafile", Localize("You must give either 'activate', 'deactivate' or 'toggle' as first argument!"), false);
	}
}

void CGameClient::ConchainSpecialInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		((CGameClient*)pUserData)->SendInfo(false);
}

void CGameClient::ConchainSpecialDummyInfoupdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		((CGameClient*)pUserData)->SendDummyInfo(false);
}

void CGameClient::ConchainSpecialDummy(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		if(g_Config.m_ClDummy && !((CGameClient*)pUserData)->Client()->DummyConnected())
			g_Config.m_ClDummy = 0;
}

IGameClient *CreateGameClient()
{
	return &g_GameClient;
}

int CGameClient::IntersectCharacter(vec2 HookPos, vec2 NewPos, vec2& NewPos2, int ownID)
{
	float PhysSize = 28.0f;
	float Distance = 0.0f;
	int ClosestID = -1;

	if (!m_Tuning[g_Config.m_ClDummy].m_PlayerHooking)
		return ClosestID;

	for (int i=0; i<MAX_CLIENTS; i++)
	{
		CClientData cData = m_aClients[i];
		CNetObj_Character Prev = m_Snap.m_aCharacters[i].m_Prev;
		CNetObj_Character Player = m_Snap.m_aCharacters[i].m_Cur;

		vec2 Position = mix(vec2(Prev.m_X, Prev.m_Y), vec2(Player.m_X, Player.m_Y), Client()->IntraGameTick());

		if (!cData.m_Active || i == ownID || !m_Teams.SameTeam(i, ownID))
			continue;

		vec2 ClosestPoint = closest_point_on_line(HookPos, NewPos, Position);
		if(distance(Position, ClosestPoint) < PhysSize+2.0f)
		{
			if(ClosestID == -1 || distance(HookPos, Position) < Distance)
			{
				NewPos2 = ClosestPoint;
				ClosestID = i;
				Distance = distance(HookPos, Position);
			}
		}
	}

	return ClosestID;
}


int CGameClient::IntersectCharacter(vec2 OldPos, vec2 NewPos, float Radius, vec2 *NewPos2, int ownID, CWorldCore *World)
{
	float PhysSize = 28.0f;
	float Distance = 0.0f;
	int ClosestID = -1;

	if(!World)
		return ClosestID;

	for (int i=0; i<MAX_CLIENTS; i++)
	{
		if(!World->m_apCharacters[i])
			continue;
		CClientData cData = m_aClients[i];

		if(!cData.m_Active || i == ownID || !m_Teams.CanCollide(i, ownID))
			continue;
		vec2 Position = World->m_apCharacters[i]->m_Pos;
		vec2 ClosestPoint = closest_point_on_line(OldPos, NewPos, Position);
		if(distance(Position, ClosestPoint) < PhysSize+Radius)
		{
			if(ClosestID == -1 || distance(OldPos, Position) < Distance)
			{
				*NewPos2 = ClosestPoint;
				ClosestID = i;
				Distance = distance(OldPos, Position);
			}
		}
	}
	return ClosestID;
}

void CLocalProjectile::Init(CGameClient *pGameClient, CWorldCore *pWorld, CCollision *pCollision, const CNetObj_Projectile *pProj)
{
	m_Active = 1;
	m_pGameClient = pGameClient;
	m_pWorld = pWorld;
	m_pCollision = pCollision;
	m_StartTick = pProj->m_StartTick;
	m_Type = pProj->m_WeaponType;
	m_Weapon = m_Type;

	ExtractInfo(pProj, &m_Pos, &m_Direction, 1);

	if(UseExtraInfo(pProj))
	{
		ExtractExtraInfo(pProj, &m_Owner, &m_Explosive, &m_Bouncing, &m_Freeze);
		m_ExtraInfo = true;
	}
	else
	{
		bool StandardVel = (fabs(1.0f - length(m_Direction)) < 0.015);
		m_Owner = -1;
		m_Explosive = ((m_Type == WEAPON_GRENADE && StandardVel) ? true : false);
		m_Bouncing = 0;
		m_Freeze = 0;
		m_ExtraInfo = false;
	}
}

void CLocalProjectile::Init(CGameClient *pGameClient, CWorldCore *pWorld, CCollision *pCollision, vec2 Direction, vec2 Pos, int StartTick, int Type, int Owner, int Weapon, bool Explosive, int Bouncing, bool Freeze, bool ExtraInfo)
{
	m_Active = 1;
	m_pGameClient = pGameClient;
	m_pWorld = pWorld;
	m_pCollision = pCollision;
	m_Direction = Direction;
	m_Pos = Pos;
	m_StartTick = StartTick;
	m_Type = Type;
	m_Weapon = Weapon;
	m_Owner = Owner;
	m_Explosive = Explosive;
	m_Bouncing = Bouncing;
	m_Freeze = Freeze;
	m_ExtraInfo = ExtraInfo;
}

vec2 CLocalProjectile::GetPos(float Time)
{
	float Curvature = 0;
	float Speed = 0;

	switch(m_Type)
	{
		case WEAPON_GRENADE:
			Curvature = m_pWorld->m_Tuning[g_Config.m_ClDummy].m_GrenadeCurvature;
			Speed = m_pWorld->m_Tuning[g_Config.m_ClDummy].m_GrenadeSpeed;
			break;

		case WEAPON_SHOTGUN:
			Curvature = m_pWorld->m_Tuning[g_Config.m_ClDummy].m_ShotgunCurvature;
			Speed = m_pWorld->m_Tuning[g_Config.m_ClDummy].m_ShotgunSpeed;
			break;

		case WEAPON_GUN:
			Curvature = m_pWorld->m_Tuning[g_Config.m_ClDummy].m_GunCurvature;
			Speed = m_pWorld->m_Tuning[g_Config.m_ClDummy].m_GunSpeed;
			break;
	}
	return CalcPos(m_Pos, m_Direction, Curvature, Speed, Time);
}

bool CLocalProjectile::GameLayerClipped(vec2 CheckPos)
{
	return round_to_int(CheckPos.x)/32 < -200 || round_to_int(CheckPos.x)/32 > m_pCollision->GetWidth()+200 ||
		round_to_int(CheckPos.y)/32 < -200 || round_to_int(CheckPos.y)/32 > m_pCollision->GetHeight()+200 ? true : false;
}

void CLocalProjectile::Tick(int CurrentTick, int GameTickSpeed, int LocalClientID)
{
	if(!m_pWorld)
		return;
	if(CurrentTick <= m_StartTick)
		return;
	float Pt = (CurrentTick-m_StartTick-1)/(float)GameTickSpeed;
	float Ct = (CurrentTick-m_StartTick)/(float)GameTickSpeed;

	vec2 PrevPos = GetPos(Pt);
	vec2 CurPos = GetPos(Ct);
	vec2 ColPos;
	vec2 NewPos;
	int Collide = 0;
	if(m_pCollision)
		Collide = m_pCollision->IntersectLine(PrevPos, CurPos, &ColPos, &NewPos);
	int Target = m_pGameClient->IntersectCharacter(PrevPos, ColPos, m_Freeze ? 1.0f : 6.0f, &ColPos, m_Owner, m_pWorld);

	bool isWeaponCollide = false;
	if(m_Owner >= 0 && Target >= 0 && m_pGameClient->m_aClients[m_Owner].m_Active && m_pGameClient->m_aClients[Target].m_Active && !m_pGameClient->m_Teams.CanCollide(m_Owner, Target))
		isWeaponCollide = true;

	bool OwnerCanProbablyHitOthers = (m_pWorld->m_Tuning[g_Config.m_ClDummy].m_PlayerCollision || m_pWorld->m_Tuning[g_Config.m_ClDummy].m_PlayerHooking);

	if(((Target >= 0 && (m_Owner >= 0 ? OwnerCanProbablyHitOthers : 1 || Target == m_Owner)) || Collide || GameLayerClipped(CurPos)) && !isWeaponCollide)
	{
		if(m_Explosive && (Target < 0 || (Target >= 0 && (!m_Freeze || (m_Weapon == WEAPON_SHOTGUN && Collide)))))
			CreateExplosion(ColPos, m_Owner);
		if(Collide && m_Bouncing != 0)
		{
			m_StartTick = CurrentTick;
			m_Pos = NewPos+(m_Direction*(-4));
			if (m_Bouncing == 1)
				m_Direction.x = -m_Direction.x;
			else if(m_Bouncing == 2)
				m_Direction.y = -m_Direction.y;
			if (fabs(m_Direction.x) < 1e-6)
				m_Direction.x = 0;
			if (fabs(m_Direction.y) < 1e-6)
				m_Direction.y = 0;
			m_Pos += m_Direction;
		}
		else if(!m_Bouncing)
			Deactivate();
	}

	if(!m_Bouncing)
	{
		int Lifetime = 0;
		if(m_Weapon == WEAPON_GRENADE)
			Lifetime = m_pGameClient->m_Tuning[g_Config.m_ClDummy].m_GrenadeLifetime * SERVER_TICK_SPEED;
		else if(m_Weapon == WEAPON_GUN)
			Lifetime = m_pGameClient->m_Tuning[g_Config.m_ClDummy].m_GrenadeLifetime * SERVER_TICK_SPEED;
		else if(m_Weapon == WEAPON_SHOTGUN)
			Lifetime = m_pGameClient->m_Tuning[g_Config.m_ClDummy].m_ShotgunLifetime * SERVER_TICK_SPEED;
		int LifeSpan = Lifetime - (CurrentTick - m_StartTick);
		if(LifeSpan == -1)
		{
			if(m_Explosive)
				CreateExplosion(ColPos, LocalClientID);
			Deactivate();
		}
	}
}

void CLocalProjectile::CreateExplosion(vec2 Pos, int LocalClientID)
{
	if(!m_pWorld)
		return;
	float Radius = 135.0f;
	float InnerRadius = 48.0f;

	bool OwnerCanProbablyHitOthers = (m_pWorld->m_Tuning[g_Config.m_ClDummy].m_PlayerCollision || m_pWorld->m_Tuning[g_Config.m_ClDummy].m_PlayerHooking);

	for(int c = 0; c < MAX_CLIENTS; c++)
	{
		if(!m_pWorld->m_apCharacters[c])
			continue;
		if(m_Owner >= 0 && c >= 0)
			if(m_pGameClient->m_aClients[c].m_Active && !m_pGameClient->m_Teams.CanCollide(c, m_Owner))
				continue;
		if(c != LocalClientID && !OwnerCanProbablyHitOthers)
			continue;
		vec2 Diff = m_pWorld->m_apCharacters[c]->m_Pos - Pos;
		vec2 ForceDir(0,1);
		float l = length(Diff);
		if(l)
			ForceDir = normalize(Diff);
		l = 1-clamp((l-InnerRadius)/(Radius-InnerRadius), 0.0f, 1.0f);

		float Strength = m_pWorld->m_Tuning[g_Config.m_ClDummy].m_ExplosionStrength;
		float Dmg = Strength * l;

		if((int)Dmg)
			m_pWorld->m_apCharacters[c]->ApplyForce(ForceDir*Dmg*2);
	}
}

CWeaponData *CGameClient::FindWeaponData(int TargetTick)
{
	CWeaponData *pData;
	int TickDiff[3] = {0, -1, 1};
	for(unsigned int i = 0; i < sizeof(TickDiff)/sizeof(int); i++)
		if((pData = GetWeaponData(TargetTick + TickDiff[i])))
			if(pData->m_Tick == TargetTick + TickDiff[i])
				return GetWeaponData(TargetTick + TickDiff[i]);
	return NULL;
}

void CGameClient::FindWeaker(bool IsWeaker[2][MAX_CLIENTS])
{
	// attempts to detect strong/weak against the player we are hooking
	static int DirAccumulated[2][MAX_CLIENTS] = {{0}};
	if(!m_Snap.m_aCharacters[m_Snap.m_LocalClientID].m_Active || !m_Snap.m_paPlayerInfos[m_Snap.m_LocalClientID])
		return;
	int HookedPlayer = m_Snap.m_aCharacters[m_Snap.m_LocalClientID].m_Prev.m_HookedPlayer;
	if(HookedPlayer >= 0 && m_Snap.m_aCharacters[HookedPlayer].m_Active && m_Snap.m_paPlayerInfos[HookedPlayer])
	{
		CCharacterCore OtherCharCur;
		OtherCharCur.Read(&m_Snap.m_aCharacters[HookedPlayer].m_Cur);
		float PredictErr[2];
		for(int dir = 0; dir < 2; dir++)
		{
			CWorldCore World;
			World.m_Tuning[g_Config.m_ClDummy] = m_Tuning[g_Config.m_ClDummy];

			CCharacterCore OtherChar;
			OtherChar.Init(&World, Collision(), &m_Teams);
			World.m_apCharacters[HookedPlayer] = &OtherChar;
			OtherChar.Read(&m_Snap.m_aCharacters[HookedPlayer].m_Prev);

			CCharacterCore LocalChar;
			LocalChar.Init(&World, Collision(), &m_Teams);
			World.m_apCharacters[m_Snap.m_LocalClientID] = &LocalChar;
			LocalChar.Read(&m_Snap.m_aCharacters[m_Snap.m_LocalClientID].m_Prev);

			for(int Tick = Client()->PrevGameTick(); Tick < Client()->GameTick(); Tick++)
			{
				if(dir == 0)
				{
					LocalChar.Tick(false, true);
					OtherChar.Tick(false, true);
				}
				else
				{
					OtherChar.Tick(false, true);
					LocalChar.Tick(false, true);
				}
				LocalChar.Move();
				LocalChar.Quantize();
				OtherChar.Move();
				OtherChar.Quantize();
			}
			PredictErr[dir] = distance(OtherChar.m_Vel, OtherCharCur.m_Vel);
		}
		const float Low = 0.0001, High = 0.07;
		if(PredictErr[1] < Low && PredictErr[0] > High)
			DirAccumulated[g_Config.m_ClDummy][HookedPlayer] = SaturatedAdd(-1, 2, DirAccumulated[g_Config.m_ClDummy][HookedPlayer], 1);
		else if(PredictErr[0] < Low && PredictErr[1] > High)
			DirAccumulated[g_Config.m_ClDummy][HookedPlayer] = SaturatedAdd(-1, 2, DirAccumulated[g_Config.m_ClDummy][HookedPlayer], -1);
		IsWeaker[g_Config.m_ClDummy][HookedPlayer] = (DirAccumulated[g_Config.m_ClDummy][HookedPlayer] > 0);
	}
}
