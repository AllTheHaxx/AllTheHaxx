/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_VERSION_H
#define GAME_VERSION_H
#include "generated/nethash.cpp"
#define GAME_VERSION "0.6.3"
#define BUILD_DATE __DATE__ ", " __TIME__

#ifdef GIT_SHORTREV_HASH
	#define ALLTHEHAXX_VERSION "beta-13.1 (v0.9-3.1) #" GIT_SHORTREV_HASH
#else
	#define ALLTHEHAXX_VERSION "beta-13.1 (v0.9-3.1) custom"
#endif

#define GAME_NETVERSION "0.6 626fce9a778df4d4"
static const char GAME_ATH_VERSION[10] = "0.13.1"; // for updater

// do not modify these, they belong to ddnet
static const char GAME_RELEASE_VERSION[8] = "10.0.1";
#define CLIENT_VERSIONNR 1000

#endif
