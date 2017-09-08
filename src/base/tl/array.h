/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef BASE_TL_ARRAY_H
#define BASE_TL_ARRAY_H

#include "base/tl/range.h"
#include "base/tl/allocator.h"
#include "base/system++/system++.h"


/*
	Class: array
		Normal dynamic array class

	Remarks:
		- Grows 50% each time it needs to fit new items
		- Use set_size() if you know how many elements
		- Use optimize() to reduce the needed space.
*/
template <class T, class ALLOCATOR = allocator_default<T> >
class array : private ALLOCATOR
{
	void init()
	{
		list = 0x0;
		clear();
	}

public:
	typedef plain_range<T> range;

	/*
		Function: array constructor
	*/
	array()
	{
		init();
	}

	/*
		Function: array copy constructor
	*/
	array(const array &other)
	{
		init();
		set_size(other.size());
		for(int i = 0; i < size(); i++)
			(*this)[i] = other[i];
	}


	/*
		Function: array destructor
	*/
	~array()
	{
		if(list)
			ALLOCATOR::free_array(list);
		list = 0x0;
	}


	/*
		Function: delete_all

		Remarks:
			- Invalidates ranges
	*/
	void delete_all()
	{
		for(int i = 0; i < size(); i++)
			delete list[i];
		clear();
	}


	/*
		Function: clear

		Remarks:
			- Invalidates ranges
	*/
	void clear()
	{
		if(list)
			ALLOCATOR::free_array(list);
		//list_size = 1;
		//list = ALLOCATOR::alloc_array(list_size);
		list_size = 0;
		list = 0x0;
		num_elements = 0;
	}

	void clear_erase()
	{
		// erase everything out
		for(int i = 0; i < size(); i++)
			mem_zero(&list[i], sizeof(T));
		clear();
	}

	/*
		Function: size
	*/
	int size() const
	{
		return num_elements;
	}

	/*
		Function: empty
	*/
	bool empty() const
	{
		return num_elements == 0;
	}

	/*
		Function: remove_index_fast

		Remarks:
			- Invalidates ranges
	*/
	void remove_index_fast(int index)
	{
		if(index < 0)
			index += num_elements;
		if(dbg_assert_strict(index >= 0 && index < num_elements, "index out of range"))
			return;
		if(index != num_elements-1)
			list[index] = list[num_elements-1];
		set_size(size()-1);
	}

	/*
		Function: remove_fast

		Remarks:
			- Invalidates ranges
	*/
	bool remove_fast(const T& item)
	{
		for(int i = 0; i < size(); i++)
			if(list[i] == item)
			{
				remove_index_fast(i);
				return true;
			}
		return false;
	}

	/*
		Function: remove_index

		Remarks:
			- Invalidates ranges
	*/
	void remove_index(int index)
	{
		if(index < 0)
			index += num_elements;
		if(dbg_assert_strict(index >= 0 && index < num_elements, "index out of range"))
			return;
		for(int i = index+1; i < num_elements; i++)
			list[i-1] = list[i];

		set_size(size()-1);
	}

	/*
		Function: remove

		Remarks:
			- Invalidates ranges
	*/
	bool remove(const T& item)
	{
		for(int i = 0; i < size(); i++)
			if(list[i] == item)
			{
				remove_index(i);
				return true;
			}
		return false;
	}

	/*
		Function: add
			Adds an item to the array.

		Arguments:
			item - Item to add.

		Remarks:
			- Invalidates ranges
			- See remarks about <array> how the array grows.
	*/
	int add(const T& item)
	{
		incsize();
		set_size(size()+1);
		//mem_copy(&list[num_elements-1], &item, sizeof(T));
		list[num_elements-1] = item;
		return num_elements-1;
	}

	/*
		Function: insert
			Inserts an item into the array at a specified location.

		Arguments:
			item - Item to insert.
			r - Range where to insert the item

		Remarks:
			- Invalidates ranges
			- See remarks about <array> how the array grows.
	*/
	int insert(const T& item, range r)
	{
		if(r.empty())
			return add(item);

		int index = (int)(&r.front()-list);
		incsize();
		set_size(size()+1);

		for(int i = num_elements-1; i > index; i--)
			list[i] = list[i-1];

		list[index] = item;

		return num_elements-1;
	}

	T* find(const T& item, int *index = 0)
	{
		for(int i = 0; i < num_elements; i++)
		{
			if(list[i] == item)
			{
				if(index)
					*index = i;
				return &(list[i]);
			}
		}

		return NULL;
	}

	/*
		Function: operator[]
	*/
	T& operator[] (int index)
	{
		if(index < 0)
			index += num_elements;
		dbg_assert_legacy(index >= 0 && index < num_elements, "index out of range");
		return list[index];
	}

	/*
		Function: const operator[]
	*/
	const T& operator[] (int index) const
	{
		if(index < 0)
			index += num_elements;
		dbg_assert_legacy(index >= 0 && index < num_elements, "index out of range");
		return list[index];
	}

	/*
		Function: base_ptr
	*/
	T *base_ptr()
	{
		return list;
	}

	/*
		Function: base_ptr
	*/
	const T *base_ptr() const
	{
		return list;
	}

	/*
		Function: set_size
			Resizes the array to the specified size.

		Arguments:
			new_size - The new size for the array.
	*/
	void set_size(int new_size)
	{
		if(list_size < new_size)
			alloc(new_size);
		num_elements = new_size;
	}

	/*
		Function: hint_size
			Allocates the number of elements wanted but
			does not increase the list size.

		Arguments:
			hint - Size to allocate.

		Remarks:
			- If the hint is smaller then the number of elements, nothing will be done.
			- Invalidates ranges
	*/
	void hint_size(int hint)
	{
		if(num_elements < hint)
			alloc(hint);
	}


	/*
		Function: optimize
			Removes unnessasary data, returns how many bytes was earned.

		Remarks:
			- Invalidates ranges
	*/
	int optimize()
	{
		int before = memusage();
		alloc(num_elements);
		return before - memusage();
	}

	/*
		Function: memusage
			Returns how much memory this dynamic array is using
	*/
	int memusage()
	{
		return sizeof(array) + sizeof(T)*list_size;
	}

	/*
		Function: operator=(array)

		Remarks:
			- Invalidates ranges
	*/
	array &operator = (const array &other)
	{
		set_size(other.size());
		for(int i = 0; i < size(); i++)
			(*this)[i] = other[i];
		return *this;
	}

	/*
		Function: all
			Returns a range that contains the whole array.
	*/
	range all() { return range(list, list+num_elements); }
protected:

	void incsize()
	{
		if(num_elements == list_size)
		{
			if(list_size < 2)
				alloc(list_size+2);
			else
				alloc(list_size+list_size/2);
		}
	}

	void alloc(int new_len)
	{
		list_size = new_len;
		T *new_list = ALLOCATOR::alloc_array(list_size);

		int end = num_elements < list_size ? num_elements : list_size;
		for(int i = 0; i < end; i++)
			new_list[i] = list[i];

		if(list)
			ALLOCATOR::free_array(list);

		num_elements = num_elements < list_size ? num_elements : list_size;
		list = new_list;
	}

	T *list;
	int list_size;
	int num_elements;
};

#endif // TL_FILE_ARRAY_HPP
