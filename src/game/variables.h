/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_VARIABLES_H
#define GAME_VARIABLES_H
#undef GAME_VARIABLES_H // this file will be included several times

#ifndef MACRO_CONFIG_INT
#define MACRO_CONFIG_INT(Name,ScriptName,Def,Min,Max,Save,Desc) ;
#define undef1
#endif
#ifndef MACRO_CONFIG_STR
#define MACRO_CONFIG_STR(Name,ScriptName,Len,Def,Save,Desc) ;
#define undef2
#endif


// client
MACRO_CONFIG_INT(ClPredict, cl_predict, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Predict client movements")
MACRO_CONFIG_INT(ClAntiPingLimit, cl_antiping_limit, 0, 0, 200, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Antiping limit (0 to disable)")
MACRO_CONFIG_INT(ClAntiPing, cl_antiping, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Enable antiping, i. e. more aggressive prediction.")
MACRO_CONFIG_INT(ClAntiPingPlayers, cl_antiping_players, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Predict other player's movement more aggressively (only enabled if cl_antiping is set to 1)")
MACRO_CONFIG_INT(ClAntiPingGrenade, cl_antiping_grenade, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Predict grenades (only enabled if cl_antiping is set to 1)")
MACRO_CONFIG_INT(ClAntiPingWeapons, cl_antiping_weapons, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Predict weapon projectiles (only enabled if cl_antiping is set to 1)")
MACRO_CONFIG_INT(ClNameplates, cl_nameplates, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show name plates")
MACRO_CONFIG_INT(ClNameplatesAlways, cl_nameplates_always, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Always show name plates disregarding of distance")
MACRO_CONFIG_INT(ClNameplatesTeamcolors, cl_nameplates_teamcolors, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Use team colors for name plates")
MACRO_CONFIG_INT(ClNameplatesSize, cl_nameplates_size, 50, 0, 100, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Size of the name plates from 0 to 100%")
MACRO_CONFIG_INT(ClNameplatesClan, cl_nameplates_clan, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show clan in name plates")
MACRO_CONFIG_INT(ClNameplatesClanSize, cl_nameplates_clan_size, 30, 0, 100, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Size of the clan plates from 0 to 100%")
MACRO_CONFIG_INT(ClNameplatesClancolors, cl_nameplates_clancolors, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Highlight your clan")
MACRO_CONFIG_INT(ClNamePlatesScore, cl_nameplates_score, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show score in name plates")
MACRO_CONFIG_INT(ClNamePlatesATH, cl_nameplates_ath, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show who uses this client.")
MACRO_CONFIG_INT(ClNamePlatesATHBlinkTime, cl_nameplates_ath_blinktime, 90, 0, 200, CFGFLAG_CLIENT|CFGFLAG_SAVE, "How fast the ATH sign blinks (in percent)")
MACRO_CONFIG_INT(ClTextEntities, cl_text_entities, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Render textual entity data")
#if defined(__ANDROID__)
MACRO_CONFIG_INT(ClAutoswitchWeapons, cl_autoswitch_weapons, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Auto switch weapon on pickup")
MACRO_CONFIG_INT(ClAutoswitchWeaponsOutOfAmmo, cl_autoswitch_weapons_out_of_ammo, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Auto switch weapon when out of ammo")
#else
MACRO_CONFIG_INT(ClAutoswitchWeapons, cl_autoswitch_weapons, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Auto switch weapon on pickup")
MACRO_CONFIG_INT(ClAutoswitchWeaponsOutOfAmmo, cl_autoswitch_weapons_out_of_ammo, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Auto switch weapon when out of ammo")
#endif

MACRO_CONFIG_INT(ClShowhud, cl_showhud, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ingame HUD")
MACRO_CONFIG_INT(ClShowhudHealthAmmo, cl_showhud_healthammo, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ingame HUD (Health + Ammo)")
MACRO_CONFIG_INT(ClShowhudScore, cl_showhud_score, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ingame HUD (Score)")
MACRO_CONFIG_INT(ClShowhudCursor, cl_showhud_cursor, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ingame cursor (aka crosshair)")
MACRO_CONFIG_INT(ClShowRecord, cl_showrecord, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show old style DDRace client records")
MACRO_CONFIG_INT(ClChat, cl_chat, 2, 0, 2, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Enable ingame chat (0=off, 1=server only, 2=all)")
MACRO_CONFIG_INT(ClShowChat, cl_showchat, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show chat")
MACRO_CONFIG_INT(ClShowChatFriends, cl_show_chat_friends, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show only chat messages from friends")
MACRO_CONFIG_INT(ClShowKillMessages, cl_showkillmessages, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show kill messages")
MACRO_CONFIG_INT(ClShowVotesAfterVoting, cl_show_votes_after_voting, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show votes window after voting")
MACRO_CONFIG_INT(ClShowLocalTimeAlways, cl_show_local_time_always, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Always show local time")
MACRO_CONFIG_INT(ClShowfps, cl_showfps, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ingame FPS counter")
MACRO_CONFIG_INT(ClShowpred, cl_showpred, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show ingame prediction time in milliseconds")
MACRO_CONFIG_INT(ClEyeWheel, cl_eye_wheel, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show eye wheel along together with emotes")
MACRO_CONFIG_INT(ClEyeDuration, cl_eye_duration, 999999, 1, 999999, CFGFLAG_CLIENT|CFGFLAG_SAVE, "How long the eyes emotes last")

MACRO_CONFIG_INT(ClAirjumpindicator, cl_airjumpindicator, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
//MACRO_CONFIG_INT(ClThreadsoundloading, cl_threadsoundloading, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Load sound files threaded")

MACRO_CONFIG_INT(ClWarningTeambalance, cl_warning_teambalance, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Warn about team balance")

#if defined(__ANDROID__)
MACRO_CONFIG_INT(ClMouseDeadzone, cl_mouse_deadzone, 800, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "") // Disable dynamic camera on Android, screen becomes jerky when you tap joystick
#else
MACRO_CONFIG_INT(ClMouseDeadzone, cl_mouse_deadzone, 300, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
#endif
MACRO_CONFIG_INT(ClMouseFollowfactor, cl_mouse_followfactor, 60, 0, 200, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
#if defined(__ANDROID__)
MACRO_CONFIG_INT(ClMouseMaxDistance, cl_mouse_max_distance, 400, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "") // Prevent crosshair from moving out of screen on Android
#else
MACRO_CONFIG_INT(ClMouseMaxDistance, cl_mouse_max_distance, 800, 0, 0, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
#endif

MACRO_CONFIG_INT(ClDyncam, cl_dyncam, 0, 0, 1, CFGFLAG_CLIENT, "Enable dyncam")
MACRO_CONFIG_INT(ClDyncamMaxDistance, cl_dyncam_max_distance, 1000, 0, 2000, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Maximal dynamic camera distance")
MACRO_CONFIG_INT(ClDyncamMousesens, cl_dyncam_mousesens, 0, 0, 100000, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Mouse sens used when dyncam is toggled on")
MACRO_CONFIG_INT(ClDyncamDeadzone, cl_dyncam_deadzone, 300, 1, 1300, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Dynamic camera dead zone")
MACRO_CONFIG_INT(ClDyncamFollowFactor, cl_dyncam_follow_factor, 60, 0, 200, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Dynamic camera follow factor")
MACRO_CONFIG_INT(ClCameraMaxDistance, cl_camera_max_distance, 200, 0, 99999, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Super dynamic camera")

MACRO_CONFIG_INT(EdZoomTarget, ed_zoom_target, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Zoom to the current mouse target")
MACRO_CONFIG_INT(EdShowkeys, ed_showkeys, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")

//MACRO_CONFIG_INT(ClFlow, cl_flow, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")

MACRO_CONFIG_INT(ClShowWelcome, cl_show_welcome, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "")
MACRO_CONFIG_INT(ClMotdTime, cl_motd_time, 10, 0, 100, CFGFLAG_CLIENT|CFGFLAG_SAVE, "How long to show the server message of the day")

MACRO_CONFIG_STR(ClDDNetVersionServer, cl_ddnet_version_server, 100, "version.ddnet.tw", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Server to use to check for ddnet version (needed for DDNet-Serverlist)")
MACRO_CONFIG_STR(ClDDNetUpdate2Server, cl_ddnet_update2_server, 100, "update2.ddnet.tw", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Server to use to update new ddnet versions")
MACRO_CONFIG_STR(ClMapDbFile, cl_mapdb_file, 128, "data/maps/urls.cfg", CFGFLAG_CLIENT|CFGFLAG_SAVE, "File to load the mapserver addresses from")

MACRO_CONFIG_STR(ClLanguagefile, cl_languagefile, 255, "", CFGFLAG_CLIENT|CFGFLAG_SAVE, "What language file to use")
MACRO_CONFIG_INT(ClVanillaSkinsOnly, cl_vanilla_skins_only, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Only show skins available in Vanilla Teeworlds")
MACRO_CONFIG_INT(ClAutoStatboardScreenshot, cl_auto_statboard_screenshot, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Automatically take game over statboard screenshot")
MACRO_CONFIG_INT(ClAutoStatboardScreenshotMax, cl_auto_statboard_screenshot_max, 10, 0, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Maximum number of automatically created statboard screenshots (0 = no limit)")

MACRO_CONFIG_INT(ClDefaultZoom, cl_default_zoom, 10, 0, 20, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Default zoom level (10 default, min 0, max 20)")

MACRO_CONFIG_INT(ClPlayerUseCustomColor, player_use_custom_color, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Toggles usage of custom colors")
MACRO_CONFIG_INT(ClPlayerColorBody, player_color_body, 65408, 0, 0xFFFFFF, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player body color")
MACRO_CONFIG_INT(ClPlayerColorFeet, player_color_feet, 65408, 0, 0xFFFFFF, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player feet color")

MACRO_CONFIG_STR(ClPlayerSkin, player_skin, 24, "default", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Player skin")

MACRO_CONFIG_INT(UiPage, ui_page, 13, 0, 17, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface page")
MACRO_CONFIG_INT(UiBrowserPage, ui_browser_page, 0, 0, 4, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface browser page")
MACRO_CONFIG_INT(UiSettingsPage, ui_settings_page, 2, 0, 9, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface settings page")
MACRO_CONFIG_INT(UiManualPage, ui_manual_page, 0, 0, 2, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface manual page")
MACRO_CONFIG_INT(UiToolboxPage, ui_toolbox_page, 0, 0, 2, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Toolbox page")
MACRO_CONFIG_STR(UiServerAddress, ui_server_address, 64, "localhost:8303", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface server address")
MACRO_CONFIG_INT(UiScale, ui_scale, 100, 50, 150, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface scale")
MACRO_CONFIG_INT(UiMousesens, ui_mousesens, 100, 5, 100000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Mouse sensitivity for menus/editor")

MACRO_CONFIG_INT(UiColorHue, ui_color_hue, 78, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color hue")
MACRO_CONFIG_INT(UiColorSat, ui_color_sat, 169, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color saturation")
MACRO_CONFIG_INT(UiColorLht, ui_color_lht, 102, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface color lightness")
MACRO_CONFIG_INT(UiColorAlpha, ui_color_alpha, 198, 0, 255, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Interface alpha")

MACRO_CONFIG_INT(UiColorizePing, br_colored_ping, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Highlight ping")
MACRO_CONFIG_INT(UiColorizeGametype, br_colored_gametype, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Highlight gametype")

MACRO_CONFIG_STR(UiDemoSelected, ui_demo_selected, 256, "", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Selected demo file")

MACRO_CONFIG_INT(GfxNoclip, gfx_noclip, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Disable clipping")

// dummy
MACRO_CONFIG_STR(ClDummyName, dummy_name, 16, "haxxless dummy", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Name of the Dummy")
MACRO_CONFIG_STR(ClDummyClan, dummy_clan, 12, "", CFGFLAG_SAVE|CFGFLAG_CLIENT, "Clan of the Dummy")
MACRO_CONFIG_INT(ClDummyCountry, dummy_country, -1, -1, 1000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Country of the Dummy")
MACRO_CONFIG_INT(ClDummyUseCustomColor, dummy_use_custom_color, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Toggles usage of custom colors")
MACRO_CONFIG_INT(ClDummyColorBody, dummy_color_body, 65408, 0, 0xFFFFFF, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Dummy body color")
MACRO_CONFIG_INT(ClDummyColorFeet, dummy_color_feet, 65408, 0, 0xFFFFFF, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Dummy feet color")
MACRO_CONFIG_STR(ClDummySkin, dummy_skin, 24, "default", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Dummy skin")
MACRO_CONFIG_INT(ClDummy, cl_dummy, 0, 0, 1, CFGFLAG_CLIENT, "0 - player / 1 - dummy")
MACRO_CONFIG_INT(ClDummyAutoSwitch, cl_dummy_auto_switch, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Switch to dummy when connecting")
MACRO_CONFIG_INT(ClDummyHammer, cl_dummy_hammer, 0, 0, 1, CFGFLAG_CLIENT, "Whether dummy is hammering for a hammerfly")
MACRO_CONFIG_INT(ClDummyResetOnSwitch, cl_dummy_resetonswitch, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Whether dummy should stop pressing keys when you switch")
MACRO_CONFIG_INT(ClDummyCopyMoves, cl_dummy_copy_moves, 0, 0, 1, CFGFLAG_CLIENT, "Whether dummy should copy your moves")
// advanced dummy
MACRO_CONFIG_INT(ClDummyHookFly, cl_dummy_hook_fly, 0, 0, 1, CFGFLAG_CLIENT, "Hook-fly with your dummy")
MACRO_CONFIG_INT(ClDummyCopyMirror, cl_dummy_copy_mirror, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Wether dummy copys your moves like a mirroŕ")

// curl http download
MACRO_CONFIG_INT(ClHTTPConnectTimeoutMs, cl_http_connect_timeout_ms, 2000, 0, 100000, CFGFLAG_CLIENT, "HTTP downloads: timeout for the connect phase in milliseconds (0 to disable)")
MACRO_CONFIG_INT(ClHTTPLowSpeedLimit, cl_http_low_speed_limit, 500, 0, 100000, CFGFLAG_CLIENT, "HTTP downloads: Set low speed limit in bytes per second (0 to disable)")
MACRO_CONFIG_INT(ClHTTPLowSpeedTime, cl_http_low_speed_time, 5, 0, 100000, CFGFLAG_CLIENT, "HTTP downloads: Set low speed limit time period (0 to disable)")


// haxx
MACRO_CONFIG_INT(BrAutoRefresh, br_auto_refresh, 0, 0, 3600, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Time in secs to refresh the serverbrowser when active (0 disables)")
MACRO_CONFIG_INT(BrAutoCache, br_auto_cache, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Enable automatic serverlist cache management (recommended)")
MACRO_CONFIG_INT(BrShowDDNet, br_show_ddnet, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show DDNet tab in serverbrowser")
MACRO_CONFIG_INT(BrAllowPureMod, br_allow_pure_mod, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Allow joining modded pure server")
MACRO_CONFIG_INT(BrLazySorting, br_lazy_sorting, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Only sort the serverlist once when reloading has finished")
//
MACRO_CONFIG_STR(ClSpoofSrvIP, cl_spoofsrv_ip, 32, "127.0.0.1", CFGFLAG_CLIENT|CFGFLAG_SAVE, "IP of the spoofing server")
MACRO_CONFIG_INT(ClSpoofSrvPort, cl_spoofsrv_port, 0, 0, 65535, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Port of the spoofing server")
MACRO_CONFIG_INT(ClSpoofAutoconnect, cl_spoof_autoconnect, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Connect to zervor on startup automatically")
MACRO_CONFIG_INT(ClUsernameFetching, cl_fetch_names, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Get rich! #namefreestyle")
MACRO_CONFIG_INT(ClChatShowIPs, cl_chat_show_ips, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Whether to show IPs in chat or just save them")
MACRO_CONFIG_INT(ClChatDennisProtection, cl_chat_dennis_protection, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Be protected like a dennis. Be a dennis.")
MACRO_CONFIG_INT(ClColorfulClient, cl_colorful_client, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Colorize everything! (highly recommended!!)")
MACRO_CONFIG_INT(ClNotifications, cl_notifications, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Show notifications")
MACRO_CONFIG_INT(ClPathFinding, cl_path_finding, 1, 0, 1, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Find and display the shortest path in ddrace using a* algorithm")
MACRO_CONFIG_INT(ClShowhudMode, cl_showhud_mode, 1, 0, 2, CFGFLAG_CLIENT|CFGFLAG_SAVE, "0: vanilla | 1: bars | 2: numbers")
MACRO_CONFIG_INT(ClShowhudChatbox, cl_showhud_chatbox, 34, 0, 100, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Nice background for the chat (alpha, 0-100)")
MACRO_CONFIG_INT(ClShowhudChatMsgTime, cl_chatmsg_time, 16, 1, 30, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show long chat messages should stay on the screen")
MACRO_CONFIG_INT(ClUiShowExtraBar, cl_ui_extra_bar, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Show the extra bar")
MACRO_CONFIG_INT(ClSmartZoom, cl_smart_zoom, 0, 0, 2, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Zoom in/out in a smart way (0=off, 1=race only, 2=all mods)")
MACRO_CONFIG_INT(ClSmartZoomVal, cl_smart_zoom_val, 100, 0, 1000, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Percentage to multiply the base zooming intensity with")
MACRO_CONFIG_INT(ClCinematicCamera, cl_cinematic_camera, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Smooth spectator cam movement")
MACRO_CONFIG_INT(ClCinematicCameraDelay, cl_cinematic_camera_delay, 25, 0, 250, CFGFLAG_CLIENT|CFGFLAG_SAVE, "How long it should take for the cinematic camera to move")
MACRO_CONFIG_INT(ClSmoothChat, cl_smooth_chat, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Smooth chat animation")
MACRO_CONFIG_INT(ClSmoothEmoteWheel, cl_smooth_emotewheel, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Smooth emote wheel animation")
MACRO_CONFIG_INT(ClSmoothEmoteWheelDelay, cl_smooth_emotewheel_delay, 40, 1, 1000, CFGFLAG_CLIENT|CFGFLAG_SAVE, "How long the smooth emote wheel animation should take")
MACRO_CONFIG_INT(ClConsoleModeEmotes, cl_console_mode_emotes, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Send \"Zzz\" emotes while being in console mode")
MACRO_CONFIG_INT(ClResetServerCfgOnDc, cl_reset_server_cfg_on_disconnect, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Discard config changes when disconnecting, use 'config_save' if you want to keep them")
MACRO_CONFIG_INT(ClSendHookline, cl_send_hookline, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Send the hookline (forced-off on stitch-* gamemodes)")
MACRO_CONFIG_STR(ClSkinDbFile, cl_skin_db_file, 128, "data/skins/urls.cfg", CFGFLAG_CLIENT|CFGFLAG_SAVE, "File which holds the skin database urls")
MACRO_CONFIG_INT(ClSkinFetcher, cl_skin_fetcher, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "[EXPERIMENTAL] Fetch skins from public skin databases automatically as they are needed  (→ see cl_skin_db_file)")
MACRO_CONFIG_INT(ClChatbubble, cl_chatbubble, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Notifiy others that you are typing")
MACRO_CONFIG_STR(FtFont, ft_font_pack, 128, "DejaVuSansCJKName", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Font to use")
MACRO_CONFIG_STR(FtMonoFont, ft_mono_font_pack, 128, "UbuntuMono", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Monospace font to use")
MACRO_CONFIG_INT(FtPreloadFonts, ft_preload_fonts, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Load all fonts in 'data/fonts/' into memory on client start (enables font-preview!)")
MACRO_CONFIG_INT(ClMonoFontSize, cl_monofont_size, 10, 6, 16, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Font size in Console and IRC")
MACRO_CONFIG_INT(ClConsoleLowCPU, cl_console_low_cpu, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Limits FPS while console is open in order to lower our CPU usage")

// irc
MACRO_CONFIG_INT(ClIRCAutoconnect, cl_irc_autoconnect, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Connect to IRC on startup automatically")
MACRO_CONFIG_STR(ClIRCNick, cl_irc_nick, 16, "", CFGFLAG_CLIENT|CFGFLAG_SAVE, "IRC nick")
//MACRO_CONFIG_STR(ClIRCRealname, cl_irc_realname, 32, "AllTheHaxx-User", CFGFLAG_CLIENT|CFGFLAG_SAVE, "IRC realname")
MACRO_CONFIG_STR(ClIRCPass, cl_irc_password, 16, "", CFGFLAG_CLIENT|CFGFLAG_SAVE, "IRC password")
MACRO_CONFIG_STR(ClIRCQAuthName, cl_irc_q_auth_name, 32, "", CFGFLAG_CLIENT|CFGFLAG_SAVE, "IRC Quakenet Q name")
MACRO_CONFIG_STR(ClIRCQAuthPass, cl_irc_q_auth_pass, 11, "", CFGFLAG_CLIENT|CFGFLAG_SAVE, "IRC Quakenet Q password")
MACRO_CONFIG_STR(ClIRCModes, cl_irc_modes, 16, "+ix-w", CFGFLAG_CLIENT|CFGFLAG_SAVE, "IRC modes")
MACRO_CONFIG_STR(ClIRCLeaveMsg, cl_irc_leavemsg, 32, "Leaving", CFGFLAG_CLIENT|CFGFLAG_SAVE, "IRC leave message")
MACRO_CONFIG_INT(ClIRCPrintChat, cl_irc_print_messages, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Print messages from the IRC chat into console")
MACRO_CONFIG_INT(ClIRCGetStartupMsgs, cl_irc_get_startup_msgs, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Whether to get some messages from the server when connecting")
MACRO_CONFIG_INT(SndIRC, snd_irc, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Play a sound when an irc chat message arrives")
MACRO_CONFIG_INT(ClIRCAllowJoin, cl_irc_allow_join, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Allow others to request the server you're playing on")
//
#if defined(FEATURE_LUA)
MACRO_CONFIG_INT(ClLua, cl_lua, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Enable Lua")
#else
MACRO_CONFIG_INT(ClLua, cl_lua, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "THIS VERSION OF ALLTHEHAXX WAS COMPILED WITHOUT LUA SUPPORT !!")
#endif
MACRO_CONFIG_INT(ClPrintStartup, cl_print_startup, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Print the client startup to console") // TODO: DO!
//
MACRO_CONFIG_INT(TexLazyLoading, tex_lazy_loading, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Wait with loading the actual textures until they're needed")
MACRO_CONFIG_STR(TexGame, tex_game, 128, "!default", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Gameskin texture")
MACRO_CONFIG_STR(TexParticles, tex_particles, 128, "!default", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Particle texture")
MACRO_CONFIG_STR(TexEmoticons, tex_emoticon, 128, "!default", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Emoticons texture")
MACRO_CONFIG_STR(TexCursor, tex_cursor, 128, "!default", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Cursor texture")

// translator stuff
MACRO_CONFIG_INT(ClTransIn, cl_trans_in, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Translate incoming messages")
MACRO_CONFIG_INT(ClTransOut, cl_trans_out, 0, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Translate outgoing messages")
MACRO_CONFIG_INT(ClTransChatCmds, cl_trans_chat_cmds, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Enable/disable client side traslation chat commands")
MACRO_CONFIG_STR(ClTransInSrc, cl_trans_in_src, 4, "ru", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Which language are the incoming messages written in?")
MACRO_CONFIG_STR(ClTransInDst, cl_trans_in_dest, 4, "en", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Which language should incoming messages be translated?")
MACRO_CONFIG_STR(ClTransOutSrc, cl_trans_out_src, 4, "en", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Which language are the outgoing messages written in?")
MACRO_CONFIG_STR(ClTransOutDst, cl_trans_out_dest, 4, "ru", CFGFLAG_CLIENT|CFGFLAG_SAVE, "Which language should outgoing messages be translated?")

// chatcrypt
MACRO_CONFIG_INT(ClFlagChat, cl_flag_chat, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Receive flagchat (anti-logging-admin), players must be nearby")
MACRO_CONFIG_INT(ClFlagChatPause, cl_flag_chat_pause, 3, 1, 10, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Speed of the flagchat; 1=high, 10=low; the faster the more errors will occur")

//
MACRO_CONFIG_INT(ClScoreboardFadeTime, cl_scoreboard_fade_duration, 400, 0, 10000, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Scoreboard fading time")
MACRO_CONFIG_INT(ClMouseRotation, cl_mouse_rotation, 0, 0, 360, CFGFLAG_SAVE|CFGFLAG_CLIENT, "Rotate the mouse cursor (degree)")

MACRO_CONFIG_INT(ClMenuBackground, cl_menu_background, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Activate Menu Background")
MACRO_CONFIG_INT(ClMenuBackgroundRotation, cl_menu_background_rotation, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Activate Menu Background Camera rotation")
MACRO_CONFIG_INT(ClMenuBackgroundRotationRadius, cl_menu_background_rotation_radius, 30, 1, 500, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Menu Background Camera rotation radius")
MACRO_CONFIG_INT(ClMenuBackgroundRotationSpeed, cl_menu_background_rotation_speed, 40, 1, 120, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Menu Background Camera rotation duration")

MACRO_CONFIG_INT(ClChatAvatar, cl_chat_avatar, 1, 0, 1, CFGFLAG_CLIENT|CFGFLAG_SAVE, "Render a small avatar in front of every chat message")

// server
MACRO_CONFIG_INT(SvWarmup, sv_warmup, 0, 0, 0, CFGFLAG_SERVER, "Number of seconds to do warmup before round starts")
MACRO_CONFIG_STR(SvMotd, sv_motd, 900, "", CFGFLAG_SERVER, "Message of the day to display for the clients")
MACRO_CONFIG_INT(SvTeamdamage, sv_teamdamage, 0, 0, 1, CFGFLAG_SERVER, "Team damage")
MACRO_CONFIG_STR(SvMaprotation, sv_maprotation, 768, "", CFGFLAG_SERVER, "Maps to rotate between")
MACRO_CONFIG_INT(SvRoundsPerMap, sv_rounds_per_map, 1, 1, 100, CFGFLAG_SERVER, "Number of rounds on each map before rotating")
MACRO_CONFIG_INT(SvRoundSwap, sv_round_swap, 1, 0, 1, CFGFLAG_SERVER, "Swap teams between rounds")
MACRO_CONFIG_INT(SvPowerups, sv_powerups, 1, 0, 1, CFGFLAG_SERVER, "Allow powerups like ninja")
MACRO_CONFIG_INT(SvScorelimit, sv_scorelimit, 20, 0, 1000, CFGFLAG_SERVER, "Score limit (0 disables)")
MACRO_CONFIG_INT(SvTimelimit, sv_timelimit, 0, 0, 1000, CFGFLAG_SERVER, "Time limit in minutes (0 disables)")
MACRO_CONFIG_INT(SvTournamentMode, sv_tournament_mode, 0, 0, 1, CFGFLAG_SERVER, "Tournament mode. When enabled, players joins the server as spectator")
MACRO_CONFIG_INT(SvSpamprotection, sv_spamprotection, 1, 0, 1, CFGFLAG_SERVER, "Spam protection")

MACRO_CONFIG_INT(SvRespawnDelayTDM, sv_respawn_delay_tdm, 3, 0, 10, CFGFLAG_SERVER, "Time needed to respawn after death in tdm gametype")

MACRO_CONFIG_INT(SvSpectatorSlots, sv_spectator_slots, 0, 0, MAX_CLIENTS, CFGFLAG_SERVER, "Number of slots to reserve for spectators")
MACRO_CONFIG_INT(SvTeambalanceTime, sv_teambalance_time, 1, 0, 1000, CFGFLAG_SERVER, "How many minutes to wait before autobalancing teams")
MACRO_CONFIG_INT(SvInactiveKickTime, sv_inactivekick_time, 0, 0, 1000, CFGFLAG_SERVER, "How many minutes to wait before taking care of inactive players")
MACRO_CONFIG_INT(SvInactiveKick, sv_inactivekick, 0, 0, 2, CFGFLAG_SERVER, "How to deal with inactive players (0=move to spectator, 1=move to free spectator slot/kick, 2=kick)")

MACRO_CONFIG_INT(SvStrictSpectateMode, sv_strict_spectate_mode, 0, 0, 1, CFGFLAG_SERVER, "Restricts information in spectator mode")
MACRO_CONFIG_INT(SvVoteSpectate, sv_vote_spectate, 1, 0, 1, CFGFLAG_SERVER, "Allow voting to move players to spectators")
MACRO_CONFIG_INT(SvVoteSpectateRejoindelay, sv_vote_spectate_rejoindelay, 3, 0, 1000, CFGFLAG_SERVER, "How many minutes to wait before a player can rejoin after being moved to spectators by vote")
MACRO_CONFIG_INT(SvVoteKick, sv_vote_kick, 1, 0, 1, CFGFLAG_SERVER, "Allow voting to kick players")
MACRO_CONFIG_INT(SvVoteKickMin, sv_vote_kick_min, 0, 0, MAX_CLIENTS, CFGFLAG_SERVER, "Minimum number of players required to start a kick vote")
MACRO_CONFIG_INT(SvVoteKickBantime, sv_vote_kick_bantime, 5, 0, 1440, CFGFLAG_SERVER, "The time in seconds to ban a player if kicked by vote. 0 makes it just use kick")
MACRO_CONFIG_INT(SvJoinVoteDelay, sv_join_vote_delay, 60, 0, 1000, CFGFLAG_SERVER, "Add a delay before recently joined players can vote (in seconds)")
MACRO_CONFIG_INT(SvOldTeleportWeapons, sv_old_teleport_weapons, 0, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Teleporting of all weapons (deprecated, use special entities instead)")
MACRO_CONFIG_INT(SvOldTeleportHook, sv_old_teleport_hook, 0, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Hook through teleporter (deprecated, use special entities instead)")
MACRO_CONFIG_INT(SvTeleportHoldHook, sv_teleport_hold_hook, 0, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Hold hook when teleported")
MACRO_CONFIG_INT(SvTeleportLoseWeapons, sv_teleport_lose_weapons, 0, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Lose weapons when teleported (useful for some race maps)")
MACRO_CONFIG_INT(SvDeepfly, sv_deepfly, 1, 0, 1, CFGFLAG_SERVER|CFGFLAG_GAME, "Allow fire non auto weapons when deep")

MACRO_CONFIG_INT(SvMapUpdateRate, sv_mapupdaterate, 5, 1, 100, CFGFLAG_SERVER, "64 player id <-> vanilla id players map update rate")

MACRO_CONFIG_INT(SvSkinStealAction, sv_skinstealaction, 0, 0, 1, CFGFLAG_SERVER, "How to punish skin stealing (currently only 1 = force pinky)")

MACRO_CONFIG_STR(SvServerType, sv_server_type, 64, "none", CFGFLAG_SERVER, "Type of the server (novice, moderate, ...)")

MACRO_CONFIG_INT(SvSendVotesPerTick, sv_send_votes_per_tick, 5, 1, 15, CFGFLAG_SERVER, "Number of vote options being send per tick")

MACRO_CONFIG_INT(SvRescue, sv_rescue, 0, 0, 1, CFGFLAG_SERVER, "Allow /rescue command so players can teleport themselves out of freeze")
MACRO_CONFIG_INT(SvRescueDelay, sv_rescue_delay, 5, 0, 1000, CFGFLAG_SERVER, "Number of seconds inbetween two rescues")

// debug
#ifdef CONF_DEBUG // this one can crash the server if not used correctly
	MACRO_CONFIG_INT(DbgDummies, dbg_dummies, 0, 0, 15, CFGFLAG_SERVER, "")
#endif

MACRO_CONFIG_INT(DbgFocus, dbg_focus, 0, 0, 1, CFGFLAG_CLIENT, "")
MACRO_CONFIG_INT(DbgTuning, dbg_tuning, 0, 0, 1, CFGFLAG_CLIENT, "")


#ifdef undef1
#undef MACRO_CONFIG_INT
#undef undef1
#endif

#ifdef undef2
#undef MACRO_CONFIG_STR
#undef undef2
#endif

#endif
