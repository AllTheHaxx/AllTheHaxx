#ifndef BASE_SYSTEMPP_SYSTEMPP_H
#define BASE_SYSTEMPP_SYSTEMPP_H

#include <csignal>
#include <exception>
#include <vector>
#include <string>

#define dbg_assert_critical(test,msg) if(!(test)) throw CTWException(__FILE__, __LINE__, #test, msg)
#if defined(CONF_DEBUG)
	#define dbg_assert(test,msg) dbg_assert_imp(__FILE__, __LINE__, test, msg)
#else
	#define dbg_assert(test,msg) dbg_assert_critical(test,msg)
#endif

#define SELF_FROM_USER(TYPE) TYPE *pSelf = (TYPE*)pUser;
#define SELF_FROM_USERDATA(TYPE) TYPE *pSelf = (TYPE*)pUserData;


class CTWException : public std::exception
{
	char m_aWhat[1024];

public:
	CTWException();
	CTWException(const char *msg);
	CTWException(const char *pFilename, int Line, const char *pAssertStr, const char *pMsg);

	virtual const char *what() const throw ();
};

void dbg_abort();
void mem_debug_dump(struct IOINTERNAL *file); // means IOHANDLE but I don't want to include system.h here...

template <class TFN>
class CDeferHandler
{
public:
//	typedef void (*DEFER_FUNCTION)();

private:
	const TFN m_pFnDeferFunc;

public:
	explicit CDeferHandler(TFN pFnDeferFunc) : m_pFnDeferFunc(pFnDeferFunc)
	{
	}

	~CDeferHandler()
	{
		m_pFnDeferFunc();
	}
};

/**
 * The given function FUNC will automatically be executed with DATA as argument when the current scope dies
 */
template <class TFN>
CDeferHandler<TFN> CreateDeferHandler(TFN pFnDeferFunc) { return CDeferHandler<TFN>(pFnDeferFunc); }

#define DEFER(FUNC) auto __DeferHandler = CreateDeferHandler(FUNC);


void StringSplit(const char *pString, const char *pDelim, std::vector<std::string> *pDest);



#endif
