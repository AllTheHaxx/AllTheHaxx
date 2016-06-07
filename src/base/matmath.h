#ifndef BASE_MATMATH_H
#define BASE_MATMATH_H

#include "vmath.h"

// ------------------------------------

template<typename T>
class mat4_base
{
	inline int toindex(int c, int r) { return r+c*4; } // cmo

public:
	T data[4 * 4];

	mat4_base()
	{
		mem_zero(data, sizeof(data)*sizeof(data[0]));
	}

	mat4_base(T diagonal)
	{
		mem_zero(data, sizeof(data)*sizeof(data[0]));
		for(int i = 0; i < 4; i++)
			data[toindex(i,i)] = diagonal;
	}

	mat4_base(const mat4_base<T>& other)
	{
		mem_copy(this->data, other.data, sizeof(data));
	}


	const mat4_base<T> operator*(const mat4_base<T>& other)
	{
		T sum;
		for(int c = 0; c < 4; c++)
		{
			for(int r = 0; r < 4; r++)
			{
				for(int e = 0; e < 4; e++)
				{
					sum += this->data[toindex(r, e)] * other.data[toindex(e, c)];
				}
				this->data[toindex(r, c)] = sum;
			}
		}
		return *this;
	}

	const mat4_base operator*=(const mat4_base& other)
	{
		mat4_base<T> result = (*this) * other;
		mem_copy(this->data, result.data, sizeof(data));
		return *this;
	}


	static const mat4_base<T> identity()
	{
		return mat4_base<T>((T)1);
	}

	static const mat4_base<T> ortho(T left, T right, T bottom, T top, T near, T far)
	{
		mat4_base<T> result((T)1);

		result.data[toindex(0, 0)] = 2.0f / (right - left);
		result.data[toindex(1, 1)] = 2.0f / (top   - bottom);
		result.data[toindex(2, 2)] = 2.0f / (near  - far);

		result.data[toindex(0, 3)] = (left + right) / (left - right);
		result.data[toindex(1, 3)] = (bottom + top) / (bottom - top);
		result.data[toindex(2, 3)] = (far + near) / (far - near);

		return result;
	}

	static const mat4_base<T> translation(const vector3_base<T>& translation)
	{
		mat4_base<T> result((T)1);
		result.data[toindex(0, 3)] = translation.x;
		result.data[toindex(1, 3)] = translation.y;
		result.data[toindex(2, 3)] = translation.z;

		return result;
	}

	static const mat4_base<T> scale(const vector3_base<T>& translation)
	{
		mat4_base<T> result((T)1);
		result.data[toindex(0, 0)] = translation.x;
		result.data[toindex(1, 1)] = translation.y;
		result.data[toindex(2, 2)] = translation.z;

		return result;
	}

	/*static const mat4_base<T> rotation(T angle, const vector3_base<T>& axis)
	{

	}*/

};

typedef mat4_base<float> mat4;

#endif /* BASE_MATMATH_H */
