#ifndef ENGINE_CURLWRAPPER_H
#define ENGINE_CURLWRAPPER_H

#include "kernel.h"

class ICurlWrapper : public IInterface
{
	MACRO_INTERFACE("curlwrapper", 0)
public:
	virtual void PerformSimplePOST(const char *pUrl, const char *pFields) = 0;
};
#endif
