/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>
#include <engine/graphics.h>
#include <engine/demo.h>

#include <game/generated/client_data.h>
#include <game/client/render.h>
#include <game/gamecore.h>
#include "particles.h"

CParticles::CParticles()
{
	OnReset();
	m_RenderTrail.m_pParts = this;
	m_RenderExplosions.m_pParts = this;
	m_RenderGeneral.m_pParts = this;
}


void CParticles::OnReset()
{
	// reset particles
	for(int i = 0; i < MAX_PARTICLES; i++)
	{
		m_aParticles[i].m_PrevPart = i-1;
		m_aParticles[i].m_NextPart = i+1;
	}

	m_aParticles[0].m_PrevPart = 0;
	m_aParticles[MAX_PARTICLES-1].m_NextPart = -1;
	m_FirstFree = 0;

	for(int i = 0; i < NUM_GROUPS; i++)
		m_aFirstPart[i] = -1;
}

void CParticles::Add(int Group, CParticle *pPart)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();
		if(pInfo->m_Paused)
			return;
	}
	else
	{
		if(m_pClient->m_Snap.m_pGameInfoObj && (m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_PAUSED))
			return;
	}

	if (m_FirstFree == -1)
		return;

	// remove from the free list
	int Id = m_FirstFree;
	m_FirstFree = m_aParticles[Id].m_NextPart;
	if(m_FirstFree != -1)
		m_aParticles[m_FirstFree].m_PrevPart = -1;

	// copy data
	m_aParticles[Id] = *pPart;

	// insert to the group list
	m_aParticles[Id].m_PrevPart = -1;
	m_aParticles[Id].m_NextPart = m_aFirstPart[Group];
	if(m_aFirstPart[Group] != -1)
		m_aParticles[m_aFirstPart[Group]].m_PrevPart = Id;
	m_aFirstPart[Group] = Id;

	// set some parameters
	m_aParticles[Id].m_Life = 0;
}

void CParticles::Update(float TimePassed)
{
	static float FrictionFraction = 0;
	FrictionFraction += TimePassed;

	if(FrictionFraction > 2.0f) // safty messure
		FrictionFraction = 0;

	int FrictionCount = 0;
	while(FrictionFraction > 0.05f)
	{
		FrictionCount++;
		FrictionFraction -= 0.05f;
	}

	for(int g = 0; g < NUM_GROUPS; g++)
	{
		int i = m_aFirstPart[g];
		while(i != -1)
		{
			int Next = m_aParticles[i].m_NextPart;
			//m_aParticles[i].vel += flow_get(m_aParticles[i].pos)*time_passed * m_aParticles[i].flow_affected;
			m_aParticles[i].m_Vel.y += m_aParticles[i].m_Gravity*TimePassed;

			for(int f = 0; f < FrictionCount; f++) // apply friction
				m_aParticles[i].m_Vel *= m_aParticles[i].m_Friction;

			// move the point
			vec2 Vel = m_aParticles[i].m_Vel*TimePassed;
			Collision()->MovePoint(&m_aParticles[i].m_Pos, &Vel, 0.1f+0.9f*frandom(), NULL);
			m_aParticles[i].m_Vel = Vel* (1.0f/TimePassed);

			m_aParticles[i].m_Life += TimePassed;
			m_aParticles[i].m_Rot += TimePassed * m_aParticles[i].m_Rotspeed;

			// check particle death
			if(m_aParticles[i].m_Life > m_aParticles[i].m_LifeSpan)
			{
				// remove it from the group list
				if(m_aParticles[i].m_PrevPart != -1)
					m_aParticles[m_aParticles[i].m_PrevPart].m_NextPart = m_aParticles[i].m_NextPart;
				else
					m_aFirstPart[g] = m_aParticles[i].m_NextPart;

				if(m_aParticles[i].m_NextPart != -1)
					m_aParticles[m_aParticles[i].m_NextPart].m_PrevPart = m_aParticles[i].m_PrevPart;

				// insert to the free list
				if(m_FirstFree != -1)
					m_aParticles[m_FirstFree].m_PrevPart = i;
				m_aParticles[i].m_PrevPart = -1;
				m_aParticles[i].m_NextPart = m_FirstFree;
				m_FirstFree = i;
			}

			i = Next;
		}
	}
}

void CParticles::OnRender()
{
	if(Client()->State() < IClient::STATE_ONLINE)
		return;

	static int64 LastTime = 0;
	int64 t = time_get();

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();
		if(!pInfo->m_Paused)
			Update((float)((t-LastTime)/(double)time_freq())*pInfo->m_Speed);
	}
	else
	{
		if(m_pClient->m_Snap.m_pGameInfoObj && !(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_PAUSED))
			Update((float)((t-LastTime)/(double)time_freq()));
	}

	LastTime = t;
}

