#include <fstream>

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
#include <game/client/components/menus.h>
#include <game/client/components/voting.h>
#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/serverbrowser.h>
#include <engine/curlwrapper.h>
#include <engine/textrender.h>
//#include <engine/client/client.h>

CLuaFile::CLuaFile(CLua *pLua, std::string Filename, bool Autoload) : m_pLua(pLua), m_Filename(Filename), m_ScriptAutoload(Autoload)
{
	m_pLuaState = 0;
	m_State = LUAFILE_STATE_IDLE;
	m_pErrorStr = 0;
	Reset();
}

CLuaFile::~CLuaFile()
{
	Unload();
}

void CLuaFile::Reset(bool error)
{
	m_UID = rand()%0xFFFF;
	m_PermissionFlags = 0x0;
	m_State = error ? LUAFILE_STATE_ERROR: LUAFILE_STATE_IDLE;

	mem_zero(m_aScriptTitle, sizeof(m_aScriptTitle));
	mem_zero(m_aScriptInfo, sizeof(m_aScriptInfo));

	m_ScriptHasSettings = false;

	LoadPermissionFlags();

	//if(m_pLuaState)
	//	lua_close(m_pLuaState);
	//m_pLuaState = luaL_newstate();
}

void CLuaFile::LoadPermissionFlags()
{
	std::ifstream f(m_Filename.c_str());
	std::string line; bool searching = true;
	while(std::getline(f, line))
	{
		if(line.find("]]") != std::string::npos)
			break;

		if(searching && line != "--[[#!")
			continue;
		if(searching)
		{
			searching = false;
			continue;
		}

		// make sure we only get what we want
		char aBuf[32]; char *p;
		str_copy(aBuf, line.c_str(), sizeof(aBuf));
		str_sanitize_strong(aBuf);
		p = aBuf;
		while(*p == ' ' || *p == '\t')
			p++;

		// some sort of syntax error there? just ignore the line
		if(p++[0] != '#')
			continue;

		if(str_comp_nocase("io", p) == 0)
			m_PermissionFlags |= LUAFILE_PERMISSION_IO;
		if(str_comp_nocase("debug", p) == 0)
			m_PermissionFlags |= LUAFILE_PERMISSION_DEBUG;
		if(str_comp_nocase("ffi", p) == 0)
			m_PermissionFlags |= LUAFILE_PERMISSION_FFI;
		if(str_comp_nocase("os", p) == 0)
			m_PermissionFlags |= LUAFILE_PERMISSION_OS;
		if(str_comp_nocase("package", p) == 0)
			m_PermissionFlags |= LUAFILE_PERMISSION_PACKAGE;
	}
	
	//m_PermissionFlags |= LUAFILE_PERMISSION_OS;
	//m_PermissionFlags |= LUAFILE_PERMISSION_DEBUG;
}

void CLuaFile::Unload(bool error)
{
//	if(m_pLuaState)			 // -- do not close it in order to prevent crashes
//		lua_close(m_pLuaState);

	// script is not loaded -> don't unload it
	if(m_State != LUAFILE_STATE_LOADED)
	{
		Reset(error);
		return;
	}

	// assertion right here because m_pLuaState just CANNOT be 0 at this
	// point, and if it is, something went wrong that we need to debug!
	dbg_assert(m_pLuaState != 0, "Something went fatally wrong! Active luafile has no state?!");

	try
	{
		LuaRef func = GetFunc("OnScriptUnload");
		if(func.cast<bool>())
			func();
	}
	catch(std::exception &e)
	{
		//printf("LUA EXCEPTION: %s\n", e.what());
		m_pLua->HandleException(e, this);
	}

	lua_gc(m_pLuaState, LUA_GCCOLLECT, 0);
	Reset(error);
}

