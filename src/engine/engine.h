/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_ENGINE_H
#define ENGINE_ENGINE_H

#include "kernel.h"
#include <engine/shared/jobs.h>

class CHostLookup
{
public:
	CJob m_Job;
	char m_aHostname[128];
	int m_Nettype;
	NETADDR m_Addr;
};

class IEngine : public IInterface
{
	MACRO_INTERFACE("engine", 0)

protected:
	CJobPool m_JobPool;

public:
	virtual ~IEngine() { }

	virtual void Init() = 0;
	virtual void InitLogfile() = 0;
	virtual void WriteErrorLog(const char *pSys, const char *pFormat, ...)
	#if defined(DO_NOT_COMPILE_THIS_CODE) && (defined(__GNUC__) || defined(__clang__)) // dunno why this gives weird errors all over the place if compiled
	__attribute__ ((format (printf, 2, 3))) /* Warn if you specify wrong arguments in printf format string */
	#endif
	= 0;

	virtual void HostLookup(CHostLookup *pLookup, const char *pHostname, int Nettype) = 0;
	virtual void AddJob(CJob *pJob, JOBFUNC pfnFunc, void *pData) = 0;
};

extern IEngine *CreateEngine(const char *pAppname);

#endif
