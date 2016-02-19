#include "lua_vec.h"
#include "lua.h"

//todo check type __add too?

//vec2
void CLuaVec2::Init(lua_State *L)
{
    //create meta table
    luaL_newmetatable(L, "vec2");
    lua_pushstring(L, "__unm");
    lua_pushcfunction(L, vec__unm);
    lua_settable(L, -3);

    lua_pushstring(L, "__add");
    lua_pushcfunction(L, vec__add);
    lua_settable(L, -3);

    lua_pushstring(L, "__sub");
    lua_pushcfunction(L, vec__sub);
    lua_settable(L, -3);

    lua_pushstring(L, "__mul");
    lua_pushcfunction(L, vec__mul);
    lua_settable(L, -3);

    lua_pushstring(L, "__div");
    lua_pushcfunction(L, vec__div);
    lua_settable(L, -3);

    lua_pushstring(L, "__eq");
    lua_pushcfunction(L, vec__eq);
    lua_settable(L, -3);

    lua_pushstring(L, "__lt");
    lua_pushcfunction(L, vec__lt);
    lua_settable(L, -3);

    lua_pushstring(L, "__le");
    lua_pushcfunction(L, vec__le);
    lua_settable(L, -3);
    lua_pop(L, 1);

    //register functions
    lua_register(L, "vec2_length", vec_length);
    lua_register(L, "vec2_distance", vec_distance);
    lua_register(L, "vec2_dot", vec_dot);
    lua_register(L, "vec2_normalize", vec_normalize);
    lua_register(L, "vec2_closest_point_on_line", vec_closest_point_on_line);

    //register ctor
    lua_register(L, "vec2", vec__new);
}

void CLuaVec2::CreateVec(lua_State *L, vec2 v)
{
    lua_createtable(L, 0, 2);

    lua_pushstring(L, "x");
    lua_pushnumber(L, v.x);
    lua_settable(L, -3);

    lua_pushstring(L, "y");
    lua_pushnumber(L, v.y);
    lua_settable(L, -3);

    luaL_getmetatable(L, "vec2");
    lua_setmetatable(L, -2);
}

bool CLuaVec2::CheckType(lua_State *L, int idx)
{
    lua_getmetatable(L, idx);
    luaL_getmetatable(L, "vec2");
    int i = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);
    return i;
}

vec2 CLuaVec2::GetVec(lua_State *L, int idx)
{
    if (!lua_istable(L, 1))
        return vec2(0, 0); //should never happen
    vec2 v;

    lua_getfield(L, idx, "x");
    v.x = lua_isnumber(L, -1) ? lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);

    lua_getfield(L, idx, "y");
    v.y = lua_isnumber(L, -1) ? lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);
    return v;
}

int CLuaVec2::vec__new(lua_State *L)
{
    vec2 v = vec2(0, 0);
    if (lua_istable(L, 1))
        v = GetVec(L, 1);
    else
    {
        v.x = lua_isnumber(L, 1) ? lua_tointeger(L, 1) : 0;
        v.y = lua_isnumber(L, 2) ? lua_tointeger(L, 2) : 0;
    }
    CreateVec(L, v);
    return 1;
}

int CLuaVec2::vec__unm(lua_State *L)
{
    CreateVec(L, -GetVec(L, 1));
    return 1;
}

int CLuaVec2::vec__add(lua_State *L)
{
    CreateVec(L, GetVec(L, 1) + GetVec(L, 2));
    return 1;
}

int CLuaVec2::vec__sub(lua_State *L)
{
    CreateVec(L, GetVec(L, 1) - GetVec(L, 2));
    return 1;
}

int CLuaVec2::vec__mul(lua_State *L)
{
    CreateVec(L, GetVec(L, 1) * GetVec(L, 2));
    return 1;
}

int CLuaVec2::vec__div(lua_State *L)
{
    CreateVec(L, GetVec(L, 1) / GetVec(L, 2));
    return 1;
}

int CLuaVec2::vec__eq(lua_State *L)
{
    lua_pushboolean(L, GetVec(L, 1) == GetVec(L, 2));
    return 1;
}

int CLuaVec2::vec__lt(lua_State *L)
{
    lua_pushboolean(L, GetVec(L, 1) < GetVec(L, 2));
    return 1;
}

int CLuaVec2::vec__le(lua_State *L)
{
    lua_pushboolean(L, GetVec(L, 1) <= GetVec(L, 2));
    return 1;
}

