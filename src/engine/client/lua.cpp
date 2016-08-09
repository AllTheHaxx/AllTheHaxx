#include <fstream>

#include <openssl/sha.h>
#include <base/system.h>
#include <engine/storage.h>
#include <engine/client.h>
#include <engine/console.h>
#include <engine/shared/network.h>

#include "lua.h"
#include "luabinding.h"


IClient * CLua::m_pClient = 0; 
CClient * CLua::m_pCClient = 0;
IGameClient * CLua::m_pGameClient = 0;
CGameClient * CLua::m_pCGameClient = 0;

#if defined(FEATURE_LUA)
using namespace luabridge;
#endif

CLua::CLua()
{
	m_pStorage = 0;
	m_pConsole = 0;
}

CLua::~CLua()
{
	Shutdown();
}

void CLua::Init(IClient *pClient, IStorageTW *pStorage, IConsole *pConsole)
{
	CALLSTACK_ADD();

	m_pClient = pClient;
	m_pCClient = (CClient*)pClient;
	m_pStorage = pStorage;
	m_pConsole = pConsole;
	m_aAutoloadFiles.clear();

	//LoadFolder(); // we can't do it that early
}

void CLua::Shutdown()
{
	CALLSTACK_ADD();

	SaveAutoloads();

	m_pLuaFiles.delete_all();
	m_pLuaFiles.clear();
}

void CLua::SaveAutoloads()
{
	CALLSTACK_ADD();
#if defined(FEATURE_LUA)
	char aFilePath[768];
	fs_storage_path("Teeworlds", aFilePath, sizeof(aFilePath));
	str_append(aFilePath, "/luafiles.cfg", sizeof(aFilePath));
	std::ofstream f(aFilePath, std::ios::out | std::ios::trunc);
	for(int i = 0; i < m_pLuaFiles.size(); i++)
		if(m_pLuaFiles[i]->GetScriptIsAutoload())
			f << m_pLuaFiles[i]->GetFilename() << std::endl;
	f.close();
#endif
}

void CLua::SortLuaFiles()
{
	CALLSTACK_ADD();

	const int NUM = m_pLuaFiles.size();
	if(NUM < 2)
		return;

	for(int curr = 0; curr < NUM-1; curr++)
	{
		int minIndex = curr;
		for(int i = curr + 1; i < NUM; i++)
		{
			int c = 4;
			for(; str_uppercase(m_pLuaFiles[i]->GetFilename()[c]) == str_uppercase(m_pLuaFiles[minIndex]->GetFilename()[c]); c++);
			if(str_uppercase(m_pLuaFiles[i]->GetFilename()[c]) < str_uppercase(m_pLuaFiles[minIndex]->GetFilename()[c]))
				minIndex = i;
		}

		if(minIndex != curr)
		{
			CLuaFile* temp = m_pLuaFiles[curr];
			m_pLuaFiles[curr] = m_pLuaFiles[minIndex];
			m_pLuaFiles[minIndex] = temp;
		}
	}
}

void CLua::SetGameClient(IGameClient *pGameClient)
{
	CALLSTACK_ADD();

	CLua::m_pGameClient = pGameClient;
	CLua::m_pCGameClient = (CGameClient*)pGameClient;
}

void CLua::AddUserscript(const char *pFilename)
{
	CALLSTACK_ADD();

#if defined(FEATURE_LUA)
	if(!pFilename || pFilename[0] == '\0' || str_length(pFilename) <= 4 || str_comp_nocase(&pFilename[str_length(pFilename)]-4, ".lua")
																		&& str_comp_nocase(&pFilename[str_length(pFilename)]-4, ".clc")) // "compiled lua chunk"
		return;

	// don't add duplicates
	for(int i = 0; i < m_pLuaFiles.size(); i++)
		if(str_comp(m_pLuaFiles[i]->GetFilename(), pFilename) == 0)
			return;

	bool Compiled = str_comp_nocase(&pFilename[str_length(pFilename)]-4, ".clc") == 0;

	std::string file = pFilename;

	// check for autoload
	bool Autoload = false;
	for(int i = 0; i < m_aAutoloadFiles.size(); i++)
		if(m_aAutoloadFiles[i] == file)
			Autoload = true;

	if(g_Config.m_Debug)
		dbg_msg("Lua", "adding%sscript '%s' to the list", Compiled ? " COMPILED " : " ", file.c_str());

	int index = m_pLuaFiles.add(new CLuaFile(this, file, Autoload));
	if(Autoload)
		m_pLuaFiles[index]->Init();
#endif
}

