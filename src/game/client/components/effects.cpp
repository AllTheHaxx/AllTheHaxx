/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/tl/sorted_array.h>

#include <engine/demo.h>
#include <engine/engine.h>

#include <engine/shared/config.h>

#include <game/generated/client_data.h>

#include <game/client/components/particles.h>
#include <game/client/components/skins.h>
#include <game/client/components/flow.h>
#include <game/client/components/damageind.h>
#include <game/client/components/sounds.h>
#include <game/client/gameclient.h>

#include "effects.h"

inline vec2 RandomDir() { return normalize(vec2(frandom()-0.5f, frandom()-0.5f)); }

CEffects::CEffects()
{
	m_Add50hz = false;
	m_Add100hz = false;
}

void CEffects::AirJump(vec2 Pos)
{
	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_AIRJUMP;
	p.m_Pos = Pos + vec2(-6.0f, 16.0f);
	p.m_Vel = vec2(0, -200);
	p.m_LifeSpan = 0.5f;
	p.m_StartSize = vec2(48.0f, 48.0f);
	p.m_EndSize = vec2(0,0);
	p.m_Rot = frandom()*pi*2;
	p.m_Rotspeed = pi*2;
	p.m_Gravity = 500;
	p.m_Friction = 0.7f;
	p.m_FlowAffected = 0.0f;
	m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);

	p.m_Pos = Pos + vec2(6.0f, 16.0f);
	m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);

	if(g_Config.m_SndGame)
		m_pClient->m_pSounds->PlayAt(CSounds::CHN_WORLD, SOUND_PLAYER_AIRJUMP, 1.0f, Pos);
}

void CEffects::DamageIndicator(vec2 Pos, vec2 Dir)
{
	m_pClient->m_pDamageind->Create(Pos, Dir);
}

void CEffects::ResetDamageIndicator()
{
	m_pClient->m_pDamageind->Reset();
}

void CEffects::PowerupShine(vec2 Pos, vec2 size)
{
	if(!m_Add50hz)
		return;

	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_SLICE;
	p.m_Pos = Pos + vec2((frandom()-0.5f)*size.x, (frandom()-0.5f)*size.y);
	p.m_Vel = vec2(0, 0);
	p.m_LifeSpan = 0.5f;
	p.m_StartSize = vec2(16.0f, 16.0f);
	p.m_EndSize = vec2(0,0);
	p.m_Rot = frandom()*pi*2;
	p.m_Rotspeed = pi*2;
	p.m_Gravity = 500;
	p.m_Friction = 0.9f;
	p.m_FlowAffected = 0.0f;
	m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);
}

void CEffects::SmokeTrail(vec2 Pos, vec2 Vel)
{
	if(!m_Add50hz)
		return;

	float StartSize = 12.0f + frandom()*8;

	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_SMOKE;
	p.m_Pos = Pos;
	p.m_Vel = Vel + RandomDir()*50.0f;
	p.m_LifeSpan = 0.5f + frandom()*0.5f;
	p.m_StartSize = vec2(StartSize, StartSize);
	p.m_EndSize = vec2(0,0);
	p.m_Friction = 0.7f;
	p.m_Gravity = frandom()*-500.0f;
	m_pClient->m_pParticles->Add(CParticles::GROUP_PROJECTILE_TRAIL, &p);
}


void CEffects::SkidTrail(vec2 Pos, vec2 Vel)
{
	if(!m_Add100hz)
		return;

	float StartSize = 24.0f + frandom()*12;

	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_SMOKE;
	p.m_Pos = Pos;
	p.m_Vel = Vel + RandomDir()*50.0f;
	p.m_LifeSpan = 0.5f + frandom()*0.5f;
	p.m_StartSize = vec2(StartSize, StartSize);
	p.m_EndSize = vec2(0,0);
	p.m_Friction = 0.7f;
	p.m_Gravity = frandom()*-500.0f;
	p.m_Color = vec4(0.75f,0.75f,0.75f,1.0f);
	m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);
}

void CEffects::BulletTrail(vec2 Pos)
{
	if(!m_Add100hz)
		return;

	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_BALL;
	p.m_Pos = Pos;
	p.m_LifeSpan = 0.25f + frandom()*0.25f;
	p.m_StartSize = vec2(28.0f, 28.0f);
	p.m_EndSize = vec2(0,0);
	p.m_Friction = 0.7f;
	m_pClient->m_pParticles->Add(CParticles::GROUP_PROJECTILE_TRAIL, &p);
}

