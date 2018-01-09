#ifndef ENGINE_CLIENT_LUAFILE_H
#define ENGINE_CLIENT_LUAFILE_H

#include <map>
#include <vector>
#include <string>
#include <base/system.h>
#include <base/tl/array.h>
// lua
#include <lua.hpp>
#include <engine/external/luabridge/LuaBridge.h>
#include <engine/external/luabridge/RefCountedPtr.h>


class IClient;
class IStorageTW;
class CLua;

class CLuaFile
{
	MACRO_ALLOC_HEAP_NO_INIT()

	friend class CLua;
	friend class CLuaBinding;
public:
	enum
	{
		STATE_ERROR=-1,
		STATE_IDLE,
		STATE_LOADED,

		STATE_CONSOLE
	};

	enum
	{
		PERMISSION_IO         = 1 << 0,
		PERMISSION_DEBUG      = 1 << 1,
		PERMISSION_FFI        = 1 << 2,
		PERMISSION_OS         = 1 << 3,
		PERMISSION_PACKAGE    = 1 << 4,
		PERMISSION_FILESYSTEM = 1 << 5,
		PERMISSION_GODMODE    = 1 << 30
	};

	#define LUA_REGINDEX_IO_OPEN "ATH_Backup:io.open"

	class CProfilingData
	{
		int64 m_Sum;
		unsigned m_NumSamples;

	public:
		CProfilingData()
		{
			m_Sum = 0;
			m_NumSamples = 0;
		}

		void AddSample(int64 Time)
		{
			m_Sum += Time; // TODO: can the time_freq() fluctuate on some systems? if so, gotta add it here
			m_NumSamples++;
		}

		double Average() const { return time_to_nanos(m_Sum)/(double)m_NumSamples; } // TODO XXX: bad average. we have to get rid of old values over time!
		double TotalTime() const { return time_to_millis(m_Sum); }
		unsigned NumSamples() const { return m_NumSamples; }

		bool operator<(const CProfilingData& other) const { return m_Sum < other.m_Sum; }
		bool operator!=(const CProfilingData& other) const { return m_Sum != other.m_Sum; }
	};

	const char *m_pErrorStr;
	array<std::string> m_Exceptions;

private:
	CLua *m_pLua;
	lua_State *m_pLuaState;
	lua_State *m_pLuaStateContainer;
	int m_State;
	const std::string m_Filename;

	int m_PermissionFlags;

	char m_aScriptTitle[64];
	char m_aScriptInfo[128];
	bool m_ScriptHidden;
	bool m_ScriptHasSettingsPage;
	bool m_ScriptAutoload;

	std::map<std::string, CProfilingData> m_ProfilingResults;
	bool m_ProfilingActive;
	int64 m_ScriptStartTime;

	void Init(); // starts the script
	void Unload(bool error = false, bool CalledFromExceptionHandler = false); // stops the script

public:
	CLuaFile(CLua *pLua, const std::string& Filename, bool Autoload);
	~CLuaFile();

	// public passthoughs
	void Activate() { Init(); }
	void Deactivate() { Unload(); }

	luabridge::LuaRef GetFunc(const char *pFuncName) const;
	template<class T> T CallFunc(const char *pFuncName, T def, bool *err=0);

	inline lua_State *L() const { return m_pLuaState; }
	int State() const { return m_State; }
	int GetPermissionFlags() const { return m_PermissionFlags; }
	const char* GetFilename() const { return m_Filename.c_str(); }
	const char* GetScriptTitle() const { return m_aScriptTitle; }
	const char* GetScriptInfo() const { return m_aScriptInfo; }
	bool GetScriptHasSettings() const { return m_ScriptHasSettingsPage; }
	bool GetScriptIsHidden() const { return m_ScriptHidden; }
	bool GetScriptIsAutoload() const { return m_ScriptAutoload; }
	bool SetScriptIsAutoload(bool NewVal) { bool ret = m_ScriptAutoload; m_ScriptAutoload = NewVal; return ret; }

	void ToggleProfiler() { m_ProfilingActive ^= true; }
	bool ProfilingActive() { return m_ProfilingActive; }
	void ProfilingDoSample(const char *pEventName, int64 Time);
	void GetProfilingResults(std::vector< std::pair<std::string, CProfilingData> > *pOut) const;
	double GetScriptAliveTime() const { return time_to_millis(time_get_raw()-m_ScriptStartTime); }

	CLua *Lua() const { return m_pLua; }

	static void RegisterLuaCallbacks(lua_State * L);

private:
	void Reset(bool error = false);
	void OpenLua();
	void LoadPermissionFlags(const char *pFilename);
	void ApplyPermissions(int Flags);
	bool LoadFile(const char *pFilename, bool Import);
	bool CheckFile(const char *pFilename);

	bool ScriptHasSettingsPage();
};

#endif
