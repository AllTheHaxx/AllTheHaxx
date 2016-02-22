#pragma once

#include <lua.hpp>
#include <engine/external/luabridge/LuaBridge.h>
#include <engine/external/luabridge/RefCountedPtr.h>

using namespace luabridge;

class CLua
{
public:

    CLua();
    ~CLua();
};
