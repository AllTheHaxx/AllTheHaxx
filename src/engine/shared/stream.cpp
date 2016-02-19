#include "stream.h"
#include <base/math.h>

CStream::CStream()
{
    m_pFirst = 0;
    m_pLast = 0;
    m_Size = 0;
}

CStream::CStream(const CStream& Other)
{
    m_pFirst = 0;
    m_pLast = 0;
    m_Size = 0;

    if (Other.m_Size <= 0)
        return;
    int Size = Other.m_Size;
    char *pNew = new char[Size];
    Other.Get(pNew, Size);

    AddInternal(pNew, Size);

    delete []pNew;
}

CStream::~CStream()
{
    ClearInternal();
}

void CStream::Push(const char *pData, int Size)
{
    AddInternal(pData, Size);
}

void CStream::Add(const char *pData, int Size)
{
    AddInternal(pData, Size);
}

void CStream::AddInternal(const char *pData, int Size)
{
    if (Size == -1)
        Size = str_length(pData);
    if (Size == 0)
        return;
    CStreamBlock *pNew = new CStreamBlock();

    pNew->m_pNext = 0;
    pNew->m_Start = 0;
    pNew->m_Size = Size;
    pNew->m_pData = (char *)malloc(Size);
    mem_copy(pNew->m_pData, pData, Size);

    if (m_pFirst == 0)
    {
        m_pFirst = pNew;
        m_pLast = pNew;
    }
    else
    {
        CStreamBlock *pLast = 0;
        if (m_pLast)
            pLast = m_pLast;
        else
        {
            pLast = m_pFirst;
            while(pLast->m_pNext)
            {
                pLast = pLast->m_pNext;
            }
        }

        pLast->m_pNext = pNew;
        m_pLast = pNew;
    }
    m_Size += Size;
}

int CStream::Pop(char *pData, int Size)
{
    Size = GetInternal(pData, Size);
    RemoveInternal(Size);
    return Size;
}

int CStream::Get(char *pData, int Size)
{
    Size = GetInternal(pData, Size);
    return Size;
}

int CStream::Get(char *pData, int Size) const
{
    Size = GetInternal(pData, Size);
    return Size;
}

int CStream::GetInternal(char *pData, int Size) const
{
    int ReadSize = 0;

    if (m_pFirst == 0)
    {
        return 0;
    }

    CStreamBlock *pItem = m_pFirst;
    while(Size > 0 && pItem)
    {
        mem_copy(pData + ReadSize, pItem->m_pData + pItem->m_Start, min(pItem->m_Size, Size));
        ReadSize += min(pItem->m_Size, Size);
        Size -= min(pItem->m_Size, Size);
        pItem = pItem->m_pNext;
    }
    return ReadSize;
}

void CStream::Remove(int Size)
{
    RemoveInternal(Size);
}

void CStream::RemoveInternal(int Size)
{
    if (m_pFirst == 0 || Size <= 0)
    {
        return;
    }
    m_Size -= Size;
    if (m_Size <= 0)
    {
        m_Size = 0;
        m_pLast = 0;
    }

    CStreamBlock *pItem = m_pFirst;
    while(Size > 0 && pItem)
    {
        if (Size >= pItem->m_Size)
        {
            Size = Size - pItem->m_Size;

            m_pFirst = pItem->m_pNext;
            free(pItem->m_pData);
            delete pItem;
            pItem = m_pFirst;
            continue;
        }
        else
        {
            pItem->m_Size = pItem->m_Size - Size;
            pItem->m_Start = pItem->m_Start + Size;

            Size = 0;
        }
        pItem = pItem->m_pNext;
    }
}

int CStream::Contains(const char *pData, int Size)
{
    int Result = -1;
    if (!pData)
        return Result;
    if (Size == -1)
        Size = str_length(pData);
    if (Size == 0)
        return Result;

    for (int j = 0; j < m_Size; j++)
    {
        for (int i = 0; i < Size; i++)
        {
            int ReadIndex = i + j;
            CStreamBlock *pReadBlock = m_pFirst;
            while (pReadBlock && ReadIndex >= pReadBlock->m_Size)
            {
                ReadIndex -= pReadBlock->m_Size;
                pReadBlock = pReadBlock->m_pNext;
            }
            if (pReadBlock && pReadBlock->m_pData[pReadBlock->m_Start + ReadIndex] == pData[i])
            {
                if (i + 1 == Size)
                {
                    Result = j;
                    break;
                }
            }
            else
                break;
        }
        if (Result != -1)
            break;
    }

    return Result;
}

void CStream::ClearInternal()
{
    if (m_pFirst == 0)
    {
        return;
    }
    m_Size = 0;

    CStreamBlock *pItem = m_pFirst;
    while(pItem)
    {
        m_pFirst = pItem->m_pNext;
        free(pItem->m_pData);
        delete pItem;
        pItem = m_pFirst;
    }
    m_pLast = 0;
}

void CStream::Clear()
{
    ClearInternal();
}

void CStream::operator = (const CStream& Other) //todo optimize this
{
    ClearInternal();
    if (Other.m_Size <= 0)
    {
        return;
    }
    int Size = Other.m_Size;
    char *pNew = new char[Size];
    Other.Get(pNew, Size);

    AddInternal(pNew, Size);

    delete []pNew;
    return;
}

void CStream::operator += (const CStream &Other)
{
    if (Other.m_Size <= 0)
        return;
    if (m_Size == 0)
    {
        *this = Other;
        return;
    }
    int Size = Other.m_Size;
    char *pNew = new char[Size];
    Other.Get(pNew, Size);

    AddInternal(pNew, Size);

    delete []pNew;
    return;
}
