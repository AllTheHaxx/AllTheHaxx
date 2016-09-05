/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_VERSION_H
#define GAME_VERSION_H
#include "generated/nethash.cpp"

#define GAME_VERSION "0.6.3"
#define ATH_VERSION "0.25.1"
#define DDNET_VERSION "10.3.1"
#define BUILD_DATE __DATE__ ", " __TIME__

#ifdef GIT_SHORTREV_HASH
	#define ALLTHEHAXX_VERSION ATH_VERSION " #" GIT_SHORTREV_HASH
#else
	#define ALLTHEHAXX_VERSION ATH_VERSION " custom"
#endif

#define GAME_NETVERSION "0.6 626fce9a778df4d4"

// for updater
static const char GAME_ATH_VERSION[10] = ATH_VERSION;
#define GAME_ATH_VERSION_NUMERIC 2510

// do not modify these, they belong to ddnet
static const char GAME_RELEASE_VERSION[8] = DDNET_VERSION;
#define CLIENT_VERSIONNR 1003

#endif
