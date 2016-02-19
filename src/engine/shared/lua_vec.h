/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
/* (c) MAP94 */
#ifndef ENGINE_SHARED_LUAVEC_H
#define ENGINE_SHARED_LUAVEC_H

#include <lua.hpp>
#include <base/system.h>
#include <base/vmath.h>
#include <base/math.h>

//todo:
//group this in namespaces?

class CLuaVec2
{
private:
    static void CreateVec(lua_State *L, vec2 v);
    static bool CheckType(lua_State *L, int idx);
    static vec2 GetVec(lua_State *L, int idx);
public:
    static void Init(lua_State *L);
    //vec
    static int vec__new(lua_State *L);
    static int vec__unm(lua_State *L);
    static int vec__add(lua_State *L);
    static int vec__sub(lua_State *L);
    static int vec__mul(lua_State *L);
    static int vec__div(lua_State *L);
    static int vec__eq(lua_State *L);
    static int vec__lt(lua_State *L);
    static int vec__le(lua_State *L);
    static int vec_length(lua_State *L);
    static int vec_distance(lua_State *L);
    static int vec_dot(lua_State *L);
    static int vec_normalize(lua_State *L);
    static int vec_closest_point_on_line(lua_State *L);
};

class CLuaVec3
{
private:
    static void CreateVec(lua_State *L, vec3 v);
    static bool CheckType(lua_State *L, int idx);
    static vec3 GetVec(lua_State *L, int idx);
public:
    static void Init(lua_State *L);
    //vec
    static int vec__new(lua_State *L);
    static int vec__unm(lua_State *L);
    static int vec__add(lua_State *L);
    static int vec__sub(lua_State *L);
    static int vec__mul(lua_State *L);
    static int vec__div(lua_State *L);
    static int vec__eq(lua_State *L);
    static int vec__lt(lua_State *L);
    static int vec__le(lua_State *L);
    static int vec_length(lua_State *L);
    static int vec_distance(lua_State *L);
    static int vec_dot(lua_State *L);
    static int vec_normalize(lua_State *L);
    static int vec_cross(lua_State *L);
};

class CLuaVec4
{
private:
    static void CreateVec(lua_State *L, vec4 v);
    static bool CheckType(lua_State *L, int idx);
    static vec4 GetVec(lua_State *L, int idx);
public:
    static void Init(lua_State *L);
    //vec
    static int vec__new(lua_State *L);
    static int vec__unm(lua_State *L);
    static int vec__add(lua_State *L);
    static int vec__sub(lua_State *L);
    static int vec__mul(lua_State *L);
    static int vec__div(lua_State *L);
    static int vec__eq(lua_State *L);
    static int vec__lt(lua_State *L);
    static int vec__le(lua_State *L);
};


#endif
