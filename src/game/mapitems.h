/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_MAPITEMS_H
#define GAME_MAPITEMS_H

#include <engine/shared/protocol.h>

// for compatibility with BW mod
#define EXTRATILE_DATA 20
#define EXTRA_VERSION 0

// layer types
enum
{
	// TODO(Shereef Marzouk): fix this for vanilla, make use of LAYERTYPE_GAME instead of using m_game variable in the editor.
	LAYERTYPE_INVALID=0,
	LAYERTYPE_GAME,
	LAYERTYPE_TILES,
	LAYERTYPE_QUADS,
	LAYERTYPE_FRONT, // unused
	LAYERTYPE_EXTRAS = LAYERTYPE_FRONT, // BW mod
	LAYERTYPE_TELE, // unused
	LAYERTYPE_SPEEDUP, // unused
	LAYERTYPE_SWITCH, // unused
	LAYERTYPE_TUNE, // unused
	LAYERTYPE_SOUNDS_DEPRECATED, // deprecated! do not use this, this is just for compatibility reasons
	LAYERTYPE_SOUNDS,

	MAPITEMTYPE_VERSION=0,
	MAPITEMTYPE_INFO,
	MAPITEMTYPE_IMAGE,
	MAPITEMTYPE_ENVELOPE,
	MAPITEMTYPE_GROUP,
	MAPITEMTYPE_LAYER,
	MAPITEMTYPE_ENVPOINTS,
	MAPITEMTYPE_SOUND,


	CURVETYPE_STEP=0,
	CURVETYPE_LINEAR,
	CURVETYPE_SLOW,
	CURVETYPE_FAST,
	CURVETYPE_SMOOTH,
	NUM_CURVETYPES,

	// game layer tiles
	// TODO define which Layer uses which tiles (needed for mapeditor)
	ENTITY_NULL=0,
	ENTITY_SPAWN,
	ENTITY_SPAWN_RED,
	ENTITY_SPAWN_BLUE,
	ENTITY_FLAGSTAND_RED,
	ENTITY_FLAGSTAND_BLUE,
	ENTITY_ARMOR_1,
	ENTITY_HEALTH_1,
	ENTITY_WEAPON_SHOTGUN,
	ENTITY_WEAPON_GRENADE,
	ENTITY_POWERUP_NINJA,
	ENTITY_WEAPON_RIFLE,
	//DDRace - Main Lasers
	ENTITY_LASER_FAST_CW,
	ENTITY_LASER_NORMAL_CW,
	ENTITY_LASER_SLOW_CW,
	ENTITY_LASER_STOP,
	ENTITY_LASER_SLOW_CCW,
	ENTITY_LASER_NORMAL_CCW,
	ENTITY_LASER_FAST_CCW,
	//DDRace - Laser Modifiers
	ENTITY_LASER_SHORT,
	ENTITY_LASER_MEDIUM,
	ENTITY_LASER_LONG,
	ENTITY_LASER_C_SLOW,
	ENTITY_LASER_C_NORMAL,
	ENTITY_LASER_C_FAST,
	ENTITY_LASER_O_SLOW,
	ENTITY_LASER_O_NORMAL,
	ENTITY_LASER_O_FAST,
	//DDRace - Plasma
	ENTITY_PLASMAE=29,
	ENTITY_PLASMAF,
	ENTITY_PLASMA,
	ENTITY_PLASMAU,
	//DDRace - Shotgun
	ENTITY_CRAZY_SHOTGUN_EX,
	ENTITY_CRAZY_SHOTGUN,
	//DDRace - Draggers
	ENTITY_DRAGGER_WEAK=42,
	ENTITY_DRAGGER_NORMAL,
	ENTITY_DRAGGER_STRONG,
	//Draggers Behind Walls
	ENTITY_DRAGGER_WEAK_NW,
	ENTITY_DRAGGER_NORMAL_NW,
	ENTITY_DRAGGER_STRONG_NW,
	//Doors
	ENTITY_DOOR=49,
	//End Of Lower Tiles
	NUM_ENTITIES,
	//Start From Top Left
	//Tile Controllers
	TILE_AIR=0,
	TILE_SOLID,
	TILE_DEATH,
	TILE_NOHOOK,
	TILE_NOLASER,
	TILE_THROUGH_CUT,
	TILE_THROUGH,
	TILE_JUMP,
	TILE_FREEZE = 9,
	TILE_TELEINEVIL,
	TILE_UNFREEZE,
	TILE_DFREEZE,
	TILE_DUNFREEZE,
	TILE_TELEINWEAPON,
	TILE_TELEINHOOK,
	TILE_WALLJUMP = 16,
	TILE_EHOOK_START,
	TILE_EHOOK_END,
	TILE_HIT_START,
	TILE_HIT_END,
	TILE_SOLO_START,
	TILE_SOLO_END,
	//Switches
	TILE_SWITCHTIMEDOPEN = 22,
	TILE_SWITCHTIMEDCLOSE,
	TILE_SWITCHOPEN,
	TILE_SWITCHCLOSE,
	TILE_TELEIN,
	TILE_TELEOUT,
	TILE_BOOST,
	TILE_TELECHECK,
	TILE_TELECHECKOUT,
	TILE_TELECHECKIN,
	TILE_REFILL_JUMPS = 32,
	TILE_BEGIN,
	TILE_END,
	TILE_STOP = 60,
	TILE_STOPS,
	TILE_STOPA,
	TILE_TELECHECKINEVIL,
	TILE_CP,
	TILE_CP_F,
	TILE_THROUGH_ALL,
	TILE_THROUGH_DIR,
	TILE_TUNE1,
	TILE_OLDLASER = 71,
	TILE_NPC,
	TILE_EHOOK,
	TILE_NOHIT,
	TILE_NPH,
	TILE_UNLOCK_TEAM,
	TILE_PENALTY = 79,
	TILE_NPC_END = 88,
	TILE_SUPER_END,
	TILE_JETPACK_END,
	TILE_NPH_END,
	TILE_BONUS = 95,
	TILE_NPC_START = 104,
	TILE_SUPER_START,
	TILE_JETPACK_START,
	TILE_NPH_START,
	TILE_ENTITIES_OFF_1 = 190,
	TILE_ENTITIES_OFF_2,

