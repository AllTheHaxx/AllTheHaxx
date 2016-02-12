#include <base/system.h>
#include <engine/shared/config.h>

#if defined(CONF_FAMILY_WINDOWS)
	#include <windows.h>
#endif

#include "irc.h"

int end_of_motd(char* params, irc_reply_data* hostd, void* conn) // our callback function
{
	IRC* irc_conn=(IRC*)conn;

	irc_conn->join((char *)"#AllTheHaxx"); // join the channel #AllTheHaxx
	return 0;
}

CIRC::CIRC()
{
	OnReset();
}

void CIRC::ListenIRCThread(void *pUser)
{
	CIRC *pData = (CIRC *)pUser;

	#if defined(CONF_FAMILY_WINDOWS)
		WSADATA wsaData; /* winsock stuff, linux/unix/bsd users need not worry about this */
	
		if (WSAStartup(MAKEWORD(1, 1), &wsaData)) /* more winsock rubbish */
		{
			printf("Failed to initialise winsock!\n");
		}
	#endif

	pData->m_Connection.hook_irc_command((char *)"376", &end_of_motd); // hook the end of MOTD message
	pData->m_Connection.start((char *)"irc.quakenet.org", 6668, (char *)"sogehts", (char *)"aberNichtMitVars", (char *)"fullname bla", (char *)"password"); // connect to the server
	pData->m_Connection.message_loop();

	#if defined(CONF_FAMILY_WINDOWS)
		WSACleanup(); /* more winsock stuff */
	#endif
}

void CIRC::OnReset()
{
	m_pIRCThread = thread_init(ListenIRCThread, this);
}
