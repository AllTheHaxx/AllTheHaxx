/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef VERSIONSRV_VERSIONSRV_H
#define VERSIONSRV_VERSIONSRV_H
static const int VERSIONSRV_PORT = 8302;
enum
{
	NEWS_SIZE = 4096,
	NEWS_LENGTH = NEWS_SIZE-1,
};

struct CMapVersion
{
	char m_aName[8];
	unsigned char m_aCrc[4];
	unsigned char m_aSize[4];
};

#endif
