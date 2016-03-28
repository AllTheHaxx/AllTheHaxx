/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_CHAT_H
#define GAME_CLIENT_COMPONENTS_CHAT_H
#include <engine/shared/ringbuffer.h>
#include <game/client/component.h>
#include <game/client/lineinput.h>

// much crypto
#include <engine/external/openssl/pem.h>
#include <engine/external/openssl/ssl.h>
#include <engine/external/openssl/rsa.h>
#include <engine/external/openssl/evp.h>
#include <engine/external/openssl/bio.h>
#include <engine/external/openssl/err.h>
// -----------

#include "translator.h"

class CChat : public CComponent
{
	CLineInput m_Input;

	enum
	{
		MAX_LINES = 25,
	};

	struct CLine
	{
		int64 m_Time;
		float m_YOffset[2];
		int m_ClientID;
		int m_Team;
		int m_NameColor;
		char m_aName[64];
		char m_aText[512];
		bool m_Highlighted;
		bool m_Hidden;
	};

	CLine m_aLines[MAX_LINES];
	int m_CurrentLine;
	int64 m_LastLine;

	// chat
	enum
	{
		MODE_NONE=0,
		MODE_ALL,
		MODE_TEAM,
		MODE_HIDDEN,
		MODE_CRYPT,

		CHAT_SERVER=0,
		CHAT_HIGHLIGHT,
		CHAT_CLIENT,
		CHAT_NUM,
	};

	int m_Mode;
	bool m_Show;
	bool m_InputUpdate;
	int m_ChatStringOffset;
	int m_OldChatStringLength;
	int m_CompletionChosen;
	char m_aCompletionBuffer[256];
	int m_PlaceholderOffset;
	int m_PlaceholderLength;

	bool m_ReverseTAB;

	struct CHistoryEntry
	{
		int m_Team;
		char m_aText[1];
	};
	CHistoryEntry *m_pHistoryEntry;
	TStaticRingBuffer<CHistoryEntry, 64*1024, CRingBufferBase::FLAG_RECYCLE> m_History;
	int m_PendingChatCounter;
	int64 m_LastChatSend;
	int64 m_aLastSoundPlayed[CHAT_NUM];

	int64 m_LastDennisTrigger;

	static void ConSay(IConsole::IResult *pResult, void *pUserData);
	static void ConSayTeam(IConsole::IResult *pResult, void *pUserData);
	static void ConChat(IConsole::IResult *pResult, void *pUserData);
	static void ConShowChat(IConsole::IResult *pResult, void *pUserData);

	static void ConGenKeys(IConsole::IResult *pResult, void *pUserData);
	static void ConSaveKeys(IConsole::IResult *pResult, void *pUserData);
	static void ConLoadKeys(IConsole::IResult *pResult, void *pUserData);

	bool LineShouldHighlight(const char *pLine, const char *pName);

	CTranslator *m_pTranslator;
	bool HandleTCommands(const char *pMsg);

public:
	CChat();

	bool IsActive() const { return m_Mode != MODE_NONE; }
	int GetMode() const { return m_Mode; }
	bool IsShown() const { return m_Show; }
	float Blend() const { return time_get() > m_aLines[m_CurrentLine].m_Time+14*time_freq() && !m_Show ? 1.0f-(time_get()-m_aLines[m_CurrentLine].m_Time-14*time_freq())/(2.0f*time_freq()) : 1.0f; }

	void AddLine(int ClientID, int Team, const char *pLine, bool Hidden = false);

	void EnableMode(int Team);

	void Say(int Team, const char *pLine, bool NoTrans = false);

	virtual void OnReset();
	virtual void OnConsoleInit();
	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnRender();
	virtual void OnRelease();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnInput(IInput::CEvent Event);

	// crypt stuff
	RSA *m_pKeyPair;
	bool m_GotKeys;
	void GenerateKeyPair(int Bytes, int Exp);
	char *ReadPubKey(RSA *pKeyPair);
	char *ReadPrivKey(RSA *pKeyPair);
	char *EncryptMsg(const char *pMsg);
	char *DecryptMsg(const char *pMsg);
	void SaveKeys(RSA *pKeyPair, const char *pKeyName);
	void LoadKeys(const char *pKeyName);
	
	std::string m_CryptSendQueue;
};
#endif