	// legacy race/custom mod support
//	TILE_AIR = 0,
//	TILE_SOLID, 1
//	TILE_DEATH, 2
//	TILE_NOHOOK, 3
//	TILE_FREEZE, 4
//	TILE_UNFREEZE, 5
	TILE_FREEZE_DEEP=6,
	TILE_UNFREEZE_DEEP,
	TILE_RACE_START,
	TILE_RACE_FINISH,
	TILE_RESTART,
	TILE_ONEWAY_RIGHT,
	TILE_ONEWAY_UP,
	TILE_ONEWAY_LEFT,
	TILE_ONEWAY_DOWN,
	TILE_VIP,
	TILE_BESTCLAN,
	TILE_BARRIER_ENTITIES,
	TILE_ENDLESSHOOK,
	TILE_SUPERHAMMER,
	TILE_GIVEPASSIVE,
	TILE_BARRIER_LEVEL_1,
	TILE_BARRIER_LEVEL_50,
	TILE_BARRIER_LEVEL_100,
	TILE_BARRIER_LEVEL_200,
	TILE_BARRIER_LEVEL_300,
	TILE_BARRIER_LEVEL_400,
	TILE_BARRIER_LEVEL_500,
	TILE_BARRIER_LEVEL_600,
	TILE_BARRIER_LEVEL_700,
	TILE_BARRIER_LEVEL_800,
	TILE_BARRIER_LEVEL_900,
	TILE_BARRIER_LEVEL_999,
	TILE_EXTRAJUMP,
	NUM_TILES,

