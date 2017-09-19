
#ifndef BASE_COLOR_H
#define BASE_COLOR_H

#include "math.h"
#include "vmath.h"

/*
	Title: Color handling
*/

/*
	Function: HueToRgb
		Converts Hue to RGB
*/
inline float HueToRgb(float v1, float v2, float h)
{
	if(h < 0.0f) h += 1;
	if(h > 1.0f) h -= 1;
	if((6.0f * h) < 1.0f) return v1 + (v2 - v1) * 6.0f * h;
	if((2.0f * h) < 1.0f) return v2;
	if((3.0f * h) < 2.0f) return v1 + (v2 - v1) * ((2.0f/3.0f) - h) * 6.0f;
	return v1;
}

inline float RgbToHue(const vec3& rgb)
{
	float h_min = min(min(rgb.r, rgb.g), rgb.b);
	float h_max = max(max(rgb.r, rgb.g), rgb.b);

	float hue = 0.0f;

	if(h_max == rgb.r)
		hue = (rgb.g-rgb.b) / (h_max-h_min);
	else if(h_max == rgb.g)
		hue = 2.0f + (rgb.b-rgb.r) / (h_max-h_min);
	else
		hue = 4.0f + (rgb.r-rgb.g) / (h_max-h_min);

	hue /= 6.0f;

	if(hue < 0.0f)
		hue += 1.0f;

	return hue;
}

/*
	Function: HslToRgb
		Converts HSL to RGB
*/
inline vec3 HslToRgb(const vec3& HSL)
{
	if(HSL.s == 0.0f)
		return vec3(HSL.l, HSL.l, HSL.l);
	else
	{
		float v2 = HSL.l < 0.5f ? HSL.l * (1.0f + HSL.s) : (HSL.l+HSL.s) - (HSL.s*HSL.l);
		float v1 = 2.0f * HSL.l - v2;

		return vec3(HueToRgb(v1, v2, HSL.h + (1.0f/3.0f)), HueToRgb(v1, v2, HSL.h), HueToRgb(v1, v2, HSL.h - (1.0f/3.0f)));
	}
}

inline vec3 HsvToRgb(const vec3& hsv)
{
	int h = int(hsv.x * 6.0f);
	float f = hsv.x * 6.0f - h;
	float p = hsv.z * (1.0f - hsv.y);
	float q = hsv.z * (1.0f - hsv.y * f);
	float t = hsv.z * (1.0f - hsv.y * (1.0f - f));

	vec3 rgb = vec3(0.0f, 0.0f, 0.0f);

	switch(h % 6)
	{
	case 0:
		rgb.r = hsv.z;
		rgb.g = t;
		rgb.b = p;
		break;

	case 1:
		rgb.r = q;
		rgb.g = hsv.z;
		rgb.b = p;
		break;

	case 2:
		rgb.r = p;
		rgb.g = hsv.z;
		rgb.b = t;
		break;

	case 3:
		rgb.r = p;
		rgb.g = q;
		rgb.b = hsv.z;
		break;

	case 4:
		rgb.r = t;
		rgb.g = p;
		rgb.b = hsv.z;
		break;

	case 5:
		rgb.r = hsv.z;
		rgb.g = p;
		rgb.b = q;
		break;
	}

	return rgb;
}

inline vec3 RgbToHsv(const vec3& rgb)
{
	float h_min = min(min(rgb.r, rgb.g), rgb.b);
	float h_max = max(max(rgb.r, rgb.g), rgb.b);

	// hue
	float hue = 0.0f;

	if(h_max == h_min)
		hue = 0.0f;
	else if(h_max == rgb.r)
		hue = (rgb.g-rgb.b) / (h_max-h_min);
	else if(h_max == rgb.g)
		hue = 2.0f + (rgb.b-rgb.r) / (h_max-h_min);
	else
		hue = 4.0f + (rgb.r-rgb.g) / (h_max-h_min);

	hue /= 6.0f;

	if(hue < 0.0f)
		hue += 1.0f;

	// saturation
	float s = 0.0f;
	if(h_max != 0.0f)
		s = (h_max - h_min)/h_max;

	// lightness
	float l = h_max;

	return vec3(hue, s, l);
}

/*
	Function: HexToRgba
		Converts Hex to Rgba

	Remarks: Hex should be RGBA8
*/
inline vec4 HexToRgba(int hex)
{
	vec4 c;
	c.r = ((hex >> 24) & 0xFF) / 255.0f;
	c.g = ((hex >> 16) & 0xFF) / 255.0f;
	c.b = ((hex >> 8) & 0xFF) / 255.0f;
	c.a = (hex & 0xFF) / 255.0f;

	return c;
}

inline vec3 RgbToHsl(const vec3& RGB)
{
	vec3 HSL;
	float maxColor = MAX3(RGB.r, RGB.g, RGB.b);
	float minColor = MIN3(RGB.r, RGB.g, RGB.b);
	if (minColor == maxColor)
		return vec3(0.0f, 0.0f, RGB.g * 255.0f);
	else
	{
		HSL.l = (minColor + maxColor) / 2;

		if (HSL.l < 0.5f)
			HSL.s = (maxColor - minColor) / (maxColor + minColor);
		else
			HSL.s = (maxColor - minColor) / (2.0f - maxColor - minColor);

		if (RGB.r == maxColor)
			HSL.h = (RGB.g - RGB.b) / (maxColor - minColor);
		else if (RGB.g == maxColor)
			HSL.h = 2.0f + (RGB.b - RGB.r) / (maxColor - minColor);
		else
			HSL.h = 4.0f + (RGB.r - RGB.g) / (maxColor - minColor);

		HSL.h /= 6; //to bring it to a number between 0 and 1
		if (HSL.h < 0) HSL.h++;
	}
	HSL.h = int(HSL.h * 255.0f);
	HSL.s = int(HSL.s * 255.0f);
	HSL.l = int(HSL.l * 255.0f);

	return HSL;
}

inline vec3 GetColorV3(int v)
{
	return HslToRgb(vec3(((v>>16)&0xff)/255.0f, ((v>>8)&0xff)/255.0f, 0.5f+(v&0xff)/255.0f*0.5f));
}


inline vec3 HslToHsv(const vec3& hsl)
{
	return RgbToHsv(HslToRgb(hsl));
}

inline vec3 HsvToHsl(const vec3& hsv)
{
	return RgbToHsl(HsvToRgb(hsv)) / 255.0f;
}


#endif
