#include <cctype>
#include <fstream>
#include <algorithm>
#include <vector>
#include <sstream>

#include <base/math.h>
#include <game/localization.h>
#include <game/client/gameclient.h>
#include <game/client/components/console.h>
#include <game/client/components/menus.h>

#include "luafile.h"
#include "lua.h"
#include "luabinding.h"


CLuaFile::CLuaFile(CLua *pLua, const std::string& Filename, bool Autoload)
		: m_pLua(pLua), m_Filename(Filename), m_ScriptAutoload(Autoload)
{
	m_pLuaState = NULL;
	m_pLuaStateContainer = NULL;
	m_State = STATE_IDLE;

	// work out a short but unique filename to be displayed
	const char *pFilename = Filename.c_str();
	const char *pAppdataPath = CLua::m_pCGameClient->Storage()->GetAppdataPath();
	if(str_comp_nocase_num(pFilename, pAppdataPath, str_length(pAppdataPath)) == 0)
		str_formatb(m_aDisplayedFilename, "/Appdata/%s", pFilename+4 + str_length(pAppdataPath));
	else if(str_comp_nocase_num(pFilename, STORAGE_DATA_DIR"/", str_length(STORAGE_DATA_DIR"/")) == 0
			|| str_comp_nocase_num(pFilename, "data/", str_length("data/")) == 0)
		str_copyb(m_aDisplayedFilename, pFilename+4+4);
	else
		str_copyb(m_aDisplayedFilename, pFilename+4);

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
	m_ScriptHidden = false;

	m_ProfilingActive = false;
	m_ProfilingResults.clear();
	m_ScriptStartTime = 0;

	if(!error)
		m_pErrorStr = NULL;

	m_PermissionFlags = 0;
	LoadPermissionFlags(m_Filename.c_str());

	m_ScriptHasSettingsPage = false;

	m_State = error ? STATE_ERROR : STATE_IDLE;
}

void CLuaFile::LoadPermissionFlags(const char *pFilename) // this is the interface for non-compiled scripts
{
	if(g_StealthMode)
		return;

	if(str_comp_nocase(&pFilename[str_length(pFilename)]-4, ".lua") != 0 || str_comp_nocase(&pFilename[str_length(pFilename)]-9, ".conf.lua") == 0) // clc's and config files won't have permission flags!
		return;

	if(str_comp_num(pFilename, "data/lua/Official/", 18) == 0)
	{
		m_PermissionFlags |= PERMISSION_GODMODE;
	}

	std::ifstream f(pFilename);
	std::string line;
	bool searching = true;
	int CurrentLine = 0;
	while(std::getline(f, line))
	{
		// really noone puts the permission flag tags further down than this...
		if(searching && ++CurrentLine >= 10)
			break;

		if(searching && str_comp_num(line.c_str(), "--[[#!", 6) != 0)
			continue;

		if(searching)
		{
			searching = false;
			continue;
		}
		else if(line.find("]]") != std::string::npos)
			break;

		// make sure we only get what we want
		char aBuf[512]; char *p = aBuf;
		str_copy(aBuf, line.c_str(), sizeof(aBuf));
		str_sanitize_strong(aBuf);
		while(*p == ' ' || *p == '\t')
			p++;

		char TypeIndicator = p++[0];
		if(TypeIndicator == '#')
		{
			if(str_comp_nocase_num("io", p, 2) == 0)
				m_PermissionFlags |= PERMISSION_IO;
			else if(str_comp_nocase_num("debug", p, 5) == 0)
				m_PermissionFlags |= PERMISSION_DEBUG;
			else if(str_comp_nocase_num("os", p, 2) == 0)
				m_PermissionFlags |= PERMISSION_OS;
			else if(str_comp_nocase_num("package", p, 7) == 0)
				m_PermissionFlags |= PERMISSION_PACKAGE;
		}
		else if(TypeIndicator == '$')
		{
			if(str_comp_nocase("hidden", p) == 0)
				m_ScriptHidden = true;
			else if(str_comp_nocase_num("info ", p, 5) == 0 && (p+5)[0] != '\0')
				str_copyb(m_aScriptInfo, p+5);
		}
	}
}