/*
void CParticles::RenderGroup(int Group)
{
	Graphics()->BlendNormal();
	//gfx_blend_additive();
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PARTICLES].m_Id);
	Graphics()->QuadsBegin();

	int i = m_aFirstPart[Group];
	while(i != -1)
	{
		RenderTools()->SelectSprite(m_aParticles[i].m_Spr);
		float a = m_aParticles[i].m_Life / m_aParticles[i].m_LifeSpan;
		vec2 p = m_aParticles[i].m_Pos;
		float Size = mix(m_aParticles[i].m_StartSize, m_aParticles[i].m_EndSize, a);

		Graphics()->QuadsSetRotation(m_aParticles[i].m_Rot);

		Graphics()->SetColor(
			m_aParticles[i].m_Color.r,
			m_aParticles[i].m_Color.g,
			m_aParticles[i].m_Color.b,
			m_aParticles[i].m_Color.a); // pow(a, 0.75f) *

		IGraphics::CQuadItem QuadItem(p.x, p.y, Size, Size);
		Graphics()->QuadsDraw(&QuadItem, 1);

		i = m_aParticles[i].m_NextPart;
	}
	Graphics()->QuadsEnd();
	Graphics()->BlendNormal();
} */
void CParticles::RenderGroup(int Group)
{
	Graphics()->BlendNormal();
	if (Group == GROUP_HCLIENT_BLOOD)
		Graphics()->TextureSet(g_pData->m_aImages[IMAGE_BLOOD].m_Id);
    else if (Group != GROUP_HCLIENT_BLOOD_BODY)
        Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PARTICLES].m_Id);

    //Graphics()->QuadsBegin();

	int i = m_aFirstPart[Group];
	while(i != -1)
	{
	    vec2 p = m_aParticles[i].m_Pos;

	    if (m_aParticles[i].m_Type == PARTICLE_WEAPON)
	    {
	    	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
	    }
	    else if (m_aParticles[i].m_Type == PARTICLE_BLOOD_L)
	    {
            int Nx = 32*((int)p.x / 32);
            int Ny = 32*((int)p.y / 32);
            int Nw = 32;
            int Nh = 32;

	        if (!Collision()->CheckPoint(vec2(Nx, Ny)))
	        {
	            m_aParticles[i].m_Life = m_aParticles[i].m_LifeSpan+1;
	            i = m_aParticles[i].m_NextPart;
	            continue;
	        }

            if (Collision()->CheckPoint(vec2(Nx+32.0f, Ny))) { Nw += 32; }
            if (Collision()->CheckPoint(vec2(Nx-32.0f, Ny))) { Nx -= 32; Nw += 32; }
            if (Collision()->CheckPoint(vec2(Nx, Ny+32.0f))) { Nh += 32; }
            if (Collision()->CheckPoint(vec2(Nx, Ny-32.0f))) { Ny -= 32; Nh += 32; }

            //vec2 Center = m_pClient->m_pCamera->m_Center;

			// set clipping
			float Points[4];
			Graphics()->GetScreen(&Points[0], &Points[1], &Points[2], &Points[3]);
			float x0 = (Nx - Points[0]) / (Points[2]-Points[0]);
			float y0 = (Ny - Points[1]) / (Points[3]-Points[1]);
			float x1 = ((Nx+Nw) - Points[0]) / (Points[2]-Points[0]);
			float y1 = ((Ny+Nh) - Points[1]) / (Points[3]-Points[1]);

			Graphics()->ClipEnable((int)(x0*Graphics()->ScreenWidth()), (int)(y0*Graphics()->ScreenHeight()),
				(int)((x1-x0)*Graphics()->ScreenWidth()), (int)((y1-y0)*Graphics()->ScreenHeight()));
	    }
	    else if (m_aParticles[i].m_Type == PARTICLE_BLOOD_BODY)
	    {
	    	CGameClient::CClientData *pClientData = &m_pClient->m_aClients[*((int*)m_aParticles[i].m_pData)];
	    	CTeeRenderInfo *pInfo = 0x0;
	    	if (pClientData)
				pInfo = &pClientData->m_RenderInfo;

	    	Graphics()->TextureSet((pInfo)?pInfo->m_Texture:-1);
	    }

	    Graphics()->QuadsBegin();

	    if(m_aParticles[i].m_Spr != -1)
            RenderTools()->SelectSprite(m_aParticles[i].m_Spr);
		float a = m_aParticles[i].m_Life / m_aParticles[i].m_LifeSpan;
		float SizeW = mix(m_aParticles[i].m_StartSize.x, m_aParticles[i].m_EndSize.x, a);
		float SizeH = mix(m_aParticles[i].m_StartSize.y, m_aParticles[i].m_EndSize.y, a);
		float EndColor = 0.7f;
		if (m_aParticles[i].m_ToBlack) { EndColor = mix(0.7f, 0.0f, a); }

		Graphics()->QuadsSetRotation(m_aParticles[i].m_Rot);

		Graphics()->SetColor(
			(m_aParticles[i].m_Color.r-(0.7f-EndColor)>0)?m_aParticles[i].m_Color.r-(0.7f-EndColor):0.0f,
			(m_aParticles[i].m_Color.g-(0.7f-EndColor)>0)?m_aParticles[i].m_Color.g-(0.7f-EndColor):0.0f,
			(m_aParticles[i].m_Color.b-(0.7f-EndColor)>0)?m_aParticles[i].m_Color.b-(0.7f-EndColor):0.0f,
			m_aParticles[i].m_Color.a); // pow(a, 0.75f) *

        IGraphics::CQuadItem QuadItem(p.x, p.y, SizeW, SizeH);
        Graphics()->QuadsDraw(&QuadItem, 1);

        Graphics()->QuadsEnd();

		if (m_aParticles[i].m_Type == PARTICLE_BLOOD_L)
            Graphics()->ClipDisable();

		i = m_aParticles[i].m_NextPart;
	}

    //Graphics()->QuadsEnd();

	Graphics()->BlendNormal();
}
