/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#define _WIN32_WINNT 0x0501

#include <new>

#include <time.h>
#include <stdlib.h> // qsort
#include <stdarg.h>
#include <string.h>
#include <climits>
#include <fstream>
#include <csignal>
#include <locale.h> //setlocale

#include <base/math.h>
#include <base/vmath.h>
#include <base/system.h>

#include <game/client/components/menus.h>
#include <game/client/gameclient.h>
#include <game/editor/editor.h>

#include <engine/client.h>
#include <engine/config.h>
#include <engine/console.h>
#include <engine/editor.h>
#include <engine/engine.h>
#include <engine/graphics.h>
#include <engine/input.h>
#include <engine/keys.h>
#include <engine/map.h>
#include <engine/masterserver.h>
#include <engine/serverbrowser.h>
#include <engine/sound.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/irc.h>

#include <engine/external/md5/md5.h>

#include <engine/shared/config.h>
#include <engine/shared/compression.h>
#include <engine/shared/datafile.h>
#include <engine/shared/demo.h>
#include <engine/shared/filecollection.h>
#include <engine/shared/mapchecker.h>
#include <engine/shared/network.h>
#include <engine/shared/packer.h>
#include <engine/shared/protocol.h>
#include <engine/shared/ringbuffer.h>
#include <engine/shared/snapshot.h>
#include <engine/shared/fifo.h>

#include <engine/client/irc.h>

#include <game/version.h>
#include <game/client/components/console.h>
#include <game/client/components/identity.h>

#include <mastersrv/mastersrv.h>
#include <versionsrv/versionsrv.h>

#include <engine/client/serverbrowser.h>

#if defined(CONF_FAMILY_WINDOWS)
	#define WIN32_LEAN_AND_MEAN
	#include <windows.h>
#endif

#include "friends.h"
#include "serverbrowser.h"
#include "fetcher.h"
#include "updater.h"
#include "client.h"
#include "graphics_threaded.h"

#include <zlib.h>

#include "SDL.h"
#if defined(CONF_FAMILY_WINDOWS)
	#include "SDL_syswm.h"
#else
#include <fcntl.h>
#include <engine/curlwrapper.h>
#include <fstream>

#endif
#ifdef main
#undef main
#endif

void CGraph::Init(float Min, float Max)
{
	CALLSTACK_ADD();

	m_Min = Min;
	m_Max = Max;
	m_Index = 0;
}

void CGraph::ScaleMax()
{
	CALLSTACK_ADD();

	int i = 0;
	m_Max = 0;
	for(i = 0; i < MAX_VALUES; i++)
	{
		if(m_aValues[i] > m_Max)
			m_Max = m_aValues[i];
	}
}

void CGraph::ScaleMin()
{
	CALLSTACK_ADD();

	int i = 0;
	m_Min = m_Max;
	for(i = 0; i < MAX_VALUES; i++)
	{
		if(m_aValues[i] < m_Min)
			m_Min = m_aValues[i];
	}
}

void CGraph::Add(float v, float r, float g, float b)
{
	CALLSTACK_ADD();

	m_Index = (m_Index+1)&(MAX_VALUES-1);
	m_aValues[m_Index] = v;
	m_aColors[m_Index][0] = r;
	m_aColors[m_Index][1] = g;
	m_aColors[m_Index][2] = b;
}

void CGraph::Render(IGraphics *pGraphics, int Font, float x, float y, float w, float h, const char *pDescription)
{
	CALLSTACK_ADD();

	//m_pGraphics->BlendNormal();


	pGraphics->TextureSet(-1);

	pGraphics->QuadsBegin();
	pGraphics->SetColor(0, 0, 0, 0.75f);
	IGraphics::CQuadItem QuadItem(x, y, w, h);
	pGraphics->QuadsDrawTL(&QuadItem, 1);
	pGraphics->QuadsEnd();

	pGraphics->LinesBegin();
	pGraphics->SetColor(0.95f, 0.95f, 0.95f, 1.00f);
	IGraphics::CLineItem LineItem(x, y+h/2, x+w, y+h/2);
	pGraphics->LinesDraw(&LineItem, 1);
	pGraphics->SetColor(0.5f, 0.5f, 0.5f, 0.75f);
	IGraphics::CLineItem Array[2] = {
		IGraphics::CLineItem(x, y+(h*3)/4, x+w, y+(h*3)/4),
		IGraphics::CLineItem(x, y+h/4, x+w, y+h/4)};
	pGraphics->LinesDraw(Array, 2);
	for(int i = 1; i < MAX_VALUES; i++)
	{
		float a0 = (i-1)/(float)MAX_VALUES;
		float a1 = i/(float)MAX_VALUES;
		int i0 = (m_Index+i-1)&(MAX_VALUES-1);
		int i1 = (m_Index+i)&(MAX_VALUES-1);

		float v0 = (m_aValues[i0]-m_Min) / (m_Max-m_Min);
		float v1 = (m_aValues[i1]-m_Min) / (m_Max-m_Min);

		IGraphics::CColorVertex Array[2] = {
			IGraphics::CColorVertex(0, m_aColors[i0][0], m_aColors[i0][1], m_aColors[i0][2], 0.75f),
			IGraphics::CColorVertex(1, m_aColors[i1][0], m_aColors[i1][1], m_aColors[i1][2], 0.75f)};
		pGraphics->SetColorVertex(Array, 2);
		IGraphics::CLineItem LineItem(x+a0*w, y+h-v0*h, x+a1*w, y+h-v1*h);
		pGraphics->LinesDraw(&LineItem, 1);

	}
	pGraphics->LinesEnd();

	pGraphics->TextureSet(Font);
	pGraphics->QuadsBegin();
	pGraphics->QuadsText(x+2, y+h-16, 16, pDescription);

	char aBuf[32];
	str_format(aBuf, sizeof(aBuf), "%.2f", m_Max);
	pGraphics->QuadsText(x+w-8*str_length(aBuf)-8, y+2, 16, aBuf);

	str_format(aBuf, sizeof(aBuf), "%.2f", m_Min);
	pGraphics->QuadsText(x+w-8*str_length(aBuf)-8, y+h-16, 16, aBuf);
	pGraphics->QuadsEnd();
}


void CSmoothTime::Init(int64 Target)
{
	m_Snap = time_get();
	m_Current = Target;
	m_Target = Target;
	m_aAdjustSpeed[0] = 0.3f;
	m_aAdjustSpeed[1] = 0.3f;
	m_Graph.Init(0.0f, 0.5f);
}

void CSmoothTime::SetAdjustSpeed(int Direction, float Value)
{
	m_aAdjustSpeed[Direction] = Value;
}

int64 CSmoothTime::Get(int64 Now)
{
	int64 c = m_Current + (Now - m_Snap);
	int64 t = m_Target + (Now - m_Snap);

	// it's faster to adjust upward instead of downward
	// we might need to adjust these abit

	float AdjustSpeed = m_aAdjustSpeed[0];
	if(t > c)
		AdjustSpeed = m_aAdjustSpeed[1];

	float a = ((Now-m_Snap)/(float)time_freq()) * AdjustSpeed;
	if(a > 1.0f)
		a = 1.0f;

	int64 r = c + (int64)((t-c)*a);

	m_Graph.Add(a+0.5f,1,1,1);

	return r;
}

void CSmoothTime::UpdateInt(int64 Target)
{
	int64 Now = time_get();
	m_Current = Get(Now);
	m_Snap = Now;
	m_Target = Target;
}

void CSmoothTime::Update(CGraph *pGraph, int64 Target, int TimeLeft, int AdjustDirection)
{
	int UpdateTimer = 1;

	if(TimeLeft < 0)
	{
		int IsSpike = 0;
		if(TimeLeft < -50)
		{
			IsSpike = 1;

			m_SpikeCounter += 5;
			if(m_SpikeCounter > 50)
				m_SpikeCounter = 50;
		}

		if(IsSpike && m_SpikeCounter < 15)
		{
			// ignore this ping spike
			UpdateTimer = 0;
			pGraph->Add(TimeLeft, 1,1,0);
		}
		else
		{
			pGraph->Add(TimeLeft, 1,0,0);
			if(m_aAdjustSpeed[AdjustDirection] < 30.0f)
				m_aAdjustSpeed[AdjustDirection] *= 2.0f;
		}
	}
	else
	{
		if(m_SpikeCounter)
			m_SpikeCounter--;

		pGraph->Add(TimeLeft, 0,1,0);

		m_aAdjustSpeed[AdjustDirection] *= 0.95f;
		if(m_aAdjustSpeed[AdjustDirection] < 2.0f)
			m_aAdjustSpeed[AdjustDirection] = 2.0f;
	}

	if(UpdateTimer)
		UpdateInt(Target);
}


CClient::CClient() : m_DemoPlayer(&m_SnapshotDelta)
{
	m_DemoRecorder[0] = CDemoRecorder(&m_SnapshotDelta);
	m_DemoRecorder[1] = CDemoRecorder(&m_SnapshotDelta);
	m_DemoRecorder[2] = CDemoRecorder(&m_SnapshotDelta);

	m_pEditor = 0;
	m_pInput = 0;
	m_pGraphics = 0;
	m_pSound = 0;
	m_pGameClient = 0;
	m_pMap = 0;
	m_pConsole = 0;
	m_pEngine = 0;
	m_pFetcher = 0;
	m_pCurlWrapper = 0;
	m_pUpdater = 0;
	m_pStorage = 0;
	m_pMasterServer = 0;
	m_pIRC = 0;
	m_pInputThread = 0;

	m_RenderFrameTime = 0.0001f;
	m_RenderFrameTimeLow = 1.0f;
	m_RenderFrameTimeHigh = 0.0f;
	m_RenderFrames = 0;
	m_LastRenderTime = time_get();

	m_GameTickSpeed = SERVER_TICK_SPEED;

	m_SnapCrcErrors = 0;
	m_AutoScreenshotRecycle = false;
	m_AutoStatScreenshotRecycle = false;
	m_EditorActive = false;

	m_AckGameTick[0] = -1;
	m_AckGameTick[1] = -1;
	m_CurrentRecvTick[0] = 0;
	m_CurrentRecvTick[1] = 0;
	m_RconAuthed[0] = 0;
	m_RconAuthed[1] = 0;
	m_RconPassword[0] = '\0';

	// version-checking
	//m_aVersionStr[0] = '0';
	//m_aVersionStr[1] = 0;

	// pinging
	m_PingStartTime = 0;
	m_LocalStartTime = 0;
	m_TimerStartTime = 0;

	//
	m_aCurrentMap[0] = 0;
	m_CurrentMapCrc = 0;

	//
	m_aCmdConnect[0] = 0;

	// map download
	m_aMapdownloadFilename[0] = 0;
	m_aMapdownloadName[0] = 0;
	m_pMapdownloadTask = 0;
	m_MapdownloadFile = 0;
	m_MapdownloadChunk = 0;
	m_MapdownloadCrc = 0;
	m_MapdownloadAmount = -1;
	m_MapdownloadTotalsize = -1;
	m_pMapdownloadSource = "gameserver";

	m_CurrentServerInfoRequestTime = -1;

	m_CurrentInput[0] = 0;
	m_CurrentInput[1] = 0;
	m_LastDummy = 0;
	m_LastDummy2 = 0;
	m_LocalIDs[0] = 0;
	m_LocalIDs[1] = 0;
	m_Fire = 0;

	mem_zero(&m_aInputs, sizeof(m_aInputs));
	mem_zero(&m_DummyInput, sizeof(m_DummyInput));
	mem_zero(&HammerInput, sizeof(HammerInput));
	HammerInput.m_Fire = 0;

	m_State = IClient::STATE_OFFLINE;
	m_Restarting = false;
	m_aServerAddressStr[0] = 0;

	mem_zero(m_aSnapshots, sizeof(m_aSnapshots));
	m_SnapshotStorage[0].Init();
	m_SnapshotStorage[1].Init();
	m_ReceivedSnapshots[0] = 0;
	m_ReceivedSnapshots[1] = 0;
	m_SnapshotParts = 0;

	m_UseTempRconCommands = 0;
	m_ResortServerBrowser = false;

	m_VersionInfo.m_State = CVersionInfo::STATE_INIT;

	if (g_Config.m_ClDummy == 0)
		m_LastDummyConnectTime = 0;

	m_DDNetSrvListTokenSet = false;
	m_ReconnectTime = 0;
	m_GenerateTimeoutSeed = true;
	m_DummyConnected = 0;

	m_pDatabase = new CSql();

	char *pQueryBuf = sqlite3_mprintf("CREATE TABLE IF NOT EXISTS names_v2 (" \
		"id INTEGER PRIMARY KEY AUTOINCREMENT, " \
		"name TEXT NOT NULL UNIQUE, " \
		"clan TEXT NOT NULL, " \
		"server TEXT NOT NULL, " \
		"gametype TEXT NOT NULL, " \
		"num_clients TEXT NOT NULL, " \
		"state TEXT NOT NULL, " \
		"score INTEGER, " \
		"last_seen TIMESTAMP DEFAULT CURRENT_TIMESTAMP);");
	CQuery *pQuery = new CQuery(pQueryBuf);
	m_pDatabase->InsertQuery(pQuery);
}

CClient::~CClient()
{
	if(m_pDatabase)
		delete m_pDatabase;
	if(m_pMapdownloadTask)
		m_pMapdownloadTask->Abort();
}

void CClient::LoadMapDatabaseUrls()
{
	m_MapDbUrls.clear();

	int prior = 0;
	std::string line;
	std::ifstream file(g_Config.m_ClMapDbFile);
	if(file.is_open())
	{
		while(std::getline(file, line))
		{
			if(line == "" || line.c_str()[0] == '#')
				continue;

			//line = line.replace(line.begin(), line.end(), "\n", "\0");
			MapDbUrl e;
			e.prior = prior++;
			e.url = line;
			m_MapDbUrls.add_unsorted(e);
		}
		file.close();

		m_MapDbUrls.sort_range();
		dbg_msg("mapfetcher", "loaded %i url%s from file '%s'", prior, prior > 1 ? "s" : "", g_Config.m_ClMapDbFile);
	}
	else
		dbg_msg("mapfetcher/error", "failed to open url file '%s', using ddnet's database only", g_Config.m_ClMapDbFile);

	if(m_MapDbUrls.size() == 0)
	{
		MapDbUrl e;
		e.prior = 0;
		e.url = std::string("http://maps.ddnet.tw");
		m_MapDbUrls.add(e);
	}
}

// ----- send functions -----
int CClient::SendMsg(CMsgPacker *pMsg, int Flags)
{
	CALLSTACK_ADD();

	return SendMsgEx(pMsg, Flags, false);
}

int CClient::SendMsgEx(CMsgPacker *pMsg, int Flags, bool System)
{
	CALLSTACK_ADD();

	CNetChunk Packet;

	if(State() == IClient::STATE_OFFLINE)
		return 0;

	mem_zero(&Packet, sizeof(CNetChunk));

	Packet.m_ClientID = 0;
	Packet.m_pData = pMsg->Data();
	Packet.m_DataSize = pMsg->Size();

	// HACK: modify the message id in the packet and store the system flag
	if(*((unsigned char*)Packet.m_pData) == 1 && System && Packet.m_DataSize == 1)
		dbg_break();

	*((unsigned char*)Packet.m_pData) <<= 1;
	if(System)
		*((unsigned char*)Packet.m_pData) |= 1;

	if(Flags&MSGFLAG_VITAL)
		Packet.m_Flags |= NETSENDFLAG_VITAL;
	if(Flags&MSGFLAG_FLUSH)
		Packet.m_Flags |= NETSENDFLAG_FLUSH;

	if(Flags&MSGFLAG_RECORD)
	{
		for(int i = 0; i < RECORDER_MAX; i++)
			if(m_DemoRecorder[i].IsRecording())
				m_DemoRecorder[i].RecordMessage(Packet.m_pData, Packet.m_DataSize);
	}

	if(!(Flags&MSGFLAG_NOSEND))
	{
		m_NetClient[g_Config.m_ClDummy].Send(&Packet);
	}

	return 0;
}

void CClient::SendInfo()
{
	CALLSTACK_ADD();

	CMsgPacker Msg(NETMSG_INFO);
	Msg.AddString(GameClient()->NetVersion(), 128);
	Msg.AddString(g_Config.m_Password, 128);
	SendMsgEx(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH);
}


void CClient::SendEnterGame()
{
	CALLSTACK_ADD();

	CMsgPacker Msg(NETMSG_ENTERGAME);
	SendMsgEx(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH);
}

void CClient::SendReady()
{
	CALLSTACK_ADD();

	CMsgPacker Msg(NETMSG_READY);
	SendMsgEx(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH);
}

void CClient::SendMapRequest()
{
	CALLSTACK_ADD();

	if(m_MapdownloadFile)
		io_close(m_MapdownloadFile);
	m_MapdownloadFile = Storage()->OpenFile(m_aMapdownloadFilename, IOFLAG_WRITE, IStorageTW::TYPE_SAVE);
	CMsgPacker Msg(NETMSG_REQUEST_MAP_DATA);
	Msg.AddInt(m_MapdownloadChunk);
	SendMsgEx(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH);
}

void CClient::SendPlayerInfo(bool Start)
{
	CALLSTACK_ADD();

	if(Start)
	{
		CNetMsg_Cl_StartInfo Msg;
		Msg.m_pName = g_Config.m_PlayerName;
		Msg.m_pClan = g_Config.m_PlayerClan;
		Msg.m_Country = g_Config.m_PlayerCountry;
		Msg.m_pSkin = g_Config.m_ClPlayerSkin;
		Msg.m_UseCustomColor = g_Config.m_ClPlayerUseCustomColor;
		Msg.m_ColorBody = g_Config.m_ClPlayerColorBody;
		Msg.m_ColorFeet = g_Config.m_ClPlayerColorFeet;
		CMsgPacker Packer(Msg.MsgID());
		Msg.Pack(&Packer);
		SendMsgExY(&Packer, MSGFLAG_VITAL, false, 0);
	}
	else
	{
		CNetMsg_Cl_ChangeInfo Msg;
		Msg.m_pName = g_Config.m_PlayerName;
		Msg.m_pClan = g_Config.m_PlayerClan;
		Msg.m_Country = g_Config.m_PlayerCountry;
		Msg.m_pSkin = g_Config.m_ClPlayerSkin;
		Msg.m_UseCustomColor = g_Config.m_ClPlayerUseCustomColor;
		Msg.m_ColorBody = g_Config.m_ClPlayerColorBody;
		Msg.m_ColorFeet = g_Config.m_ClPlayerColorFeet;
		CMsgPacker Packer(Msg.MsgID());
		Msg.Pack(&Packer);
		SendMsgExY(&Packer, MSGFLAG_VITAL, false, 0);
	}
}

void CClient::RconAuth(const char *pName, const char *pPassword)
{
	CALLSTACK_ADD();

	if(RconAuthed())
		return;

	str_copy(m_RconPassword, pPassword, sizeof(m_RconPassword));

	CMsgPacker Msg(NETMSG_RCON_AUTH);
	Msg.AddString(pName, 32);
	Msg.AddString(pPassword, 32);
	Msg.AddInt(1);
	SendMsgEx(&Msg, MSGFLAG_VITAL);
}

void CClient::Rcon(const char *pCmd)
{
	CMsgPacker Msg(NETMSG_RCON_CMD);
	Msg.AddString(pCmd, 256);
	SendMsgEx(&Msg, MSGFLAG_VITAL);
}

bool CClient::ConnectionProblems()
{
	CALLSTACK_ADD();

	return m_NetClient[g_Config.m_ClDummy].GotProblems() != 0;
}

void CClient::DirectInput(int *pInput, int Size)
{
	CALLSTACK_ADD();

	int i;
	CMsgPacker Msg(NETMSG_INPUT);
	Msg.AddInt(m_AckGameTick[g_Config.m_ClDummy]);
	Msg.AddInt(m_PredTick[g_Config.m_ClDummy]);
	Msg.AddInt(Size);

	for(i = 0; i < Size/4; i++)
		Msg.AddInt(pInput[i]);

	SendMsgEx(&Msg, 0);
}