void CEffects::PlayerSpawn(vec2 Pos)
{
	for(int i = 0; i < 32; i++)
	{
		float StartSize = 64.0f + frandom()*32;

		CParticle p;
		p.SetDefault();
		p.m_Spr = SPRITE_PART_SHELL;
		p.m_Pos = Pos;
		p.m_Vel = RandomDir() * (powf(frandom(), 3)*600.0f);
		p.m_LifeSpan = 0.3f + frandom()*0.3f;
		p.m_StartSize = vec2(StartSize, StartSize);
		p.m_EndSize = vec2(0,0);
		p.m_Rot = frandom()*pi*2;
		p.m_Rotspeed = frandom();
		p.m_Gravity = frandom()*-400.0f;
		p.m_Friction = 0.7f;
		p.m_Color = vec4(0xb5/255.0f, 0x50/255.0f, 0xcb/255.0f, 1.0f);
		m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);

	}
	if(g_Config.m_SndGame)
		m_pClient->m_pSounds->PlayAt(CSounds::CHN_WORLD, SOUND_PLAYER_SPAWN, 1.0f, Pos);
}

void CEffects::PlayerDeath(vec2 Pos, int ClientID)
{
	if (!g_Config.m_ClGoreStyle || (g_Config.m_ClGoreStyle && ClientID < 0)) // gore
	{
		vec3 BloodColor(1.0f,1.0f,1.0f);

		if(ClientID >= 0)
		{
			if(m_pClient->m_aClients[ClientID].m_UseCustomColor)
				BloodColor = m_pClient->m_pSkins->GetColorV3(m_pClient->m_aClients[ClientID].m_ColorBody);
			else
			{
				const CSkins::CSkin *s = m_pClient->m_pSkins->Get(m_pClient->m_aClients[ClientID].m_SkinID);
				if(s)
					BloodColor = s->m_BloodColor;
			}
		}

		for(int i = 0; i < 64; i++)
		{
			float StartSize = 24.0f + frandom()*16;

			CParticle p;
			p.SetDefault();
			p.m_Spr = SPRITE_PART_SPLAT01 + (rand()%3);
			p.m_Pos = Pos;
			p.m_Vel = RandomDir() * ((frandom()+0.1f)*900.0f);
			p.m_LifeSpan = 0.3f + frandom()*0.3f;
			p.m_StartSize = vec2(StartSize, StartSize);
			p.m_EndSize = vec2(0,0);
			p.m_Rot = frandom()*pi*2;
			p.m_Rotspeed = (frandom()-0.5f) * pi;
			p.m_Gravity = 800.0f;
			p.m_Friction = 0.8f;
			vec3 c = BloodColor * (0.75f + frandom()*0.25f);
			p.m_Color = vec4(c.r, c.g, c.b, 0.75f);
			m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);
		}
	}
	else
	{
		Blood(Pos, vec2(0.0f, 0.0f), 1, ClientID);
	}
}

