#include <base/system.h>

#include <mastersrv/mastersrv.h>
#include <engine/masterserver.h>

#include <engine/config.h>
#include <engine/shared/memheap.h>
#include <engine/shared/network.h>
#include <engine/client/serverbrowser.h>
#include <engine/serverbrowser.h>

CNetClient *pNet;
CServerBrowser *m_pServerBrowser;

static int Run()
{
	NETADDR BindAddr = {NETTYPE_IPV4, {0}, 4200};

	dbg_msg("dataminer", "starting...");

	if(!pNet->Open(BindAddr, 0))
		return 0;

	while(1)
	{
		CNetChunk p;
		pNet->Update();
		while(pNet->Recv(&p))
		{
			if(p.m_DataSize >= (int)sizeof(SERVERBROWSE_INFO) && 
				mem_comp(p.m_pData, SERVERBROWSE_INFO, sizeof(SERVERBROWSE_INFO)) == 0)
			{
				dbg_msg("dbg", "yo");
			}
		}

		thread_sleep(100);
	}
}

int main(int argc, char **argv)
{
	dbg_logger_stdout();
	pNet = new CNetClient();
	m_pServerBrowser = new CServerBrowser();

	int RunReturn = Run();

	delete pNet;
	return RunReturn;
}
