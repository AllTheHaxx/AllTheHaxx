#ifndef ENGINE_CLIENT_LUA_H
#define ENGINE_CLIENT_LUA_H

#include <string>
#if defined(FEATURE_LUA)
#include <lua.hpp>
#endif
#include <base/tl/array.h>
#include <engine/external/openssl/sha.h>
#include <engine/external/zlib/zlib.h>
#include "luafile.h"

#if defined(FEATURE_LUA)
#define LUA_FIRE_EVENT(EVENTNAME, ...) \
	{ \
		if(g_Config.m_ClLua) \
			for(int ijdfg = 0; ijdfg < Client()->Lua()->GetLuaFiles().size(); ijdfg++) \
			{ \
				if(Client()->Lua()->GetLuaFiles()[ijdfg]->State() != CLuaFile::STATE_LOADED) \
					continue; \
				LuaRef lfunc = Client()->Lua()->GetLuaFiles()[ijdfg]->GetFunc(EVENTNAME); \
				if(lfunc) try { lfunc(__VA_ARGS__); } catch(std::exception &e) { Client()->Lua()->HandleException(e, Client()->Lua()->GetLuaFiles()[ijdfg]); } \
			} \
			LuaRef confunc = getGlobal(CGameConsole::m_pStatLuaConsole->m_LuaHandler.m_pLuaState, EVENTNAME); \
			if(confunc) try { confunc(__VA_ARGS__); } catch(std::exception &e) { printf("LUA EXCEPTION: %s\n", e.what()); } \
	}
#else
#define LUA_FIRE_EVENT(EVENTNAME, ...) ;
#endif

class IClient;
class CClient;
class IStorageTW;
class IGameClient;
class CGameClient;
class CLuaFile;

#if defined(FEATURE_LUA)
using namespace luabridge;
#endif

struct LuaBinaryCert
{
	char aIssuer[64];
	char aDate[64];
	unsigned char aHashMD[SHA256_DIGEST_LENGTH];
	int PermissionFlags;
};

struct LuaCertHeader
{
	enum { LUA_CERT_VERSION = 3 };
	short Version;
	bool FileBigEndian;
	int DataSize;
};

/** functions in here are taken from the zlib usage example */
class CUnzip
{
	enum { CHUNK=16384 };

public:
	/** Decompress from file source to file dest until stream ends or EOF.
	 *
	 * @return inf() returns <code>Z_OK</code> on success, <code>Z_MEM_ERROR</code> if memory could not be
	 * allocated for processing, <code>Z_DATA_ERROR</code> if the deflate data is
	 * invalid or incomplete, <code>Z_VERSION_ERROR</code> if the version of zlib.h and
	 * the version of the library linked do not match, or <code>Z_ERRNO</code> if there
	 * is an error reading or writing the files.
	 */
	int inf(FILE *source, FILE *dest)
	{
		int ret;
		unsigned have;
		z_stream strm;
		static unsigned char in[CHUNK];
		static unsigned char out[CHUNK];

		/* allocate inflate state */
		strm.zalloc = Z_NULL;
		strm.zfree = Z_NULL;
		strm.opaque = Z_NULL;
		strm.avail_in = 0;
		strm.next_in = Z_NULL;
		ret = inflateInit(&strm);
		if (ret != Z_OK)
			return ret;

		/* decompress until deflate stream ends or end of file */
		do {
			strm.avail_in = fread(in, 1, CHUNK, source);
			if (ferror(source)) {
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}
			if (strm.avail_in == 0)
				break;
			strm.next_in = in;

			/* run inflate() on input until output buffer not full */
			do {
				strm.avail_out = CHUNK;
				strm.next_out = out;
				ret = inflate(&strm, Z_NO_FLUSH);
				assert(ret != Z_STREAM_ERROR);  /* state not clobbered */
				switch (ret) {
					case Z_NEED_DICT:
						ret = Z_DATA_ERROR;     /* and fall through */
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
						(void)inflateEnd(&strm);
						return ret;
				}
				have = CHUNK - strm.avail_out;
				if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
					(void)inflateEnd(&strm);
					return Z_ERRNO;
				}
			} while (strm.avail_out == 0);

			/* done when inflate() says it's done */
		} while (ret != Z_STREAM_END);

		/* clean up and return */
		(void)inflateEnd(&strm);
		return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
	}

	/** report a zlib or i/o error */
	void zerr(int ret)
	{
		fputs("zpipe: ", stderr);
		switch (ret) {
			case Z_ERRNO:
				if (ferror(stdin))
					fputs("error reading stdin\n", stderr);
				if (ferror(stdout))
					fputs("error writing stdout\n", stderr);
				break;
			case Z_STREAM_ERROR:
				fputs("invalid compression level\n", stderr);
				break;
			case Z_DATA_ERROR:
				fputs("invalid or incomplete deflate data\n", stderr);
				break;
			case Z_MEM_ERROR:
				fputs("out of memory\n", stderr);
				break;
			case Z_VERSION_ERROR:
				fputs("zlib version mismatch!\n", stderr);
		}
	}
};

class CLua
{
	array<CLuaFile*> m_pLuaFiles;
	array<std::string> m_aAutoloadFiles;
	IStorageTW *m_pStorage;
	class IConsole *m_pConsole;

	struct LuaErrorCounter
	{
		CLuaFile* culprit;
		int count;
	};
	array<LuaErrorCounter> m_ErrorCounter;

public:
	CLua();
	~CLua();
	
	void Init(IClient *pClient, IStorageTW *pStorage, IConsole *pConsole);
	void Shutdown();
	void SaveAutoloads();
	void AddUserscript(const char *pFilename);
	void LoadFolder();
	void LoadFolder(const char *pFolder);
	void SortLuaFiles();


	static int ErrorFunc(lua_State *L);
	static int Panic(lua_State *L);
	int HandleException(std::exception &e, CLuaFile*);

	static CClient * m_pCClient;
	static IClient *m_pClient;
	static IGameClient *m_pGameClient;
	static IClient *Client() { return m_pClient; }
	static IGameClient *GameClient() { return m_pGameClient; }
	static CGameClient * m_pCGameClient;
	
	void SetGameClient(IGameClient *pGameClient);
	array<CLuaFile*> &GetLuaFiles() { return m_pLuaFiles; }

	IStorageTW *Storage() const { return m_pStorage; }

	struct LuaLoadHelper
	{
		MACRO_ALLOC_HEAP()
	public:
		CLua * pLua;
		const char * pString;
	};

private:
	static int LoadFolderCallback(const char *pName, int IsDir, int DirType, void *pUser);

};

#endif
