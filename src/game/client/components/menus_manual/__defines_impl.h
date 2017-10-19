#ifndef GAME_CLIENT_COMPONENTS_MENUS_MANUAL__DEFINES_IMPL_H
#define GAME_CLIENT_COMPONENTS_MENUS_MANUAL__DEFINES_IMPL_H
#undef GAME_CLIENT_COMPONENTS_MENUS_MANUAL__DEFINES_IMPL_H // this file will be included multiple times

#ifdef PUTLINE
#warning polluted define "PUTLINE"
#endif

#ifdef NEWLINE
#warning polluted define "NEWLINE"
#endif

#define PUTLINE(LINE) \
			if(str_length(LINE) > 0 && LINE[0] == '|' && LINE[str_length(LINE)-1] == '|') \
			{ \
				MainView.HSplitTop(30.0f, &Label, &MainView); \
				char aBuf[512]; \
				str_copy(aBuf, LINE, sizeof(aBuf)); \
				aBuf[str_length(aBuf)-1] = '\0'; \
				UI()->DoLabelScaled(&Label, aBuf+1, 20.0f, -1, MainView.w); \
				for(int i = 1; i < TextRender()->TextLineCount(0, 20.0f, LINE, MainView.w); i++) MainView.HSplitTop(30.0f, &Label, &MainView); \
			} \
			else \
			{ \
				MainView.HSplitTop(20.0f, &Label, &MainView); \
				UI()->DoLabelScaled(&Label, LINE, 15.f, -1, (MainView.w - 30.0f)); \
				for(int i = 1; i < TextRender()->TextLineCount(0, 15.0f, LINE, (MainView.w - 30.0f)); i++) \
					MainView.HSplitTop(20.0f, &Label, &MainView); \
			}
#define NEWLINE() MainView.HSplitTop(20.0f, &Label, &MainView);

#endif
