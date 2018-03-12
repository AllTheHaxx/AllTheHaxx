#include "luafile.h"
#include "luabinding.h"

#include <base/math.h>
#include <game/collision.h>
//#include <game/client/gameclient.h>
#include <game/client/components/astar.h>
#include <game/client/components/chat.h>
#include <game/client/components/emoticon.h>
#include <game/client/components/controls.h>
#include <game/client/components/hud.h>
#include <game/client/components/irc.h>
#include <game/client/components/menus.h>
#include <game/client/components/voting.h>
#include <engine/console.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/irc.h>
#include <engine/sound.h>
#include <engine/serverbrowser.h>
#include <engine/curlwrapper.h>
#include <engine/textrender.h>
//#include <engine/client/client.h>
#include "lua/luajson.h"


void CLuaFile::RegisterLuaCallbacks(lua_State *L) // LUABRIDGE!
{
	if(g_StealthMode)
		return;

	lua_register(L, "print", CLuaBinding::LuaPrintOverride);
	lua_register(L, "_io_open", CLuaBinding::LuaIO_Open);
	lua_register(L, "throw", CLuaBinding::LuaThrow); // adds an exception, but doesn't jump out like 'error' does
	lua_register(L, "Import", CLuaBinding::LuaImport);
	//lua_register(L, "Exec", CLuaBinding::LuaExec);
	lua_register(L, "KillScript", CLuaBinding::LuaKillScript);
	lua_register(L, "Listdir", CLuaBinding::LuaListdir);
	lua_register(L, "EnterFullscreen", CLuaBinding::LuaEnterFullscreen);
	lua_register(L, "ExitFullscreen", CLuaBinding::LuaExitFullscreen);
	lua_register(L, "ScriptPath", CLuaBinding::LuaScriptPath);
	lua_register(L, "StrIsNetAddr", CLuaBinding::LuaStrIsNetAddr);
	lua_register(L, "GetIRCUserlist", CLuaBinding::LuaGetIrcUserlist);

	// re-bind common functions
	luaL_dostring(L, "dofile = Import");


	getGlobalNamespace(L)

#if defined(CONF_DEBUG)
		.beginNamespace("_debug")
			.addFunction("DumpStack", &CLua::DbgPrintLuaStack)
		.endNamespace()
#endif
		// client namespace XXX: cleanup
		.beginNamespace("_client")
			// external info
			.addFunction("GetPlayerScore", &CLuaBinding::LuaGetPlayerScore)
		.endNamespace()

		// graphics namespace XXX: cleanup
		.beginNamespace("_graphics")
			.addFunction("RenderTexture", &CLuaBinding::LuaRenderTexture)
			.addFunction("RenderQuad", &CLuaBinding::LuaRenderQuadRaw)
		.endNamespace()

		// global types
		.beginClass< vector2_base<float> >("vec2")
			.addConstructor <void (*) (float, float)> ()
			.addFunction("__add", &vector2_base<float>::operator+)
			.addFunction("__sub", &vector2_base<float>::operator-)
			.addFunction("__mul", &vector2_base<float>::operator*)
			.addFunction("__div", &vector2_base<float>::operator/)
			.addFunction("__eq", &vector2_base<float>::operator==)
			.addFunction("__tostring", &vector2_base<float>::tostring)
			.addData("x", &vector2_base<float>::x)
			.addData("y", &vector2_base<float>::y)
			.addData("u", &vector2_base<float>::u)
			.addData("v", &vector2_base<float>::v)
		.endClass()
		.beginClass< vector3_base<float> >("vec3")
			.addConstructor <void (*) (float, float, float)> ()
			.addFunction("__add", &vector3_base<float>::operator+)
			.addFunction("__sub", &vector3_base<float>::operator-)
			.addFunction("__mul", &vector3_base<float>::operator*)
			.addFunction("__div", &vector3_base<float>::operator/)
			.addFunction("__eq", &vector3_base<float>::operator==)
			.addFunction("__tostring", &vector3_base<float>::tostring)
			.addData("x", &vector3_base<float>::x)
			.addData("y", &vector3_base<float>::y)
			.addData("z", &vector3_base<float>::z)
			.addData("r", &vector3_base<float>::r)
			.addData("g", &vector3_base<float>::g)
			.addData("b", &vector3_base<float>::b)
			.addData("h", &vector3_base<float>::h)
			.addData("s", &vector3_base<float>::s)
			.addData("l", &vector3_base<float>::l)
			.addData("v", &vector3_base<float>::v)
		.endClass()
		.beginClass< vector4_base<float> >("vec4")
			.addConstructor <void (*) (float, float, float, float)> ()
			.addFunction("__add", &vector4_base<float>::operator+)
			.addFunction("__sub", &vector4_base<float>::sub)
			.addFunction("__mul", &vector4_base<float>::mul)
			.addFunction("__div", &vector4_base<float>::div)
			.addFunction("__eq", &vector4_base<float>::operator==)
			.addFunction("__tostring", &vector4_base<float>::tostring)
			.addData("r", &vector4_base<float>::r)
			.addData("g", &vector4_base<float>::g)
			.addData("b", &vector4_base<float>::b)
			.addData("a", &vector4_base<float>::a)
			.addData("x", &vector4_base<float>::x)
			.addData("y", &vector4_base<float>::y)
			.addData("z", &vector4_base<float>::z)
			.addData("w", &vector4_base<float>::w)
			.addData("u", &vector4_base<float>::u)
			.addData("v", &vector4_base<float>::v)
		.endClass()

		.beginClass< CUIRect >("UIRect")
			.addConstructor <void (*) (float, float, float, float)> ()
			.addFunction("copy", &CUIRect::LuaCopy)
			.addData("x", &CUIRect::x)
			.addData("y", &CUIRect::y)
			.addData("w", &CUIRect::w)
			.addData("h", &CUIRect::h)
			.addFunction("HSplitMid", &CUIRect::HSplitMid)
			.addFunction("HSplitTop", &CUIRect::HSplitTop)
			.addFunction("HSplitBottom", &CUIRect::HSplitBottom)
			.addFunction("VSplitMid", &CUIRect::VSplitMid)
			.addFunction("VSplitLeft", &CUIRect::VSplitLeft)
			.addFunction("VSplitRight", &CUIRect::VSplitRight)
			.addFunction("Margin", &CUIRect::Margin)
			.addFunction("VMargin", &CUIRect::VMargin)
			.addFunction("HMargin", &CUIRect::HMargin)
		.endClass()

		.beginClass< CTeeRenderInfo >("TeeRenderInfo")
			.addConstructor <void (*) ()> ()
			.addData("Texture", &CTeeRenderInfo::m_Texture)
			.addData("ColorBody", &CTeeRenderInfo::m_ColorBody)
			.addData("ColorFeet", &CTeeRenderInfo::m_ColorFeet)
			.addData("Size", &CTeeRenderInfo::m_Size)
			.addData("GotAirJump", &CTeeRenderInfo::m_GotAirJump)
		.endClass()

		.beginClass< IGraphics::CQuadItem >("QuadItem")
			.addConstructor <void (*) (float, float, float, float)> ()
		.endClass()

		.beginClass< IGraphics::CLineItem >("LineItem")
			.addConstructor <void (*) (float, float, float, float)> ()
		.endClass()

		.beginClass< CButtonContainer >("ButtonContainer")
			.addConstructor <void (*) ()> ()
            //.addFunction("GetID", &CButtonContainer::GetID)
		.endClass()

		.beginClass< CMenus::lua::CEditboxContainer >("EditboxContainer")
			.addConstructor <void (*) ()> ()
            .addFunction("GetString", &CMenus::lua::CEditboxContainer::GetString)
            .addFunction("SetString", &CMenus::lua::CEditboxContainer::SetString)
		.endClass()

		.beginClass< CTextCursor >("TextCursor")
			.addConstructor <void (*) ()> ()
			.addData("Flags", &CTextCursor::m_Flags)
			.addData("LineCount", &CTextCursor::m_LineCount)
			.addData("CharCount", &CTextCursor::m_CharCount)
			.addData("MaxLines", &CTextCursor::m_MaxLines)
			.addData("StartX", &CTextCursor::m_StartX)
			.addData("StartY", &CTextCursor::m_StartY)
			.addData("LineWidth", &CTextCursor::m_LineWidth)
			.addData("X", &CTextCursor::m_X)
			.addData("Y", &CTextCursor::m_Y)
			//.addData("Font", &CTextCursor::m_pFont)
			.addData("FontSize", &CTextCursor::m_FontSize)
		.endClass()

		.beginClass< ISound::CVoiceHandle >("VoiceHandle")
			.addConstructor <void (*) ()> ()
		.endClass()


		.beginClass< CJsonValue >("JsonValue")
			//.addFunction("__tostring", &CJsonValue::ToString) TODO: serialize
			//.addFunction("__tonumber", &CJsonValue::ToNumber)
			.addFunction("Destroy", &CJsonValue::Destroy)
			.addFunction("GetType", &CJsonValue::GetType)
			.addFunction("ToString", &CJsonValue::ToString)
			.addFunction("ToNumber", &CJsonValue::ToNumber)
			.addFunction("ToBoolean", &CJsonValue::ToBoolean)
			.addFunction("ToTable", &CJsonValue::ToTable)
			.addFunction("ToObject", &CJsonValue::ToObject)
		.endClass()

		.beginClass< CLuaJson >("CLuaJson")
		.endClass()

		.beginNamespace("json")
			.addFunction("Parse", &CLuaJson::Parse)
			.addFunction("Convert", &CLuaJson::Convert)
			.addFunction("Serialize", &CLuaJson::Serialize)
		.endNamespace()



		// ------------------------------ ICLIENT ------------------------------

		/// Game.Client
		.beginClass<IClient>("IClient")
			.addProperty("Tick", &IClient::GameTick)
			.addProperty("TickSpeed", &IClient::GameTickSpeed)
			.addProperty("IntraGameTick", &IClient::IntraGameTick)
			.addProperty("PredIntraGameTick", &IClient::PredIntraGameTick)
			.addProperty("PredGameTick", &IClient::PredGameTick)
			.addProperty("LocalTime", &IClient::SteadyTimer)
			.addProperty("ConnTime", &IClient::LocalTime)

			.addFunction("Connect", &IClient::Connect)
			.addFunction("Disconnect", &IClient::Disconnect)

			.addFunction("DummyConnect", &IClient::DummyConnect)
			.addFunction("DummyDisconnect", &IClient::DummyDisconnect)
			.addFunction("DummiesConnected", &IClient::DummiesConnected)
			.addFunction("DummyConnecting", &IClient::DummyConnecting)

			.addFunction("SendInfo", &IClient::SendPlayerInfo)

			.addFunction("RconAuth", &IClient::RconAuth)
			.addFunction("RconAuthed", &IClient::RconAuthed)
			.addFunction("RconSend", &IClient::Rcon)

			.addProperty("FPS", &IClient::GetFPS)
			.addProperty("State", &IClient::State)

			.addFunction("DemoStart", &IClient::DemoRecorder_Start)
			.addFunction("DemoStop", &IClient::DemoRecorder_Stop)

		//	.addFunction("Quit", &IClient::Quit)
		.endClass()


		// ------------------------------ COMPONENTS ------------------------------

		/// Game.Ui
		.beginClass<CUI>("CUI")
			.addFunction("DoLabel", &CUI::DoLabelLua)
			.addFunction("DoLabelScaled", &CUI::DoLabelScaledLua)
			.addFunction("DoButtonLogic", &CUI::DoButtonLogicLua)
			.addFunction("DoPickerLogic", &CUI::DoPickerLogicLua)
			.addFunction("Scale", &CUI::Scale)
			.addFunction("Screen", &CUI::Screen)
			.addFunction("MouseX", &CUI::MouseX)
			.addFunction("MouseY", &CUI::MouseY)
			.addFunction("MouseWorldX", &CUI::MouseWorldX)
			.addFunction("MouseWorldY", &CUI::MouseWorldY)
			.addFunction("MouseButton", &CUI::MouseButton)
			.addFunction("MouseButtonClicked", &CUI::MouseButtonClicked)
			.addFunction("MouseInside", &CUI::MouseInside)
			.addFunction("ClipEnable", &CUI::ClipEnable)
			.addFunction("ClipDisable", &CUI::ClipDisable)
		.endClass()

		/// Game.RenderTools
		.beginClass<CRenderTools>("CRenderTools")
			.addFunction("SelectSprite", &CRenderTools::SelectSpriteLua)
			.addFunction("DrawSprite", &CRenderTools::DrawSprite)
			.addFunction("DrawRoundRect", &CRenderTools::DrawRoundRect)
			.addFunction("DrawRoundRectExt", &CRenderTools::DrawRoundRectExt)
			.addFunction("DrawUIRect", &CRenderTools::DrawUIRect)
			.addFunction("DrawCircle", &CRenderTools::DrawCircle)
			.addFunction("RenderTee", &CRenderTools::RenderTeeLua)
		.endClass()

		/// Game.Chat
		.beginClass<CChat>("CChat")
			.addFunction("Say", &CChat::SayLua)
			.addFunction("Print", &CChat::AddLine)
			.addFunction("AddLine", &CChat::AddLine)
			.addProperty("Mode", &CChat::GetMode)
		.endClass()

		/// Game.Console
		.beginClass<IConsole>("IConsole")
			.addFunction("Print", &IConsole::Print)
			.addFunction("LineIsValid", &IConsole::LineIsValid)
		//	.addFunction("ExecuteLine", &IConsole::ExecuteLine)
		.endClass()

		/// Game.IRC
		.beginClass<IIRC>("IIRC")
			.addFunction("SendMsg", &IIRC::SendMsgLua)
			.addFunction("JoinTo", &IIRC::JoinTo)
			.addFunction("GetNick", &IIRC::GetNickStd)
		/*	.addFunction("GetUserlist", &CLuaBinding::LuaGetIrcUserlist) */
		.endClass()

		/// Game.AStar
		.beginClass<CAStar>("CAStar")
			.addProperty("NumNodes", &CAStar::LuaGetNumNodes)
			.addFunction("GetNode", &CAStar::LuaGetNode)
			.addFunction("InitPathBuilder", &CAStar::InitPathBuilder)
		.endClass()

		/// Game.Sound
		.beginClass<ISound>("ISound")
			.addFunction("PlaySound", &ISound::Play)
			.addFunction("StopSound", &ISound::Stop)
			.addFunction("StopAllSounds", &ISound::StopAll)
			.addFunction("SetChannel", &ISound::SetChannel)
			.addFunction("LoadSoundOpus", &ISound::LoadOpus)
			.addFunction("LoadSoundWave", &ISound::LoadWV)
			.addFunction("LoadSoundOpusMemory", &ISound::LoadOpusFromMem)
			.addFunction("LoadSoundWaveMemory", &ISound::LoadWV)
			.addFunction("UnloadSound", &ISound::UnloadSample)
			.addFunction("MaxDuration", &ISound::GetSampleDuration)
		.endClass()

		/// Game.Emote
		.beginClass<CEmoticon>("CEmoticon")
			.addFunction("Send", &CEmoticon::Emote)
			.addFunction("SendEye", &CEmoticon::EyeEmote)
			.addProperty("Active", &CEmoticon::Active)
		.endClass()

		/// Game.Collision
		.beginClass<CCollision>("CCollision")
			.addProperty("GetMapWidth", &CCollision::GetWidth)
			.addProperty("GetMapHeight", &CCollision::GetHeight)

			.addFunction("Distance", &CCollision::Distance)
			.addFunction("Normalize", &CCollision::Normalize)
			.addFunction("ClosestPointOnLine", &CCollision::ClosestPointOnLine)

			.addFunction("GetTile", &CCollision::GetTileRaw)
			.addFunction("CheckPoint", &CCollision::CheckPointLua)

			.addFunction("IntersectLine", &CCollision::IntersectLine)
			.addFunction("IntersectLineTeleHook", &CCollision::IntersectLineTeleHookLua)
			.addFunction("MovePoint", &CCollision::MovePoint)
			.addFunction("MoveBox", &CCollision::MoveBox)
			.addFunction("TestBox", &CCollision::TestBox)
		.endClass()

		/// Game.HUD
		.beginClass<CHud>("CHud")
			.addFunction("PushNotification", &CHud::PushNotification)
		.endClass()

		/// Game.Menus
		.beginClass<CMenus>("CMenus")
			.addProperty("Active", &CMenus::IsActive)
			.addProperty("ActivePage", &CMenus::GetActivePage, &CMenus::SetActivePage)
			.addProperty("MousePos", &CMenus::GetMousePosRel, &CMenus::SetMousePosRel)
			.addFunction("ButtonColorMul", &CMenus::ButtonColorMul)
			.addFunction("DoButton_Menu", &CMenus::DoButton_Menu)
			.addFunction("DoButton_MenuTab", &CMenus::DoButton_MenuTab)
			.addFunction("DoButton_CheckBox", &CMenus::DoButton_CheckBox)
			.addFunction("DoButton_Toggle", &CMenus::DoButton_Toggle)
			.addFunction("DoButton_CheckBox_Number", &CMenus::DoButton_CheckBox_Number)
			.addFunction("DoButton_Sprite", &CMenus::DoButton_Sprite)
			.addFunction("DoScrollbarV", &CMenus::DoScrollbarV)
			.addFunction("DoScrollbarH", &CMenus::DoScrollbarH)
			.addFunction("DoEditbox", &CMenus::DoEditBoxLua)
			.addFunction("DoColorPicker", &CMenus::DoColorPicker)
		.endClass()

		/// Game.Voting
		.beginClass<CVoting>("CVoting")
			.addFunction("CallvoteSpectate", &CVoting::CallvoteSpectate)
			.addFunction("CallvoteKick", &CVoting::CallvoteKick)
			.addFunction("CallvoteOption", &CVoting::CallvoteOption)
			.addFunction("Vote", &CVoting::Vote)
			.addFunction("VoteYes", &CVoting::VoteYes)
			.addFunction("VoteNo", &CVoting::VoteNo)

			.addProperty("VoteDescription", &CVoting::VoteDescription)
			.addProperty("VoteReason", &CVoting::VoteReason)
			.addProperty("SecondsLeft", &CVoting::SecondsLeft)
			.addProperty("IsVoting", &CVoting::IsVoting)
			.addProperty("TakenChoice", &CVoting::TakenChoice)

			.addProperty("Yes", &CVoting::GetYes)
			.addProperty("No", &CVoting::GetNo)
			.addProperty("Pass", &CVoting::GetPass)
			.addProperty("Total", &CVoting::GetTotal)
		.endClass()

		// local playerinfo
		/// TODO: doc!
		.beginClass<CNetObj_CharacterCore>("CNetObj_CharacterCore")  // TODO : Add the whole class!
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

		/// Game.Snap:PlayerInfos(ID)
		.beginClass<CNetObj_PlayerInfo>("CNetObj_PlayerInfo")
			.addData("Local", &CNetObj_PlayerInfo::m_Local, false)
			.addData("ClientID", &CNetObj_PlayerInfo::m_ClientID, false)
			.addData("Team", &CNetObj_PlayerInfo::m_Team, false)
			.addData("Score", &CNetObj_PlayerInfo::m_Score, false)
			.addData("Latency", &CNetObj_PlayerInfo::m_Latency, false)
		.endClass()


		/// Game.VClient(i).Input
		.beginClass<CNetObj_PlayerInput>("CNetObj_PlayerInput")
			.addData("Direction", &CNetObj_PlayerInput::m_ViewDir)
			.addData("Fire", &CNetObj_PlayerInput::m_FCount)
			.addData("Hook", &CNetObj_PlayerInput::m_Hook)
			.addData("Jump", &CNetObj_PlayerInput::m_Jump)
			.addData("WantedWeapon", &CNetObj_PlayerInput::m_WantedWeapon)
			.addData("TargetX", &CNetObj_PlayerInput::m_AimX)
			.addData("TargetY", &CNetObj_PlayerInput::m_AimY)
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
			.addFunction("__tonumber", &CTuneParam::GetFloat)
			.addProperty("Value", &CTuneParam::Get)
			.addFunction("Get", &CTuneParam::GetFloat)
		.endClass()

#define MACRO_TUNING_PARAM(Name,ScriptName,Value,Description) \
	.addData(#Name, &CTuningParams::m_##Name) \
	.addData(#ScriptName, &CTuningParams::m_##Name)

		.beginClass<CTuningParams>("CTuningParams") /// Game.Tuning()
			#include <game/tuning.h>
		.endClass()

#undef MACRO_TUNING_PARAM

		/// Game.Input
		.beginClass<CControls>("CControls")
			.addProperty("Direction", &CControls::GetDirection, &CControls::SetDirection)
			.addProperty("Fire", &CControls::GetFire, &CControls::SetFire)
			.addProperty("Hook", &CControls::GetHook, &CControls::SetHook)
			.addProperty("Jump", &CControls::GetJump, &CControls::SetJump)
			.addProperty("WantedWeapon", &CControls::GetWantedWeapon, &CControls::SetWantedWeapon)
			.addProperty("TargetX", &CControls::GetTargetX, &CControls::SetTargetX)
			.addProperty("TargetY", &CControls::GetTargetY, &CControls::SetTargetY)
			.addProperty("MouseX", &CControls::GetMouseX, &CControls::SetMouseX)
			.addProperty("MouseY", &CControls::GetMouseY, &CControls::SetMouseY)
			.addFunction("SetScoreboardFlag", &CControls::SetScoreboardFlag)
		.endClass()

		/// Engine.Input
		.beginClass<IInput>("IInput")
			.addFunction("KeyPress", &IInput::KeyPress)
			.addFunction("KeyIsPressed", &IInput::KeyIsPressedLua)
			.addFunction("KeyName", &IInput::KeyNameSTD)
			.addFunction("KeyID", &IInput::GetKeyID)
			.addFunction("GetClipboardText", &IInput::GetClipboardTextSTD)
			.addFunction("SetClipboardText", &IInput::SetClipboardTextSTD)

			.addFunction("MouseModeRelative", &IInput::MouseModeRelative)
			.addFunction("MouseModeAbsolute", &IInput::MouseModeAbsolute)
			.addFunction("MouseDoubleClick", &IInput::MouseDoubleClick)

		//	.addFunction("SimulateKeyPressDirect", &IInput::SimulateKeyPress)
		//	.addFunction("SimulateKeyPress", &IInput::SimulateKeyPressSTD)
		//	.addFunction("SimulateKeyReleaseDirect", &IInput::SimulateKeyRelease)
		//	.addFunction("SimulateKeyRelease", &IInput::SimulateKeyReleaseSTD)

			.addFunction("SetIMEState", &IInput::SetIMEState)
			.addFunction("GetIMEState", &IInput::GetIMEState)
		.endClass()

		/// Engine.Curl
		.beginClass<ICurlWrapper>("ICurlWrapper")
			.addFunction("httpSimplePost", &ICurlWrapper::PerformSimplePOST)
		.endClass()

		/// Game.ServerInfo
		.beginClass<CServerInfo>("CServerInfo")
			.addProperty("GameMode", &CServerInfo::LuaGetGameType)
			.addProperty("Name", &CServerInfo::LuaGetName)
			.addProperty("Map", &CServerInfo::LuaGetMap)
			.addProperty("Version", &CServerInfo::LuaGetVersion)
			.addProperty("IP", &CServerInfo::LuaGetIP)
			.addData("Latency", &CServerInfo::m_Latency, false)
			.addData("MaxPlayers", &CServerInfo::m_MaxPlayers, false)
			.addData("NumPlayers", &CServerInfo::m_NumPlayers, false)
			.addData("MaxClients", &CServerInfo::m_MaxClients, false)
			.addData("NumClients", &CServerInfo::m_NumClients, false)
		.endClass()

		// Game.Snap
		.beginClass<CGameClient::CSnapState>("CSnapState")
			.addData("Tee", &CGameClient::CSnapState::m_pLocalCharacter)
			.addData("ClientID", &CGameClient::CSnapState::m_LocalClientID)

			.addFunction("PlayerInfos", &CGameClient::CSnapState::LuaGetPlayerInfos)
			.addFunction("InfoByScore", &CGameClient::CSnapState::LuaGetInfoByScore)
			.addFunction("InfoByName", &CGameClient::CSnapState::LuaGetInfoByName)
			.addFunction("InfoByDDTeam", &CGameClient::CSnapState::LuaGetInfoByDDTeam)
		.endClass()

		/// Game.CharSnap(ID)
		.beginClass<CGameClient::CSnapState::CCharacterInfo>("CCharacterInfo")
			.addData("Active", &CGameClient::CSnapState::CCharacterInfo::m_Active, false)
			.addData("Cur", &CGameClient::CSnapState::CCharacterInfo::m_Cur, false)
		.endClass()

		/// Game.SpecInfo
		.beginClass<CGameClient::CSnapState::CSpectateInfo>("CSpectateInfo")
			.addData("Active", &CGameClient::CSnapState::CSpectateInfo::m_Active)
			.addData("SpectatorID", &CGameClient::CSnapState::CSpectateInfo::m_SpectatorID)
			.addData("UsePosition", &CGameClient::CSnapState::CSpectateInfo::m_UsePosition)
			.addData("Position", &CGameClient::CSnapState::CSpectateInfo::m_Position)
		.endClass()

		/// Game.Players(ID)
		.beginClass<CGameClient::CClientData>("CClientData")
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
			.addProperty("RenderInfo", &CGameClient::CClientData::GetTeeRenderInfo)
		.endClass()

		/// Engine.Graphics
		.beginClass<IGraphics>("IGraphics")
			.addFunction("QuadsBegin", &IGraphics::QuadsBeginLua)
			.addFunction("QuadsEnd", &IGraphics::QuadsEndLua)
			.addFunction("QuadsDraw", &IGraphics::QuadsDrawLua)
			.addFunction("QuadsDrawTL", &IGraphics::QuadsDrawTLLua)
			.addFunction("LinesBegin", &IGraphics::LinesBeginLua)
			.addFunction("LinesEnd", &IGraphics::LinesEndLua)
			.addFunction("LinesDraw", &IGraphics::LinesDrawLua)

			.addFunction("SetRotation", &IGraphics::QuadsSetRotationLua)
			.addFunction("SetColor", &IGraphics::SetColorLua)
			.addFunction("BlendNone", &IGraphics::BlendNone)
			.addFunction("BlendNormal", &IGraphics::BlendNormal)
			.addFunction("BlendAdditive", &IGraphics::BlendAdditive)

			.addFunction("LoadTexture", &IGraphics::LoadTexture)
			.addFunction("UnloadTexture", &IGraphics::UnloadTexture)
			.addFunction("TextureSet", &IGraphics::TextureSetLua)

			.addFunction("MapScreen", &IGraphics::MapScreen)

			.addProperty("ScreenAspect", &IGraphics::ScreenAspect)
			.addProperty("ScreenWidth", &IGraphics::ScreenWidth)
			.addProperty("ScreenHeight", &IGraphics::ScreenHeight)
			
			.addFunction("ClipEnable", &IGraphics::ClipEnable)
			.addFunction("ClipDisable", &IGraphics::ClipDisable)
		.endClass()

		/// Engine.TextRender
		.beginClass<ITextRender>("ITextRender")
			.addFunction("Text", &ITextRender::Text)
			.addFunction("TextEx", &ITextRender::TextEx)
			.addFunction("TextWidth", &ITextRender::TextWidth)
			.addFunction("TextLineCount", &ITextRender::TextLineCount)
			.addFunction("TextColor", &ITextRender::TextColor)
			.addFunction("TextOutlineColor", &ITextRender::TextOutlineColor)
			.addFunction("SetCursor", &ITextRender::SetCursorLua)
		.endClass()

		/// Engine.Storage
		.beginClass<IStorageTW>("IStorageTW")
			.addFunction("CreateFolder", &IStorageTW::CreateFolderLua)
			.addFunction("CreateDir", &IStorageTW::CreateFolderLua) // alias
		.endClass()

		// XXX: cleanup!
		/// TW.Game
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
			.addFunction("SendKill", &CGameClient::SendKill)
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
			.addVariable("IRC", &CLua::m_pCGameClient->m_pIRC, false)
			.addVariable("AStar", &CLua::m_pCGameClient->m_pAStar, false)
			.addVariable("Sound", &CLua::m_pCGameClient->m_pSound, false)
			.addVariable("Menus", &CLua::m_pCGameClient->m_pMenus, false)
			.addVariable("Voting", &CLua::m_pCGameClient->m_pVoting, false)
			.addVariable("SpecInfo", &CLua::m_pCGameClient->m_Snap.m_SpecInfo, false)
			//.addData("Client", &CGameClient::m_pClient, false)   // "Game" resembles GameClient, Game.Client => Client
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
			.addFunction("GetSnap", &CGameClient::LuaGetFullSnap)
			.addVariable("Snap", &CLua::m_pCGameClient->m_Snap, false)
			.addFunction("Tuning", &CGameClient::LuaGetTuning)
			//dummy access
			.addFunction("DummyInput", &CControls::LuaGetInputData)
		.endNamespace()

		.beginNamespace("Engine")
			.addVariable("Graphics", &CLua::m_pCGameClient->m_pGraphics)
			.addVariable("TextRender", &CLua::m_pCGameClient->m_pTextRender)
			.addVariable("Input", &CLua::m_pCGameClient->m_pInput)
			.addVariable("Storage", &CLua::m_pCGameClient->m_pStorage)
			.addVariable("Curl", &CLua::m_pCGameClient->m_pCurlWrapper)
		.endNamespace()

		// g_Config stuff... EVERYTHING AT ONCE!
		/// Config.<var_name>
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

	if(g_Config.m_Debug)
		dbg_msg("lua/debug", "registering lua bindings complete (L=%p)", L);


	// register legacy type aliases (deprecated)
	luaL_dostring(L, "vec2f=vec2");
	luaL_dostring(L, "vec3f=vec3");
	luaL_dostring(L, "vec4f=vec4");
}
