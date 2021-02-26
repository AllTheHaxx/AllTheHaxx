/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.				*/

#include <base/tl/string.h>

#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/keys.h>
#include <engine/shared/config.h>

#define ECB 0
#include <engine/external/aes128/aes.h>
#undef ECB

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/animstate.h>
#include <game/client/gameclient.h>

#include <game/client/components/scoreboard.h>
#include <game/client/components/sounds.h>
#include <game/localization.h>

#ifdef CONF_PLATFORM_MACOSX
#include <osx/notification.h>
#endif

#include "hud.h"
#include "chat.h"
#include "console.h"
#include "menus.h"
#include "translator.h"


CChat::CChat()
{
	m_pTranslator = 0;

	m_GotKey = false;
	mem_zero(&m_CryptKey, sizeof(m_CryptKey));
	mem_zero(&m_CryptIV, sizeof(m_CryptIV));

	OnReset();
}


void CChat::OnInit()
{
	m_pTranslator = new CTranslator();
	if(!m_pTranslator->Init())
	{
		delete m_pTranslator;
		m_pTranslator = 0;
		dbg_msg("chat", "failed to init translator");
	}
}

void CChat::OnShutdown()
{
	if(m_pTranslator)
	{
//		m_pTranslator->Shutdown();
		delete m_pTranslator;
	}
}

void CChat::OnReset()
{
	for(int i = 0; i < MAX_LINES; i++)
	{
		m_aLines[i].m_Time = 0;
		m_aLines[i].m_aText[0] = 0;
		m_aLines[i].m_aName[0] = 0;
	}

	m_ReverseTAB = false;
	m_Mode = MODE_NONE;
	m_Show = false;
	m_InputUpdate = false;
	m_ChatStringOffset = 0;
	m_CompletionChosen = -1;
	m_aCompletionBuffer[0] = 0;
	m_PlaceholderOffset = 0;
	m_PlaceholderLength = 0;
	m_pHistoryEntry = 0x0;
	m_PendingChatCounter = 0;
	m_LastChatSend = 0;

	for(int i = 0; i < CHAT_NUM; ++i)
		m_aLastSoundPlayed[i] = 0;
}

void CChat::OnRelease()
{
	CALLSTACK_ADD();

	m_Show = false;
}

void CChat::OnStateChange(int NewState, int OldState)
{
	CALLSTACK_ADD();

	if(OldState <= IClient::STATE_CONNECTING)
	{
		m_Mode = MODE_NONE;
		Input()->SetIMEState(false);
		for(int i = 0; i < MAX_LINES; i++)
		{
			m_aLines[i].m_Time = 0;
		}
		m_CurrentLine = 0;
	}
}

void CChat::ConSay(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	((CChat*)pUserData)->Say(0, pResult->GetString(0));
}

void CChat::ConSayTeam(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	((CChat*)pUserData)->Say(1, pResult->GetString(0));
}

void CChat::ConChat(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	SELF_FROM_USERDATA(CChat);

	const char *pMode = pResult->GetString(0);
	if(str_comp(pMode, "all") == 0)
		pSelf->EnableMode(0);
	else if(str_comp(pMode, "team") == 0)
		pSelf->EnableMode(1);
	else if(str_comp(pMode, "hidden") == 0)
		pSelf->EnableMode(2);
	else if(str_comp(pMode, "crypt") == 0)
		pSelf->EnableMode(3);
	else
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "console", "expected all, team, hidden or crypt as mode");

	if(pResult->GetString(1)[0] || g_Config.m_ClChatReset)
		pSelf->m_Input.Set(pResult->GetString(1));
}

void CChat::ConShowChat(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	((CChat *)pUserData)->m_Show = pResult->GetInteger(0) != 0;
}

void CChat::ConGenKey(IConsole::IResult *pResult, void *pUserData)
{
	CChat *pSelf = (CChat *)pUserData;

	char aPassword[64];
	secure_random_password(aPassword, sizeof(aPassword), 62);

	// store the password as md5 (it fortunately has the same size as our AES key)
	MD5_HASH PwHash = md5_simple((unsigned char*)aPassword, (unsigned int)str_length(aPassword));
	mem_copy(pSelf->m_CryptKey.key, PwHash.digest, sizeof(pSelf->m_CryptKey.key));

	if(g_Config.m_Debug)
	{
		char aBuf[128];
		str_hex_simplebb(aBuf, pSelf->m_CryptKey.key);
		pSelf->Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "crypto", "%s", aBuf);
	}

	pSelf->m_GotKey = true;
}

void CChat::ConSetKey(IConsole::IResult *pResult, void *pUserData)
{
	CChat *pSelf = (CChat *)pUserData;

	const char *pPassword = NULL;
	if(pResult->NumArguments() > 0)
		pPassword = pResult->GetString(0);

	pSelf->SetKey(pPassword);
}

void CChat::SetKey(const char *pPassword)
{
	if(!pPassword || pPassword[0] == '\0')
	{
		mem_zerob(m_CryptKey.key);
		mem_zerob(m_CryptIV.iv);
		m_GotKey = false;

		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "crypt", "Key cleared!");
		m_pClient->m_pHud->PushNotification("Crypt key cleared!");
		return;
	}

	// store the password as md5 (it fortunately has the same size as our AES key)
	MD5_HASH PwHash = md5_simple((unsigned char*)pPassword, (unsigned int)str_length(pPassword));
	mem_copy(m_CryptKey.key, PwHash.digest, sizeof(m_CryptKey.key));

	if(g_Config.m_Debug)
	{
		char aBuf[128];
		str_hex_simplebb(aBuf, m_CryptKey.key);
		Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "crypto", "%s", aBuf);
	}

	// perform a test
/*	for(int i = 0; i < 3; i++)
	{
		const char *pTestString;

		switch(i)
		{
			case 0:
				pTestString = "Who would even want to eat dead beef?!";
				break;
			case 1:
				pTestString = "nix";
				break;
			default:
				pTestString = "göhen 4u(|-| \\/ö|_LSö |)1€ 50’Ð€®¥€ı©Ħ€’";
		}

		unsigned int DataSize;
		uint8_t *pData = str_aes128_encrypt(pTestString, &m_CryptKey, &DataSize, &m_CryptIV);

		// decryption
		char aDecrypted[128];
		str_aes128_decrypt(pData, DataSize, &m_CryptKey, aDecrypted, sizeof(aDecrypted), &m_CryptIV);

		char aBuf[256];

		str_hex_simplebb(aBuf, m_CryptKey.key);
		dbg_msg("crypt", "testing key '%s'; PayloadLen = %i, OutputLen = %i, DataSize = %u", aBuf, str_length(pTestString), str_length(aDecrypted), DataSize);
		str_hex_simpleb(aBuf, pData, DataSize);
		dbg_msg("crypt", "> encrypted: '%s'", aBuf);
		str_hex_simpleb(aBuf, (const unsigned char *)aDecrypted, sizeof(aDecrypted));
		dbg_msg("crypt", "> decrypted: '%s'", aBuf);
		dbg_msg("crypt", "> decoded..: '%s'", aDecrypted);
		str_hex_simplebb(aBuf, m_CryptIV.iv);
		dbg_msg("crypt", "> InitalVec: '%s'", aBuf);

		mem_free(pData);
	}
 */

	m_GotKey = true;
}