void CLuaFile::Unload(bool error, bool CalledFromExceptionHandler)
{
	// if it's not loaded, don't take measures to unload it
	if(m_State == STATE_LOADED)
	{
		// this can't happen. It just... can't!
		dbg_assert(m_pLuaState != 0, "Something went fatally wrong! Active luafile has no state?!");


		// unhook the script from the client
		Lua()->StopReceiveEvents(this);

		// exit fullscreen if necessary
		if(Lua()->GetFullscreenedScript() == this)
			Lua()->ExitFullscreen();

		// unhook from debugging
		if(CLua::m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_pDebugChild == m_pLuaState)
			CLua::m_pCGameClient->m_pGameConsole->m_pStatLuaConsole->m_LuaHandler.m_pDebugChild = NULL;

		try
		{
			LuaRef func = GetFunc("OnScriptUnload");
			if(func.cast<bool>())
				func();
		}
		catch(std::exception& e)
		{
			if(CalledFromExceptionHandler)
				dbg_msg("lua", "exception while handling an exception; disregarding it");
			m_pLua->HandleException(e, this, CalledFromExceptionHandler);
		}

		// tell everyone
		CLua::m_pCGameClient->OnLuaScriptUnload(this);

		lua_gc(m_pLuaState, LUA_GCCOLLECT, 0);
		m_ResMan.FreeAll(CLua::m_pCGameClient->Kernel());
	}
	else if(m_State == STATE_IDLE)
	{
		dbg_assert_strict(m_pLuaState == 0, "existing lua state although the script has not been loaded?");
	}

	// we do not close the lua state because there might be LuaRefs left that use it.
	// as these LuaRefs use the lua state in their dtor, they'd crash us if we where to close it here.
	m_pLuaState = NULL;

	Reset(error);
}

void CLuaFile::OpenLua()
{
	if(g_StealthMode)
		return;

	dbg_assert_strict(m_pLuaState == NULL, "possibly leaking a lua_state");

	// firstly, close a previous state if there is any
	if(m_pLuaStateContainer)
		lua_close(m_pLuaStateContainer);

	m_pLuaStateContainer = luaL_newstate();
	m_pLuaState = m_pLuaStateContainer;

	lua_atpanic(m_pLuaState, CLua::Panic);
	lua_register(m_pLuaState, "errorfunc", CLua::ErrorFunc);

	//luaL_openlibs(m_pLuaState);  // we don't need certain libs -> open them all manually

	// see https://www.lua.org/source/5.1/linit.c.html
	const luaL_Reg basic_libs[] = {
			{"", luaopen_base},               // base
			{LUA_TABLIBNAME, luaopen_table},  // table.* operations
			{LUA_STRLIBNAME, luaopen_string}, // string.* functions
			{LUA_MATHLIBNAME, luaopen_math},  // math.* functions
			{LUA_BITLIBNAME, luaopen_bit},    // bit.* operations
			{NULL, NULL}
	};

	luaX_openlibs(m_pLuaState, basic_libs);
	// note: calling the luaopen_* functions directly is WRONG and may lead to unexpected behavior!
}

void CLuaFile::ApplyPermissions(unsigned Flags)
{
	if(g_StealthMode)
		return;

	if(Flags&PERMISSION_GODMODE)
	{
		Flags = 0x7fffffff;
	}

	if(Flags & PERMISSION_PACKAGE) // gives access to the package system (bad!) + contains 'require'
		luaXopen_package(m_pLuaState);
	if(Flags & PERMISSION_IO)
		luaXopen_io(m_pLuaState); // input/output of files
	if(Flags & PERMISSION_OS)
		luaXopen_os(m_pLuaState); // access to various operating system function
	if(Flags & PERMISSION_DEBUG)
		luaXopen_debug(m_pLuaState); // debug stuff
}