	EXTRAS_TELEPORT_FROM=1,
	EXTRAS_TELEPORT_TO,
	EXTRAS_SPEEDUP,
	EXTRAS_FREEZE,
	EXTRAS_UNFREEZE,
	EXTRAS_DOOR,
	EXTRAS_DOOR_HANDLE,
	EXTRAS_BUF1,//replace
	EXTRAS_ZONE_PROTECTION,
	EXTRAS_ZONE_SPAWN,
	EXTRAS_ZONE_UNTOUCHABLE,
	EXTRAS_MAP,
	EXTRAS_SELL_SKINMANI,
	EXTRAS_SELL_GUNDESIGN,
	EXTRAS_SELL_KNOCKOUT,
	EXTRAS_SELL_EXTRAS,
	EXTRAS_PLAYERCOUNT,
	EXTRAS_HOOKTHROUGH,
	EXTRAS_HOOKTHROUGH_TOP,
	EXTRAS_HOOKTHROUGH_BOTTOM,
	EXTRAS_HOOKTHROUGH_LEFT,
	EXTRAS_HOOKTHROUGH_RIGHT,
	EXTRAS_BUF0,//replace
	EXTRAS_INFO_LEVEL,
	EXTRAS_LASERGUN,
	EXTRAS_LASERGUN_TRIGGER,
	NUM_EXTRAS,


	//End of higher tiles
	//Layers
	LAYER_GAME=0,
	LAYER_FRONT,
	LAYER_TELE,
	LAYER_SPEEDUP,
	LAYER_SWITCH,
	LAYER_TUNE,
	NUM_LAYERS,
	//Flags
	TILEFLAG_VFLIP=1,
	TILEFLAG_HFLIP=2,
	TILEFLAG_OPAQUE=4,
	TILEFLAG_ROTATE=8,
	//Rotation
	ROTATION_0 = 0,
	ROTATION_90 = TILEFLAG_ROTATE,
	ROTATION_180 = (TILEFLAG_VFLIP|TILEFLAG_HFLIP),
	ROTATION_270 = (TILEFLAG_VFLIP|TILEFLAG_HFLIP|TILEFLAG_ROTATE),

	LAYERFLAG_DETAIL=1,
	TILESLAYERFLAG_GAME=1,
	TILESLAYERFLAG_TELE=2,
	TILESLAYERFLAG_SPEEDUP=4,
	TILESLAYERFLAG_FRONT=8,
	TILESLAYERFLAG_SWITCH=16,
	TILESLAYERFLAG_TUNE=32,

	ENTITY_OFFSET=255-16*4,
};

struct CPoint
{
	int x, y; // 22.10 fixed point
};

struct CColor
{
	int r, g, b, a;
};

struct CQuad
{
	CPoint m_aPoints[5];
	CColor m_aColors[4];
	CPoint m_aTexcoords[4];

	int m_PosEnv;
	int m_PosEnvOffset;

	int m_ColorEnv;
	int m_ColorEnvOffset;
};

class CTile
{
public:
	unsigned char m_Index;
	unsigned char m_Flags;
	unsigned char m_Skip;
	unsigned char m_Reserved;
};

class CExtrasData
{
public:
	char m_aData[EXTRATILE_DATA];
};

struct CMapItemInfo
{
	int m_Version;
	int m_Author;
	int m_MapVersion;
	int m_Credits;
	int m_License;
} ;

struct CMapItemInfoSettings : CMapItemInfo
{
	int m_Settings;
} ;

struct CMapItemImage
{
	int m_Version;
	int m_Width;
	int m_Height;
	int m_External;
	int m_ImageName;
	int m_ImageData;
} ;

struct CMapItemGroup_v1
{
	int m_Version;
	int m_OffsetX;
	int m_OffsetY;
	int m_ParallaxX;
	int m_ParallaxY;

	int m_StartLayer;
	int m_NumLayers;
} ;


struct CMapItemGroup : public CMapItemGroup_v1
{
	enum { CURRENT_VERSION=3 };

	int m_UseClipping;
	int m_ClipX;
	int m_ClipY;
	int m_ClipW;
	int m_ClipH;

	int m_aName[3];
} ;

