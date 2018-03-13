/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SOUND_H
#define ENGINE_SOUND_H

#include <lua.hpp>
#include "kernel.h"

class ISound : public IInterface
{
	MACRO_INTERFACE("sound", 0)
public:
	enum
	{
		FLAG_LOOP=1<<0,
		FLAG_POS=1<<1,
		FLAG_NO_PANNING=1<<2,
		FLAG_ALL=FLAG_LOOP|FLAG_POS|FLAG_NO_PANNING,
	};

	enum
	{
		SHAPE_CIRCLE,
		SHAPE_RECTANGLE,
	};

	struct CVoiceShapeCircle
	{
		float m_Radius;
	};

	struct CVoiceShapeRectangle
	{
		float m_Width;
		float m_Height;
	};

	class CVoiceHandle
	{
		friend class ISound;
		int m_Id;
		int m_Age;
	public:
		CVoiceHandle()
		: m_Id(-1), m_Age(-1)
		{}

		bool IsValid() const { return (Id() >= 0) && (Age() >= 0); }
		int Id() const { return m_Id; }
		int Age() const { return m_Age; }

		bool operator ==(const CVoiceHandle &Other) const { return m_Id == Other.m_Id && m_Age == Other.m_Age; }
	};


	virtual bool IsSoundEnabled() = 0;

	virtual int LoadWV(const char *pFilename) = 0;
	virtual int LoadOpus(const char *pFilename) = 0;
	virtual int LoadWVFromMem(const void *pData, unsigned DataSize, bool FromEditor = false) = 0;
	virtual int LoadOpusFromMem(const void *pData, unsigned DataSize, bool FromEditor = false) = 0;
	virtual void UnloadSample(int SampleID) = 0;

	// for lua
	virtual int LoadWVLua(const char *pFilename, lua_State *L) = 0;
	virtual int LoadOpusLua(const char *pFilename, lua_State *L) = 0;
	virtual void UnloadSampleLua(int SampleID, lua_State *L) = 0;

	virtual float GetSampleDuration(int SampleID) = 0; // in s

	virtual void SetChannel(int ChannelID, float Volume, float Panning) = 0;
	virtual void SetListenerPos(float x, float y) = 0;

	virtual void SetVoiceVolume(CVoiceHandle Voice, float Volume) = 0;
	virtual void SetVoiceFalloff(CVoiceHandle Voice, float Falloff) = 0;
	virtual void SetVoiceLocation(CVoiceHandle Voice, float x, float y) = 0;
	virtual void SetVoiceTimeOffset(CVoiceHandle Voice, float offset) = 0; // in s

	virtual void SetVoiceCircle(CVoiceHandle Voice, float Radius) = 0;
	virtual void SetVoiceRectangle(CVoiceHandle Voice, float Width, float Height) = 0;

	virtual CVoiceHandle PlayAt(int ChannelID, int SampleID, int Flags, float x, float y) = 0;
	virtual CVoiceHandle Play(int ChannelID, int SampleID, int Flags) = 0;
	virtual void Stop(int SampleID) = 0;
	virtual void StopAll() = 0;
	virtual void StopVoice(CVoiceHandle Voice) = 0;

protected:
	inline CVoiceHandle CreateVoiceHandle(int Index, int Age)
	{
		CVoiceHandle Voice;
		Voice.m_Id = Index;
		Voice.m_Age = Age;
		return Voice;
	}
};


class IEngineSound : public ISound
{
	MACRO_INTERFACE("enginesound", 0)
public:
	virtual int Init() = 0;
	virtual int Update() = 0;
	virtual int Shutdown() = 0;
};

extern IEngineSound *CreateEngineSound();

#endif
