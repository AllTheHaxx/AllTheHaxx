/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef BASE_TL_SORTED_ARRAY_H
#define BASE_TL_SORTED_ARRAY_H

#include "base/tl/algorithm.h"
#include "base/tl/array.h"

template <class T, class ALLOCATOR = allocator_default<T> >
class sorted_array : public array<T, ALLOCATOR>
{
public:
	typedef plain_range_sorted<T> range;

protected:
	typedef array<T, ALLOCATOR> parent;

	// insert and size is not allowed
	int insert(const T& item, typename parent::range r) { dbg_break(); return 0; }
	int set_size(int new_size) { dbg_break(); return 0; }

public:

	int add(const T& item)
	{
		// instantiated sorted_array with pointer contents; sorting will be random!
		#if defined(CONF_DEBUG)
		if(std::is_pointer<T>::value == 1)
			dbg_break();
		#endif

		return parent::insert(item, partition_binary(all(), item));
	}

	int add_unsorted(const T& item)
	{
		return parent::add(item);
	}

	void sort_range()
	{
		#if defined(CONF_DEBUG)
		if(std::is_pointer<T>::value == 1)
			dbg_break();
		#endif

		sort(all());
	}

	/*
		Function: all
			Returns a sorted range that contains the whole array.
	*/
	range all() { return range(parent::list, parent::list+parent::num_elements); }
};

// used for sorting pointers to heap objects by the objects value instead of the pointer's
template <class T, class ALLOCATOR = allocator_default<T> >
class sorted_ptr_array : public sorted_array<T, ALLOCATOR>
{
public:
	int add(const T& item)
	{
		return sorted_array<T, ALLOCATOR>::parent::insert(item, partition_binary_ptr(sorted_array<T, ALLOCATOR>::all(), item));
	}

	void sort_range()
	{
		sort_ptr(sorted_array<T, ALLOCATOR>::all());
	}
};

#endif // TL_FILE_SORTED_ARRAY_HPP