void CLua::LoadFolder()
{
	CALLSTACK_ADD();

	LoadFolder("lua");
}

void CLua::LoadFolder(const char *pFolder)
{
	CALLSTACK_ADD();

#if defined(FEATURE_LUA)
	// get the files which should be auto-loaded from file
	{
		m_aAutoloadFiles.clear();
		char aFilePath[768];
		fs_storage_path("Teeworlds", aFilePath, sizeof(aFilePath));
		str_append(aFilePath, "/luafiles.cfg", sizeof(aFilePath));
		std::ifstream f(aFilePath);
		if(f.is_open())
		{
			std::string line;
			while(std::getline(f, line))
				m_aAutoloadFiles.add(line);
			f.close();
		}
	}

	//char FullDir[256];
	//str_format(FullDir, sizeof(FullDir), "lua");

	dbg_msg("Lua", "Loading Folder '%s'", pFolder);
	CLua::LuaLoadHelper * pParams = new CLua::LuaLoadHelper;
	pParams->pLua = this;
	pParams->pString = pFolder;

	m_pStorage->ListDirectory(IStorageTW::TYPE_ALL, pFolder, LoadFolderCallback, pParams);

	delete pParams;

	SortLuaFiles();
#endif
}

int CLua::LoadFolderCallback(const char *pName, int IsDir, int DirType, void *pUser)
{
	CALLSTACK_ADD();

	if(pName[0] == '.')
		return 0;

	LuaLoadHelper *pParams = (LuaLoadHelper *)pUser;

	CLua *pSelf = pParams->pLua;
	const char *pFullDir = pParams->pString;

	char File[64];
	str_format(File, sizeof(File), "%s/%s", pFullDir, pName);
	//dbg_msg("Lua", "-> Found File %s", File);

	if(IsDir)
		pParams->pLua->LoadFolder(File);
	else
		pSelf->AddUserscript(File);
	return 0;
}

int CLua::HandleException(std::exception &e, CLuaFile* culprit)
{
	for(int i = 0; i < m_ErrorCounter.size(); i++)
	{
		if(m_ErrorCounter[i].culprit == culprit)
		{
			char aError[1024];
			str_format(aError, sizeof(aError), "{%i/511} %s", m_ErrorCounter[i].count, e.what());
			m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "lua|EXCEPTION", aError);
			if(++m_ErrorCounter[i].count < 512)
				return m_ErrorCounter[i].count;
			m_ErrorCounter[i].count = 0;
			culprit->m_pErrorStr = "Error count limit exceeded (too many exceptions thrown)";
			culprit->Unload(true);
			dbg_msg("lua|ERROR", "<<< unloaded script '%s' (error count exceeded limit)", culprit->GetFilename());
		}
	}

	LuaErrorCounter x;
	x.culprit = culprit;
	x.count = 1;
	m_ErrorCounter.add(x);

	return 1;
}

int CLua::Panic(lua_State *L)
{
	CALLSTACK_ADD();

#if defined(FEATURE_LUA)
	dbg_msg("LUA/FATAL", "panic [%p] %s", L, lua_tostring(L, -1));
	dbg_break();
#endif
	return 0;
}

int CLua::ErrorFunc(lua_State *L)
{
	CALLSTACK_ADD();

#if defined(FEATURE_LUA)
	dbg_msg("Lua", "Lua Script Error! :");
	//lua_getglobal(L, "pLUA");
	//CLua *pSelf = (CLua *)lua_touserdata(L, -1);
	//lua_pop(L, 1);

	//int depth = 0;
	//int frameskip = 1;
	//lua_Debug frame;

	if (lua_tostring(L, -1) == 0)
	{
		//dbg_msg("Lua", "PANOS");
		return 0;
	}
	
	//dbg_msg("Lua", pSelf->m_aFilename);
	dbg_msg("Lua", lua_tostring(L, -1));
	/*dbg_msg("Lua", "Backtrace:");

	while(lua_getstack(L, depth, &frame) == 1)
	{
		depth++;
		lua_getinfo(L, "nlSf", &frame);
		// check for functions that just report errors. these frames just confuses more then they help
		if(frameskip && str_comp(frame.short_src, "[C]") == 0 && frame.currentline == -1)
			continue;
		frameskip = 0;
		// print stack frame
		dbg_msg("Lua", "%s(%d): %s %s", frame.short_src, frame.currentline, frame.name, frame.namewhat);
	}*/
	lua_pop(L, 1); // remove error message
	lua_gc(L, LUA_GCCOLLECT, 0);
#endif
	return 0;
}



