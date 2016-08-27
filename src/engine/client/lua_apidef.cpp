#include "luafile.h"
#include "luabinding.h"

#include <base/math.h>
#include <game/collision.h>
//#include <game/client/gameclient.h>
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


void CLuaFile::RegisterLuaCallbacks(lua_State *L) // LUABRIDGE!
{
#if defined(FEATURE_LUA)

	lua_register(L, "Import", CLuaBinding::LuaImport);
	lua_register(L, "KillScript", CLuaBinding::LuaKillScript);
	lua_register(L, "Listdir", CLuaBinding::LuaListdir);
	lua_register(L, "ScriptPath", CLuaBinding::LuaScriptPath);

	getGlobalNamespace(L)

		//.addFunction("Import", &CLuaBinding::LuaImport)
		//.addFunction("KillScript", &CLuaBinding::LuaKillScript)
		//.addFunction("print", &CLuaFile::LuaPrintOverride)       // TODO: do this with a low level implementation :D

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


		// ------------------------------ ICLIENT ------------------------------

		/// Game.Client
		.beginClass<IClient>("IClient")
			.addProperty("Tick", &IClient::GameTick)
			.addProperty("IntraGameTick", &IClient::IntraGameTick)
			.addProperty("PredIntraGameTick", &IClient::PredIntraGameTick)
			.addProperty("PredGameTick", &IClient::PredGameTick)
			.addProperty("LocalTime", &IClient::SteadyTimer)
			.addProperty("ConnTime", &IClient::LocalTime)

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


		// ------------------------------ COMPONENTS ------------------------------

		/// Game.UI
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

		/// Game.RenderTools
		.beginClass<CRenderTools>("CRenderTools")
			.addFunction("SelectSprite", &CRenderTools::SelectSpriteLua)
			.addFunction("DrawSprite", &CRenderTools::DrawSprite)
			.addFunction("DrawRoundRect", &CRenderTools::DrawRoundRect)
			.addFunction("DrawRoundRectExt", &CRenderTools::DrawRoundRectExt)
			.addFunction("DrawUIRect", &CRenderTools::DrawUIRect)
			.addFunction("DrawCircle", &CRenderTools::DrawCircle)
		.endClass()

		/// Game.Chat
		.beginClass<CChat>("CChat")
			.addFunction("Say", &CChat::Say)
			.addFunction("Print", &CChat::AddLine)
			.addFunction("AddLine", &CChat::AddLine)
			.addProperty("Mode", &CChat::GetMode)
		.endClass()

		/// Game.Console
		.beginClass<IConsole>("IConsole")
			.addFunction("Print", &IConsole::Print)
			.addFunction("LineIsValid", &IConsole::LineIsValid)
			.addFunction("ExecuteLine", &IConsole::ExecuteLine)
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

		/// Game.Voting
		.beginClass<CVoting>("CVoting")
			.addFunction("CallvoteSpectate", &CVoting::CallvoteSpectate)
			.addFunction("CallvoteKick", &CVoting::CallvoteKick)
			.addFunction("CallvoteOption", &CVoting::CallvoteOption)
			.addFunction("Vote", &CVoting::Vote)

			.addProperty("SecondsLeft", &CVoting::SecondsLeft)
			.addProperty("IsVoting", &CVoting::IsVoting)
			.addProperty("TakenChoice", &CVoting::TakenChoice)
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

			.addFunction("SimulateKeyPressDirect", &IInput::SimulateKeyPress)
			.addFunction("SimulateKeyPress", &IInput::SimulateKeyPressSTD)
			.addFunction("SimulateKeyReleaseDirect", &IInput::SimulateKeyRelease)
			.addFunction("SimulateKeyRelease", &IInput::SimulateKeyReleaseSTD)
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
		.endClass()

		/// TODO: doc!
		.beginClass<CGameClient::CSnapState>("CSnapState")
			.addData("Tee", &CGameClient::CSnapState::m_pLocalCharacter)
			.addData("ClientID", &CGameClient::CSnapState::m_LocalClientID)
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
		.endClass()

		/// Engine.Graphics
		.beginClass<IGraphics>("IGraphics")
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

		/// Engine.TextRender
		.beginClass<ITextRender>("ITextRender")
			.addFunction("Text", &ITextRender::Text)
			.addFunction("TextWidth", &ITextRender::TextWidth)
			.addFunction("TextLineCount", &ITextRender::TextLineCount)
			.addFunction("TextColor", &ITextRender::TextColor)
			.addFunction("TextOutlineColor", &ITextRender::TextOutlineColor)
		.endClass()

		// XXX: cleanup!
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
			.addFunction("Tuning", &CGameClient::LuaGetTuning)
		.endNamespace()

		.beginNamespace("Engine")
			.addVariable("Graphics", &CLua::m_pCGameClient->m_pGraphics)
			.addVariable("TextRender", &CLua::m_pCGameClient->m_pTextRender)
			.addVariable("Input", &CLua::m_pCGameClient->m_pInput)
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

	// kill everything malicious
	luaL_loadstring(L, "os.exit=nil os.execute=nil os.rename=nil os.remove=nil os.setlocal=nil dofile=nil require=nil");
	lua_pcall(L, 0, LUA_MULTRET, 0);

	if(g_Config.m_Debug)
		dbg_msg("lua", "registering lua bindings complete (L=%p)", L);

#endif
}
