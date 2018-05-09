/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/config.h>
#include <engine/engine.h>
#include <engine/storage.h>
#include <engine/shared/config.h>
#include <engine/shared/protocol.h>
#include <base/system++/system++.h>

CConfiguration g_Config;

class CConfig : public IConfig
{
	IStorageTW *m_pStorage;
	IOHANDLE m_ConfigFile;

	struct CCallback
	{
		SAVECALLBACKFUNC m_pfnFunc;
		void *m_pUserData;
	};

	enum
	{
		MAX_CALLBACKS = 16
	};

	CCallback m_aCallbacks[MAX_CALLBACKS];
	int m_NumCallbacks;

	void EscapeParam(char *pDst, const char *pSrc, int Size)
	{
		str_escape(&pDst, pSrc, pDst + Size);
	}

public:

	CConfig()
	{
		m_ConfigFile = 0;
		m_NumCallbacks = 0;
	}

	virtual void Init()
	{
		m_pStorage = Kernel()->RequestInterface<IStorageTW>();
		Reset();
	}

	virtual void Reset()
	{
		#define MACRO_CONFIG_INT(Name,ScriptName,def,min,max,flags,desc) g_Config.m_##Name = def;
		#define MACRO_CONFIG_STR(Name,ScriptName,len,def,flags,desc) str_copy(g_Config.m_##Name, def, len);

		#include "config_variables.h"

		#undef MACRO_CONFIG_INT
		#undef MACRO_CONFIG_STR
	}

	virtual bool Save(bool Force=false)
	{
		if(!m_pStorage || (!Force && !g_Config.m_ClSaveSettings))
			return false;
		m_ConfigFile = m_pStorage->OpenFile(CONFIG_FILE, IOFLAG_WRITE, IStorageTW::TYPE_SAVE);

		if(!m_ConfigFile)
		{
			IEngine *pEngine = Kernel()->RequestInterface<IEngine>();
			if(pEngine)
				pEngine->WriteErrorLog("config", "failed to save config: file not accessible");
			return false;
		}

		char aLineBuf[1024*2];
		char aEscapeBuf[1024*2];

		#define MACRO_CONFIG_INT(Name,ScriptName,def,min,max,flags,desc) if((flags)&CFGFLAG_SAVE) { str_format(aLineBuf, sizeof(aLineBuf), "%s %i", #ScriptName, g_Config.m_##Name); WriteLine(aLineBuf); }
		#define MACRO_CONFIG_STR(Name,ScriptName,len,def,flags,desc) if((flags)&CFGFLAG_SAVE) { EscapeParam(aEscapeBuf, g_Config.m_##Name, sizeof(aEscapeBuf)); str_format(aLineBuf, sizeof(aLineBuf), "%s \"%s\"", #ScriptName, aEscapeBuf); WriteLine(aLineBuf); }

		#include "config_variables.h"

		#undef MACRO_CONFIG_INT
		#undef MACRO_CONFIG_STR

		for(int i = 0; i < m_NumCallbacks; i++)
			m_aCallbacks[i].m_pfnFunc(this, m_aCallbacks[i].m_pUserData);

		io_close(m_ConfigFile);
		m_ConfigFile = 0;

		return true;
	}

	virtual void RegisterCallback(SAVECALLBACKFUNC pfnFunc, void *pUserData)
	{
		dbg_assert(m_NumCallbacks < MAX_CALLBACKS, "too many config callbacks");
		m_aCallbacks[m_NumCallbacks].m_pfnFunc = pfnFunc;
		m_aCallbacks[m_NumCallbacks].m_pUserData = pUserData;
		m_NumCallbacks++;
	}

	virtual void WriteLine(const char *pLine)
	{
		if(!m_ConfigFile)
			return;
		io_write(m_ConfigFile, pLine, str_length(pLine));
		io_write_newline(m_ConfigFile);
	}
};

IConfig *CreateConfig() { return new CConfig; }
