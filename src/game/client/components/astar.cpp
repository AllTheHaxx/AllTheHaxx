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

CAStar::CAStar()
{
	m_MapReloaded = false;
	m_ThreadsShouldExit = false;
	m_pBuilderThread = NULL;
	m_pScoreThread = NULL;
	OnReset();
}

void CAStar::OnShutdown()
{
	dbg_msg("astar", "waiting for threads to finish...");
	StopThreads();
}

void CAStar::OnReset() // is being called right after OnMapLoad()
{
	m_Path.clear();
	m_LastClosestNode = -1;
	m_LastPos = vec2(0);
}

void CAStar::OnPlayerDeath()
{
	if(!PathFound() || !g_Config.m_ClPathFinding || m_LastPos == vec2(0))
		return;

	m_pScoreThread = thread_init(StartCalcScoreThread, this);
}

void CAStar::OnRender()
{
	if(!g_Config.m_ClPathFinding || Client()->State() != IClient::STATE_ONLINE)
		return;

	// find the path one second after joining to be buffered
	{
		static int64 s_ActivationTime = 0;
		if(m_MapReloaded)
		{
			s_ActivationTime = time_get();
			m_MapReloaded = false;
		}

		if(s_ActivationTime && time_get() > s_ActivationTime+time_freq())
		{
			m_pBuilderThread = thread_init_named(BuildPath, this, "astar builder");
			s_ActivationTime = 0;
		}
	}


	const CNetObj_Character *pPlayerChar = m_pClient->m_Snap.m_pLocalCharacter;
	const CNetObj_Character *pPrevChar = m_pClient->m_Snap.m_pLocalPrevCharacter;

	if (pPlayerChar && pPrevChar)
		m_LastPos = mix(vec2(pPrevChar->m_X, pPrevChar->m_Y), vec2(pPlayerChar->m_X, pPlayerChar->m_Y), Client()->IntraGameTick());

	// visualize the path
	Graphics()->BlendAdditive();
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PARTICLES].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.9f, 0.9f, 0.9f, 0.85f);

	for(int i = 0; i < m_Path.size(); i++)
	{
		if(i == m_LastClosestNode && g_Config.m_ClPathFindingColor)
		{
			Graphics()->QuadsEnd();
			Graphics()->BlendNormal();
			Graphics()->QuadsBegin();
			Graphics()->SetColor(
					(float)g_Config.m_ClPathFindingColorR/100.0f,
					(float)g_Config.m_ClPathFindingColorG/100.0f,
					(float)g_Config.m_ClPathFindingColorB/100.0f,
					(float)g_Config.m_ClPathFindingColorA/100.0f
			);
		}

		// don't render out of view
		vec2 Pos = GameClient()->m_Snap.m_SpecInfo.m_Active ? vec2(GameClient()->m_Snap.m_pSpectatorInfo->m_X, GameClient()->m_Snap.m_pSpectatorInfo->m_X) : m_LastPos;
		if(distance(Pos, m_Path[i].m_Pos) > 1000)
			continue;

		static const int aSprites[] = {SPRITE_PART_SPLAT01, SPRITE_PART_SPLAT02, SPRITE_PART_SPLAT03};
		RenderTools()->SelectSprite(aSprites[i%3]);
		//Graphics()->QuadsSetRotation(Client()->GameTick());
		IGraphics::CQuadItem QuadItem(m_Path[i].m_Pos.x, m_Path[i].m_Pos.y, 16, 16);

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
				if(NumTiles >= sizeof(aTiles))
					break;
				aTiles[NumTiles++] = iy*w+ix;
			}
		}
	}

	int middle = NumTiles > 0 ? aTiles[NumTiles/2] : -1;
	//mem_free(aTiles);
	return middle;
}


char *CAStar::FillGrid(bool NoFreeze) // NoFreeze: do not go through freeze tiles
{
	// feed the grid with data from the map
	char *pField = (char *)mem_alloc(Collision()->GetWidth() * Collision()->GetHeight() * sizeof(char), 1);
	for(int y = 0; y < Collision()->GetHeight()-1; y++)
	{
		for(int x = 0; x < Collision()->GetWidth()-1; x++)
		{
			if(m_ThreadsShouldExit)
				return pField;

			pField[y*Collision()->GetWidth()+x] = (NoFreeze && Collision()->GetTileRaw(x * 32, y * 32) == TILE_FREEZE) || // treat freeze as air if allowed
					!(
							Collision()->CheckPoint(x * 32, y * 32) || // the following makes sure that the path isn't going through edge-passages
									(Collision()->CheckPoint((x-1) * 32, y * 32) && Collision()->CheckPoint(x * 32, (y-1) * 32)) ||
									(Collision()->CheckPoint((x-1) * 32, y * 32) && Collision()->CheckPoint(x * 32, (y+1) * 32)) ||
									(Collision()->CheckPoint((x+1) * 32, y * 32) && Collision()->CheckPoint(x * 32, (y-1) * 32)) ||
									(Collision()->CheckPoint((x+1) * 32, y * 32) && Collision()->CheckPoint(x * 32, (y+1) * 32))
					);
		}
	}

	return pField;
}

