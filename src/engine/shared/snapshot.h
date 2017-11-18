/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_SNAPSHOT_H
#define ENGINE_SHARED_SNAPSHOT_H

#include <base/system.h>
#include <base/system++/pool.h>

// CSnapshot

class CSnapshotItem
{
public:
	int m_TypeAndID;

	int *Data() { return (int *)(this+1); }
	int Type() { return m_TypeAndID>>16; }
	int ID() { return m_TypeAndID&0xffff; }
	int Key() { return m_TypeAndID; }
};


class CSnapshot
{
	friend class CSnapshotBuilder;
	int m_DataSize;
	int m_NumItems;

	int *Offsets() const { return (int *)(this+1); }
	char *DataStart() const { return (char*)(Offsets()+m_NumItems); }

public:
	enum
	{
		OFFSET_UUID_TYPE=0x4000,
		MAX_TYPE=0x7fff,
		MAX_PARTS=64,
		MAX_SIZE=MAX_PARTS*1024
	};

	void Clear() { m_DataSize = 0; m_NumItems = 0; }
	int NumItems() const { return m_NumItems; }
	CSnapshotItem *GetItem(int Index);
	int GetItemSize(int Index);
	int GetItemIndex(int Key);
	int GetItemType(int Index);

	int Crc();
	void DebugDump();
};


// CSnapshotDelta

class CSnapshotDelta
{
public:
	class CData
	{
	public:
		int m_NumDeletedItems;
		int m_NumUpdateItems;
		int m_NumTempItems; // needed?
		int m_pData[1];
	};

private:
	// TODO: strange arbitrary number
	short m_aItemSizes[64];
	int m_aSnapshotDataRate[0xffff];
	int m_aSnapshotDataUpdates[0xffff];
	int m_SnapshotCurrent;
	CData m_Empty;

	void UndiffItem(int *pPast, int *pDiff, int *pOut, int Size);

public:
	CSnapshotDelta();
	int GetDataRate(int Index) { return m_aSnapshotDataRate[Index]; }
	int GetDataUpdates(int Index) { return m_aSnapshotDataUpdates[Index]; }
	void SetStaticsize(int ItemType, int Size);
	CData *EmptyDelta();
	int CreateDelta(class CSnapshot *pFrom, class CSnapshot *pTo, void *pData);
	int UnpackDelta(class CSnapshot *pFrom, class CSnapshot *pTo, void *pData, int DataSize);
};


// CSnapshotStorage

class CSnapshotStorage
{
public:
	class CHolder
	{
	public:
		CHolder *m_pPrev;
		CHolder *m_pNext;

		int64 m_Tagtime;
		int m_Tick;

		int m_SnapSize;
		CSnapshot *m_pSnap;
		CSnapshot *m_pAltSnap;
	};

	~CSnapshotStorage();

	CHolder *m_pFirst;
	CHolder *m_pLast;
	CPool<CHolder> m_HeapPool;

	void Init();
	void PurgeAll();
	void PurgeUntil(int Tick);
	void Add(int Tick, int64 Tagtime, int DataSize, void *pData, int CreateAlt);
	int Get(int Tick, int64 *Tagtime, CSnapshot **pData, CSnapshot **ppAltData);
};

class CSnapshotBuilder
{
	enum
	{
		MAX_ITEMS = 1024,
		MAX_EXTENDED_ITEM_TYPES = 64,
	};

	char m_aData[CSnapshot::MAX_SIZE];
	int m_DataSize;

	int m_aOffsets[MAX_ITEMS];
	int m_NumItems;

	int m_aExtendedItemTypes[MAX_EXTENDED_ITEM_TYPES];
	int m_NumExtendedItemTypes;

	void AddExtendedItemType(int Index);
	int GetExtendedItemTypeIndex(int TypeID);

public:
	CSnapshotBuilder();

	void Init();

	void *NewItem(int Type, int ID, int Size);

	CSnapshotItem *GetItem(int Index);
	int *GetItemData(int Key);

	int Finish(void *Snapdata);
};


#endif // ENGINE_SNAPSHOT_H