void CChat::OnConsoleInit()
{
	CALLSTACK_ADD();

	Console()->Register("say", "r[message]", CFGFLAG_CLIENT, ConSay, this, "Say in chat");
	Console()->Register("say_team", "r[message]", CFGFLAG_CLIENT, ConSayTeam, this, "Say in team chat");
	Console()->Register("chat", "s['all'|'team'|'hidden'|'crypt'] ?r[message]", CFGFLAG_CLIENT, ConChat, this, "Enable chat with all/team mode");
	Console()->Register("+show_chat", "", CFGFLAG_CLIENT, ConShowChat, this, "Show chat");

#if defined(CONF_DEBUG)
	Console()->Register("chat_crypt_gen_key", "", CFGFLAG_CLIENT, ConGenKey, this, "Generate a random key"); // there is actually no use from this...
#endif
	Console()->Register("chat_crypt_set_key", "?r", CFGFLAG_CLIENT, ConSetKey, this, "Set or clear the key for encrypted chat messages");
}

bool CChat::OnInput(IInput::CEvent Event)
{
	CALLSTACK_ADD();

	if(m_Mode == MODE_NONE)
		return false;

	if(m_pClient->m_pMenus->IsActive() && Event.m_Key != KEY_ESCAPE)
		return false;

	bool Handled = false;
	if(Event.m_Flags&IInput::FLAG_PRESS)
	{
		if(Input()->KeyIsPressed(KEY_LCTRL) || Input()->KeyIsPressed(KEY_RCTRL))
		{
			Handled = true;
			if(Event.m_Key == KEY_V) // paste
			{
				const char *pText = Input()->GetClipboardText();
				if(pText)
				{
					// if the text has more than one line, we send all lines except the last one
					// the last one is set as in the text field
					char aLine[256];
					int i, Begin = 0;
					for(i = 0; i < str_length(pText); i++)
					{
						if(pText[i] == '\n')
						{
							int max = min(i - Begin + 1, (int)sizeof(aLine));
							str_copy(aLine, pText + Begin, max);
							Begin = i+1;
							SayChat(aLine);
							while(pText[i] == '\n') i++;
						}
					}
					pText += Begin;

					char aRightPart[256];
					str_copy(aRightPart, m_Input.GetString() + m_Input.GetCursorOffset(), sizeof(aRightPart));
					str_copy(aLine, m_Input.GetString(), min(m_Input.GetCursorOffset()+1, (int)sizeof(aLine)));
					str_append(aLine, pText, sizeof(aLine));
					str_append(aLine, aRightPart, sizeof(aLine));
					m_Input.Set(aLine);
					m_Input.SetCursorOffset(str_length(aLine)-str_length(aRightPart));
				}
				else if(g_Config.m_Debug)
					dbg_msg("chat", "paste failed: got no text from clipboard");
			}
			else if(Event.m_Key == KEY_C || Event.m_Key == KEY_X) // copy/cut
			{
				Input()->SetClipboardText(m_Input.GetString());
				if(Event.m_Key == KEY_X)
					m_Input.Clear();
			}
			else // handle skipping: jump to spaces and special ASCII characters
			{
				CLineInput::HandleSkipping(Event, &m_Input);
			}
		}
		else if(Event.m_Key == KEY_ESCAPE)
		{
			m_Mode = MODE_NONE;
			if(!m_pClient->m_pMenus->IsActive())
				m_pClient->m_pMenus->UseMouseButtons(false);
			m_pClient->OnRelease();
			if(g_Config.m_ClChatReset)
				m_Input.Clear();

			// abort text editing when pressing escape
			Input()->SetIMEState(false);
		}
		else if(Event.m_Key == KEY_RETURN || Event.m_Key == KEY_KP_ENTER)
		{
			if(m_Input.GetString()[0])
			{
				bool AddEntry = false;

				if(m_LastChatSend+time_freq() < time_get())
				{
					if(m_Mode == MODE_HIDDEN)
					{
						if(g_Config.m_ClFlagChat)
							m_CryptSendQueue = std::string(m_Input.GetString());
						else
						{
							const char *pMsg = Localize("You need to enable 'Hidden Chat' before you can send hidden messages!");
							if(g_Config.m_ClNotifications)
								m_pClient->m_pHud->PushNotification(pMsg);
							else
								AddLine(m_pClient->m_Snap.m_LocalClientID, 0, pMsg, true);
						}
					}
					else if(m_Mode == MODE_CRYPT)
					{
						char aBuf[512];
						char *pEncrypted = EncryptMsg(aBuf, sizeof(aBuf), m_Input.GetString());
						if(pEncrypted)
						{
							Say(0, pEncrypted, true);
						}

					}
					else
						Say(m_Mode == MODE_ALL ? 0 : 1, m_Input.GetString());
					AddEntry = true;
				}
				else if(m_PendingChatCounter < 3)
				{
					++m_PendingChatCounter;
					AddEntry = true;
				}

				// don't add a line multiple times in a row
				CHistoryEntry *pTest = m_History.Last();
				if(pTest)
				{
					if(str_comp(pTest->m_aText, m_Input.GetString()) == 0)
						AddEntry = false;
				}

				if(AddEntry)
				{
					CHistoryEntry *pEntry = m_History.Allocate(sizeof(CHistoryEntry)+m_Input.GetLength());
					pEntry->m_Team = m_Mode == MODE_TEAM ? 1 : 0;
					mem_copy(pEntry->m_aText, m_Input.GetString(), m_Input.GetLength()+1);
				}
			}
			m_pHistoryEntry = 0x0;
			m_Mode = MODE_NONE;
			if(!m_pClient->m_pMenus->IsActive())
				m_pClient->m_pMenus->UseMouseButtons(false);
			m_pClient->OnRelease();
			m_Input.Clear();

			// stop text editing after send chat.
			Input()->SetIMEState(false);
		}
		else if(Event.m_Key == KEY_TAB)
		{
			// fill the completion buffer
			if(m_CompletionChosen < 0)
			{
				const char *pCursor = m_Input.GetString()+m_Input.GetCursorOffset();
				for(int Count = 0; Count < m_Input.GetCursorOffset() && *(pCursor-1) != ' '; --pCursor, ++Count);
				m_PlaceholderOffset = pCursor-m_Input.GetString();

				for(m_PlaceholderLength = 0; *pCursor && *pCursor != ' '; ++pCursor)
					++m_PlaceholderLength;

				str_copy(m_aCompletionBuffer, m_Input.GetString()+m_PlaceholderOffset, min(static_cast<int>(sizeof(m_aCompletionBuffer)), m_PlaceholderLength+1));
			}

			// find next possible name
			const char *pCompletionString = 0;

			if(m_ReverseTAB)
				m_CompletionChosen = (m_CompletionChosen-1 + 2*MAX_CLIENTS)%(2*MAX_CLIENTS);
			else
				m_CompletionChosen = (m_CompletionChosen+1)%(2*MAX_CLIENTS);

			for(int i = 0; i < 2*MAX_CLIENTS; ++i)
			{
				int SearchType;
				int Index;

				if(m_ReverseTAB)
				{
					SearchType = ((m_CompletionChosen-i +2*MAX_CLIENTS)%(2*MAX_CLIENTS))/MAX_CLIENTS;
					Index = (m_CompletionChosen-i + MAX_CLIENTS )%MAX_CLIENTS;
				}
				else
				{
					SearchType = ((m_CompletionChosen+i)%(2*MAX_CLIENTS))/MAX_CLIENTS;
					Index = (m_CompletionChosen+i)%MAX_CLIENTS;
				}


				if(!m_pClient->m_Snap.m_paInfoByName[Index])
					continue;

				int Index2 = m_pClient->m_Snap.m_paInfoByName[Index]->m_ClientID;

				bool Found = false;
				if(SearchType == 1)
				{
					if(str_comp_nocase_num(m_pClient->m_aClients[Index2].m_aName, m_aCompletionBuffer, str_length(m_aCompletionBuffer)) &&
					   str_find_nocase(m_pClient->m_aClients[Index2].m_aName, m_aCompletionBuffer))
						Found = true;
				}
				else if(!str_comp_nocase_num(m_pClient->m_aClients[Index2].m_aName, m_aCompletionBuffer, str_length(m_aCompletionBuffer)))
					Found = true;

				if(Found)
				{
					pCompletionString = m_pClient->m_aClients[Index2].m_aName;
					m_CompletionChosen = Index+SearchType*MAX_CLIENTS;
					break;
				}
			}

			// insert the name
			if(pCompletionString)
			{
				char aBuf[256];
				// add part before the name
				str_copy(aBuf, m_Input.GetString(), min(static_cast<int>(sizeof(aBuf)), m_PlaceholderOffset+1));

				// add the name
				str_append(aBuf, pCompletionString, sizeof(aBuf));

				// add seperator
				const char *pSeparator = "";
				if(*(m_Input.GetString()+m_PlaceholderOffset+m_PlaceholderLength) != ' ')
					pSeparator = m_PlaceholderOffset == 0 ? ": " : " ";
				else if(m_PlaceholderOffset == 0)
					pSeparator = ":";
				if(*pSeparator)
					str_append(aBuf, pSeparator, sizeof(aBuf));

				// add part after the name
				str_append(aBuf, m_Input.GetString()+m_PlaceholderOffset+m_PlaceholderLength, sizeof(aBuf));

				m_PlaceholderLength = str_length(pSeparator)+str_length(pCompletionString);
				m_OldChatStringLength = m_Input.GetLength();
				m_Input.Set(aBuf); // TODO: Use Add instead
				m_Input.SetCursorOffset(m_PlaceholderOffset+m_PlaceholderLength);
				m_InputUpdate = true;
			}
		}
		else if(Event.m_Key == KEY_UP)
		{
			if(m_pHistoryEntry)
			{
				CHistoryEntry *pTest = m_History.Prev(m_pHistoryEntry);

				if(pTest)
					m_pHistoryEntry = pTest;
			}
			else
				m_pHistoryEntry = m_History.Last();

			if(m_pHistoryEntry)
				m_Input.Set(m_pHistoryEntry->m_aText);
		}
		else if (Event.m_Key == KEY_DOWN)
		{
			if(m_pHistoryEntry)
				m_pHistoryEntry = m_History.Next(m_pHistoryEntry);

			if (m_pHistoryEntry)
				m_Input.Set(m_pHistoryEntry->m_aText);
			else
				m_Input.Clear();
		}
		else if(Event.m_Key == KEY_LSHIFT)
		{
			m_ReverseTAB = true;
		}
	}
	else if(Event.m_Flags&IInput::FLAG_RELEASE && Event.m_Key == KEY_LSHIFT)
	{
		m_ReverseTAB = false;
	}


	m_OldChatStringLength = m_Input.GetLength();
	if(!Handled)
	{
		// reset name completion process
		if(Event.m_Key != KEY_TAB && Event.m_Key != KEY_LSHIFT && Event.m_Key != KEY_LCTRL && Event.m_Key != KEY_RCTRL)
			m_CompletionChosen = -1;
		m_Input.ProcessInput(Event);
	}
	m_InputUpdate = true;

	return true;
}


