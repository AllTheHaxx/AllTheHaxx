/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_HUD_H
#define GAME_CLIENT_COMPONENTS_HUD_H
#include <base/tl/sorted_array.h>
#include <game/client/component.h>

class CHud : public CComponent
{
	struct CNotification
	{
		char m_aMsg[256];
		float m_SpawnTime;
		float m_xOffset;
		vec4 m_Color;

		bool operator<(CNotification& other) { return this->m_SpawnTime > other.m_SpawnTime; }
	};

	enum
	{
		MAX_NOTIFICATIONS=15
	};

	float m_Width, m_Height;
	float m_AverageFPS;

	void RenderCursor();

	void RenderTextInfo();
	void RenderConnectionWarning();
	void RenderTeambalanceWarning();
	void RenderNotifications();
	void RenderIRCNotifications(CUIRect Rect);
	void RenderChatBox();
	void RenderVoting();
	void RenderHealthAndAmmo(const CNetObj_Character *pCharacter);
	void RenderGameTimer();
	void RenderPauseNotification();
	void RenderSuddenDeath();
	void RenderScoreHud();
	void RenderSpectatorHud();
	void RenderWarmupTimer();
	void RenderLocalTime(float x);

	void MapscreenToGroup(float CenterX, float CenterY, struct CMapItemGroup *PGroup);

public:
	CHud();

	virtual void OnReset();
	virtual void OnRender();

	// DDRace
	virtual void OnMessage(int MsgType, void *pRawMsg);

	void PushNotification(const char *pMsg, vec4 Color = vec4(1,1,1,1));
	const char *GetNotification(int index);

private:
	void RenderRecord();
	void RenderDDRaceEffects();
	float m_CheckpointDiff;
	float m_ServerRecord;
	float m_PlayerRecord;
	int m_DDRaceTime;
	int m_LastReceivedTimeTick;
	int m_CheckpointTick;
	int m_DDRaceTick;
	bool m_FinishTime;
	bool m_DDRaceTimeReceived;
	sorted_array<CNotification> m_Notifications;
	int m_MaxHealth;
	int m_MaxArmor;
	int m_MaxAmmo;
};

#endif
