#ifndef GAME_CLIENT_COMPONENTS_MENUS_MANUAL_ABOUT_H
#define GAME_CLIENT_COMPONENTS_MENUS_MANUAL_ABOUT_H
#undef GAME_CLIENT_COMPONENTS_MENUS_MANUAL_ABOUT_H // this file will be included multiple times

#ifndef PUTLINE
#define PUTLINE(LINE) ;
#endif

#ifndef NEWLINE
#define NEWLINE() ;
#endif

PUTLINE("|AllTheHaxx Beta - Version " ATH_VERSION "|")
PUTLINE("Exact version: 'ATH-" ALLTHEHAXX_VERSION "'  |  Build Date: " BUILD_DATE)
PUTLINE("Teeworlds client by Henritees, FuroS, xush'")
PUTLINE("Official Website: allthehaxx.github.io  - ANYTHING ELSE IS SCAM!!")
NEWLINE()
PUTLINE("This client provides cool features like a full-featured Lua API, a special design with many colors and much smoothness an a built-in IRC client to get you chatting with each other ;)")
PUTLINE("The Lua-API provides it's users with an interface to create their own \"addons\" and add features to the client modularily")
NEWLINE()
PUTLINE("|Overview over some other features:|")
PUTLINE("- SDL2 - smoother gameplay, window resizing without restarting and copy-paste")
PUTLINE("- Serverlist cache, fast serverlist updating")
PUTLINE("- Recent servers")
PUTLINE("- Skin fetcher - automatically get custom skins from other players!")
PUTLINE("- Consolemode, no graphics, just console (low CPU usage)")
PUTLINE("- Enter console commands right into the black console window")
PUTLINE("- Identity management system")
PUTLINE("- Translator for chat (powered by MyMemory)")
PUTLINE("- A* path finding for race maps")
PUTLINE("- Epic Zoom & SmartZoom")
PUTLINE("- Crypted Chat (using RSA)")
PUTLINE("- Tee-to-Tee-/Hidden-Chat (aka \"Flagchat\")")
PUTLINE("- Change textures on the fly from the settings")
PUTLINE("- Always up-to-date DDNet version")
PUTLINE("- Opensource")
PUTLINE("- Random splashes!")
PUTLINE("- Contains free melon power (may also contain traces of peanuts)!")

#endif