void CChat::EnableMode(int Team)
{
	CALLSTACK_ADD();

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
		return;

	if(m_Mode == MODE_NONE)
	{
		if(Team == 3)
			m_Mode = MODE_CRYPT;
		else if(Team == 2)
			m_Mode = MODE_HIDDEN;
		else if(Team == 1)
			m_Mode = MODE_TEAM;
		else
			m_Mode = MODE_ALL;

		Input()->SetIMEState(true);
		Input()->Clear();
		m_pClient->m_pMenus->UseMouseButtons(true);
		m_CompletionChosen = -1;
		UI()->AndroidShowTextInput("", Team ? Localize("Team chat") : Localize("Chat"));
	}
}

void CChat::OnMessage(int MsgType, void *pRawMsg)
{
	CALLSTACK_ADD();

	static const char *apNotificationMsgs[] = {
			"You are now in a solo part.",
			"You are now out of the solo part.",
			"You are now in a solo part",
			"You are now out of the solo part",
			"Rescue is not enabled on this server",
			"You aren't freezed!",
			"You are not freezed!",
			"Please join a team before you start",
			"Server admin requires you to be in a team and with other tees to start",
			"You have a jetpack gun",
			"You lost your jetpack gun",
			"You can't hook others",
			"You can hook others",
			"You can jump",
			"You have unlimited air jumps",
			"You don't have unlimited air jumps",
			"You can collide with others",
			"You can't collide with others",
			"Endless hook has been activated",
			"Endless hook has been deactivated",
			"You can hit others",
			"You can't hit others",
			"You can hammer hit others",
			"You can't hammer hit others",
			"You can shoot others with shotgun",
			"You can't shoot others with shotgun",
			"You can shoot others with grenade",
			"You can't shoot others with grenade",
			"You can shoot others with rifle",
			"You can't shoot others with rifle",
			"Timer isn't displayed.",
			"Timer is displayed in game/round timer.",
			"Timer is displayed in broadcast.",
			"Timer is displayed in both game/round timer and broadcast.",
			"You will now see all tees on this server, no matter the distance",
			"You will no longer see all tees on this server",
	};

	if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		bool HideChat = false;

		if(g_Config.m_ClNotifications)
		{
			for(int i = 0; i < int(sizeof(apNotificationMsgs)/sizeof(apNotificationMsgs[0])); i++)
			{
				if(str_comp_nocase(pMsg->m_pMessage, apNotificationMsgs[i]) == 0)
				{
					m_pClient->m_pHud->PushNotification(pMsg->m_pMessage);
					HideChat = true;
				}
			}
		}

		// return down here in order to always receive the race notifications
		if(g_Config.m_ClChat == 0 || (g_Config.m_ClChat == 1 && pMsg->m_ClientID >= 0))
			return;

		// EVENT CALL
		LUA_FIRE_EVENT_RES({
			if(_LuaEventResult.isBool())
				HideChat |= _LuaEventResult.cast<bool>();
						   }, "OnChat", pMsg->m_ClientID, pMsg->m_Team, std::string(pMsg->m_pMessage))

		// some dennis (you can never have enough)
		if(pMsg->m_ClientID == -1)
			if(g_Config.m_ClChatDennisProtection && m_LastDennisTrigger + time_freq() * 3 < time_get() &&
					str_find_nocase(pMsg->m_pMessage, "entered and joined the") && (
			   		str_find_nocase(pMsg->m_pMessage, "Dennis") ||
					str_find_nocase(pMsg->m_pMessage, "deen") ||
					str_find_nocase(pMsg->m_pMessage, "Dune"))
				)
			{
				Say(0, "DENNIS!");
				m_LastDennisTrigger = time_get();
			}

		NETADDR Addr;
		if(net_addr_from_str(&Addr, pMsg->m_pMessage) == 0)
		{
			// such dennis
			if(g_Config.m_ClChatDennisProtection && m_LastDennisTrigger + time_freq() * 10 < time_get() &&
				Addr.port != 1337)
			{
				Say(0, "DENNIS!");
				m_LastDennisTrigger = time_get();
			}

			m_pClient->m_aClients[pMsg->m_ClientID].m_Spoofable = true; // presume the player is spoofable as he gave us his IP

			// thx 4 u ip nab ICKSDEHHHH
			str_copy(m_pClient->m_aClients[pMsg->m_ClientID].m_Addr, pMsg->m_pMessage, sizeof(m_pClient->m_aClients[pMsg->m_ClientID].m_Addr));
			if(!g_Config.m_ClChatShowIPs)
				HideChat = true;
		}

		// try to decrypt everything we can
		char *pDecrypted = 0;
		if(pMsg->m_ClientID != -1)
		{
			char aBuf[512];
			pDecrypted = DecryptMsg(aBuf, sizeof(aBuf), pMsg->m_pMessage);
			if(pDecrypted) // determine whether to hide the raw hexadecimal data
			{
				if((pMsg->m_ClientID != m_pClient->m_Snap.m_LocalClientID && !(g_Config.m_ClShowChatCryptData & 1)) // show theirs?
				   || (pMsg->m_ClientID == m_pClient->m_Snap.m_LocalClientID && !(g_Config.m_ClShowChatCryptData & 2))) // show our?
					HideChat = true;
			}
		}

		// add the original line first
		if(!HideChat)
			AddLine(pMsg->m_ClientID, pMsg->m_Team, pMsg->m_pMessage);

		// add the decrypted message if we got one
		if(pDecrypted)
			AddLine(pMsg->m_ClientID, 0, pDecrypted, true);

		// request translations, even for decrypted chat!
		if(TranslatorAvailable() && g_Config.m_ClTransIn &&
				str_length(pMsg->m_pMessage) > 1 &&
				pMsg->m_ClientID != m_pClient->m_Snap.m_LocalClientID &&
				pMsg->m_ClientID != -1)
		{
			char aMentionedName[MAX_NAME_LENGTH];
			const char *pTransStart = PrepareMsgForTrans(pDecrypted ? pDecrypted : pMsg->m_pMessage, aMentionedName);
			if(str_length(pTransStart) > 1)
				m_pTranslator->RequestTranslation(g_Config.m_ClTransInSrc, g_Config.m_ClTransInDst, pTransStart, true,
												  aMentionedName, pMsg->m_ClientID);
		}
	}
}

