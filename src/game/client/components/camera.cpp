/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/tl/string.h>

#include <engine/shared/config.h>

#include <base/math.h>
#include <game/collision.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>

#include "camera.h"
#include "controls.h"

#include <engine/serverbrowser.h>

CCamera::CCamera()
{
	m_CamType = CAMTYPE_UNDEFINED;
	m_ZoomSet = false;
	m_Zoom = 1.0;
	m_WantedZoom = 1.0f;
	m_WantedCenter = vec2(0.0f, 0.0f);
	m_RotationCenter = vec2(500.0f, 500.0f);
	m_GodlikeSpec = false;
}

void CCamera::OnRender()
{
	CALLSTACK_ADD();

	// ------------------ zooming ------------------
	if(!(m_pClient->m_Snap.m_SpecInfo.m_Active || !IsVanilla(Client()->GetServerInfo(0)) || Client()->State() == IClient::STATE_DEMOPLAYBACK))
	{
		m_ZoomSet = false;
		m_Zoom = 1.0;
	}
	else if(!m_ZoomSet && g_Config.m_ClDefaultZoom != 10)
	{
		m_ZoomSet = true;
		OnReset();
	}

	// smart zoom
	float TotalZoom = m_WantedZoom;
	vec2 Vel(0);
	if(m_pClient->m_Snap.m_SpecInfo.m_Active)
		Vel = mix(m_pClient->m_aClients[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID].m_PrevPredicted.m_Vel, m_pClient->m_aClients[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID].m_Predicted.m_Vel, Client()->IntraGameTick()); // TODO: fix this!
	else if(m_pClient->m_Snap.m_pLocalCharacter)
		Vel = vec2(m_pClient->m_Snap.m_pLocalCharacter->m_VelX, m_pClient->m_Snap.m_pLocalCharacter->m_VelY);

	if(Client()->State() == IClient::STATE_ONLINE && Vel != vec2(0))
	{
		if((g_Config.m_ClSmartZoom == 1 && (IsRace(Client()->GetServerInfo(0)) || IsDDNet(Client()->GetServerInfo(0)))) ||
				(g_Config.m_ClSmartZoom == 2 && !IsVanilla(Client()->GetServerInfo(0))))
		{
			float ExtraZoom = (length(Vel) / 24000.0f) * ((float)g_Config.m_ClSmartZoomVal/100.0f);
			TotalZoom += ExtraZoom;
		}
	}

	smooth_set(&m_Zoom, TotalZoom, 105.0f, Client()->RenderFrameTime());
	// ------------------ end zooming ------------------


	// update camera center
	if(Client()->State() == IClient::STATE_OFFLINE)
	{
		m_Zoom = 0.7f;
		m_WantedZoom = 0.7f;
		static vec2 Dir = vec2(1.0f, 0.0f);

		if(g_Config.m_ClMenuBackground && g_Config.m_ClMenuBackgroundRotation && distance(m_Center, m_RotationCenter) <= (float)g_Config.m_ClMenuBackgroundRotationRadius+0.5f)
		{
			// do little rotation
			float RotPerTick = 360.0f/(float)g_Config.m_ClMenuBackgroundRotationSpeed * Client()->RenderFrameTime();
			Dir = rotate(Dir, RotPerTick);
			m_WantedCenter = m_RotationCenter+Dir*(float)g_Config.m_ClMenuBackgroundRotationRadius/*(length(m_Center)/length(m_WantedCenter))*/;
			//m_WantedCenter = vec2(0.0f, 0.0f);
		}
		else
			m_WantedCenter = m_RotationCenter;
		//else ///// THIS PART IS DONE BY THE CINEMATIC CAMERA /////
		//{
		//	Dir = normalize(m_RotationCenter-m_WantedCenter);
		//	m_WantedCenter += Dir*(500.0f*Client()->RenderFrameTime());
		//	Dir = normalize(m_WantedCenter-m_RotationCenter);
		//}
	}
	else if(m_pClient->m_Snap.m_SpecInfo.m_Active && !m_pClient->m_Snap.m_SpecInfo.m_UsePosition)
	{
		if(m_CamType != CAMTYPE_SPEC)
		{
			m_LastPos[g_Config.m_ClDummy] = m_pClient->m_pControls->m_MousePos[g_Config.m_ClDummy];
			m_pClient->m_pControls->m_MousePos[g_Config.m_ClDummy] = m_PrevCenter;
			m_pClient->m_pControls->ClampMousePos();
			m_CamType = CAMTYPE_SPEC;
		}

	/*	if(m_GodlikeSpec && false) // TODO: this needs some more work.
		{
			vec2 Middlwerd(0.0f, 0.0f);
			int Num = 0;
			for(int i = 0; i < MAX_CLIENTS; i++)
			{
				CGameClient::CClientData *pClient = &m_pClient->m_aClients[i];
				if(!pClient->m_Active || pClient->m_Team == TEAM_SPECTATORS)
					continue;
				Num++;
				vec2 Pos = mix(pClient->m_Predicted.m_Pos, pClient->m_PrevPredicted.m_Pos, 0.5f);
				Middlwerd += Pos*32;
				//dbg_msg("debug", "Player '%s' at (%.1f, %.1f)", pClient->m_aName, Pos.x, Pos.y);
			}

			if(Middlwerd.x > 0.0f && Middlwerd.y > 0.0f)
			{
				Middlwerd.x /= Num;
				Middlwerd.y /= Num;
				m_WantedCenter = Middlwerd;
				dbg_msg("Middlwert", "(%.1f %.1f)", Middlwerd.x, Middlwerd.y);
			}
			else
				m_WantedCenter = m_pClient->m_pControls->m_MousePos[g_Config.m_ClDummy];
		}
		else*/
			m_WantedCenter = m_pClient->m_pControls->m_MousePos[g_Config.m_ClDummy];
		//dbg_msg("center", "(%.1f %.1f) wanted (%.1f %.1f)", m_Center.x, m_Center.y, m_WantedCenter.x, m_WantedCenter.y);
	}
	else
	{
		if(m_CamType != CAMTYPE_PLAYER)
		{
			m_pClient->m_pControls->m_MousePos[g_Config.m_ClDummy] = m_LastPos[g_Config.m_ClDummy];
			m_pClient->m_pControls->ClampMousePos();
			m_CamType = CAMTYPE_PLAYER;
		}

		vec2 CameraOffset(0, 0);

		float l = length(m_pClient->m_pControls->m_MousePos[g_Config.m_ClDummy]);
		if(l > 0.0001f) // make sure that this isn't 0
		{
			float DeadZone = g_Config.m_ClDyncam ? g_Config.m_ClDyncamDeadzone : g_Config.m_ClMouseDeadzone;
			float FollowFactor = (g_Config.m_ClDyncam ? g_Config.m_ClDyncamFollowFactor : g_Config.m_ClMouseFollowfactor) / 100.0f;
			float OffsetAmount = max(l-DeadZone, 0.0f) * FollowFactor;

			CameraOffset = normalize(m_pClient->m_pControls->m_MousePos[g_Config.m_ClDummy])*OffsetAmount;
		}

		if(m_pClient->m_Snap.m_SpecInfo.m_Active)
			m_WantedCenter = m_pClient->m_Snap.m_SpecInfo.m_Position + CameraOffset;
		else
			m_WantedCenter = m_pClient->m_LocalCharacterPos + CameraOffset;
	}

	if(m_WantedCenter != vec2(0.0f, 0.0f) &&
			(((g_Config.m_ClCinematicCamera && m_pClient->m_Snap.m_SpecInfo.m_Active) || g_Config.m_ClCinematicCamera == 2)
			 || Client()->State() == IClient::STATE_OFFLINE))
	{
		vec2 Speed(0);
		if(!m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_pLocalCharacter)
			Speed = vec2(m_pClient->m_Snap.m_pLocalCharacter->m_VelX, m_pClient->m_Snap.m_pLocalCharacter->m_VelY) / 100.0f;
		const float delay = (Client()->State() == IClient::STATE_OFFLINE ? 50.0f : g_Config.m_ClCinematicCameraDelay);
		smooth_set(&m_Center.x, m_WantedCenter.x+Speed.x, delay, Client()->RenderFrameTime());
		smooth_set(&m_Center.y, m_WantedCenter.y+Speed.y, delay, Client()->RenderFrameTime());
	}
	else
		m_Center = m_WantedCenter;

	m_PrevCenter = m_Center;
}

