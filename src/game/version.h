/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_VERSION_H
#define GAME_VERSION_H
#include "generated/nethash.cpp"

#define GAME_VERSION "0.6.4"
#define ATH_VERSION "0.37"
#define DDNET_VERSION "10.7.1+"
#define BUILD_DATE __DATE__ ", " __TIME__

#define ALLTHEHAXX_VERSIONID ATH_VERSION
#if defined(GIT_SHORTREV_HASH)
	#define ALLTHEHAXX_VERSION ALLTHEHAXX_VERSIONID " #" GIT_SHORTREV_HASH
#else
	#define ALLTHEHAXX_VERSION ALLTHEHAXX_VERSIONID " custom"
#endif

#define GAME_NETVERSION "0.6 626fce9a778df4d4"

// for updater
static const char GAME_ATH_VERSION[10] = ATH_VERSION;
#define GAME_ATH_VERSION_NUMERIC 3700

// do not modify these, they belong to ddnet
static const char GAME_RELEASE_VERSION[8] = DDNET_VERSION;
#define CLIENT_VERSIONNR 10073

#endif
