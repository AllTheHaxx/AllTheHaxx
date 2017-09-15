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

#include <engine/external/astar-algorithm-cpp/worldmap.h>
#include <engine/external/astar-algorithm-cpp/mapsearchnode.h>
#include <engine/external/astar-algorithm-cpp/stlastar.h>
#include <engine/graphics.h>

#include "astar.h"
#include "effects.h"
#include "hud.h"
#include "controls.h"
#include "camera.h"


CAStar::CAStar()
{
	m_MapReloaded = false;
	m_ThreadsShouldExit = false;
	m_pBuilderThread = NULL;
	m_pScoreThread = NULL;
	m_pCurrentMapGrid = NULL;
	OnReset();
}

void CAStar::OnConsoleInit()
{
	Console()->Register("path_to_mouse", "", CFGFLAG_CLIENT, ConPathToMouse, this, "Try finding a path to your mouse cursor (use unlocked mouse!)");
}

void CAStar::OnShutdown()
{
	if(m_pBuilderThread || m_pScoreThread)
		dbg_msg("astar", "waiting for threads to finish...");
	OnReset();
}

void CAStar::OnReset() // is being called right after OnMapLoad()
{
	StopThreads();
	mem_zerob(m_aCurrentMap);
	m_Path.clear();
	m_LastClosestNode = -1;
	m_LastPos = vec2(0);
	delete m_pCurrentMapGrid;
	m_pCurrentMapGrid = NULL;
}

void CAStar::OnPlayerDeath()
{
	if(!PathFound() || !g_Config.m_ClPathFinding || m_LastPos == vec2(0))
		return;

	m_pScoreThread = thread_init(CalcScoreThreadProxy, this);
}

void CAStar::OnRender()
{
	if(!g_Config.m_ClPathFinding || Client()->State() != IClient::STATE_ONLINE)
		return;

	// find the path one second after joining to be buffered
	if(IsRace(Client()->GetServerInfo()) || IsDDNet(Client()->GetServerInfo()));
	{
		static int64 s_ActivationTime = 0;
		if(m_MapReloaded)
		{
			s_ActivationTime = time_get();
			m_MapReloaded = false;
		}

		if(s_ActivationTime && time_get() > s_ActivationTime+time_freq())
		{
			BuildPathRace();
			s_ActivationTime = 0;
		}
	}

	// don't interfere with the builder thread
	LOCK_SECTION_LAZY_DBG(m_PathLock);
	if(!__SectionLock.TryToLock())
		return;

	// check if there is anything to be shown
	if(m_Path.empty())
		return;

	// set up pointers
	const CNetObj_Character *pPlayerChar = m_pClient->m_Snap.m_pLocalCharacter;
	const CNetObj_Character *pPrevChar = m_pClient->m_Snap.m_pLocalPrevCharacter;

	if (pPlayerChar && pPrevChar && !m_pScoreThread)
		m_LastPos = mix(vec2(pPrevChar->m_X, pPrevChar->m_Y), vec2(pPlayerChar->m_X, pPlayerChar->m_Y), Client()->IntraGameTick());

	const bool DebugVisualisation = g_Config.m_Debug && g_Config.m_DbgAStar;

	// visualize the path
	Graphics()->BlendAdditive();
	Graphics()->TextureSet(DebugVisualisation ? -1 : g_pData->m_aImages[IMAGE_PARTICLES].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.9f, 0.9f, 0.9f, 0.85f);

	for(int i = m_Path.size()-1; i >= 0; i--)
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
		if(distance(GameClient()->m_pCamera->m_Center, m_Path[i].m_Pos) > 1000)
			continue;

		static const int aSprites[] = {SPRITE_PART_SPLAT01, SPRITE_PART_SPLAT02, SPRITE_PART_SPLAT03};
		RenderTools()->SelectSprite(aSprites[i%3]);
		if(!DebugVisualisation || g_Config.m_Debug > 1) Graphics()->QuadsSetRotation((float)i/3.1415f);
		IGraphics::CQuadItem QuadItem(m_Path[i].m_Pos.x, m_Path[i].m_Pos.y, DebugVisualisation ? 30 : 16, DebugVisualisation ? 30 : 16);
		Graphics()->QuadsDraw(&QuadItem, 1);
	}
	Graphics()->QuadsEnd();


	// debug
	if(DebugVisualisation)
	{
		Graphics()->BlendNormal();
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		int Height = m_pCurrentMapGrid->GetHeight();
		int Width = m_pCurrentMapGrid->GetWidth();
		for(int y = 0; y < Height; y++)
			for(int x = 0; x < Width; x++)
			{
				// don't render out of view
				if(distance(GameClient()->m_pCamera->m_Center, vec2(x*32, y*32)) > 1000)
					continue;

				switch(m_pCurrentMapGrid->GetField(x, y))
				{
					case COST_AIR: Graphics()->SetColor(1,1,1,0.15f); break;
					case COST_NEAR_FREEZE: Graphics()->SetColor(0,0,0,0.25f); break;
					case COST_FREEZE: Graphics()->SetColor(0,0,0.5f,0.25f); break;
					case COST_NEAR_DEATH: Graphics()->SetColor(0.5f,0,0,0.25f); break;
					case COST_SOLID: Graphics()->SetColor(0,0,0,0.50f); break;
					default: Graphics()->SetColor(1,0,1,0.5f); break;
				}

				IGraphics::CQuadItem QuadItem(x*32, y*32, 30, 30);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
			}
		Graphics()->QuadsEnd();
	}
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

