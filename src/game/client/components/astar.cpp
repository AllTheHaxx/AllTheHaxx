#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>
#include <base/tl/array.h>

#include <game/generated/protocol.h>
#include <game/generated/client_data.h>
#include <engine/shared/config.h>

#include <game/collision.h>
#include <game/mapitems.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>

#include <engine/external/astar-algorithm-cpp/stlastar.h>
#include <engine/graphics.h>
#include <astar-algorithm-cpp/stlastar.h>
#include <astar-algorithm-cpp/mapsearchnode.h>
#include <astar-algorithm-cpp/worldmap.h>

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
	if(m_pBuilderThread || m_pScoreThread)
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

bool CAStar::GetTileAreaCenter(vec2 *pResult, int TileID, int x, int y, int w, int h)
{
	if(x < 0 || y < 0 || !pResult)
		return false;

	if(w < 1 || w > Collision()->GetWidth()-x)
		w = Collision()->GetWidth()-x;
	if(h < 1 || h > Collision()->GetHeight()-y)
		h = Collision()->GetHeight()-y;

	size_t NumTiles = 0;
	//int *aTiles = (int*)mem_alloc((w-x)*(h-y)*sizeof(int), 0); // <--that gonna be better but unsafer (leakyleak :0)
	const unsigned MAX_TILES_IN_AREA = 255;
	vec2 aTiles[MAX_TILES_IN_AREA];

	//dbg_msg("path/tilefinder", "Searching for tile=%i in AREA=(x%02i y%02i w%02i h%02i)", TileID, x, y, w, h);
	for(int iy=y; iy < h; iy++)
	{
		for(int ix=x; ix < w; ix++)
		{
			if(Collision()->GetTileRaw(ix*32, iy*32) == TileID)
			{
				//dbg_msg("path/tilefinder", "Found: tile=%i at x=%i y=%i", TileID, ix, iy);
				aTiles[NumTiles++] = vec2(ix, iy);
				if(NumTiles >= MAX_TILES_IN_AREA)
					break;
			}
		}
	}

	if(NumTiles > 0)
	{
		*pResult = aTiles[NumTiles/2];
		return true;
	}
	return false;
}


AStarWorldMap* CAStar::FillGrid(AStarWorldMap *pMap)
{
	const int Height = pMap->GetHeight();
	const int Width = pMap->GetWidth();


	// feed the grid with data from the map
	for(int y = 0; y < Height-1; y++)
	{
		for(int x = 0; x < Width-1; x++)
		{
			if(m_ThreadsShouldExit)
				return pMap;

			if(Collision()->GetTileRaw(x * 32, y * 32) == TILE_FREEZE)
				pMap->AddNext(5); // 5 to prevent freeze if possible
			else if(
						!(
								Collision()->CheckPoint(x * 32, y * 32) || // the following makes sure that the path isn't going through edge-passages
										(Collision()->CheckPoint((x-1) * 32, y * 32) && Collision()->CheckPoint(x * 32, (y-1) * 32)) ||
										(Collision()->CheckPoint((x-1) * 32, y * 32) && Collision()->CheckPoint(x * 32, (y+1) * 32)) ||
										(Collision()->CheckPoint((x+1) * 32, y * 32) && Collision()->CheckPoint(x * 32, (y-1) * 32)) ||
										(Collision()->CheckPoint((x+1) * 32, y * 32) && Collision()->CheckPoint(x * 32, (y+1) * 32))
						) || (
								Collision()->GetTileRaw(x * 32, y * 32) == TILE_STOP // I have no clue how we could handle one-way stop tiles :o
								// TODO: have a clue how to handle one-way stop tiles + implement it.
						)
					)
				pMap->AddNext(9); // 9 means not passable
			else
				pMap->AddNext(0); // 0 is flawlessly passable (like air)
		}
	}

	return pMap;
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


/* function copied and edited from https://github.com/justinhj/astar-algorithm-cpp/blob/master/cpp/findpath.cpp */
void CAStar::BuildPath(void *pUser)
{
	CAStar *pSelf = (CAStar*)pUser;

	{
		const CServerInfo *pInfo = pSelf->Client()->GetServerInfo();
		if(!g_Config.m_ClPathFinding || (!IsRace(pInfo) && !IsDDNet(pInfo)))
			return;
	}

	int SolutionLength = -1;
	float SolutionCost = -1;
	vec2 Start, Finish;

	if(!pSelf->GetStart(&Start) || !pSelf->GetFinish(&Start))
	{
		dbg_msg("path", "missing start or finish");
		return;
	}

	AStarWorldMap Field(pSelf->Collision()->GetWidth(), pSelf->Collision()->GetHeight());
	pSelf->FillGrid(&Field);

	AStarSearch<AStarMapSearchNode> astarsearch;

	// Create a start state
	AStarMapSearchNode nodeStart;
	nodeStart.x = (int)Start.x;
	nodeStart.y = (int)Start.y;

	// Define the goal state
	AStarMapSearchNode nodeEnd;
	nodeEnd.x = (int)Finish.x;
	nodeEnd.y = (int)Finish.y;

	// Set Start and goal states

	astarsearch.SetStartAndGoalStates( &Field, nodeStart, nodeEnd );

	unsigned int SearchState;
	unsigned int SearchSteps = 0;

	dbg_msg("astar", "starting search. Path: %s to %s", Start.tocstring(), Finish.tocstring());

	do
	{
		if(pSelf->m_ThreadsShouldExit)
		{
			return;
		}

		SearchState = astarsearch.SearchStep();
		SearchSteps++;
	}
	while( SearchState == AStarSearch<AStarMapSearchNode>::SEARCH_STATE_SEARCHING );

	if( SearchState == AStarSearch<AStarMapSearchNode>::SEARCH_STATE_SUCCEEDED )
	{
		dbg_msg("astar", "Search found goal state");

		AStarMapSearchNode *node = astarsearch.GetSolutionStart();

		int steps = 0;

		SolutionLength = astarsearch.GetStepCount();
		SolutionCost = astarsearch.GetSolutionCost();

		pSelf->m_Path.clear();

		for( ;; )
		{
			if(pSelf->m_ThreadsShouldExit)
			{
				return;
			}

			pSelf->m_Path.add_unsorted(Node(steps, vec2(node->x, node->y)));

			node = astarsearch.GetSolutionNext();

			if( !node )
			{
				break;
			}

			steps ++;
		};

		pSelf->m_Path.sort_range();


		dbg_msg("astar", "Solution steps %i / path length %i", steps, pSelf->m_Path.size());

		// Once you're done with the solution you can free the nodes up
		astarsearch.FreeSolutionNodes();


	}
	else if( SearchState == AStarSearch<AStarMapSearchNode>::SEARCH_STATE_FAILED )
	{
		dbg_msg("astar", "Search terminated. Did not find goal state");
	}

	if(g_Config.m_Debug)
		dbg_msg("astar", "SearchSteps : %i", SearchState);

	astarsearch.EnsureMemoryFreed();


	if(SolutionLength > -1)
	{
		dbg_msg("path", "found path: start=%s, finish=%s, length=%i, cost=%.2f", Start.tocstring(), Finish.tocstring(), SolutionLength, SolutionCost);
	}


	if(pSelf->m_ThreadsShouldExit)
	{
		return;
	}

	if(SolutionLength > -1)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Found path. Length: %i", SolutionLength);
		pSelf->m_pClient->m_pHud->PushNotification(aBuf);
	}
	else
		pSelf->m_pClient->m_pHud->PushNotification("No possible path found.");

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
