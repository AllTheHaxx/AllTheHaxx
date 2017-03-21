#ifndef ENGINE_CLIENT_DEBUG_H
#define ENGINE_CLIENT_DEBUG_H


class CDebugger
{
	MACRO_ALLOC_HEAP()
public:
	CDebugger();

public:
	static void Signalhandler(int sig);

private:
	void RegisterSignals();

};

#endif
