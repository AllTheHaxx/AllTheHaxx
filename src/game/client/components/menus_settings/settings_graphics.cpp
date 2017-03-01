#include "../menus.h"

#include <engine/graphics.h>


void CMenus::RenderSettingsGraphics(CUIRect MainView)
{
	CALLSTACK_ADD();

	CUIRect Button;
	char aBuf[128];
	bool CheckSettings = false;

	static const int MAX_RESOLUTIONS = 256;
	static CVideoMode s_aModes[MAX_RESOLUTIONS];
	static int s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
	static int s_GfxScreenWidth = g_Config.m_GfxScreenWidth;
	static int s_GfxScreenHeight = g_Config.m_GfxScreenHeight;
	static int s_GfxColorDepth = g_Config.m_GfxColorDepth;
	static int s_GfxVsync = g_Config.m_GfxVsync;
	static int s_GfxFsaaSamples = g_Config.m_GfxFsaaSamples;
	static int s_GfxTextureQuality = g_Config.m_GfxTextureQuality;
	static int s_GfxTextureCompression = g_Config.m_GfxTextureCompression;
	static int s_GfxHighdpi = g_Config.m_GfxHighdpi;

	CUIRect ModeList;
	MainView.VSplitLeft(300.0f, &MainView, &ModeList);

	// draw allmodes switch
	ModeList.HSplitTop(20, &Button, &ModeList);
	static CButtonContainer s_CheckboxDisplayAllModes;
	if(DoButton_CheckBox(&s_CheckboxDisplayAllModes, Localize("Show only supported"), g_Config.m_GfxDisplayAllModes^1, &Button))
	{
		g_Config.m_GfxDisplayAllModes ^= 1;
		s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
	}

	// display mode list
	static float s_ScrollValue = 0;
	int OldSelected = -1;
	int G = gcd(s_GfxScreenWidth, s_GfxScreenHeight);
	str_format(aBuf, sizeof(aBuf), "%s: %dx%d %d bit (%d:%d)", Localize("Current"), s_GfxScreenWidth, s_GfxScreenHeight, s_GfxColorDepth, s_GfxScreenWidth/G, s_GfxScreenHeight/G);
	static CButtonContainer s_Listbox;
	UiDoListboxStart(&s_Listbox , &ModeList, 24.0f, Localize("Display Modes"), aBuf, s_NumNodes, 1, OldSelected, s_ScrollValue);

	for(int i = 0; i < s_NumNodes; ++i)
	{
		const int Depth = s_aModes[i].m_Red+s_aModes[i].m_Green+s_aModes[i].m_Blue > 16 ? 24 : 16;
		if(g_Config.m_GfxColorDepth == Depth &&
			g_Config.m_GfxScreenWidth == s_aModes[i].m_Width &&
			g_Config.m_GfxScreenHeight == s_aModes[i].m_Height)
		{
			OldSelected = i;
		}

		CPointerContainer Container(&s_aModes[i]);
		CListboxItem Item = UiDoListboxNextItem(&Container, OldSelected == i);
		if(Item.m_Visible)
		{
			int G = gcd(s_aModes[i].m_Width, s_aModes[i].m_Height);
			str_format(aBuf, sizeof(aBuf), " %dx%d %d bit (%d:%d)", s_aModes[i].m_Width, s_aModes[i].m_Height, Depth, s_aModes[i].m_Width/G, s_aModes[i].m_Height/G);
			UI()->DoLabelScaled(&Item.m_Rect, aBuf, 16.0f, -1);
		}
	}

	const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
	if(OldSelected != NewSelected)
	{
		const int Depth = s_aModes[NewSelected].m_Red+s_aModes[NewSelected].m_Green+s_aModes[NewSelected].m_Blue > 16 ? 24 : 16;
		g_Config.m_GfxColorDepth = Depth;
		g_Config.m_GfxScreenWidth = s_aModes[NewSelected].m_Width;
		g_Config.m_GfxScreenHeight = s_aModes[NewSelected].m_Height;
#if defined(SDL_VIDEO_DRIVER_X11)
		Graphics()->Resize(g_Config.m_GfxScreenWidth, g_Config.m_GfxScreenHeight);
#else
		CheckSettings = true;
#endif
	}

	// switches
	static CButtonContainer s_Checkbox1, s_Checkbox2, s_Checkbox3, s_Checkbox4, s_Checkbox5, s_Checkbox6, s_Checkbox7;
	MainView.VSplitRight(30.0f, &MainView, 0);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&s_Checkbox1, Localize("Borderless window"), g_Config.m_GfxBorderless, &Button))
	{
		Client()->ToggleWindowBordered();
	}

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&s_Checkbox2, Localize("Fullscreen"), g_Config.m_GfxFullscreen, &Button))
	{
		Client()->ToggleFullscreen();
	}

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&s_Checkbox3, Localize("V-Sync"), g_Config.m_GfxVsync, &Button, Localize("Disable this if your game reacts too slow")))
	{
		Client()->ToggleWindowVSync();
	}

	if(Graphics()->GetNumScreens() > 1)
	{
		int NumScreens = Graphics()->GetNumScreens();
		MainView.HSplitTop(3.0f, 0, &MainView);
		MainView.HSplitTop(20.0f, &Button, &MainView);
		int Screen_MouseButton = DoButton_CheckBox_Number(&s_Checkbox4, Localize("Screen"), g_Config.m_GfxScreen, &Button);
		if(Screen_MouseButton == 1) //inc
		{
			Client()->SwitchWindowScreen((g_Config.m_GfxScreen+1)%NumScreens);
			s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
		}
		else if(Screen_MouseButton == 2) //dec
		{
			Client()->SwitchWindowScreen((g_Config.m_GfxScreen-1+NumScreens)%NumScreens);
			s_NumNodes = Graphics()->GetVideoModes(s_aModes, MAX_RESOLUTIONS, g_Config.m_GfxScreen);
		}
	}

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	int GfxFsaaSamples_MouseButton = DoButton_CheckBox_Number(&s_Checkbox5, Localize("FSAA samples"), g_Config.m_GfxFsaaSamples, &Button, Localize("Smooths graphics at the expense of FPS"));
	if(GfxFsaaSamples_MouseButton == 1) //inc
	{
		g_Config.m_GfxFsaaSamples = (g_Config.m_GfxFsaaSamples+1)%17;
		CheckSettings = true;
	}
	else if(GfxFsaaSamples_MouseButton == 2) //dec
	{
		g_Config.m_GfxFsaaSamples = (g_Config.m_GfxFsaaSamples-1 +17)%17;
		CheckSettings = true;
	}

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&s_Checkbox6, Localize("Quality Textures"), g_Config.m_GfxTextureQuality, &Button))
	{
		g_Config.m_GfxTextureQuality ^= 1;
		CheckSettings = true;
	}

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	if(DoButton_CheckBox(&s_Checkbox7, Localize("Texture Compression"), g_Config.m_GfxTextureCompression, &Button, Localize("Disable this if you get blurry textures")))
	{
		g_Config.m_GfxTextureCompression ^= 1;
		CheckSettings = true;
	}

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxHighDetail;
	if(DoButton_CheckBox(&s_CheckboxHighDetail, Localize("High Detail"), g_Config.m_GfxHighDetail, &Button, Localize("Show map decoration elements")))
		g_Config.m_GfxHighDetail ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxLowGraphics;
	if(DoButton_CheckBox(&s_CheckboxLowGraphics, Localize("Low Graphics Mode"), g_Config.m_GfxLowGraphics, &Button, Localize("Disable fancy effects to gain more fps")))
		g_Config.m_GfxLowGraphics ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxBackgroundRender;
	if(DoButton_CheckBox(&s_CheckboxBackgroundRender, Localize("Render when inactive"), g_Config.m_GfxBackgroundRender, &Button, Localize("Render graphics when window is in background")))
		g_Config.m_GfxBackgroundRender ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxNoclip;
	if(DoButton_CheckBox(&s_CheckboxNoclip, Localize("Disable clipping"), g_Config.m_GfxNoclip, &Button, Localize("May kill any performance teeworlds could have. Be careful with it.\n~ Info for nerds: GL_SCISSOR_TEST will be disabled and thus EVERYTHING will be rendered → hard laggs.")))
		g_Config.m_GfxNoclip ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxQuadAsTriangle;
	if(DoButton_CheckBox(&s_CheckboxQuadAsTriangle, Localize("Render quads as triangles"), g_Config.m_GfxQuadAsTriangle, &Button, Localize("Fixes quad coloring on some GPUs")))
		g_Config.m_GfxQuadAsTriangle ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxFinish;
	if(DoButton_CheckBox(&s_CheckboxFinish, Localize("Wait for GL commands to finish"), g_Config.m_GfxFinish, &Button, Localize("Can cause FPS laggs if enabled\n~ Info for nerds: glFinish() blocks until the effects of all GL executions are complete.")))
		g_Config.m_GfxFinish ^= 1;

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxHighdpi;
	if(DoButton_CheckBox(&s_CheckboxHighdpi, Localize("High-DPI screen support"), g_Config.m_GfxHighdpi, &Button, Localize("Be careful: experimental")))
	{
		g_Config.m_GfxHighdpi ^= 1;
		CheckSettings = true;
	}

	MainView.HSplitTop(3.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Button, &MainView);
	static CButtonContainer s_CheckboxConsoleCPU;
	if(DoButton_CheckBox(&s_CheckboxConsoleCPU, Localize("Low CPU usage console"), g_Config.m_ClConsoleLowCPU, &Button, Localize("Limits FPS while console is open in order to lower our CPU usage")))
		g_Config.m_ClConsoleLowCPU ^= 1;


	// check if the new settings require a restart
	if(CheckSettings)
	{
		if(s_GfxScreenWidth == g_Config.m_GfxScreenWidth &&
			s_GfxScreenHeight == g_Config.m_GfxScreenHeight &&
			s_GfxColorDepth == g_Config.m_GfxColorDepth &&
			s_GfxVsync == g_Config.m_GfxVsync &&
			s_GfxFsaaSamples == g_Config.m_GfxFsaaSamples &&
			s_GfxTextureQuality == g_Config.m_GfxTextureQuality &&
			s_GfxTextureCompression == g_Config.m_GfxTextureCompression &&
			s_GfxHighdpi == g_Config.m_GfxHighdpi)
			m_NeedRestartGraphics = false;
		else
			m_NeedRestartGraphics = true;
	}

	CUIRect Text;
	MainView.HSplitTop(20.0f, 0, &MainView);
	MainView.HSplitTop(20.0f, &Text, &MainView);
	//text.VSplitLeft(15.0f, 0, &text);
	{
		CUIRect temp;
		temp = Text;
		temp.y -= 2.5f*1.5f;
		temp.h = 21.0f*5+2.75f;
		RenderTools()->DrawUIRect(&temp, vec4(0,0,0,0.2f), CUI::CORNER_ALL, 5.0f);
	}
	Text.VMargin(15.0f, &Text);
	UI()->DoLabelScaled(&Text, Localize("UI Color"), 14.0f, -1);

	const char *paLabels[] = {
		Localize("Hue"),
		Localize("Sat."),
		Localize("Lht."),
		Localize("Alpha")};
	int *pColorSlider[4] = {&g_Config.m_UiColorHue, &g_Config.m_UiColorSat, &g_Config.m_UiColorLht, &g_Config.m_UiColorAlpha};
	for(int s = 0; s < 4; s++)
	{
		CUIRect Text;
		MainView.HSplitTop(19.0f, &Button, &MainView);
		Button.VMargin(15.0f, &Button);
		Button.VSplitLeft(100.0f, &Text, &Button);
		//Button.VSplitRight(5.0f, &Button, 0);
		Button.HSplitTop(4.0f, 0, &Button);

		float k = (*pColorSlider[s]) / 255.0f;
		CPointerContainer Container(pColorSlider[s]);
		k = DoScrollbarH(&Container, &Button, k, 0, k*255.0f);
		*pColorSlider[s] = (int)(k*255.0f);
		UI()->DoLabelScaled(&Text, paLabels[s], 15.0f, -1);
	}

	MainView.HSplitTop(20.0f, 0, &MainView);

	MainView.HSplitTop(20.0f+10.0f+15.0f, &Text, 0);
	Text.HMargin(-2.75f, &Text);
	Text.h += 2.75f;
	RenderTools()->DrawUIRect(&Text, vec4(0,0,0,0.2f), CUI::CORNER_ALL, 5.0f);

	MainView.VMargin(15.0f, &MainView);
	MainView.HSplitTop(20.0f, &Text, &MainView);
	UI()->DoLabelScaled(&Text, Localize("UI Scale"), 14.0f, -1);

	MainView.HSplitTop(10.0f, 0, &MainView);
	MainView.HSplitTop(15.0f, &Text, &MainView);
	static CButtonContainer s_Scrollbar;
	static int s_NewVal = g_Config.m_UiScale; // proxy it to not instantly change the ui size
	if(g_Config.m_UiScale != s_NewVal && UI()->ActiveItem() != (void*)&s_Scrollbar) // if it has been changed in f1
		s_NewVal = g_Config.m_UiScale;
	s_NewVal = round_to_int(50.0f+100.0f*DoScrollbarH(&s_Scrollbar, &Text, ((float)s_NewVal-50.0f)/100.0f, Localize("READ BEFORE CHANGING:\nIf you happen to mess it up so that this slider\nis not on your screen anymore, type in f1:\nui_scale 100"), s_NewVal));
	if(UI()->ActiveItem() != (void*)&s_Scrollbar)
		g_Config.m_UiScale = s_NewVal;
}