bool CLuaFile::CheckCertificate(const char *pFilename)
{
#if defined(FEATURE_LUA)
	if(str_comp_nocase(&pFilename[str_length(pFilename)]-4, ".clc") == 0)
	{
		char aCertFile[256];
		str_copy(aCertFile, pFilename, sizeof(aCertFile)); // get the path of the file
		str_replace_char_rev_num(aCertFile, 1, '/', '\0'); // cut off the filename
		str_append(aCertFile, "/.cert/", sizeof(aCertFile)); // append the certs folder name
		str_append(aCertFile, str_find_rev(pFilename, "/"), sizeof(aCertFile)); // re-append the name of the luafile
		str_replace_char_rev_num(aCertFile, 1, '.', '\0');  // cut off the file ending
		str_append(aCertFile, ".cert", sizeof(aCertFile));  // append the file ending of certs
		IOHANDLE f = Lua()->Storage()->OpenFile(aCertFile, IOFLAG_READ, IStorageTW::TYPE_ALL); // hope that it works.
		if(!f)
		{
			dbg_msg("lua", "failed to open certificate file '%s'", aCertFile);
			return false;
		}

#ifdef CONF_ARCH_ENDIAN_LITTLE
		bool CurrBigEndian = false;
#elif defined(CONF_ARCH_ENDIAN_BIG)
		bool CurrBigEndian = true;
#endif
		// some (uncompressed) meta data
		int DataSize;
		bool FileBigEndian;
		io_read(f, &FileBigEndian, sizeof(bool));
		io_read(f, &DataSize, sizeof(int));

		// the (compressed) certificate data
		char aData[sizeof(LuaBinaryCert)] = {0};
		if(io_read(f, aData, (unsigned int)DataSize) != DataSize)
		{
			dbg_msg("lua", "corrupted certificate '%s'", aCertFile);
			io_close(f);
			return false;
		}
		io_close(f);

		// correct the endianess if neccesary
		if(CurrBigEndian != FileBigEndian)
			swap_endian(aData, 1, (unsigned int)DataSize);

		LuaBinaryCert cert;
		mem_zero(&cert, sizeof(cert));

		if(DataSize == sizeof(LuaBinaryCert)) // saved data is not compressed
			mem_copy(&cert, aData, (unsigned int)DataSize); // -> copy it as is
		else
		{
			int DecompressedSize = CNetBase::Decompress(aData, DataSize, &cert, sizeof(LuaBinaryCert));
			if(DecompressedSize <= 0)
			{
				dbg_msg("lua", "failed to decompress cert '%s' (%i => %i)", aCertFile, DataSize, sizeof(LuaBinaryCert));
				return false;
			}

			if(g_Config.m_Debug)
				dbg_msg("lua", "decompressed cert '%s' (%i => %i)", aCertFile, DataSize, DecompressedSize);

			// check the certificate
			f = Lua()->Storage()->OpenFile(pFilename, IOFLAG_READ, IStorageTW::TYPE_ALL);
			if(!f)
			{
				dbg_msg("lua", "failed to open lua script file '%s' for reading", pFilename);
				return false;
			}
			unsigned int len = (unsigned int)io_length(f);
			char *aScript = (char*)mem_alloc(len, 0);
			io_read(f, aScript, len);
			unsigned char md[SHA256_DIGEST_LENGTH] = {0};
			int ret = simpleSHA256(aScript, len, md);
			mem_free(aScript);
			if(ret != 0)
			{
				dbg_msg("lua", "failed to hash compiled script '%s' for cert check [ERROR %i]", pFilename, ret);
				return false;
			};

			// assemble the hashes into strings
			char aStrHash[2][128] = {{0}};
			for(int k = 0; k < 2; k++)
			{
				for(int i = 0; i < SHA256_DIGEST_LENGTH; i++)
				{
					char aHex[3];
					str_format(aHex, sizeof(aHex), "%02x", k == 0 ? md[i] : cert.aHashMD[i]);
					str_append(aStrHash[k], aHex, sizeof(aStrHash[k]));
				}
			}

			if(str_comp(aStrHash[0], aStrHash[1]) != 0)
			{
				dbg_msg("lua", "certificate mismatch for script '%s'", pFilename);
				dbg_msg("lua", " :  (%s != %s)", aStrHash[0], aStrHash[1]);
				return false;
			}

			dbg_msg("lua", "success: certificate check for '%s' [[ ISSUER='%s' DATE='%s' ]]", pFilename, cert.aIssuer, cert.aDate);
		}
	}

	return true;
#else
	return false;
#endif
}
