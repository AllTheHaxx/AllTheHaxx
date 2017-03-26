/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_CONSOLE_H
#define GAME_CLIENT_COMPONENTS_CONSOLE_H
#include <engine/shared/ringbuffer.h>
#include <game/client/component.h>
#include <game/client/lineinput.h>

enum
{
	CONSOLE_CLOSED,
	CONSOLE_OPENING,
	CONSOLE_OPEN,
	CONSOLE_CLOSING,
};

class CGameConsole : public CComponent
{
	class CInstance
	{
	public:
		struct CBacklogEntry
		{
			float m_YOffset;
			bool m_Highlighted;
			char m_aText[1];
		};
		TStaticRingBuffer<CBacklogEntry, 64*1024, CRingBufferBase::FLAG_RECYCLE> m_Backlog;
		TStaticRingBuffer<char, 64*1024, CRingBufferBase::FLAG_RECYCLE> m_History;
		char *m_pHistoryEntry;

		CLineInput m_Input;
		int m_Type;
		int m_CompletionEnumerationCount;
		int m_BacklogActLine;
		int m_BacklogActLineOld;
		int m_SearchFound;
		bool m_NoFound;
		int m_AtEnd;

	public:
		CGameConsole *m_pGameConsole;

		char m_aCompletionBuffer[128];
		int m_CompletionChosen;
		int m_CompletionFlagmask;
		float m_CompletionRenderOffset;
		bool m_ReverseTAB;
		bool m_CTRLPressed;


		bool m_IsCommand;
		char m_aCommandName[IConsole::TEMPCMD_NAME_LENGTH];
		char m_aCommandHelp[IConsole::TEMPCMD_HELP_LENGTH];
		char m_aCommandParams[IConsole::TEMPCMD_PARAMS_LENGTH];

		CInstance(int t);
		void Init(CGameConsole *pGameConsole);
		void InitLua();

		void ClearBacklog();
		void ClearHistory();

		void ExecuteLine(const char *pLine);

		void OnInput(IInput::CEvent Event);
		void PrintLine(const char *pLine, bool Highlighted = false);

		const char *GetString() const { return m_Input.GetString(); }
		static void PossibleCommandsCompleteCallback(const char *pStr, void *pUser);

		struct
		{
			lua_State *m_pLuaState;
			lua_State *m_pDebugChild;
			bool m_Inited;

			int m_ScopeCount;
			std::string m_FullLine;

		} m_LuaHandler;

		bool LoadLuaFile(const char *pFile);

		bool UserAuthAvailable() const;
		bool UserGot() const { return m_aUser[0] != '\0'; }
//		void SetUser(const char *pName) const
//		{
//			if(!pName || pName[0] == '\0')
//				m_aUser[0] = 1;
//			else
//				str_copyb(m_aUser, pName);
//		}
		const char *GetUser() const
		{
			if(m_aUser[0] == 1)
				return "";
			else
				return m_aUser;
		}
		bool UsingUserAuth() const { return m_UseUser; }
		void ResetRconLogin();

	private:
		char m_aUser[64];
		bool m_UseUser;
	};

	class IConsole *m_pConsole;

	CInstance m_LocalConsole;
	CInstance m_RemoteConsole;
	CInstance m_LuaConsole;

	CInstance *CurrentConsole();
	float TimeNow();
	int m_PrintCBIndex;

	static const char *m_pSearchString;

	int m_ConsoleType;
	int m_ConsoleState;
	float m_StateChangeEnd;
	float m_StateChangeDuration;

	void Toggle(int Type);
	void Dump(int Type);

	static void PossibleCommandsRenderCallback(const char *pStr, void *pUser);
	static void ClientConsolePrintCallback(const char *pStr, void *pUserData, bool Highlighted);
	static void ConToggleLocalConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConToggleRemoteConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConToggleLuaConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConClearLocalConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConClearRemoteConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConDumpLocalConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConDumpRemoteConsole(IConsole::IResult *pResult, void *pUserData);
	static void ConchainConsoleOutputLevelUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void Con_Lua(IConsole::IResult *pResult, void *pUserData);

public:
	enum
	{
		CONSOLETYPE_LOCAL=0,
		CONSOLETYPE_REMOTE,
		CONSOLETYPE_LUA,
	};

	CGameConsole();

	void PrintLine(int Type, const char *pLine);

	static int PrintLuaLine(lua_State *L);
	void AttachLuaDebugger(const class CLuaFile *pLF);
	lua_State *GetDebuggerChild() const { return m_LuaConsole.m_LuaHandler.m_pDebugChild; }
	static CInstance * m_pStatLuaConsole;

	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnConsoleInit();
	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnInput(IInput::CEvent Events);

	bool IsClosed() { return m_ConsoleState == CONSOLE_CLOSED; }
};
#endif
