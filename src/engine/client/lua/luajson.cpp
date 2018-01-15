#include <base/system.h>
#include <engine/client/luabinding.h>
#include <json-parser/json.hpp>
#include "luajson.h"

CJsonValue CLuaJson::Parse(const char *pJsonString, lua_State *L)
{
	json_value *pJson = json_parse(pJsonString, (size_t)str_length(pJsonString));
	if(pJson == NULL)
		luaL_error(L, "error parsing json string");

	CJsonValue Wrapper;
	Wrapper.m_pValue = pJson;
	return Wrapper;
}

std::string CJsonValue::GetType(lua_State *L) const
{
	switch(m_pValue->type)
	{
		case json_object:
			return "object";
		case json_array:
			return "array";
		case json_integer:
		case json_double:
			return "number";
		case json_string:
			return "string";
		case json_boolean:
			return "boolean";
		case json_none:
		case json_null:
			return "nil";
	}
	return "error";
}

void CJsonValue::PushJsonValueSelect(lua_State *L, const json_value& value)
{
	switch(value.type)
	{
		case json_integer:
		case json_double:
			lua_pushnumber(L, (double)value);
			break;
		case json_boolean:
			lua_pushboolean(L, (bool)value);
			break;
		case json_string:
			lua_pushstring(L, (const char*)value);
			break;
		case json_array:
			PushJsonArray(L, value);
			break;
		case json_object:
			PushJsonObject(L, value);
			break;
		case json_none:
		case json_null:
			lua_pushnil(L);
			break;
	}
}

void CJsonValue::PushJsonObject(lua_State *L, const json_value& obj)
{
	lua_newtable(L);
	for(unsigned int i = 0; i < obj.u.object.length; i++)
	{
		const char *pKey = obj.u.object.values[i].name;
		const json_value& value = *(obj.u.object.values[i].value);

		PushJsonValueSelect(L, value);
		lua_setfield(L, -2, pKey);
	}
}

void CJsonValue::PushJsonArray(lua_State *L, const json_value& array)
{
	lua_newtable(L);
	for(unsigned int i = 0; i < array.u.array.length ; i++)
	{
		const json_value& value = array[i];

		lua_pushinteger(L, i);
		PushJsonValueSelect(L, value);

		/* STACK:
		 * -1 value
		 * -2 key (i)
		 * -3 table
		 */
		lua_settable(L, -3);
	}

}

luabridge::LuaRef CJsonValue::ToTable(lua_State *L)
{
	if(m_pValue->type != json_array)
		luaL_error(L, "json value is not an array");

	const json_value& array = *m_pValue;

	PushJsonArray(L, array);
	luabridge::LuaRef Result = luabridge::Stack<LuaRef>::get(L, -1);
	lua_pop(L, 1);
	return Result;
}

luabridge::LuaRef CJsonValue::ToObject(lua_State *L)
{
	if(m_pValue->type != json_object)
		luaL_error(L, "json value is not an object");

	const json_value& obj = *m_pValue;

	PushJsonObject(L, obj);
	luabridge::LuaRef Result = luabridge::Stack<LuaRef>::get(L, -1);
	lua_pop(L, 1);
	return Result;
}