bool CChat::LineShouldHighlight(const char *pLine, const char *pName)
{
	CALLSTACK_ADD();

	const char *pHL = str_find_nocase(pLine, pName);

	if (pHL)
	{
		int Length = str_length(pName);

		if((pLine == pHL || pHL[-1] == ' ') &&
				(pHL[Length] == 0 || pHL[Length] == ' ' || pHL[Length] == '.' || pHL[Length] == '!' || pHL[Length] == ',' || pHL[Length] == '?' || pHL[Length] == ':'))
		{
			// DEBUGGING CHECK to catch invalid highlights
			if(str_comp_nocase(pName, g_Config.m_PlayerName) != 0 && str_comp_nocase(pName, g_Config.m_ClDummyName) != 0)
			{
				// if the name is neither the player name nor the dummy name, something might be fishy (or the server force-changed our name)
				Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "POSSIBLE BUG",
								  "CChat::LineShouldHighlight | player_name='%s' dummy_name='%s' pName='%s' offset=%i in_len=%i", g_Config.m_PlayerName, g_Config.m_ClDummyName, pName, (int)(pHL - pLine), Length);
			}

			return true;
		}
	}

	return false;
}

void CChat::AddLine(int ClientID, int Team, const char *pLine, bool Hidden)
{
	CALLSTACK_ADD();

	if(!pLine || pLine[0] == '\0' || ClientID >= MAX_CLIENTS)
		return;

	if(ClientID >= 0)
	{
		if(m_pClient->m_aClients[ClientID].m_aName[0] == '\0' || m_pClient->m_aClients[ClientID].m_ChatIgnore ||
		   (m_pClient->m_Snap.m_LocalClientID != ClientID && g_Config.m_ClShowChatFriends && !m_pClient->m_aClients[ClientID].m_Friend) ||
		   (m_pClient->m_Snap.m_LocalClientID != ClientID && m_pClient->m_aClients[ClientID].m_Foe))
			return;
	}

	// trim right and set maximum length to 256 utf8-characters
	int Length = 0;
	const char *pStr = pLine;
	const char *pEnd = 0;
	while(*pStr)
	{
		const char *pStrOld = pStr;
		int Code = str_utf8_decode(&pStr);

		// check if unicode is not empty
		if(str_utf8_isspace(Code))
		{
			pEnd = 0;
		}
		else if(pEnd == 0)
			pEnd = pStrOld;

		if(++Length >= 256)
		{
			*(const_cast<char *>(pStr)) = 0;
			break;
		}
	}
	if(pEnd != 0)
		*(const_cast<char *>(pEnd)) = 0;

	bool Highlighted = false;
	char *p = const_cast<char*>(pLine);
	while(*p)
	{
		Highlighted = false;
		pLine = p;
		// find line seperator and strip multiline
		while(*p)
		{
			if(*p++ == '\n')
			{
				*(p-1) = 0;
				break;
			}
		}

		m_CurrentLine = (m_CurrentLine+1)%MAX_LINES;
		m_aLines[m_CurrentLine].m_Time = time_get();
		m_aLines[m_CurrentLine].m_YOffset[0] = -1.0f;
		m_aLines[m_CurrentLine].m_YOffset[1] = -1.0f;
		m_aLines[m_CurrentLine].m_ClientID = ClientID;
		m_aLines[m_CurrentLine].m_Team = Team;
		m_aLines[m_CurrentLine].m_NameColor = -2;

			// check for highlighted name
			if (Client()->State() != IClient::STATE_DEMOPLAYBACK)
			{
				if(ClientID != m_pClient->Client()->m_LocalIDs[0])
				{
					// main character
					if (LineShouldHighlight(pLine, m_pClient->m_aClients[m_pClient->Client()->m_LocalIDs[0]].m_aName))
						Highlighted = true;
					// dummy
					if(m_pClient->Client()->DummyConnected() && LineShouldHighlight(pLine, m_pClient->m_aClients[m_pClient->Client()->m_LocalIDs[1]].m_aName))
						Highlighted = true;
				}
			}
			else
			{
				// on demo playback use local id from snap directly,
				// since m_LocalIDs isn't valid there
				if (LineShouldHighlight(pLine, m_pClient->m_aClients[m_pClient->m_Snap.m_LocalClientID].m_aName))
					Highlighted = true;
			}

		m_aLines[m_CurrentLine].m_Hidden = false;
		if(!Hidden)
			m_aLines[m_CurrentLine].m_Highlighted = Highlighted;
		else
			m_aLines[m_CurrentLine].m_Hidden = true;

		if(ClientID == -1) // server message
		{
			str_copy(m_aLines[m_CurrentLine].m_aName, "*** ", sizeof(m_aLines[m_CurrentLine].m_aName));
			str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "%s", pLine);
		}
		else if(m_aLines[m_CurrentLine].m_Team == MODE_TRANS) // TODO: move xd
		{
			//mem_zero(m_aLines[m_CurrentLine].m_aName, sizeof(m_aLines[m_CurrentLine].m_aName));
			str_copy(m_aLines[m_CurrentLine].m_aName, m_pClient->m_aClients[ClientID].m_aName, sizeof(m_aLines[m_CurrentLine].m_aName));
			m_aLines[m_CurrentLine].m_NameColor = MODE_TRANS;
			str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), ": %s", pLine);
		}
		else if(ClientID <= FAKE_ID_LUA)
		{
			str_copy(m_aLines[m_CurrentLine].m_aName, "[*Lua*]: ", sizeof(m_aLines[m_CurrentLine].m_aName));
			str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), "%s", pLine);
		}
		else
		{
			if(m_pClient->m_aClients[ClientID].m_Team == TEAM_SPECTATORS)
				m_aLines[m_CurrentLine].m_NameColor = TEAM_SPECTATORS;

			if(m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
			{
				if(m_pClient->m_aClients[ClientID].m_Team == TEAM_RED)
					m_aLines[m_CurrentLine].m_NameColor = TEAM_RED;
				else if(m_pClient->m_aClients[ClientID].m_Team == TEAM_BLUE)
					m_aLines[m_CurrentLine].m_NameColor = TEAM_BLUE;
			}

			if (Team == 2) // whisper send
			{
				str_format(m_aLines[m_CurrentLine].m_aName, sizeof(m_aLines[m_CurrentLine].m_aName), "→ %s", m_pClient->m_aClients[ClientID].m_aName);
				m_aLines[m_CurrentLine].m_NameColor = TEAM_BLUE;
				m_aLines[m_CurrentLine].m_Highlighted = false;
				m_aLines[m_CurrentLine].m_Team = 0;
				Highlighted = false;
			}
			else if (Team == 3) // whisper recv
			{
				str_format(m_aLines[m_CurrentLine].m_aName, sizeof(m_aLines[m_CurrentLine].m_aName), "← %s", m_pClient->m_aClients[ClientID].m_aName);
				m_aLines[m_CurrentLine].m_NameColor = TEAM_RED;
				m_aLines[m_CurrentLine].m_Highlighted = true;
				m_aLines[m_CurrentLine].m_Team = 0;
				Highlighted = true;
			}
			else
			{
				str_copy(m_aLines[m_CurrentLine].m_aName, m_pClient->m_aClients[ClientID].m_aName, sizeof(m_aLines[m_CurrentLine].m_aName));
			}

			str_format(m_aLines[m_CurrentLine].m_aText, sizeof(m_aLines[m_CurrentLine].m_aText), ": %s", pLine);
		}

		char aBuf[1024];
		str_format(aBuf, sizeof(aBuf), "%s%s", m_aLines[m_CurrentLine].m_aName, m_aLines[m_CurrentLine].m_aText);
		if(m_aLines[m_CurrentLine].m_Team == MODE_TRANS)
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "trans", aBuf, Highlighted);
		else
			Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, ClientID == -1?"serv":Team >= 2?"whisper":(m_aLines[m_CurrentLine].m_Team?"teamchat":"chat"), aBuf, Highlighted);
	}

	// play sound
	int64 Now = time_get();
	if(ClientID == -1)
	{
		if(Now-m_aLastSoundPlayed[CHAT_SERVER] >= time_freq()*3/10)
		{
			if(g_Config.m_SndServerMessage)
			{
				m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_CHAT_SERVER, 0);
				m_aLastSoundPlayed[CHAT_SERVER] = Now;
			}
		}
	}
	else if(Highlighted)
	{
		if(Now-m_aLastSoundPlayed[CHAT_HIGHLIGHT] >= time_freq()*3/10)
		{
#ifdef CONF_PLATFORM_MACOSX
			char aBuf[1024];
			str_format(aBuf, sizeof(aBuf), "%s%s", m_aLines[m_CurrentLine].m_aName, m_aLines[m_CurrentLine].m_aText);
			CNotification::notify("AllTheHaxx-Chat", aBuf);
#else
			Graphics()->NotifyWindow();
#endif
			if(g_Config.m_SndHighlight)
			{
				m_pClient->m_pSounds->Play(CSounds::CHN_GUI, g_Config.m_SndHighlightVanilla ? SOUND_CHAT_CLIENT : SOUND_CHAT_HIGHLIGHT, 0);
				m_aLastSoundPlayed[CHAT_HIGHLIGHT] = Now;
			}
		}
	}
	else if(Team != 2)
	{
		if(Now-m_aLastSoundPlayed[CHAT_CLIENT] >= time_freq()*3/10)
		{
			if ((g_Config.m_SndTeamChat || !m_aLines[m_CurrentLine].m_Team)
				&& (g_Config.m_SndChat || m_aLines[m_CurrentLine].m_Team))
			{
				m_pClient->m_pSounds->Play(CSounds::CHN_GUI, g_Config.m_SndHighlightVanilla ? SOUND_CHAT_HIGHLIGHT : SOUND_CHAT_CLIENT, 0);
				m_aLastSoundPlayed[CHAT_CLIENT] = Now;
			}
		}
	}
}