int CLuaVec2::vec_length(lua_State *L)
{
    if (!CheckType(L, 1))
    {
        lua_pushstring(L, "Attempt to call 'vec2_length' with invalid parameters");
        lua_error(L);
        return 0;
    }
    lua_pushnumber(L, length(GetVec(L, 1)));
    return 1;
}

int CLuaVec2::vec_distance(lua_State *L)
{
    if (!CheckType(L, 1))
    {
        lua_pushstring(L, "Attempt to call 'vec2_distance' with invalid parameters");
        lua_error(L);
        return 0;
    }
    lua_pushnumber(L, distance(GetVec(L, 1), GetVec(L, 2)));
    return 1;
}

int CLuaVec2::vec_dot(lua_State *L)
{
    if (!CheckType(L, 1))
    {
        lua_pushstring(L, "Attempt to call 'vec2_dot' with invalid parameters");
        lua_error(L);
        return 0;
    }
    lua_pushnumber(L, dot(GetVec(L, 1), GetVec(L, 2)));
    return 1;
}

int CLuaVec2::vec_normalize(lua_State *L)
{
    if (!CheckType(L, 1))
    {
        lua_pushstring(L, "Attempt to call 'vec2_normalize' with invalid parameters");
        lua_error(L);
        return 0;
    }
    CreateVec(L, normalize(GetVec(L, 1)));
    return 1;
}

int CLuaVec2::vec_closest_point_on_line(lua_State *L)
{
    if (!CheckType(L, 1))
    {
        lua_pushstring(L, "Attempt to call 'vec2_closest_point_on_line' with invalid parameters");
        lua_error(L);
        return 0;
    }
    CreateVec(L, closest_point_on_line(GetVec(L, 1), GetVec(L, 2), GetVec(L, 3)));
    return 1;
}


//vec3
void CLuaVec3::Init(lua_State *L)
{
    //create meta table
    luaL_newmetatable(L, "vec3");
    lua_pushstring(L, "__unm");
    lua_pushcfunction(L, vec__unm);
    lua_settable(L, -3);

    lua_pushstring(L, "__add");
    lua_pushcfunction(L, vec__add);
    lua_settable(L, -3);

    lua_pushstring(L, "__sub");
    lua_pushcfunction(L, vec__sub);
    lua_settable(L, -3);

    lua_pushstring(L, "__mul");
    lua_pushcfunction(L, vec__mul);
    lua_settable(L, -3);

    lua_pushstring(L, "__div");
    lua_pushcfunction(L, vec__div);
    lua_settable(L, -3);

    lua_pushstring(L, "__eq");
    lua_pushcfunction(L, vec__eq);
    lua_settable(L, -3);

    lua_pushstring(L, "__lt");
    lua_pushcfunction(L, vec__lt);
    lua_settable(L, -3);

    lua_pushstring(L, "__le");
    lua_pushcfunction(L, vec__le);
    lua_settable(L, -3);
    lua_pop(L, 1);

    //register functions
    lua_register(L, "vec3_length", vec_length);
    lua_register(L, "vec3_distance", vec_distance);
    lua_register(L, "vec3_dot", vec_dot);
    lua_register(L, "vec3_normalize", vec_normalize);
    lua_register(L, "vec3_cross", vec_cross);

    //register ctor
    lua_register(L, "vec3", vec__new);
}

void CLuaVec3::CreateVec(lua_State *L, vec3 v)
{
    lua_createtable(L, 0, 3);

    lua_pushstring(L, "x");
    lua_pushnumber(L, v.x);
    lua_settable(L, -3);

    lua_pushstring(L, "y");
    lua_pushnumber(L, v.y);
    lua_settable(L, -3);

    lua_pushstring(L, "z");
    lua_pushnumber(L, v.z);
    lua_settable(L, -3);

    luaL_getmetatable(L, "vec3");
    lua_setmetatable(L, -2);
}

bool CLuaVec3::CheckType(lua_State *L, int idx)
{
    lua_getmetatable(L, idx);
    luaL_getmetatable(L, "vec3");
    int i = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);
    return i;
}

vec3 CLuaVec3::GetVec(lua_State *L, int idx)
{
    if (!lua_istable(L, 1))
        return vec3(0, 0, 0); //should never happen
    vec3 v;

    lua_getfield(L, idx, "x");
    v.x = lua_isnumber(L, -1) ? lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);

    lua_getfield(L, idx, "y");
    v.y = lua_isnumber(L, -1) ? lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);

    lua_getfield(L, idx, "z");
    v.z = lua_isnumber(L, -1) ? lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);
    return v;
}

