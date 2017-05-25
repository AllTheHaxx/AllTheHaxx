#ifndef BASE_SYSTEMPP_POOL_H
#define BASE_SYSTEMPP_POOL_H

#include <base/system.h>

/**
 * This class provides an interface to manage and heap memory with only little overhead.
 * Objects allocated from this pool can later be returned to it, so that they don't have
 *   to be allocated over and over again but can simply be reused, which is much quicker.
 * @tparam T Type of the objects this pool should provide
 */
template <class T>
class CPool
{
	struct CChunk
	{
		CChunk *m_pNext;
		T m_Data;

		CChunk()
		{
			m_pNext = NULL;
		}
	};

	CChunk *m_pFirst;
	CChunk *m_pLast;
	unsigned int m_Size;


	void AddToPool(CChunk *pChunk)
	{
		if(m_pLast)
			m_pLast->m_pNext = pChunk;
		else
		{
			m_pFirst = pChunk;
			m_pLast = m_pFirst;
		}
		m_Size++;
	}

	/**
	 * Allocate a chunk of data
	 * @return A pointer to the data which is part of the chunk
	 */
	static T* Alloc()
	{
		CChunk *pChunk = mem_allocb(CChunk, 1);
		return &(pChunk->m_Data);
	}

	inline CChunk *DataToChunk(T* pData) const
	{
		return (CChunk*)(pData-sizeof(CChunk*));
	}

public:
	CPool()
	{
		m_pFirst = NULL;
		m_pLast = NULL;
		m_Size = 0;
	}

	~CPool()
	{
		Clear();
	}

	/**
	 * Allocates a new object bound to this bool
	 * @return the new object
	 * @remark This object must be 'freed' by using Free(); no automatic memory management is being performed!
	 * @remark This function invokes T's default constructor
	 */
	T* Allocate()
	{
		if(!m_pFirst)
			return CPool<T>::Alloc();

		// take a chunk from the allocated pool
		CChunk *pChunk = m_pFirst;
		m_pFirst = pChunk->m_pNext;

		// invoke the ctor and return a pointer to the object
		pChunk->m_Data.T();
		return &(pChunk->m_Data);
	}


	/**
	 * Returns the data back to the pool. After this operation, the data must obviously be considered invalid.
	 * @param pData the data to free
	 * @remark The data doesn't need to have been allocated from _this_ pool
	 */
	void Free(T *pData)
	{
		// add the chunk back to the reserved pool
		CChunk *pChunk = DataToChunk(pData);
		AddToPool(pChunk);
	}

	/**
	 * Pre-allocate as many elements as given into this pool, to have a fair share of memory already handy when needed
	 * @param SizeHint How many elements to pre-allocate
	 */
	void HintSize(unsigned int SizeHint)
	{
		if(m_Size >= SizeHint)
			return;

		// allocate as many chunks as needed and add them to the reserved pool
		unsigned int AllocSize = m_Size - SizeHint;
		for(unsigned int i = 0; i < AllocSize; i++)
			Free(Alloc());
	}

	/**
	 * Clears the pool, deleting all its elements (technically undoes the effect of HintSize)
	 * @remark this function does NOT take care of any elements that are still in use, they must be returned to the pool using Free() first!
	 */
	void Clear()
	{
		CChunk *pChunk = m_pFirst;
		while(pChunk)
		{
			CChunk *pNext = pChunk->m_pNext;
			mem_free(pChunk);
			pChunk = pNext;
		}
		m_pFirst = NULL;
		m_pLast = NULL;
		m_Size = 0;
	}
};

#endif
