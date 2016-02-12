#ifndef GAME_CLIENT_COMPONENTS_IRC_H
#define GAME_CLIENT_COMPONENTS_IRC_H

#include <engine/external/cpirc/IRC.h>
#include <game/client/component.h>

class CIRC : public CComponent
{
private:
	IRC m_Connection;

	void *m_pIRCThread;
	static void ListenIRCThread(void *pUser);

public:
	CIRC();

	virtual void OnConsoleInit();
	virtual void OnReset();

};
#endif
