//Copyright (c) by MAP94 2014
#ifndef ENGINE_STREAM_H
#define ENGINE_STREAM_H
#include <base/system.h>
#include "string.h"

struct CStreamBlock
{
    char *m_pData;
    int m_Start;
    int m_Size;
    CStreamBlock *m_pNext;
};

class CStream
{
private:
    CStreamBlock *m_pFirst;
    CStreamBlock *m_pLast;

    int m_Size;


    int GetInternal(char *pData, int Size) const;
    void RemoveInternal(int Size);
    void AddInternal(const char *pData, int Size);
    void ClearInternal();
public:
    CStream();
    CStream(const CStream& Other);
    ~CStream();

    void Push(const char *pData, int Size = -1);
    void Add(const char *pData, int Size = -1);
    int Get(char *pData, int Size) const;
    int Get(char *pData, int Size);
    int Pop(char *pData, int Size);
    void Remove(int Size);
    void Clear();

    int Contains(const char *pData, int Size = -1);

    int Size() const { return m_Size; }

    void operator = (const CStream& Other);
    void operator += (const CStream& Other);

};

//TODO:
//optimize -> one string ?

#endif
