/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef BASE_MATH_H
#define BASE_MATH_H

#include <stdlib.h>

#define MIN3(x,y,z)  ((y) <= (z) ? \
	((x) <= (y) ? (x) : (y)) \
	: \
	((x) <= (z) ? (x) : (z)))

#define MAX3(x,y,z)  ((y) >= (z) ? \
	((x) >= (y) ? (x) : (y)) \
	: \
	((x) >= (z) ? (x) : (z)))



template <typename T>
inline T clamp(T val, T min, T max)
{
	if(val < min)
		return min;
	if(val > max)
		return max;
	return val;
}

inline float map_val(float val, float a_min, float a_max, float b_min, float b_max)
{
	// pct of 2 on 1 through 3 = 50%
//	float Percentage = (val-min_a) / (max_a-min_a); // y = (x-a)/(x-b)
//	float mapped_val = (-min_b + max_b*Percentage) / (Percentage - 1); // x = (-a + b y) / (y - 1)
//	return mapped_val;
	const float slope = (b_max - b_min) / (a_max - a_min);
	float mapped = slope * (val - a_min) + b_min;
	return mapped;
}

inline float sign(float f)
{
	return f<0.0f?-1.0f:1.0f;
}

inline int round_to_int(float f)
{
	if(f > 0)
		return (int)(f+0.5f);
	return (int)(f-0.5f);
}

template<typename T, typename TB>
inline T mix(const T& a, const T& b, TB amount)
{
	return a + (b-a)*amount;
}

inline float frandom() { return rand()/(float)(RAND_MAX); }

/* smoothly sets the value to target, must be called every tick to do so;
 * val is a pointer to a static float
 * returns the newly calculated value */
inline float smooth_set(float* val, float target, float delay, float RenderFrameTime)
{
	if(delay <= 0.0f)
		delay = 0.01f;

	delay *= (0.005f/RenderFrameTime);

	if(delay < 1.0f || *val == target)
		return *val;

/*	if(snaprange)
	{
		if(*val > target * (1.0f - snaprange))
			*val = target;
		if(*val < target + snaprange)
			*val = target;
	}*/

	if(*val < target)
		*val += (target-*val)/delay;
	if(*val > target)
		*val -= (*val-target)/delay;

	return *val;
}

// float to fixed
inline int f2fx(float v) { return (int)(v*(float)(1<<10)); }
inline float fx2f(int v) { return v*(1.0f/(1<<10)); }

inline int gcd(int a, int b)
{
	while(b != 0)
	{
		int c = a % b;
		a = b;
		b = c;
	}
	return a;
}

class fxp
{
	int value;
public:
	void set(int v) { value = v; }
	int get() const { return value; }
	fxp &operator = (int v) { value = v<<10; return *this; }
	fxp &operator = (float v) { value = (int)(v*(float)(1<<10)); return *this; }
	operator float() const { return value/(float)(1<<10); }
};

const float pi = 3.1415926535897932384626433f;

template <typename T> inline T min(T a, T b) { return a<b?a:b; }
template <typename T> inline T max(T a, T b) { return a>b?a:b; }
template <typename T> inline T absolute(T a) { return a<T(0)?-a:a; }

#endif // BASE_MATH_H
