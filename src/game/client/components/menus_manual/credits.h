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
PUTLINE("Teeworlds client by the teeworlds (and also the DDNet) community. There are too many developers to list,")
PUTLINE("so credits to everyone who ever played DDNet and/or teeworlds, especially to all active developers.")
PUTLINE("However currently the maintainers of this client are FuroS and xush'. Former maintainers: Henritees, fallenkn")
PUTLINE("Official Website: https://allthehaxx.github.io  - ANYTHING ELSE IS SCAM!!!")
NEWLINE()
NEWLINE()
PUTLINE("|- Credits and Thanks -|")
PUTLINE("All rights of this client modification belong to \"The AllTheHaxx Team\", which must be mentioned in all modified distributions - see license.")
PUTLINE("A list of contributors can be found on our GitHub page: https://github.com/AllTheHaxx/AllTheHaxx")
PUTLINE("The client is built upon the DDNet client (see ddnet.tw) for full compatibility with the race gamemodes and various code snippets")
PUTLINE("The built-in version is DDNet " DDNET_VERSION)
NEWLINE()
PUTLINE("|Special Thanks go to...|")
PUTLINE("- \"unsigned char*\" (@CytraL) for his IRC implementation in HClient, of which we use a highly modified version")
PUTLINE("- \"Stitch626\" (@Stitch626) for spending hours and nights to help me with debugging even though he doesn't use the client :D")
PUTLINE("- \"heinrich5991\" for allowing us to use his maps database for webdl")
PUTLINE("- \"raven_kus\" (@raven-kus) for the russian translation")
PUTLINE("- \"DrAzZeR\" for ideas, ongoing feedback and spreading of the client")
NEWLINE()
PUTLINE("|More general thanks go to...|")
PUTLINE("- All our beta testers")
PUTLINE("- All contributors to DDNet and Teeworlds")
PUTLINE("- matricks for inventing this awesome game! :P")
NEWLINE()
PUTLINE("This is just a draft, so if you feel like I forgot you here, please tell me!")
NEWLINE()
NEWLINE()
PUTLINE("|Contact|")
PUTLINE("For general questions or just wanting to have a good talk, join the IRC #AllTheHaxx@Quakenet (→ press F5 :D)")
PUTLINE("For more specific requests or script proposes, add me to Skype ('henningstone') or write me an email: henritees@mein-twserver.de")

#endif
