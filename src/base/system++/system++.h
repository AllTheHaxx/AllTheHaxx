#ifndef BASE_SYSTEMPP_SYSTEMPP_H
#define BASE_SYSTEMPP_SYSTEMPP_H

#include <exception>


#define dbg_assert(test,msg) if(!(test)) throw CTWException(__FILE__, __LINE__, #test, msg)

#define SELF_FROM_USER(TYPE) TYPE *pSelf = (TYPE*)pUser;


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


class CDeferHandler
{
public:
	typedef void (*DEFER_FUNCTION)(void *pData);

private:
	DEFER_FUNCTION m_pFnDeferFunc;
	void *m_pData;

public:
	CDeferHandler(DEFER_FUNCTION pFnDeferFunc, void *pData)
	{
		m_pFnDeferFunc = pFnDeferFunc;
		m_pData = pData;
	}

	~CDeferHandler()
	{
		m_pFnDeferFunc(m_pData);
	}
};


/**
 * The given function FUNC will automatically be executed with DATA as argument right before the current function returns
 */
#define DEFER(DATA, FUNC) CDeferHandler __DeferHandler(FUNC, DATA);


#endif
