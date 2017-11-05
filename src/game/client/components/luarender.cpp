#include "luarender.h"
#include "console.h"

void CLuaComponent::OnRender()
{
	// EVENT CALL
	LUA_FIRE_EVENT("OnRenderLevel", m_Level);
}

bool CLuaComponent::OnInput(IInput::CEvent Event)
{
	if(g_StealthMode || !g_Config.m_ClLua)
		return false;

	bool result = false;
	for(int ijdfg = 0; ijdfg < CLua::Client()->Lua()->GetLuaFiles().size(); ijdfg++)
	{
		CLuaFile *pLF = CLua::Client()->Lua()->GetLuaFiles()[ijdfg];
		if(pLF->State() != CLuaFile::STATE_LOADED)
			continue;
		luabridge::LuaRef EventTable(pLF->L());
		EventTable = newTable(pLF->L());
		EventTable["Key"] = Event.m_Key;
		EventTable["Flags"] = Event.m_Flags;
		EventTable["Text"] = std::string(Event.m_aText);
		EventTable["InputCount"] = Event.m_InputCount;
		LuaRef lfunc = pLF->GetFunc("OnInputLevel");
		if(lfunc) try { result |= lfunc(m_Level, Input()->KeyName(Event.m_Key), EventTable).cast<bool>(); CLua::Client()->LuaCheckDrawingState(pLF->L(), "OnInputLevel"); } catch(std::exception &e) { CLua::Client()->Lua()->HandleException(e, pLF); }
	}
	if(CGameConsole::m_pStatLuaConsole->m_LuaHandler.m_pDebugChild == NULL)
	{
		lua_State *L = CGameConsole::m_pStatLuaConsole->m_LuaHandler.m_pLuaState;
		luabridge::LuaRef EventTable(L);
		EventTable = newTable(L);
		EventTable["Key"] = Event.m_Key;
		EventTable["Flags"] = Event.m_Flags;
		EventTable["Text"] = std::string(Event.m_aText);
		EventTable["InputCount"] = Event.m_InputCount;
		LuaRef confunc = getGlobal(L, "OnInputLevel");
		if(confunc) try { result |= confunc(m_Level, Input()->KeyName(Event.m_Key), EventTable).cast<bool>(); } catch(std::exception &e) { CLua::Client()->Lua()->HandleException(e, L); }
	}

	return result;
}