void CLuaFile::OpenLua()
{
	if(m_pLuaState)
		lua_close(m_pLuaState);

	m_pLuaState = luaL_newstate();

	lua_atpanic(m_pLuaState, CLua::Panic);
	lua_register(m_pLuaState, "errorfunc", CLua::ErrorFunc);

	//luaL_openlibs(m_pLuaState);  // we don't need certain libs -> open them all manually

	luaopen_base(m_pLuaState);	// base
	luaopen_math(m_pLuaState);	// math.* functions
	luaopen_string(m_pLuaState);// string.* functions
	luaopen_table(m_pLuaState);	// table operations
	luaopen_bit(m_pLuaState);	// bit operations
	//luaopen_jit(m_pLuaState);	// control the jit-compiler [don't needed]

	if(m_PermissionFlags&LUAFILE_PERMISSION_IO)
		luaopen_io(m_pLuaState);	// input/output of files
	//if(m_PermissionFlags&LUAFILE_PERMISSION_DEBUG) XXX
		luaopen_debug(m_pLuaState);	// debug stuff for whatever... can be removed in further patches
	if(m_PermissionFlags&LUAFILE_PERMISSION_FFI)
		luaopen_ffi(m_pLuaState);	// register and write own C-Functions and call them in lua (whoever may need that...)
	//if(m_PermissionFlags&LUAFILE_PERMISSION_OS) XXX
		luaopen_os(m_pLuaState);	// evil
	if(m_PermissionFlags&LUAFILE_PERMISSION_PACKAGE)
		luaopen_package(m_pLuaState); //used for modules etc... not sure whether we should load this

}

void CLuaFile::Init()
{
	if(m_State == LUAFILE_STATE_LOADED)
		Unload();

	m_State = LUAFILE_STATE_IDLE;

	LoadPermissionFlags();
	OpenLua();

	if(!LoadFile("data/luabase/events.lua")) // try the usual script file first
	{
		//if(!LoadFile("data/lua/events.luac")) // try for the compiled file if script not found
			m_State = LUAFILE_STATE_ERROR;
		//else
		//	RegisterLuaCallbacks(m_pLuaState);
	}
	else
		RegisterLuaCallbacks(m_pLuaState);

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
	lua_getglobal(m_pLuaState, "g_ScriptTitle");
	if(lua_isstring(m_pLuaState, -1))
		str_copy(m_aScriptTitle, lua_tostring(m_pLuaState, -1), sizeof(m_aScriptTitle));
	lua_pop(m_pLuaState, -1);

	lua_getglobal(m_pLuaState, "g_ScriptInfo");
	if(lua_isstring(m_pLuaState, -1))
		str_copy(m_aScriptInfo, lua_tostring(m_pLuaState, -1), sizeof(m_aScriptInfo));
	lua_pop(m_pLuaState, -1);

	m_ScriptHasSettings = ScriptHasSettings();

	// pass the uid to the script
	lua_pushinteger(m_pLuaState, m_UID);
	lua_setglobal(m_pLuaState, "g_ScriptUID");

	// call the OnScriptInit function if we have one
	bool success = true;
	LUA_CALL_FUNC(m_pLuaState, "OnScriptInit", bool, success);
	if(!success)
	{
		dbg_msg("lua", "script '%s' rejected being loaded, did 'OnScriptInit()' return true...?", m_Filename.c_str());
		Unload(true);
		return;
	}
}

