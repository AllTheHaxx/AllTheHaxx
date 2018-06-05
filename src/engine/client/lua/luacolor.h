#ifndef ENGINE_CLIENT_LUA_LUACOLOR_H
#define ENGINE_CLIENT_LUA_LUACOLOR_H

#include <base/color.h>

class CLuaColor
{
public:
	static vec4 HueToRgbLua(float v1, float v2, float h);
	static vec4 RgbToHueLua(const vec3& rgb);
	static vec3 HslToRgbLua(const vec3& HSL);
	static vec3 HsvToRgbLua(const vec3& hsv);
	static vec3 RgbToHsvLua(const vec3& rgb);
	static vec4 HexToRgbaLua(int hex);
	static vec3 RgbToHslLua(const vec3& RGB);
	static vec3 GetColorV3Lua(int v);
	static vec3 HslToHsvLua(const vec3& hsl);
	static vec3 HsvToHslLua(const vec3& hsv);

};

#endif