struct CMapItemLayer
{
	int m_Version;
	int m_Type;
	int m_Flags;
} ;

struct CMapItemLayerTilemap
{
	CMapItemLayer m_Layer;
	int m_Version;

	int m_Width;
	int m_Height;
	int m_Flags;

	CColor m_Color;
	int m_ColorEnv;
	int m_ColorEnvOffset;

	int m_Image;
	int m_Data;

	int m_aName[3];

	// DDRace

	int m_Tele;
	int m_Speedup;
	int m_Front;
	int m_Switch;
	int m_Tune;
} ;

struct CMapItemLayerTilemapBW
{
	CMapItemLayer m_Layer;
	int m_Version;

	int m_Width;
	int m_Height;
	int m_Flags;

	CColor m_Color;
	int m_ColorEnv;
	int m_ColorEnvOffset;

	int m_Image;
	int m_Data;

	int m_aName[3];

	int m_ExtrasData;
	unsigned char m_ExtraVersion;
} ;

struct CMapItemLayerQuads
{
	CMapItemLayer m_Layer;
	int m_Version;

	int m_NumQuads;
	int m_Data;
	int m_Image;

	int m_aName[3];
} ;

struct CMapItemVersion
{
	int m_Version;
} ;

struct CEnvPoint
{
	int m_Time; // in ms
	int m_Curvetype;
	int m_aValues[4]; // 1-4 depending on envelope (22.10 fixed point)

	bool operator<(const CEnvPoint &Other) { return m_Time < Other.m_Time; }
} ;

struct CMapItemEnvelope_v1
{
	int m_Version;
	int m_Channels;
	int m_StartPoint;
	int m_NumPoints;
	int m_aName[8];
} ;

struct CMapItemEnvelope : public CMapItemEnvelope_v1
{
	enum { CURRENT_VERSION=2 };
	int m_Synchronized;
};

struct CSoundShape
{
	enum
	{
		SHAPE_RECTANGLE = 0,
		SHAPE_CIRCLE,
		NUM_SHAPES,
	};

	struct CRectangle
	{
		int m_Width, m_Height; // fxp 22.10
	};

	struct CCircle
	{
		int m_Radius;
	};

	int m_Type;

	union
	{
		CRectangle m_Rectangle;
		CCircle m_Circle;
	};
};

struct CSoundSource
{
	CPoint m_Position;
	int m_Loop;
	int m_Pan; // 0 - no panning, 1 - panning
	int m_TimeDelay; // in s
	int m_Falloff; // [0,255] // 0 - No falloff, 255 - full

	int m_PosEnv;
	int m_PosEnvOffset;
	int m_SoundEnv;
	int m_SoundEnvOffset;

	CSoundShape m_Shape;
};

struct CMapItemLayerSounds
{
	enum { CURRENT_VERSION=2 };

	CMapItemLayer m_Layer;
	int m_Version;

	int m_NumSources;
	int m_Data;
	int m_Sound;

	int m_aName[3];
};

struct CMapItemSound
{
	int m_Version;

	int m_External;

	int m_SoundName;
	int m_SoundData;
	int m_SoundDataSize;
} ;


// DDRace

class CTeleTile
{
public:
	unsigned char m_Number;
	unsigned char m_Type;
};

class CSpeedupTile
{
public:
	unsigned char m_Force;
	unsigned char m_MaxSpeed;
	unsigned char m_Type;
	short m_Angle;
};

class CSwitchTile
{
public:
	unsigned char m_Number;
	unsigned char m_Type;
	unsigned char m_Flags;
	unsigned char m_Delay;
};

class CDoorTile
{
public:
	unsigned char m_Index;
	unsigned char m_Flags;
	int m_Number;
};

class CTuneTile
{
public:
	unsigned char m_Number;
	unsigned char m_Type;
};


bool IsValidGameTile(int Index);
bool IsValidFrontTile(int Index);
bool IsValidTeleTile(int Index);
bool IsValidSpeedupTile(int Index);
bool IsValidSwitchTile(int Index);
bool IsValidEntity(int Index);

#endif
