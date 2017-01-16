#include <fstream>

#include <base/math.h>
#include <game/localization.h>

#include "luafile.h"
#include "lua.h"
#include "luabinding.h"

// little helper
#if defined(FEATURE_LUA)
#define LUA_CALL_FUNC(LUA_STATE, FUNC_NAME, TYPE, RETURN, ...) { try { \
	LuaRef func = getGlobal(LUA_STATE, FUNC_NAME); \
	if(func) \
		RETURN = func(__VA_ARGS__).cast<TYPE>(); }\
	catch (std::exception& e) \
	{ Lua()->HandleException(e, LUA_STATE); } }
#else
#define LUA_CALL_FUNC(LUA_STATE, FUNC_NAME, TYPE, RETURN, ...) ;;
#endif

CLuaFile::CLuaFile(CLua *pLua, std::string Filename, bool Autoload) : m_pLua(pLua), m_Filename(Filename), m_ScriptAutoload(Autoload)
{
	m_pLuaState = 0;
	m_State = STATE_IDLE;
	Reset();
}

CLuaFile::~CLuaFile()
{
	Unload();
}

void CLuaFile::Reset(bool error)
{
	mem_zero(m_aScriptTitle, sizeof(m_aScriptTitle));
	mem_zero(m_aScriptInfo, sizeof(m_aScriptInfo));

	if(!error)
		m_pErrorStr = 0;

	m_PermissionFlags = 0;
	LoadPermissionFlags(m_Filename.c_str());

	m_ScriptHasSettingsPage = false;

	m_State = error ? STATE_ERROR : STATE_IDLE;
}

void CLuaFile::LoadPermissionFlags(const char *pFilename) // this is the interface for non-compiled scripts
{
#if defined(FEATURE_LUA)
	if(str_comp_nocase(&pFilename[str_length(pFilename)]-4, ".lua") != 0 || str_comp_nocase(&pFilename[str_length(pFilename)]-9, ".conf.lua") == 0) // clc's and config files won't have permission flags!
		return;

	std::ifstream f(pFilename);
	std::string line; bool searching = true;
	while(std::getline(f, line))
	{
		if(searching && line != "--[[#!")
			continue;

		if(searching)
		{
			searching = false;
			continue;
		}
		else if(line.find("]]") != std::string::npos)
			break;

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
			m_PermissionFlags |= PERMISSION_IO;
		if(str_comp_nocase("debug", p) == 0)
			m_PermissionFlags |= PERMISSION_DEBUG;
		if(str_comp_nocase("ffi", p) == 0)
			m_PermissionFlags |= PERMISSION_FFI;
		if(str_comp_nocase("os", p) == 0)
			m_PermissionFlags |= PERMISSION_OS;
		if(str_comp_nocase("package", p) == 0)
			m_PermissionFlags |= PERMISSION_PACKAGE;
	}

#endif
}

void CLuaFile::Unload(bool error)
{
#if defined(FEATURE_LUA)
//	if(m_pLuaState)			 // -- do not close it in order to prevent crashes
//		lua_close(m_pLuaState);

	// script is not loaded -> don't unload it
	if(m_State != STATE_LOADED)
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
		m_pLua->HandleException(e, this);
	}

	lua_gc(m_pLuaState, LUA_GCCOLLECT, 0);
	Reset(error);
#endif
}

void CLuaFile::OpenLua()
{
#if defined(FEATURE_LUA)
	if(m_pLuaState)
		lua_close(m_pLuaState);

	m_pLuaState = luaL_newstate();

	lua_atpanic(m_pLuaState, CLua::Panic);
	lua_register(m_pLuaState, "errorfunc", CLua::ErrorFunc);
	//lua_register(m_pLuaState, "print", CLuaFile::LuaPrintOverride);

	//luaL_openlibs(m_pLuaState);  // we don't need certain libs -> open them all manually

	luaopen_base(m_pLuaState);	// base
	luaopen_math(m_pLuaState);	// math.* functions
	luaopen_string(m_pLuaState);// string.* functions
	luaopen_table(m_pLuaState);	// table operations
	luaopen_bit(m_pLuaState);	// bit operations
	//luaopen_jit(m_pLuaState);	// control the jit-compiler [don't needed]

#endif
}

void CLuaFile::ApplyPermissions(int Flags)
{
	if(Flags&PERMISSION_IO)
		luaopen_io(m_pLuaState);	// input/output of files
	//if(Flags&PERMISSION_DEBUG) XXX
	luaopen_debug(m_pLuaState);	// debug stuff for whatever... can be removed in further patches
	if(Flags&PERMISSION_FFI)
		luaopen_ffi(m_pLuaState);	// register and write own C-Functions and call them in lua (whoever may need that...)
	//if(Flags&PERMISSION_OS) XXX
	luaopen_os(m_pLuaState);	// evil
	if(Flags&PERMISSION_PACKAGE)
		luaopen_package(m_pLuaState); //used for modules etc... not sure whether we should load this

}

