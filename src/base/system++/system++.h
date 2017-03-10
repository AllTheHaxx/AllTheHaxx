#ifndef BASE_SYSTEMPP_SYSTEMPP_H
#define BASE_SYSTEMPP_SYSTEMPP_H

#include <exception>


#define dbg_assert(test,msg) if(!(test)) throw CTWException(__FILE__, __LINE__, #test, msg)

class CTWException : public std::exception
{
	char m_aWhat[1024];

public:
	CTWException();
	CTWException(const char *msg);
	CTWException(const char *pFilename, int Line, const char *pAssertStr, const char *pMsg);

	virtual const char *what() const throw ();
};

void dbg_break();


#endif