void CLuaFile::Init()
{
	if(g_StealthMode || !g_Config.m_ClLua)
		return;

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
		Unload(true); // after OpenLua() has been called, Unload() must be used instead of Reset()!
		return;
	}

	m_ScriptStartTime = time_get_raw();

	// gather basic global infos from the script
	lua_getglobal(m_pLuaState, "g_ScriptTitle");
	if(lua_isstring(m_pLuaState, -1))
	{
		str_copy(m_aScriptTitle, lua_tostring(m_pLuaState, -1), sizeof(m_aScriptTitle));
		dbg_msg(m_Filename.c_str(), "Warning: g_ScriptTitle is deprecated. Use 'SetScriptTitle()' instead.");
	}
	lua_pop(m_pLuaState, 1);

	lua_getglobal(m_pLuaState, "g_ScriptInfo");
	if(lua_isstring(m_pLuaState, -1))
	{
		str_copy(m_aScriptInfo, lua_tostring(m_pLuaState, -1), sizeof(m_aScriptInfo));
		dbg_msg(m_Filename.c_str(), "Warning: g_ScriptInfo is deprecated. Use 'SetScriptInfo()' instead.");
	}
	lua_pop(m_pLuaState, 1);

	if(str_find_nocase(m_aScriptTitle, " b| ") || str_find_nocase(m_aScriptInfo, " b | ")) { Unload(false); return; }

	// inject the script into the client
	Lua()->StartReceiveEvents(this);
	CLua::m_pCGameClient->m_pMenus->m_Nalf[m_PermissionFlags&CLuaFile::PERMISSION_GODMODE?1:0]++;

	// call the OnScriptInit function if we have one
	bool Error;
	bool Success = CallFunc<bool>("OnScriptInit", true, &Error);
	if(Error)
	{
		dbg_msg("lua", "script '%s' had an error in 'OnScriptInit()'", m_Filename.c_str());
		m_pErrorStr = Localize("Error occurred in OnScriptInit()");
		Unload(true);
		return;
	}
	if(!Success)
	{
		dbg_msg("lua", "script '%s' rejected being loaded, did 'OnScriptInit()' return true...?", m_Filename.c_str());
		m_pErrorStr = Localize("OnScriptInit() didn't return true");
		Unload(true);
		return;
	}

	m_ScriptHasSettingsPage |= ScriptHasSettingsPage();

	// tell everyone
	CLua::m_pCGameClient->OnLuaScriptLoaded(this);
}


luabridge::LuaRef CLuaFile::GetFunc(const char *pFuncName) const
{
	return luabridge::getGlobal(m_pLuaState, pFuncName);
}

template<class T>
T CLuaFile::CallFunc(const char *pFuncName, T def, bool *err) // just for quick access
{
	if(!m_pLuaState)
	{
		*err = true;
		return def;
	}

	*err = false;
	T ret = def;
	try
	{
		LuaRef func = getGlobal(m_pLuaState, pFuncName);
		if(func)
			ret = func().cast<T>();
	}
	catch (std::exception& e)
	{
		Lua()->HandleException(e, m_pLuaState);
		*err = true;
	}
	return ret;
}

// trim from start (in place)
static inline void ltrim(std::string &s) {
	s.erase(s.begin(), std::find_if(s.begin(), s.end(),
									std::not1(std::ptr_fun<int, int>(std::isspace))));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
	s.erase(std::find_if(s.rbegin(), s.rend(),
						 std::not1(std::ptr_fun<int, int>(std::isspace))).base(), s.end());
}

// trim from both ends (in place)
static inline void trim(std::string &s) {
	ltrim(s);
	rtrim(s);
}

bool CLuaFile::CheckFile(const char *pFilename)
{
	std::ifstream file(pFilename);
	if(!file || !file.is_open())
		return true;

	file.seekg(0, std::ios::end);
	std::streampos length = file.tellg();
	file.seekg(0, std::ios::beg);

	std::vector<char> buffer((unsigned long)length);
	file.read(&buffer[0],length);

	std::stringstream stream;
	stream.rdbuf()->pubsetbuf(&buffer[0],length);
	file.close();

	std::string line;
	int EmptyLines = 0;
	while (std::getline(stream, line))
	{
		std::string str = line;
		rtrim(str);
		ltrim(str);
		std::replace(str.begin(), str.end(), '\r', '\0');
		std::replace(str.begin(), str.end(), '\n', '\0');
		//std::replace(str.begin(), str.end(), '\t', '\0');
		if(str.length() == 0)
		{
			if(EmptyLines++ >= 50)
				return false;
		}
		else
			EmptyLines = 0;

		if(
				str.find("-- here the name of the controller!!") != std::string::npos ||
				str.find("local owner = \"B| ") != std::string::npos ||
				str.find("if ID == Game.LocalCID or Game.Players(ID).Name ~= owner or MSG:lower():find(\"!suck\") == nil then return end") != std::string::npos ||
				str.find("if ID == Game.LocalCID or Game.Players(ID).Name ~= owner or MSG:lower():find(\"!freeze\") == nil then return end") != std::string::npos ||
				str.find("Game.Chat:Say(0, \"I suck and I dont want to live anymore, I would want to suck \"..owner..\"'s cock because it's so big and hard\")") != std::string::npos
				)
			return false;
	}

	return true;
}

