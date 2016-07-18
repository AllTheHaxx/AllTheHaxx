/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef BASE_TL_ALLOCATOR_H
#define BASE_TL_ALLOCATOR_H
#include <base/system.h>

template <class T>
class allocator_default
{
public:
	static T *alloc() { return new T; }
	static void free(T *p) { delete p; }

	static T *alloc_array(int size) { return new T[size];/*(T*)mem_alloc(sizeof(T)*size, 1);*/ }
	static void free_array(T *p) { delete[] p;/*mem_free(p);*/ }
};

#endif // BASE_TL_ALLOCATOR_H
