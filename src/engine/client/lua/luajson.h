#ifndef ENGINE_CLIENT_LUA_LUAJSON_H
#define ENGINE_CLIENT_LUA_LUAJSON_H

#include <string>
#include <lua.hpp>
#include <json-parser/json.hpp>

class CJsonValue
{
	friend class CLuaJson;
	json_value *m_pValue;

public:
	CJsonValue()
	{
		m_pValue = NULL;
	}

	std::string GetType(lua_State *L) const;

	std::string ToString(lua_State *L) const
	{
		if(m_pValue->type != json_string)
			luaL_error(L, "json value is not a string");
		return std::string((const char*)*m_pValue);
	}

	double ToNumber(lua_State *L) const
	{
		if(m_pValue->type != json_integer && m_pValue->type != json_double)
			luaL_error(L, "json value is not a number");
		return (double)*m_pValue;
	}

	bool ToBoolean(lua_State *L) const
	{
		if(m_pValue->type != json_boolean)
			luaL_error(L, "json value is not a boolean");
		return (bool)*m_pValue;
	}

	luabridge::LuaRef ToTable(lua_State *L);

	luabridge::LuaRef ToObject(lua_State *L);

private:
	static void PushJsonValueSelect(lua_State *L, const json_value& value);
	static void PushJsonArray(lua_State *L, const json_value& array);
	static void PushJsonObject(lua_State *L, const json_value& obj);
};


// namespace-global

class CLuaJson
{
public:
	static CJsonValue Parse(const char *pJsonString, lua_State *L);
};

#endif
