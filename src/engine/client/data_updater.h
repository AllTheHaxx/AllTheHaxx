#ifndef ENGINE_CLIENT_DATA_UPDATER_H
#define ENGINE_CLIENT_DATA_UPDATER_H
#include <string>
#include <map>

class CDataUpdater
{
	friend class CUpdater;

	class CUpdater *m_pUpdater;
	std::string m_Tree;

	// these two map file paths against their hashes
	std::map<std::string, std::string> m_LocalGlob;
	std::map<std::string, std::string> m_RemoteGlob;
	volatile int m_Progress;

public:
	void Init(class CUpdater *pUpdater, const char *pTreeStr);

	static void GitHashStr(const char *pFile, char *pBuffer, unsigned BufferSize);

private:
	static void ListdirThread(void *pUser);
	static void HashingThread(void *pUser);
	static int ListdirCallback(const char *pName, const char *pFullPath, int is_dir, int dir_type, void *pUser);
};


#endif