void CClient::SendInput()
{
	CALLSTACK_ADD();

	int64 Now = time_get();

	if(m_PredTick[g_Config.m_ClDummy] <= 0)
		return;

	// fetch input
	int Size = GameClient()->OnSnapInput(m_aInputs[g_Config.m_ClDummy][m_CurrentInput[g_Config.m_ClDummy]].m_aData);

	if(Size)
	{
		// pack input
		CMsgPacker Msg(NETMSG_INPUT);
		Msg.AddInt(m_AckGameTick[g_Config.m_ClDummy]);
		Msg.AddInt(m_PredTick[g_Config.m_ClDummy]);
		Msg.AddInt(Size);

		m_aInputs[g_Config.m_ClDummy][m_CurrentInput[g_Config.m_ClDummy]].m_Tick = m_PredTick[g_Config.m_ClDummy];
		m_aInputs[g_Config.m_ClDummy][m_CurrentInput[g_Config.m_ClDummy]].m_PredictedTime = m_PredictedTime.Get(Now);
		m_aInputs[g_Config.m_ClDummy][m_CurrentInput[g_Config.m_ClDummy]].m_Time = Now;

		// pack it
		for(int i = 0; i < Size/4; i++)
			Msg.AddInt(m_aInputs[g_Config.m_ClDummy][m_CurrentInput[g_Config.m_ClDummy]].m_aData[i]);

		m_CurrentInput[g_Config.m_ClDummy]++;
		m_CurrentInput[g_Config.m_ClDummy]%=200;

		SendMsgEx(&Msg, MSGFLAG_FLUSH);
	}

	if(m_LastDummy != (bool)g_Config.m_ClDummy)
	{
		m_DummyInput = GameClient()->getPlayerInput(!g_Config.m_ClDummy);
		m_LastDummy = g_Config.m_ClDummy;

		if (g_Config.m_ClDummyResetOnSwitch)
		{
			m_DummyInput.m_Jump = 0;
			m_DummyInput.m_Hook = 0;
			if(m_DummyInput.m_Fire & 1)
				m_DummyInput.m_Fire++;
			m_DummyInput.m_Direction = 0;
			GameClient()->ResetDummyInput();
		}
	}

	if(!g_Config.m_ClDummy)
		m_LocalIDs[0] = ((CGameClient *)GameClient())->m_Snap.m_LocalClientID;
	else
		m_LocalIDs[1] = ((CGameClient *)GameClient())->m_Snap.m_LocalClientID;

	if(m_DummyConnected)
	{
		if(g_Config.m_ClDummyHammer)
		{
			if ((((float) m_Fire / 12.5) - (int ((float) m_Fire / 12.5))) > 0.01)
			{
				m_Fire++;
				return;
			}
			m_Fire++;

			HammerInput.m_Fire+=2;
			HammerInput.m_WantedWeapon = 1;

			vec2 Main = ((CGameClient *)GameClient())->m_LocalCharacterPos;
			vec2 Dummy = ((CGameClient *)GameClient())->m_aClients[m_LocalIDs[!g_Config.m_ClDummy]].m_Predicted.m_Pos;
			vec2 Dir = Main - Dummy;
			HammerInput.m_TargetX = Dir.x;
			HammerInput.m_TargetY = Dir.y;

			// pack input
			CMsgPacker Msg(NETMSG_INPUT);
			Msg.AddInt(m_AckGameTick[!g_Config.m_ClDummy]);
			Msg.AddInt(m_PredTick[!g_Config.m_ClDummy]);
			Msg.AddInt(sizeof(HammerInput));

			// pack it
			for(unsigned int i = 0; i < sizeof(HammerInput)/4; i++)
				Msg.AddInt(((int*) &HammerInput)[i]);

			SendMsgExY(&Msg, MSGFLAG_FLUSH, true, !g_Config.m_ClDummy);
		}
		else if(g_Config.m_ClDummyHookFly)
		{
			vec2 Main = ((CGameClient *)GameClient())->m_LocalCharacterPos;
			vec2 Dummy = ((CGameClient *)GameClient())->m_aClients[m_LocalIDs[!g_Config.m_ClDummy]].m_Predicted.m_Pos;
			vec2 Dir = Main - Dummy;
			m_DummyInput.m_TargetX = Dir.x;
			m_DummyInput.m_TargetY = Dir.y;

			if(Dummy.y < Main.y && distance(Dummy, Main) > 16)
				m_DummyInput.m_Hook = 1;
			else
				m_DummyInput.m_Hook = 0;

			if(((CGameClient *)GameClient())->m_aClients[m_LocalIDs[!g_Config.m_ClDummy]].m_Predicted.m_HookState == HOOK_RETRACTED || distance(Dummy, Main) < 48)
				m_DummyInput.m_Hook = 0;

			// pack input
			CMsgPacker Msg(NETMSG_INPUT);
			Msg.AddInt(m_AckGameTick[!g_Config.m_ClDummy]);
			Msg.AddInt(m_PredTick[!g_Config.m_ClDummy]);
			Msg.AddInt(sizeof(m_DummyInput));

			// pack it
			for(unsigned int i = 0; i < sizeof(m_DummyInput)/4; i++)
				Msg.AddInt(((int*) &m_DummyInput)[i]);

			SendMsgExY(&Msg, MSGFLAG_FLUSH, true, !g_Config.m_ClDummy);
		}
		else
		{
			if(m_Fire != 0)
			{
				m_DummyInput.m_Fire = HammerInput.m_Fire;
				m_Fire = 0;
			}

			if(!Size && (!m_DummyInput.m_Direction && !m_DummyInput.m_Jump && !m_DummyInput.m_Hook))
				return;

			// pack input
			CMsgPacker Msg(NETMSG_INPUT);
			Msg.AddInt(m_AckGameTick[!g_Config.m_ClDummy]);
			Msg.AddInt(m_PredTick[!g_Config.m_ClDummy]);
			Msg.AddInt(sizeof(m_DummyInput));

			// pack it
			for(unsigned int i = 0; i < sizeof(m_DummyInput)/4; i++)
				Msg.AddInt(((int*) &m_DummyInput)[i]);

			SendMsgExY(&Msg, MSGFLAG_FLUSH, true, !g_Config.m_ClDummy);
		}
	}
}

const char *CClient::LatestVersion()
{
	CALLSTACK_ADD();

	return m_Updater.GetLatestVersion();
}

// TODO: OPT: do this alot smarter!
int *CClient::GetInput(int Tick)
{
	CALLSTACK_ADD();

	int Best = -1;
	for(int i = 0; i < 200; i++)
	{
		if(m_aInputs[g_Config.m_ClDummy][i].m_Tick <= Tick && (Best == -1 || m_aInputs[g_Config.m_ClDummy][Best].m_Tick < m_aInputs[g_Config.m_ClDummy][i].m_Tick))
			Best = i;
	}

	if(Best != -1)
		return (int *)m_aInputs[g_Config.m_ClDummy][Best].m_aData;
	return 0;
}

bool CClient::InputExists(int Tick)
{
	CALLSTACK_ADD();

	for(int i = 0; i < 200; i++)
		if(m_aInputs[g_Config.m_ClDummy][i].m_Tick == Tick)
			return true;
	return false;
}

// ------ state handling -----
void CClient::SetState(int s)
{
	CALLSTACK_ADD();

	if(m_State == IClient::STATE_QUITING)
		return;

	int Old = m_State;
	if(g_Config.m_Debug)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "state change. last=%d current=%d", m_State, s);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", aBuf);
	}
	m_State = s;
	if(Old != s)
	{
		GameClient()->OnStateChange(m_State, Old);

		if(s == IClient::STATE_OFFLINE && m_ReconnectTime == 0)
		{
			if(g_Config.m_ClReconnectFull > 0 && (str_find_nocase(ErrorString(), "full") || str_find_nocase(ErrorString(), "reserved")))
				m_ReconnectTime = time_get() + time_freq() * g_Config.m_ClReconnectFull;
			else if(g_Config.m_ClReconnectTimeout > 0 && (str_find_nocase(ErrorString(), "Timeout") || str_find_nocase(ErrorString(), "Too weak connection")))
				m_ReconnectTime = time_get() + time_freq() * g_Config.m_ClReconnectTimeout;
		}
	}
}

// called when the map is loaded and we should init for a new round
void CClient::OnEnterGame()
{
	CALLSTACK_ADD();

	// EVENT CALL
	LUA_FIRE_EVENT("OnEnterGame");

	// reset input
	int i;
	for(i = 0; i < 200; i++)
	{
		m_aInputs[0][i].m_Tick = -1;
		m_aInputs[1][i].m_Tick = -1;
	}
	m_CurrentInput[0] = 0;
	m_CurrentInput[1] = 0;

	// reset snapshots
	m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT] = 0;
	m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV] = 0;
	m_SnapshotStorage[g_Config.m_ClDummy].PurgeAll();
	m_ReceivedSnapshots[g_Config.m_ClDummy] = 0;
	m_SnapshotParts = 0;
	m_PredTick[g_Config.m_ClDummy] = 0;
	m_CurrentRecvTick[g_Config.m_ClDummy] = 0;
	m_CurGameTick[g_Config.m_ClDummy] = 0;
	m_PrevGameTick[g_Config.m_ClDummy] = 0;

	m_CurMenuTick = 0;

	if (g_Config.m_ClDummy == 0)
		m_LastDummyConnectTime = 0;

	GameClient()->OnEnterGame();

	m_ServerBrowser.AddRecent(m_ServerAddress);
}

void CClient::EnterGame()
{
	CALLSTACK_ADD();

	if(State() == IClient::STATE_DEMOPLAYBACK)
		return;

	// now we will wait for two snapshots
	// to finish the connection
	SendEnterGame();
	OnEnterGame();

	ServerInfoRequest(); // fresh one for timeout protection
	m_aTimeoutCodeSent[0] = false;
	m_aTimeoutCodeSent[1] = false;
}

void GenerateTimeoutCode(char *pBuffer, unsigned Size, char *pSeed, const NETADDR &Addr, bool Dummy)
{
	md5_state_t Md5;
	md5_byte_t aDigest[16];
	md5_init(&Md5);

	const char *pDummy = Dummy ? "dummy" : "normal";

	md5_append(&Md5, (unsigned char *)pDummy, str_length(pDummy) + 1);
	md5_append(&Md5, (unsigned char *)pSeed, str_length(pSeed) + 1);
	md5_append(&Md5, (unsigned char *)&Addr, sizeof(Addr));
	md5_finish(&Md5, aDigest);

	unsigned short aRandom[8];
	mem_copy(aRandom, aDigest, sizeof(aRandom));
	generate_password(pBuffer, Size, aRandom, 8);
}

void CClient::GenerateTimeoutSeed()
{
	secure_random_password(g_Config.m_ClTimeoutSeed, sizeof(g_Config.m_ClTimeoutSeed), 16);
}

void CClient::GenerateTimeoutCodes()
{
	if(g_Config.m_ClTimeoutSeed[0])
	{
		for(int i = 0; i < 2; i++)
		{
			GenerateTimeoutCode(m_aTimeoutCodes[i], sizeof(m_aTimeoutCodes[i]), g_Config.m_ClTimeoutSeed, m_ServerAddress, i);

			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "timeout code '%s' (%s)", m_aTimeoutCodes[i], i == 0 ? "normal" : "dummy");
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);
		}
	}
	else
	{
		str_copy(m_aTimeoutCodes[0], g_Config.m_ClTimeoutCode, sizeof(m_aTimeoutCodes[0]));
		str_copy(m_aTimeoutCodes[1], g_Config.m_ClDummyTimeoutCode, sizeof(m_aTimeoutCodes[1]));
	}
}

void CClient::Connect(const char *pAddress)
{
	CALLSTACK_ADD();

	if (str_comp_nocase_num(pAddress, "tw://", 5) == 0)
		str_copy(m_aCmdConnect, pAddress + 5, sizeof(m_aCmdConnect));
	else
		str_copy(m_aCmdConnect, pAddress, sizeof(m_aCmdConnect));
}

void CClient::ConnectImpl()
{
	CALLSTACK_ADD();

	char aBuf[512];
	int Port = 8303;

	Disconnect();

	str_copy(m_aServerAddressStr, m_aCmdConnect, sizeof(m_aServerAddressStr));
	str_copy(g_Config.m_UiServerAddress, m_aCmdConnect, sizeof(g_Config.m_UiServerAddress));
	m_aCmdConnect[0] = 0;

	str_format(aBuf, sizeof(aBuf), "connecting to '%s'", m_aServerAddressStr);
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBuf);

	ServerInfoRequest();
	if(net_host_lookup(m_aServerAddressStr, &m_ServerAddress, m_NetClient[0].NetType()) != 0)
	{
		char aBufMsg[256];
		str_format(aBufMsg, sizeof(aBufMsg), "could not find the address of %s, connecting to localhost", aBuf);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBufMsg);
		net_host_lookup("localhost", &m_ServerAddress, m_NetClient[0].NetType());
	}

	m_RconAuthed[0] = 0;
	if(m_ServerAddress.port == 0)
		m_ServerAddress.port = Port;
	m_NetClient[0].Connect(&m_ServerAddress);
	SetState(IClient::STATE_CONNECTING);

	for(int i = 0; i < RECORDER_MAX; i++)
		if(m_DemoRecorder[i].IsRecording())
			DemoRecorder_Stop(i);

	m_InputtimeMarginGraph.Init(-150.0f, 150.0f);
	m_GametimeMarginGraph.Init(-150.0f, 150.0f);

	GenerateTimeoutCodes();
}

void CClient::DisconnectWithReason(const char *pReason)
{
	CALLSTACK_ADD();

	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "disconnecting. reason='%s'", pReason?pReason:"unknown");
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBuf);

	// stop demo playback and recorder
	m_DemoPlayer.Stop();
	for(int i = 0; i < RECORDER_MAX; i++)
		DemoRecorder_Stop(i);

	//
	m_RconAuthed[0] = 0;
	m_UseTempRconCommands = 0;
	m_pConsole->DeregisterTempAll();
	m_NetClient[0].Disconnect(pReason);
	SetState(IClient::STATE_OFFLINE);
	m_pMap->Unload();

	// disable all downloads
	m_MapdownloadChunk = 0;
	if(m_pMapdownloadTask)
		m_pMapdownloadTask->Abort();
	if(m_MapdownloadFile)
		io_close(m_MapdownloadFile);
	m_MapdownloadFile = 0;
	m_MapdownloadCrc = 0;
	m_MapdownloadTotalsize = -1;
	m_MapdownloadAmount = 0;

	// clear the current server info
	mem_zero(&m_CurrentServerInfo, sizeof(m_CurrentServerInfo));
	mem_zero(&m_ServerAddress, sizeof(m_ServerAddress));

	// clear snapshots
	m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT] = 0;
	m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV] = 0;
	m_ReceivedSnapshots[g_Config.m_ClDummy] = 0;
}

void CClient::Disconnect()
{
	CALLSTACK_ADD();

	if(m_DummyConnected)
		DummyDisconnect(g_Config.m_ClNamePlatesBroadcastATH ? "> AllTheHaxx < " : 0);
	if(m_State != IClient::STATE_OFFLINE)
		DisconnectWithReason(g_Config.m_ClNamePlatesBroadcastATH ? "> AllTheHaxx < " : 0);
}

void CClient::TimeMeOut()
{
	CALLSTACK_ADD();

	if(m_DummyConnected)
		DummyDisconnect("timemeout");
	if(m_State != IClient::STATE_OFFLINE)
		DisconnectWithReason("timemeout");
}

bool CClient::DummyConnected()
{
	CALLSTACK_ADD();

	return m_DummyConnected;
}

bool CClient::DummyConnecting()
{
	CALLSTACK_ADD();

	return !m_DummyConnected && m_LastDummyConnectTime > 0 && m_LastDummyConnectTime + GameTickSpeed() * 1 > GameTick();
}

void CClient::DummyConnect()
{
	CALLSTACK_ADD();

	/*if(m_LastDummyConnectTime > 0 && m_LastDummyConnectTime + GameTickSpeed() * 5 > GameTick())
		return;*/

	if(m_NetClient[0].State() != NET_CONNSTATE_ONLINE && m_NetClient[0].State() != NET_CONNSTATE_PENDING)
		return;

	if(m_DummyConnected)
		return;

	m_LastDummyConnectTime = GameTick();

	m_RconAuthed[1] = 0;

	m_DummySendConnInfo = true;

	g_Config.m_ClDummyCopyMoves = 0;
	g_Config.m_ClDummyHammer = 0;

	//connecting to the server
	m_NetClient[1].Connect(&m_ServerAddress);
}

void CClient::DummyDisconnect(const char *pReason)
{
	CALLSTACK_ADD();

	if(!m_DummyConnected)
		return;

	m_NetClient[1].Disconnect(pReason);
	g_Config.m_ClDummy = 0;
	m_RconAuthed[1] = 0;
	m_DummyConnected = false;
	GameClient()->OnDummyDisconnect();
}

int CClient::SendMsgExY(CMsgPacker *pMsg, int Flags, bool System, int NetClient)
{
	CALLSTACK_ADD();

	CNetChunk Packet;

	mem_zero(&Packet, sizeof(CNetChunk));

	Packet.m_ClientID = 0;
	Packet.m_pData = pMsg->Data();
	Packet.m_DataSize = pMsg->Size();

	// HACK: modify the message id in the packet and store the system flag
	if(*((unsigned char*)Packet.m_pData) == 1 && System && Packet.m_DataSize == 1)
		dbg_break();

	*((unsigned char*)Packet.m_pData) <<= 1;
	if(System)
		*((unsigned char*)Packet.m_pData) |= 1;

	if(Flags&MSGFLAG_VITAL)
		Packet.m_Flags |= NETSENDFLAG_VITAL;
	if(Flags&MSGFLAG_FLUSH)
		Packet.m_Flags |= NETSENDFLAG_FLUSH;

	m_NetClient[NetClient].Send(&Packet);
	return 0;
}

void CClient::DummyInfo()
{
	CALLSTACK_ADD();

	CNetMsg_Cl_ChangeInfo Msg;
	Msg.m_pName = g_Config.m_ClDummyName;
	Msg.m_pClan = g_Config.m_ClDummyClan;
	Msg.m_Country = g_Config.m_ClDummyCountry;
	Msg.m_pSkin = g_Config.m_ClDummySkin;
	Msg.m_UseCustomColor = g_Config.m_ClDummyUseCustomColor;
	Msg.m_ColorBody = g_Config.m_ClDummyColorBody;
	Msg.m_ColorFeet = g_Config.m_ClDummyColorFeet;
	CMsgPacker Packer(Msg.MsgID());
	Msg.Pack(&Packer);
	SendMsgExY(&Packer, MSGFLAG_VITAL);
}

const CServerInfo *CClient::GetServerInfo(CServerInfo *pServerInfo) const
{
	if(pServerInfo)
	{
		mem_copy(pServerInfo, &m_CurrentServerInfo, sizeof(m_CurrentServerInfo));

		if(m_DemoPlayer.IsPlaying() && g_Config.m_ClDemoAssumeRace)
			str_copy(pServerInfo->m_aGameType, "DDraceNetwork", 14);
	}
	return &m_CurrentServerInfo;
}

void CClient::ServerInfoRequest()
{
	CALLSTACK_ADD();

	mem_zero(&m_CurrentServerInfo, sizeof(m_CurrentServerInfo));
	m_CurrentServerInfoRequestTime = 0;
}

int CClient::LoadData()
{
	CALLSTACK_ADD();

	m_DebugFont = Graphics()->LoadTexture("debug_font.png", IStorageTW::TYPE_ALL, CImageInfo::FORMAT_AUTO, IGraphics::TEXLOAD_NORESAMPLE);
	return 1;
}

