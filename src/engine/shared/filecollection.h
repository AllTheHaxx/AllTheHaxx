/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_FILECOLLECTION_H
#define ENGINE_SHARED_FILECOLLECTION_H

class CFileCollection
{
	enum
	{
		MAX_ENTRIES=1000,
		TIMESTAMP_LENGTH=20,	// _YYYY-MM-DD_HH-MM-SS
	};

	int64 m_aTimestamps[MAX_ENTRIES];
	int m_NumTimestamps;
	int m_MaxEntries;
	char m_aFileDesc[128];
	int m_FileDescLength;
	char m_aFileExt[32];
	int m_FileExtLength;
	char m_aPath[512];
	IStorageTW *m_pStorage;
	int64 m_Remove; // Timestamp we want to remove

	bool IsFilenameValid(const char *pFilename);
	int64 ExtractTimestamp(const char *pTimestring);
	void BuildTimestring(int64 Timestamp, char *pTimestring);
	int64 GetTimestamp(const char *pFilename);

public:
	void Init(IStorageTW *pStorage, const char *pPath, const char *pFileDesc, const char *pFileExt, int MaxEntries);
	void AddEntry(int64 Timestamp);

	static int FilelistCallback(const char *pFilename, int IsDir, int StorageType, void *pUser);
	static int RemoveCallback(const char *pFilename, int IsDir, int StorageType, void *pUser);
};

#endif