void CChat::OnRender()
{
	CALLSTACK_ADD();

	if(TranslatorAvailable())
	{
		if(m_pTranslator->HasTranslation())
		{
			char aBuf[1024];
			CTranslator::CTransEntry Entry = m_pTranslator->NextTranslation();
			if(Entry.m_In)
			{
				str_format(aBuf, sizeof(aBuf), "'%s%s%s' (%s → %s)",
						   Entry.m_aMentionedName, Entry.m_aMentionedName[0] != '\0' ? ": " : "", Entry.m_aText,
						   Entry.m_aSrcLang, Entry.m_aDstLang);
				AddLine(Entry.m_SaidBy, MODE_TRANS, aBuf);
			}
			else
			{
				if(Entry.m_aMentionedName[0] != '\0')
				{
					str_formatb(aBuf, "%s: %s", Entry.m_aMentionedName, Entry.m_aText);
					Say(0, aBuf, true);
				}
				else
					Say(0, Entry.m_aText, true);
			}
		}
	}

	if (!g_Config.m_ClShowChat)
		return;

	// send pending chat messages
	if(m_PendingChatCounter > 0 && m_LastChatSend+time_freq() < time_get())
	{
		CHistoryEntry *pEntry = m_History.Last();
		for(int i = m_PendingChatCounter-1; pEntry; --i, pEntry = m_History.Prev(pEntry))
		{
			if(i == 0)
			{
				Say(pEntry->m_Team, pEntry->m_aText);
				break;
			}
		}
		--m_PendingChatCounter;
	}

	float Width = 300.0f*Graphics()->ScreenAspect();
	Graphics()->MapScreen(0.0f, 0.0f, Width, 300.0f);
	float x = 5.0f;
	float y = 300.0f-20.0f;
	if(m_Mode != MODE_NONE)
	{
		// render chat input
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, x, y, 8.0f, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = m_pClient->m_pScoreboard->Active() ? 90.0f : 200.0f;
		Cursor.m_MaxLines = 2;

		if(m_Mode == MODE_ALL)
			TextRender()->TextEx(&Cursor, Localize("All"), -1);
		else if(m_Mode == MODE_TEAM)
			TextRender()->TextEx(&Cursor, Localize("Team"), -1);
		else if(m_Mode == MODE_HIDDEN)
			TextRender()->TextEx(&Cursor, Localize("Hidden"), -1);
		else if(m_Mode == MODE_CRYPT)
			TextRender()->TextEx(&Cursor, Localize("Crypt"), -1);
		else
			TextRender()->TextEx(&Cursor, Localize("Chat"), -1);

		TextRender()->TextEx(&Cursor, ": ", -1);

		// IME candidate editing
		bool Editing = false;
		int EditingCursor = Input()->GetEditingCursor();
		if (Input()->GetIMEState())
		{
			if(str_length(Input()->GetIMECandidate()))
			{
				m_Input.Editing(Input()->GetIMECandidate(), EditingCursor);
				Editing = true;
			}
		}

		// check if the visible text has to be moved
		if(m_InputUpdate)
		{
			if(m_ChatStringOffset > 0 && m_Input.GetLength(Editing) < m_OldChatStringLength)
				m_ChatStringOffset = max(0, m_ChatStringOffset-(m_OldChatStringLength-m_Input.GetLength(Editing)));

			if(m_ChatStringOffset > m_Input.GetCursorOffset(Editing))
				m_ChatStringOffset -= m_ChatStringOffset-m_Input.GetCursorOffset(Editing);
			else
			{
				CTextCursor Temp = Cursor;
				Temp.m_Flags = 0;
				TextRender()->TextEx(&Temp, m_Input.GetString(Editing)+m_ChatStringOffset, m_Input.GetCursorOffset(Editing)-m_ChatStringOffset);
				TextRender()->TextEx(&Temp, "|", -1);
				while(Temp.m_LineCount > 1)
				{
					++m_ChatStringOffset;
					Temp = Cursor;
					Temp.m_Flags = 0;
					TextRender()->TextEx(&Temp, m_Input.GetString(Editing)+m_ChatStringOffset, m_Input.GetCursorOffset(Editing)-m_ChatStringOffset);
					TextRender()->TextEx(&Temp, "|", -1);
				}
			}
			m_InputUpdate = false;
		}

		TextRender()->TextEx(&Cursor, m_Input.GetString(Editing)+m_ChatStringOffset, m_Input.GetCursorOffset(Editing)-m_ChatStringOffset);
		static float MarkerOffset = TextRender()->TextWidth(0, 8.0f, "|", -1)/3;
		CTextCursor Marker = Cursor;
		Marker.m_X -= MarkerOffset;
		TextRender()->TextEx(&Marker, "|", -1);
		TextRender()->TextEx(&Cursor, m_Input.GetString(Editing)+m_Input.GetCursorOffset(Editing), -1);
	}

	y -= 8.0f;
#if defined(__ANDROID__)
	x += 120.0f;
#endif

	int64 Now = time_get();
	float LineWidth = m_pClient->m_pScoreboard->Active() ? 90.0f : 200.0f;
	float HeightLimit = m_pClient->m_pScoreboard->Active() ? 230.0f : m_Show ? 50.0f : 200.0f;
	float Begin = x;
#if defined(__ANDROID__)
	float FontSize = 10.0f;
#else
	float FontSize = 6.0f;
#endif
	CTextCursor Cursor;
	int OffsetType = m_pClient->m_pScoreboard->Active() ? 1 : 0;
	for(int i = 0; i < MAX_LINES; i++)
	{
		int r = ((m_CurrentLine-i)+MAX_LINES)%MAX_LINES;
		if(Now > m_aLines[r].m_Time+g_Config.m_ClShowhudChatMsgTime*time_freq() && !m_Show)
			break;

		float SmoothBlend = 1.0f;
		float SmoothYOffs = 0.0f;
		if(g_Config.m_ClSmoothChat)
		{
			float X = ((float)Now - (float)m_aLines[r].m_Time+1.0f) / time_freq();
			SmoothBlend = 1.0f - 3.0f * expf(-4.0f * (X + 0.5f) + 1.6f);
			SmoothYOffs = 3.0f - SmoothBlend * 3.0f;
		}

		char aName[64] = "";
		if(g_Config.m_ClShowIDsChat && m_aLines[r].m_ClientID != -1 && m_aLines[r].m_aName[0] != '\0')
		{
			if (m_aLines[r].m_ClientID >= 10)
				str_format(aName, sizeof(aName),"%d: ", m_aLines[r].m_ClientID);
			else
				str_format(aName, sizeof(aName)," %d: ", m_aLines[r].m_ClientID);
			str_append(aName, m_aLines[r].m_aName,sizeof(aName));
		}
		else
		{
			str_copy(aName, m_aLines[r].m_aName, sizeof(aName));
		}

		// get the y offset (calculate it if we haven't done that yet)
		if(m_aLines[r].m_YOffset[OffsetType] < 0.0f)
		{
			TextRender()->SetCursor(&Cursor, Begin, 0.0f, FontSize, 0);
			Cursor.m_LineWidth = LineWidth;
			TextRender()->TextEx(&Cursor, "♥ ", -1);
			TextRender()->TextEx(&Cursor, aName, -1);
			TextRender()->TextExParse(&Cursor, m_aLines[r].m_aText, g_Config.m_ClShowChatIgnoreColors);
			m_aLines[r].m_YOffset[OffsetType] = Cursor.m_Y + Cursor.m_FontSize;
		}
		y -= m_aLines[r].m_YOffset[OffsetType];

		// cut off if msgs waste too much space
		if(y < HeightLimit)
			break;

		y += SmoothYOffs;

		float Blend = min(SmoothBlend, Now > m_aLines[r].m_Time+14*time_freq() && !m_Show ? 1.0f-(Now-m_aLines[r].m_Time-14*time_freq())/(2.0f*time_freq()) : 1.0f);

		// reset the cursor
		TextRender()->SetCursor(&Cursor, Begin + (g_Config.m_ClChatAvatar ? 3.0f : 0.0f), y, FontSize, TEXTFLAG_RENDER);
		Cursor.m_LineWidth = LineWidth;

		// render name
		if (m_aLines[r].m_ClientID == -1)
		{
			//TextRender()->TextColor(1.0f, 1.0f, 0.5f, Blend); // system
			vec3 rgb = HslToRgb(vec3(g_Config.m_ClMessageSystemHue / 255.0f, g_Config.m_ClMessageSystemSat / 255.0f, g_Config.m_ClMessageSystemLht / 255.0f));
			TextRender()->TextColor(rgb.r, rgb.g, rgb.b, Blend);
		}
		else if (m_aLines[r].m_Team)
			TextRender()->TextColor(0.45f, 0.9f, 0.45f, Blend); // team message
		else if(m_aLines[r].m_NameColor == TEAM_RED)
			TextRender()->TextColor(1.0f, 0.5f, 0.5f, Blend); // red
		else if(m_aLines[r].m_NameColor == TEAM_BLUE)
			TextRender()->TextColor(0.7f, 0.7f, 1.0f, Blend); // blue
		else if(m_aLines[r].m_NameColor == TEAM_SPECTATORS)
			TextRender()->TextColor(0.75f, 0.5f, 0.75f, Blend); // spectator
		else if(m_aLines[r].m_ClientID >= 0 && g_Config.m_ClChatTeamColors && m_pClient->m_Teams.Team(m_aLines[r].m_ClientID))
		{
			vec3 rgb = HslToRgb(vec3(m_pClient->m_Teams.Team(m_aLines[r].m_ClientID) / 64.0f, 1.0f, 0.75f));
			TextRender()->TextColor(rgb.r, rgb.g, rgb.b, Blend);
		}
		else
			TextRender()->TextColor(0.8f, 0.8f, 0.8f, Blend);

		// friends always in green // TODO: settings pls!
		if(m_aLines[r].m_ClientID > 0 && m_aLines[r].m_ClientID < MAX_CLIENTS && g_Config.m_ClColorfulClient && m_pClient->Friends()->IsFriend(m_pClient->m_aClients[m_aLines[r].m_ClientID].m_aName, m_pClient->m_aClients[m_aLines[r].m_ClientID].m_aClan, true))
			TextRender()->TextColor(0,0.7f,0,Blend);
		else if (m_aLines[r].m_Team == MODE_TRANS) // translator in blue
			TextRender()->TextColor(0.2f,0.2f,0.7f,Blend);
		else if (m_aLines[r].m_ClientID <= FAKE_ID_LUA) // lua in ReD
			TextRender()->TextColor(0.7f,0.2f,0.2f,Blend);
		TextRender()->TextEx(&Cursor, aName, -1);

		// render line
		vec3 rgb;
		if (m_aLines[r].m_ClientID == -1) // system message
			rgb = HslToRgb(vec3(g_Config.m_ClMessageSystemHue / 255.0f, g_Config.m_ClMessageSystemSat / 255.0f, g_Config.m_ClMessageSystemLht / 255.0f));
		else if (m_aLines[r].m_Team == MODE_TRANS) // translator
			rgb = vec3(0.45f, 0.45f, 1.0f);
		else if (m_aLines[r].m_ClientID <= FAKE_ID_LUA) // lua
			rgb = vec3(1.0f, 0.45f, 0.45f);
		else if (m_aLines[r].m_Highlighted)
			rgb = HslToRgb(vec3(g_Config.m_ClMessageHighlightHue / 255.0f, g_Config.m_ClMessageHighlightSat / 255.0f, g_Config.m_ClMessageHighlightLht / 255.0f));
		else if (m_aLines[r].m_Team)
			rgb = HslToRgb(vec3(g_Config.m_ClMessageTeamHue / 255.0f, g_Config.m_ClMessageTeamSat / 255.0f, g_Config.m_ClMessageTeamLht / 255.0f));
		else if(m_aLines[r].m_Hidden)
			rgb = vec3(1.0f, 0.7f, 0.0f);
		else
			rgb = HslToRgb(vec3(g_Config.m_ClMessageHue / 255.0f, g_Config.m_ClMessageSat / 255.0f, g_Config.m_ClMessageLht / 255.0f));

		if(g_Config.m_ClChatAvatar && m_aLines[r].m_ClientID >= 0 && m_pClient->m_aClients[m_aLines[r].m_ClientID].m_Active)
		{
			CGameClient::CClientData *pClientData = &m_pClient->m_aClients[m_aLines[r].m_ClientID];
			CTeeRenderInfo RenderInfo = pClientData->m_RenderInfo;
			RenderInfo.m_Size = 8.0f;
			RenderInfo.m_ColorBody.a = Blend;
			RenderInfo.m_ColorFeet.a = Blend;

			RenderTools()->RenderTee(CAnimState::GetIdle(), &RenderInfo, 0, vec2(-1.0f, 0.0f), vec2(Begin, y+FontSize-1.5f), true);
		}

		TextRender()->TextColor(rgb.r, rgb.g, rgb.b, Blend);
		TextRender()->TextExParse(&Cursor, m_aLines[r].m_aText, g_Config.m_ClShowChatIgnoreColors);
	}

	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

#if defined(__ANDROID__)
	static int deferEvent = 0;
	if( UI()->AndroidTextInputShown() )
	{
		if(m_Mode == MODE_NONE)
		{
			deferEvent++;
			if( deferEvent > 2 )
				EnableMode(0);
		}
		else
			deferEvent = 0;
	}
	else
	{
		if(m_Mode != MODE_NONE)
		{
			deferEvent++;
			if( deferEvent > 2 )
			{
				IInput::CEvent Event;
				Event.m_Flags = IInput::FLAG_PRESS;
				Event.m_Key = KEY_RETURN;
				OnInput(Event);
			}
		}
		else
			deferEvent = 0;
	}
#endif
}