// stolen from H-Client :3
void CEffects::Blood(vec2 Pos, vec2 Dir, int Type, int ClientID)
{
    vec3 BloodColor(1.0f, 0.0f, 0.0f);
    vec3 TeeColor(1.0f, 1.0f, 1.0f);

    if(ClientID >= 0 && ClientID < MAX_CLIENTS)
    {
        if(m_pClient->m_aClients[ClientID].m_UseCustomColor)
        	TeeColor = m_pClient->m_pSkins->GetColorV3(m_pClient->m_aClients[ClientID].m_ColorBody);
        else
        {
            const CSkins::CSkin *s = m_pClient->m_pSkins->Get(m_pClient->m_aClients[ClientID].m_SkinID);
            if(s)
            	TeeColor = s->m_BloodColor;
        }
    }

    if (g_Config.m_ClGoreStyleTeeColors)
    	BloodColor = TeeColor;

    if (Type == 0)
    {
        int SubType = 0;
        for(int i = 0; i < 25; i++)
        {
        	float StartSize = 10.0f + frandom()*16.0f;

            CParticle p;
            p.SetDefault();

            if (!SubType)
            {
                p.m_Spr = SPRITE_BLOOD_BODY_PART;
                p.m_Vel = Dir * (powf(frandom(), 3)*600.0f);
            }
            else
            {
                p.m_Spr = SPRITE_BLOOD_BODY_PART;
                p.m_Vel = RandomDir() * (powf(frandom(), 3)*200.0f);
            }

            p.m_Pos = Pos;
            p.m_LifeSpan = 1.5f + frandom()*0.3f;
            p.m_StartSize = vec2(StartSize, StartSize);
            p.m_EndSize = vec2(0.0f, 0.0f);
            p.m_Rot = frandom()*pi*2;
            p.m_Rotspeed = frandom();
            p.m_Gravity = frandom()*400.0f;
            p.m_Friction = 0.7f;
            p.m_Collide = false;
            p.m_Color = vec4(BloodColor.r, BloodColor.g, BloodColor.b, 0.75f);
            m_pClient->m_pParticles->Add(CParticles::GROUP_HCLIENT_BLOOD, &p);

            if (i == 10)
                SubType ^= 1;
        }
    }
    else if (Type == 1)
    {
    	// Blood
        for(int i = 0; i < 64; i++)
        {
        	float StartSize = 5.0f + frandom()*16.0f;

            CParticle p;
            p.SetDefault();
            p.m_Spr = SPRITE_BLOOD_BODY_PART;
            p.m_Pos = Pos;
            p.m_Vel = RandomDir() * (powf(frandom(), 3)*1800.0f);
            p.m_LifeSpan = 4.0f + frandom()*0.3f;
            p.m_StartSize = vec2(StartSize, StartSize);
            p.m_EndSize = vec2(0.0f, 0.0f);
            p.m_Rot = frandom()*pi*2;
            p.m_Rotspeed = frandom();
            p.m_Gravity = 3000.0f;
            p.m_Friction = 0.95f;
            p.m_Collide = false;
            p.m_Color = vec4(BloodColor.r, BloodColor.g, BloodColor.b, 0.75f);
            p.m_Type = CParticles::PARTICLE_BLOOD;
            m_pClient->m_pParticles->Add(CParticles::GROUP_HCLIENT_BLOOD, &p);
        }

    	// Body Parts
        for(int i = 0; i < 2; i++)
        {
            CParticle p_hand;
            p_hand.SetDefault();
            p_hand.m_Spr = SPRITE_TEE_HAND;
            p_hand.m_Pos = Pos;
            p_hand.m_Vel = RandomDir() * (powf(frandom(), 3)*1800.0f);
            p_hand.m_LifeSpan = 8.0f + frandom()*0.3f;
            p_hand.m_StartSize = vec2(20.0f, 20.0f);
            p_hand.m_EndSize = vec2(20.0f, 20.0f);
            p_hand.m_Rot = frandom()*pi*2;
            p_hand.m_Gravity = 3000.0f;
            p_hand.m_Friction = 0.95f;
            p_hand.m_Collide = true;
            p_hand.m_Color = vec4(TeeColor.r, TeeColor.g, TeeColor.b, 1.0f);
            p_hand.m_Type = CParticles::PARTICLE_BLOOD_BODY;
            p_hand.m_pData = mem_alloc(sizeof(int), 1);
            mem_copy(p_hand.m_pData, &ClientID, sizeof(int));
            m_pClient->m_pParticles->Add(CParticles::GROUP_HCLIENT_BLOOD_BODY, &p_hand);

            CParticle p_foot;
            p_foot.SetDefault();
            p_foot.m_Spr = SPRITE_TEE_FOOT;
            p_foot.m_Pos = Pos;
            p_foot.m_Vel = RandomDir() * (powf(frandom(), 3)*1800.0f);
            p_foot.m_LifeSpan = 8.0f + frandom()*0.3f;
            p_foot.m_StartSize = vec2(64.0f, 32.0f);
            p_foot.m_EndSize = vec2(64.0f, 32.0f);
            p_foot.m_Rot = frandom()*pi*2;
            p_foot.m_Gravity = 3000.0f;
            p_foot.m_Friction = 0.95f;
            p_foot.m_Collide = true;
            p_foot.m_Color = vec4(TeeColor.r, TeeColor.g, TeeColor.b, 1.0f);
            p_foot.m_Type = CParticles::PARTICLE_BLOOD_BODY;
            p_foot.m_pData = mem_alloc(sizeof(int), 1);
            mem_copy(p_foot.m_pData, &ClientID, sizeof(int));
            m_pClient->m_pParticles->Add(CParticles::GROUP_HCLIENT_BLOOD_BODY, &p_foot);
        }

        // Body
        for(int i = 0; i < 18; i++)
        {
        	float size = 15.0f*frandom();
            CParticle p_body;
            p_body.SetDefault();
            p_body.m_Spr = SPRITE_TEE_BODY;
            p_body.m_Pos = Pos;
            p_body.m_Vel = RandomDir() * (powf(frandom(), 3)*1800.0f);
            p_body.m_LifeSpan = 8.0f + frandom()*0.3f;
            p_body.m_StartSize = vec2(size, size);
            p_body.m_EndSize = vec2(size, size);
            p_body.m_Rot = frandom()*pi*2;
            p_body.m_Gravity = 3000.0f;
            p_body.m_Friction = 0.95f;
            p_body.m_Collide = true;
            p_body.m_Color = vec4(TeeColor.r, TeeColor.g, TeeColor.b, 1.0f);
            p_body.m_Type = CParticles::PARTICLE_BLOOD_BODY;
            p_body.m_pData = mem_alloc(sizeof(int), 1);
            mem_copy(p_body.m_pData, &ClientID, sizeof(int));
            m_pClient->m_pParticles->Add(CParticles::GROUP_HCLIENT_BLOOD_BODY, &p_body);
        }

        // Weapon
        int weapon = m_pClient->m_aClients[ClientID].m_Predicted.m_ActiveWeapon;
        if (g_Config.m_ClGoreStyleDropWeapons && (weapon == WEAPON_HAMMER || weapon == WEAPON_GUN || weapon == WEAPON_SHOTGUN || weapon == WEAPON_GRENADE || weapon == WEAPON_RIFLE))
        {
			CParticle p_weapon;
			p_weapon.SetDefault();

			float f = 0.0f;
			switch (weapon)
			{
				case WEAPON_HAMMER:
					f = sqrtf(128*128 + 96*96);
					p_weapon.m_Spr = SPRITE_WEAPON_HAMMER_BODY;
					p_weapon.m_StartSize = vec2(g_pData->m_Weapons.m_aId[WEAPON_HAMMER].m_VisualSize * (128/f), g_pData->m_Weapons.m_aId[WEAPON_HAMMER].m_VisualSize * (96/f));
					p_weapon.m_EndSize = p_weapon.m_StartSize;
					break;
				case WEAPON_GUN:
					f = sqrtf(128*128 + 64*64);
					p_weapon.m_Spr = SPRITE_WEAPON_GUN_BODY;
					p_weapon.m_StartSize = vec2(g_pData->m_Weapons.m_aId[WEAPON_GUN].m_VisualSize * (128/f), g_pData->m_Weapons.m_aId[WEAPON_GUN].m_VisualSize * (64/f));
					p_weapon.m_EndSize = p_weapon.m_StartSize;
					break;
				case WEAPON_SHOTGUN:
					f = sqrtf(224*224 + 64*64);
					p_weapon.m_Spr = SPRITE_WEAPON_SHOTGUN_BODY;
					p_weapon.m_StartSize = vec2(g_pData->m_Weapons.m_aId[WEAPON_SHOTGUN].m_VisualSize * (224/f), g_pData->m_Weapons.m_aId[WEAPON_SHOTGUN].m_VisualSize * (64/f));
					p_weapon.m_EndSize = p_weapon.m_StartSize;
					break;
				case WEAPON_GRENADE:
					f = sqrtf(224*224 + 64*64);
					p_weapon.m_Spr = SPRITE_WEAPON_GRENADE_BODY;
					p_weapon.m_StartSize = vec2(g_pData->m_Weapons.m_aId[WEAPON_GRENADE].m_VisualSize * (224/f), g_pData->m_Weapons.m_aId[WEAPON_GRENADE].m_VisualSize * (64/f));
					p_weapon.m_EndSize = p_weapon.m_StartSize;
					break;
				case WEAPON_RIFLE:
					f = sqrtf(224*224 + 96*96);
					p_weapon.m_Spr = SPRITE_WEAPON_RIFLE_BODY;
					p_weapon.m_StartSize = vec2(g_pData->m_Weapons.m_aId[WEAPON_RIFLE].m_VisualSize * (224/f), g_pData->m_Weapons.m_aId[WEAPON_RIFLE].m_VisualSize * (96/f));
					p_weapon.m_EndSize = p_weapon.m_StartSize;
					break;
			}
			p_weapon.m_Pos = Pos;
			p_weapon.m_Vel = RandomDir() * (powf(frandom(), 3)*1800.0f);
			p_weapon.m_LifeSpan = 8.0f + frandom()*0.3f;
			p_weapon.m_Rot = frandom()*pi*2;
			p_weapon.m_Gravity = 3000.0f;
			p_weapon.m_Friction = 0.95f;
			p_weapon.m_Collide = true;
			p_weapon.m_Color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			p_weapon.m_Type = CParticles::PARTICLE_WEAPON;
			m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p_weapon);
        }
    }
}
//


