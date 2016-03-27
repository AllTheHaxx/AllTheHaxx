/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <engine/map.h>
#include <engine/storage.h>
#include "datafile.h"

class CMap : public IEngineMap
{
	int m_ActiveDataFile;
	CDataFileReader m_DataFile[16];
public:
	CMap() { m_ActiveDataFile = 0;}

	virtual void *GetData(int Index) { return m_DataFile[m_ActiveDataFile].GetData(Index); }
	virtual int GetUncompressedDataSize(int Index) { return m_DataFile[m_ActiveDataFile].GetUncompressedDataSize(Index); }
	virtual void *GetDataSwapped(int Index) { return m_DataFile[m_ActiveDataFile].GetDataSwapped(Index); }
	virtual void UnloadData(int Index) { m_DataFile[m_ActiveDataFile].UnloadData(Index); }
	virtual void *GetItem(int Index, int *pType, int *pID) { return m_DataFile[m_ActiveDataFile].GetItem(Index, pType, pID); }
	virtual int GetItemSize(int Index) { return m_DataFile[m_ActiveDataFile].GetItemSize(Index); }
	virtual void GetType(int Type, int *pStart, int *pNum) { m_DataFile[m_ActiveDataFile].GetType(Type, pStart, pNum); }
	virtual void *FindItem(int Type, int ID) { return m_DataFile[m_ActiveDataFile].FindItem(Type, ID); }
	virtual int NumItems() { return m_DataFile[m_ActiveDataFile].NumItems(); }
	virtual void SetActiveDataFile(int n) { m_ActiveDataFile = n; }
	virtual int ActiveMapDataFile() { return m_ActiveDataFile; }

	virtual void Unload(int n)
	{
		m_DataFile[n].Close();
	}

	virtual bool Load(int n, const char *pMapName)
	{
		IStorageTW *pStorage = Kernel()->RequestInterface<IStorageTW>();
		if(!pStorage)
			return false;
		return m_DataFile[n].Open(pStorage, pMapName, IStorageTW::TYPE_ALL);
	}

	virtual bool IsLoaded(int n)
	{
		return m_DataFile[n].IsOpen();
	}

	virtual unsigned Crc(int n)
	{
		return m_DataFile[n].Crc();
	}
};

extern IEngineMap *CreateEngineMap() { return new CMap; }