const char *CChat::PrepareMsgForTrans(const char *pMessage, char aNameBuf[MAX_NAME_LENGTH]) const
{
	// check for player mentions and ignore them
	const char *pTranslateStart = pMessage;
	aNameBuf[0] = '\0';
	const char *pColonAt = str_find(pMessage, ":");
	if(pColonAt)
	{
		// search for a player with this name to check if it's actually a name
		int PrefixLen = (int)(pColonAt - pMessage);
		if(PrefixLen < MAX_NAME_LENGTH)
		{
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				if(m_pClient->m_aClients[i].m_Active &&
				   str_comp_num(m_pClient->m_aClients[i].m_aName, pMessage, PrefixLen) == 0)
				{
					pTranslateStart = pColonAt+1;
					str_copy(aNameBuf, pMessage, min((int)MAX_NAME_LENGTH, PrefixLen+1));
					return pTranslateStart;
				}
			}
		}
	}
	return pTranslateStart;
}

bool CChat::HandleTCommands(const char *pMsg)
{
	CALLSTACK_ADD();

	if(!TranslatorAvailable() || !g_Config.m_ClTransChatCmds || !(pMsg[0] == '$' && pMsg[1] == '$'))
		return false;

	char aCmd[512][256];
	mem_zerob(aCmd);

	int Cmd = 0;
	int Char = 0;

	for(int i = 1; i < str_length(pMsg); i++)
	{
		if(pMsg[i] == ' ')
		{
			Cmd++;
			Char = 0;
			continue;
		}

		aCmd[Cmd][Char] = pMsg[i];
		Char++;
	}

	if(!str_comp_nocase(aCmd[0], "$cmdlist"))
	{
		AddLine(0, MODE_TRANS, "~~~~ Commands ~~~~");
		AddLine(0, MODE_TRANS, "'$$tout <dst> <message>': Translate a message");
		AddLine(0, MODE_TRANS, "'$$tin <src> <ID>': Translate message in the chat ($tin 0 to translate the last message)");
		return true;
	}
	else if(!str_comp_nocase(aCmd[0], "$tout"))
	{
		if(!aCmd[1][0] || !aCmd[2][0])
		{
			AddLine(0, MODE_TRANS, "Please use '$$tout <dst> <message>'");
			return true;
		}

		int i = 2;
		char aBuf[256];
		char aRawMsg[512];

		mem_zero(&aRawMsg, sizeof(aRawMsg));

		while(aCmd[i][0])
		{
			str_format(aBuf, sizeof(aBuf), "%s ", aCmd[i]);
			str_append(aRawMsg, aBuf, sizeof(aRawMsg));
			i++;
		}

		char aMentionedName[MAX_NAME_LENGTH];
		const char *pTransStart = PrepareMsgForTrans(aRawMsg, aMentionedName);
		m_pTranslator->RequestTranslation(g_Config.m_ClTransOutSrc, aCmd[1], pTransStart, false, aMentionedName);
		return true;
	}
	else if(!str_comp_nocase(aCmd[0], "$tin"))
	{
		if(!aCmd[1][0] || !aCmd[2][0])
		{
			AddLine(0, MODE_TRANS, "Please use '$$tin <src> <ID>'");
			return true;
		}

		int MsgID = str_toint(aCmd[2]);
		// DENNIS XXX TODO RANGE CHECK

		char aMentionedName[MAX_NAME_LENGTH];
		const char *pTransStart = PrepareMsgForTrans(m_aLines[m_CurrentLine-MsgID].m_aText, aMentionedName);
		m_pTranslator->RequestTranslation(aCmd[1], g_Config.m_ClTransInDst, pTransStart, true,
										  aMentionedName, m_aLines[m_CurrentLine-MsgID].m_ClientID);
		return true;
	}

	//AddLine(FAKE_ID_TRANS, 0, "Unknown command. Try '$$cmdlist'!");
	//return true;
	return false;
}

