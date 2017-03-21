#include <cstdio> // XXX
#include <cstdlib>
#include <csignal>

#if defined(CONF_FAMILY_WINDOWS)
#include <dbghelp.h>
#include <shellapi.h>
#include <shlobj.h>
#endif

#include <base/system.h>
#include "debug.h"

CDebugger::CDebugger()
{
#if defined(FEATURE_DEBUGGER) && !defined(CONF_DEBUG)
	RegisterSignals();
#endif
}

void CDebugger::RegisterSignals()
{
	signal(SIGABRT, CDebugger::Signalhandler);
	signal(SIGFPE, CDebugger::Signalhandler);
	signal(SIGILL, CDebugger::Signalhandler);
	/*signal(SIGINT, CDebugger::Signalhandler);*/ // DON'T ever catch this!!
	signal(SIGSEGV, CDebugger::Signalhandler);
	/*signal(SIGTERM, CDebugger::Signalhandler);*/ // not sure, but I think this shall not be caught aswell
#if defined(CONF_FAMILY_UNIX)
	signal(SIGSTKFLT, CDebugger::Signalhandler);
	signal(SIGPIPE, CDebugger::Signalhandler);
	signal(SIGHUP, CDebugger::Signalhandler);
#endif

	dbg_msg("debugger", "signal hooks registered");
}

#if defined(CONF_FAMILY_WINDOWS) && defined(FEATURE_DEBUGGER) && !defined(CONF_DEBUG)
static int GenerateDump(char *pDst, unsigned BufferSize)
{
	BOOL bMiniDumpSuccessful;
	WCHAR szPath[MAX_PATH];
	WCHAR szFileName[MAX_PATH];
	WCHAR* szAppName = L"AppName";
	WCHAR* szVersion = L"v1.0";
	DWORD dwBufferSize = MAX_PATH;
	HANDLE hDumpFile;
	SYSTEMTIME stLocalTime;

	GetLocalTime( &stLocalTime );
	GetTempPath( dwBufferSize, szPath );

	StringCchPrintf( szFileName, MAX_PATH, L"%s%s", szPath, szAppName );
	CreateDirectory( szFileName, NULL );

	StringCchPrintf( szFileName, MAX_PATH, L"%s%s\\%s-%04d%02d%02d-%02d%02d%02d-%ld-%ld.dmp",
					 szPath, szAppName, szVersion,
					 stLocalTime.wYear, stLocalTime.wMonth, stLocalTime.wDay,
					 stLocalTime.wHour, stLocalTime.wMinute, stLocalTime.wSecond,
					 GetCurrentProcessId(), GetCurrentThreadId());
	hDumpFile = CreateFile(szFileName, GENERIC_READ|GENERIC_WRITE,
						   FILE_SHARE_WRITE|FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	bMiniDumpSuccessful = MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(),
											hDumpFile, MiniDumpWithDataSegs, NULL, NULL, NULL);

	return EXCEPTION_EXECUTE_HANDLER;
}
#endif


void CDebugger::Signalhandler(int Sig)
{
	const char *paSigNames[] = {
		"_INTERNAL_ERROR" // there is no 0
		"SIGHUP",	//	1	/* Hangup (POSIX).  */
		"SIGINT",	//	2	/* Interrupt (ANSI).  */
		"SIGQUIT",	//	3	/* Quit (POSIX).  */
		"SIGILL",	//	4	/* Illegal instruction (ANSI).  */
		"SIGTRAP",	//	5	/* Trace trap (POSIX).  */
		"SIGABRT",	//	6	/* Abort (ANSI).  */
		"SIGIOT",	//	6	/* IOT trap (4.2 BSD).  */
		"SIGBUS",	//	7	/* BUS error (4.2 BSD).  */
		"SIGFPE",	//	8	/* Floating-point exception (ANSI).  */
		"SIGKILL",	//	9	/* Kill, unblockable (POSIX).  */
		"SIGUSR1",	//	10	/* User-defined signal 1 (POSIX).  */
		"SIGSEGV",	//	11	/* Segmentation violation (ANSI).  */
		"SIGUSR2",	//	12	/* User-defined signal 2 (POSIX).  */
		"SIGPIPE",	//	13	/* Broken pipe (POSIX).  */
		"SIGALRM",	//	14	/* Alarm clock (POSIX).  */
		"SIGTERM",	//	15	/* Termination (ANSI).  */
		"SIGSTKFLT"	//	16	/* Stack fault.  */
	};
	printf("\n\n\n\n\n\n\n\nBUG BUG! sig=%s (%i)\n", paSigNames[Sig>16||Sig<1?0:Sig], Sig);


	char aBuf[256];
	char aMsg[512];
	str_copyb(aMsg, "Whoups, the client just crashed. I'm sorry... ");

#if defined(CONF_FAMILY_WINDOWS)

	char aFile[512];
    __try
    {
        int *pBadPtr = NULL;
        *pBadPtr = 0;
    }
    __except(GenerateDump(GetExceptionInformation(), aFile, sizeof(aFile)))
    {
    }

	str_format(aBuf, sizeof(aBuf), "A crash dump has been saved to '%s' and will be sent on next client start."
			" It doesn't contain any sensitive info and will be very helpful for us to fix the bug, thanks! :)", aFile);

#endif

	str_appendb(aMsg, "   [If your mouse is still grabbed, you can press either ENTER or ALt+F4 to close this]");

	const char *paFunnyMessages[] = { // these should - please - fit to a crash. Thanks in advance.
			"I know what you think...",
			"This was not supposed to happen!",
			"Who is responsible for that mess?",
			"That's not my fault!",
			"My cat ate the alien intelligence",
			"What are we going to do now?",
			"YOU DIDN'T SAW THAT! IT'S CONFIDENTIAL!",
			"Please act as if nothing had happened",
			"Panda??",
			"Paper > All",
			"Grab a pencil and a pigeon",
			"asm()",
	};

	size_t n = rand()%(sizeof(paFunnyMessages)/sizeof(paFunnyMessages[0]));
	str_format(aBuf, sizeof(aBuf), "AllTheHaxx Crash  --  %s", paFunnyMessages[n]);
	gui_messagebox(aBuf, aMsg);
	printf("\nexiting\n");

#if defined(CONF_FAMILY_WINDOWS)
	exit(Sig);
#endif

}
