#ifndef BASE_SYSTEMPP_LINKED_LIST_H
#define BASE_SYSTEMPP_LINKED_LIST_H

#include "pool.h"

template <class T>
class CLinkedList
{
public:
	class CNode
	{
		friend class CLinkedList;

		CNode *m_pPrev;
		CNode *m_pNext;
		T* m_pData;

	public:
		CNode()
		{
			m_pPrev = NULL;
			m_pNext = NULL;
			m_pData = NULL;
		}

		void Relink(CNode *pPrev, CNode *pNext, T* pData)
		{
			m_pPrev = pPrev;
			m_pNext = pNext;
			m_pData = pData;

			if(pPrev)
				pPrev->m_pNext = this;
			if(pNext)
				pNext->m_pPrev = this;
		}

		CNode *Prev() { return m_pPrev; }
		CNode *Next() { return m_pNext; }
		T* Data() { return m_pData; }
	};
private:

	CNode *m_pFirst;
	CNode *m_pLast;
	CPool<CNode> m_Pool;

public:
	CLinkedList()
	{
		m_pFirst = NULL;
		m_pLast = NULL;
	}

	/**
	 * Appends an element to the end of the chain
	 * @param pData pointer to the data to store
	 */
	void PushBack(T *pData)
	{
		if(!m_pFirst)
		{
			CNode *pNode = m_Pool.New();
			pNode->Relink(NULL, NULL, pData);
			m_pFirst = pNode;
			m_pLast = m_pFirst;
		}
		else
		{
			CNode *pNode = m_Pool.New();
			pNode->Relink(m_pLast, NULL, pData);
		}
	}

	/**
	 * Adds an element to the front of the chain
	 * @param pData pointer to the data to store
	 */
	void PushFront(T *pData)
	{
		if(!m_pFirst)
		{
			CNode *pNode = m_Pool.New();
			pNode->Relink(NULL, NULL, pData);
			m_pFirst = pNode;
			m_pLast = m_pFirst;
		}
		else
		{
			CNode *pNode = m_Pool.New();
			pNode->Relink(NULL, m_pFirst, pData);
		}
	}

	/**
	 * Deletes all elements of the chain
	 */
	void DisposeChain()
	{
		while(m_pFirst)
		{
			CNode *pNext = m_pFirst;
			m_Pool.Delete(m_pFirst);
			m_pFirst = pNext;
		}
	}

};

#endif
