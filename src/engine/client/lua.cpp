#include <base/system.h>
#include <engine/storage.h>

#include "lua.h"
#include "luabinding.h"


lua_State * CLua::m_pStaticLua = 0;
IClient * CLua::m_pClient = 0; 
IGameClient * CLua::m_pGameClient = 0;

using namespace luabridge;

CLua::CLua()
{
	m_pLuaState = 0;
	CLuaBinding::m_pUiContainer = new CLuaBinding::UiContainer;
}

CLua::~CLua()
{
}

void CLua::Init(IClient * pClient, IStorage * pStorage)
{
	m_pClient = pClient;
	m_pStorage = pStorage;
	
    m_pLuaState = luaL_newstate();
	//m_pStaticLua = m_pLua;
	
    lua_atpanic(m_pLuaState, this->Panic);
    lua_register(m_pLuaState, "errorfunc", this->ErrorFunc);

    //dont use openlibs. this include io and os functions
    luaL_openlibs(m_pLuaState);
    luaopen_base(m_pLuaState);
    luaopen_math(m_pLuaState);
    luaopen_string(m_pLuaState);
    luaopen_table(m_pLuaState);
    //luaopen_io(m_pLua);
    luaopen_os(m_pLuaState);
    //luaopen_package(m_pLua); //not sure whether we should load this
    luaopen_debug(m_pLuaState);
    luaopen_bit(m_pLuaState);
    luaopen_jit(m_pLuaState);
    luaopen_ffi(m_pLuaState); //dont know about this yet. could be a sand box leak.
	
	LoadFolder("main");
	RegisterLuaCallbacks();

	//LoadFile("lua/Test.lua");
}

void CLua::RegisterLuaCallbacks()  //LUABRIDGE!
{
	// client namespace
	getGlobalNamespace(m_pLuaState)

		.beginNamespace("_client")
			.addFunction("GetTick", &CLuaBinding::LuaGetTick)
		.endNamespace()


		// lua namespace
		.beginNamespace("_lua")
			.addFunction("SetScriptTitle", &CLuaBinding::LuaSetScriptTitle)
			.addFunction("SetScriptInfo", &CLuaBinding::LuaSetScriptInfo)
			.addFunction("SetScriptHasSettings", &CLuaBinding::LuaSetScriptHasSettings)
		.endNamespace()


		// ui namespace
		.beginNamespace("_ui")
			.addFunction("SetUiColor", &CLuaBinding::LuaSetUiColor)
			.addFunction("DrawUiRect", &CLuaBinding::LuaDrawUiRect)
		.endNamespace()


		// components namespace
		.beginNamespace("_components")
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

LuaRef CLua::GetFunc(const char *pFuncName)
{
	LuaRef func = getGlobal(m_pLuaState, pFuncName);

	if(func == 0)
		dbg_msg("Lua", "Error : Function '%s' not found.", pFuncName);
	
	return func;  //return 0 if the function is not found!	
}

void CLua::CallFunc(const char *pFuncName)
{
	//TODO : Find a way to pass the LuaFunction up to 8 arguments (no matter of the type)
}

bool CLua::LoadFile(const char *pFilename)
{
	if(!pFilename || pFilename[0] == '\0' || str_length(pFilename) <= 4 ||
			str_comp_nocase(&pFilename[str_length(pFilename)]-4, ".lua"))
		return false;

    int Status = luaL_loadfile(m_pLuaState, pFilename);
    if (Status)
    {
        //does this work?
        ErrorFunc(m_pLuaState);
        return false;
    }

    Status = lua_pcall(m_pLuaState, 0, LUA_MULTRET, 0);
    if (Status)
    {
        ErrorFunc(m_pLuaState);
        return false;
    }

    LuaFile newfile;
	str_copy(newfile.aName, pFilename, sizeof(newfile.aName));
	//str_copy(newfile.aDir, FullDir, sizeof(newfile.aDir));
	m_LuaFiles.add(newfile);
    return true;
}

void CLua::LoadFolder(const char *pFolder)
{
	char FullDir[256];
	str_format(FullDir, sizeof(FullDir), "lua/%s", pFolder);
	
	dbg_msg("Lua", "Loading Folder '%s'", FullDir);
	LuaLoadHelper * pParams = new LuaLoadHelper;
	pParams->pLua = this;
	pParams->pString = FullDir;
	
	m_pStorage->ListDirectory(IStorage::TYPE_ALL, FullDir, LoadFolderCallback, pParams);
	
	delete pParams;
}

int CLua::LoadFolderCallback(const char *pName, int IsDir, int DirType, void *pUser)
{	
	if(pName[0] == '.')
		return 0;
	
	LuaLoadHelper *pParams = (LuaLoadHelper*) pUser;
	
	CLua *pSelf = pParams->pLua;
	const char * FullDir = pParams->pString;
	
	char File[64];
	str_format(File, sizeof(File), "%s/%s", FullDir, pName);
	dbg_msg("Lua", "->Loading File %s", File);
	
	pSelf->LoadFile(File);
	return 0;
}

int CLua::Panic(lua_State *L)
{
    dbg_break();
    return 0;
}

int CLua::ErrorFunc(lua_State *L)
{
	dbg_msg("Lua", "Lua Script Error! :");
    lua_getglobal(L, "pLUA");
    CLua *pSelf = (CLua *)lua_touserdata(L, -1);
    lua_pop(L, 1);

    int depth = 0;
    int frameskip = 1;
    lua_Debug frame;

    if (lua_tostring(L, -1) == 0)
	{
		//dbg_msg("Lua", "PANOS");
        return 0;
	}
	
    //dbg_msg("Lua", pSelf->m_aFilename);
    dbg_msg("Lua", lua_tostring(L, -1));
    dbg_msg("Lua", "Backtrace:");

    while(lua_getstack(L, depth, &frame) == 1)
    {
        depth++;
        lua_getinfo(L, "nlSf", &frame);
        /* check for functions that just report errors. these frames just confuses more then they help */
        if(frameskip && str_comp(frame.short_src, "[C]") == 0 && frame.currentline == -1)
            continue;
        frameskip = 0;
        /* print stack frame */
        dbg_msg("Lua", "%s(%d): %s %s", frame.short_src, frame.currentline, frame.name, frame.namewhat);
    }
    lua_pop(L, 1); // remove error message
    lua_gc(L, LUA_GCCOLLECT, 0);
    return 0;
}