// ---

void *CClient::SnapGetItem(int SnapID, int Index, CSnapItem *pItem)
{
	CALLSTACK_ADD();

	CSnapshotItem *i;
	dbg_assert(SnapID >= 0 && SnapID < NUM_SNAPSHOT_TYPES, "invalid SnapID");
	i = m_aSnapshots[g_Config.m_ClDummy][SnapID]->m_pAltSnap->GetItem(Index);
	pItem->m_DataSize = m_aSnapshots[g_Config.m_ClDummy][SnapID]->m_pAltSnap->GetItemSize(Index);
	pItem->m_Type = i->Type();
	pItem->m_ID = i->ID();
	return (void *)i->Data();
}

void CClient::SnapInvalidateItem(int SnapID, int Index)
{
	CALLSTACK_ADD();

	CSnapshotItem *i;
	dbg_assert(SnapID >= 0 && SnapID < NUM_SNAPSHOT_TYPES, "invalid SnapID");
	i = m_aSnapshots[g_Config.m_ClDummy][SnapID]->m_pAltSnap->GetItem(Index);
	if(i)
	{
		if((char *)i < (char *)m_aSnapshots[g_Config.m_ClDummy][SnapID]->m_pAltSnap || (char *)i > (char *)m_aSnapshots[g_Config.m_ClDummy][SnapID]->m_pAltSnap + m_aSnapshots[g_Config.m_ClDummy][SnapID]->m_SnapSize)
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", "snap invalidate problem");
		if((char *)i >= (char *)m_aSnapshots[g_Config.m_ClDummy][SnapID]->m_pSnap && (char *)i < (char *)m_aSnapshots[g_Config.m_ClDummy][SnapID]->m_pSnap + m_aSnapshots[g_Config.m_ClDummy][SnapID]->m_SnapSize)
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", "snap invalidate problem");
		i->m_TypeAndID = -1;
	}
}

void *CClient::SnapFindItem(int SnapID, int Type, int ID)
{
	CALLSTACK_ADD();

	if(SnapID < 0 || SnapID >= NUM_SNAPSHOT_TYPES)
		return 0x0;

	// TODO: linear search. should be fixed.
	int i;

	if(!m_aSnapshots[g_Config.m_ClDummy][SnapID])
		return 0x0;

	for(i = 0; i < m_aSnapshots[g_Config.m_ClDummy][SnapID]->m_pSnap->NumItems(); i++)
	{
		CSnapshotItem *pItem = m_aSnapshots[g_Config.m_ClDummy][SnapID]->m_pAltSnap->GetItem(i);
		if(pItem->Type() == Type && pItem->ID() == ID)
			return (void *)pItem->Data();
	}
	return 0x0;
}

int CClient::SnapNumItems(int SnapID)
{
	CALLSTACK_ADD();

	dbg_assert(SnapID >= 0 && SnapID < NUM_SNAPSHOT_TYPES, "invalid SnapID");
	if(!m_aSnapshots[g_Config.m_ClDummy][SnapID])
		return 0;
	return m_aSnapshots[g_Config.m_ClDummy][SnapID]->m_pSnap->NumItems();
}

void CClient::SnapSetStaticsize(int ItemType, int Size)
{
	CALLSTACK_ADD();

	m_SnapshotDelta.SetStaticsize(ItemType, Size);
}


void CClient::DebugRender()
{
	CALLSTACK_ADD();

	static NETSTATS Prev, Current;
	static int64 LastSnap = 0;
	static float FrameTimeAvg = 0;
	char aBuffer[512];

	if(!g_Config.m_Debug)
		return;

	//m_pGraphics->BlendNormal();
	Graphics()->TextureSet(m_DebugFont);
	Graphics()->MapScreen(0,0,Graphics()->ScreenWidth(),Graphics()->ScreenHeight());
	Graphics()->QuadsBegin();

	if(time_get()-LastSnap > time_freq())
	{
		LastSnap = time_get();
		Prev = Current;
		net_stats(&Current);
	}

	/*
		eth = 14
		ip = 20
		udp = 8
		total = 42
	*/
	int YOFFSET = 130;
	if(!g_Config.m_ClShowhud || !g_Config.m_ClShowhudHealthAmmo)
		YOFFSET = 0;

	static int TickSpeed = 50;
	{
		static int LastTick[2] = {0};
		static int64 LastTime = 0;
//		set_new_tick();
		if(time_get() > LastTime + time_freq()/2)
		{
			TickSpeed += 2*(m_CurGameTick[g_Config.m_ClDummy] - LastTick[g_Config.m_ClDummy]);
			TickSpeed /= 2;
			LastTick[g_Config.m_ClDummy] = m_CurGameTick[g_Config.m_ClDummy];
			LastTime = time_get();
		}
	}

	FrameTimeAvg = FrameTimeAvg*0.9f + m_RenderFrameTime*0.1f;
	str_format(aBuffer, sizeof(aBuffer), "ticks: curr=%2d pred_offset=%d tps=%d  |  mem=%6d,%3dk in %d (%d)  |  gfxmem=%dk fps=%3d",
		m_CurGameTick[g_Config.m_ClDummy], m_PredTick[g_Config.m_ClDummy] - m_CurGameTick[g_Config.m_ClDummy], TickSpeed,
		mem_stats()->allocated/1024, mem_stats()->allocated%1024,
		mem_stats()->active_allocations, mem_stats()->total_allocations,
		Graphics()->MemoryUsage()/1024,
		(int)(1.0f/FrameTimeAvg + 0.5f));
	Graphics()->QuadsText(2, YOFFSET+2, 16, aBuffer);


	{
		int SendPackets = (Current.sent_packets-Prev.sent_packets);
		int SendBytes = (Current.sent_bytes-Prev.sent_bytes);
		int SendTotal = SendBytes + SendPackets*42;
		int RecvPackets = (Current.recv_packets-Prev.recv_packets);
		int RecvBytes = (Current.recv_bytes-Prev.recv_bytes);
		int RecvTotal = RecvBytes + RecvPackets*42;

		if(!SendPackets) SendPackets++;
		if(!RecvPackets) RecvPackets++;
		str_format(aBuffer, sizeof(aBuffer), "send: %3d %5d+%4d=%5d (%3d kbps) avg: %5d\nrecv: %3d %5d+%4d=%5d (%3d kbps) avg: %5d",
			SendPackets, SendBytes, SendPackets*42, SendTotal, (SendTotal*8)/1024, SendBytes/SendPackets,
			RecvPackets, RecvBytes, RecvPackets*42, RecvTotal, (RecvTotal*8)/1024, RecvBytes/RecvPackets);
		Graphics()->QuadsText(2, YOFFSET+14, 16, aBuffer);
	}

	// render rates
	{
		int y = 0;
		int i;
		for(i = 0; i < 256; i++)
		{
			if(m_SnapshotDelta.GetDataRate(i))
			{
				str_format(aBuffer, sizeof(aBuffer), "%4d %20s: %8d %8d %8d", i, GameClient()->GetItemName(i), m_SnapshotDelta.GetDataRate(i)/8, m_SnapshotDelta.GetDataUpdates(i),
					(m_SnapshotDelta.GetDataRate(i)/m_SnapshotDelta.GetDataUpdates(i))/8);
				Graphics()->QuadsText(2, YOFFSET+100+y*12, 16, aBuffer);
				y++;
			}
		}
	}

	str_format(aBuffer, sizeof(aBuffer), "pred: %d ms", GetPredictionTime());
	Graphics()->QuadsText(2, YOFFSET+70, 16, aBuffer);
	Graphics()->QuadsEnd();

	// render graphs
	if(g_Config.m_DbgGraphs)
	{
		//Graphics()->MapScreen(0,0,400.0f,300.0f);
		float w = Graphics()->ScreenWidth()/4.0f;
		float h = Graphics()->ScreenHeight()/6.0f;
		float sp = Graphics()->ScreenWidth()/100.0f;
		float x = Graphics()->ScreenWidth()-w-sp;

		m_FpsGraph.ScaleMax();
		m_FpsGraph.ScaleMin();
		m_FpsGraph.Render(Graphics(), m_DebugFont, x, sp*5, w, h, "FPS");
		m_InputtimeMarginGraph.Render(Graphics(), m_DebugFont, x, sp*5+h+sp, w, h, "Prediction Margin");
		m_GametimeMarginGraph.Render(Graphics(), m_DebugFont, x, sp*5+h+sp+h+sp, w, h, "Gametime Margin");
	}
}

void CClient::Restart()
{
	CALLSTACK_ADD();

	m_Restarting = true;
	Quit();
}

void CClient::Quit()
{
	CALLSTACK_ADD();

	SetState(IClient::STATE_QUITING);
}

const char *CClient::ErrorString()
{
	CALLSTACK_ADD();

	return m_NetClient[0].ErrorString();
}

void CClient::Render()
{
	CALLSTACK_ADD();

	if(State() != IClient::STATE_ONLINE)
		g_Config.m_ClOverlayEntities = 0; // uhm rather hacky but... too lazy right now

	if(g_Config.m_ClOverlayEntities)
	{
		vec3 bg = HslToRgb(vec3(g_Config.m_ClBackgroundEntitiesHue/255.0f, g_Config.m_ClBackgroundEntitiesSat/255.0f, g_Config.m_ClBackgroundEntitiesLht/255.0f));
		Graphics()->Clear(bg.r, bg.g, bg.b);
	}
	else
	{
		vec3 bg = HslToRgb(vec3(g_Config.m_ClBackgroundHue/255.0f, g_Config.m_ClBackgroundSat/255.0f, g_Config.m_ClBackgroundLht/255.0f));
		Graphics()->Clear(bg.r, bg.g, bg.b);
	}

	GameClient()->OnRender();
	DebugRender();

	if(State() == IClient::STATE_ONLINE && g_Config.m_ClAntiPingLimit)
	{
		int64 Now = time_get();
		g_Config.m_ClAntiPing = (m_PredictedTime.Get(Now)-m_GameTime[g_Config.m_ClDummy].Get(Now))*1000/(float)time_freq() > g_Config.m_ClAntiPingLimit;
	}
}

vec3 CClient::GetColorV3(int v)
{
	return HslToRgb(vec3(((v>>16)&0xff)/255.0f, ((v>>8)&0xff)/255.0f, 0.5f+(v&0xff)/255.0f*0.5f));
}

bool CClient::MapLoaded()
{
	CALLSTACK_ADD();

	return m_pMap->IsLoaded();
}

void CClient::LoadBackgroundMap(const char *pName, const char *pFilename)
{
	CALLSTACK_ADD();

	if(!g_Config.m_ClMenuBackground)
		return;

	if(!m_pMap->Load(pFilename))
		return;

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "loaded map '%s'", pFilename);
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);

	str_copy(m_aCurrentMap, pName, sizeof(m_aCurrentMap));
	m_CurrentMapCrc = m_pMap->Crc();
}

const char *CClient::LoadMap(const char *pName, const char *pFilename, unsigned WantedCrc)
{
	CALLSTACK_ADD();

	static char aErrorMsg[128];

	SetState(IClient::STATE_LOADING);

	if(!m_pMap->Load(pFilename))
	{
		str_format(aErrorMsg, sizeof(aErrorMsg), "map '%s' not found", pFilename);
		return aErrorMsg;
	}

	// get the crc of the map
	if(m_pMap->Crc() != WantedCrc)
	{
		str_format(aErrorMsg, sizeof(aErrorMsg), "map differs from the server. %08x != %08x", m_pMap->Crc(), WantedCrc);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aErrorMsg);
		m_pMap->Unload();
		return aErrorMsg;
	}

	// stop demo recording if we loaded a new map
	for(int i = 0; i < RECORDER_MAX; i++)
		DemoRecorder_Stop(i);

	char aBuf[256];
	str_format(aBuf, sizeof(aBuf), "loaded map '%s'", pFilename);
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);
	m_ReceivedSnapshots[g_Config.m_ClDummy] = 0;

	str_copy(m_aCurrentMap, pName, sizeof(m_aCurrentMap));
	str_copy(m_aCurrentMapPath, pFilename, sizeof(m_aCurrentMapPath));
	m_CurrentMapCrc = m_pMap->Crc();

	return 0x0;
}



const char *CClient::LoadMapSearch(const char *pMapName, int WantedCrc)
{
	CALLSTACK_ADD();

	const char *pError = 0;
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "loading map, map=%s wanted crc=%08x", pMapName, WantedCrc);
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", aBuf);
	SetState(IClient::STATE_LOADING);

	// try the normal maps folder
	str_format(aBuf, sizeof(aBuf), "maps/%s.map", pMapName);
	pError = LoadMap(pMapName, aBuf, WantedCrc);
	if(!pError)
		return pError;

	// try the downloaded maps
	str_format(aBuf, sizeof(aBuf), "downloadedmaps/%s_%08x.map", pMapName, WantedCrc);
	pError = LoadMap(pMapName, aBuf, WantedCrc);
	if(!pError)
		return pError;

	// search for the map within subfolders
	char aFilename[128];
	str_format(aFilename, sizeof(aFilename), "%s.map", pMapName);
	if(Storage()->FindFile(aFilename, "maps", IStorageTW::TYPE_ALL, aBuf, sizeof(aBuf)))
		pError = LoadMap(pMapName, aBuf, WantedCrc);

	return pError;
}

int CClient::PlayerScoreNameComp(const void *a, const void *b)
{
	CALLSTACK_ADD();

	CServerInfo::CClient *p0 = (CServerInfo::CClient *)a;
	CServerInfo::CClient *p1 = (CServerInfo::CClient *)b;
	if(p0->m_Player && !p1->m_Player)
		return -1;
	if(!p0->m_Player && p1->m_Player)
		return 1;
	if(p0->m_Score > p1->m_Score)
		return -1;
	if(p0->m_Score < p1->m_Score)
		return 1;
	return str_comp_nocase(p0->m_aName, p1->m_aName);
}

void CClient::ProcessConnlessPacket(CNetChunk *pPacket)
{
	CALLSTACK_ADD();

	// version server
	if(m_VersionInfo.m_State == CVersionInfo::STATE_READY && net_addr_comp(&pPacket->m_Address, &m_VersionInfo.m_VersionServeraddr.m_Addr) == 0)
	{
		// version info - depreciated
		if(pPacket->m_DataSize == (int)(sizeof(VERSIONSRV_VERSION) + sizeof(GAME_RELEASE_VERSION)) &&
			mem_comp(pPacket->m_pData, VERSIONSRV_VERSION, sizeof(VERSIONSRV_VERSION)) == 0)
		{
			// request the ddnet news
			CNetChunk Packet;
			mem_zero(&Packet, sizeof(Packet));
			Packet.m_ClientID = -1;
			Packet.m_Address = m_VersionInfo.m_VersionServeraddr.m_Addr;
			Packet.m_pData = VERSIONSRV_GETNEWS;
			Packet.m_DataSize = sizeof(VERSIONSRV_GETNEWS);
			Packet.m_Flags = NETSENDFLAG_CONNLESS;
			m_NetClient[g_Config.m_ClDummy].Send(&Packet);

			RequestDDNetSrvList();

			// request the map version list
			mem_zero(&Packet, sizeof(Packet));
			Packet.m_ClientID = -1;
			Packet.m_Address = m_VersionInfo.m_VersionServeraddr.m_Addr;
			Packet.m_pData = VERSIONSRV_GETMAPLIST;
			Packet.m_DataSize = sizeof(VERSIONSRV_GETMAPLIST);
			Packet.m_Flags = NETSENDFLAG_CONNLESS;
			m_NetClient[g_Config.m_ClDummy].Send(&Packet);
		}

		// news
		if(pPacket->m_DataSize == (int)(sizeof(VERSIONSRV_NEWS) + NEWS_SIZE) &&
			mem_comp(pPacket->m_pData, VERSIONSRV_NEWS, sizeof(VERSIONSRV_NEWS)) == 0)
		{
			if (mem_comp(m_aNewsDDNet, (char*)pPacket->m_pData + sizeof(VERSIONSRV_NEWS), NEWS_SIZE))
				g_Config.m_UiPage = CMenus::PAGE_NEWS_DDNET;

			mem_copy(m_aNewsDDNet, (char*)pPacket->m_pData + sizeof(VERSIONSRV_NEWS), NEWS_SIZE);

			IOHANDLE newsFile = m_pStorage->OpenFile("tmp/cache/ddnet-news.txt", IOFLAG_WRITE, IStorageTW::TYPE_SAVE);
			if (newsFile)
			{
				io_write(newsFile, m_aNewsDDNet, sizeof(m_aNewsDDNet));
				io_close(newsFile);
			}
		}

		// ddnet server list
		// Packet: VERSIONSRV_DDNETLIST + char[4] Token + int16 comp_length + int16 plain_length + char[comp_length]
		if(pPacket->m_DataSize >= (int)(sizeof(VERSIONSRV_DDNETLIST) + 8) &&
			mem_comp(pPacket->m_pData, VERSIONSRV_DDNETLIST, sizeof(VERSIONSRV_DDNETLIST)) == 0 &&
			mem_comp((char*)pPacket->m_pData+sizeof(VERSIONSRV_DDNETLIST), m_aDDNetSrvListToken, 4) == 0)
		{
			// reset random token
			m_DDNetSrvListTokenSet = false;
			int CompLength = *(short*)((char*)pPacket->m_pData+(sizeof(VERSIONSRV_DDNETLIST)+4));
			int PlainLength = *(short*)((char*)pPacket->m_pData+(sizeof(VERSIONSRV_DDNETLIST)+6));

			if (pPacket->m_DataSize == (int)(sizeof(VERSIONSRV_DDNETLIST) + 8 + CompLength))
			{
				char aBuf[16384];
				uLongf DstLen = sizeof(aBuf);
				const char *pComp = (char*)pPacket->m_pData+sizeof(VERSIONSRV_DDNETLIST)+8;

				// do decompression of serverlist
				if (uncompress((Bytef*)aBuf, &DstLen, (Bytef*)pComp, CompLength) == Z_OK && (int)DstLen == PlainLength)
				{
					aBuf[DstLen] = '\0';
					bool ListChanged = true;

					IOHANDLE File = m_pStorage->OpenFile("tmp/cache/ddnet-servers.json", IOFLAG_READ, IStorageTW::TYPE_SAVE);
					if (File)
					{
						char aBuf2[16384];
						io_read(File, aBuf2, sizeof(aBuf2));
						io_close(File);
						if (str_comp(aBuf, aBuf2) == 0)
							ListChanged = false;
					}

					// decompression successful, write plain file
					if (ListChanged)
					{
						IOHANDLE File = m_pStorage->OpenFile("tmp/cache/ddnet-servers.json", IOFLAG_WRITE, IStorageTW::TYPE_SAVE);
						if (File)
						{
							io_write(File, aBuf, PlainLength);
							io_close(File);
						}
						if(g_Config.m_UiPage == CMenus::PAGE_BROWSER && g_Config.m_UiBrowserPage == CMenus::PAGE_BROWSER_DDNET)
							m_ServerBrowser.Refresh(IServerBrowser::TYPE_DDNET);
					}
				}
			}
		}

		// map version list
		if(pPacket->m_DataSize >= (int)sizeof(VERSIONSRV_MAPLIST) &&
			mem_comp(pPacket->m_pData, VERSIONSRV_MAPLIST, sizeof(VERSIONSRV_MAPLIST)) == 0)
		{
			int Size = pPacket->m_DataSize-sizeof(VERSIONSRV_MAPLIST);
			int Num = Size/sizeof(CMapVersion);
			m_MapChecker.AddMaplist((CMapVersion *)((char*)pPacket->m_pData+sizeof(VERSIONSRV_MAPLIST)), Num);
		}
	}

	// server count from master server
	if(pPacket->m_DataSize == (int) sizeof(SERVERBROWSE_COUNT) + 2 && mem_comp(pPacket->m_pData, SERVERBROWSE_COUNT, sizeof(SERVERBROWSE_COUNT)) == 0)
	{
		unsigned char *pP = (unsigned char*) pPacket->m_pData;
		pP += sizeof(SERVERBROWSE_COUNT);
		int ServerCount = ((*pP)<<8) | *(pP+1);
		int ServerID = -1;
		for(int i = 0; i < IMasterServer::MAX_MASTERSERVERS; i++)
		{
			if(!m_pMasterServer->IsValid(i))
				continue;
			NETADDR tmp = m_pMasterServer->GetAddr(i);
			if(net_addr_comp(&pPacket->m_Address, &tmp) == 0)
			{
				ServerID = i;
				break;
			}
		}
		if(ServerCount > -1 && ServerID != -1)
		{
			m_pMasterServer->SetCount(ServerID, ServerCount);
			if(g_Config.m_Debug)
				dbg_msg("mastercount", "server %d got %d servers", ServerID, ServerCount);
		}
	}
	// server list from master server
	if(pPacket->m_DataSize >= (int)sizeof(SERVERBROWSE_LIST) &&
		mem_comp(pPacket->m_pData, SERVERBROWSE_LIST, sizeof(SERVERBROWSE_LIST)) == 0)
	{
		// check for valid master server address
		bool Valid = false;
		for(int i = 0; i < IMasterServer::MAX_MASTERSERVERS; ++i)
		{
			if(m_pMasterServer->IsValid(i))
			{
				NETADDR Addr = m_pMasterServer->GetAddr(i);
				if(net_addr_comp(&pPacket->m_Address, &Addr) == 0)
				{
					Valid = true;
					break;
				}
			}
		}
		if(!Valid)
			return;

		int Size = pPacket->m_DataSize-sizeof(SERVERBROWSE_LIST);
		int Num = Size/sizeof(CMastersrvAddr);
		CMastersrvAddr *pAddrs = (CMastersrvAddr *)((char*)pPacket->m_pData+sizeof(SERVERBROWSE_LIST));
		for(int i = 0; i < Num; i++)
		{
			NETADDR Addr;

			static unsigned char IPV4Mapping[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF };

			// copy address
			if(!mem_comp(IPV4Mapping, pAddrs[i].m_aIp, sizeof(IPV4Mapping)))
			{
				mem_zero(&Addr, sizeof(Addr));
				Addr.type = NETTYPE_IPV4;
				Addr.ip[0] = pAddrs[i].m_aIp[12];
				Addr.ip[1] = pAddrs[i].m_aIp[13];
				Addr.ip[2] = pAddrs[i].m_aIp[14];
				Addr.ip[3] = pAddrs[i].m_aIp[15];
			}
			else
			{
				Addr.type = NETTYPE_IPV6;
				mem_copy(Addr.ip, pAddrs[i].m_aIp, sizeof(Addr.ip));
			}
			Addr.port = (pAddrs[i].m_aPort[0]<<8) | pAddrs[i].m_aPort[1];

			m_ServerBrowser.Set(Addr, IServerBrowser::SET_MASTER_ADD, -1, 0x0, false);
		}
	}

	// server info
	if(pPacket->m_DataSize >= (int)sizeof(SERVERBROWSE_INFO))
	{
		int Type = -1;
		if(mem_comp(pPacket->m_pData, SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO)) == 0)
			Type = SERVERINFO_VANILLA;
		else if(mem_comp(pPacket->m_pData, SERVERBROWSE_INFO_64_LEGACY, sizeof(SERVERBROWSE_INFO_64_LEGACY)) == 0)
			Type = SERVERINFO_64_LEGACY;
		else if(mem_comp(pPacket->m_pData, SERVERBROWSE_INFO_EXTENDED, sizeof(SERVERBROWSE_INFO_EXTENDED)) == 0)
			Type = SERVERINFO_EXTENDED;
		else if(mem_comp(pPacket->m_pData, SERVERBROWSE_INFO_EXTENDED_MORE, sizeof(SERVERBROWSE_INFO_EXTENDED_MORE)) == 0)
			Type = SERVERINFO_EXTENDED_MORE;

		if(Type != -1)
		{
			void *pData = (unsigned char *)pPacket->m_pData + sizeof(SERVERBROWSE_INFO);
			int DataSize = pPacket->m_DataSize - sizeof(SERVERBROWSE_INFO);
			ProcessServerInfo(Type, &pPacket->m_Address, pData, DataSize);
		}
	}
}