int CLuaVec3::vec__new(lua_State *L)
{
    vec3 v = vec3(0, 0, 0);
    if (lua_istable(L, 1))
        v = GetVec(L, 1);
    else
    {
        v.x = lua_isnumber(L, 1) ? lua_tointeger(L, 1) : 0;
        v.y = lua_isnumber(L, 2) ? lua_tointeger(L, 2) : 0;
        v.z = lua_isnumber(L, 3) ? lua_tointeger(L, 3) : 0;
    }
    CreateVec(L, v);
    return 1;
}

int CLuaVec3::vec__unm(lua_State *L)
{
    CreateVec(L, -GetVec(L, 1));
    return 1;
}

int CLuaVec3::vec__add(lua_State *L)
{
    CreateVec(L, GetVec(L, 1) + GetVec(L, 2));
    return 1;
}

int CLuaVec3::vec__sub(lua_State *L)
{
    CreateVec(L, GetVec(L, 1) - GetVec(L, 2));
    return 1;
}

int CLuaVec3::vec__mul(lua_State *L)
{
    CreateVec(L, GetVec(L, 1) * GetVec(L, 2));
    return 1;
}

int CLuaVec3::vec__div(lua_State *L)
{
    CreateVec(L, GetVec(L, 1) / GetVec(L, 2));
    return 1;
}

int CLuaVec3::vec__eq(lua_State *L)
{
    lua_pushboolean(L, GetVec(L, 1) == GetVec(L, 2));
    return 1;
}

int CLuaVec3::vec__lt(lua_State *L)
{
    lua_pushboolean(L, GetVec(L, 1) < GetVec(L, 2));
    return 1;
}

int CLuaVec3::vec__le(lua_State *L)
{
    lua_pushboolean(L, GetVec(L, 1) <= GetVec(L, 2));
    return 1;
}

int CLuaVec3::vec_length(lua_State *L)
{
    if (!CheckType(L, 1))
    {
        lua_pushstring(L, "Attempt to call 'vec3_length' with invalid parameters");
        lua_error(L);
        return 0;
    }
    lua_pushnumber(L, length(GetVec(L, 1)));
    return 1;
}

int CLuaVec3::vec_distance(lua_State *L)
{
    if (!CheckType(L, 1))
    {
        lua_pushstring(L, "Attempt to call 'vec3_distance' with invalid parameters");
        lua_error(L);
        return 0;
    }
    if (!CheckType(L, 2))
    {
        lua_pushstring(L, "Attempt to call 'vec3_distance' with invalid parameters");
        lua_error(L);
        return 0;
    }
    lua_pushnumber(L, distance(GetVec(L, 1), GetVec(L, 2)));
    return 1;
}

int CLuaVec3::vec_dot(lua_State *L)
{
    if (!CheckType(L, 1))
    {
        lua_pushstring(L, "Attempt to call 'vec3_dot' with invalid parameters");
        lua_error(L);
        return 0;
    }
    if (!CheckType(L, 2))
    {
        lua_pushstring(L, "Attempt to call 'vec3_dot' with invalid parameters");
        lua_error(L);
        return 0;
    }
    lua_pushnumber(L, dot(GetVec(L, 1), GetVec(L, 2)));
    return 1;
}

int CLuaVec3::vec_normalize(lua_State *L)
{
    if (!CheckType(L, 1))
    {
        lua_pushstring(L, "Attempt to call 'vec3_normalize' with invalid parameters");
        lua_error(L);
        return 0;
    }
    CreateVec(L, normalize(GetVec(L, 1)));
    return 1;
}

int CLuaVec3::vec_cross(lua_State *L)
{
    if (!CheckType(L, 1))
    {
        lua_pushstring(L, "Attempt to call 'vec3_cross' with invalid parameters");
        lua_error(L);
        return 0;
    }
    if (!CheckType(L, 2))
    {
        lua_pushstring(L, "Attempt to call 'vec3_cross' with invalid parameters");
        lua_error(L);
        return 0;
    }
    CreateVec(L, cross(GetVec(L, 1), GetVec(L, 2)));
    return 1;
}

