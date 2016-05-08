#include <base/system.h>
#include <base/math.h>
#include <base/tl/array.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>
#include <engine/shared/config.h>

#include <game/collision.h>
#include <game/mapitems.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>

#include <engine/external/astar-jps/AStar.h>
#include <engine/graphics.h>

#include "astar.h"
#include "effects.h"
#include "hud.h"

// TODO: make all of this shit optional

CAStar::CAStar()
{
	m_pField = NULL;
	m_PathFound = false;
	m_MapReloaded = false;
	m_ThreadShouldExit = false;
	m_pThread = 0;
	OnReset();
}

CAStar::~CAStar()
{
	if(m_pField)
		mem_free(m_pField);
}

void CAStar::OnReset() // is being called right after OnMapLoad()
{
	m_Path.clear();
	m_PathFound = false;
}

void CAStar::OnPlayerDeath() // TODO!! FIX THIS!!
{
	if(!m_PathFound || !g_Config.m_ClPathFinding)
		return;

	// fitness calculation
	float ClosestNode = -1.0f;
	int ClosestID = -1;
	for(int i = m_Path.size(); i >= 0; i--)
	{
		//dbg_msg("debug", "LAST=(%.2f %.2f) ITER(%i)=(%.2f %.2f)", m_LastPos.x, m_LastPos.y, i, m_Path[i].x, m_Path[i].y);
		if((distance<float>(m_LastPos, m_Path[i]) < ClosestNode || ClosestNode < 0.0f) && !Collision()->IntersectLine(m_LastPos, m_Path[i], 0x0, 0x0))
		{
			ClosestNode = distance<float>(m_LastPos, m_Path[i]);
			ClosestID = i;
			//dbg_msg("FOUND NEW CLOSEST NODE", "i=%i with dist=%.2f", ClosestID, ClosestNode);
		}
	}

	if(g_Config.m_ClNotifications && ClosestID != -1)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Fitness Score: %i/%i (%.2f%%)", m_Path.size()-ClosestID, m_Path.size(), ((float)(m_Path.size()-ClosestID)/(float)m_Path.size())*100.0f);
		m_pClient->m_pHud->PushNotification(aBuf);
	}
}

void CAStar::OnRender()
{
	if(!g_Config.m_ClPathFinding)
		return;

	// find the path one second after joining to be buffered
	{
		static int64 activationTime = 0;
		if(m_MapReloaded)
		{
			activationTime = time_get();
		}

		if(activationTime && time_get() > activationTime+time_freq())
		{
			//FillGrid(true);
			m_pThread = thread_init(BuildPath, this);
			//thread_detach(m_pThread);
			activationTime = 0;
		}
		else
			m_MapReloaded = false;
	}


	const CNetObj_Character * pPlayerChar = m_pClient->m_Snap.m_pLocalCharacter;
	const CNetObj_Character * pPrevChar = m_pClient->m_Snap.m_pLocalPrevCharacter;

	if (pPlayerChar && pPrevChar)
		m_LastPos = mix(vec2(pPrevChar->m_X, pPrevChar->m_Y), vec2(pPlayerChar->m_X, pPlayerChar->m_Y), Client()->IntraGameTick());

	if(Client()->State() != IClient::STATE_ONLINE)
		return;

	// visualize the path
	Graphics()->BlendAdditive();
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PARTICLES].m_Id);
	Graphics()->QuadsBegin();

	for(int i = 0; i < m_Path.size(); i++)
	{
		int aSprites[] = {SPRITE_PART_SPLAT01, SPRITE_PART_SPLAT02, SPRITE_PART_SPLAT03};
		RenderTools()->SelectSprite(aSprites[i%3]);
		//Graphics()->QuadsSetRotation(Client()->GameTick());
		Graphics()->SetColor(0.9f, 0.9f, 0.9f, 0.85f);
		IGraphics::CQuadItem QuadItem(m_Path[i].x, m_Path[i].y, 16, 16);
		Graphics()->QuadsDraw(&QuadItem, 1);
	}
	Graphics()->QuadsEnd();
}

