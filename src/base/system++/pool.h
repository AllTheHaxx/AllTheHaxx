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
	class CChunk
	{
	public:
		struct
		{
			friend class CChunk;
			CChunk *m_pNext;
		private:
			unsigned m_ExtraSpace;
		} m_Header;

		void *m_pDataStart; // the address of this variable is the pointer to the start of the stored data


		void Init(unsigned ExtraSpace)
		{
			m_Header.m_pNext = NULL;
			m_Header.m_ExtraSpace = ExtraSpace;
			m_pDataStart = (void*)0x700C1349; // magic number 'too clean' for debugging
		}

		inline T* GetData() { return (T*)&m_pDataStart; }
		inline unsigned GetExtraSpace() const { return m_Header.m_ExtraSpace; }

		/* Layout of a Chunk
		 *  CCCCUUUUDDDDDDDD...
		 *  ^   ^   ^
		 *  |   |   &m_pDataStart
		 *  |   &m_ExtraSpace
		 *  &m_pNext
		 */
		static CChunk* DataToChunk(T *pData) // this will be &m_pDataStart
		{
			return reinterpret_cast<CChunk *>((char*)pData - sizeof(m_Header));
		}
	};

	CChunk *m_pFirst;
	unsigned int m_Size;


	void AddToPool(CChunk *pChunk)
	{
		CChunk *pOldFirst = m_pFirst;
		m_pFirst = pChunk;
		pChunk->m_Header.m_pNext = pOldFirst;
		m_Size++;
	}

	CChunk* TakeFromPool(unsigned ExtraSpace)
	{
		// take a chunk from the allocated pool and return it
		CChunk *pChunk = m_pFirst;
		m_pFirst = pChunk->m_Header.m_pNext; // rip it out either way, even if we reallocate it
		m_Size--;

		// ensure there is enough size
		if(pChunk->GetExtraSpace() < ExtraSpace)
		{
			// reallocating is much faster than searching for a large enough chunk
			//  and probably having to reallocate after searching anyway
			mem_free(pChunk); // we need to remove the chunk in order to replace it by a larger one
			return CPool<T>::AllocChunk(ExtraSpace);
		}
		else
		{
			return pChunk;
		}
	}

	/**
	 * Allocate a chunk of data
	 * @return A pointer to the data which is part of the chunk
	 */
	static CChunk* AllocChunk(unsigned ExtraSpace)
	{
		CChunk *pChunk = static_cast<CChunk *>(mem_alloc(NeededChunkSize(ExtraSpace), 0));
		pChunk->Init(ExtraSpace);
		return pChunk;
	}

	static inline unsigned NeededChunkSize(unsigned ExtraSpace) { return sizeof(CChunk) + sizeof(T) + ExtraSpace; }

public:
	CPool()
	{
		m_pFirst = NULL;
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
	inline T* Allocate(unsigned ExtraSpace = 0)
	{
		// pool is empty? create a new object and return it right away
		if(!m_pFirst)
			return CPool<T>::AllocChunk(ExtraSpace)->GetData();

		return TakeFromPool(ExtraSpace)->GetData();
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
	 * @remark The data doesn't need to have been allocated from _this_ pool -> you can give it back to any of the same type
	 */
	void Free(T *pData)
	{
		// add the chunk back to the pool
		AddToPool(CChunk::DataToChunk(pData));
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
	void HintSize(unsigned int SizeHint, unsigned ExtraSpace = 0)
	{
		// never shrink the pool
		if(m_Size >= SizeHint)
			return;

		// allocate as many chunks as needed and add them to the reserved pool
		unsigned int AllocSize = SizeHint - m_Size;
		for(unsigned int i = 0; i < AllocSize; i++)
			AddToPool(AllocChunk(ExtraSpace));
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
			CChunk *pNext = pChunk->m_Header.m_pNext;
			mem_free(pChunk);
			pChunk = pNext;
		}
		m_pFirst = NULL;
		m_Size = 0;
	}
};

#endif
