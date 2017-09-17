#include "system++.h"
#include <base/system.h>
#include <base/math.h>

CTWException::CTWException()
{
	str_copyb(m_aWhat, "unknown error");
#if defined(CONF_DEBUG)
	dbg_msg("exception", "an exception was thrown: %s", m_aWhat);
#endif
}

CTWException::CTWException(const char *msg)
{
	str_formatb(m_aWhat, "%s", msg);
#if defined(CONF_DEBUG)
	dbg_msg("exception", "an exception was thrown: %s", m_aWhat);
#endif
}

CTWException::CTWException(const char *pFilename, int Line, const char *pAssertStr, const char *pMsg)
{
	str_formatb(m_aWhat, "At %s(%d) assert '%s' failed: %s", pFilename, Line, pAssertStr, pMsg);
#if defined(CONF_DEBUG)
	dbg_msg("assert", "%s", m_aWhat);
#endif
}

const char *CTWException::what() const throw ()
{
	return m_aWhat;
}


void dbg_break()
{
	wait_log_queue();
	throw CTWException("dbg_break");
}

void StringSplit(const char *pString, const char *pDelim, std::vector<std::string> *pDest)
{
	const char *pFound = pString;
	const char *pLast = pString;
	while((pFound = str_find(pFound, pDelim)))
	{
		pFound++;
		char aPart[512];
		str_copy(aPart, pLast, (int)min<long unsigned int>((long unsigned int)sizeof(aPart), (long unsigned int)(pFound - pLast)));
		pDest->push_back(std::string(aPart));

		if(*(pLast = pFound) == '\0')
			break;
	}

	pDest->push_back(pLast);
}
