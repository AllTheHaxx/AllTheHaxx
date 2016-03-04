#include <base/math.h>

#include "luafile.h"
#include "lua.h"
#include "luabinding.h"

//#include <game/client/gameclient.h>
#include <game/collision.h>
#include <game/client/components/chat.h>
#include <game/client/components/emoticon.h>
#include <game/client/components/controls.h>
#include <game/client/components/hud.h>
#include <engine/graphics.h>
#include <engine/serverbrowser.h>
//#include <engine/client/client.h>

CLuaFile::CLuaFile(CLua *pLua, std::string Filename) : m_pLua(pLua), m_Filename(Filename)
{
	m_pLuaState = 0;
	m_State = LUAFILE_STATE_IDLE;
	Reset();
}

void CLuaFile::Reset(bool error)
{
	m_UID = rand()%0xFFFF;
	m_State = error ? LUAFILE_STATE_ERROR: LUAFILE_STATE_IDLE;

	mem_zero(m_aScriptTitle, sizeof(m_aScriptTitle));
	mem_zero(m_aScriptInfo, sizeof(m_aScriptInfo));

	m_ScriptHasSettings = false;

	//if(m_pLuaState)
	//	lua_close(m_pLuaState);
	//m_pLuaState = luaL_newstate();
}

void CLuaFile::Unload()
{
//	if(m_pLuaState)             // -- do not close it in order to prevent crashes
//		lua_close(m_pLuaState);
	lua_gc(m_pLuaState, LUA_GCCOLLECT, 0);
	Reset();
}

void CLuaFile::OpenLua()
{
	if(m_pLuaState)
		lua_close(m_pLuaState);

	m_pLuaState = luaL_newstate();

	lua_atpanic(m_pLuaState, CLua::Panic);
	lua_register(m_pLuaState, "errorfunc", CLua::ErrorFunc);

	luaL_openlibs(m_pLuaState);
	luaopen_base(m_pLuaState);
	luaopen_math(m_pLuaState);
	luaopen_string(m_pLuaState);
	luaopen_table(m_pLuaState);
	//luaopen_io(m_pLua);
	luaopen_os(m_pLuaState);
	//luaopen_package(m_pLua); // not sure whether we should load this
	luaopen_debug(m_pLuaState);
	luaopen_bit(m_pLuaState);
	luaopen_jit(m_pLuaState);
	luaopen_ffi(m_pLuaState); // don't know about this yet. could be a sand box leak.
}

void CLuaFile::Init()
{
	m_State = LUAFILE_STATE_IDLE;

	OpenLua();

	if(!LoadFile("data/luabase/events.lua")) // try the usual script file first
	{
		if(!LoadFile("data/lua/events.luac")) // try for the compiled file if script not found
			m_State = LUAFILE_STATE_ERROR;
		else
			RegisterLuaCallbacks();
	}
	else
		RegisterLuaCallbacks();

	if(m_State != LUAFILE_STATE_ERROR)
	{
		if(LoadFile(m_Filename.c_str()))
			m_State = LUAFILE_STATE_LOADED;
		else
			m_State = LUAFILE_STATE_ERROR;
	}

	// if we errored so far, don't go any further
	if(m_State == LUAFILE_STATE_ERROR)
	{
		Reset(true);
		return;
	}

	// gather basic global infos from the script
	lua_getglobal(m_pLuaState, "_g_ScriptTitle");
	if(lua_isstring(m_pLuaState, -1))
		str_copy(m_aScriptTitle, lua_tostring(m_pLuaState, -1), sizeof(m_aScriptTitle));
	lua_pop(m_pLuaState, -1);

	lua_getglobal(m_pLuaState, "_g_ScriptInfo");
	if(lua_isstring(m_pLuaState, -1))
		str_copy(m_aScriptInfo, lua_tostring(m_pLuaState, -1), sizeof(m_aScriptInfo));
	lua_pop(m_pLuaState, -1);

	m_ScriptHasSettings = ScriptHasSettings();

	// pass the uid to the script
	lua_pushinteger(m_pLuaState, m_UID);
	lua_setglobal(m_pLuaState, "_g_ScriptUID");

	// call the OnScriptInit function if we have one
	try
	{
		LuaRef func = GetFunc("OnScriptInit");
		if(func)
			if(!func().cast<bool>())
				Reset(true);
	}
	catch (std::exception& e)
	{
		printf("LUA EXCEPTION: %s\n", e.what());
		Reset(true);
	}
}

