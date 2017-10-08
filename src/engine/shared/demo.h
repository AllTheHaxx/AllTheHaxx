/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_DEMO_H
#define ENGINE_SHARED_DEMO_H

#include <engine/demo.h>
#include <engine/shared/protocol.h>

#include "snapshot.h"

class CDemoRecorder : public IDemoRecorder
{
	class IConsole *m_pConsole;
	IOHANDLE m_File;
	int m_LastTickMarker;
	int m_LastKeyFrame;
	int m_FirstTick;
	unsigned char m_aLastSnapshotData[CSnapshot::MAX_SIZE];
	class CSnapshotDelta *m_pSnapshotDelta;
	int m_NumTimelineMarkers;
	int m_aTimelineMarkers[MAX_TIMELINE_MARKERS];
	bool m_NoMapData;
	unsigned int m_MapSize;
	unsigned char *m_pMapData;

	DEMOFUNC_FILTER m_pfnFilter;
	void *m_pUser;

	void WriteTickMarker(int Tick, int Keyframe);
	void Write(int Type, const void *pData, int Size);
public:
	CDemoRecorder(class CSnapshotDelta *pSnapshotDelta, bool NoMapData = false);
	CDemoRecorder() {}

	int Start(class IStorageTW *pStorage, class IConsole *pConsole, const char *pFilename, const char *pNetversion, const char *pMap, unsigned MapCrc, const char *pType, unsigned int MapSize, unsigned char *pMapData, IOHANDLE MapFile = 0, DEMOFUNC_FILTER pfnFilter = 0, void *pUser = 0);
	int Stop();
	void AddDemoMarker();

	void RecordSnapshot(int Tick, const void *pData, int Size);
	void RecordMessage(const void *pData, int Size);

	bool IsRecording() const { return m_File != 0; }

	int Length() const { return (m_LastTickMarker - m_FirstTick)/SERVER_TICK_SPEED; }
};

class CDemoPlayer : public IDemoPlayer
{
public:
	class IListener
	{
	public:
		virtual ~IListener() {}
		virtual void OnDemoPlayerSnapshot(void *pData, int Size) = 0;
		virtual void OnDemoPlayerMessage(void *pData, int Size) = 0;
	};

	struct CPlaybackInfo
	{
		CDemoHeader m_Header;
		CTimelineMarkers m_TimelineMarkers;

		IDemoPlayer::CInfo m_Info;

		int64 m_LastUpdate;
		int64 m_CurrentTime;

		int m_SeekablePoints;

		int m_NextTick;
		int m_PreviousTick;

		float m_IntraTick;
		float m_TickTime;
	};

	struct CMapInfo
	{
		char m_aName[128];
		int m_Crc;
		int m_Size;
	};

private:
	IListener *m_pListener;


	// Playback
	struct CKeyFrame
	{
		long m_Filepos;
		int m_Tick;
	};

	struct CKeyFrameSearch
	{
		CKeyFrame m_Frame;
		CKeyFrameSearch *m_pNext;
	};

	class IConsole *m_pConsole;
	IOHANDLE m_File;
	char m_aFilename[256];
	CKeyFrame *m_pKeyFrames;
	CMapInfo m_MapInfo;
	int m_SpeedIndex;

	CPlaybackInfo m_Info;
	int m_DemoType;
	unsigned char m_aLastSnapshotData[CSnapshot::MAX_SIZE];
	int m_LastSnapshotDataSize;
	class CSnapshotDelta *m_pSnapshotDelta;

	int ReadChunkHeader(int *pType, int *pSize, int *pTick);
	void DoTick();
	void ScanFile();
	int NextFrame();

public:

	CDemoPlayer(class CSnapshotDelta *m_pSnapshotDelta);

	void SetListener(IListener *pListener);

	int Load(class IStorageTW *pStorage, class IConsole *pConsole, const char *pFilename, int StorageType);
	int Play();
	void Pause();
	void Unpause();
	int Stop();
	void SetSpeed(float Speed);
	void SetSpeedIndex(int Offset);
	int SetPos(float Percent);
	const CInfo *BaseInfo() const { return &m_Info.m_Info; }
	void GetDemoName(char *pBuffer, int BufferSize) const;
	bool GetDemoInfo(class IStorageTW *pStorage, const char *pFilename, int StorageType, CDemoHeader *pDemoHeader) const;
	const char *GetDemoFileName() { return m_aFilename; };
	int GetDemoType() const;

	int Update(bool RealTime=true);

	const CPlaybackInfo *Info() const { return &m_Info; }
	virtual bool IsPlaying() const { return m_File != 0; }
	const CMapInfo *GetMapInfo() { return &m_MapInfo; };
};

class CDemoEditor : public IDemoEditor, public CDemoPlayer::IListener
{
	CDemoPlayer *m_pDemoPlayer;
	CDemoRecorder *m_pDemoRecorder;
	IConsole *m_pConsole;
	IStorageTW *m_pStorage;
	class CSnapshotDelta *m_pSnapshotDelta;
	const char *m_pNetVersion;

	bool m_Stop;
	int m_SliceFrom;
	int m_SliceTo;

public:
	virtual void Init(const char *pNetVersion, class CSnapshotDelta *pSnapshotDelta, class IConsole *pConsole, class IStorageTW *pStorage);
	virtual void Slice(const char *pDemo, const char *pDst, int StartTick, int EndTick, DEMOFUNC_FILTER pfnFilter, void *pUser);

	virtual void OnDemoPlayerSnapshot(void *pData, int Size);
	virtual void OnDemoPlayerMessage(void *pData, int Size);
};

#endif