void CEffects::Explosion(vec2 Pos)
{
	// add to flow
	for(int y = -8; y <= 8; y++)
		for(int x = -8; x <= 8; x++)
		{
			if(x == 0 && y == 0)
				continue;

			float a = 1 - (length(vec2(x,y)) / length(vec2(8,8)));
			m_pClient->m_pFlow->Add(Pos+vec2(x,y)*16, normalize(vec2(x,y))*5000.0f*a, 10.0f);
		}

	// add the explosion
	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_EXPL01;
	p.m_Pos = Pos;
	p.m_LifeSpan = 0.4f;
	p.m_StartSize = vec2(150.0f, 150.0f);
	p.m_EndSize = vec2(0,0);
	p.m_Rot = frandom()*pi*2;
	m_pClient->m_pParticles->Add(CParticles::GROUP_EXPLOSIONS, &p);

	// add the smoke
	for(int i = 0; i < 24; i++)
	{
		float StartSize = 32.0f + frandom()*8;

		CParticle p;
		p.SetDefault();
		p.m_Spr = SPRITE_PART_SMOKE;
		p.m_Pos = Pos;
		p.m_Vel = RandomDir() * ((1.0f + frandom()*0.2f) * 1000.0f);
		p.m_LifeSpan = 0.5f + frandom()*0.4f;
		p.m_StartSize = vec2(StartSize, StartSize);
		p.m_EndSize = vec2(0,0);
		p.m_Gravity = frandom()*-800.0f;
		p.m_Friction = 0.4f;
		p.m_Color = mix(vec4(0.75f,0.75f,0.75f,1.0f), vec4(0.5f,0.5f,0.5f,1.0f), frandom());
		m_pClient->m_pParticles->Add(CParticles::GROUP_GENERAL, &p);
	}
}


