#include "luacolor.h"


vec4 CLuaColor::HueToRgbLua(float v1, float v2, float h)
{
	return HueToRgb(v1, v2, h);
}

vec4 CLuaColor::RgbToHueLua(const vec3& rgb)
{
	return RgbToHue(rgb);
}

vec3 CLuaColor::HslToRgbLua(const vec3& HSL)
{
	return HslToRgb(HSL);
}

vec3 CLuaColor::HsvToRgbLua(const vec3& hsv)
{
	return HsvToRgb(hsv);
}

vec3 CLuaColor::RgbToHsvLua(const vec3& rgb)
{
	return RgbToHsv(rgb);
}

vec4 CLuaColor::HexToRgbaLua(int hex)
{
	return HexToRgba(hex);
}

vec3 CLuaColor::RgbToHslLua(const vec3& RGB)
{
	return RgbToHsl(RGB);
}

vec3 CLuaColor::GetColorV3Lua(int v)
{
	return GetColorV3(v);
}

vec3 CLuaColor::HslToHsvLua(const vec3& hsl)
{
	return HslToHsv(hsl);
}

vec3 CLuaColor::HsvToHslLua(const vec3& hsv)
{
	return HsvToHsl(hsv);
}