CAStarWorldMap* CAStar::FillGrid(CAStarWorldMap *pMap)
{
	const int Height = pMap->GetHeight();
	const int Width = pMap->GetWidth();

	dbg_msg("astar", "started filling grid. width=%i height=%i size=%i", Width, Height, Width*Height);

	// feed the grid with data from the map
	for(int iy = 0; iy < Height; iy++)
	{
		for(int ix = 0; ix < Width; ix++)
		{
			if(m_ThreadsShouldExit)
				return pMap;

			int x = ix * 32 + 16;
			int y = iy * 32 + 16;

			if(Collision()->GetTileRaw(x, y) == TILE_FREEZE)
			{
				pMap->AddNext(COST_FREEZE); // 5 to prevent freeze if possible
			}
			else if(Collision()->CheckPoint(x, y) || Collision()->GetTileRaw(x, y) == TILE_STOP || // TODO: have a clue how to handle one-way stop tiles + implement it.
					Collision()->GetTileRaw(x, y) == TILE_DEATH)
			{
				pMap->AddNext(COST_SOLID); // 9 means not passable (solid)
			}
			else if(Collision()->GetTileRaw(x, y-32) == TILE_FREEZE || Collision()->GetTileRaw(x+32, y) == TILE_FREEZE ||
					Collision()->GetTileRaw(x, y+32) == TILE_FREEZE || Collision()->GetTileRaw(x-32, y) == TILE_FREEZE)
			{
				pMap->AddNext(COST_NEAR_FREEZE); // we're not really keen on going right next to freeze tiles
			}
			else if(Collision()->GetTileRaw(x, y-32) == TILE_DEATH || Collision()->GetTileRaw(x+32, y) == TILE_DEATH ||
					Collision()->GetTileRaw(x, y+32) == TILE_DEATH || Collision()->GetTileRaw(x-32, y) == TILE_DEATH)
			{
				pMap->AddNext(COST_NEAR_DEATH); // death tiles are more scary than freeze
			}
			else
			{
				pMap->AddNext(COST_AIR); // 0 is flawlessly passable (air)
			}
		}
	}

	return pMap;
}

