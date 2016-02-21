#ifndef BASE_DEBUG_H
#define BASE_DEBUG_H

#include <stdlib.h>
#include <list>
#include <iostream>
#include <fstream>
#include "system.h"

#define DEBUG_CALLSTACK_SIZE 10

#define CALLSTACK_ADD() \
	{char aDCSBUFFER[64]; str_format(aDCSBUFFER, sizeof(aDCSBUFFER), "%s(%i) In '%s'", __FILE__, __LINE__, __func__); \
	g_CallStack.push_back(aDCSBUFFER);} debug_check()

static std::list<std::string> g_CallStack;

static void debug_check()
{
	if(g_CallStack.size() > DEBUG_CALLSTACK_SIZE)
	{
		while(g_CallStack.size() > DEBUG_CALLSTACK_SIZE)
			g_CallStack.pop_front();
	}
}

void debug_sighandler(int code)
{
	printf("\n\n\n");
	printf("##################################\n");
	printf("#### CAUGHT ILLEGAL SIGNAL %i ####\n", code);
	printf("##################################\n\n");
	printf("> Congraz, you found a bug. I'm sorry for this...\n\n");
	printf("Backtrace:\n");

	// print the stacktrace
	for(std::list<std::string>::iterator it = g_CallStack.end(); it != g_CallStack.begin(); it--)
		std::cout << " " << *it << std::endl;
	printf("\n");

	// save it to a file
	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	std::ofstream f;
	char aBuf[512];
	str_format(aBuf, sizeof(aBuf), "crash_%i-%i-%i_%i-%i.txt", timeinfo->tm_year+1900, timeinfo->tm_mon, timeinfo->tm_mday, timeinfo->tm_hour, timeinfo->tm_min);
	f.open(aBuf);

	if(!f.is_open())
	{
		printf("Failed to save the bug report: Could not open file");
		exit(code);
	}

	f << "### BEGIN CRASH REPORT ###\n\n";

#if defined(CONF_FAMILY_WINDOWS)
	f << "OS: Windows\n";
#else
#if defined(CONF_FAMILY_ANDROID)
	f << "OS: Android\n";
#else
#if defined(CONF_FAMILY_UNIX)
	f << "OS: Linux\n";
#else
	f << "OS: OSX (Not supported!)\n";
#endif
#endif
#endif

	f << aBuf << "\n";

	for(std::list<std::string>::iterator it = g_CallStack.end(); it != g_CallStack.begin(); it--)
		f << " " << *it << "\n";


	f << "\n### END CRASH REPORT ###";
	f.close();

	printf("A bug report has been saved in your teeworlds folder.\n");
	printf("I would be very grateful if you could create a ticket in our issues tracker:");
	printf("https://github.com/AllTheHaxx/AllTheHaxx/issues\n\n");

	exit(code);
}

#endif