void CAStar::OnStateChange(int NewState, int OldState)
{
	StopThreads();
	m_Path.clear();
}

void CAStar::OnMapLoad()
{
	m_MapReloaded = true;
}

void CAStar::StopThreads()
{
	m_ThreadsShouldExit = true;
	if(m_pScoreThread)
	{
		thread_destroy(m_pScoreThread);
		m_pScoreThread = 0;
	}
	if(m_pBuilderThread)
	{
		thread_destroy(m_pBuilderThread);
		m_pBuilderThread = 0;
	}
	m_ThreadsShouldExit = false;
}

// THREADS

void CAStar::BuildPath(void *pUser)
{
	CAStar* pSelf = (CAStar*)pUser;

	{
		CServerInfo Info; pSelf->Client()->GetServerInfo(&Info);
		if(!g_Config.m_ClPathFinding || (!IsRace(&Info) && !IsDDNet(&Info)))
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
		char *pField = pSelf->FillGrid(false);
		pSolution = astar_compute((const char *)pField, &SolutionLength, pSelf->Collision()->GetWidth(), pSelf->Collision()->GetHeight(), Start, Finish);
		mem_free(pField);
		dbg_msg("path", "start=%i, finish=%i, length=%i", Start, Finish, SolutionLength);
	}

	if(pSelf->m_ThreadsShouldExit)
	{
		if(pSolution)
			free(pSolution);
		return;
	}

	if(SolutionLength == -1) // try again, ignoring freeze
	{
		char *pField = pSelf->FillGrid(false);
		pSolution = astar_compute((const char*)pField, &SolutionLength, pSelf->Collision()->GetWidth(), pSelf->Collision()->GetHeight(), Start, Finish);
		mem_free(pField);
		dbg_msg("path", "ignored freeze: start=%i, finish=%i, length=%i", Start, Finish, SolutionLength);
	}

	if(pSelf->m_ThreadsShouldExit)
	{
		if(pSolution)
			free(pSolution);
		return;
	}

	if(SolutionLength != -1)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Found path. Length: %i", SolutionLength);
		pSelf->m_pClient->m_pHud->PushNotification(aBuf);
	}
	else
		pSelf->m_pClient->m_pHud->PushNotification("No possible path found.");

	if(pSolution)
	{
		if(SolutionLength > 0)
		{
			for(int i = SolutionLength; i >= 0 ; i--)
			{
				pSelf->m_Path.add_unsorted(Node(i-SolutionLength, pSelf->Collision()->GetPos(pSolution[i])));
				if(pSelf->m_ThreadsShouldExit)
				{
					free(pSolution);
					return;
				}
			}
			pSelf->m_Path.sort_range();
		}
		free(pSolution);
	}
}

void CAStar::StartCalcScoreThread(void *pUser)
{
	CAStar *pSelf = (CAStar*)pUser;
	pSelf->CalcScoreThread();
}

void CAStar::CalcScoreThread()
{
	// fitness calculation
	float ClosestNodeDist = -1.0f;
	int ClosestID = -1;
	for(int i = m_Path.size()-1; i >= 0; i--)
	{
		if(m_ThreadsShouldExit)
			return;

//		dbg_msg("debug", "LAST=(%.2f %.2f) ITER(%i)=(%.2f %.2f)", m_LastPos.x, m_LastPos.y, i, m_Path[i].m_Pos.x, m_Path[i].m_Pos.y);
		float d = distance(m_LastPos, m_Path[i].m_Pos);
		if(ClosestNodeDist >= 0.0f && d > ClosestNodeDist)
		{
//			dbg_msg("debug", "abort condition met; %.2f > %.2f", d, ClosestNodeDist);
			break;
		}

		if((ClosestNodeDist < 0.0f || d < ClosestNodeDist) && !Collision()->IntersectLine(m_LastPos, m_Path[i].m_Pos, 0x0, 0x0))
		{
			ClosestNodeDist = d;
			ClosestID = i;
//			dbg_msg("FOUND NEW CLOSEST NODE", "i=%i with dist=%.2f", ClosestID, ClosestNodeDist);
		}
	}

	m_LastClosestNode = ClosestID;
	if(ClosestID > 0)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Fitness Score: %i/%i (%.2f%%)", m_Path.size()-ClosestID, m_Path.size(), (((float)m_Path.size()-ClosestID)/(float)m_Path.size())*100.0f);
		m_pClient->m_pHud->PushNotification(aBuf);
	}
}
