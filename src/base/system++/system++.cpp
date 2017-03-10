#include "system++.h"
#include <base/system.h>

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
