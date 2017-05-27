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
		MACRO_ALLOC_HEAP()
	public:
		CChunk *m_pNext;
		T *m_pData;

		CChunk()
		{
			m_pNext = NULL;
			m_pData = mem_allocb(T, 1);
		}

		~CChunk()
		{
			if(m_pData)
				mem_free(m_pData);
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
		CChunk *pChunk = new CChunk();
		return pChunk->m_pData;
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
	 * @remark This object must be 'freed' by using Free() or Delete(); no automatic memory management is being performed!
	 */
	T* Allocate()
	{
		// pool is empty? create a new object and return it
		if(!m_pFirst)
			return CPool<T>::Alloc();

		// take a chunk from the allocated pool and return it
		CChunk *pChunk = m_pFirst;
		m_pFirst = pChunk->m_pNext;

		return pChunk->m_pData;
	}

	/**
	 * Like Allocate(), but also calls the constructor
	 */
	T* New()
	{
		T* pData = Allocate();

		// invoke the ctor and return a pointer to the object
		pData->T();
		return pData;
	}

	/**
	 * Returns the data back to the pool. After this operation, the data must obviously be considered invalid.
	 * @param pData the data to free
	 * @remark The data doesn't need to have been allocated from _this_ pool
	 */
	void Free(T *pData)
	{
		// add the chunk back to the pool
		AddToPool(DataToChunk(pData));
	}

	/**
	 * Like Free(), but also calls the destructor
	 */
	void Delete(T *pData)
	{
		// call the destructor
		pData->~T();
		Free(pData);
	}

	/**
	 * Pre-allocate as many elements as given into this pool, to have a fair share of memory already handy when needed
	 * @param SizeHint How many elements to pre-allocate
	 */
	void HintSize(unsigned int SizeHint)
	{
		// never shrink the pool
		if(m_Size >= SizeHint)
			return;

		// allocate as many chunks as needed and add them to the reserved pool
		unsigned int AllocSize = m_Size - SizeHint;
		for(unsigned int i = 0; i < AllocSize; i++)
			AddToPool(DataToChunk(Alloc()));
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
			delete pChunk;
			pChunk = pNext;
		}
		m_pFirst = NULL;
		m_pLast = NULL;
		m_Size = 0;
	}
};

#endif