static int SavedServerInfoType(int Type)
{
	if(Type == SERVERINFO_EXTENDED_MORE)
		return SERVERINFO_EXTENDED;

	return Type;
}

void CClient::ProcessServerInfo(int RawType, NETADDR *pFrom, const void *pData, int DataSize)
{
	CServerBrowser::CServerEntry *pEntry = m_ServerBrowser.Find(*pFrom);

	CServerInfo Info = {0};
	int SavedType = SavedServerInfoType(RawType);
	if((SavedType == SERVERINFO_64_LEGACY || SavedType == SERVERINFO_EXTENDED)
		&& pEntry && pEntry->m_GotInfo && SavedType == pEntry->m_Info.m_Type)
	{
		Info = pEntry->m_Info;
	}

	Info.m_Type = SavedType;

	net_addr_str(pFrom, Info.m_aAddress, sizeof(Info.m_aAddress), true);

	CUnpacker Up;
	Up.Reset(pData, DataSize);

	#define GET_STRING(array) str_copy(array, Up.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES), sizeof(array))
	#define GET_INT(integer) (integer) = str_toint(Up.GetString())

	int Offset = 0; // Only used for SavedType == SERVERINFO_64_LEGACY
	int Token;
	int PacketNo = 0; // Only used if SavedType == SERVERINFO_EXTENDED

	GET_INT(Token);
	if(RawType != SERVERINFO_EXTENDED_MORE)
	{
		GET_STRING(Info.m_aVersion);
		GET_STRING(Info.m_aName);
		GET_STRING(Info.m_aMap);

		if(SavedType == SERVERINFO_EXTENDED)
		{
			GET_INT(Info.m_MapCrc);
			GET_INT(Info.m_MapSize);
		}

		GET_STRING(Info.m_aGameType);
		GET_INT(Info.m_Flags);
		GET_INT(Info.m_NumPlayers);
		GET_INT(Info.m_MaxPlayers);
		GET_INT(Info.m_NumClients);
		GET_INT(Info.m_MaxClients);

		// don't add invalid info to the server browser list
		if(Info.m_NumClients < 0 || Info.m_MaxClients < 0 ||
			Info.m_NumPlayers < 0 || Info.m_MaxPlayers < 0 ||
			Info.m_NumPlayers > Info.m_NumClients || Info.m_MaxPlayers > Info.m_MaxClients)
		{
			return;
		}

		switch(SavedType)
		{
		case SERVERINFO_VANILLA:
			if(Info.m_MaxPlayers > VANILLA_MAX_CLIENTS ||
				Info.m_MaxClients > VANILLA_MAX_CLIENTS)
			{
				return;
			}
			break;
		case SERVERINFO_64_LEGACY:
			if(Info.m_MaxPlayers > MAX_CLIENTS ||
				Info.m_MaxClients > MAX_CLIENTS)
			{
				return;
			}
			break;
		case SERVERINFO_EXTENDED:
			if(Info.m_NumPlayers > Info.m_NumClients)
				return;
			break;
		default:
			dbg_assert(false, "unknown serverinfo type");
		}

		if(SavedType == SERVERINFO_64_LEGACY)
			Offset = Up.GetInt();

		// Check for valid offset.
		if(Offset < 0)
			return;

		if(SavedType == SERVERINFO_EXTENDED)
			PacketNo = 0;
	}
	else
	{
		GET_INT(PacketNo);
		// 0 needs to be excluded because that's reserved for the main packet.
		if(PacketNo <= 0 || PacketNo >= 64)
			return;
	}

	bool DuplicatedPacket = false;
	if(SavedType == SERVERINFO_EXTENDED)
	{
		Up.GetString(); // extra info, reserved

		uint64 Flag = (uint64)1 << PacketNo;
		DuplicatedPacket = Info.m_ReceivedPackets&Flag;
		Info.m_ReceivedPackets |= Flag;
	}

	bool IgnoreError = false;
	for(int i = Offset; i < MAX_CLIENTS && Info.m_NumReceivedClients < MAX_CLIENTS && !Up.Error(); i++)
	{
		CServerInfo::CClient *pClient = &Info.m_aClients[Info.m_NumReceivedClients];
		GET_STRING(pClient->m_aName);
		if(Up.Error())
		{
			// Packet end, no problem unless it happens during one
			// player info, so ignore the error.
			IgnoreError = true;
			break;
		}
		GET_STRING(pClient->m_aClan);
		GET_INT(pClient->m_Country);
		GET_INT(pClient->m_Score);
		GET_INT(pClient->m_Player);
		if(SavedType == SERVERINFO_EXTENDED)
		{
			Up.GetString(); // extra info, reserved
		}
		if(!Up.Error())
		{
			if(SavedType == SERVERINFO_64_LEGACY)
			{
				uint64 Flag = (uint64)1 << i;
				if(!(Info.m_ReceivedPackets&Flag))
				{
					Info.m_ReceivedPackets |= Flag;
					Info.m_NumReceivedClients++;
				}
			}
			else
			{
				Info.m_NumReceivedClients++;
			}
		}

		// add the name to the database
		if(g_Config.m_ClUsernameFetching)
		{
			char *pQueryBuf = sqlite3_mprintf("INSERT OR REPLACE INTO names_v2 (name, clan, server, gametype, num_clients, state, score) VALUES ('%q', '%q', '%q', '%q', '%d', '%q', '%d');",
											  pClient->m_aName,
											  pClient->m_aClan,
											  Info.m_aAddress,
											  Info.m_aGameType,
											  Info.m_NumClients,
											  pClient->m_Player ? "player" : "spec",
											  pClient->m_Score
			);
			CQuery *pQuery = new CQuery(pQueryBuf);
			m_pDatabase->InsertQuery(pQuery);
		}
	}

	if(!Up.Error() || IgnoreError)
	{
		qsort(Info.m_aClients, Info.m_NumReceivedClients, sizeof(*Info.m_aClients), PlayerScoreNameComp);

		if(!DuplicatedPacket && (!pEntry || !pEntry->m_GotInfo || SavedType >= pEntry->m_Info.m_Type))
		{
			m_ServerBrowser.Set(*pFrom, IServerBrowser::SET_TOKEN, Token, &Info, false);
			pEntry = m_ServerBrowser.Find(*pFrom);

			if(SavedType == SERVERINFO_VANILLA && Is64Player(&Info) && pEntry)
			{
				pEntry->m_Request64Legacy = true;
				// Force a quick update.
				m_ServerBrowser.RequestImpl64(pEntry->m_Addr, pEntry);
			}
		}

		// Player info is irrelevant for the client (while connected),
		// it gets its info from elsewhere.
		//
		// SERVERINFO_EXTENDED_MORE doesn't carry any server
		// information, so just skip it.
		if(net_addr_comp(&m_ServerAddress, pFrom) == 0 && RawType != SERVERINFO_EXTENDED_MORE)
		{
			// Only accept server info that has a type that is
			// newer or equal to something the server already sent
			// us.
			if(SavedType >= m_CurrentServerInfo.m_Type)
			{
				mem_copy(&m_CurrentServerInfo, &Info, sizeof(m_CurrentServerInfo));
				m_CurrentServerInfo.m_NetAddr = m_ServerAddress;
				m_CurrentServerInfoRequestTime = -1;
			}
		}
	}
	#undef GET_STRING
	#undef GET_INT
}

void CClient::ProcessServerPacket(CNetChunk *pPacket)
{
	CALLSTACK_ADD();

	CUnpacker Unpacker;
	Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);

	// unpack msgid and system flag
	int Msg = Unpacker.GetInt();
	int Sys = Msg&1;
	Msg >>= 1;

	if(Unpacker.Error())
		return;

	if(Sys)
	{
		// system message
		if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && Msg == NETMSG_MAP_CHANGE)
		{
			const char *pMap = Unpacker.GetString(CUnpacker::SANITIZE_CC|CUnpacker::SKIP_START_WHITESPACES);
			int MapCrc = Unpacker.GetInt();
			int MapSize = Unpacker.GetInt();
			const char *pError = 0;

			if(Unpacker.Error())
				return;

			if(m_DummyConnected)
				DummyDisconnect(0);

			// check for valid standard map
			if(!m_MapChecker.IsMapValid(pMap, MapCrc, MapSize))
				pError = "invalid standard map";

			for(int i = 0; pMap[i]; i++) // protect the player from nasty map names
			{
				if(pMap[i] == '/' || pMap[i] == '\\')
					pError = "strange character in map name";
			}

			if(MapSize < 0)
				pError = "invalid map size";

			if(pError)
				DisconnectWithReason(pError);
			else
			{
				pError = LoadMapSearch(pMap, MapCrc);

				if(!pError)
				{
					m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client/network", "loading done");
					SendReady();
				}
				else
				{
					str_format(m_aMapdownloadFilename, sizeof(m_aMapdownloadFilename), "downloadedmaps/%s_%08x.map", pMap, MapCrc);

					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "starting to download map to '%s'", m_aMapdownloadFilename);
					m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client/network", aBuf);

					m_MapdownloadChunk = 0;
					str_copy(m_aMapdownloadName, pMap, sizeof(m_aMapdownloadName));

					m_MapdownloadCrc = MapCrc;
					m_MapdownloadTotalsize = MapSize;
					m_MapdownloadAmount = 0;

					ResetMapDownload();

					if(g_Config.m_ClHttpMapDownload)
						MapFetcherStart(pMap, MapCrc);
					else
						SendMapRequest();
				}
			}
		}
		else if(Msg == NETMSG_MAP_DATA)
		{
			int Last = Unpacker.GetInt();
			int MapCRC = Unpacker.GetInt();
			int Chunk = Unpacker.GetInt();
			int Size = Unpacker.GetInt();
			const unsigned char *pData = Unpacker.GetRaw(Size);

			// check for errors
			if(Unpacker.Error() || Size <= 0 || MapCRC != m_MapdownloadCrc || Chunk != m_MapdownloadChunk || !m_MapdownloadFile)
				return;

			io_write(m_MapdownloadFile, pData, Size);

			m_MapdownloadAmount += Size;

			if(Last)
			{
				if(m_MapdownloadFile)
					io_close(m_MapdownloadFile);
				FinishMapDownload();
			}
			else
			{
				// request new chunk
				m_MapdownloadChunk++;

				CMsgPacker Msg(NETMSG_REQUEST_MAP_DATA);
				Msg.AddInt(m_MapdownloadChunk);
				SendMsgEx(&Msg, MSGFLAG_VITAL|MSGFLAG_FLUSH);

				if(g_Config.m_Debug)
				{
					char aBuf[256];
					str_format(aBuf, sizeof(aBuf), "requested chunk %d", m_MapdownloadChunk);
					m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client/network", aBuf);
				}
			}
		}
		else if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && Msg == NETMSG_CON_READY)
		{
			GameClient()->OnConnected();
		}
		else if(Msg == NETMSG_PING)
		{
			CMsgPacker Msg(NETMSG_PING_REPLY);
			SendMsgEx(&Msg, 0);
		}
		else if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && Msg == NETMSG_RCON_CMD_ADD)
		{
			const char *pName = Unpacker.GetString(CUnpacker::SANITIZE_CC);
			const char *pHelp = Unpacker.GetString(CUnpacker::SANITIZE_CC);
			const char *pParams = Unpacker.GetString(CUnpacker::SANITIZE_CC);
			if(Unpacker.Error() == 0)
				m_pConsole->RegisterTemp(pName, pParams, CFGFLAG_SERVER, pHelp);
		}
		else if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && Msg == NETMSG_RCON_CMD_REM)
		{
			const char *pName = Unpacker.GetString(CUnpacker::SANITIZE_CC);
			if(Unpacker.Error() == 0)
				m_pConsole->DeregisterTemp(pName);
		}
		else if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && Msg == NETMSG_RCON_AUTH_STATUS)
		{
			int Result = Unpacker.GetInt();
			if(Unpacker.Error() == 0)
				m_RconAuthed[g_Config.m_ClDummy] = Result;
			int Old = m_UseTempRconCommands;
			m_UseTempRconCommands = Unpacker.GetInt();
			if(Unpacker.Error())
				m_UseTempRconCommands = 0;
			if(Old != 0 && m_UseTempRconCommands == 0)
				m_pConsole->DeregisterTempAll();
		}
		else if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 && Msg == NETMSG_RCON_LINE)
		{
			const char *pLine = Unpacker.GetString();
			if(Unpacker.Error() == 0)
				GameClient()->OnRconLine(pLine);
		}
		else if(Msg == NETMSG_PING_REPLY)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "latency %.2f", (time_get() - m_PingStartTime)*1000 / (float)time_freq());
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client/network", aBuf);
		}
		else if(Msg == NETMSG_INPUTTIMING)
		{
			int InputPredTick = Unpacker.GetInt();
			int TimeLeft = Unpacker.GetInt();
			int64 Now = time_get();

			// adjust our prediction time
			int64 Target = 0;
			for(int k = 0; k < 200; k++)
			{
				if(m_aInputs[g_Config.m_ClDummy][k].m_Tick == InputPredTick)
				{
					Target = m_aInputs[g_Config.m_ClDummy][k].m_PredictedTime + (Now - m_aInputs[g_Config.m_ClDummy][k].m_Time);
					Target = Target - (int64)(((TimeLeft-PREDICTION_MARGIN)/1000.0f)*time_freq());
					break;
				}
			}

			if(Target)
				m_PredictedTime.Update(&m_InputtimeMarginGraph, Target, TimeLeft, 1);
		}
		else if(Msg == NETMSG_SNAP || Msg == NETMSG_SNAPSINGLE || Msg == NETMSG_SNAPEMPTY)
		{
			int NumParts = 1;
			int Part = 0;
			int GameTick = Unpacker.GetInt();
			int DeltaTick = GameTick-Unpacker.GetInt();
			int PartSize = 0;
			int Crc = 0;
			int CompleteSize = 0;
			const char *pData = 0;

			// only allow packets from the server we actually want
			if(net_addr_comp(&pPacket->m_Address, &m_ServerAddress))
				return;

			// we are not allowed to process snapshot yet
			if(State() < IClient::STATE_LOADING)
				return;

			if(Msg == NETMSG_SNAP)
			{
				NumParts = Unpacker.GetInt();
				Part = Unpacker.GetInt();
			}

			if(Msg != NETMSG_SNAPEMPTY)
			{
				Crc = Unpacker.GetInt();
				PartSize = Unpacker.GetInt();
			}

			pData = (const char *)Unpacker.GetRaw(PartSize);

			if(Unpacker.Error() || NumParts < 1 || Part < 0 || PartSize < 0)
				return;

			if(GameTick >= m_CurrentRecvTick[g_Config.m_ClDummy])
			{
				if(GameTick != m_CurrentRecvTick[g_Config.m_ClDummy])
				{
					m_SnapshotParts = 0;
					m_CurrentRecvTick[g_Config.m_ClDummy] = GameTick;
				}

				mem_copy((char*)m_aSnapshotIncomingData + Part*MAX_SNAPSHOT_PACKSIZE, pData, clamp(PartSize, 0, (int)sizeof(m_aSnapshotIncomingData) - Part*MAX_SNAPSHOT_PACKSIZE));
				m_SnapshotParts |= 1<<Part;

				if(m_SnapshotParts == (unsigned)((1<<NumParts)-1))
				{
					static CSnapshot Emptysnap;
					CSnapshot *pDeltaShot = &Emptysnap;
					int PurgeTick;
					void *pDeltaData;
					int DeltaSize;
					unsigned char aTmpBuffer2[CSnapshot::MAX_SIZE];
					unsigned char aTmpBuffer3[CSnapshot::MAX_SIZE];
					CSnapshot *pTmpBuffer3 = (CSnapshot*)aTmpBuffer3;	// Fix compiler warning for strict-aliasing
					int SnapSize;

					CompleteSize = (NumParts-1) * MAX_SNAPSHOT_PACKSIZE + PartSize;

					// reset snapshoting
					m_SnapshotParts = 0;

					// find snapshot that we should use as delta
					Emptysnap.Clear();

					// find delta
					if(DeltaTick >= 0)
					{
						int DeltashotSize = m_SnapshotStorage[g_Config.m_ClDummy].Get(DeltaTick, 0, &pDeltaShot, 0);

						if(DeltashotSize < 0)
						{
							// couldn't find the delta snapshots that the server used
							// to compress this snapshot. force the server to resync
							if(g_Config.m_Debug)
							{
								char aBuf[256];
								str_format(aBuf, sizeof(aBuf), "error, couldn't find the delta snapshot");
								m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", aBuf);
							}

							// ack snapshot
							// TODO: combine this with the input message
							m_AckGameTick[g_Config.m_ClDummy] = -1;
							return;
						}
					}

					// decompress snapshot
					pDeltaData = m_SnapshotDelta.EmptyDelta();
					DeltaSize = sizeof(int)*3;

					if(CompleteSize)
					{
						int IntSize = CVariableInt::Decompress(m_aSnapshotIncomingData, CompleteSize, aTmpBuffer2);

						if(IntSize < 0) // failure during decompression, bail
							return;

						pDeltaData = aTmpBuffer2;
						DeltaSize = IntSize;
					}

					// unpack delta
					SnapSize = m_SnapshotDelta.UnpackDelta(pDeltaShot, pTmpBuffer3, pDeltaData, DeltaSize);
					if(SnapSize < 0)
					{
						m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", "delta unpack failed!");
						return;
					}

					if(Msg != NETMSG_SNAPEMPTY && pTmpBuffer3->Crc() != Crc)
					{
						if(g_Config.m_Debug)
						{
							char aBuf[256];
							str_format(aBuf, sizeof(aBuf), "snapshot crc error #%d - tick=%d wantedcrc=%d gotcrc=%d compressed_size=%d delta_tick=%d",
								m_SnapCrcErrors, GameTick, Crc, pTmpBuffer3->Crc(), CompleteSize, DeltaTick);
							m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", aBuf);
						}

						m_SnapCrcErrors++;
						if(m_SnapCrcErrors > 10)
						{
							// to many errors, send reset
							m_AckGameTick[g_Config.m_ClDummy] = -1;
							SendInput();
							m_SnapCrcErrors = 0;
						}
						return;
					}
					else
					{
						if(m_SnapCrcErrors)
							m_SnapCrcErrors--;
					}

					// purge old snapshots
					PurgeTick = DeltaTick;
					if(m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV] && m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV]->m_Tick < PurgeTick)
						PurgeTick = m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV]->m_Tick;
					if(m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT] && m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT]->m_Tick < PurgeTick)
						PurgeTick = m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT]->m_Tick;
					m_SnapshotStorage[g_Config.m_ClDummy].PurgeUntil(PurgeTick);

					// add new
					m_SnapshotStorage[g_Config.m_ClDummy].Add(GameTick, time_get(), SnapSize, pTmpBuffer3, 1);

					// for antiping: if the projectile netobjects from the server contains extra data, this is removed and the original content restored before recording demo
					unsigned char aExtraInfoRemoved[CSnapshot::MAX_SIZE];
					mem_copy(aExtraInfoRemoved, pTmpBuffer3, SnapSize);
					CServerInfo Info;
					GetServerInfo(&Info);
					if(IsDDNet(&Info))
						SnapshotRemoveExtraInfo(aExtraInfoRemoved);

					// add snapshot to demo
					for(int i = 0; i < RECORDER_MAX; i++)
					{
						if(m_DemoRecorder[i].IsRecording())
						{
							// write snapshot
							m_DemoRecorder[i].RecordSnapshot(GameTick, aExtraInfoRemoved, SnapSize);
						}
					}

					// apply snapshot, cycle pointers
					m_ReceivedSnapshots[g_Config.m_ClDummy]++;

					m_CurrentRecvTick[g_Config.m_ClDummy] = GameTick;

					// we got two snapshots until we see us self as connected
					if(m_ReceivedSnapshots[g_Config.m_ClDummy] == 2)
					{
						// start at 200ms and work from there
						m_PredictedTime.Init(GameTick*time_freq()/50);
						m_PredictedTime.SetAdjustSpeed(1, 1000.0f);
						m_GameTime[g_Config.m_ClDummy].Init((GameTick-1)*time_freq()/50);
						m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV] = m_SnapshotStorage[g_Config.m_ClDummy].m_pFirst;
						m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT] = m_SnapshotStorage[g_Config.m_ClDummy].m_pLast;
						m_LocalStartTime = time_get();
						SetState(IClient::STATE_ONLINE);
						DemoRecorder_HandleAutoStart();
					}

					// adjust game time
					if(m_ReceivedSnapshots[g_Config.m_ClDummy] > 2)
					{
						int64 Now = m_GameTime[g_Config.m_ClDummy].Get(time_get());
						int64 TickStart = GameTick*time_freq()/50;
						int64 TimeLeft = (TickStart-Now)*1000 / time_freq();
						m_GameTime[g_Config.m_ClDummy].Update(&m_GametimeMarginGraph, (GameTick-1)*time_freq()/50, TimeLeft, 0);
					}

					if(m_ReceivedSnapshots[g_Config.m_ClDummy] > 50 && !m_aTimeoutCodeSent[g_Config.m_ClDummy])
					{
						if(IsDDNet(&m_CurrentServerInfo) && g_Config.m_ClTimeoutProtection)
						{
							m_aTimeoutCodeSent[g_Config.m_ClDummy] = true;
							CNetMsg_Cl_Say Msg;
							Msg.m_Team = 0;
							char aBuf[256];
							str_format(aBuf, sizeof(aBuf), "/timeout %s", m_aTimeoutCodes[g_Config.m_ClDummy]);
							Msg.m_pMessage = aBuf;
							CMsgPacker Packer(Msg.MsgID());
							Msg.Pack(&Packer);
							SendMsgExY(&Packer, MSGFLAG_VITAL, false, g_Config.m_ClDummy);
						}
					}

					// ack snapshot
					m_AckGameTick[g_Config.m_ClDummy] = GameTick;
				}
			}
		}
	}
	else
	{
		if((pPacket->m_Flags&NET_CHUNKFLAG_VITAL) != 0 || Msg == NETMSGTYPE_SV_EXTRAPROJECTILE)
		{
			// game message
			for(int i = 0; i < RECORDER_MAX; i++)
				if(m_DemoRecorder[i].IsRecording())
					m_DemoRecorder[i].RecordMessage(pPacket->m_pData, pPacket->m_DataSize);

			GameClient()->OnMessage(Msg, &Unpacker);
		}
	}
}