void CLuaFile::RegisterLuaCallbacks(lua_State *L) // LUABRIDGE!
{		
	getGlobalNamespace(L)

		.addFunction("Import", &CLuaBinding::LuaImport)
		.addFunction("KillScript", &CLuaBinding::LuaKillScript)
		//.addFunction("print", &CLuaFile::LuaPrintOverride)

		// client namespace
		.beginNamespace("_client")
			// external info
			.addFunction("GetPlayerScore", &CLuaBinding::LuaGetPlayerScore)
		.endNamespace()

		// graphics namespace
		.beginNamespace("_graphics")
			.addFunction("RenderTexture", &CLuaBinding::LuaRenderTexture)
			.addFunction("RenderQuad", &CLuaBinding::LuaRenderQuadRaw)
		.endNamespace()

		// global types
		.beginClass< vector2_base<int> >("vec2")
			.addConstructor <void (*) (int, int)> ()
			.addFunction("__add", &vector2_base<int>::operator+)
			.addFunction("__sub", &vector2_base<int>::operator-)
			.addFunction("__mul", &vector2_base<int>::operator*)
			.addFunction("__div", &vector2_base<int>::operator/)
			.addFunction("__eq", &vector2_base<int>::operator==)
			.addData("x", &vector2_base<int>::x)
			.addData("y", &vector2_base<int>::y)
		.endClass()
		.beginClass< vector3_base<int> >("vec3")
			.addConstructor <void (*) (int, int, int)> ()
			.addFunction("__add", &vector3_base<int>::operator+)
			.addFunction("__sub", &vector3_base<int>::operator-)
			.addFunction("__mul", &vector3_base<int>::operator*)
			.addFunction("__div", &vector3_base<int>::operator/)
			.addFunction("__eq", &vector3_base<int>::operator==)
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
			.addFunction("__add", &vector2_base<float>::operator+)
			.addFunction("__sub", &vector2_base<float>::operator-)
			.addFunction("__mul", &vector2_base<float>::operator*)
			.addFunction("__div", &vector2_base<float>::operator/)
			.addFunction("__eq", &vector2_base<float>::operator==)
			.addData("x", &vector2_base<float>::x)
			.addData("y", &vector2_base<float>::y)
		.endClass()
		.beginClass< vector3_base<float> >("vec3f")
			.addConstructor <void (*) (float, float, float)> ()
			.addFunction("__add", &vector3_base<float>::operator+)
			.addFunction("__sub", &vector3_base<float>::operator-)
			.addFunction("__mul", &vector3_base<float>::operator*)
			.addFunction("__div", &vector3_base<float>::operator/)
			.addFunction("__eq", &vector3_base<float>::operator==)
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
		
		.beginClass< CUIRect >("UIRect")
			.addConstructor <void (*) ()> ()
			.addConstructor <void (*) (float)> ()
			.addConstructor <void (*) (float, float, float, float)> ()
			.addData("x", &CUIRect::x)
			.addData("y", &CUIRect::y)
			.addData("w", &CUIRect::w)
			.addData("h", &CUIRect::h)
			.addFunction("HSplitMid", &CUIRect::HSplitMid)
			.addFunction("HSplitTop", &CUIRect::HSplitTop)
			.addFunction("HSplitBottom", &CUIRect::HSplitBottom)
			.addFunction("VSplitMid", &CUIRect::HSplitMid)
			.addFunction("VSplitLeft", &CUIRect::VSplitLeft)
			.addFunction("VSplitRight", &CUIRect::VSplitRight)
			.addFunction("Margin", &CUIRect::Margin)
			.addFunction("VMargin", &CUIRect::VMargin)
			.addFunction("HMargin", &CUIRect::HMargin)
		.endClass()

		.beginClass< IGraphics::CQuadItem >("QuadItem")
			.addConstructor <void (*) ()> ()
			.addConstructor <void (*) (float, float, float, float)> ()
		.endClass()

		.beginClass< IGraphics::CLineItem >("LineItem")
			.addConstructor <void (*) ()> ()
			.addConstructor <void (*) (float, float, float, float)> ()
		.endClass()

		.beginClass< CMenus::CButtonContainer >("ButtonContainer")
			.addConstructor <void (*) ()> ()
		.endClass()

		//OOP BEGINS HERE
		//ICLIENT
		.beginClass<IClient>("IClient")
			.addProperty("Tick", &IClient::GameTick)
			.addProperty("IntraGameTick", &IClient::IntraGameTick)
			.addProperty("LocalTime", &IClient::LocalTime)

			.addFunction("Connect", &IClient::Connect)
			.addFunction("Disconnect", &IClient::Disconnect)

			.addFunction("DummyConnect", &IClient::DummyConnect)
			.addFunction("DummyDisconnect", &IClient::DummyDisconnect)
			.addFunction("DummyConnected", &IClient::DummyConnected)
			.addFunction("DummyConnecting", &IClient::DummyConnecting)

			.addFunction("SendInfo", &IClient::SendPlayerInfo)

			.addFunction("RconAuth", &IClient::RconAuth)
			.addFunction("RconAuthed", &IClient::RconAuthed)
			.addFunction("RconSend", &IClient::Rcon)

			.addProperty("FPS", &IClient::GetFPS)
			.addProperty("State", &IClient::State)

			.addFunction("DemoStart", &IClient::DemoRecorder_Start)
			.addFunction("DemoStop", &IClient::DemoRecorder_Stop)

			.addFunction("Quit", &IClient::Quit)
		.endClass()

		//COMPONENTS
		.beginClass<CUI>("CUI")
			.addFunction("DoLabel", &CUI::DoLabel)
			.addFunction("DoLabelScaled", &CUI::DoLabelScaled)
			.addFunction("Scale", &CUI::Scale)
			.addFunction("Screen", &CUI::Screen)
			.addFunction("MouseX", &CUI::MouseX)
			.addFunction("MouseY", &CUI::MouseY)
			.addFunction("MouseWorldX", &CUI::MouseWorldX)
			.addFunction("MouseWorldY", &CUI::MouseWorldY)
			.addFunction("MouseButton", &CUI::MouseButton)
			.addFunction("MouseButtonClicked", &CUI::MouseButtonClicked)
			.addFunction("MouseInside", &CUI::MouseInside)
		.endClass()

		.beginClass<CRenderTools>("CRenderTools")
			.addFunction("SelectSprite", &CRenderTools::SelectSpriteLua)
			.addFunction("DrawSprite", &CRenderTools::DrawSprite)
			.addFunction("DrawRoundRect", &CRenderTools::DrawRoundRect)
			.addFunction("DrawRoundRectExt", &CRenderTools::DrawRoundRectExt)
			.addFunction("DrawUIRect", &CRenderTools::DrawUIRect)
			.addFunction("DrawCircle", &CRenderTools::DrawCircle)
		.endClass()

		.beginClass<CChat>("CChat") /// Game.Chat
			.addFunction("Say", &CChat::Say)
			.addFunction("Print", &CChat::AddLine)
			.addFunction("AddLine", &CChat::AddLine)
			.addProperty("Mode", &CChat::GetMode)
		.endClass()

		.beginClass<IConsole>("IConsole")
			.addFunction("Print", &IConsole::Print)
			.addFunction("LineIsValid", &IConsole::LineIsValid)
			.addFunction("ExecuteLine", &IConsole::ExecuteLine)
		.endClass()

		.beginClass<CEmoticon>("CEmoticon") /// Game.Emote
			.addFunction("Send", &CEmoticon::Emote)
			.addFunction("SendEye", &CEmoticon::EyeEmote)
			.addProperty("Active", &CEmoticon::Active)
		.endClass()

		.beginClass<CCollision>("CCollision") /// Game.Collision
			.addProperty("GetMapWidth", &CCollision::GetWidth)
			.addProperty("GetMapHeight", &CCollision::GetHeight)

			.addFunction("Distance", &CCollision::Distance)
			.addFunction("Normalize", &CCollision::Normalize)
			.addFunction("ClosestPointOnLine", &CCollision::ClosestPointOnLine)

			.addFunction("GetTile", &CCollision::GetTileRaw)
			.addFunction("CheckPoint", &CCollision::CheckPointLua)

			.addFunction("IntersectLine", &CCollision::IntersectLine)
			.addFunction("MovePoint", &CCollision::MovePoint)
			.addFunction("MoveBox", &CCollision::MoveBox)
			.addFunction("TestBox", &CCollision::TestBox)
		.endClass()

		.beginClass<CHud>("CHud") /// Game.HUD
			.addFunction("PushNotification", &CHud::PushNotification)
		.endClass()

		.beginClass<CMenus>("CMenus") /// Game.Menus
			.addProperty("Active", &CMenus::IsActive)
			.addProperty("ActivePage", &CMenus::GetActivePage, &CMenus::SetActivePage)
			.addProperty("MousePos", &CMenus::GetMousePos, &CMenus::SetMousePos)
			.addFunction("ButtonColorMul", &CMenus::ButtonColorMul)
			.addFunction("DoButton_Menu", &CMenus::DoButton_Menu)
			.addFunction("DoButton_MenuTab", &CMenus::DoButton_MenuTab)
			.addFunction("DoButton_CheckBox", &CMenus::DoButton_CheckBox)
			.addFunction("DoButton_Toggle", &CMenus::DoButton_Toggle)
			.addFunction("DoButton_CheckBox_Number", &CMenus::DoButton_CheckBox_Number)
			.addFunction("DoButton_Sprite", &CMenus::DoButton_Sprite)
			.addFunction("DoScrollbarV", &CMenus::DoScrollbarV)
			.addFunction("DoScrollbarH", &CMenus::DoScrollbarH)
		.endClass()

		.beginClass<CVoting>("CVoting") /// Game.Voting
			.addFunction("CallvoteSpectate", &CVoting::CallvoteSpectate)
			.addFunction("CallvoteKick", &CVoting::CallvoteKick)
			.addFunction("CallvoteOption", &CVoting::CallvoteOption)
			.addFunction("Vote", &CVoting::Vote)

			.addProperty("SecondsLeft", &CVoting::SecondsLeft)
			.addProperty("IsVoting", &CVoting::IsVoting)
			.addProperty("TakenChoice", &CVoting::TakenChoice)
		.endClass()

		// local playerinfo
		.beginClass<CNetObj_CharacterCore>("CNetObj_CharacterCore")  //TODO : Add the whole class!
			.addData("PosX", &CNetObj_CharacterCore::m_X, false)
			.addData("PosY", &CNetObj_CharacterCore::m_Y, false)
			.addData("VelX", &CNetObj_CharacterCore::m_VelX, false)
			.addData("VelY", &CNetObj_CharacterCore::m_VelY, false)

			.addData("Angle", &CNetObj_Character::m_Angle, false)
			.addData("Direction", &CNetObj_Character::m_Direction, false)

			.addData("Jumped", &CNetObj_CharacterCore::m_Jumped, false)

			.addData("HookedPlayer", &CNetObj_CharacterCore::m_HookedPlayer, false)
			.addData("HookState", &CNetObj_CharacterCore::m_HookState, false)
			.addData("HookTick", &CNetObj_CharacterCore::m_HookTick, false)
		.endClass()

		/// Game.CharSnap(ID).Cur
		.deriveClass<CNetObj_Character, CNetObj_CharacterCore>("CNetObj_Character") // TODO: Ppb add the rest
			.addData("PlayerFlags", &CNetObj_Character::m_PlayerFlags)

			.addData("Health", &CNetObj_Character::m_Health)
			.addData("Armor", &CNetObj_Character::m_Armor)
			.addData("Ammo", &CNetObj_Character::m_AmmoCount)
			.addData("Weapon", &CNetObj_Character::m_Weapon)

			.addData("Emote", &CNetObj_Character::m_Emote)
			.addData("AttackTick", &CNetObj_Character::m_AttackTick)
		.endClass()

		/// Game.Players(ID).Tee
		/// Game.LocalTee        <--self
		.beginClass<CCharacterCore>("CCharacterCore")
			.addData("Pos", &CCharacterCore::m_Pos)
			.addData("Vel", &CCharacterCore::m_Vel)
			.addData("Hook", &CCharacterCore::m_Hook)
			.addData("Collision", &CCharacterCore::m_Collision)
			.addData("HookPos", &CCharacterCore::m_HookPos)
			.addData("HookTick", &CCharacterCore::m_HookTick)
			.addData("HookState", &CCharacterCore::m_HookState)
			.addData("HookDir", &CCharacterCore::m_HookDir)
			.addData("HookedPlayer", &CCharacterCore::m_HookedPlayer)
			.addData("Weapon", &CCharacterCore::m_ActiveWeapon)
			.addData("Direction", &CCharacterCore::m_Direction)
			.addData("Angle", &CCharacterCore::m_Angle)
			.addData("Jumps", &CCharacterCore::m_Jumps)
			.addData("JumpedTotal", &CCharacterCore::m_JumpedTotal)
			.addData("Jumped", &CCharacterCore::m_Jumped)
			.addFunction("Reset", &CCharacterCore::Reset)
		.endClass()

		.beginClass<CTuneParam>("CTuneParam")
			.addProperty("Value", &CTuneParam::Get)
		.endClass()


#define MACRO_TUNING_PARAM(Name,ScriptName,Value,Description) \
	.addData(#Name, &CTuningParams::m_##Name) \
	.addData(#ScriptName, &CTuningParams::m_##Name)

		.beginClass<CTuningParams>("CTuningParams") /// Game.Tuning()
			#include <game/tuning.h>
		.endClass()

#undef MACRO_TUNING_PARAM


		.beginClass<CControls>("CControls") /// Game.Input
			.addProperty("Direction", &CControls::GetDirection, &CControls::SetDirection)
			.addProperty("Fire", &CControls::GetFire, &CControls::SetFire)
			.addProperty("Hook", &CControls::GetHook, &CControls::SetHook)
			.addProperty("Jump", &CControls::GetJump, &CControls::SetJump)
			.addProperty("WantedWeapon", &CControls::GetWantedWeapon, &CControls::SetWantedWeapon)
			.addProperty("TargetX", &CControls::GetTargetX, &CControls::SetTargetX)
			.addProperty("TargetY", &CControls::GetTargetY, &CControls::SetTargetY)
			.addProperty("MouseX", &CControls::GetMouseX, &CControls::SetMouseX)
			.addProperty("MouseY", &CControls::GetMouseY, &CControls::SetMouseY)
		.endClass()

		.beginClass<IInput>("IInput") /// Engine.Input
			.addFunction("KeyPress", &IInput::KeyPress)
			.addFunction("KeyIsPressed", &IInput::KeyIsPressedLua)
			.addFunction("KeyName", &IInput::KeyNameSTD)
			.addFunction("KeyID", &IInput::GetKeyID)
			.addFunction("GetClipboardText", &IInput::GetClipboardTextSTD)
			.addFunction("SetClipboardText", &IInput::SetClipboardTextSTD)

			.addFunction("MouseModeRelative", &IInput::MouseModeRelative)
			.addFunction("MouseModeAbsolute", &IInput::MouseModeAbsolute)
			.addFunction("MouseDoubleClick", &IInput::MouseDoubleClick)

			.addFunction("SimulateKeyPressDirect", &IInput::SimulateKeyPress)
			.addFunction("SimulateKeyPress", &IInput::SimulateKeyPressSTD)
			.addFunction("SimulateKeyReleaseDirect", &IInput::SimulateKeyRelease)
			.addFunction("SimulateKeyRelease", &IInput::SimulateKeyReleaseSTD)
		.endClass()

		.beginClass<ICurlWrapper>("ICurlWrapper") /// Engine.Curl
			.addFunction("httpSimplePost", &ICurlWrapper::PerformSimplePOST)
		.endClass()

		// serverinfo
		.beginClass<CServerInfo>("CServerInfo") /// Game.ServerInfo
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

		.beginClass<CGameClient::CSnapState::CCharacterInfo>("CCharacterInfo") // Game.CharSnap(ID)
			.addData("Active", &CGameClient::CSnapState::CCharacterInfo::m_Active, false)
			.addData("Cur", &CGameClient::CSnapState::CCharacterInfo::m_Cur, false)
		.endClass()

		.beginClass<CGameClient::CSnapState::CSpectateInfo>("CSpectateInfo")
			.addData("Active", &CGameClient::CSnapState::CSpectateInfo::m_Active)
			.addData("SpectatorID", &CGameClient::CSnapState::CSpectateInfo::m_SpectatorID)
			.addData("UsePosition", &CGameClient::CSnapState::CSpectateInfo::m_UsePosition)
			.addData("Position", &CGameClient::CSnapState::CSpectateInfo::m_Position)
		.endClass()

		.beginClass<CGameClient::CClientData>("CClientData") /// Game.Players(ID)
			.addData("Active", &CGameClient::CClientData::m_Active)
			.addData("ColorBody", &CGameClient::CClientData::m_ColorBody)
			.addData("ColorFeet", &CGameClient::CClientData::m_ColorFeet)
			.addData("UseCustomColor", &CGameClient::CClientData::m_UseCustomColor)
			.addData("SkinID", &CGameClient::CClientData::m_SkinID)
			.addData("Country", &CGameClient::CClientData::m_Country)
			.addData("SkinColor", &CGameClient::CClientData::m_SkinColor)
			.addData("Team", &CGameClient::CClientData::m_Team)
			.addData("Emote", &CGameClient::CClientData::m_Emoticon)
			.addData("EmoteStart", &CGameClient::CClientData::m_EmoticonStart)
			.addData("Friend", &CGameClient::CClientData::m_Friend)
			.addData("Foe", &CGameClient::CClientData::m_Foe)
			.addProperty("Name", &CGameClient::CClientData::GetName)
			.addProperty("Clan", &CGameClient::CClientData::GetClan)
			.addProperty("SkinName", &CGameClient::CClientData::GetSkinName)
			.addData("Tee", &CGameClient::CClientData::m_Predicted)
		.endClass()

		.beginClass<IGraphics>("IGraphics") /// Engine.Graphics
			.addFunction("QuadsBegin", &IGraphics::QuadsBegin)
			.addFunction("QuadsEnd", &IGraphics::QuadsEnd)
			.addFunction("QuadsDraw", &IGraphics::QuadsDraw)
			.addFunction("LinesBegin", &IGraphics::LinesBegin)
			.addFunction("LinesEnd", &IGraphics::LinesEnd)
			.addFunction("LinesDraw", &IGraphics::LinesDraw)

			.addFunction("SetRotation", &IGraphics::QuadsSetRotation)
			.addFunction("SetColor", &IGraphics::SetColor)
			.addFunction("BlendNone", &IGraphics::BlendNone)
			.addFunction("BlendNormal", &IGraphics::BlendNormal)
			.addFunction("BlendAdditive", &IGraphics::BlendAdditive)

			.addFunction("LoadTexture", &IGraphics::LoadTexture)
			.addFunction("UnloadTexture", &IGraphics::UnloadTexture)
			.addFunction("TextureSet", &IGraphics::TextureSet)

			.addFunction("MapScreen", &IGraphics::MapScreen)

			.addProperty("ScreenWidth", &IGraphics::ScreenWidth)
			.addProperty("ScreenHeight", &IGraphics::ScreenHeight)
		.endClass()

		.beginClass<ITextRender>("ITextRender")
			.addFunction("Text", &ITextRender::Text)
			.addFunction("TextWidth", &ITextRender::TextWidth)
			.addFunction("TextLineCount", &ITextRender::TextLineCount)
			.addFunction("TextColor", &ITextRender::TextColor)
			.addFunction("TextOutlineColor", &ITextRender::TextOutlineColor)
		.endClass()

		.beginClass<CGameClient>("CGameClient")   //this class is kinda outdated due to "Game"
		/*	.addData("Chat", &CGameClient::m_pChat)
			.addData("ServerInfo", &CGameClient::m_CurrentServerInfo, false)
			.addData("Emote", &CGameClient::m_pEmoticon, false)
			.addData("HUD", &CGameClient::m_pHud, false)
			//.addData("Client", &CGameClient::m_pClient, false)   //"Game" resembles GameClient, Game.Client => Client
			.addData("Input", &CGameClient::m_pControls)
			.addData("Collision", &CGameClient::m_pCollision)
			//pointer to components & stuff from gameclient
			//.addData("Local", &CGameClient::m_Snap)
			.addData("LocalTee", &CGameClient::m_PredictedChar) */

			.addFunction("IntersectCharacter", &CGameClient::IntersectCharacterLua)
		.endClass()

		// MAIN NAMESPACE
		.beginNamespace("TW")
			.addVariable("Game", &CLua::m_pCGameClient, false)
			.addVariable("Client", &CLua::m_pClient, false)
		.endNamespace()

		.beginNamespace("Game")
			.addVariable("Chat", &CLua::m_pCGameClient->m_pChat, false)
			.addVariable("Console", &CLua::m_pCGameClient->m_pConsole, false)
			.addVariable("ServerInfo", &CLua::m_pCGameClient->m_CurrentServerInfo, false)
			.addVariable("Emote", &CLua::m_pCGameClient->m_pEmoticon, false)
			.addVariable("HUD", &CLua::m_pCGameClient->m_pHud, false)
			.addVariable("Menus", &CLua::m_pCGameClient->m_pMenus, false)
			.addVariable("Voting", &CLua::m_pCGameClient->m_pVoting, false)
			.addVariable("SpecInfo", &CLua::m_pCGameClient->m_Snap.m_SpecInfo, false)
			//.addData("Client", &CGameClient::m_pClient, false)   //"Game" resembles GameClient, Game.Client => Client
			.addVariable("Input", &CLua::m_pCGameClient->m_pControls, false)
			.addVariable("Collision", &CLua::m_pCGameClient->m_pCollision, false)
			.addVariable("Ui", &CLua::m_pCGameClient->m_UI, false)
			.addVariable("RenderTools", &CLua::m_pCGameClient->m_RenderTools, false)
			//pointer to components & stuff from gameclient
			//.addVariable("Local", &CLua::m_pCGameClient->m_Snap, false)
			.addVariable("LocalTee", &CLua::m_pCGameClient->m_PredictedChar, false)
			.addVariable("LocalCID", &CLua::m_pCGameClient->m_Snap.m_LocalClientID, false)
			.addVariable("Client", &CLua::m_pClient, false)
			.addFunction("Players", &CGameClient::LuaGetClientData)
			.addFunction("CharSnap", &CGameClient::LuaGetCharacterInfo)
			.addFunction("Tuning", &CGameClient::LuaGetTuning)
		.endNamespace()

		.beginNamespace("Engine")
			.addVariable("Graphics", &CLua::m_pCGameClient->m_pGraphics) //dunno, this should be maybe an own subspace :O
			.addVariable("TextRender", &CLua::m_pCGameClient->m_pTextRender)
			.addVariable("Input", &CLua::m_pCGameClient->m_pInput) //dunno, this should be maybe an own subspace :O
			.addVariable("Curl", &CLua::m_pCGameClient->m_pCurlWrapper)
		.endNamespace()

		// g_Config stuff... EVERYTHING AT ONCE!
#define MACRO_CONFIG_STR(Name,ScriptName,Len,Def,Save,Desc) \
			.addStaticProperty(#Name, &CConfigProperties::GetConfig_##Name, &CConfigProperties::SetConfig_##Name) \
			.addStaticProperty(#ScriptName, &CConfigProperties::GetConfig_##Name, &CConfigProperties::SetConfig_##Name)

#define MACRO_CONFIG_INT(Name,ScriptName,Def,Min,Max,Save,Desc) \
			.addStaticProperty(#Name, &CConfigProperties::GetConfig_##Name, &CConfigProperties::SetConfig_##Name) \
			.addStaticProperty(#ScriptName, &CConfigProperties::GetConfig_##Name, &CConfigProperties::SetConfig_##Name)

		.beginClass<CConfigProperties>("Config")
			#include <engine/shared/config_variables.h>
		.endClass()

#undef MACRO_CONFIG_STR
#undef MACRO_CONFIG_INT


		// OOP ENDS HERE
	;

	luaL_loadstring(L, "os.exit=nil os.execute=nil os.rename=nil os.remove=nil os.setlocal=nil"); // TODO
	lua_pcall(L, 0, LUA_MULTRET, 0);
	
	if(g_Config.m_Debug)
		dbg_msg("lua", "registering lua bindings complete (L=%p)", L);
}

luabridge::LuaRef CLuaFile::GetFunc(const char *pFuncName)
{
	LuaRef func = getGlobal(m_pLuaState, pFuncName);
	if(func == 0)
		dbg_msg("lua", "error: function '%s' not found in file '%s'", pFuncName, m_Filename.c_str());

	return func;  // return 0 if the function is not found!
}

template<class T>
T CLuaFile::CallFunc(const char *pFuncName) // just for quick access
{
	if(!m_pLuaState)
		return;

	T ret;
	LUA_CALL_FUNC(m_pLuaState, pFuncName, T, ret);
	return ret;
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
	
	lua_resume(m_pLuaState, 0);

	//Status = lua_pcall(m_pLuaState, 0, LUA_MULTRET, 0);
	//if (Status)
	//{
	//	CLua::ErrorFunc(m_pLuaState);
	//	return false;
	//}

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
