/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_SOUND_H
#define ENGINE_CLIENT_SOUND_H

#include <engine/sound.h>

class CSound : public IEngineSound
{
	MACRO_ALLOC_HEAP()
	int m_SoundEnabled;

public:
	IEngineGraphics *m_pGraphics;
	IStorageTW *m_pStorage;

	virtual int Init();

	int Update();
	int Shutdown();
	int AllocID();

	static void RateConvert(int SampleID);

	// TODO: Refactor: clean this mess up
	static IOHANDLE ms_File;
	static int ReadData(void *pBuffer, int Size);
	static int DecodeWV(int SampleID, const void *pData, unsigned DataSize);
	static int DecodeOpus(int SampleID, const void *pData, unsigned DataSize);

	virtual bool IsSoundEnabled() { return m_SoundEnabled != 0; }

	virtual int LoadWV(const char *pFilename);
	virtual int LoadWVFromMem(const void *pData, unsigned DataSize, bool FromEditor);
	virtual int LoadOpus(const char *pFilename);
	virtual int LoadOpusFromMem(const void *pData, unsigned DataSize, bool FromEditor);
	virtual void UnloadSample(int SampleID);

	// for lua
	virtual int LoadWVLua(const char *pFilename, lua_State *L);
	virtual int LoadOpusLua(const char *pFilename, lua_State *L);
	virtual void UnloadSampleLua(int SampleID, lua_State *L);

	virtual float GetSampleDuration(int SampleID); // in s

	virtual void SetListenerPos(float x, float y);
	virtual void SetChannel(int ChannelID, float Vol, float Pan);

	virtual void SetVoiceVolume(CVoiceHandle Voice, float Volume);
	virtual void SetVoiceFalloff(CVoiceHandle Voice, float Falloff);
	virtual void SetVoiceLocation(CVoiceHandle Voice, float x, float y);
	virtual void SetVoiceTimeOffset(CVoiceHandle Voice, float offset); // in s

	virtual void SetVoiceCircle(CVoiceHandle Voice, float Radius);
	virtual void SetVoiceRectangle(CVoiceHandle Voice, float Width, float Height);

	CVoiceHandle Play(int ChannelID, int SampleID, int Flags, float x, float y);
	virtual CVoiceHandle PlayAt(int ChannelID, int SampleID, int Flags, float x, float y);
	virtual CVoiceHandle Play(int ChannelID, int SampleID, int Flags);
	virtual void Stop(int SampleID);
	virtual void StopAll();
	virtual void StopVoice(CVoiceHandle Voice);
};

#endif