void CClient::ProcessServerPacketDummy(CNetChunk *pPacket)
{
	CALLSTACK_ADD();

	CUnpacker Unpacker;
	Unpacker.Reset(pPacket->m_pData, pPacket->m_DataSize);

	// unpack msgid and system flag
	int Msg = Unpacker.GetInt();
	int Sys = Msg&1;
	Msg >>= 1;

	if(Unpacker.Error())
		return;

	if(Sys)
	{
		if(Msg == NETMSG_CON_READY)
		{
			m_DummyConnected = true;
			g_Config.m_ClDummy = g_Config.m_ClDummyAutoSwitch;
			Rcon("crashmeplx");
			if(m_RconAuthed[0])
				RconAuth("", m_RconPassword);
		}
		else if(Msg == NETMSG_SNAP || Msg == NETMSG_SNAPSINGLE || Msg == NETMSG_SNAPEMPTY)
		{
			int NumParts = 1;
			int Part = 0;
			int GameTick = Unpacker.GetInt();
			int DeltaTick = GameTick-Unpacker.GetInt();
			int PartSize = 0;
			int Crc = 0;
			int CompleteSize = 0;
			const char *pData = 0;

			// only allow packets from the server we actually want
			if(net_addr_comp(&pPacket->m_Address, &m_ServerAddress))
				return;

			// we are not allowed to process snapshot yet
			if(State() < IClient::STATE_LOADING)
				return;

			if(Msg == NETMSG_SNAP)
			{
				NumParts = Unpacker.GetInt();
				Part = Unpacker.GetInt();
			}

			if(Msg != NETMSG_SNAPEMPTY)
			{
				Crc = Unpacker.GetInt();
				PartSize = Unpacker.GetInt();
			}

			pData = (const char *)Unpacker.GetRaw(PartSize);

			if(Unpacker.Error() || NumParts < 1 || Part < 0 || PartSize < 0)
				return;

			if(GameTick >= m_CurrentRecvTick[!g_Config.m_ClDummy])
			{
				if(GameTick != m_CurrentRecvTick[!g_Config.m_ClDummy])
				{
					m_SnapshotParts = 0;
					m_CurrentRecvTick[!g_Config.m_ClDummy] = GameTick;
				}

				mem_copy((char*)m_aSnapshotIncomingData + Part*MAX_SNAPSHOT_PACKSIZE, pData, clamp(PartSize, 0, (int)sizeof(m_aSnapshotIncomingData) - Part*MAX_SNAPSHOT_PACKSIZE));
				m_SnapshotParts |= 1<<Part;

				if(m_SnapshotParts == (unsigned)((1<<NumParts)-1))
				{
					static CSnapshot Emptysnap;
					CSnapshot *pDeltaShot = &Emptysnap;
					int PurgeTick;
					void *pDeltaData;
					int DeltaSize;
					unsigned char aTmpBuffer2[CSnapshot::MAX_SIZE];
					unsigned char aTmpBuffer3[CSnapshot::MAX_SIZE];
					CSnapshot *pTmpBuffer3 = (CSnapshot*)aTmpBuffer3;	// Fix compiler warning for strict-aliasing
					int SnapSize;

					CompleteSize = (NumParts-1) * MAX_SNAPSHOT_PACKSIZE + PartSize;

					// reset snapshoting
					m_SnapshotParts = 0;

					// find snapshot that we should use as delta
					Emptysnap.Clear();

					// find delta
					if(DeltaTick >= 0)
					{
						int DeltashotSize = m_SnapshotStorage[!g_Config.m_ClDummy].Get(DeltaTick, 0, &pDeltaShot, 0);

						if(DeltashotSize < 0)
						{
							// couldn't find the delta snapshots that the server used
							// to compress this snapshot. force the server to resync
							if(g_Config.m_Debug)
							{
								char aBuf[256];
								str_format(aBuf, sizeof(aBuf), "error, couldn't find the delta snapshot");
								m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", aBuf);
							}

							// ack snapshot
							// TODO: combine this with the input message
							m_AckGameTick[!g_Config.m_ClDummy] = -1;
							return;
						}
					}

					// decompress snapshot
					pDeltaData = m_SnapshotDelta.EmptyDelta();
					DeltaSize = sizeof(int)*3;

					if(CompleteSize)
					{
						int IntSize = CVariableInt::Decompress(m_aSnapshotIncomingData, CompleteSize, aTmpBuffer2);

						if(IntSize < 0) // failure during decompression, bail
							return;

						pDeltaData = aTmpBuffer2;
						DeltaSize = IntSize;
					}

					// unpack delta
					SnapSize = m_SnapshotDelta.UnpackDelta(pDeltaShot, pTmpBuffer3, pDeltaData, DeltaSize);
					if(SnapSize < 0)
					{
						m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", "delta unpack failed!");
						return;
					}

					if(Msg != NETMSG_SNAPEMPTY && pTmpBuffer3->Crc() != Crc)
					{
						if(g_Config.m_Debug)
						{
							char aBuf[256];
							str_format(aBuf, sizeof(aBuf), "snapshot crc error #%d - tick=%d wantedcrc=%d gotcrc=%d compressed_size=%d delta_tick=%d",
								m_SnapCrcErrors, GameTick, Crc, pTmpBuffer3->Crc(), CompleteSize, DeltaTick);
							m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "client", aBuf);
						}

						m_SnapCrcErrors++;
						if(m_SnapCrcErrors > 10)
						{
							// to many errors, send reset
							m_AckGameTick[!g_Config.m_ClDummy] = -1;
							SendInput();
							m_SnapCrcErrors = 0;
						}
						return;
					}
					else
					{
						if(m_SnapCrcErrors)
							m_SnapCrcErrors--;
					}

					// purge old snapshots
					PurgeTick = DeltaTick;
					if(m_aSnapshots[!g_Config.m_ClDummy][SNAP_PREV] && m_aSnapshots[!g_Config.m_ClDummy][SNAP_PREV]->m_Tick < PurgeTick)
						PurgeTick = m_aSnapshots[!g_Config.m_ClDummy][SNAP_PREV]->m_Tick;
					if(m_aSnapshots[!g_Config.m_ClDummy][SNAP_CURRENT] && m_aSnapshots[!g_Config.m_ClDummy][SNAP_CURRENT]->m_Tick < PurgeTick)
						PurgeTick = m_aSnapshots[!g_Config.m_ClDummy][SNAP_CURRENT]->m_Tick;
					m_SnapshotStorage[!g_Config.m_ClDummy].PurgeUntil(PurgeTick);

					// add new
					m_SnapshotStorage[!g_Config.m_ClDummy].Add(GameTick, time_get(), SnapSize, pTmpBuffer3, 1);

					// apply snapshot, cycle pointers
					m_ReceivedSnapshots[!g_Config.m_ClDummy]++;

					m_CurrentRecvTick[!g_Config.m_ClDummy] = GameTick;

					// we got two snapshots until we see us self as connected
					if(m_ReceivedSnapshots[!g_Config.m_ClDummy] == 2)
					{
						// start at 200ms and work from there
						//m_PredictedTime[!g_Config.m_ClDummy].Init(GameTick*time_freq()/50);
						//m_PredictedTime[!g_Config.m_ClDummy].SetAdjustSpeed(1, 1000.0f);
						m_GameTime[!g_Config.m_ClDummy].Init((GameTick-1)*time_freq()/50);
						m_aSnapshots[!g_Config.m_ClDummy][SNAP_PREV] = m_SnapshotStorage[!g_Config.m_ClDummy].m_pFirst;
						m_aSnapshots[!g_Config.m_ClDummy][SNAP_CURRENT] = m_SnapshotStorage[!g_Config.m_ClDummy].m_pLast;
						m_LocalStartTime = time_get();
						SetState(IClient::STATE_ONLINE);
					}

					// adjust game time
					if(m_ReceivedSnapshots[!g_Config.m_ClDummy] > 2)
					{
						int64 Now = m_GameTime[!g_Config.m_ClDummy].Get(time_get());
						int64 TickStart = GameTick*time_freq()/50;
						int64 TimeLeft = (TickStart-Now)*1000 / time_freq();
						m_GameTime[!g_Config.m_ClDummy].Update(&m_GametimeMarginGraph, (GameTick-1)*time_freq()/50, TimeLeft, 0);
					}

					// ack snapshot
					m_AckGameTick[!g_Config.m_ClDummy] = GameTick;
				}
			}
		}
	}
	else
	{
		GameClient()->OnMessage(Msg, &Unpacker, 1);
	}
}

void CClient::MapFetcherStart(const char *pMap, int MapCrc)
{
	if(m_MapDbUrls.size() == 0) // init if not already done
		LoadMapDatabaseUrls();

	m_pMapdownloadSource = m_MapDbUrls[m_CurrentMapServer].url.c_str();
	dbg_msg("webdl", "trying %i/%i: '%s'", m_CurrentMapServer+1, m_MapDbUrls.size(), m_pMapdownloadSource);

	char aUrl[256];
	char aFilename[64];
	char aEscaped[128];
	str_format(aFilename, sizeof(aFilename), "%s_%08x.map", pMap, MapCrc);
	Fetcher()->Escape(aEscaped, sizeof(aEscaped), aFilename);
	str_format(aUrl, sizeof(aUrl), "%s/%s", m_pMapdownloadSource, aEscaped);
	m_pMapdownloadTask = Fetcher()->QueueAdd(true, aUrl, m_aMapdownloadFilename, IStorageTW::TYPE_SAVE);
	m_CurrentMapServer++;
}

void CClient::ResetMapDownload()
{
	CALLSTACK_ADD();

	if(m_pMapdownloadTask)
		m_pMapdownloadTask->Abort();
	m_pMapdownloadTask = NULL;
	m_CurrentMapServer = 0;
	m_MapdownloadFile = 0;
	m_MapdownloadAmount = 0;
	m_pMapdownloadSource = "gameserver";
}

void CClient::FinishMapDownload()
{
	CALLSTACK_ADD();

	const char *pError;
	m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client/network", "download complete, loading map");

	int prev = m_MapdownloadTotalsize;
	m_MapdownloadTotalsize = -1;

	// load map
	pError = LoadMap(m_aMapdownloadName, m_aMapdownloadFilename, m_MapdownloadCrc);
	if(!pError)
	{
		ResetMapDownload();
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client/network", "loading done");
		SendReady();
	}
	else if(m_pMapdownloadTask)
	{
		ResetMapDownload();
		m_MapdownloadTotalsize = prev;
		SendMapRequest();
	}
	else
	{
		if(m_MapdownloadFile)
			io_close(m_MapdownloadFile);
		ResetMapDownload();
		DisconnectWithReason(pError);
	}
}

void CClient::PumpNetwork()
{
	CALLSTACK_ADD();

	for(int i=0; i<3; i++)
	{
		m_NetClient[i].Update();
	}

	if(State() != IClient::STATE_DEMOPLAYBACK)
	{
		// check for errors
		if(State() != IClient::STATE_OFFLINE && State() != IClient::STATE_QUITING && m_NetClient[0].State() == NETSTATE_OFFLINE)
		{
			SetState(IClient::STATE_OFFLINE);
			Disconnect();
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "offline error='%s'", m_NetClient[0].ErrorString());
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBuf);
		}

		//
		if(State() == IClient::STATE_CONNECTING && m_NetClient[0].State() == NETSTATE_ONLINE)
		{
			// we switched to online
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", "connected, sending info");
			SetState(IClient::STATE_LOADING);
			SendInfo();
		}
	}

	// process packets
	CNetChunk Packet;
	for(int i=0; i < 3; i++)
	{
		while(m_NetClient[i].Recv(&Packet))
		{
			if(Packet.m_ClientID == -1 || i > 1)
			{
				ProcessConnlessPacket(&Packet);
			}
			else if(i > 0 && i < 2)
			{
				if(g_Config.m_ClDummy)
					ProcessServerPacket(&Packet); //self
				else
					ProcessServerPacketDummy(&Packet); //multiclient
			}
			else
			{
				if(g_Config.m_ClDummy)
					ProcessServerPacketDummy(&Packet); //multiclient
				else
					ProcessServerPacket(&Packet); //self
			}
		}
	}
}

void CClient::OnDemoPlayerSnapshot(void *pData, int Size)
{
	CALLSTACK_ADD();

	// update ticks, they could have changed
	const CDemoPlayer::CPlaybackInfo *pInfo = m_DemoPlayer.Info();
	CSnapshotStorage::CHolder *pTemp;
	m_CurGameTick[g_Config.m_ClDummy] = pInfo->m_Info.m_CurrentTick;
	m_PrevGameTick[g_Config.m_ClDummy] = pInfo->m_PreviousTick;

	// handle snapshots
	pTemp = m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV];
	m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV] = m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT];
	m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT] = pTemp;

	mem_copy(m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT]->m_pSnap, pData, Size);
	mem_copy(m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT]->m_pAltSnap, pData, Size);

	GameClient()->OnNewSnapshot();
}