void CLuaFile::RegisterLuaCallbacks() // LUABRIDGE!
{		
	getGlobalNamespace(m_pLuaState)

		// system namespace
		.beginNamespace("_system")
			.addFunction("Import", &CLuaBinding::LuaImport)
		.endNamespace()

		// client namespace
		.beginNamespace("_client")
			// external info
			.addFunction("GetPlayerScore", &CLuaBinding::LuaGetPlayerScore)
		.endNamespace()

		// ui namespace
		.beginNamespace("_ui")
			.addFunction("SetUiColor", &CLuaBinding::LuaSetUiColor)
			.addFunction("DrawUiRect", &CLuaBinding::LuaDrawUiRect)
			.addFunction("DoButton_Menu", &CLuaBinding::LuaDoButton_Menu)
		.endNamespace()

		// graphics namespace
		.beginNamespace("_graphics")
			.addFunction("DrawLine", &CLuaBinding::LuaDrawLine)
			.addFunction("RenderTexture", &CLuaBinding::LuaRenderTexture)
		.endNamespace()

		// global types
		.beginClass< vector2_base<int> >("vec2")
			.addConstructor <void (*) (int, int)> ()
			.addData("x", &vector2_base<int>::x)
			.addData("y", &vector2_base<int>::y)
		.endClass()
		.beginClass< vector3_base<int> >("vec3")
			.addConstructor <void (*) (int, int, int)> ()
			.addData("x", &vector3_base<int>::x)
			.addData("y", &vector3_base<int>::y)
			.addData("z", &vector3_base<int>::z)
		.endClass()
		.beginClass< vector4_base<int> >("vec4")
			.addConstructor <void (*) (int, int, int, int)> ()
			.addData("r", &vector4_base<int>::r)
			.addData("g", &vector4_base<int>::g)
			.addData("b", &vector4_base<int>::b)
			.addData("a", &vector4_base<int>::a)
		.endClass()

		.beginClass< vector2_base<float> >("vec2f")
			.addConstructor <void (*) (float, float)> ()
			.addData("x", &vector2_base<float>::x)
			.addData("y", &vector2_base<float>::y)
		.endClass()
		.beginClass< vector3_base<float> >("vec3f")
			.addConstructor <void (*) (float, float, float)> ()
			.addData("x", &vector3_base<float>::x)
			.addData("y", &vector3_base<float>::y)
			.addData("z", &vector3_base<float>::z)
		.endClass()
		.beginClass< vector4_base<float> >("vec4f")
			.addConstructor <void (*) (float, float, float, float)> ()
			.addData("r", &vector4_base<float>::r)
			.addData("g", &vector4_base<float>::g)
			.addData("b", &vector4_base<float>::b)
			.addData("a", &vector4_base<float>::a)
		.endClass()
		
		//OOP BEGINS HERE		
		//ICLIENT
		.beginClass<IClient>("IClient")
			.addProperty("Tick", &IClient::GameTick)
			.addFunction("Connect", &IClient::Connect)
			.addFunction("Disconnect", &IClient::Disconnect)
			.addFunction("Quit", &IClient::Quit)
			.addFunction("RconSend", &IClient::Rcon)
			.addFunction("RconAuth", &IClient::RconAuth)
			.addFunction("DummyConnect", &IClient::DummyConnect)
			.addFunction("DummyDisconnect", &IClient::DummyDisconnect)
			.addProperty("FPS",&IClient::GetFPS)
		.endClass()
		
		//COMPONENTS
		.beginClass<CChat>("CChat")
			.addFunction("Say", &CChat::Say)
			.addProperty("Mode", &CChat::GetMode)
		.endClass()
		
		.beginClass<CEmoticon>("CEmoticon")
			.addFunction("Send", &CEmoticon::Emote)
			.addFunction("SendEye", &CEmoticon::EyeEmote)
			.addProperty("Active", &CEmoticon::Active)
		.endClass()

		.beginClass<CCollision>("CCollision")
			.addFunction("GetMapWidth", &CCollision::GetWidth)
			.addFunction("GetMapHeight", &CCollision::GetHeight)
			.addFunction("GetTile", &CCollision::GetTileRaw)
		.endClass()
		
		.beginClass<CHud>("CHud")
			.addFunction("PushNotification", &CHud::PushNotification)
		.endClass()

		//Local player Infos
		.beginClass<CNetObj_CharacterCore>("CNetObj_CharacterCore")  //TODO : Add the whole class!
			.addData("PosX", &CNetObj_CharacterCore::m_X, false)
			.addData("PosY", &CNetObj_CharacterCore::m_Y, false)
			.addData("HookedPlayer", &CNetObj_CharacterCore::m_HookedPlayer, false)
			.addData("HookState", &CNetObj_CharacterCore::m_HookState, false)
		.endClass()
		.deriveClass<CNetObj_Character, CNetObj_CharacterCore>("CNetObj_Character")  //TODO: Ppb add the rest
			.addData("Weapon", &CNetObj_Character::m_Weapon)
			.addData("Armor", &CNetObj_Character::m_Armor)
			.addData("Health", &CNetObj_Character::m_Health)
			.addData("Ammo", &CNetObj_Character::m_AmmoCount)
		.endClass()
		
		.beginClass<CControls>("CControls")
			.addProperty("Direction", &CControls::GetDirection, &CControls::SetDirection)
			.addProperty("Fire", &CControls::GetFire, &CControls::SetFire)
			.addProperty("Hook", &CControls::GetHook, &CControls::SetHook)
			.addProperty("Jump", &CControls::GetJump, &CControls::SetJump)
			.addProperty("WantedWeapon", &CControls::GetWantedWeapon, &CControls::SetWantedWeapon)
			.addProperty("TargetX", &CControls::GetTargetX, &CControls::SetTargetX)
			.addProperty("TargetY", &CControls::GetTargetY, &CControls::SetTargetY)
		.endClass()
		
		//Server Infos
		.beginClass<CServerInfo>("CServerInfo")
			.addProperty("GameMode", &CServerInfo::LuaGetGameType)
			.addProperty("Name", &CServerInfo::LuaGetName)
			.addProperty("Map", &CServerInfo::LuaGetMap)
			.addProperty("Version", &CServerInfo::LuaGetVersion)
			.addProperty("IP", &CServerInfo::LuaGetIP)
			.addData("Latency", &CServerInfo::m_Latency, false)
			.addData("MaxPlayers", &CServerInfo::m_MaxPlayers, false)
			.addData("NumPlayers", &CServerInfo::m_NumPlayers, false)
		.endClass()
		
		.beginClass<CGameClient::CSnapState>("CSnapState")
			.addData("Tee", &CGameClient::CSnapState::m_pLocalCharacter)
			.addData("ClientID", &CGameClient::CSnapState::m_LocalClientID)
		.endClass()
		
		.beginClass<IGraphics>("IGraphics")
			.addFunction("QuadsBegin", &IGraphics::QuadsBegin)
			.addFunction("QuadsEnd", &IGraphics::QuadsEnd)
			.addFunction("LinesBegin", &IGraphics::LinesBegin)
			.addFunction("LinesEnd", &IGraphics::LinesEnd)
			
			.addFunction("SetColor", &IGraphics::SetColor)
			.addFunction("BlendNone", &IGraphics::BlendNone)
			.addFunction("BlendNormal", &IGraphics::BlendNormal)
			.addFunction("BlendAdditive", &IGraphics::BlendAdditive)

			.addFunction("LoadTexture", &IGraphics::LoadTexture)

			.addProperty("ScreenWidth", &IGraphics::ScreenWidth)
			.addProperty("ScreenHeight", &IGraphics::ScreenHeight)
		.endClass()

		.beginClass<CGameClient>("CGameClient")
			.addData("Chat", &CGameClient::m_pChat)		
			.addData("ServerInfo", &CGameClient::m_CurrentServerInfo, false)
			.addData("Emote", &CGameClient::m_pEmoticon, false)
			.addData("HUD", &CGameClient::m_pHud, false)
			//.addData("Client", &CGameClient::m_pClient, false)   //"Game" resembles GameClient, Game.Client => Client
			.addData("Input", &CGameClient::m_pControls)
			.addData("Collision", &CGameClient::m_pCollision)
			//pointer to components & stuff from gameclient
			.addData("Local", &CGameClient::m_Snap)
		.endClass()
		
		//MAIN NAMESPACE
		.beginNamespace("TW")
			.addVariable("Game", &CLua::m_pCGameClient, false)
			.addVariable("Client", &CLua::m_pClient, false)
		.endNamespace()
		
		.beginNamespace("Game")
            .addVariable("Chat", &CLua::m_pCGameClient->m_pChat, false)
            .addVariable("ServerInfo", &CLua::m_pCGameClient->m_CurrentServerInfo, false)
            .addVariable("Emote", &CLua::m_pCGameClient->m_pEmoticon, false)
            .addVariable("HUD", &CLua::m_pCGameClient->m_pHud, false)
            //.addData("Client", &CGameClient::m_pClient, false)   //"Game" resembles GameClient, Game.Client => Client
            .addVariable("Input", &CLua::m_pCGameClient->m_pControls, false)
            .addVariable("Collision", &CLua::m_pCGameClient->m_pCollision, false)
            //pointer to components & stuff from gameclient
            .addVariable("Local", &CLua::m_pCGameClient->m_Snap, false)
            .addVariable("Client", &CLua::m_pClient, false)
		.endNamespace()

		.beginNamespace("Graphics")
			.addVariable("Engine", &CLua::m_pCGameClient->m_pGraphics) //dunno, this should be maybe an own subspace :O
		.endNamespace()
		
		.beginClass<CConfigProperties>("Config")   // g_Config stuff...
			.addStaticProperty("PlayerName", &CConfigProperties::GetConfigPlayerName, &CConfigProperties::SetConfigPlayerName)
			.addStaticProperty("PlayerClan", &CConfigProperties::GetConfigPlayerClan, &CConfigProperties::SetConfigPlayerClan)  //char-arrays
			.addStaticData("PlayerCountry", &CConfigProperties::m_pConfig->m_PlayerCountry)  //ints
			
			.addStaticProperty("PlayerSkin", &CConfigProperties::GetConfigPlayerSkin, &CConfigProperties::SetConfigPlayerSkin)
			.addStaticData("PlayerColorBody", &CConfigProperties::m_pConfig->m_ClPlayerColorBody)
			.addStaticData("PlayerColorFeet", &CConfigProperties::m_pConfig->m_ClPlayerColorFeet)
			.addStaticData("PlayerUseCustomColor", &CConfigProperties::m_pConfig->m_ClPlayerUseCustomColor)
		.endClass()
		
		
		//OOP ENDS HERE
	;
	dbg_msg("Lua", "Registering LuaBindings complete.");
}