void CAStar::ScanMap()
{
	dbg_assert_strict(m_pCurrentMapGrid == NULL, "[pathfinding] double-scanned map?!");
	delete m_pCurrentMapGrid; // for release-mode; deleting nullpointer doesn't do anything

	m_pCurrentMapGrid = new CAStarWorldMap(Collision()->GetWidth(), Collision()->GetHeight());
	FillGrid(m_pCurrentMapGrid);
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
		dbg_msg("astar", "waiting for score thread to finish...");
		thread_destroy(m_pScoreThread);
		m_pScoreThread = NULL;
	}
	if(m_pBuilderThread)
	{
		dbg_msg("astar", "waiting for builder thread to finish...");
		thread_destroy(m_pBuilderThread);
		m_pBuilderThread = NULL;
	}
	m_ThreadsShouldExit = false;
}

// THREADS

/* function copied and edited from https://github.com/justinhj/astar-algorithm-cpp/blob/master/cpp/findpath.cpp */
void CAStar::BuildPath(void *pData)
{
	CPathBuilderParams *pParams = (CPathBuilderParams*)pData;
	CAStar *pSelf = pParams->m_pSelf;
	const vec2 Start = pParams->m_From;
	const vec2 Finish = pParams->m_To;
	delete pParams;

	DEFER(pSelf, [](void *pUser){
		((CAStar*)pUser)->m_pBuilderThread = NULL;
	})

	if(!g_Config.m_ClPathFinding)
		return;

	int SolutionLength = -1;
	float SolutionCost = -1;

	if(!(pSelf->m_pCurrentMapGrid))
		pSelf->ScanMap();

	CAStarSearch<CAStarMapSearchNode> astarsearch((unsigned int)(pSelf->m_pCurrentMapGrid->GetSize()));

	// Create a start state
	CAStarMapSearchNode nodeStart;
	nodeStart.m_X = (int)Start.x;
	nodeStart.m_Y = (int)Start.y;

	// Define the goal state
	CAStarMapSearchNode nodeEnd;
	nodeEnd.m_X = (int)Finish.x;
	nodeEnd.m_Y = (int)Finish.y;

	// Set Start and goal states

	astarsearch.SetStartAndGoalStates( pSelf->m_pCurrentMapGrid, nodeStart, nodeEnd );

	unsigned int SearchState;
	unsigned int SearchSteps = 0;

	dbg_msg("astar", "starting search. Path: %s to %s", Start.tostring().c_str(), Finish.tostring().c_str());

	do
	{
		if(pSelf->m_ThreadsShouldExit)
		{
			astarsearch.CancelSearch();
		}

		SearchState = astarsearch.SearchStep();
		SearchSteps++;
	}
	while( SearchState == CAStarSearch<CAStarMapSearchNode>::SEARCH_STATE_SEARCHING );

	LOCK_SECTION_DBG(pSelf->m_PathLock);

	if( SearchState == CAStarSearch<CAStarMapSearchNode>::SEARCH_STATE_SUCCEEDED )
	{
		dbg_msg("astar", "Search found goal state in %i iterations", SearchSteps);

		CAStarMapSearchNode *node = dynamic_cast<CAStarMapSearchNode *>(astarsearch.GetSolutionStart());

		SolutionLength = 0;
		SolutionCost = astarsearch.GetSolutionCost();

		pSelf->m_Path.clear();
		pSelf->m_LastClosestNode = -1;

		for( ;; )
		{
			if(pSelf->m_ThreadsShouldExit)
			{
				pSelf->m_Path.clear();
				SolutionLength = -1;
				break;
			}

			pSelf->m_Path.add_unsorted(Node(SolutionLength, vec2(node->m_X*32+16, node->m_Y*32+16)));

			node = dynamic_cast<CAStarMapSearchNode *>(astarsearch.GetSolutionNext());

			SolutionLength++;

			if( !node )
			{
				break;
			}
		};

		pSelf->m_Path.sort_range();
		//for(int i = 0; i < pSelf->m_Path.size(); i++)
		//	dbg_msg("astar/dbg", "Node #%03i (%03i) %s", i, pSelf->m_Path[i].m_ID, pSelf->m_Path[i].m_Pos.tostring().c_str());

		if(pSelf->m_Path.empty())
		{
			dbg_msg("astar", "Solution found but analysis was aborted.");
		}
		else
			dbg_msg("astar", "Solution steps %i (path length %i)", SolutionLength, pSelf->m_Path.size());

		// Once you're done with the solution you can free the nodes up
		astarsearch.FreeSolutionNodes();


	}
	else if( SearchState == CAStarSearch<CAStarMapSearchNode>::SEARCH_STATE_FAILED )
	{
		dbg_msg("astar", "Search terminated. Did %i iterations but failed to find goal state", SearchSteps);
	}
	else if( SearchState == CAStarSearch<CAStarMapSearchNode>::SEARCH_STATE_OUT_OF_MEMORY )
	{
		dbg_msg("astar", "Search terminated. Engine reported out of memory after %i iterations!", SearchSteps);
	}

	astarsearch.EnsureMemoryFreed();


	if(SolutionLength > -1)
	{
		dbg_msg("path", "found path: start=%s, finish=%s, length=%i, cost=%.2f", Start.tostring().c_str(), Finish.tostring().c_str(), SolutionLength, SolutionCost);
	}


	if(pSelf->m_ThreadsShouldExit)
	{
		return;
	}

	if(SolutionLength > -1)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Found path. Length: %i, Cost: %.2f", SolutionLength, SolutionCost);
		pSelf->m_pClient->m_pHud->PushNotification(aBuf);
	}
	else
		pSelf->m_pClient->m_pHud->PushNotification("No possible path found.");

}