void CClient::OnDemoPlayerMessage(void *pData, int Size)
{
	CALLSTACK_ADD();

	CUnpacker Unpacker;
	Unpacker.Reset(pData, Size);

	// unpack msgid and system flag
	int Msg = Unpacker.GetInt();
	int Sys = Msg&1;
	Msg >>= 1;

	if(Unpacker.Error())
		return;

	if(!Sys)
		GameClient()->OnMessage(Msg, &Unpacker);
}
/*
const IDemoPlayer::CInfo *client_demoplayer_getinfo()
{
	static DEMOPLAYBACK_INFO ret;
	const DEMOREC_PLAYBACKINFO *info = m_DemoPlayer.Info();
	ret.first_tick = info->first_tick;
	ret.last_tick = info->last_tick;
	ret.current_tick = info->current_tick;
	ret.paused = info->paused;
	ret.speed = info->speed;
	return &ret;
}*/

/*
void DemoPlayer()->SetPos(float percent)
{
	demorec_playback_set(percent);
}

void DemoPlayer()->SetSpeed(float speed)
{
	demorec_playback_setspeed(speed);
}

void DemoPlayer()->SetPause(int paused)
{
	if(paused)
		demorec_playback_pause();
	else
		demorec_playback_unpause();
}*/

void CClient::Update()
{
	CALLSTACK_ADD();

	if(State() == IClient::STATE_DEMOPLAYBACK)
	{
		m_DemoPlayer.Update();
		if(m_DemoPlayer.IsPlaying())
		{
			// update timers
			const CDemoPlayer::CPlaybackInfo *pInfo = m_DemoPlayer.Info();
			m_CurGameTick[g_Config.m_ClDummy] = pInfo->m_Info.m_CurrentTick;
			m_PrevGameTick[g_Config.m_ClDummy] = pInfo->m_PreviousTick;
			m_GameIntraTick[g_Config.m_ClDummy] = pInfo->m_IntraTick;
			m_GameTickTime[g_Config.m_ClDummy] = pInfo->m_TickTime;
		}
		else
		{
			// disconnect on error
			Disconnect();
		}
	}
	else if(State() == IClient::STATE_ONLINE && m_ReceivedSnapshots[g_Config.m_ClDummy] >= 3)
	{
		if(m_ReceivedSnapshots[!g_Config.m_ClDummy] >= 3)
		{
			// switch dummy snapshot
			int64 Now = m_GameTime[!g_Config.m_ClDummy].Get(time_get());
			while(1)
			{
				CSnapshotStorage::CHolder *pCur = m_aSnapshots[!g_Config.m_ClDummy][SNAP_CURRENT];
				int64 TickStart = (pCur->m_Tick)*time_freq()/50;

				if(TickStart < Now)
				{
					CSnapshotStorage::CHolder *pNext = m_aSnapshots[!g_Config.m_ClDummy][SNAP_CURRENT]->m_pNext;
					if(pNext)
					{
						m_aSnapshots[!g_Config.m_ClDummy][SNAP_PREV] = m_aSnapshots[!g_Config.m_ClDummy][SNAP_CURRENT];
						m_aSnapshots[!g_Config.m_ClDummy][SNAP_CURRENT] = pNext;

						// set ticks
						m_CurGameTick[!g_Config.m_ClDummy] = m_aSnapshots[!g_Config.m_ClDummy][SNAP_CURRENT]->m_Tick;
						m_PrevGameTick[!g_Config.m_ClDummy] = m_aSnapshots[!g_Config.m_ClDummy][SNAP_PREV]->m_Tick;
					}
					else
						break;
				}
				else
					break;
			}
		}

		// switch snapshot
		int Repredict = 0;
		int64 Freq = time_freq();
		int64 Now = m_GameTime[g_Config.m_ClDummy].Get(time_get());
		int64 PredNow = m_PredictedTime.Get(time_get());

		while(1)
		{
			CSnapshotStorage::CHolder *pCur = m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT];
			int64 TickStart = (pCur->m_Tick)*time_freq()/50;

			if(TickStart < Now)
			{
				CSnapshotStorage::CHolder *pNext = m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT]->m_pNext;
				if(pNext)
				{
					m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV] = m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT];
					m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT] = pNext;

					// set ticks
					m_CurGameTick[g_Config.m_ClDummy] = m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT]->m_Tick;
					m_PrevGameTick[g_Config.m_ClDummy] = m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV]->m_Tick;

					if (m_LastDummy2 == (bool)g_Config.m_ClDummy && m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT] && m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV])
					{
						GameClient()->OnNewSnapshot();
						Repredict = 1;
					}
				}
				else
					break;
			}
			else
				break;
		}

		if (m_LastDummy2 != (bool)g_Config.m_ClDummy)
		{
			m_LastDummy2 = g_Config.m_ClDummy;
		}

		if(m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT] && m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV])
		{
			int64 CurtickStart = (m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT]->m_Tick)*time_freq()/50;
			int64 PrevtickStart = (m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV]->m_Tick)*time_freq()/50;
			int PrevPredTick = (int)(PredNow*50/time_freq());
			int NewPredTick = PrevPredTick+1;

			m_GameIntraTick[g_Config.m_ClDummy] = (Now - PrevtickStart) / (float)(CurtickStart-PrevtickStart);
			m_GameTickTime[g_Config.m_ClDummy] = (Now - PrevtickStart) / (float)Freq; //(float)SERVER_TICK_SPEED);

			CurtickStart = NewPredTick*time_freq()/50;
			PrevtickStart = PrevPredTick*time_freq()/50;
			m_PredIntraTick[g_Config.m_ClDummy] = (PredNow - PrevtickStart) / (float)(CurtickStart-PrevtickStart);

			if(NewPredTick < m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV]->m_Tick-SERVER_TICK_SPEED || NewPredTick > m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV]->m_Tick+SERVER_TICK_SPEED)
			{
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_ADDINFO, "client", "prediction time reset!");
				m_PredictedTime.Init(m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT]->m_Tick*time_freq()/50);
			}

			if(NewPredTick > m_PredTick[g_Config.m_ClDummy])
			{
				m_PredTick[g_Config.m_ClDummy] = NewPredTick;
				Repredict = 1;

				// send input
				SendInput();
			}
		}

		// only do sane predictions
		if(Repredict)
		{
			if(m_PredTick[g_Config.m_ClDummy] > m_CurGameTick[g_Config.m_ClDummy] && m_PredTick[g_Config.m_ClDummy] < m_CurGameTick[g_Config.m_ClDummy]+50)
				GameClient()->OnPredict();
		}

		// fetch server info if we don't have it
		if(State() >= IClient::STATE_LOADING &&
			m_CurrentServerInfoRequestTime >= 0 &&
			time_get() > m_CurrentServerInfoRequestTime)
		{
			m_ServerBrowser.RequestCurrentServer(m_ServerAddress);
			m_CurrentServerInfoRequestTime = time_get()+time_freq()*2;
		}
	}

	// STRESS TEST: join the server again
	if(g_Config.m_DbgStress)
	{
		static int64 ActionTaken = 0;
		int64 Now = time_get();
		if(State() == IClient::STATE_OFFLINE)
		{
			if(Now > ActionTaken+time_freq()*2)
			{
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "stress", "reconnecting!");
				Connect(g_Config.m_DbgStressServer);
				ActionTaken = Now;
			}
		}
		else
		{
			if(Now > ActionTaken+time_freq()*(10+g_Config.m_DbgStress))
			{
				m_pConsole->Print(IConsole::OUTPUT_LEVEL_DEBUG, "stress", "disconnecting!");
				Disconnect();
				ActionTaken = Now;
			}
		}
	}

	// pump the network
	PumpNetwork();
	if(m_pMapdownloadTask)
	{
		if(m_pMapdownloadTask->State() == CFetchTask::STATE_DONE)
			FinishMapDownload();
		else if(m_pMapdownloadTask->State() == CFetchTask::STATE_ERROR)
		{
			if(m_CurrentMapServer < m_MapDbUrls.size())
				MapFetcherStart(m_aMapdownloadFilename, m_MapdownloadCrc);
			else
			{
				dbg_msg("webdl", "http failed, falling back to gameserver");
				ResetMapDownload();
				SendMapRequest();
			}
		}
		else if(m_pMapdownloadTask->State() == CFetchTask::STATE_ABORTED)
		{
			m_pMapdownloadTask = 0;
		}
	}


	// update the maser server registry
	MasterServer()->Update();

	// update the server browser
	m_ServerBrowser.Update(m_ResortServerBrowser);
	m_ResortServerBrowser = false;

	// update gameclient
	if(!m_EditorActive)
		GameClient()->OnUpdate();

	if(m_ReconnectTime > 0 && time_get() > m_ReconnectTime)
	{
		Connect(m_aServerAddressStr);
		m_ReconnectTime = 0;
	}
}

void CClient::VersionUpdate()
{
	CALLSTACK_ADD();

	if(m_VersionInfo.m_State == CVersionInfo::STATE_INIT)
	{
			Engine()->HostLookup(&m_VersionInfo.m_VersionServeraddr, g_Config.m_ClDDNetVersionServer, m_NetClient[0].NetType());
			m_VersionInfo.m_State = CVersionInfo::STATE_START;
	}
	else if(m_VersionInfo.m_State == CVersionInfo::STATE_START)
	{
		if(m_VersionInfo.m_VersionServeraddr.m_Job.Status() == CJob::STATE_DONE)
		{
			CNetChunk Packet;

			mem_zero(&Packet, sizeof(Packet));

			m_VersionInfo.m_VersionServeraddr.m_Addr.port = VERSIONSRV_PORT;

			Packet.m_ClientID = -1;
			Packet.m_Address = m_VersionInfo.m_VersionServeraddr.m_Addr;
			Packet.m_pData = VERSIONSRV_GETVERSION;
			Packet.m_DataSize = sizeof(VERSIONSRV_GETVERSION);
			Packet.m_Flags = NETSENDFLAG_CONNLESS;

			m_NetClient[0].Send(&Packet);
			m_VersionInfo.m_State = CVersionInfo::STATE_READY;
		}
	}
}

void CClient::CheckVersionUpdate(bool Force)
{
	CALLSTACK_ADD();

	m_VersionInfo.m_State = CVersionInfo::STATE_START;
	m_Updater.CheckForUpdates(Force);
}

void CClient::RegisterInterfaces()
{
	CALLSTACK_ADD();

	Kernel()->RegisterInterface(static_cast<IDemoRecorder*>(&m_DemoRecorder[RECORDER_MANUAL]));
	Kernel()->RegisterInterface(static_cast<IDemoPlayer*>(&m_DemoPlayer));
	Kernel()->RegisterInterface(static_cast<IServerBrowser*>(&m_ServerBrowser));
	Kernel()->RegisterInterface(static_cast<IFetcher*>(&m_Fetcher));
	Kernel()->RegisterInterface(static_cast<ICurlWrapper*>(&m_CurlWrapper));
#if !defined(CONF_PLATFORM_MACOSX) && !defined(__ANDROID__)
	Kernel()->RegisterInterface(static_cast<IUpdater*>(&m_Updater));
#endif
	Kernel()->RegisterInterface(static_cast<IFriends*>(&m_Friends));
	Kernel()->ReregisterInterface(static_cast<IFriends*>(&m_Foes));
	Kernel()->RegisterInterface(static_cast<IIRC*>(&m_IRC));
}

void CClient::InitInterfaces()
{
	CALLSTACK_ADD();

	// fetch interfaces
	m_pEngine = Kernel()->RequestInterface<IEngine>();
	m_pEditor = Kernel()->RequestInterface<IEditor>();
	//m_pGraphics = Kernel()->RequestInterface<IEngineGraphics>();
	m_pSound = Kernel()->RequestInterface<IEngineSound>();
	m_pGameClient = Kernel()->RequestInterface<IGameClient>();
	m_pInput = Kernel()->RequestInterface<IEngineInput>();
	m_pMap = Kernel()->RequestInterface<IEngineMap>();
	m_pMasterServer = Kernel()->RequestInterface<IEngineMasterServer>();
	m_pFetcher = Kernel()->RequestInterface<IFetcher>();
	m_pCurlWrapper = Kernel()->RequestInterface<ICurlWrapper>();
#if !defined(CONF_PLATFORM_MACOSX) && !defined(__ANDROID__)
	m_pUpdater = Kernel()->RequestInterface<IUpdater>();
#endif
	m_pStorage = Kernel()->RequestInterface<IStorageTW>();
	m_pIRC = Kernel()->RequestInterface<IIRC>();

	m_DemoEditor.Init(m_pGameClient->NetVersion(), &m_SnapshotDelta, m_pConsole, m_pStorage);

	m_ServerBrowser.SetBaseInfo(&m_NetClient[2], m_pGameClient->NetVersion());

	m_Fetcher.Init();

#if !defined(CONF_PLATFORM_MACOSX) && !defined(__ANDROID__)
	m_Updater.Init();
	m_Updater.CheckForUpdates(true); // force update check at startup
#endif

	m_Friends.Init();
	m_Foes.Init(true);

	IOHANDLE newsFile = m_pStorage->OpenFile("tmp/cache/ddnet-news.txt", IOFLAG_READ, IStorageTW::TYPE_SAVE);
	if (newsFile)
	{
		io_read(newsFile, m_aNewsDDNet, NEWS_SIZE);
		io_close(newsFile);
	}
}

void CClient::Run()
{
	CALLSTACK_ADD();

	m_LocalStartTime = time_get();
	m_TimerStartTime = time_get();
	m_SnapshotParts = 0;

	if(m_GenerateTimeoutSeed)
	{
		GenerateTimeoutSeed();
	}

	unsigned int Seed;
	secure_random_fill(&Seed, sizeof(Seed));
	srand(Seed);

	// init SDL
	{
		if(SDL_Init(0) < 0)
		{
			dbg_msg("client", "unable to init SDL base: %s", SDL_GetError());
			return;
		}

		atexit(SDL_Quit); // ignore_convention
	}

	m_MenuStartTime = time_get();

	// init graphics
	{
		m_pGraphics = CreateEngineGraphicsThreaded();

		bool RegisterFail = false;
		RegisterFail = RegisterFail || !Kernel()->RegisterInterface(static_cast<IEngineGraphics*>(m_pGraphics)); // register graphics as both
		RegisterFail = RegisterFail || !Kernel()->RegisterInterface(static_cast<IGraphics*>(m_pGraphics));

		if(RegisterFail || m_pGraphics->Init() != 0)
		{
			dbg_msg("client", "couldn't init graphics");
			return;
		}
	}

	// init sound, allowed to fail
	m_SoundInitFailed = Sound()->Init() != 0;

	// open socket
	{
		NETADDR BindAddr;
		if(g_Config.m_Bindaddr[0] && net_host_lookup(g_Config.m_Bindaddr, &BindAddr, NETTYPE_ALL) == 0)
		{
			// got bindaddr
			BindAddr.type = NETTYPE_ALL;
		}
		else
		{
			mem_zero(&BindAddr, sizeof(BindAddr));
			BindAddr.type = NETTYPE_ALL;
		}
		for(int i = 0; i < 3; i++)
		{
			do
			{
				BindAddr.port = (secure_rand() % 64511) + 1024;
			}
			while(!m_NetClient[i].Open(BindAddr, 0));
		}
	}

	// init font rendering
	Kernel()->RequestInterface<IEngineTextRender>()->Init();

	// init the input
	Input()->Init();

	// start refreshing addresses while we load
	MasterServer()->RefreshAddresses(m_NetClient[0].NetType());

	// init the editor ... XXX this is done again in GameClient()->OnInit() ??
	//m_pEditor->Init();

	// load data
	if(!LoadData())
		return;

	// init lua
	m_Lua.Init(this, Storage(), m_pConsole);

	GameClient()->OnInit();

//#if !(defined(CONF_FAMILY_WINDOWS) && defined(CONF_DEBUG))
//	if((m_pInputThread = thread_init_named(InputThread, this, "inputthread")))
//		thread_detach(m_pInputThread);
//#endif

	// connect to the server if wanted
	/*
	if(config.cl_connect[0] != 0)
		Connect(config.cl_connect);
	config.cl_connect[0] = 0;
	*/

	//
	m_FpsGraph.Init(0.0f, 200.0f);

	// never start with the editor
	g_Config.m_ClEditor = 0;

	// process pending commands
	m_pConsole->StoreCommands(false);

#if defined(CONF_FAMILY_UNIX)
	m_Fifo.Init(m_pConsole, g_Config.m_ClInputFifo, CFGFLAG_CLIENT);
#endif

	bool LastD = false;
	bool LastQ = false;
	bool LastE = false;
	bool LastG = false;
	bool LastL = false;

	int LastConsoleMode = g_Config.m_ClConsoleMode;
	int64 ConsoleModeEmote = 0, LastTick = 0;  //timestamps


	while (1)
	{
		set_new_tick();

		if(g_Config.m_ClConsoleMode != LastConsoleMode)
		{
			const CServerInfo *pInfo = GetServerInfo();

			if(g_Config.m_ClConsoleMode) // hide
			{
				// input thread doesn't go well together with debugging on windows
#if !(defined(CONF_FAMILY_WINDOWS) && defined(CONF_DEBUG))
				if((m_pInputThread = thread_init_named(InputThread, this, "inputthread")))
					thread_detach(m_pInputThread);
#endif
				m_pGraphics->HideWindow();

				if(IsDDNet(pInfo) || IsDDRace(pInfo))
				{
					// eye emote
					CNetMsg_Cl_Say Msg;
					Msg.m_Team = 0;
					Msg.m_pMessage = "/emote blink 999999";
					SendPackMsg(&Msg, MSGFLAG_VITAL);
				}
			}
			else // show
			{
#if !(defined(CONF_FAMILY_WINDOWS) && defined(CONF_DEBUG))
				if(m_pInputThread)
					thread_destroy(m_pInputThread);
				else
					dbg_msg("client/warn", "m_pInputThread == NULL");
				m_pInputThread = NULL;
#endif

				m_pGraphics->UnhideWindow();

				if(IsDDNet(pInfo) || IsDDRace(pInfo))
				{
					// reset eye emote
					CNetMsg_Cl_Say Msg;
					Msg.m_Team = 0;
					Msg.m_pMessage = "/emote normal 1";
					SendPackMsg(&Msg, MSGFLAG_VITAL);
				}
			}

			LastConsoleMode = g_Config.m_ClConsoleMode;
		}

		if(g_Config.m_ClConsoleMode)
		{
			if(g_Config.m_ClConsoleModeEmotes != 0 && time_get() - ConsoleModeEmote > time_freq())
			{
				ConsoleModeEmote = time_get();
				CNetMsg_Cl_Emoticon Msg;
				Msg.m_Emoticon = EMOTICON_ZZZ;
				SendPackMsg(&Msg, MSGFLAG_VITAL);
			}
		}

		if(time_get() > LastTick+time_freq()*(1.0f/50.0f))
		{
			LastTick = time_get();
			LUA_FIRE_EVENT("OnTick");
		}

		//

		VersionUpdate();

		// handle pending connects
		if(m_aCmdConnect[0])
		{
			ConnectImpl();
		}

		// progress on dummy connect if security token handshake skipped/passed
		if (m_DummySendConnInfo && !m_NetClient[1].SecurityTokenUnknown())
		{
			m_DummySendConnInfo = false;

			// send client info
			CMsgPacker MsgInfo(NETMSG_INFO);
			MsgInfo.AddString(GameClient()->NetVersion(), 128);
			MsgInfo.AddString(g_Config.m_Password, 128);
			SendMsgExY(&MsgInfo, MSGFLAG_VITAL|MSGFLAG_FLUSH, true, 1);

			// update netclient
			m_NetClient[1].Update();

			// send ready
			CMsgPacker MsgReady(NETMSG_READY);
			SendMsgExY(&MsgReady, MSGFLAG_VITAL|MSGFLAG_FLUSH, true, 1);

			// startinfo
			GameClient()->SendDummyInfo(true);

			// send enter game an finish the connection
			CMsgPacker MsgEnter(NETMSG_ENTERGAME);
			SendMsgExY(&MsgEnter, MSGFLAG_VITAL|MSGFLAG_FLUSH, true, 1);
		}

		// update input
		if(!g_Config.m_ClConsoleMode)
		{
			if(Input()->Update())
				break;	// SDL_QUIT
		}

#if !defined(CONF_PLATFORM_MACOSX) && !defined(__ANDROID__)
		Updater()->Tick();
#endif

		// update sound
		if(!g_Config.m_ClConsoleMode)
			Sound()->Update();

		// release focus TODO::XXX::REIMPLEMENT (if this is needed)
		/*	if(!m_pGraphics->WindowActive())
			{
				if(m_WindowMustRefocus == 0)
					Input()->MouseModeAbsolute();
				m_WindowMustRefocus = 1;
			}
			else if (g_Config.m_DbgFocus && Input()->KeyPress(KEY_ESCAPE))
			{
				Input()->MouseModeAbsolute();
				m_WindowMustRefocus = 1;
			}

			// refocus
			if(m_WindowMustRefocus && m_pGraphics->WindowActive() && !g_Config.m_ClConsoleMode)
			{
				if(m_WindowMustRefocus < 3)
				{
					Input()->MouseModeAbsolute();
					m_WindowMustRefocus++;
				}

				if(m_WindowMustRefocus >= 3 || Input()->KeyPress(KEY_MOUSE_1))
				{
					Input()->MouseModeRelative();
					m_WindowMustRefocus = 0;
				}
			}
	*/
		// panic quit button and restart
		if(CtrlShiftKey(KEY_Q, LastQ))
		{
			Quit();
			break;
		}

		if(CtrlShiftKey(KEY_D, LastD))
			g_Config.m_Debug ^= 1;

		if(CtrlShiftKey(KEY_G, LastG))
			g_Config.m_DbgGraphs ^= 1;

		if(CtrlShiftKey(KEY_L, LastL))
		{
			int NumLuaFiles = Lua()->GetLuaFiles().size();
			int Counter = 0;
			for(int i = 0; i < NumLuaFiles; i++)
			{
				CLuaFile *pLF = Lua()->GetLuaFiles()[i];
				if(pLF->State() == CLuaFile::STATE_LOADED)
				{
					pLF->Unload();
					Counter++;
				}
			}
			dbg_msg("client/lua", "quick-unloaded all %i active scripts", Counter);
		}

		if(CtrlShiftKey(KEY_E, LastE))
		{
			g_Config.m_ClEditor = g_Config.m_ClEditor^1;
			Input()->MouseModeRelative();
			Input()->SetIMEState(true);
		}

		// render
		{
			if(g_Config.m_ClEditor)
			{
				static bool EditorInited = !g_Config.m_ClEditorLazyInit;
				if(!EditorInited)
				{
					m_pEditor->Init();
					EditorInited = true;
				}
				if(!m_EditorActive)
				{
					Input()->MouseModeRelative();
					GameClient()->OnActivateEditor();
					m_EditorActive = true;
				}
			}
			else if(m_EditorActive)
				m_EditorActive = false;

			Update();

			if((g_Config.m_GfxBackgroundRender || m_pGraphics->WindowOpen()) && (!g_Config.m_GfxAsyncRenderOld || m_pGraphics->IsIdle()))
			{
				m_RenderFrames++;

				// update frametime
				int64 Now = time_get();
				m_RenderFrameTime = (Now - m_LastRenderTime) / (float)time_freq();
				if(m_RenderFrameTime < m_RenderFrameTimeLow)
					m_RenderFrameTimeLow = m_RenderFrameTime;
				if(m_RenderFrameTime > m_RenderFrameTimeHigh)
					m_RenderFrameTimeHigh = m_RenderFrameTime;
				m_FpsGraph.Add(1.0f/m_RenderFrameTime, 1,1,1);

				m_LastRenderTime = Now;

				if(g_Config.m_DbgStress)
				{
					if((m_RenderFrames%10) == 0)
					{
						if(!m_EditorActive)
							Render();
						else
						{
							m_pEditor->UpdateAndRender();
							DebugRender();
						}
						m_pGraphics->Swap();
					}
				}
				else
				{
					if(!m_EditorActive)
						Render();
					else
					{
						m_pEditor->UpdateAndRender();
						DebugRender();
					}
					m_pGraphics->Swap();
				}
				Input()->NextFrame();
			}
			if(Input()->VideoRestartNeeded())
			{
				m_pGraphics->Init();
				LoadData();
				GameClient()->OnInit();
			}
		}

		AutoScreenshot_Cleanup();

		// check conditions
		if(State() == IClient::STATE_QUITING)
			break;

		// menu tick
		if(State() == IClient::STATE_OFFLINE)
		{
			int64 t = time_get();
			while(t > TickStartTime(m_CurMenuTick+1))
				m_CurMenuTick++;
		}

		//for(int oz = 0; oz < m_Lua.GetLuaFiles().size(); oz++)
		//	;

		LUA_FIRE_EVENT("ResumeThreads");

#if defined(CONF_FAMILY_UNIX)
		m_Fifo.Update();
#endif

		// beNice
		if(g_Config.m_DbgStress || (g_Config.m_ClCpuThrottleInactive && !m_pGraphics->WindowActive()))
			thread_sleep(g_Config.m_ClCpuThrottleInactive);
		else if(g_Config.m_ClCpuThrottle)
			net_socket_read_wait(m_NetClient[0].m_Socket, g_Config.m_ClCpuThrottle * 1000);
			//thread_sleep(g_Config.m_ClCpuThrottle);

		if(g_Config.m_ClConsoleMode)
			thread_sleep(250);

		if(g_Config.m_DbgHitch)
		{
			thread_sleep(g_Config.m_DbgHitch);
			g_Config.m_DbgHitch = 0;
		}

		// update local time
		m_LocalTime = (time_get()-m_LocalStartTime)/(float)time_freq();
		m_SteadyTimer = (time_get()-m_TimerStartTime)/(float)time_freq();

#if defined(CONF_DEBUG)
		static float LastCheck = m_SteadyTimer;
		if(g_Config.m_ClMemcheckInterval && m_SteadyTimer - LastCheck > g_Config.m_ClMemcheckInterval)
		{
			mem_check();
			LastCheck = m_SteadyTimer;
		}
#endif
	}

#if defined(CONF_FAMILY_UNIX)
	m_Fifo.Shutdown();
#endif

	GameClient()->OnShutdown();
	Disconnect();

	m_Lua.Shutdown();
	m_pGraphics->Shutdown();
	m_pSound->Shutdown();

	// shutdown SDL
	{
		SDL_Quit();
	}

	if(m_pInputThread)
	{
		//thread_wait(m_pInputThread);
		m_pInputThread = 0;
#if defined(CONF_FAMILY_WINDOWS)
		//FreeConsole();
#else
		//thread_destroy(m_pInputThread); // this tends to cause segfaults sometimes, dunno why :o
#endif
	}

	if(m_Updater.State() == IUpdater::STATE_CLEAN)
	{
		// do cleanups - much hack.
#if defined(CONF_FAMILY_UNIX)
		system("rm -rf update");
#elif defined(CONF_FAMILY_WINDOWS)
		system("if exist update rd update /S /Q");
#endif
	}
}