luabridge::LuaRef CLuaFile::GetFunc(const char *pFuncName)
{
	LuaRef func = getGlobal(m_pLuaState, pFuncName);
	if(func == 0)
		dbg_msg("Lua", "Error: Function '%s' not found.", pFuncName);

	return func;  // return 0 if the function is not found!
}

void CLuaFile::CallFunc(const char *pFuncName)
{
	if(!m_pLuaState)
		return;

	// TODO : Find a way to pass the LuaFunction up to 8 arguments (no matter of the type)
}

bool CLuaFile::LoadFile(const char *pFilename)
{
	if(!pFilename || pFilename[0] == '\0' || str_length(pFilename) <= 4 ||
			(str_comp_nocase(&pFilename[str_length(pFilename)]-4, ".lua") &&
			str_comp_nocase(&pFilename[str_length(pFilename)]-7, ".config")) || !m_pLuaState)
		return false;

    int Status = luaL_loadfile(m_pLuaState, pFilename);
    if (Status)
    {
        // does this work? -- I don't think so, Henritees.
        CLua::ErrorFunc(m_pLuaState);
        return false;
    }

    Status = lua_pcall(m_pLuaState, 0, LUA_MULTRET, 0);
    if (Status)
    {
    	CLua::ErrorFunc(m_pLuaState);
        return false;
    }

    return true;
}

bool CLuaFile::ScriptHasSettings()
{
	LuaRef func1 = GetFunc("OnScriptRenderSettings");
	LuaRef func2 = GetFunc("OnScriptSaveSettings");
	if(func1.cast<bool>() && func2.cast<bool>())
		return true;
	return false;
}