void CChat::Say(int Team, const char *pLine, bool NoTrans, bool CalledByLua)
{
	CALLSTACK_ADD();

	if(!pLine)
		return;

	m_LastChatSend = time_get();

	if(HandleTCommands(pLine))
		return;

	char aMessage[1024];
	str_copy(aMessage, pLine, sizeof(aMessage));

	if(!NoTrans && g_Config.m_ClTransOut && TranslatorAvailable())
	{
		// translate
		char aMentionedName[MAX_NAME_LENGTH];
		const char *pTransStart = PrepareMsgForTrans(aMessage, aMentionedName);
		if(str_length(pTransStart) > 1 && pTransStart[0] != '/')
		{
			m_pTranslator->RequestTranslation(g_Config.m_ClTransOutSrc, g_Config.m_ClTransOutDst, pTransStart, false, aMentionedName);
			return;
		}
	}

	int DiscardChat = false;
	//LUA_FIRE_EVENT("OnChatSend", Team, pLine);
	if(!CalledByLua)
	{
		LUA_FIRE_EVENT_RES({
			if(_LuaEventResult.isBool())
				DiscardChat |= _LuaEventResult.cast<bool>();
						   }, "OnChatSend", Team, pLine)
	}

	if(!g_Config.m_ClChat || DiscardChat)
	{
		if(!g_Config.m_ClChat)
			//m_pClient->m_pHud->PushNotification(Localize("Chat is disabled. Set 'cl_chat' to 1 or 2 to send messages!"));
			AddLine(-1, 0, Localize("Chat is disabled. Set 'cl_chat' to 1 or 2 to send messages!"), true);
		return;
	}

	// send chat message
	CNetMsg_Cl_Say Msg;
	Msg.m_Team = Team;
	Msg.m_pMessage = aMessage;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}

