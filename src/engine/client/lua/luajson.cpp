#include <base/system.h>
#include <engine/client/luabinding.h>
#include <json-parser/json.hpp>
#include <json-builder/json-builder.h>
#include "luajson.h"

static const json_serialize_opts json_opts_common = {
		json_serialize_mode_multiline,
		0,
		2
};
static const json_serialize_opts json_opts_packed = {
		json_serialize_mode_packed,
		json_serialize_opt_no_space_after_colon|json_serialize_opt_no_space_after_comma|json_serialize_opt_pack_brackets,
		0
};


void CJsonValue::Destroy(lua_State *L)
{
	CheckValid(L);

	if(m_pValue)
		json_value_free(m_pValue);
	m_pValue = NULL;
}

CJsonValue CLuaJson::Parse(const char *pJsonString, lua_State *L)
{
	json_settings settings = {};
	settings.value_extra = json_builder_extra; // space for json-builder state
	settings.mem_alloc = [](size_t size, int zero, void * user_data) -> void* {
		void *p = mem_alloc((unsigned)size, 0);
		if(zero)
			mem_zero(p, (unsigned)size);
		return p;
	};
	settings.mem_free = [](void *p, void * user_data) {
		mem_free(p);
	};

	char aErrorBuf[json_error_max];
	aErrorBuf[0] = '\0';
	json_value *pJson = json_parse_ex(&settings, pJsonString, (size_t)str_length(pJsonString), aErrorBuf);
	if(pJson == NULL)
		luaL_error(L, "json parsing error: %s", aErrorBuf);

	CJsonValue Wrapper;
	Wrapper.m_pValue = pJson;
	return Wrapper;
}

std::string CLuaJson::Serialize(const CJsonValue& json_value, bool shorten, lua_State *L)
{
	json_value.CheckValid(L);

	const json_serialize_opts& opts = shorten ? json_opts_packed : json_opts_common;

	char *pJsonBuf = mem_allocb(char, json_measure_ex(json_value.m_pValue, opts));
	json_serialize_ex(pJsonBuf, json_value.m_pValue, opts);
	std::string Result(pJsonBuf);
	mem_free(pJsonBuf);
	return Result;
}

std::string CJsonValue::GetType(lua_State *L) const
{
	CheckValid(L);
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
	CheckValid(L);
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
	CheckValid(L);
	if(m_pValue->type != json_object)
		luaL_error(L, "json value is not an object");

	const json_value& obj = *m_pValue;

	PushJsonObject(L, obj);
	luabridge::LuaRef Result = luabridge::Stack<LuaRef>::get(L, -1);
	lua_pop(L, 1);
	return Result;
}

static json_value *ConvertSomething(const LuaRef& data)
{
	json_value *value = NULL;
	switch(data.type())
	{
		case LUA_TNONE:
		case LUA_TNIL:
			value = json_null_new();
			break;
		case LUA_TNUMBER:
			value = json_double_new(data.cast<double>());
			break;
		case LUA_TBOOLEAN:
			value = json_boolean_new(data.cast<bool>());
			break;
		case LUA_TSTRING:
			value = json_string_new(data.cast<const char*>());
			break;
		case LUA_TTABLE:
			// find out whether table is ipairs-iteratable (yes: array, no: object)
		{
			bool IsArray = true;
			lua_State *L = data.state();
			lua_pushnil(L);
			while(lua_next(L, -2) != 0)
			{
				DEFER([L](){ lua_pop(L, 1); });

				// uses 'key' (at index -2) and 'value' (at index -1)
				if(lua_type(L, -2) != LUA_TNUMBER)
				{
					IsArray = false;
					break;
				}
			}
			lua_pop(L, 1);

			if(IsArray)
				value = json_array_new((size_t)data.length());
			else
				value = json_object_new((size_t)data.length());

			// add everything to the json data structure
			for(luabridge::Iterator it(data); !it.isNil(); ++it)
			{
				json_value *entry = ConvertSomething(*it);
				if(IsArray)
					json_array_push(value, entry);
				else
					json_object_push(value, it.key(), entry);
			}
		} break;
		default:
		{
			// handles function, userdata, thread
			lua_State *L = data.state();
			luaL_error(L, "value of type %s cannot be converted to json", lua_typename(L, data.type()));
			break;
		}
	}

	return value;
}

CJsonValue CLuaJson::Convert(LuaRef data)
{
	CJsonValue Result;
	Result.m_pValue = ConvertSomething(data);
	return Result;
}