void CAStar::CalcScoreThreadProxy(void *pUser)
{
	CAStar *pSelf = (CAStar*)pUser;
	pSelf->CalcScoreThread();
}

void CAStar::CalcScoreThread()
{
	DEFER(this, [](void *pUser){
		((CAStar*)pUser)->m_pScoreThread = NULL;
	})

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

		if((ClosestNodeDist < 0.0f || d < ClosestNodeDist))
			if(!Collision()->IntersectLine(m_LastPos, m_Path[i].m_Pos, 0x0, 0x0))
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
		str_format(aBuf, sizeof(aBuf), "Fitness Score: %i/%i (%.2f%%)", ClosestID, m_Path.size(), (((float)ClosestID)/(float)m_Path.size())*100.0f);
		m_pClient->m_pHud->PushNotification(aBuf);
	}
}

void CAStar::BuildPathRace()
{
	vec2 Start, Finish;
	if(!GetStart(&Start) || !GetFinish(&Finish))
	{
		dbg_msg("path", "missing start or finish");
		return;
	}

	InitPathBuilder(Start, Finish);
}

void CAStar::ConPathToMouse(IConsole::IResult *pResult, void *pUserData)
{
	CAStar *pSelf = (CAStar*)pUserData;

	vec2 Start(
			pSelf->m_pClient->m_Snap.m_pLocalCharacter->m_X/32.0f,
			pSelf->m_pClient->m_Snap.m_pLocalCharacter->m_Y/32.0f
	);
	vec2 Finish(
			(pSelf->m_pClient->m_Snap.m_pLocalCharacter->m_X+pSelf->m_pClient->m_pControls->m_MousePos[g_Config.m_ClDummy].x)/32.0f,
			(pSelf->m_pClient->m_Snap.m_pLocalCharacter->m_Y+pSelf->m_pClient->m_pControls->m_MousePos[g_Config.m_ClDummy].y)/32.0f
	);

	pSelf->InitPathBuilder(Start, Finish);
}

void CAStar::InitPathBuilder(const vec2& From, const vec2& To)
{
	StopThreads();

	CPathBuilderParams *pParams = new CPathBuilderParams(this, From, To);
	m_pBuilderThread = thread_init_named(BuildPath, pParams, "A* path finder");
}
