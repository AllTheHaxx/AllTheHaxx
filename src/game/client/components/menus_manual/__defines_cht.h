#ifndef GAME_CLIENT_COMPONENTS_MENUS_MANUAL__DEFINES_CHT_H
#define GAME_CLIENT_COMPONENTS_MENUS_MANUAL__DEFINES_CHT_H
#undef GAME_CLIENT_COMPONENTS_MENUS_MANUAL__DEFINES_CHT_H // this file will be included multiple times

#ifdef PUTLINE
#warning polluted define "PUTLINE"
#endif

#ifdef NEWLINE
#warning polluted define "NEWLINE"
#endif

#define PUTLINE(LINE) \
			if(str_length(LINE) > 0 && LINE[0] == '|' && LINE[str_length(LINE)-1] == '|') \
			{ \
				s_TotalHeight += 32.0f; \
				for(int i = 1; i < TextRender()->TextLineCount(0, 20.0f, LINE, (int)MainView.w); i++) s_TotalHeight += 32.0f; \
			} \
			else \
			{ \
				s_TotalHeight += 22.0f; \
				for(int i = 1; i < TextRender()->TextLineCount(0, 15.0f, LINE, (int)MainView.w); i++) s_TotalHeight += 22.0f; \
			}
#define NEWLINE() s_TotalHeight += 22.0f;

#endif