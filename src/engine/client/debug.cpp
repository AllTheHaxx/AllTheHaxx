#include <stdio.h> // XXX
#include <stdlib.h>

#include <base/system.h>
#include <game/version.h>
#include "debug.h"

#define CALLSTACK_SIZE 10

IStorageTW * CDebugger::m_pStorage = 0;

CCallstack gDebugInfo;

CDebugger::CDebugger()
{
	for(int i = 0; i < CALLSTACK_SIZE; i++)
		gDebugInfo.m_CallStack.push_front("..");

	RegisterSignals();
}

void CDebugger::RegisterSignals()
{
	signal(SIGABRT, signalhandler);
	signal(SIGFPE, signalhandler);
	signal(SIGILL, signalhandler);
	/*signal(SIGINT, signalhandler);*/ // DON'T ever catch this!!
	signal(SIGSEGV, signalhandler);
	/*signal(SIGTERM, signalhandler);*/ // not sure, but I think this shall not be caught aswell
#if defined(CONF_FAMILY_UNIX)
	signal(SIGSTKFLT, signalhandler);
	signal(SIGPIPE, signalhandler);
	signal(SIGHUP, signalhandler);
#endif
}

void CCallstack::CallstackAdd(const char *file, int line, const char *function)
{
	static char aStamp[128]; // static so that we only need to allocate once -> faster
	str_format(aStamp, sizeof(aStamp), "at %s:%i in %s", file, line, function);
	gDebugInfo.m_CallStack.pop_back();
	gDebugInfo.m_CallStack.push_front(std::string(aStamp));
}

void signalhandler(int sig) { CDebugger::signalhandler_ex(sig); }
void CDebugger::signalhandler_ex(int sig)
{
	printf("\n\n\n\n\n\nBUG BUG! sig=%i\n", sig);

	for(std::list<std::string>::iterator it = gDebugInfo.m_CallStack.begin(); it != gDebugInfo.m_CallStack.end(); it++)
	{
		printf("%s\n", it->c_str());
	}

	fs_makedir("./crashlogs");
	char aFile[512], aBuf[768];
	time_t rawtime;
	time(&rawtime);
	str_timestamp_ex(rawtime, aFile, sizeof(aFile), "crashlogs/report_%Y%m%d%H%M%S.log");
	IOHANDLE f = io_open(aFile, IOFLAG_WRITE);
	bool ReportExists = true;

	if(!f)
	{
		ReportExists = false;
		printf("<<<< FAILED TO OPEN '%s' FOR WRITING >>>>\n", aFile);
		printf("<<<<  no crash report will be saved!  >>>>\n\n\n");
	}
	else
	{
		#define WRITE_LINE() \
				io_write(f, aBuf, str_length(aBuf)); \
				io_write_newline(f)

		str_format(aBuf, sizeof(aBuf), "### athcrash.report");
		WRITE_LINE();

		str_format(aBuf, sizeof(aBuf), "### %s", aFile);
		WRITE_LINE();
		io_write_newline(f);

		str_format(aBuf, sizeof(aBuf), "OS: " CONF_FAMILY_STRING "/" CONF_PLATFORM_STRING);
		WRITE_LINE();

		str_format(aBuf, sizeof(aBuf), "VERSION: " ALLTHEHAXX_VERSION ".%i.%i (" GAME_VERSION "/" ATH_VERSION ")", GAME_ATH_VERSION_NUMERIC, CLIENT_VERSIONNR);
		WRITE_LINE();

		str_format(aBuf, sizeof(aBuf), "ERRSIG: %i", sig);
		WRITE_LINE();

		io_write_newline(f);
		io_write_newline(f);

		str_format(aBuf, sizeof(aBuf), "******* BEGIN CALLSTACK *******");
		WRITE_LINE();

		for(std::list<std::string>::iterator it = gDebugInfo.m_CallStack.begin(); it != gDebugInfo.m_CallStack.end(); it++)
		{
			io_write(f, it->c_str(), (unsigned int)str_length(it->c_str()));
			io_write_newline(f);
		}

		str_format(aBuf, sizeof(aBuf), "*******  END CALLSTACK  *******");
		WRITE_LINE();

		io_flush(f);
		io_close(f);
	}

	char aMsg[512];
	str_format(aMsg, sizeof(aMsg), "Whoups, the client just crashed. I'm sorry... ");

	if(ReportExists)
	{
		str_format(aBuf, sizeof(aBuf), "A crash report has been saved to '%s' and will be sent on next client start. "
										 "It doesn't contain any sensitive info and will be very helpful for us to fix the bug, thanks! :)", aFile);

	}
	else
		str_format(aBuf, sizeof(aBuf), "Unfortunately, we couldn't write to the file '%s' in order to save a report :/", aFile);

	str_append(aMsg, aBuf, sizeof(aMsg));

	gui_messagebox("AllTheHaxx Crash :S", aMsg);
	printf("\nexiting\n");
	exit(0xEE00|sig);
}