void CChat::SayLua(int Team, const char *pLine, bool NoTrans)
{
	Say(Team, pLine, NoTrans, true);
}


////////////////////////////
// chat crypt stuff below //
////////////////////////////

char *CChat::EncryptMsg(char *pBuffer, unsigned int BufferSize, const char *pMsg)
{
	CALLSTACK_ADD();

	if(!m_GotKey)
	{
		m_pClient->m_pHud->PushNotification("Set a key before using crypt chat!");
		return 0;
	}

	// with this we can later check the message
	char aMsg[512] = { (char)m_CryptKey.key[0], (char)m_CryptKey.key[3], '\0' };
	str_appendb(aMsg, pMsg);

	unsigned int OutputSize;
	uint8_t *pEncrypted = str_aes128_encrypt(aMsg, &m_CryptKey, &OutputSize, &m_CryptIV);

	str_hex_simple(pBuffer, BufferSize, pEncrypted, OutputSize);

	mem_free(pEncrypted);

	return pBuffer;
}

char *CChat::DecryptMsg(char *pBuffer, unsigned int BufferSize, const char *pMsg)
{
	CALLSTACK_ADD();

	if(!m_GotKey)
		return 0;

	unsigned DataSize = 0;
	unsigned char aEncrypted[1024] = {0};
	for(int i = 0, j = 0; pMsg[j]; i++, j+=2)
	{
		char aBuf[3];
		str_copy(aBuf, &pMsg[j], sizeof(aBuf));
		aEncrypted[i] = (unsigned char)strtol(aBuf, 0, 16);
		DataSize++;
	}

	char *pDecrypted = str_aes128_decrypt(aEncrypted, DataSize, &m_CryptKey, pBuffer, BufferSize, &m_CryptIV);

	// verify the message
	if((unsigned char)pDecrypted[0] == (unsigned char)m_CryptKey.key[0] && (unsigned char)pDecrypted[1] == (unsigned char)m_CryptKey.key[3])
		return pDecrypted + 2;

	if(g_Config.m_Debug)
		dbg_msg("crypt", "%02x,%02x != %02x,%02x",
				(unsigned char)pDecrypted[0], (unsigned char)pDecrypted[1],
				(unsigned char)m_CryptKey.key[0], (unsigned char)m_CryptKey.key[3]
		);
	return 0;
}


void CChat::SayChat(const char *pLine)
{
	CALLSTACK_ADD();

	if(!pLine || str_length(pLine) < 1)
		return;

	bool AddEntry = false;

	if(m_LastChatSend+time_freq() < time_get())
	{
		Say(m_Mode == MODE_ALL ? 0 : 1, pLine);
		AddEntry = true;
	}
	else if(m_PendingChatCounter < 3)
	{
		++m_PendingChatCounter;
		AddEntry = true;
	}

	if(AddEntry)
	{
		CHistoryEntry *pEntry = m_History.Allocate(sizeof(CHistoryEntry)+str_length(pLine)-1);
		pEntry->m_Team = m_Mode == MODE_ALL ? 0 : 1;
		mem_copy(pEntry->m_aText, pLine, str_length(pLine));
	}
}
