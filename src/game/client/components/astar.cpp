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
	OnReset();
}

CAStar::~CAStar()
{

}

void CAStar::OnReset() // is being called right after OnMapLoad()
{
	m_Path.clear();
	m_LineItems.clear();
	m_PathFound = false;
}

void CAStar::OnPlayerDeath()
{
	if(!m_PathFound)
		return;

	// fitness calculation
	int ClosestNode = 1337*1337;
	int ClosestID = -1;
	for(int i = m_Path.size(); i >= 0; i--)
	{
		// better make a SAFE CAAAAAAALLLL
		if(distance(m_Path[i], m_LastPos) < ClosestNode && !Collision()->IntersectLine(m_Path[i], m_LastPos, 0x0, 0x0, false))
		{
			ClosestNode = distance(m_Path[i], m_LastPos);
			ClosestID = i;
		}
	}

	if(g_Config.m_ClNotifications && ClosestID != -1)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "Fitness Score: %i (%.2f%%)", m_Path.size()-ClosestID, (((float)m_Path.size()-(float)ClosestID)/(float)m_Path.size())*100.0f);
		m_pClient->m_pHud->PushNotification(aBuf);
	}
}

void CAStar::OnRender()
{
	// find the path one second after joining to be buffered
	{
		static int64 activationTime = 0;
		if(m_MapReloaded)
		{
			activationTime = time_get();
		}

		if(activationTime && time_get() > activationTime+time_freq())
		{
			FillGrid(true);
			BuildPath();
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
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	for(int i = 0; i < m_Path.size()-1; i++)
	{
		vec3 RGB;
		vec2 Pos = vec2(m_Path[i+1].x, m_Path[i+1].y);
		vec2 From = vec2(m_Path[i].x, m_Path[i].y);
		vec2 Dir = normalize(Pos-From);

		vec2 Out, Border;

		//vec4 inner_color(0.15f,0.35f,0.75f,1.0f);
		//vec4 outer_color(0.65f,0.85f,1.0f,1.0f);

		// do outline
		Graphics()->SetColor(0.95f, 0.05f, 0.1f, 0.55f);
		Out = vec2(Dir.y, -Dir.x) * (3.0f);

		IGraphics::CFreeformItem Freeform(
				From.x-Out.x, From.y-Out.y,
				From.x+Out.x, From.y+Out.y,
				Pos.x-Out.x, Pos.y-Out.y,
				Pos.x+Out.x, Pos.y+Out.y);
		Graphics()->QuadsDrawFreeform(&Freeform, 1);
	}
	Graphics()->QuadsEnd();
}

int CAStar::GetStart()
{
	int NumTiles = 0;
	int aTiles[128];

	for(int y = 0; y < Collision()->GetHeight(); y++)
	{
		for(int x = 0; x < Collision()->GetWidth(); x++)
		{
			// hack. but apperently this can indeed happen
			if(x < Collision()->GetWidth() && y < Collision()->GetHeight() && Collision()->GetIndex(x, y) == TILE_BEGIN)
				aTiles[NumTiles++] = Collision()->GetIndex(vec2(x*32, y*32));
		}
	}

	if(NumTiles)
		return aTiles[NumTiles/2]; // some tile from the middle
	else
		return -1;
}

int CAStar::GetFinish()
{
	int NumTiles = 0;
	int aTiles[128];

	for(int y = 0; y < Collision()->GetHeight(); y++)
	{
		for(int x = 0; x < Collision()->GetWidth(); x++)
		{
			// hack. but apperently this can indeed happen
			if(x < Collision()->GetWidth() && y < Collision()->GetHeight() && Collision()->GetIndex(x, y) == TILE_END)
				aTiles[NumTiles++] = Collision()->GetIndex(vec2(x*32, y*32));
		}
	}

	if(NumTiles)
		return aTiles[NumTiles/2]; // some tile from the middle
	else
		return -1;
}

void CAStar::BuildPath()
{
	int Start = GetStart();
	int Finish = GetFinish();
	int SolutionLength = 0;
	int *pSolution = astar_compute((const char *)m_pField, &SolutionLength, Collision()->GetWidth(), Collision()->GetHeight(), Start, Finish);
	dbg_msg("path", "start=%i finish=%i solution length=%i", Start, Finish, SolutionLength);

	bool NoFreeze = true; // avoid freeze?
	if(SolutionLength == -1) // try again ignoring freeze
	{
		FillGrid(false);
		pSolution = astar_compute((const char *)m_pField, &SolutionLength, Collision()->GetWidth(), Collision()->GetHeight(), Start, Finish);
		bool NoFreeze = false;
	}

	if(g_Config.m_ClNotifications)
	{
		if(SolutionLength != -1)
		{
			char aBuf[256];
			str_format(aBuf, sizeof(aBuf), "Found path. Length: %i.%s", SolutionLength, NoFreeze?"":" (Ignored freeze)");
			m_pClient->m_pHud->PushNotification(aBuf);
		}
		else
			m_pClient->m_pHud->PushNotification("No possible path found.");
	}
	
	if(pSolution)
	{
		if(SolutionLength > 0)
		{
			m_PathFound = true;
			for(int i = 0; i < SolutionLength; i++)
				m_Path.add(Collision()->GetPos(pSolution[i]));
			for(int i = 0; i < m_Path.size()-1; i++)
			{
				for(int j = 0; j < 7; j++)
				{
					IGraphics::CLineItem l;
					l.m_X0 = m_Path[i].x+j;
					l.m_X1 = m_Path[i+1].x+j;
					l.m_Y0 = m_Path[i].y+j;
					l.m_Y1 = m_Path[i+1].y+j;
					m_LineItems.add(l);
				}
			}
		}
		free(pSolution);
	}
}

void CAStar::FillGrid(bool NoFreeze)
{
	if(m_pField)
		mem_free(m_pField);

	m_pField = (char *)mem_alloc(Collision()->GetWidth() * Collision()->GetHeight() * sizeof(char), 1);
	for(int y = 0; y < Collision()->GetHeight(); y++)
	{
		for(int x = 0; x < Collision()->GetWidth(); x++)
		{
			if(Collision()->CheckPoint(x * 32, y * 32) || (NoFreeze && Collision()->GetIndex(x, y) == TILE_FREEZE))
				m_pField[Collision()->GetIndex(vec2(x*32, y*32))] = 0;
			else
				m_pField[Collision()->GetIndex(vec2(x*32, y*32))] = 1;
		}
	}
}

void CAStar::OnMapLoad()
{
	m_MapReloaded = true;
}
