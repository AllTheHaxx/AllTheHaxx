#ifndef ENGINE_CLIENT_LUAFILE_H
#define ENGINE_CLIENT_LUAFILE_H

#include <lua.hpp>
#include <engine/external/luabridge/LuaBridge.h>
#include <engine/external/luabridge/RefCountedPtr.h>

class IClient;
class IStorage;
class CLua;

class CLuaFile
{
public:
	enum
	{
		LUAFILE_STATE_ERROR=-1,
		LUAFILE_STATE_IDLE,
		LUAFILE_STATE_LOADED
	};

private:
	CLua *m_pLua;
	lua_State *m_pLuaState;
	int m_State;
	std::string m_Filename;

	int m_UID; // the script can use this to identify itself
	char m_aScriptTitle[64];
	char m_aScriptInfo[128];
	bool m_ScriptHasSettings;

public:
	CLuaFile(CLua *pLua, std::string Filename);
	void Init();
	void Reset(bool error = false);
	void Unload();
	void OpenLua();
	luabridge::LuaRef GetFunc(const char *pFuncName);
	void CallFunc(const char *pFuncName);
	bool LoadFile(const char *pFilename);

	int State() const { return m_State; }
	int GetUID() const { return m_UID; }
	const char* GetFilename() const { return m_Filename.c_str(); }
	const char* GetScriptTitle() const { return m_aScriptTitle; }
	const char* GetScriptInfo() const { return m_aScriptInfo; }
	bool GetScriptHasSettings() const { return m_ScriptHasSettings; }

	CLua *Lua() const { return m_pLua; }
	
	static void RegisterLuaCallbacks(lua_State * L);

private:
	bool ScriptHasSettings();
};

#endif