void CEffects::HammerHit(vec2 Pos)
{
	// add the explosion
	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_HIT01;
	p.m_Pos = Pos;
	p.m_LifeSpan = 0.3f;
	p.m_StartSize = vec2(120.0f, 120.0f);
	p.m_EndSize = vec2(0,0);
	p.m_Rot = frandom()*pi*2;
	m_pClient->m_pParticles->Add(CParticles::GROUP_EXPLOSIONS, &p);
	if(g_Config.m_SndGame)
		m_pClient->m_pSounds->PlayAt(CSounds::CHN_WORLD, SOUND_HAMMER_HIT, 1.0f, Pos);
}

// H-Client
void CEffects::LaserTrail(vec2 Pos, vec2 Vel, vec4 color)
{
	if(!m_Add50hz)
		return;

	float StartSize = 5.0f + frandom()*6;

	CParticle p;
	p.SetDefault();
	p.m_Spr = SPRITE_PART_SMOKE;
	p.m_Pos = Pos;
	p.m_Vel = Vel + RandomDir()*50.0f;
	p.m_LifeSpan = frandom()*0.2f;
	p.m_StartSize = vec2(StartSize, StartSize);
	p.m_EndSize = vec2(0.0f, 0.0f);
	p.m_Friction = 0.7f;
	p.m_Gravity = 0.0f;
	p.m_Color = color;
	m_pClient->m_pParticles->Add(CParticles::GROUP_PROJECTILE_TRAIL, &p);
}

void CEffects::OnRender()
{
	static int64 LastUpdate100hz = 0;
	static int64 LastUpdate50hz = 0;

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		const IDemoPlayer::CInfo *pInfo = DemoPlayer()->BaseInfo();

		if(time_get()-LastUpdate100hz > time_freq()/(100*pInfo->m_Speed))
		{
			m_Add100hz = true;
			LastUpdate100hz = time_get();
		}
		else
			m_Add100hz = false;

		if(time_get()-LastUpdate50hz > time_freq()/(50*pInfo->m_Speed))
		{
			m_Add50hz = true;
			LastUpdate50hz = time_get();
		}
		else
			m_Add50hz = false;

		if(m_Add50hz)
			m_pClient->m_pFlow->Update();

		return;
	}

	if(time_get()-LastUpdate100hz > time_freq()/100)
	{
		m_Add100hz = true;
		LastUpdate100hz = time_get();
	}
	else
		m_Add100hz = false;

	if(time_get()-LastUpdate50hz > time_freq()/50)
	{
		m_Add50hz = true;
		LastUpdate50hz = time_get();
	}
	else
		m_Add50hz = false;

	if(m_Add50hz)
		m_pClient->m_pFlow->Update();
}
