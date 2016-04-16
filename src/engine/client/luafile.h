#ifndef ENGINE_CLIENT_LUAFILE_H
#define ENGINE_CLIENT_LUAFILE_H

#include <lua.hpp>
#include <engine/external/luabridge/LuaBridge.h>
#include <engine/external/luabridge/RefCountedPtr.h>

#include "base/system.h"

#define LUA_CALL_FUNC(LUA_STATE, FUNC_NAME, TYPE, RETURN, ...) { try { \
	LuaRef func = getGlobal(LUA_STATE, FUNC_NAME); \
	if(func) \
		RETURN = func(__VA_ARGS__).cast<TYPE>(); }\
	catch (std::exception& e) \
	{ printf("LUA EXCEPTION: %s\n", e.what()); } }

class IClient;
class IStorageTW;
class CLua;

class CLuaFile
{
	friend class CLuaBinding;
public:
	enum
	{
		LUAFILE_STATE_ERROR=-1,
		LUAFILE_STATE_IDLE,
		LUAFILE_STATE_LOADED
	};

	enum
	{
		LUAFILE_PERMISSION_IO		= 1 << 0,
		LUAFILE_PERMISSION_DEBUG	= 1 << 1,
		LUAFILE_PERMISSION_FFI		= 1 << 2,
		LUAFILE_PERMISSION_OS		= 1 << 3,
		LUAFILE_PERMISSION_PACKAGE	= 1 << 4,
		LUAFILE_NUM_PERMISSIONS
	};

private:
	CLua *m_pLua;
	lua_State *m_pLuaState;
	int m_State;
	std::string m_Filename;

	int m_UID; // the script can use this to identify itself
	int m_PermissionFlags;

	char m_aScriptTitle[64];
	char m_aScriptInfo[128];
	bool m_ScriptHasSettings;
	bool m_ScriptAutoload;

public:
	void operator delete(void *p) { mem_free(p); }

	CLuaFile(CLua *pLua, std::string Filename, bool Autoload);
	~CLuaFile();
	void Init();
	void Reset(bool error = false);
	void LoadPermissionFlags();
	void Unload(bool error = false);
	luabridge::LuaRef GetFunc(const char *pFuncName);
	template<class T> T CallFunc(const char *pFuncName);

	int State() const { return m_State; }
	int GetUID() const { return m_UID; }
	int GetPermissionFlags() const { return m_PermissionFlags; }
	const char* GetFilename() const { return m_Filename.c_str(); }
	const char* GetScriptTitle() const { return m_aScriptTitle; }
	const char* GetScriptInfo() const { return m_aScriptInfo; }
	bool GetScriptHasSettings() const { return m_ScriptHasSettings; }
	bool GetScriptIsAutoload() const { return m_ScriptAutoload; }
	bool SetScriptIsAutoload(bool NewVal) { bool ret = m_ScriptAutoload; m_ScriptAutoload = NewVal; return ret; }

	CLua *Lua() const { return m_pLua; }
	
	static void RegisterLuaCallbacks(lua_State * L);

private:
	void OpenLua();
	bool LoadFile(const char *pFilename);

	bool ScriptHasSettings();
};

#endif