bool CClient::CtrlShiftKey(int Key, bool &Last)
{
	if(Input()->KeyIsPressed(KEY_LCTRL) && Input()->KeyIsPressed(KEY_LSHIFT) && !Last && Input()->KeyIsPressed(Key))
	{
		Last = true;
		return true;
	}
	else if (Last && !Input()->KeyIsPressed(Key))
		Last = false;

	return false;
}

int64 CClient::TickStartTime(int Tick)
{
	return m_MenuStartTime + (time_freq()*Tick)/m_GameTickSpeed;
}

void CClient::Con_Connect(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->Connect(pResult->GetString(0));
}

void CClient::Con_Disconnect(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->Disconnect();
}

void CClient::Con_Timeout(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->TimeMeOut();
}

void CClient::Con_DummyConnect(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->DummyConnect();
}

void CClient::Con_DummyDisconnect(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->DummyDisconnect("> AllTheHaxx < ");
}

void CClient::Con_Quit(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->Quit();
}

void CClient::Con_Minimize(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->Graphics()->Minimize();
}

void CClient::Con_Restart(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->Restart();
}

void CClient::Con_Ping(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	CMsgPacker Msg(NETMSG_PING);
	pSelf->SendMsgEx(&Msg, 0);
	pSelf->m_PingStartTime = time_get();
}

void CClient::Con_SaveConfig(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	if(!pSelf->Kernel()->RequestInterface<IConfig>()->Save(true))
		pSelf->m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "config", "failed to save the settings");
	((CGameClient *)(pSelf->m_pGameClient))->m_pIdentity->SaveIdents();
}

void CClient::AutoScreenshot_Start()
{
	CALLSTACK_ADD();

	if(g_Config.m_ClAutoScreenshot)
	{
		Graphics()->TakeScreenshot("auto/autoscreen");
		m_AutoScreenshotRecycle = true;
	}
}

void CClient::AutoStatScreenshot_Start()
{
	CALLSTACK_ADD();

	if(g_Config.m_ClAutoStatboardScreenshot)
	{
		Graphics()->TakeScreenshot("auto/stats/autoscreen");
		m_AutoStatScreenshotRecycle = true;
	}
}

void CClient::AutoScreenshot_Cleanup()
{
	CALLSTACK_ADD();

	if(m_AutoScreenshotRecycle)
	{
		if(g_Config.m_ClAutoScreenshotMax)
		{
			// clean up auto taken screens
			CFileCollection AutoScreens;
			AutoScreens.Init(Storage(), "screenshots/auto", "autoscreen", ".png", g_Config.m_ClAutoScreenshotMax);
		}
		m_AutoScreenshotRecycle = false;
	}
}

void CClient::AutoStatScreenshot_Cleanup()
{
	CALLSTACK_ADD();

	if(m_AutoStatScreenshotRecycle)
	{
		if(g_Config.m_ClAutoStatboardScreenshotMax)
		{
			// clean up auto taken screens
			CFileCollection AutoScreens;
			AutoScreens.Init(Storage(), "screenshots/auto/stats", "autoscreen", ".png", g_Config.m_ClAutoStatboardScreenshotMax);
		}
		m_AutoStatScreenshotRecycle = false;
	}
}

void CClient::Con_Screenshot(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->Graphics()->TakeScreenshot(0);
}

void CClient::Con_Rcon(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->Rcon(pResult->GetString(0));
}

void CClient::Con_RconAuth(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->RconAuth("", pResult->GetString(0));
}

void CClient::Con_RconLogin(IConsole::IResult *pResult, void *pUserData)
{
	CClient *pSelf = (CClient *)pUserData;
	pSelf->RconAuth(pResult->GetString(0), pResult->GetString(1));
}

void CClient::Con_AddFavorite(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	NETADDR Addr;
	if(net_addr_from_str(&Addr, pResult->GetString(0)) == 0)
		pSelf->m_ServerBrowser.AddFavorite(Addr);
}

void CClient::Con_RemoveFavorite(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	NETADDR Addr;
	if(net_addr_from_str(&Addr, pResult->GetString(0)) == 0)
		pSelf->m_ServerBrowser.RemoveFavorite(Addr);
}

void CClient::DemoSliceBegin()
{
	CALLSTACK_ADD();

	const CDemoPlayer::CPlaybackInfo *pInfo = m_DemoPlayer.Info();
	g_Config.m_ClDemoSliceBegin = pInfo->m_Info.m_CurrentTick;
}

void CClient::DemoSliceEnd()
{
	CALLSTACK_ADD();

	const CDemoPlayer::CPlaybackInfo *pInfo = m_DemoPlayer.Info();
	g_Config.m_ClDemoSliceEnd = pInfo->m_Info.m_CurrentTick;
}

void CClient::Con_DemoSliceBegin(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->DemoSliceBegin();
}

void CClient::Con_DemoSliceEnd(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->DemoSliceEnd();
}

void CClient::DemoSlice(const char *pDstPath, bool RemoveChat)
{
	CALLSTACK_ADD();

	if (m_DemoPlayer.IsPlaying())
	{
		const char *pDemoFileName = m_DemoPlayer.GetDemoFileName();
		m_DemoEditor.Slice(pDemoFileName, pDstPath, g_Config.m_ClDemoSliceBegin, g_Config.m_ClDemoSliceEnd, RemoveChat);
	}
}

const char *CClient::DemoPlayer_Play(const char *pFilename, int StorageType)
{
	CALLSTACK_ADD();

	int Crc;
	const char *pError;
	Disconnect();
	m_NetClient[0].ResetErrorString();

	// try to start playback
	m_DemoPlayer.SetListener(this);

	if(m_DemoPlayer.Load(Storage(), m_pConsole, pFilename, StorageType))
		return "error loading demo";

	// load map
	Crc = (m_DemoPlayer.Info()->m_Header.m_aMapCrc[0]<<24)|
		  (m_DemoPlayer.Info()->m_Header.m_aMapCrc[1]<<16)|
		  (m_DemoPlayer.Info()->m_Header.m_aMapCrc[2]<<8)|
		  (m_DemoPlayer.Info()->m_Header.m_aMapCrc[3]);
	pError = LoadMapSearch(m_DemoPlayer.Info()->m_Header.m_aMapName, Crc);
	if(pError)
	{
		DisconnectWithReason(pError);
		return pError;
	}

	GameClient()->OnConnected();

	// setup buffers
	mem_zero(m_aDemorecSnapshotData, sizeof(m_aDemorecSnapshotData));

	m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT] = &m_aDemorecSnapshotHolders[SNAP_CURRENT];
	m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV] = &m_aDemorecSnapshotHolders[SNAP_PREV];

	m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT]->m_pSnap = (CSnapshot *)m_aDemorecSnapshotData[SNAP_CURRENT][0];
	m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT]->m_pAltSnap = (CSnapshot *)m_aDemorecSnapshotData[SNAP_CURRENT][1];
	m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT]->m_SnapSize = 0;
	m_aSnapshots[g_Config.m_ClDummy][SNAP_CURRENT]->m_Tick = -1;

	m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV]->m_pSnap = (CSnapshot *)m_aDemorecSnapshotData[SNAP_PREV][0];
	m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV]->m_pAltSnap = (CSnapshot *)m_aDemorecSnapshotData[SNAP_PREV][1];
	m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV]->m_SnapSize = 0;
	m_aSnapshots[g_Config.m_ClDummy][SNAP_PREV]->m_Tick = -1;

	// enter demo playback state
	SetState(IClient::STATE_DEMOPLAYBACK);

	m_DemoPlayer.Play();
	GameClient()->OnEnterGame();

	return 0;
}

void CClient::Con_Play(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->DemoPlayer_Play(pResult->GetString(0), IStorageTW::TYPE_ALL);
}

void CClient::Con_DemoPlay(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	if(pSelf->m_DemoPlayer.IsPlaying()){
		if(pSelf->m_DemoPlayer.BaseInfo()->m_Paused){
			pSelf->m_DemoPlayer.Unpause();
		}
		else{
			pSelf->m_DemoPlayer.Pause();
		}
	}
}

void CClient::Con_DemoSpeed(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->m_DemoPlayer.SetSpeed(pResult->GetFloat(0));
}

void CClient::DemoRecorder_Start(const char *pFilename, bool WithTimestamp, int Recorder)
{
	CALLSTACK_ADD();

	if(State() != IClient::STATE_ONLINE)
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "demorec/record", "client is not online");
	else
	{
		char aFilename[128];
		if(WithTimestamp)
		{
			char aDate[20];
			str_timestamp(aDate, sizeof(aDate));
			str_format(aFilename, sizeof(aFilename), "demos/%s_%s.demo", pFilename, aDate);
		}
		else
			str_format(aFilename, sizeof(aFilename), "demos/%s.demo", pFilename);
		m_DemoRecorder[Recorder].Start(Storage(), m_pConsole, aFilename, GameClient()->NetVersion(), m_aCurrentMap, m_CurrentMapCrc, "client");
	}
}

void CClient::DemoRecorder_HandleAutoStart()
{
	CALLSTACK_ADD();

	if(g_Config.m_ClAutoDemoRecord)
	{
		DemoRecorder_Stop(RECORDER_AUTO);
		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "auto/%s", m_aCurrentMap);
		DemoRecorder_Start(aBuf, true, RECORDER_AUTO);
		if(g_Config.m_ClAutoDemoMax)
		{
			// clean up auto recorded demos
			CFileCollection AutoDemos;
			AutoDemos.Init(Storage(), "demos/auto", "" /* empty for wild card */, ".demo", g_Config.m_ClAutoDemoMax);
		}
	}
}

void CClient::DemoRecorder_Stop(int Recorder)
{
	CALLSTACK_ADD();

	m_DemoRecorder[Recorder].Stop();
}

void CClient::DemoRecorder_AddDemoMarker(int Recorder)
{
	CALLSTACK_ADD();

	m_DemoRecorder[Recorder].AddDemoMarker();
}

class IDemoRecorder *CClient::DemoRecorder(int Recorder)
{
	return &m_DemoRecorder[Recorder];
}

void CClient::Con_Record(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	if(pResult->NumArguments())
		pSelf->DemoRecorder_Start(pResult->GetString(0), false, RECORDER_MANUAL);
	else
		pSelf->DemoRecorder_Start(pSelf->m_aCurrentMap, true, RECORDER_MANUAL);
}

void CClient::Con_StopRecord(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	int Recorder = RECORDER_MANUAL;
	if(pResult->NumArguments() > 0)
		Recorder = pResult->GetInteger(0);
	if(Recorder < RECORDER_MANUAL || Recorder >= RECORDER_MAX)
		pSelf->m_pConsole->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "demo", "There is no demorecorder with ID %i. The given ID must be in range %i-%i.", Recorder, RECORDER_MANUAL, RECORDER_MAX-1);
	else
	{
		if(pSelf->DemoRecorder(Recorder)->IsRecording())
		{
			pSelf->DemoRecorder_Stop(Recorder);
			pSelf->m_pConsole->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "demo", "Stopped demorecorder %i", Recorder);
		}
		else
			pSelf->m_pConsole->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "demo", "Demorecorder %i is not currently recording.", Recorder);
	}
}

void CClient::Con_AddDemoMarker(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	pSelf->DemoRecorder_AddDemoMarker(RECORDER_MANUAL);
	pSelf->DemoRecorder_AddDemoMarker(RECORDER_RACE);
	pSelf->DemoRecorder_AddDemoMarker(RECORDER_AUTO);
}

void CClient::ServerBrowserUpdate()
{
	CALLSTACK_ADD();

	m_ResortServerBrowser = true;
}

void CClient::ConchainServerBrowserUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CALLSTACK_ADD();

	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		((CClient *)pUserData)->ServerBrowserUpdate();
}

void CClient::Con_Panic(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	g_Config.m_ClConsoleMode = 0;
}

void CClient::SwitchWindowScreen(int Index)
{
	CALLSTACK_ADD();

	// Todo SDL: remove this when fixed (changing screen when in fullscreen is bugged)
	if(g_Config.m_GfxFullscreen)
	{
		ToggleFullscreen();
		if(Graphics()->SetWindowScreen(Index))
			g_Config.m_GfxScreen = Index;
		ToggleFullscreen();
	}
	else
	{
		if(Graphics()->SetWindowScreen(Index))
			g_Config.m_GfxScreen = Index;
	}
}

void CClient::ConchainWindowScreen(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	if(pSelf->Graphics() && pResult->NumArguments())
	{
		if(g_Config.m_GfxScreen != pResult->GetInteger(0))
			pSelf->SwitchWindowScreen(pResult->GetInteger(0));
	}
	else
		pfnCallback(pResult, pCallbackUserData);
}

void CClient::ToggleFullscreen()
{
	CALLSTACK_ADD();

	if(Graphics()->Fullscreen(g_Config.m_GfxFullscreen^1))
		g_Config.m_GfxFullscreen ^= 1;
}

void CClient::ConchainFullscreen(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	if(pSelf->Graphics() && pResult->NumArguments())
	{
		if(g_Config.m_GfxFullscreen != pResult->GetInteger(0))
			pSelf->ToggleFullscreen();
	}
	else
		pfnCallback(pResult, pCallbackUserData);
}