void CLuaFile::Init()
{
#if defined(FEATURE_LUA)
	if(m_State == STATE_LOADED)
		Unload();

	m_Exceptions.clear();

	m_State = STATE_IDLE;

	OpenLua(); // create the state, open basic libraries

	if(!LoadFile("data/luabase/events.lua", false)) // load all default event callbacks
	{
		m_State = STATE_ERROR;
		m_pErrorStr = Localize("Failed to load 'data/luabase/events.lua'");
	}
	else // if successful
	{
		RegisterLuaCallbacks(m_pLuaState);
		if(LoadFile(m_Filename.c_str(), false))
			m_State = STATE_LOADED;
		else
			m_State = STATE_ERROR;
	}

	// if we errored so far, don't go any further
	if(m_State == STATE_ERROR)
	{
		Reset(true);
		return;
	}

	// gather basic global infos from the script
	lua_getglobal(m_pLuaState, "g_ScriptTitle");
	if(lua_isstring(m_pLuaState, -1))
		str_copy(m_aScriptTitle, lua_tostring(m_pLuaState, -1), sizeof(m_aScriptTitle));
	lua_pop(m_pLuaState, 1);

	lua_getglobal(m_pLuaState, "g_ScriptInfo");
	if(lua_isstring(m_pLuaState, -1))
		str_copy(m_aScriptInfo, lua_tostring(m_pLuaState, -1), sizeof(m_aScriptInfo));
	lua_pop(m_pLuaState, 1);

	if(str_find_nocase(m_aScriptTitle, " b| ") || str_find_nocase(m_aScriptInfo, " b| ")) { Unload(false); return; }

	// call the OnScriptInit function if we have one
	if(!CallFunc<bool>("OnScriptInit", true))
	{
		dbg_msg("lua", "script '%s' rejected being loaded, did 'OnScriptInit()' return true...?", m_Filename.c_str());
		m_pErrorStr = Localize("OnScriptInit() didn't return true");
		Unload(true);
		return;
	}

	m_ScriptHasSettingsPage |= ScriptHasSettingsPage();

#endif
}


#if defined(FEATURE_LUA)
luabridge::LuaRef CLuaFile::GetFunc(const char *pFuncName)
{
	return luabridge::getGlobal(m_pLuaState, pFuncName);
}
#endif

template<class T>
T CLuaFile::CallFunc(const char *pFuncName, T def) // just for quick access
{
	if(!m_pLuaState)
		return (T)0;

	T ret = def;
	LUA_CALL_FUNC(m_pLuaState, pFuncName, T, ret);
	return ret;
}

bool CLuaFile::LoadFile(const char *pFilename, bool Import)
{
#if defined(FEATURE_LUA)
	if(!pFilename || pFilename[0] == '\0' || str_length(pFilename) <= 4 ||
			(str_comp_nocase(&pFilename[str_length(pFilename)]-4, ".lua") &&
			str_comp_nocase(&pFilename[str_length(pFilename)]-4, ".clc") /*&&
			str_comp_nocase(&pFilename[str_length(pFilename)]-7, ".config")*/) || !m_pLuaState)
		return false;

	// some security steps right here...
	int BeforePermissions = Import ? m_PermissionFlags : 0;
	LoadPermissionFlags(pFilename);
	ApplyPermissions(m_PermissionFlags & ~BeforePermissions); // only apply those that are new

	// kill everything malicious
	static const char s_aBlacklist[][64] = {
			"os.exit",
			"os.execute",
			"os.rename",
			"os.remove",
			"os.setlocal",
			"dofile",
			"require",
			"load",
			"loadfile",
			"loadstring",
	};
	for(unsigned i = 0; i < sizeof(s_aBlacklist)/sizeof(s_aBlacklist[0]); i++)
	{
		char aCmd[128];
		str_format(aCmd, sizeof(aCmd), "%s=nil", s_aBlacklist[i]);
		luaL_dostring(m_pLuaState, aCmd);
		if(g_Config.m_Debug)
			dbg_msg("lua", "disabler: '%s'", aCmd);
	}


	// make sure that source code scripts are what they're supposed to be
	IOHANDLE f = io_open(pFilename, IOFLAG_READ);
	if(!f)
	{
		if(g_Config.m_Debug)
			dbg_msg("Lua/debug", "Could not load file '%s' (file not accessible)", pFilename);
		return false;
	}

	if(g_Config.m_Debug)
		dbg_msg("lua/debug", "loading '%s' with flags %i", pFilename, m_PermissionFlags);


	char aData[sizeof(LUA_SIGNATURE)] = {0};
	io_read(f, aData, sizeof(aData));
	io_close(f);
	char aHeader[2][7];
	str_format(aHeader[0], sizeof(aHeader[0]), "\\x%02x%s", aData[0], aData+1);
	str_format(aHeader[1], sizeof(aHeader[1]), "\\x%02x%s", LUA_SIGNATURE[0], LUA_SIGNATURE+1);

	if(str_comp(aHeader[0], aHeader[1]) == 0)
	{
		dbg_msg("lua", "!! WARNING: YOU CANNOT LOAD PRECOMPILED SCRIPTS FOR SECURITY REASONS !!");
		dbg_msg("lua", "!! :  %s", pFilename);
		m_pErrorStr = Localize("Cannot load bytecode scripts!");
		return false;
	}

	int Status = luaL_loadfile(m_pLuaState, pFilename);
	if (Status != 0)
	{
		CLua::ErrorFunc(m_pLuaState);
		return false;
	}

	if(Import)
		Status = lua_pcall(m_pLuaState, 0, LUA_MULTRET, 0); // execute imported files straight away to get all their stuff
	else
		Status = lua_resume(m_pLuaState, 0);

	if (Status != 0)
	{
		CLua::ErrorFunc(m_pLuaState);
		return false;
	}

	return true;
#else
	return false;
#endif
}

bool CLuaFile::ScriptHasSettingsPage()
{
#if defined(FEATURE_LUA)
	LuaRef func1 = GetFunc("OnScriptRenderSettings");
	LuaRef func2 = GetFunc("OnScriptSaveSettings");
	if(func1.cast<bool>() && func2.cast<bool>())
		return true;
#endif
	return false;
}
