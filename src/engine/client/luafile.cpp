#include "luafile.h"
#include "lua.h"
#include "luabinding.h"

CLuaFile::CLuaFile(CLua *pLua, std::string Filename) : m_pLua(pLua), m_Filename(Filename)
{
	m_pLuaState = 0;
	m_State = LUAFILE_STATE_IDLE;
	//str_copy(m_aFilename, pFilename, sizeof(m_aFilename));
	Reset();
}

void CLuaFile::Reset()
{
	m_State = LUAFILE_STATE_IDLE;

	mem_zero(m_aScriptTitle, sizeof(m_aScriptTitle));
	mem_zero(m_aScriptInfo, sizeof(m_aScriptInfo));

	m_ScriptHasSettings = false;

	if(m_pLuaState)
		lua_close(m_pLuaState);
	m_pLuaState = luaL_newstate();
}

void CLuaFile::Unload()
{
//	if(m_pLuaState)             // -- do not close it in order to prevent crashes
//		lua_close(m_pLuaState);
	lua_gc(m_pLuaState, LUA_GCCOLLECT, 0);
	m_State = LUAFILE_STATE_IDLE; // -- just change the state so that everybody knews
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
}

void CLuaFile::RegisterLuaCallbacks() // LUABRIDGE!
{
	getGlobalNamespace(m_pLuaState)

		// client namespace
		.beginNamespace("_client")
			.addFunction("Connect", &CLuaBinding::LuaConnect)
			.addFunction("GetTick", &CLuaBinding::LuaGetTick)
			// local info
			.addFunction("GetLocalCharacterID", &CLuaBinding::LuaGetLocalCharacterID)
			//.addFunction("GetLocalCharacterPos", &CLuaBinding::LuaGetLocalCharacterPos)
			.addFunction("GetLocalCharacterWeapon", &CLuaBinding::LuaGetLocalCharacterWeapon)
			.addFunction("GetLocalCharacterWeaponAmmo", &CLuaBinding::LuaGetLocalCharacterWeaponAmmo)
			.addFunction("GetLocalCharacterHealth", &CLuaBinding::LuaGetLocalCharacterHealth)
			.addFunction("GetLocalCharacterArmor", &CLuaBinding::LuaGetLocalCharacterArmor)
			.addFunction("GetFPS", &CLuaBinding::LuaGetFPS)
		.endNamespace()


		// lua namespace
		.beginNamespace("_lua")
			//.addFunction("GetLuaFile", &CLuaFile::LuaGetLuaFile)
			.beginClass<CLuaFile>("CLuaFile")
				.addFunction("SetScriptTitle", &CLuaFile::LuaSetScriptTitle)
				.addFunction("SetScriptInfo", &CLuaFile::LuaSetScriptInfo)
				.addFunction("SetScriptHasSettings", &CLuaFile::LuaSetScriptHasSettings)
			.endClass()
		.endNamespace()


		// ui namespace
		.beginNamespace("_ui")
			.addFunction("SetUiColor", &CLuaBinding::LuaSetUiColor)
			.addFunction("DrawUiRect", &CLuaBinding::LuaDrawUiRect)
			.addFunction("DoButton_Menu", &CLuaBinding::LuaDoButton_Menu)
		.endNamespace()


		// components namespace
		.beginNamespace("_game")
			.beginNamespace("chat")
				.addFunction("Send", &CLuaBinding::LuaChatSend)
				.addFunction("Active", &CLuaBinding::LuaChatActive)
				.addFunction("AllActive", &CLuaBinding::LuaChatAllActive)
				.addFunction("TeamActive", &CLuaBinding::LuaChatTeamActive)

			.endNamespace()

			.beginNamespace("collision")
				.addFunction("GetMapWidth", &CLuaBinding::LuaColGetMapWidth)
				.addFunction("GetMapHeight", &CLuaBinding::LuaColGetMapHeight)
				.addFunction("GetTile", &CLuaBinding::LuaColGetTile)
			.endNamespace()

			.beginNamespace("emote")
				.addFunction("Send", &CLuaBinding::LuaEmoteSend)
			.endNamespace()
		.endNamespace()


		// graphics namespace
		.beginNamespace("_graphics")
			.addFunction("GetScreenWidth", &CLuaBinding::LuaGetScreenWidth)
			.addFunction("GetScreenHeight", &CLuaBinding::LuaGetScreenHeight)
			.addFunction("BlendNone", &CLuaBinding::LuaBlendNone)
			.addFunction("BlendNormal", &CLuaBinding::LuaBlendNormal)
			.addFunction("BlendAdditive", &CLuaBinding::LuaBlendAdditive)
			.addFunction("SetColor", &CLuaBinding::LuaSetColor)
			.addFunction("LoadTexture", &CLuaBinding::LuaLoadTexture)
			.addFunction("RenderTexture", &CLuaBinding::LuaRenderTexture)
		.endNamespace()

	;
	dbg_msg("Lua", "Registering LuaBindings complete.");
}

luabridge::LuaRef CLuaFile::GetFunc(const char *pFuncName)
{
//	bool nostate = m_pLuaState == 0;
//	if(nostate)                       // SOOO MUCH HACK
//		m_pLuaState = luaL_newstate();

	LuaRef func = getGlobal(m_pLuaState, pFuncName);

//	if(nostate)
//		Unload(); // MUCH MUCH HACK

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
			str_comp_nocase(&pFilename[str_length(pFilename)]-4, ".lua") || !m_pLuaState)
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


// lua namespace
void CLuaFile::LuaSetScriptTitle(std::string Title)
{
	str_copy(m_aScriptTitle, Title.c_str(), sizeof(m_aScriptTitle));
}

void CLuaFile::LuaSetScriptInfo(std::string Infotext)
{
	str_copy(m_aScriptInfo, Infotext.c_str(), sizeof(m_aScriptInfo));
}

void CLuaFile::LuaSetScriptHasSettings(int Val)
{
	m_ScriptHasSettings = Val > 0;
}