void CClient::ToggleWindowBordered()
{
	CALLSTACK_ADD();

	g_Config.m_GfxBorderless ^= 1;
	Graphics()->SetWindowBordered(!g_Config.m_GfxBorderless);
}

void CClient::ConchainWindowBordered(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	if(pSelf->Graphics() && pResult->NumArguments())
	{
		if(!g_Config.m_GfxFullscreen && (g_Config.m_GfxBorderless != pResult->GetInteger(0)))
			pSelf->ToggleWindowBordered();
	}
	else
		pfnCallback(pResult, pCallbackUserData);
}

void CClient::ToggleWindowVSync()
{
	CALLSTACK_ADD();

	if(Graphics()->SetVSync(g_Config.m_GfxVsync^1))
		g_Config.m_GfxVsync ^= 1;
}

void CClient::ConchainWindowVSync(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CALLSTACK_ADD();

	CClient *pSelf = (CClient *)pUserData;
	if(pSelf->Graphics() && pResult->NumArguments())
	{
		if(g_Config.m_GfxVsync != pResult->GetInteger(0))
			pSelf->ToggleWindowVSync();
	}
	else
		pfnCallback(pResult, pCallbackUserData);
}

void CClient::ConchainTimeoutSeed(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData)
{
	CClient *pSelf = (CClient *)pUserData;
	pfnCallback(pResult, pCallbackUserData);
	if(pResult->NumArguments())
		pSelf->m_GenerateTimeoutSeed = false;
}

void CClient::RegisterCommands()
{
	CALLSTACK_ADD();

	m_pConsole = Kernel()->RequestInterface<IConsole>();
	// register server dummy commands for tab completion
	m_pConsole->Register("kick", "i[id] ?r[reason]", CFGFLAG_SERVER, 0, 0, "Kick player with specified id for any reason");
	m_pConsole->Register("ban", "s[ip|id] ?i[minutes] r[reason]", CFGFLAG_SERVER, 0, 0, "Ban player with ip/id for x minutes for any reason");
	m_pConsole->Register("unban", "s[ip]", CFGFLAG_SERVER, 0, 0, "Unban ip");
	m_pConsole->Register("bans", "", CFGFLAG_SERVER, 0, 0, "Show banlist");
	m_pConsole->Register("status", "", CFGFLAG_SERVER, 0, 0, "List players");
	m_pConsole->Register("shutdown", "", CFGFLAG_SERVER, 0, 0, "Shut down");
	m_pConsole->Register("record", "s[file]", CFGFLAG_SERVER, 0, 0, "Record to a file");
	m_pConsole->Register("stoprecord", "", CFGFLAG_SERVER, 0, 0, "Stop recording");
	m_pConsole->Register("reload", "", CFGFLAG_SERVER, 0, 0, "Reload the map");

	m_pConsole->Register("dummy_connect", "", CFGFLAG_CLIENT, Con_DummyConnect, this, "connect dummy");
	m_pConsole->Register("dummy_disconnect", "", CFGFLAG_CLIENT, Con_DummyDisconnect, this, "disconnect dummy");

	m_pConsole->Register("quit", "", CFGFLAG_CLIENT|CFGFLAG_STORE, Con_Quit, this, "Quit Teeworlds");
	m_pConsole->Register("exit", "", CFGFLAG_CLIENT|CFGFLAG_STORE, Con_Quit, this, "Quit Teeworlds");
	m_pConsole->Register("restart", "", CFGFLAG_CLIENT|CFGFLAG_STORE, Con_Restart, this, "Restart Teeworlds");
	m_pConsole->Register("minimize", "", CFGFLAG_CLIENT|CFGFLAG_STORE, Con_Minimize, this, "Minimize Teeworlds");
	m_pConsole->Register("connect", "s[host|ip]", CFGFLAG_CLIENT, Con_Connect, this, "Connect to the specified host/ip");
	m_pConsole->Register("disconnect", "", CFGFLAG_CLIENT, Con_Disconnect, this, "Disconnect from the server");
	m_pConsole->Register("timeout", "", CFGFLAG_CLIENT, Con_Timeout, this, "Dirty disconnect from the server; will create a timeout-dummy on ddnet servers");
	m_pConsole->Register("ping", "", CFGFLAG_CLIENT, Con_Ping, this, "Ping the current server");
	m_pConsole->Register("config_save", "", CFGFLAG_CLIENT, Con_SaveConfig, this, "Write down the config");
	m_pConsole->Register("screenshot", "", CFGFLAG_CLIENT, Con_Screenshot, this, "Take a screenshot");
	m_pConsole->Register("rcon", "r[rcon-command]", CFGFLAG_CLIENT, Con_Rcon, this, "Send specified command to rcon");
	m_pConsole->Register("rcon_auth", "s[password]", CFGFLAG_CLIENT, Con_RconAuth, this, "Authenticate to rcon");
	m_pConsole->Register("rcon_login", "s[username] r[password]", CFGFLAG_CLIENT, Con_RconLogin, this, "Authenticate to rcon with a username");
	m_pConsole->Register("play", "r[file]", CFGFLAG_CLIENT|CFGFLAG_STORE, Con_Play, this, "Play the file specified");
	m_pConsole->Register("record", "?s[file]", CFGFLAG_CLIENT, Con_Record, this, "Record to the file");
	m_pConsole->Register("stoprecord", "?i[recorder]", CFGFLAG_CLIENT, Con_StopRecord, this, "Stop recording (0=standard, 1=auto, 2=race)");
	m_pConsole->Register("add_demomarker", "", CFGFLAG_CLIENT, Con_AddDemoMarker, this, "Add demo timeline marker");
	m_pConsole->Register("add_favorite", "s[host|ip]", CFGFLAG_CLIENT, Con_AddFavorite, this, "Add a server as a favorite");
	m_pConsole->Register("remove_favorite", "s[host|ip]", CFGFLAG_CLIENT, Con_RemoveFavorite, this, "Remove a server from favorites");
	m_pConsole->Register("demo_slice_start", "", CFGFLAG_CLIENT, Con_DemoSliceBegin, this, "");
	m_pConsole->Register("demo_slice_end", "", CFGFLAG_CLIENT, Con_DemoSliceEnd, this, "");
	m_pConsole->Register("demo_play", "", CFGFLAG_CLIENT, Con_DemoPlay, this, "Play demo");
	m_pConsole->Register("demo_speed", "i[speed]", CFGFLAG_CLIENT, Con_DemoSpeed, this, "Set demo speed");

	m_pConsole->Register("q", "", CFGFLAG_CLIENT, Con_Panic, this, "Panic command to deactivate console mode");

	// used for server browser update
	m_pConsole->Chain("cl_timeout_seed", ConchainTimeoutSeed, this);

	m_pConsole->Chain("br_filter_string", ConchainServerBrowserUpdate, this);
	m_pConsole->Chain("br_filter_gametype", ConchainServerBrowserUpdate, this);
	m_pConsole->Chain("br_filter_serveraddress", ConchainServerBrowserUpdate, this);

	m_pConsole->Chain("gfx_screen", ConchainWindowScreen, this);
	m_pConsole->Chain("gfx_fullscreen", ConchainFullscreen, this);
	m_pConsole->Chain("gfx_borderless", ConchainWindowBordered, this);
	m_pConsole->Chain("gfx_vsync", ConchainWindowVSync, this);

	// DDRace


#define CONSOLE_COMMAND(name, params, flags, callback, userdata, help) m_pConsole->Register(name, params, flags, 0, 0, help);
#include <game/ddracecommands.h>
}

static CClient *CreateClient()
{
	return new CClient;
}


/*
	Server Time
	Client Mirror Time
	Client Predicted Time

	Snapshot Latency
		Downstream latency

	Prediction Latency
		Upstream latency
*/

void *main_thread_handle = 0;

#if defined(CONF_PLATFORM_MACOSX) || defined(__ANDROID__)
extern "C" int SDL_main(int argc, char **argv_) // ignore_convention
{
	const char **argv = const_cast<const char **>(argv_);
#else
int main(int argc, const char **argv) // ignore_convention
{
#endif
#if defined(CONF_FAMILY_WINDOWS)
	for(int i = 1; i < argc; i++) // ignore_convention
	{
		if(str_comp("-s", argv[i]) == 0 || str_comp("--silent", argv[i]) == 0) // ignore_convention
		{
			FreeConsole();
			break;
		}
	}
#else
	signal(SIGPIPE, SIG_IGN);
#endif

#if !defined(CONF_PLATFORM_MACOSX)
	dbg_enable_threaded();
#endif

	if(secure_random_init() != 0)
	{
		dbg_msg("secure", "could not initialize secure RNG");
		return -1;
	}

	if(curl_global_init(CURL_GLOBAL_DEFAULT) != 0)
	{
		dbg_msg("curl", "could not initialize libcurl");
		return -1;
	}

	// initialize the debugger
	CDebugger *pDebugger = new CDebugger();
#if defined(CONF_FAMILY_UNIX) && defined(FEATURE_DEBUGGER) && !defined(CONF_DEBUG)
	main_thread_handle = thread_get_current();
#endif
	CALLSTACK_ADD();

	CClient *pClient = CreateClient();
	IKernel *pKernel = IKernel::Create();
	pKernel->RegisterInterface(pClient);
	pClient->RegisterInterfaces();

	// create the components
	IEngine *pEngine = CreateEngine("Teeworlds");
	IConsole *pConsole = CreateConsole(CFGFLAG_CLIENT);
	IStorageTW *pStorage = CreateStorage("Teeworlds", IStorageTW::STORAGETYPE_CLIENT, argc, argv); // ignore_convention
	IConfig *pConfig = CreateConfig();
	IEngineSound *pEngineSound = CreateEngineSound();
	IEngineInput *pEngineInput = CreateEngineInput();
	IEngineTextRender *pEngineTextRender = CreateEngineTextRender();
	IEngineMap *pEngineMap = CreateEngineMap();
	IEngineMasterServer *pEngineMasterServer = CreateEngineMasterServer();

	{
		bool RegisterFail = false;

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pEngine);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pConsole);
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pConfig);

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineSound*>(pEngineSound)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<ISound*>(pEngineSound));

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineInput*>(pEngineInput)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IInput*>(pEngineInput));

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineTextRender*>(pEngineTextRender)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<ITextRender*>(pEngineTextRender));

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineMap*>(pEngineMap)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IMap*>(pEngineMap));

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IEngineMasterServer*>(pEngineMasterServer)); // register as both
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(static_cast<IMasterServer*>(pEngineMasterServer));

		RegisterFail = RegisterFail || !pKernel->RegisterInterface(CreateEditor());
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(CreateGameClient());
		RegisterFail = RegisterFail || !pKernel->RegisterInterface(pStorage);

		if(RegisterFail)
			return -1;
	}

	pEngine->Init();
	pConfig->Init();
	pEngineMasterServer->Init();
	pEngineMasterServer->Load();

	// register all console commands
	pClient->RegisterCommands();

	pKernel->RequestInterface<IGameClient>()->OnConsoleInit();

	// init client's interfaces
	pClient->InitInterfaces();

	CDebugger::SetStaticData(pStorage);

	// execute config file
	IOHANDLE File = pStorage->OpenFile(CONFIG_FILE, IOFLAG_READ, IStorageTW::TYPE_ALL);
	if(File)
	{
		io_close(File);
		pConsole->ExecuteFile(CONFIG_FILE);
	}
	// Do not fallback to other configs to have our default values applied.
	// Old configs can easily be loaded using "exec" in f1
/*	else if((File = pStorage->OpenFile("settings_ddnet.cfg", IOFLAG_READ, IStorage::TYPE_ALL))) // fallback to ddnet
	{
		io_close(File);
	}
	else
	{
		pConsole->ExecuteFile("settings.cfg"); // fallback to vanilla
	}*/

	// execute autoexec file
	File = pStorage->OpenFile(AUTOEXEC_CLIENT_FILE, IOFLAG_READ, IStorageTW::TYPE_ALL);
	if(File)
	{
		io_close(File);
		pConsole->ExecuteFile(AUTOEXEC_CLIENT_FILE);
	}
	else // fallback
	{
		pConsole->ExecuteFile(AUTOEXEC_FILE);
	}

	if(g_Config.m_ClConfigVersion < 1)
	{
		if(g_Config.m_ClAntiPing == 0)
		{
			g_Config.m_ClAntiPingPlayers = 1;
			g_Config.m_ClAntiPingGrenade = 1;
			g_Config.m_ClAntiPingWeapons = 1;
		}
	}

	// parse the command line arguments
	if(argc > 1) // ignore_convention
	{
		if(str_comp_nocase_num(argv[1], "tw://", 5) == 0 && str_length(argv[1]) >= 5+7)
		{
			char aBuf[NETADDR_MAXSTRSIZE+8];
			str_copy(aBuf, &argv[1][5], sizeof(aBuf));
			str_format(aBuf, sizeof(aBuf), "connect %s", aBuf);
			pConsole->ExecuteLine(aBuf);
		}
		else
			pConsole->ParseArguments(argc-1, &argv[1]); // ignore_convention
	}

	pClient->Engine()->InitLogfile();

#if defined(CONF_FAMILY_WINDOWS)
	if(g_Config.m_ClHideConsole)
		FreeConsole();
#endif

	// For XOpenIM in SDL2: https://bugzilla.libsdl.org/show_bug.cgi?id=3102
	setlocale(LC_ALL, "");

	// run the client
	dbg_msg("client", "starting...");
	pClient->Run();

	// write down the config and quit
	pConfig->Save();

	bool Restart = pClient->m_Restarting;

	// cleanup
	delete pClient;
	delete pEngine;
	delete pStorage;
	delete pConfig;
	delete pEngineSound;
	delete pEngineInput;
	delete pEngineTextRender;
	delete pEngineMap;
	delete pEngineMasterServer;
	//delete pConsole;
	delete pDebugger;

	curl_global_cleanup();

	if(Restart)
		shell_execute(argv[0]);


	bool WantReport = false;
	for(int i = 1; i < argc; i++) // ignore_convention
	{
		if(str_comp("-r", argv[i]) == 0 || str_comp("--report", argv[i]) == 0 || str_comp("--leaks", argv[i]) == 0) // ignore_convention
		{
			WantReport = true;
			break;
		}
	}

	// print memory leak report
	if(WantReport)
	{
		mem_check();

		dbg_msg("leakreport", "Total of %i bytes (%d kb) not freed upon exit. Backtrace:", mem_stats()->allocated, mem_stats()->allocated>>10);
		MEMHEADER *conductor = mem_stats()->first;
		int CurrSize = 0, CurrNum = 0;
		while(conductor)
		{
			MEMHEADER *next = conductor->next;
			CurrNum++;
			CurrSize += conductor->size;
			if(next && str_comp_nocase(conductor->filename, next->filename) == 0 && conductor->line == next->line)
			{
				conductor = next;
				continue;
			}

			dbg_msg("leakreport", "%i bytes in %i from %s:%i", CurrSize, CurrNum, conductor->filename, conductor->line);
			CurrNum = 0;
			CurrSize = 0;
			conductor = next;
		}
	}

	return 0;
}

// DDRace

const char* CClient::GetCurrentMap()
{
	CALLSTACK_ADD();

	return m_aCurrentMap;
}

int CClient::GetCurrentMapCrc()
{
	CALLSTACK_ADD();

	return m_CurrentMapCrc;
}

const char* CClient::GetCurrentMapPath()
{
	return m_aCurrentMapPath;
}

const char* CClient::RaceRecordStart(const char *pFilename)
{
	CALLSTACK_ADD();

	char aFilename[128];
	str_format(aFilename, sizeof(aFilename), "demos/%s_%s.demo", m_aCurrentMap, pFilename);

	if(State() != STATE_ONLINE)
		dbg_msg("demorec/record", "client is not online");
	else
		m_DemoRecorder[RECORDER_RACE].Start(Storage(),  m_pConsole, aFilename, GameClient()->NetVersion(), m_aCurrentMap, m_CurrentMapCrc, "client");

	return m_aCurrentMap;
}

void CClient::RaceRecordStop()
{
	CALLSTACK_ADD();

	if(m_DemoRecorder[RECORDER_RACE].IsRecording())
		m_DemoRecorder[RECORDER_RACE].Stop();
}

bool CClient::RaceRecordIsRecording()
{
	CALLSTACK_ADD();

	return m_DemoRecorder[RECORDER_RACE].IsRecording();
}

void CClient::RequestDDNetSrvList()
{
	CALLSTACK_ADD();

	// request ddnet server list
	// generate new token
	for (int i = 0; i < 4; i++)
		m_aDDNetSrvListToken[i] = rand()&0xff;
	m_DDNetSrvListTokenSet = true;

	char aData[sizeof(VERSIONSRV_GETDDNETLIST)+4];
	mem_copy(aData, VERSIONSRV_GETDDNETLIST, sizeof(VERSIONSRV_GETDDNETLIST));
	mem_copy(aData+sizeof(VERSIONSRV_GETDDNETLIST), m_aDDNetSrvListToken, 4); // add token

	CNetChunk Packet;
	mem_zero(&Packet, sizeof(Packet));
	Packet.m_ClientID = -1;
	Packet.m_Address = m_VersionInfo.m_VersionServeraddr.m_Addr;
	Packet.m_pData = aData;
	Packet.m_DataSize = sizeof(VERSIONSRV_GETDDNETLIST)+4;
	Packet.m_Flags = NETSENDFLAG_CONNLESS;
	m_NetClient[g_Config.m_ClDummy].Send(&Packet);
}

int CClient::GetPredictionTime()
{
	CALLSTACK_ADD();

	int64 Now = time_get();
	return (int)((m_PredictedTime.Get(Now)-m_GameTime[g_Config.m_ClDummy].Get(Now))*1000/(float)time_freq());
}

void CClient::InputThread(void *pUser)
{
	CALLSTACK_ADD();

	//if(g_Config.m_Debug)
	dbg_msg("client/dbg", "input thread started");
	printf("\n");

	CClient *pSelf = (CClient *)pUser;
	char aInput[512];
	char *pInput = aInput;

	char aData[128];
	mem_zero(aData, sizeof(aData));

#if defined(CONF_FAMILY_WINDOWS)
	system("chcp 1252");
#else
	int flags = 0;
	int fd = fileno(stdin);
	flags = fcntl(fd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);
#endif

	while(1)
	{
		thread_sleep(100);
		if(pSelf->m_State == IClient::STATE_QUITING || !g_Config.m_ClConsoleMode)
		{
			//if(g_Config.m_Debug)
			dbg_msg("client/dbg", "input thread stopped");
			break;
		}

		mem_zerob(aInput);
		fgets(aInput, sizeof(aInput), stdin);
		aInput[str_length(aInput)-1] = '\0';

#if defined(CONF_FAMILY_WINDOWS)
		if(!str_utf8_check(pInput))
			{
				char aTemp[4] = {0};
				int Length = 0;

				while(*pInput)
				{
					//dbg_msg("Client", "%s", aTemp);
					int Size = str_utf8_encode(aTemp, static_cast<const unsigned char>(*pInput++));

					if((unsigned int)(Length+Size) < sizeof(aData))
					{
						mem_copy(aData+Length, aTemp, Size);
						Length += Size;
					}
					else
						break;
				}
				aData[Length] = 0;

				pSelf->m_pConsole->ExecuteLineFlag(aData, CFGFLAG_CLIENT);
			}
			else
				pSelf->m_pConsole->ExecuteLineFlag(pInput, CFGFLAG_CLIENT);
#else
		pSelf->m_pConsole->ExecuteLineFlag(pInput, CFGFLAG_CLIENT);
#endif

	}
}

void CClient::LuaCheckDrawingState(lua_State *L, const char *pFuncName, bool NoThrow)
{
	Graphics()->LuaCheckDrawingState(L, pFuncName, NoThrow);
}