void CCamera::OnConsoleInit()
{
	CALLSTACK_ADD();

	Console()->Register("zoom+", "", CFGFLAG_CLIENT, ConZoomPlus, this, "Zoom increase");
	Console()->Register("zoom-", "", CFGFLAG_CLIENT, ConZoomMinus, this, "Zoom decrease");
	Console()->Register("zoom", "?i", CFGFLAG_CLIENT, ConZoomReset, this, "Zoom reset or set");
	//Console()->Register("godlike_spec", "", CFGFLAG_CLIENT, ConToggleGodlikeSpec, this, "Toggle godlike spectator cam");
}

const float ZoomStep = 0.866025f;

void CCamera::OnReset()
{
	m_Zoom = 1.0f;

	for (int i = g_Config.m_ClDefaultZoom; i < 10; i++)
	{
		m_Zoom *= 1/ZoomStep;
	}
	for (int i = g_Config.m_ClDefaultZoom; i > 10; i--)
	{
		m_Zoom *= ZoomStep;
	}

	m_WantedZoom = m_Zoom;
	m_WantedCenter = vec2(0.0f, 0.0f);
}

void CCamera::ConZoomPlus(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CCamera *pSelf = (CCamera *)pUserData;
	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_Active || !IsVanilla(pSelf->Client()->GetServerInfo(0)) || pSelf->Client()->State() == IClient::STATE_DEMOPLAYBACK)
		((CCamera *)pUserData)->m_WantedZoom = ((CCamera *)pUserData)->m_WantedZoom*ZoomStep;
}
void CCamera::ConZoomMinus(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CCamera *pSelf = (CCamera *)pUserData;
	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_Active || !IsVanilla(pSelf->Client()->GetServerInfo(0)) || pSelf->Client()->State() == IClient::STATE_DEMOPLAYBACK)
		((CCamera *)pUserData)->m_WantedZoom = ((CCamera *)pUserData)->m_WantedZoom*(1/ZoomStep);
}
void CCamera::ConZoomReset(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CCamera *pSelf = (CCamera *)pUserData;
	CServerInfo Info;
	pSelf->Client()->GetServerInfo(&Info);
	((CCamera *)pUserData)->OnReset();
}

void CCamera::ConToggleGodlikeSpec(IConsole::IResult *pResult, void *pUserData)
{
	CALLSTACK_ADD();

	CCamera *pSelf = (CCamera *)pUserData;
	pSelf->m_GodlikeSpec ^= true;
}
