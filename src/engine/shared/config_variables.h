/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_SHARED_CONFIG_VARIABLES_H
#define ENGINE_SHARED_CONFIG_VARIABLES_H
#undef ENGINE_SHARED_CONFIG_VARIABLES_H // this file will be included several times

// TODO: remove this
#include "././game/variables.h"

#ifndef MACRO_CONFIG_INT
#define MACRO_CONFIG_INT(Name,ScriptName,Def,Min,Max,Save,Desc) ;
#define undef1
#endif
#ifndef MACRO_CONFIG_STR
#define MACRO_CONFIG_STR(Name,ScriptName,Len,Def,Save,Desc) ;
#define undef2
#endif


MACRO_CONFIG_STR(PlayerName, player_name, 16, "haxxless tee", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Name of the player")
MACRO_CONFIG_STR(PlayerClan, player_clan, 12, "AllTheHaxx", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Clan of the player")
MACRO_CONFIG_INT(PlayerCountry, player_country, -1, -1, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Country of the player")
MACRO_CONFIG_STR(Password, password, 32, "", CFGFLAG_CLIENT|CFGFLAG_SERVER, "Password to the server")
MACRO_CONFIG_STR(Logfile, logfile, 128, "", CFGFLAG_SAVE|CFGFLAG_CLIENT|CFGFLAG_SERVER, "Filename to log all output to")
MACRO_CONFIG_INT(ConsoleOutputLevel, console_output_level, 0, 0, 2, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Adjusts the amount of information in the console")

MACRO_CONFIG_INT(ClSaveSettings, cl_save_settings, 1, 0, 1, CFGFLAG_CLIENT, "Write the settings file on exit")
MACRO_CONFIG_INT(ClCpuThrottle, cl_cpu_throttle, 1, 0, 100, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Makes the client use less CPU, too high values result in stuttering")
MACRO_CONFIG_INT(ClCpuThrottleInactive, cl_cpu_throttle_inactive, 5, 0, 100, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(ClEditor, cl_editor, 0, 0, 1, CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(ClEditorUndo, cl_editorundo, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Undo function in editor")
MACRO_CONFIG_INT(ClEditorLazyInit, cl_editor_lazy_init, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Delay the editor init after client startup to speed up loading")
MACRO_CONFIG_INT(ClLoadCountryFlags, cl_load_country_flags, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Load and show country flags")
MACRO_CONFIG_STR(ClSkinFilterString, cl_skin_filter_string, 25, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Skin filtering string")
MACRO_CONFIG_INT(ClSkinFilterAdvanced, cl_skin_filter_advanced, 0, 0, 2, CFGFLAG_SAVE|CFGFLAG_CLIENT, "0: Show all | 1: Show Vanilla only | 3: Show non-vanilla only")
MACRO_CONFIG_STR(ClLuaFilterString, cl_lua_filter_string, 32, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Lua filtering string")

MACRO_CONFIG_INT(ClAutoDemoRecord, cl_auto_demo_record, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Automatically record demos")
MACRO_CONFIG_INT(ClAutoDemoMax, cl_auto_demo_max, 10, 0, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Maximum number of automatically recorded demos (0 = no limit)")
MACRO_CONFIG_INT(ClAutoScreenshot, cl_auto_screenshot, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Automatically take game over screenshot")
MACRO_CONFIG_INT(ClAutoScreenshotMax, cl_auto_screenshot_max, 10, 0, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Maximum number of automatically created screenshots (0 = no limit)")
MACRO_CONFIG_INT(ClResetWantedWeaponOnDeath, cl_reset_wanted_weapon_on_death, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Reset wanted weapon on death")
MACRO_CONFIG_INT(ClShowBroadcasts, cl_show_broadcasts, 1, 0, 1, CFGFLAG_CLIENT, "Show broadcasts ingame")
MACRO_CONFIG_INT(ClPrintBroadcasts, cl_print_broadcasts, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Print broadcasts to console")
MACRO_CONFIG_INT(ClPrintMotd, cl_print_motd, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Print motd to console")
MACRO_CONFIG_INT(ClFriendsIgnoreClan, cl_friends_ignore_clan, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Ignore clan tag when searching for friends")

MACRO_CONFIG_INT(ClSimpleLoadingScreen, cl_simple_loading_screen, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Use a static loading screen (can speed up client start a lot on certain machines)")
MACRO_CONFIG_INT(ClEventthread, cl_eventthread, 0, 0, 1, CFGFLAG_CLIENT, "Enables the usage of a thread to pump the events")

#if !defined(CONF_PLATFORM_MACOSX)
MACRO_CONFIG_INT(InpGrab, inp_grab, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Use forceful input grabbing method")
#else
MACRO_CONFIG_INT(InpGrab, inp_grab, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Use forceful input grabbing method")
#endif

MACRO_CONFIG_STR(BrFilterString, br_filter_string, 25, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Server browser filtering string")
MACRO_CONFIG_STR(BrExcludeString, br_exclude_string, 25, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Server browser exclusion string")
MACRO_CONFIG_INT(BrFilterFull, br_filter_full, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out full server in browser")
MACRO_CONFIG_INT(BrFilterEmpty, br_filter_empty, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out empty server in browser")
MACRO_CONFIG_INT(BrFilterNonEmpty, br_filter_nonempty, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out non-empty server in browser")
MACRO_CONFIG_INT(BrFilterSpectators, br_filter_spectators, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out spectators from player numbers")
MACRO_CONFIG_INT(BrFilterFriends, br_filter_friends, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out servers with no friends")
MACRO_CONFIG_INT(BrFilterCountry, br_filter_country, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out servers with non-matching player country")
MACRO_CONFIG_INT(BrFilterCountryIndex, br_filter_country_index, -1, -1, 999, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Player country to filter by in the server browser")
MACRO_CONFIG_INT(BrFilterPw, br_filter_pw, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out password protected servers in browser")
MACRO_CONFIG_INT(BrFilterPing, br_filter_ping, 999, 0, 999, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Ping to filter by in the server browser")
MACRO_CONFIG_STR(BrFilterGametype, br_filter_gametype, 128, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Game types to filter")
MACRO_CONFIG_STR(BrFilterVersion, br_filter_version, 128, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Game version to filter")
MACRO_CONFIG_INT(BrFilterGametypeStrict, br_filter_gametype_strict, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Strict gametype filter")
MACRO_CONFIG_INT(BrFilterVersionStrict, br_filter_version_strict, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Strict version filter")
//MACRO_CONFIG_INT(BrFilterDDRaceNetwork, br_filter_ddracenetwork, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Don't show official DDRaceNetwork servers in this list (use the DDNet browser instead)")
MACRO_CONFIG_STR(BrFilterServerAddress, br_filter_serveraddress, 128, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Server address to filter")
MACRO_CONFIG_INT(BrFilterPure, br_filter_pure, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out non-standard servers in browser")
MACRO_CONFIG_INT(BrFilterPureMap, br_filter_pure_map, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out non-standard maps in browser")
MACRO_CONFIG_INT(BrFilterCompatversion, br_filter_compatversion, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out non-compatible servers in browser")

MACRO_CONFIG_STR(BrFilterExcludeCountries, br_filter_exclude_countries, 128, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out ddnet servers by country")
MACRO_CONFIG_STR(BrFilterExcludeTypes, br_filter_exclude_types, 128, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Filter out ddnet servers by type (mod)")

MACRO_CONFIG_INT(BrSort, br_sort, 4, 0, 256, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(BrSortOrder, br_sort_order, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(BrMaxRequests, br_max_requests, 25, 0, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Number of requests to use when refreshing server browser")

MACRO_CONFIG_INT(BrDemoSort, br_demo_sort, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(BrDemoSortOrder, br_demo_sort_order, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")

MACRO_CONFIG_INT(SndBufferSize, snd_buffer_size, 512, 128, 32768, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound buffer size")
#if defined(__ANDROID__)
MACRO_CONFIG_INT(SndRate, snd_rate, 44100, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound mixing rate")
#else
MACRO_CONFIG_INT(SndRate, snd_rate, 48000, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound mixing rate")
#endif
MACRO_CONFIG_INT(SndEnable, snd_enable, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound enable")
MACRO_CONFIG_INT(SndMusic, snd_enable_music, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Play background music")
MACRO_CONFIG_INT(SndVolume, snd_volume, 50, 0, 100, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Sound volume")
MACRO_CONFIG_INT(SndDevice, snd_device, -1, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "(deprecated) Sound device to use")
MACRO_CONFIG_INT(SndMapSoundVolume, snd_ambient_volume, 70, 0, 100, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Map Sound sound volume")

MACRO_CONFIG_INT(SndNonactiveMute, snd_nonactive_mute, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(SndGame, snd_game, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable game sounds")
MACRO_CONFIG_INT(SndHammer, snd_hammer, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable hammer sound")
MACRO_CONFIG_INT(SndGun, snd_gun, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable gun sound")
MACRO_CONFIG_INT(SndShotgun, snd_shotgun, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable shotgun sound")
MACRO_CONFIG_INT(SndGrenade, snd_grenade, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable grenade sound")
MACRO_CONFIG_INT(SndRifle, snd_rifle, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable rifle sound")
MACRO_CONFIG_INT(SndJump, snd_jump, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable jump sounds")
MACRO_CONFIG_INT(SndSpawn, snd_spawn, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable spawn sounds")
MACRO_CONFIG_INT(SndChat, snd_chat, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable regular chat sound")
MACRO_CONFIG_INT(SndTeamChat, snd_team_chat, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable team chat sound")
MACRO_CONFIG_INT(SndServerMessage, snd_servermessage, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable server message sound")
MACRO_CONFIG_INT(SndHighlight, snd_highlight, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable highlighted chat sound")

MACRO_CONFIG_INT(GfxScreen, gfx_screen, 0, 0, 15, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen index")
MACRO_CONFIG_INT(GfxScreenWidth, gfx_screen_width, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen resolution width")
MACRO_CONFIG_INT(GfxScreenHeight, gfx_screen_height, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen resolution height")
#if defined(__ANDROID__)
MACRO_CONFIG_INT(GfxBorderless, gfx_borderless, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Borderless window (not to be used with fullscreen)")
MACRO_CONFIG_INT(GfxFullscreen, gfx_fullscreen, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Fullscreen")
MACRO_CONFIG_INT(GfxAlphabits, gfx_alphabits, 1, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Alpha bits for framebuffer (fullscreen only)")
#else
MACRO_CONFIG_INT(GfxBorderless, gfx_borderless, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Borderless window (not to be used with fullscreen)")
MACRO_CONFIG_INT(GfxFullscreen, gfx_fullscreen, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Fullscreen")
MACRO_CONFIG_INT(GfxAlphabits, gfx_alphabits, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Alpha bits for framebuffer (fullscreen only)")
#endif
MACRO_CONFIG_INT(GfxColorDepth, gfx_color_depth, 24, 16, 24, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Colors bits for framebuffer (fullscreen only)")
//MACRO_CONFIG_INT(GfxClear, gfx_clear, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Clear screen before rendering")
MACRO_CONFIG_INT(GfxVsync, gfx_vsync, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Vertical sync")
MACRO_CONFIG_INT(GfxResizable, gfx_resizable, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enables window resizing")
MACRO_CONFIG_INT(GfxDisplayAllModes, gfx_display_all_modes, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(GfxTextureCompression, gfx_texture_compression, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Use texture compression")
//MACRO_CONFIG_INT(GfxTextureCaching, gfx_texture_caching, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable texture caching; uses more RAM but reduces some loading times")
#if defined(__ANDROID__)
MACRO_CONFIG_INT(GfxHighDetail, gfx_high_detail, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "High detail")
MACRO_CONFIG_INT(GfxLowGraphics, gfx_low_graphics, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Disables various graphic effects")
MACRO_CONFIG_INT(GfxTextureQuality, gfx_texture_quality, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
#else
MACRO_CONFIG_INT(GfxHighDetail, gfx_high_detail, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "High detail")
MACRO_CONFIG_INT(GfxLowGraphics, gfx_low_graphics, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Disables various graphic effects")
MACRO_CONFIG_INT(GfxTextureQuality, gfx_texture_quality, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
#endif
MACRO_CONFIG_INT(GfxFsaaSamples, gfx_fsaa_samples, 0, 0, 16, CFGFLAG_SAVE|CFGFLAG_CLIENT, "FSAA Samples")
MACRO_CONFIG_INT(GfxRefreshRate, gfx_refresh_rate, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Screen refresh rate")
MACRO_CONFIG_INT(GfxFinish, gfx_finish, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(GfxBackgroundRender, gfx_backgroundrender, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Render graphics when window is in background")
MACRO_CONFIG_INT(GfxTextOverlay, gfx_text_overlay, 10, 1, 100, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Stop rendering textoverlay in editor or with entities: high value = less details = more speed")
#if defined(__ANDROID__)
MACRO_CONFIG_INT(GfxAsyncRenderOld, gfx_asyncrender_old, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Do rendering async from the the update")
#else
MACRO_CONFIG_INT(GfxAsyncRenderOld, gfx_asyncrender_old, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Do rendering async from the the update")
#endif
MACRO_CONFIG_INT(GfxTuneOverlay, gfx_tune_overlay, 20, 1, 100, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Stop rendering text overlay in tuning zone in editor: high value = less details = more speed")
#if defined(__ANDROID__)
MACRO_CONFIG_INT(GfxQuadAsTriangle, gfx_quad_as_triangle, 0, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Render quads as triangles (fixes quad coloring on some GPUs)")
#else
MACRO_CONFIG_INT(GfxQuadAsTriangle, gfx_quad_as_triangle, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Render quads as triangles (fixes quad coloring on some GPUs)")
#endif
MACRO_CONFIG_INT(GfxHighdpi, gfx_highdpi, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Try to use high-dpi screen features")
MACRO_CONFIG_INT(GfxLaserTrail, gfx_lasertrail, 1, 0, 3, CFGFLAG_SAVE|CFGFLAG_CLIENT, "0: off | 1: vanilla only | 2: not on race servers | 3: everywhere")

MACRO_CONFIG_INT(InpMousesens, inp_mousesens, 130, 5, 100000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Mouse sensitivity")
MACRO_CONFIG_INT(InpMouseOld, inp_mouseold, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Use old mouse mode (warp mouse instead of raw input) -- MIGHT BE BUGGY ON SOME SYSTEMS!!")
MACRO_CONFIG_INT(InpIgnoredModifiers, inp_ignored_modifiers, 0, 0, 65536, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Ignored keyboard modifier mask")

#if defined(CONF_DEBUG)
MACRO_CONFIG_INT(ClMemcheckInterval, cl_memcheck_interval, 60, 0, 6000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Interval for validating the heap")
#endif

MACRO_CONFIG_STR(SvName, sv_name, 128, "unnamed server", CFGFLAG_SERVER, "Server name")
MACRO_CONFIG_STR(Bindaddr, bindaddr, 128, "", CFGFLAG_CLIENT|CFGFLAG_SERVER|CFGFLAG_MASTER, "Address to bind the client/server to")
MACRO_CONFIG_INT(SvPort, sv_port, 8303, 0, 0, CFGFLAG_SERVER, "Port to use for the server (Only ports 8303-8310 work in LAN server browser)")
MACRO_CONFIG_INT(SvExternalPort, sv_external_port, 0, 0, 0, CFGFLAG_SERVER, "External port to report to the master servers")
MACRO_CONFIG_STR(SvMap, sv_map, 128, "Kobra 4", CFGFLAG_SERVER, "Map to use on the server")
MACRO_CONFIG_INT(SvMaxClients, sv_max_clients, MAX_CLIENTS, 1, MAX_CLIENTS, CFGFLAG_SERVER, "Maximum number of clients that are allowed on a server")
MACRO_CONFIG_INT(SvMaxClientsPerIP, sv_max_clients_per_ip, 4, 1, MAX_CLIENTS, CFGFLAG_SERVER, "Maximum number of clients with the same IP that can connect to the server")
MACRO_CONFIG_INT(SvHighBandwidth, sv_high_bandwidth, 0, 0, 1, CFGFLAG_SERVER, "Use high bandwidth mode. Doubles the bandwidth required for the server. LAN use only")
MACRO_CONFIG_INT(SvRegister, sv_register, 1, 0, 1, CFGFLAG_SERVER, "Register server with master server for public listing")
MACRO_CONFIG_STR(SvRconPassword, sv_rcon_password, 32, "", CFGFLAG_SERVER, "Remote console password (full access)")
MACRO_CONFIG_STR(SvRconModPassword, sv_rcon_mod_password, 32, "", CFGFLAG_SERVER, "Remote console password for moderators (limited access)")
MACRO_CONFIG_STR(SvRconHelperPassword, sv_rcon_helper_password, 32, "", CFGFLAG_SERVER, "Remote console password for helpers (limited access)")
MACRO_CONFIG_INT(SvRconMaxTries, sv_rcon_max_tries, 30, 0, 100, CFGFLAG_SERVER, "Maximum number of tries for remote console authentication")
MACRO_CONFIG_INT(SvRconBantime, sv_rcon_bantime, 5, 0, 1440, CFGFLAG_SERVER, "The time a client gets banned if remote console authentication fails. 0 makes it just use kick")
MACRO_CONFIG_INT(SvAutoDemoRecord, sv_auto_demo_record, 0, 0, 1, CFGFLAG_SERVER, "Automatically record demos")
MACRO_CONFIG_INT(SvAutoDemoMax, sv_auto_demo_max, 10, 0, 1000, CFGFLAG_SERVER, "Maximum number of automatically recorded demos (0 = no limit)")
MACRO_CONFIG_INT(SvVanillaAntiSpoof, sv_vanilla_antispoof, 1, 0, 1, CFGFLAG_SERVER, "Enable vanilla Antispoof")
MACRO_CONFIG_INT(SvDnsbl, sv_dnsbl, 0, 0, 1, CFGFLAG_SERVER, "Enable DNSBL (DNS-based Blackhole List)")
MACRO_CONFIG_STR(SvDnsblHost, sv_dnsbl_host, 128, "", CFGFLAG_SERVER, "Hostname of DNSBL provider to use for IP Verification")
MACRO_CONFIG_STR(SvDnsblKey, sv_dnsbl_key, 128, "", CFGFLAG_SERVER, "Optional Authentification Key for the specified DNSBL provider")
MACRO_CONFIG_INT(SvDnsblVote, sv_dnsbl_vote, 0, 0, 1, CFGFLAG_SERVER, "Block votes by blacklisted addresses")
MACRO_CONFIG_INT(SvDnsblBan, sv_dnsbl_ban, 0, 0, 1, CFGFLAG_SERVER, "Automatically ban blacklisted addresses")


MACRO_CONFIG_INT(SvPlayerDemoRecord, sv_player_demo_record, 0, 0, 1, CFGFLAG_SERVER, "Automatically record demos for each player")
MACRO_CONFIG_INT(SvDemoChat, sv_demo_chat, 0, 0, 1, CFGFLAG_SERVER, "Record chat for demos")
MACRO_CONFIG_INT(SvServerInfoPerSecond, sv_server_info_per_second, 50, 1, 1000, CFGFLAG_SERVER, "Maximum number of complete server info responses that are sent out per second")
MACRO_CONFIG_INT(SvVanConnPerSecond, sv_van_conn_per_second, 10, 1, 1000, CFGFLAG_SERVER, "Antispoof specific ratelimit")

MACRO_CONFIG_STR(EcBindaddr, ec_bindaddr, 128, "localhost", CFGFLAG_ECON, "Address to bind the external console to. Anything but 'localhost' is dangerous")
MACRO_CONFIG_INT(EcPort, ec_port, 0, 0, 0, CFGFLAG_ECON, "Port to use for the external console")
MACRO_CONFIG_STR(EcPassword, ec_password, 32, "", CFGFLAG_ECON, "External console password")
MACRO_CONFIG_INT(EcBantime, ec_bantime, 0, 0, 1440, CFGFLAG_ECON, "The time a client gets banned if econ authentication fails. 0 just closes the connection")
MACRO_CONFIG_INT(EcAuthTimeout, ec_auth_timeout, 30, 1, 120, CFGFLAG_ECON, "Time in seconds before the the econ authentification times out")
MACRO_CONFIG_INT(EcOutputLevel, ec_output_level, 1, 0, 2, CFGFLAG_ECON, "Adjusts the amount of information in the external console")

MACRO_CONFIG_INT(Debug, debug, 0, 0, 3, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Debug mode")
MACRO_CONFIG_INT(DbgDirections, dbg_directions, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Debug player aiming directions")
MACRO_CONFIG_INT(DbgStress, dbg_stress, 0, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Stress systems")
MACRO_CONFIG_INT(DbgStressNetwork, dbg_stress_network, 0, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SERVER, "Stress network")
MACRO_CONFIG_INT(DbgPref, dbg_pref, 0, 0, 1, CFGFLAG_SERVER, "Performance outputs")
MACRO_CONFIG_INT(DbgGraphs, dbg_graphs, 0, 0, 1, CFGFLAG_CLIENT, "Performance graphs")
MACRO_CONFIG_INT(DbgHitch, dbg_hitch, 0, 0, 0, CFGFLAG_SERVER, "Hitch warnings")
MACRO_CONFIG_STR(DbgStressServer, dbg_stress_server, 32, "localhost", CFGFLAG_CLIENT, "Server to stress")

// DDRace
MACRO_CONFIG_STR(SvWelcome, sv_welcome, 64, "", CFGFLAG_SERVER, "Message that will be displayed to players who join the server")
MACRO_CONFIG_INT(SvReservedSlots, sv_reserved_slots, 0, 0, 16, CFGFLAG_SERVER, "The number of slots that are reserved for special players")
MACRO_CONFIG_STR(SvReservedSlotsPass, sv_reserved_slots_pass, 32, "", CFGFLAG_SERVER, "The password that is required to use a reserved slot")
MACRO_CONFIG_INT(SvHit, sv_hit, 1, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Whether players can hammer/grenade/laser eachother or not")
MACRO_CONFIG_INT(SvEndlessDrag, sv_endless_drag, 0, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Turns endless hooking on/off")
MACRO_CONFIG_INT(SvTestingCommands, sv_test_cmds, 0, 0, 1, CFGFLAG_SERVER, "Turns testing commands aka cheats on/off")
MACRO_CONFIG_INT(SvFreezeDelay, sv_freeze_delay, 3, 1, 30, CFGFLAG_SERVER|CFGFLAG_GAME, "How many seconds the players will remain frozen (applies to all except delayed freeze in switch layer & deepfreeze)")
MACRO_CONFIG_INT(ClDDRaceBinds, cl_race_binds, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Enable Default DDRace builds when pressing the reset binds button")
MACRO_CONFIG_INT(ClDDRaceBindsSet, cl_race_binds_set, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Whether the DDRace binds set or not (this is automated you don't need to use this)")
MACRO_CONFIG_INT(SvEndlessSuperHook, sv_endless_super_hook, 0, 0, 1, CFGFLAG_SERVER, "Endless hook for super players on/off")
MACRO_CONFIG_INT(SvHideScore, sv_hide_score, 0, 0, 1, CFGFLAG_SERVER, "Whether players scores will be announced or not")
MACRO_CONFIG_INT(SvSaveWorseScores, sv_save_worse_scores, 1, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Whether to save worse scores when you already have a better one")
MACRO_CONFIG_INT(SvPauseable, sv_pauseable, 1, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Whether players can pause their char or not")
MACRO_CONFIG_INT(SvPauseMessages, sv_pause_messages, 0, 0, 1, CFGFLAG_SERVER, "Whether to show messages when a player pauses and resumes")
MACRO_CONFIG_INT(SvPauseTime, sv_pause_time, 0, 0, 1, CFGFLAG_SERVER, "Whether '/pause' and 'sv_max_dc_restore' pauses the time of player or not")
MACRO_CONFIG_INT(SvPauseFrequency, sv_pause_frequency, 1, 0, 9999, CFGFLAG_SERVER, "The minimum allowed delay between pauses")

MACRO_CONFIG_INT(SvEmotionalTees, sv_emotional_tees, 1, -1, 1, CFGFLAG_SERVER, "Whether eye change of tees is enabled with emoticons = 1, not = 0, -1 not at all")
MACRO_CONFIG_INT(SvEmoticonDelay, sv_emoticon_delay, 3, 0, 9999, CFGFLAG_SERVER, "The time in seconds between over-head emoticons")
MACRO_CONFIG_INT(SvEyeEmoteChangeDelay, sv_eye_emote_change_delay, 1, 0, 9999, CFGFLAG_SERVER, "The time in seconds between eye emoticons change")


MACRO_CONFIG_INT(SvChatDelay, sv_chat_delay, 1, 0, 9999, CFGFLAG_SERVER, "The time in seconds between chat messages")
MACRO_CONFIG_INT(SvTeamChangeDelay, sv_team_change_delay, 3, 0, 9999, CFGFLAG_SERVER, "The time in seconds between team changes (spectator/in game)")
MACRO_CONFIG_INT(SvInfoChangeDelay, sv_info_change_delay, 5, 0, 9999, CFGFLAG_SERVER, "The time in seconds between info changes (name/skin/color), to avoid ranbow mod set this to a very high time")
MACRO_CONFIG_INT(SvVoteTime, sv_vote_time, 25, 1, 9999, CFGFLAG_SERVER, "The time in seconds a vote lasts")
MACRO_CONFIG_INT(SvVoteMapTimeDelay, sv_vote_map_delay,0,0,9999,CFGFLAG_SERVER, "The minimum time in seconds between map votes")
MACRO_CONFIG_INT(SvVoteDelay, sv_vote_delay, 3, 0, 9999, CFGFLAG_SERVER, "The time in seconds between any vote")
MACRO_CONFIG_INT(SvVoteKickTimeDelay, sv_vote_kick_delay, 0, 0, 9999, CFGFLAG_SERVER, "The minimum time in seconds between kick votes")
MACRO_CONFIG_INT(SvVoteYesPercentage, sv_vote_yes_percentage, 50, 1, 100, CFGFLAG_SERVER, "The percent of people that need to agree or deny for the vote to succeed/fail")
MACRO_CONFIG_INT(SvVoteMajority, sv_vote_majority, 0, 0, 1, CFGFLAG_SERVER, "Whether No. of Yes is compared to No. of No votes or to number of total Players ( Default is 0 Y compare N)")
MACRO_CONFIG_INT(SvVoteMaxTotal, sv_vote_max_total, 0, 0, MAX_CLIENTS, CFGFLAG_SERVER, "How many people can participate in a vote at max (0 = no limit by default)")
MACRO_CONFIG_INT(SvVoteVetoTime, sv_vote_veto_time, 20, 0, 1000, CFGFLAG_SERVER, "Minutes of time on a server until a player can veto map change votes (0 = disabled)")
MACRO_CONFIG_INT(SvSpectatorVotes, sv_spectator_votes, 1, 0, 1, CFGFLAG_SERVER, "Choose if spectators are allowed to start votes")
MACRO_CONFIG_INT(SvKillDelay, sv_kill_delay,3,0,9999,CFGFLAG_SERVER, "The minimum time in seconds between kills")
MACRO_CONFIG_INT(SvSuicidePenalty, sv_suicide_penalty,0,0,9999,CFGFLAG_SERVER, "The minimum time in seconds between kill or /kills and respawn")

MACRO_CONFIG_INT(SvMapWindow, sv_map_window, 15, 0, 100, CFGFLAG_SERVER, "Map downloading send-ahead window")
MACRO_CONFIG_INT(SvFastDownload, sv_fast_download, 1, 0, 1, CFGFLAG_SERVER, "Enables fast download of maps")

MACRO_CONFIG_INT(SvShotgunBulletSound, sv_shotgun_bullet_sound, 0, 0, 1, CFGFLAG_SERVER, "Crazy shotgun bullet sound on/off")

MACRO_CONFIG_INT(SvCheckpointSave, sv_checkpoint_save, 1, 0, 1, CFGFLAG_SERVER, "Whether to save checkpoint times to the score file")
MACRO_CONFIG_STR(SvScoreFolder, sv_score_folder, 32, "records", CFGFLAG_SERVER, "Folder to save score files to")

#if defined(CONF_SQL)
MACRO_CONFIG_INT(SvUseSQL, sv_use_sql, 0, 0, 1, CFGFLAG_SERVER, "Enables SQL DB instead of record file")
MACRO_CONFIG_STR(SvSqlServerName, sv_sql_servername, 5, "UNK", CFGFLAG_SERVER, "SQL Server name that is inserted into record table")
MACRO_CONFIG_INT(SvSaveGames, sv_savegames, 1, 0, 1, CFGFLAG_SERVER, "Enables savegames (/save and /load)")
MACRO_CONFIG_INT(SvSaveGamesDelay, sv_savegames_delay, 60, 0, 10000, CFGFLAG_SERVER, "Delay in seconds for loading a savegame")

MACRO_CONFIG_STR(SvSqlFailureFile, sv_sql_failure_file, 64, "failed_sql.sql", CFGFLAG_SERVER, "File to store failed Sql-Inserts (ranks)")
MACRO_CONFIG_INT(SvSqlQueriesDelay, sv_sql_queries_delay, 1, 0, 20, CFGFLAG_SERVER, "Delay in seconds between SQL queries of a single player")
#endif

MACRO_CONFIG_INT(SvDDRaceRules, sv_ddrace_rules, 1, 0, 1, CFGFLAG_SERVER, "Whether the default mod rules are displayed or not")
MACRO_CONFIG_STR(SvRulesLine1, sv_rules_line1, 128, "", CFGFLAG_SERVER, "Rules line 1")
MACRO_CONFIG_STR(SvRulesLine2, sv_rules_line2, 128, "", CFGFLAG_SERVER, "Rules line 2")
MACRO_CONFIG_STR(SvRulesLine3, sv_rules_line3, 128, "", CFGFLAG_SERVER, "Rules line 3")
MACRO_CONFIG_STR(SvRulesLine4, sv_rules_line4, 128, "", CFGFLAG_SERVER, "Rules line 4")
MACRO_CONFIG_STR(SvRulesLine5, sv_rules_line5, 128, "", CFGFLAG_SERVER, "Rules line 5")
MACRO_CONFIG_STR(SvRulesLine6, sv_rules_line6, 128, "", CFGFLAG_SERVER, "Rules line 6")
MACRO_CONFIG_STR(SvRulesLine7, sv_rules_line7, 128, "", CFGFLAG_SERVER, "Rules line 7")
MACRO_CONFIG_STR(SvRulesLine8, sv_rules_line8, 128, "", CFGFLAG_SERVER, "Rules line 8")
MACRO_CONFIG_STR(SvRulesLine9, sv_rules_line9, 128, "", CFGFLAG_SERVER, "Rules line 9")
MACRO_CONFIG_STR(SvRulesLine10, sv_rules_line10, 128, "", CFGFLAG_SERVER, "Rules line 10")

MACRO_CONFIG_INT(SvTeam, sv_team, 1, 0, 3, CFGFLAG_SERVER|CFGFLAG_GAME, "Teams configuration (0 = off, 1 = on but optional, 2 = must play only with teams, 3 = forced random team only for you)")
MACRO_CONFIG_INT(SvTeamMaxSize, sv_max_team_size, MAX_CLIENTS, 1, MAX_CLIENTS, CFGFLAG_SERVER|CFGFLAG_GAME, "Maximum team size (from 2 to 64)")
MACRO_CONFIG_INT(SvTeamLock, sv_team_lock, 1, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Enable team lock")
MACRO_CONFIG_INT(SvMapVote, sv_map_vote, 1, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Whether to allow /map")

MACRO_CONFIG_STR(SvAnnouncementFileName, sv_announcement_filename, 24, "announcement.txt", CFGFLAG_SERVER, "file which will have the announcement, each one at a line")
MACRO_CONFIG_INT(SvAnnouncementInterval, sv_announcement_interval, 300, 1, 9999, CFGFLAG_SERVER, "time(minutes) in which the announcement will be displayed from the announcement file")
MACRO_CONFIG_INT(SvAnnouncementRandom, sv_announcement_random, 1, 0, 1, CFGFLAG_SERVER, "Whether announcements are sequential or random")

MACRO_CONFIG_INT(SvOldLaser, sv_old_laser, 0, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Whether lasers can hit you if you shot them and that they pull you towards the bounce origin (0 for all new maps) or lasers can't hit you if you shot them, and they pull others towards the shooter")
MACRO_CONFIG_INT(SvSlashMe, sv_slash_me, 0, 0, 1, CFGFLAG_SERVER, "Whether /me is active on the server or not")
MACRO_CONFIG_INT(SvRejoinTeam0, sv_rejoin_team_0, 1, 0, 1, CFGFLAG_SERVER, "Make a team automatically rejoin team 0 after finish (only if not locked)")

MACRO_CONFIG_INT(ClReconnectTimeout, cl_reconnect_timeout, 5, 0, 600, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How many seconds to wait before reconnecting (after timeout, 0 for off)")
MACRO_CONFIG_INT(ClReconnectFull, cl_reconnect_full, 5, 0, 600, CFGFLAG_CLIENT | CFGFLAG_SAVE, "How many seconds to wait before reconnecting (when server is full, 0 for off)")

MACRO_CONFIG_INT(ClMessageSystemHue, cl_message_system_hue, 42, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "System message color hue")
MACRO_CONFIG_INT(ClMessageSystemSat, cl_message_system_sat, 255, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "System message color saturation")
MACRO_CONFIG_INT(ClMessageSystemLht, cl_message_system_lht, 192, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "System message color lightness")

MACRO_CONFIG_INT(ClMessageHighlightHue, cl_message_highlight_hue, 0, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Highlighted message color hue")
MACRO_CONFIG_INT(ClMessageHighlightSat, cl_message_highlight_sat, 255, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Highlighted message color saturation")
MACRO_CONFIG_INT(ClMessageHighlightLht, cl_message_highlight_lht, 192, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Highlighted message color lightness")

MACRO_CONFIG_INT(ClMessageTeamHue, cl_message_team_hue, 85, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Team message color hue")
MACRO_CONFIG_INT(ClMessageTeamSat, cl_message_team_sat, 255, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Team message color saturation")
MACRO_CONFIG_INT(ClMessageTeamLht, cl_message_team_lht, 212, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Team message color lightness")

MACRO_CONFIG_INT(ClMessageHue, cl_message_hue, 0, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Message color hue")
MACRO_CONFIG_INT(ClMessageSat, cl_message_sat, 0, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Message color saturation")
MACRO_CONFIG_INT(ClMessageLht, cl_message_lht, 255, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Message color lightness")

MACRO_CONFIG_INT(ClLaserInnerHue, cl_laser_inner_hue, 170, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Laser inner color hue")
MACRO_CONFIG_INT(ClLaserInnerSat, cl_laser_inner_sat, 255, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Laser inner color saturation")
MACRO_CONFIG_INT(ClLaserInnerLht, cl_laser_inner_lht, 191, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Laser inner color lightness")

MACRO_CONFIG_INT(ClLaserOutlineHue, cl_laser_outline_hue, 170, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Laser outline color hue")
MACRO_CONFIG_INT(ClLaserOutlineSat, cl_laser_outline_sat, 137, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Laser outline color saturation")
MACRO_CONFIG_INT(ClLaserOutlineLht, cl_laser_outline_lht, 41, 0, 255, CFGFLAG_CLIENT | CFGFLAG_SAVE, "Laser outline color lightness")

MACRO_CONFIG_INT(ConnTimeout, conn_timeout, 100, 5, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT|CFGFLAG_SERVER, "Network timeout")
MACRO_CONFIG_INT(ConnTimeoutProtection, conn_timeout_protection, 1000, 5, 10000, CFGFLAG_SAVE|CFGFLAG_SERVER, "Network timeout protection")
MACRO_CONFIG_INT(ClShowIDsScoreboard, cl_show_ids_scoreboard, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Whether to show client ids in scoreboard")
MACRO_CONFIG_INT(ClShowIDsChat, cl_show_ids_chat, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Whether to show client ids in chat")
MACRO_CONFIG_INT(ClScoreboardShowATH, cl_scoreboard_show_ath, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Whether to show ATH users in scoreboard aswell")
MACRO_CONFIG_INT(ClScoreboardOnDeath, cl_scoreboard_on_death, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Whether to show scoreboard after death or not")
MACRO_CONFIG_INT(ClAutoRaceRecord, cl_auto_race_record, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Save the best demo of each race")
MACRO_CONFIG_INT(ClDemoName, cl_demo_name, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Save the player name within the demo")
MACRO_CONFIG_INT(ClDemoAssumeRace, cl_demo_assume_race, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Assume that demos are race demos")
MACRO_CONFIG_INT(ClRaceGhost, cl_race_ghost, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Enable ghost")
MACRO_CONFIG_INT(ClRaceShowGhost, cl_race_show_ghost, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ghost")
MACRO_CONFIG_INT(ClRaceSaveGhost, cl_race_save_ghost, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Save ghost")
MACRO_CONFIG_INT(ClDDRaceScoreBoard, cl_ddrace_scoreboard, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable DDRace Scoreboard ")
MACRO_CONFIG_INT(ClShowDecisecs, cl_show_decisecs, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Show deciseconds in game time")
MACRO_CONFIG_INT(SvResetPickups, sv_reset_pickups, 0, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Whether the weapons are reset on passing the start tile or not")
MACRO_CONFIG_INT(ClShowOthers, cl_show_others, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show players in other teams")
MACRO_CONFIG_INT(ClShowOthersAlpha, cl_show_others_alpha, 40, 0, 100, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show players in other teams (alpha value, 0 invisible, 100 fully visible)")
MACRO_CONFIG_INT(ClOverlayEntities, cl_overlay_entities, 0, 0, 100, CFGFLAG_CLIENT, "Overlay game tiles with a percentage of opacity")
MACRO_CONFIG_INT(ClShowQuads, cl_show_quads, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show quads")
MACRO_CONFIG_INT(ClZoomBackgroundLayers, cl_zoom_background_layers, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Zoom background layers")
MACRO_CONFIG_INT(ClBackgroundHue, cl_background_hue, 0, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Background color hue")
MACRO_CONFIG_INT(ClBackgroundSat, cl_background_sat, 0, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Background color saturation")
MACRO_CONFIG_INT(ClBackgroundLht, cl_background_lht, 128, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Background color lightness")
MACRO_CONFIG_INT(ClBackgroundEntitiesHue, cl_background_entities_hue, 0, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Background (entities) color hue")
MACRO_CONFIG_INT(ClBackgroundEntitiesSat, cl_background_entities_sat, 0, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Background (entities) color saturation")
MACRO_CONFIG_INT(ClBackgroundEntitiesLht, cl_background_entities_lht, 128, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Background (entities) color lightness")
MACRO_CONFIG_STR(ClBackgroundEntities, cl_background_entities, 100, "", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Background (entities)")
MACRO_CONFIG_INT(ClBackgroundShowTilesLayers, cl_background_show_tiles_layers, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Whether draw tiles layers when using custom background (entities)")
MACRO_CONFIG_INT(SvShowOthers, sv_show_others, 1, 0, 1, CFGFLAG_SERVER, "Whether players can user the command showothers or not")
MACRO_CONFIG_INT(SvShowOthersDefault, sv_show_others_default, 0, 0, 1, CFGFLAG_SERVER, "Whether players see others by default")
MACRO_CONFIG_INT(SvShowAllDefault, sv_show_all_default, 0, 0, 1, CFGFLAG_SERVER, "Whether players see all tees by default")
MACRO_CONFIG_INT(SvMaxAfkTime, sv_max_afk_time, 0, 0, 9999, CFGFLAG_SERVER, "The time in seconds a player is allowed to be afk (0 = disabled)")
MACRO_CONFIG_INT(SvMaxAfkVoteTime, sv_max_afk_vote_time, 300, 0, 9999, CFGFLAG_SERVER, "The time in seconds a player can be afk and his votes still count (0 = disabled)")
MACRO_CONFIG_INT(SvPlasmaRange, sv_plasma_range, 700, 1, 99999, CFGFLAG_SERVER|CFGFLAG_GAME, "How far will the plasma gun track tees")
MACRO_CONFIG_INT(SvPlasmaPerSec, sv_plasma_per_sec, 3, 0, 50, CFGFLAG_SERVER|CFGFLAG_GAME, "How many shots does the plasma gun fire per seconds")
MACRO_CONFIG_INT(SvDraggerRange, sv_dragger_range, 700, 1, 99999, CFGFLAG_SERVER|CFGFLAG_GAME, "How far will the dragger track tees")
MACRO_CONFIG_INT(SvVotePause, sv_vote_pause, 1, 0, 1, CFGFLAG_SERVER, "Allow voting to pause players (instead of moving to spectators)")
MACRO_CONFIG_INT(SvVotePauseTime, sv_vote_pause_time, 10, 0, 360, CFGFLAG_SERVER, "The time (in seconds) players have to wait in pause when paused by vote")
MACRO_CONFIG_INT(SvTuneReset, sv_tune_reset, 1, 0, 1, CFGFLAG_SERVER, "Whether tuning is reset after each map change or not")
MACRO_CONFIG_STR(SvResetFile, sv_reset_file, 128, "reset.cfg", CFGFLAG_SERVER, "File to execute on map change or reload to set the default server settings")
MACRO_CONFIG_STR(SvInputFifo, sv_input_fifo, 128, "", CFGFLAG_SERVER, "Fifo file to use as input for server console")
MACRO_CONFIG_INT(SvDDRaceTuneReset, sv_ddrace_tune_reset, 1, 0, 1, CFGFLAG_SERVER, "Whether DDRace tuning(sv_hit, Sv_Endless_Drag & Sv_Old_Laser) is reset after each map change or not")
MACRO_CONFIG_INT(SvNamelessScore, sv_nameless_score, 0, 0, 1, CFGFLAG_SERVER, "Whether nameless tee has a score or not")
MACRO_CONFIG_INT(SvTimeInBroadcastInterval, sv_time_in_broadcast_interval, 1, 0, 60, CFGFLAG_SERVER, "How often to update the broadcast time")
MACRO_CONFIG_INT(SvDefaultTimerType, sv_default_timer_type, 0, 0, 1, CFGFLAG_SERVER, "Default way of displaying time either game/round timer or broadcast. 0 = game/round timer, 1 = broadcast")


// these might need some fine tuning
MACRO_CONFIG_INT(SvChatPenalty, sv_chat_penalty, 250, 50, 1000, CFGFLAG_SERVER, "chat score will be increased by this on every message, and decremented by 1 on every tick.")
MACRO_CONFIG_INT(SvChatThreshold, sv_chat_threshold, 1000, 50, 10000 , CFGFLAG_SERVER, "if chats core exceeds this, the player will be muted for sv_spam_mute_duration seconds")
MACRO_CONFIG_INT(SvSpamMuteDuration, sv_spam_mute_duration, 60, 0, 3600 , CFGFLAG_SERVER, "how many seconds to mute, if player triggers mute on spam. 0 = off")

MACRO_CONFIG_INT(SvEvents, sv_events, 1, 0, 1, CFGFLAG_SERVER, "Enable triggering of server events, like the happy eyeemotes on some holidays.")
MACRO_CONFIG_INT(SvRankCheats, sv_rank_cheats, 0, 0, 1, CFGFLAG_SERVER, "Enable ranks after cheats have been used (file based server only)")
MACRO_CONFIG_INT(SvShutdownWhenEmpty, sv_shutdown_when_empty, 0, 0, 1, CFGFLAG_SERVER, "Shutdown server as soon as noone is on it anymore")
MACRO_CONFIG_INT(SvReloadWhenEmpty, sv_reload_when_empty, 0, 0, 2, CFGFLAG_SERVER, "Reload map when server is empty (1 = reload once, 2 = reload everytime server gets empty)")
MACRO_CONFIG_INT(SvKillProtection, sv_kill_protection, 20, 0, 9999, CFGFLAG_SERVER, "0 - Disable, 1-9999 minutes")
MACRO_CONFIG_INT(SvSoloServer, sv_solo_server, 0, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Set server to solo mode (no player interactions, has to be set before loading the map)")
MACRO_CONFIG_STR(SvClientSuggestion, sv_client_suggestion, 128, "Get DDNet client from DDNet.tw to use all features on DDNet!", CFGFLAG_SERVER, "Broadcast to display to players without DDNet client")
MACRO_CONFIG_STR(SvClientSuggestionOld, sv_client_suggestion_old, 128, "Your DDNet client is old, update it on DDNet.tw!", CFGFLAG_SERVER, "Broadcast to display to players with an old version of DDNet client")
MACRO_CONFIG_STR(SvClientSuggestionBot, sv_client_suggestion_bot, 128, "Your client has bots and can be remote controlled!\nPlease use another client like DDNet client from DDNet.tw", CFGFLAG_SERVER, "Broadcast to display to players with a known botting client")

// netlimit
MACRO_CONFIG_INT(SvNetlimit, sv_netlimit, 0, 0, 10000, CFGFLAG_SERVER, "Netlimit: Maximum amount of traffic a client is allowed to use (in kb/s)")
MACRO_CONFIG_INT(SvNetlimitAlpha, sv_netlimit_alpha, 50, 1, 100, CFGFLAG_SERVER, "Netlimit: Alpha of Exponention moving average")

MACRO_CONFIG_INT(SvConnlimit, sv_connlimit, 4, 0, 100, CFGFLAG_SERVER, "Connlimit: Number of connections an IP is allowed to do in a timespan")
MACRO_CONFIG_INT(SvConnlimitTime, sv_connlimit_time, 20, 0, 1000, CFGFLAG_SERVER, "Connlimit: Time in which IP's connections are counted")

MACRO_CONFIG_INT(ClUnpredictedShadow, cl_unpredicted_shadow, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show unpredicted shadow tee to estimate your delay")
MACRO_CONFIG_INT(ClPredictDDRace, cl_predict_ddrace, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Predict some DDRace tiles")
MACRO_CONFIG_INT(ClShowNinja, cl_show_ninja, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ninja skin")
MACRO_CONFIG_INT(ClShowOtherHookColl, cl_show_other_hook_coll, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show other players' hook collision line")
MACRO_CONFIG_INT(ClChatTeamColors, cl_chat_teamcolors, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show names in chat in team colors")
MACRO_CONFIG_INT(ClChatReset, cl_chat_reset, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Reset chat when pressing escape")
MACRO_CONFIG_INT(ClShowDirection, cl_show_direction, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Show tee direction")
MACRO_CONFIG_INT(ClHttpMapDownload, cl_http_map_download, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Try fast HTTP map download first")
MACRO_CONFIG_INT(ClOldGunPosition, cl_old_gun_position, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Tees hold gun a bit higher like in TW 0.6.1 and older")
MACRO_CONFIG_INT(ClConfirmDisconnect, cl_confirm_disconnect, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Confirmation popup before disconnecting")
MACRO_CONFIG_INT(ClTimeoutProtection, cl_timeout_protection, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Enable Timeout Protection")
MACRO_CONFIG_STR(ClTimeoutCode, cl_timeout_code, 64, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Timeout code to use")
MACRO_CONFIG_STR(ClDummyTimeoutCode, cl_dummy_timeout_code, 64, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Dummy Timeout code to use")
MACRO_CONFIG_STR(ClTimeoutSeed, cl_timeout_seed, 64, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Timeout seed")
MACRO_CONFIG_STR(ClInputFifo, cl_input_fifo, 128, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Fifo file to use as input for client console")
MACRO_CONFIG_INT(ClHideConsole, cl_hide_console, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Hide console window (Windows only)")

MACRO_CONFIG_INT(ClConsoleMode, cl_console_mode, 0, 0, 1, CFGFLAG_CLIENT, "Console only mode")

// sniffing
MACRO_CONFIG_INT(ClSniffSendConn, cl_sniff_send_conn, 0, 0, 1, CFGFLAG_CLIENT, "Sniff outgoing conn packets")
MACRO_CONFIG_INT(ClSniffSendConnless, cl_sniff_send_connless, 0, 0, 1, CFGFLAG_CLIENT, "Sniff outgoing connless packets")
MACRO_CONFIG_INT(ClSniffRecvConn, cl_sniff_recv_conn, 0, 0, 1, CFGFLAG_CLIENT, "Sniff incoming conn packets")
MACRO_CONFIG_INT(ClSniffRecvConnless, cl_sniff_recv_connless, 0, 0, 1, CFGFLAG_CLIENT, "Sniff incoming connless packets")

#if defined(__ANDROID__)
MACRO_CONFIG_INT(InpJoystick, inp_joystick, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Try to use a joystick as input")
#else
MACRO_CONFIG_INT(InpJoystick, inp_joystick, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Try to use a joystick as input")
#endif
MACRO_CONFIG_INT(ClConfigVersion, cl_config_version, 0, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "The config version. Helps newer clients fix bugs with older configs.")

// demo editor
MACRO_CONFIG_INT(ClDemoSliceBegin, cl_demo_slice_begin, -1, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Begin marker for demo slice")
MACRO_CONFIG_INT(ClDemoSliceEnd, cl_demo_slice_end, -1, 0, 0, CFGFLAG_SAVE|CFGFLAG_CLIENT, "End marker for demo slice")
MACRO_CONFIG_INT(ClDemoShowSpeed, cl_demo_show_speed, 0, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Show speed meter on change")


#ifdef undef1
#undef MACRO_CONFIG_INT
#undef undef1
#endif

#ifdef undef2
#undef MACRO_CONFIG_STR
#undef undef2
#endif

#endif
