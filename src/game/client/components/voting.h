/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_VOTING_H
#define GAME_CLIENT_COMPONENTS_VOTING_H

#include <engine/shared/memheap.h>

#include <game/voting.h>
#include <game/client/component.h>
#include <game/client/ui.h>

class CVoting : public CComponent
{
	CHeap m_Heap;

	static void ConCallvote(IConsole::IResult *pResult, void *pUserData);
	static void ConVote(IConsole::IResult *pResult, void *pUserData);

	int64 m_Closetime;
	char m_aDescription[VOTE_DESC_LENGTH];
	char m_aReason[VOTE_REASON_LENGTH];
	int m_Voted;
	int m_Yes, m_No, m_Pass, m_Total;

	// for rendering
	float m_YesVal, m_NoVal;

	void AddOption(const char *pDescription);
	void ClearOptions();
	void Callvote(const char *pType, const char *pValue, const char *pReason);

public:
	int m_NumVoteOptions;
	CVoteOptionClient *m_pFirst;
	CVoteOptionClient *m_pLast;

	CVoteOptionClient *m_pRecycleFirst;
	CVoteOptionClient *m_pRecycleLast;

	CVoting();
	virtual void OnReset();
	virtual void OnConsoleInit();
	virtual void OnMessage(int Msgtype, void *pRawMsg);
	virtual void OnRender();

	void CalculateBars();
	void RenderBars(const CUIRect& Bars, bool Text) const;
	void RenderBarsVertical(const CUIRect& VBars, bool Text) const;

	void CallvoteSpectate(int ClientID, const char *pReason, bool ForceVote = false);
	void CallvoteKick(int ClientID, const char *pReason, bool ForceVote = false);
	void CallvoteOption(int OptionID, const char *pReason, bool ForceVote = false);
	void RemovevoteOption(int OptionID);
	void AddvoteOption(const char *pDescription, const char *pCommand);

	void Vote(int v); // -1 = no, 1 = yes

	int SecondsLeft() const { return (int)((m_Closetime - time_get())/time_freq()); }
	bool IsVoting() const { return m_Closetime != 0; }
	int TakenChoice() const { return m_Voted; }
	const char *VoteDescription() const { return m_aDescription; }
	const char *VoteReason() const { return m_aReason; }

	// for lua
	void VoteYes() { Vote(1); }
	void VoteNo() { Vote(-1); }
	int GetYes() const { return m_Yes; }
	int GetNo() const { return m_No; }
	int GetPass() const { return m_Pass; }
	int GetTotal() const { return m_Total; }
	std::string VoteDescriptionSTD() const { return std::string(m_aDescription); }
	std::string VoteReasonSTD() const { return std::string(m_aReason); }
};

#endif