int CAStar::GetTileAreaCenter(int TileID, int x, int y, int w, int h)
{
	if(x < 0 || y < 0)
		return -1;

	if(w < 1 || w > Collision()->GetWidth()-x)
		w = Collision()->GetWidth()-x;
	if(h < 1 || h > Collision()->GetHeight()-y)
		h = Collision()->GetHeight()-y;

	size_t NumTiles = 0;
	//int *aTiles = (int*)mem_alloc((w-x)*(h-y)*sizeof(int), 0); // <--that gonna be better but unsafer (leakyleak :0)
	int aTiles[255];

	//dbg_msg("path/tilefinder", "Searching for tile=%i in AREA=(x%02i y%02i w%02i h%02i)", TileID, x, y, w, h);
	for(int iy=y; iy < h; iy++)
	{
		for(int ix=x; ix < w; ix++)
		{
			if(Collision()->GetTileRaw(ix*32, iy*32) == TileID)
			{
				//dbg_msg("path/tilefinder", "Found: tile=%i at x=%i y=%i", TileID, ix, iy);
				aTiles[NumTiles++] = iy*w+ix;
			}
		}
	}

	int middle = NumTiles > 0 ? aTiles[NumTiles/2] : -1;
	//mem_free(aTiles);
	return middle;
}


void CAStar::BuildPath(void *pUser)
{
	CAStar* pSelf = (CAStar*)pUser;

	{
		CServerInfo Info; pSelf->Client()->GetServerInfo(&Info);
		if(!g_Config.m_ClPathFinding || !(IsRace(&Info) || IsDDNet(&Info)))
			return;
	}

	int SolutionLength = -1;
	int *pSolution = 0;
	int Start = pSelf->GetStart();
	int Finish = pSelf->GetFinish();

/*	if(Start == -1)
		dbg_msg("path", "didn't find start tile");
	if(Finish == -1)
		dbg_msg("path", "didn't find finish tile");
*/
	if(Start >= 0 && Finish >= 0)
	{
		pSelf->FillGrid(true);
		pSolution = astar_compute((const char *)pSelf->m_pField, &SolutionLength, pSelf->Collision()->GetWidth(), pSelf->Collision()->GetHeight(), Start, Finish);
		dbg_msg("path", "start=%i, finish=%i, length=%i", Start, Finish, SolutionLength);
	}

	if(SolutionLength == -1) // try again, ignoring freeze
	{
		pSelf->FillGrid(false);
		pSolution = astar_compute((const char *)pSelf->m_pField, &SolutionLength, pSelf->Collision()->GetWidth(), pSelf->Collision()->GetHeight(), Start, Finish);
		dbg_msg("path", "ignored freeze: start=%i, finish=%i, length=%i", Start, Finish, SolutionLength);
	}

	if(g_Config.m_ClNotifications)
	{
		if(SolutionLength != -1)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "Found path. Length: %i", SolutionLength);
			pSelf->m_pClient->m_pHud->PushNotification(aBuf);
		}
		else
			pSelf->m_pClient->m_pHud->PushNotification("No possible path found.");
	}
	
	if(pSolution)
	{
		if(SolutionLength > 0)
		{
			pSelf->m_PathFound = true;
			for(int i = SolutionLength; i >= 0 ; i--)
			{
				pSelf->m_Path.add(pSelf->Collision()->GetPos(pSolution[i]));
				thread_sleep(10);
				if(pSelf->m_ThreadShouldExit)
				{
					return;
				}
			}
		}
		free(pSolution);
	}
}

void CAStar::FillGrid(bool NoFreeze) // NoFreeze: do not go through freeze tiles
{
	if(m_pField)
		mem_free(m_pField);

	m_pField = (char *)mem_alloc(Collision()->GetWidth() * Collision()->GetHeight() * sizeof(char), 1);
	for(int y = 0; y < Collision()->GetHeight(); y++)
	{
		for(int x = 0; x < Collision()->GetWidth(); x++)
		{
			m_pField[y*Collision()->GetWidth()+x] =
					(Collision()->CheckPoint(x * 32, y * 32) || (NoFreeze && Collision()->GetTileRaw(x*32, y*32) == TILE_FREEZE)) ? 0 : 1;
		}
	}
}

void CAStar::OnStateChange(int NewState, int OldState)
{
	if(m_pThread)
	{
		m_ThreadShouldExit = true;
		thread_destroy(m_pThread);
		m_pThread = 0;
		m_ThreadShouldExit = false;
	}
	m_Path.clear();
}

void CAStar::OnMapLoad()
{
	m_MapReloaded = true;
}