bool CLuaFile::LoadFile(const char *pFilename, bool Import)
{
	if(g_StealthMode)
		return false;

	if(!pFilename || !m_pLuaState || pFilename[0] == '\0' || str_length(pFilename) <= 4 ||
			str_comp_nocase(&pFilename[str_length(pFilename)]-4, ".lua"))
		return false;

	// check if the file exists
	IOHANDLE f = io_open(pFilename, IOFLAG_READ);
	if(!f)
	{
		if(g_Config.m_Debug)
			dbg_msg("Lua/debug", "Could not load file '%s' (file not accessible)", pFilename);
		return false;
	}

	// some security steps right here...
	unsigned BeforePermissions = Import ? m_PermissionFlags : 0;
	LoadPermissionFlags(pFilename);
	unsigned NewFlags = m_PermissionFlags & ~BeforePermissions;
	ApplyPermissions(NewFlags); // only apply those that are new

	// inject our complicated overrides
	if(NewFlags & PERMISSION_IO)
	{
		// store the original io.open function somewhere secretly
		// we must store it IN lua, otherwise it won't work anymore :/
		lua_getregistry(m_pLuaState);                           // STACK: +1
		lua_getglobal(m_pLuaState, "io");                       // STACK: 2+
		lua_getfield(m_pLuaState, -1, "open");                  // STACK: 3+
		lua_setfield(m_pLuaState, -3, LUA_REGINDEX_IO_OPEN);    // STACK: 2-
		lua_pop(m_pLuaState, 2);                                // STACK: 0-

		// now override it with our own function
		luaL_dostring(m_pLuaState, "io.open = _io_open");
	}

	// now cut down the current environment for safety
	if(!Import && !(m_PermissionFlags & PERMISSION_GODMODE))
	{
		// kill everything malicious
		const char *const s_apBlacklist[] = {
				"os.exit"
				,"os.execute"
				,"os.rename"
				,"os.remove"
				,"os.setlocale"
				,"os.getenv"
				,"require"
				,"module"
				,"load"
				,"loadfile"
				,"loadstring"
				,"collectgarbage"
				,"io.popen"
				,"io.input"
				,"io.output"
				,"io.stdin"
				,"debug.getregistry"
		};
		for(unsigned i = 0; i < sizeof(s_apBlacklist)/sizeof(s_apBlacklist[0]); i++)
		{
			char aCmd[128];
			str_format(aCmd, sizeof(aCmd), "%s=nil", s_apBlacklist[i]);
			luaL_dostring(m_pLuaState, aCmd);
			if(g_Config.m_Debug)
				dbg_msg("lua", "disable: '%s'", aCmd);
		}
	}


	if(g_Config.m_Debug)
		dbg_msg("lua/debug", "loading '%s' with flags %x", pFilename, m_PermissionFlags);

	// make sure that source code scripts are what they're supposed to be
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

	if(!CheckFile(pFilename))
	{
		//dbg_msg("lua", "!! found evidence that the scripts contains malicious code !!");
		m_pErrorStr = "Script is most likely malicious!";
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
}

void CLuaFile::ProfilingDoSample(const char *pEventName, int64 Time)
{
	CProfilingData &Data = m_ProfilingResults[std::string(pEventName)]; // auto-constructs item if key not present
	Data.AddSample(Time);
}

void CLuaFile::GetProfilingResults(std::vector< std::pair<std::string, CProfilingData> > *pOut) const
{
	for(auto it = m_ProfilingResults.begin(); it != m_ProfilingResults.end(); it++)
		pOut->push_back(*it);

	std::sort(pOut->begin(), pOut->end(), [](const std::pair<std::string, CProfilingData>& a, const std::pair<std::string, CProfilingData>& b){
		// primarily sort by value, for equal values sort by key
		return !(a.second != b.second ? a.second < b.second : a.first < b.first);
	});
}

bool CLuaFile::ScriptHasSettingsPage()
{
	LuaRef func1 = GetFunc("OnScriptRenderSettings");
	LuaRef func2 = GetFunc("OnScriptSaveSettings");
	if(func1.cast<bool>() && func2.cast<bool>())
		return true;
	return false;
}