//vec4
void CLuaVec4::Init(lua_State *L)
{
    //create meta table
    luaL_newmetatable(L, "vec4");
    lua_pushstring(L, "__unm");
    lua_pushcfunction(L, vec__unm);
    lua_settable(L, -3);

    lua_pushstring(L, "__add");
    lua_pushcfunction(L, vec__add);
    lua_settable(L, -3);

    lua_pushstring(L, "__sub");
    lua_pushcfunction(L, vec__sub);
    lua_settable(L, -3);

    lua_pushstring(L, "__mul");
    lua_pushcfunction(L, vec__mul);
    lua_settable(L, -3);

    lua_pushstring(L, "__div");
    lua_pushcfunction(L, vec__div);
    lua_settable(L, -3);

    lua_pushstring(L, "__eq");
    lua_pushcfunction(L, vec__eq);
    lua_settable(L, -3);

    lua_pushstring(L, "__lt");
    lua_pushcfunction(L, vec__lt);
    lua_settable(L, -3);

    lua_pushstring(L, "__le");
    lua_pushcfunction(L, vec__le);
    lua_settable(L, -3);

    lua_pop(L, 1);

    //register ctor
    lua_register(L, "vec4", vec__new);
}

void CLuaVec4::CreateVec(lua_State *L, vec4 v)
{
    lua_createtable(L, 0, 4);

    lua_pushstring(L, "r");
    lua_pushnumber(L, v.r);
    lua_settable(L, -3);

    lua_pushstring(L, "g");
    lua_pushnumber(L, v.g);
    lua_settable(L, -3);

    lua_pushstring(L, "b");
    lua_pushnumber(L, v.b);
    lua_settable(L, -3);

    lua_pushstring(L, "a");
    lua_pushnumber(L, v.a);
    lua_settable(L, -3);

    luaL_getmetatable(L, "vec4");
    lua_setmetatable(L, -2);
}

bool CLuaVec4::CheckType(lua_State *L, int idx)
{
    lua_getmetatable(L, idx);
    luaL_getmetatable(L, "vec4");
    int i = lua_rawequal(L, -1, -2);
    lua_pop(L, 2);
    return i;
}

vec4 CLuaVec4::GetVec(lua_State *L, int idx)
{
    if (!lua_istable(L, 1))
        return vec4(0, 0, 0, 0); //should never happen
    vec4 v;

    lua_getfield(L, idx, "r");
    v.r = lua_isnumber(L, -1) ? lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);

    lua_getfield(L, idx, "g");
    v.g = lua_isnumber(L, -1) ? lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);

    lua_getfield(L, idx, "b");
    v.b = lua_isnumber(L, -1) ? lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);

    lua_getfield(L, idx, "a");
    v.a = lua_isnumber(L, -1) ? lua_tointeger(L, -1) : 0;
    lua_pop(L, 1);
    return v;
}

int CLuaVec4::vec__new(lua_State *L)
{
    vec4 v = vec4(0, 0, 0, 0);
    if (lua_istable(L, 1))
        v = GetVec(L, 1);
    else
    {
        v.r = lua_isnumber(L, 1) ? lua_tointeger(L, 1) : 0;
        v.g = lua_isnumber(L, 2) ? lua_tointeger(L, 2) : 0;
        v.b = lua_isnumber(L, 3) ? lua_tointeger(L, 3) : 0;
        v.a = lua_isnumber(L, 4) ? lua_tointeger(L, 4) : 0;
    }
    CreateVec(L, v);
    return 1;
}

int CLuaVec4::vec__unm(lua_State *L)
{
    CreateVec(L, -GetVec(L, 1));
    return 1;
}

int CLuaVec4::vec__add(lua_State *L)
{
    CreateVec(L, GetVec(L, 1) + GetVec(L, 2));
    return 1;
}

int CLuaVec4::vec__sub(lua_State *L)
{
    CreateVec(L, GetVec(L, 1) - GetVec(L, 2));
    return 1;
}

int CLuaVec4::vec__mul(lua_State *L)
{
    CreateVec(L, GetVec(L, 1) * GetVec(L, 2));
    return 1;
}

int CLuaVec4::vec__div(lua_State *L)
{
    CreateVec(L, GetVec(L, 1) / GetVec(L, 2));
    return 1;
}

int CLuaVec4::vec__eq(lua_State *L)
{
    lua_pushboolean(L, GetVec(L, 1) == GetVec(L, 2));
    return 1;
}

int CLuaVec4::vec__lt(lua_State *L)
{
    lua_pushboolean(L, GetVec(L, 1) < GetVec(L, 2));
    return 1;
}

int CLuaVec4::vec__le(lua_State *L)
{
    lua_pushboolean(L, GetVec(L, 1) <= GetVec(L, 2));
    return 1;
}

