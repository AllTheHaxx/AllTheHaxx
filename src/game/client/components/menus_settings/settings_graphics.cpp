#include "../menus.h"

#include <engine/graphics.h>
#include <engine/textrender.h>


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

	// prepare the grid
	CUIRect Left, Right, ModeList;
	MainView.VSplitLeft(300.0f, &Left, &MainView);
	MainView.VSplitLeft(300.0f, &Right, &ModeList);
	Left.VSplitRight(10.0f/2.0f, &Left, 0);
	Right.VMargin(10.0f/2.0f, &Right);
	ModeList.VSplitLeft(10.0f/2.0f, 0, &ModeList);


	// draw allmodes switch
	ModeList.HSplitTop(20.0f, &Button, &ModeList);
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
//#if defined(SDL_VIDEO_DRIVER_X11)
		Graphics()->Resize(g_Config.m_GfxScreenWidth, g_Config.m_GfxScreenHeight);
//#else
//		CheckSettings = true;
//#endif
	}

	// switches
	static CButtonContainer s_Checkbox1, s_Checkbox2, s_Checkbox3, s_Checkbox4, s_Checkbox5, s_Checkbox6, s_Checkbox7;
	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&s_Checkbox1, Localize("Borderless window"), g_Config.m_GfxWindowMode == IGraphics::WINDOWMODE_BORDERLESS, &Button, 0, CUI::CORNER_T))
	{
		if(g_Config.m_GfxWindowMode == IGraphics::WINDOWMODE_BORDERLESS)
			g_Config.m_GfxWindowMode = IGraphics::WINDOWMODE_WINDOWED;
		else
			g_Config.m_GfxWindowMode = IGraphics::WINDOWMODE_BORDERLESS;
		Client()->GfxUpdateWindowMode();
	}

	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&s_Checkbox2, Localize("Fullscreen"), g_Config.m_GfxWindowMode == IGraphics::WINDOWMODE_FULLSCREEN, &Button, 0, CUI::CORNER_B))
	{
		if(g_Config.m_GfxWindowMode == IGraphics::WINDOWMODE_FULLSCREEN)
			g_Config.m_GfxWindowMode = IGraphics::WINDOWMODE_WINDOWED;
		else
			g_Config.m_GfxWindowMode = IGraphics::WINDOWMODE_FULLSCREEN;
		Client()->GfxUpdateWindowMode();
	}


	if(Graphics()->GetNumScreens() > 1)
	{
		int NumScreens = Graphics()->GetNumScreens();
		Right.HSplitTop(3.0f, 0, &Right);
		Right.HSplitTop(20.0f, &Button, &Right);
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


	Left.HSplitTop(3.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	if(DoButton_CheckBox(&s_Checkbox6, Localize("Quality Textures"), g_Config.m_GfxTextureQuality, &Button))
	{
		g_Config.m_GfxTextureQuality ^= 1;
		CheckSettings = true;
	}

	Left.HSplitTop(3.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_CheckboxHighDetail;
	if(DoButton_CheckBox(&s_CheckboxHighDetail, Localize("High Detail"), g_Config.m_GfxHighDetail, &Button, Localize("Show map decoration elements")))
		g_Config.m_GfxHighDetail ^= 1;

	Left.HSplitTop(3.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_CheckboxLowGraphics;
	if(DoButton_CheckBox(&s_CheckboxLowGraphics, Localize("Low Graphics Mode"), g_Config.m_GfxLowGraphics, &Button, Localize("Disable fancy effects to gain more fps")))
		g_Config.m_GfxLowGraphics ^= 1;

	Left.HSplitTop(3.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_CheckboxMenuMap;
	if(DoButton_CheckBox(&s_CheckboxMenuMap, Localize("Menu Map"), g_Config.m_ClMenuBackground, &Button, Localize("Use the menu map")))
		g_Config.m_ClMenuBackground ^= 1;

	if(g_Config.m_ClMenuBackground)
	{
		CUIRect Label;
		Left.HSplitTop(3.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(10.0f, 0, &Button);
		Button.VSplitLeft(TextRender()->TextWidth(0, 12.0f, Localize("Map")), &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Map"), 12.0f, CUI::ALIGN_LEFT);

		Button.VSplitLeft(10.0f, 0, &Button);
		static char s_aEditboxContents[64] = {1};
		if(*s_aEditboxContents == 1)
			str_copyb(s_aEditboxContents, g_Config.m_ClMenuBackgroundMap);
		const bool Changed = str_comp_filenames(s_aEditboxContents, g_Config.m_ClMenuBackgroundMap) != 0;
		if(Changed)
			Button.VSplitRight(TextRender()->TextWidth(0, 12.0f, Localize("Apply")) + 2*10.0f, &Button, &Label);
		static float s_Offset = 0;
		static CButtonContainer s_EditboxMenuMapSelection;
		DoEditBox(&s_EditboxMenuMapSelection, &Button, s_aEditboxContents, sizeof(s_aEditboxContents), 12.0f, &s_Offset, false, Changed ? CUI::CORNER_L: CUI::CORNER_ALL, "ui/menu_day.map", CUI::ALIGN_LEFT,
				  "Examples:\n"
					"- ui/my_menu_map.map\n"
					"- maps/Kobra 4.map\n"
					"- downloadedmaps/Aip-Gores_4d8869b8.map");
		static CButtonContainer s_ButtonApplyMenuMap;
		if(Changed)
			if(DoButton_Menu(&s_ButtonApplyMenuMap, Localize("Apply"), 1, &Label, 0, CUI::CORNER_R))
			{
				g_Config.m_ClMenuBackground = 1;
				str_copyb(g_Config.m_ClMenuBackgroundMap, s_aEditboxContents);
			}


		Left.HSplitTop(3.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(10.0f, 0, &Button);
		Button.VSplitLeft(TextRender()->TextWidth(0, 12.0f, Localize("Distance")), &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Distance"), 12.0f, CUI::ALIGN_LEFT);
		Button.VSplitLeft(10.0f, 0, &Button);
		Button.HMargin(2.5f, &Button);
		static CButtonContainer s_ScrollbarMenuMapDistance;
		DoScrollbarIntSelect(&s_ScrollbarMenuMapDistance, &Button, &g_Config.m_ClMenuBackgroundDistance, 10, 150);

/*		Left.HSplitTop(3.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(10.0f, 0, &Button);
		Button.VSplitLeft(TextRender()->TextWidth(0, 12.0f, Localize("Pos X")), &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Pos X"), 12.0f, CUI::ALIGN_LEFT);
		Button.VSplitLeft(10.0f, 0, &Button);
		Button.HMargin(2.5f, &Button);
		static CButtonContainer s_ScrollbarMenuMapPosX;
		DoScrollbarIntSelect(&s_ScrollbarMenuMapPosX, &Button, &g_Config.m_ClMenuBackgroundPositionX, -1, 100);

		Left.HSplitTop(3.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(10.0f, 0, &Button);
		Button.VSplitLeft(TextRender()->TextWidth(0, 12.0f, Localize("Pos Y")), &Label, &Button);
		UI()->DoLabelScaled(&Label, Localize("Pos Y"), 12.0f, CUI::ALIGN_LEFT);
		Button.VSplitLeft(10.0f, 0, &Button);
		Button.HMargin(2.5f, &Button);
		static CButtonContainer s_ScrollbarMenuMapPosY;
		DoScrollbarIntSelect(&s_ScrollbarMenuMapPosY, &Button, &g_Config.m_ClMenuBackgroundPositionY, -1, 100);
*/
		Left.HSplitTop(3.0f, 0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		Button.VSplitLeft(10.0f, 0, &Button);
		static CButtonContainer s_CheckboxMenuMapRotation;
		if(DoButton_CheckBox(&s_CheckboxMenuMapRotation, Localize("Enable rotation"), g_Config.m_ClMenuBackgroundRotation, &Button, Localize("Gives the menu background a little more dynamicness")))
			g_Config.m_ClMenuBackgroundRotation ^= 1;

		if(g_Config.m_ClMenuBackgroundRotation)
		{
			Left.HSplitTop(3.0f, 0, &Left);
			Left.HSplitTop(20.0f, &Button, &Left);
			Button.VSplitLeft(2*10.0f, 0, &Button);
			Button.VSplitLeft(TextRender()->TextWidth(0, 12.0f, Localize("Rotation Delay")), &Label, &Button);
			UI()->DoLabelScaled(&Label, Localize("Rotation Delay"), 12.0f, CUI::ALIGN_LEFT);
			Button.VSplitLeft(10.0f, 0, &Button);
			Button.HMargin(2.5f, &Button);
			static CButtonContainer s_ScrollbarMenuMapRotationSpeed;
			DoScrollbarIntSelect(&s_ScrollbarMenuMapRotationSpeed, &Button, &g_Config.m_ClMenuBackgroundRotationSpeed, 1, 120);

			Left.HSplitTop(3.0f, 0, &Left);
			Left.HSplitTop(20.0f, &Button, &Left);
			Button.VSplitLeft(2*10.0f, 0, &Button);
			Button.VSplitLeft(TextRender()->TextWidth(0, 12.0f, Localize("Rotation Radius")), &Label, &Button);
			UI()->DoLabelScaled(&Label, Localize("Rotation Radius"), 12.0f, CUI::ALIGN_LEFT);
			Button.VSplitLeft(10.0f, 0, &Button);
			Button.HMargin(2.5f, &Button);
			static CButtonContainer s_ScrollbarMenuMapRotationRadius;
			DoScrollbarIntSelect(&s_ScrollbarMenuMapRotationRadius, &Button, &g_Config.m_ClMenuBackgroundRotationRadius, 1, 500);
		}
	}

	Left.HSplitTop(3.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_CheckboxLowInactiveCPU;
	if(DoButton_CheckBox(&s_CheckboxLowInactiveCPU, Localize("Low FPS when in background"), g_Config.m_ClInactiveLowCPU, &Button, Localize("Limits CPU usage while ATH is in the background")))
		g_Config.m_ClInactiveLowCPU ^= 1;

	Left.HSplitTop(3.0f, 0, &Left);
	Left.HSplitTop(20.0f, &Button, &Left);
	static CButtonContainer s_CheckboxLowConsoleCPU;
	if(DoButton_CheckBox(&s_CheckboxLowConsoleCPU, Localize("Low FPS when in console"), g_Config.m_ClConsoleLowCPU, &Button, Localize("Limits FPS while console is open in order to lower our CPU usage")))
		g_Config.m_ClConsoleLowCPU ^= 1;


	Right.HSplitTop(20.0f, &Button, &Right);
	if(DoButton_CheckBox(&s_Checkbox3, Localize("V-Sync"), g_Config.m_GfxVsync, &Button, Localize("Disable this if your game reacts too slow")))
	{
		g_Config.m_GfxVsync ^= 1;
		Client()->GfxUpdateVSync();
	}

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	int GfxFsaaSamples_MouseButton = DoButton_CheckBox_Number(&s_Checkbox5, Localize("FSAA samples"), g_Config.m_GfxFsaaSamples, &Button, Localize("Anti-Aliasing smooths graphics at the expense of FPS"));
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

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	if(DoButton_CheckBox(&s_Checkbox7, Localize("Texture Compression"), g_Config.m_GfxTextureCompression, &Button, Localize("Disable this if you get blurry textures")))
	{
		g_Config.m_GfxTextureCompression ^= 1;
		CheckSettings = true;
	}

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxBackgroundRender;
	if(DoButton_CheckBox(&s_CheckboxBackgroundRender, Localize("Render when inactive"), g_Config.m_GfxBackgroundRender, &Button, Localize("Render graphics when window is in background")))
		g_Config.m_GfxBackgroundRender ^= 1;

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxNoclip;
	if(DoButton_CheckBox(&s_CheckboxNoclip, Localize("Disable clipping"), g_Config.m_GfxNoclip, &Button, Localize("May kill any performance teeworlds could have. Be careful with it.\n~ Info for nerds: GL_SCISSOR_TEST will be disabled and thus EVERYTHING will be rendered = hard laggs.")))
		g_Config.m_GfxNoclip ^= 1;

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxQuadAsTriangle;
	if(DoButton_CheckBox(&s_CheckboxQuadAsTriangle, Localize("Render quads as triangles"), g_Config.m_GfxQuadAsTriangle, &Button, Localize("Fixes quad coloring on some GPUs, but needs more vertices")))
		g_Config.m_GfxQuadAsTriangle ^= 1;

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxFinish;
	if(DoButton_CheckBox(&s_CheckboxFinish, Localize("Wait for GL commands to finish"), g_Config.m_GfxFinish, &Button, Localize("Can cause FPS laggs if enabled\n~ Info for nerds: glFinish() blocks until the effects of all GL executions are complete.")))
		g_Config.m_GfxFinish ^= 1;

	Right.HSplitTop(3.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Button, &Right);
	static CButtonContainer s_CheckboxHighdpi;
	if(DoButton_CheckBox(&s_CheckboxHighdpi, Localize("High-DPI screen support"), g_Config.m_GfxHighdpi, &Button, Localize("Be careful: experimental")))
	{
		g_Config.m_GfxHighdpi ^= 1;
		CheckSettings = true;
	}


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

	// UI Scale
	Left.HSplitTop(20.0f, 0, &Left);

	Left.HSplitTop(20.0f+10.0f+15.0f, &Text, 0);
	Text.HMargin(-2.75f, &Text);
	Text.h += 2.75f;
	RenderTools()->DrawUIRect(&Text, vec4(0,0,0,0.2f), CUI::CORNER_ALL, 5.0f);

	{
		Left.VMargin(15.0f, &Left);
		Left.HSplitTop(20.0f, &Text, &Left);
		UI()->DoLabelScaled(&Text, Localize("UI Scale"), 14.0f, -1);
	}

	Left.HSplitTop(10.0f, 0, &Left);
	Left.HSplitTop(15.0f, &Text, &Left);
	static CButtonContainer s_ScrollbarScale;
	static int s_NewVal = g_Config.m_UiScale; // proxy it to not instantly change the ui size
	if(g_Config.m_UiScale != s_NewVal && UI()->ActiveItem() != s_ScrollbarScale.GetID()) // if it has been changed in f1
		s_NewVal = g_Config.m_UiScale;
	s_NewVal = round_to_int(50.0f+100.0f*DoScrollbarH(&s_ScrollbarScale, &Text, ((float)s_NewVal-50.0f)/100.0f, Localize("READ BEFORE CHANGING:\nIf you happen to mess it up so that this slider is not on your screen anymore, type in f1:\nui_scale 100"), s_NewVal));
	if(UI()->ActiveItem() != s_ScrollbarScale.GetID())
		g_Config.m_UiScale = s_NewVal;

	// Corner Rounding
	Left.VMargin(-15.0f, &Left);
	Left.HSplitTop(20.0f, 0, &Left);

	Left.HSplitTop(20.0f+10.0f+15.0f, &Text, 0);
	Text.HMargin(-2.75f, &Text);
	Text.h += 2.75f;
	RenderTools()->DrawUIRect(&Text, vec4(0,0,0,0.2f), CUI::CORNER_ALL, 5.0f);

	{
		Left.VMargin(15.0f, &Left);
		Left.HSplitTop(20.0f, &Text, &Left);
		UI()->DoLabelScaled(&Text, Localize("Corner Rounding"), 14.0f, -1);
	}

	Left.HSplitTop(10.0f, 0, &Left);
	Left.HSplitTop(15.0f, &Text, &Left);
	static CButtonContainer s_ScrollbarRounding;
	g_Config.m_UiCornerRoundingPercentage = round_to_int(250.0f*DoScrollbarH(&s_ScrollbarRounding, &Text, (float)g_Config.m_UiCornerRoundingPercentage/250.0f, 0, g_Config.m_UiCornerRoundingPercentage));

	// ui color
	Right.HSplitTop(20.0f-4.0f, 0, &Right);
	{
		CUIRect Rect;
		Right.HSplitTop(20+150+5+19+4 + 3, &Rect, 0);
		RenderTools()->DrawUIRect(&Rect, vec4(0,0,0,0.2f), CUI::CORNER_ALL, 5.0f);
	}
	Right.HSplitTop(4.0f, 0, &Right);
	Right.HSplitTop(20.0f, &Text, &Right);
	Text.VMargin(15.0f, &Text);
	UI()->DoLabelScaled(&Text, Localize("UI Color"), 14.0f, -1);
	Text.HMargin(2.0f, &Text);
	Text.VSplitRight(60.0f, 0, &Text);
	static CButtonContainer s_ButtonReset;
	if(g_Config.m_UiColorHue != 78 || g_Config.m_UiColorSat != 203 || g_Config.m_UiColorVal != 170 || g_Config.m_UiColorAlpha != 180) // TODO: change these values aswell when the defaults get changed!
		if(DoButton_Menu(&s_ButtonReset, Localize("Reset"), 0, &Text))
		{
			g_Config.m_UiColorHue = 78;
			g_Config.m_UiColorSat = 203;
			g_Config.m_UiColorVal = 170;
			g_Config.m_UiColorAlpha = 180;
		}

	// fancy color picker
	Right.HSplitTop(150.0f, &Button, &Right);
	static CButtonContainer s_ColorPickerA, s_ColorPickerB;
	vec3 ColorHSV = vec3(g_Config.m_UiColorHue/255.0f, g_Config.m_UiColorSat/255.0f, g_Config.m_UiColorVal/255.0f);
	if(DoColorPicker(&s_ColorPickerA, &s_ColorPickerB, &Button, &ColorHSV))
	{
		g_Config.m_UiColorHue = round_to_int(ColorHSV.h*255.0f);
		g_Config.m_UiColorSat = round_to_int(ColorHSV.s*255.0f);
		g_Config.m_UiColorVal = round_to_int(ColorHSV.v*255.0f);
	}

	Right.HSplitTop(5.0f, 0, &Right);
	{
		CUIRect Text;
		Right.HSplitTop(19.0f, &Button, &Right);
		Button.VMargin(15.0f, &Button);
		Button.VSplitRight(100.0f, &Text, &Button);
		//Button.VSplitRight(5.0f, &Button, 0);
		Button.HSplitTop(4.0f, 0, &Button);

		float k = g_Config.m_UiColorAlpha / 255.0f;
		static CButtonContainer s_Container;
		k = DoScrollbarH(&s_Container, &Button, k, 0, (int)(k * 255.0f));
		g_Config.m_UiColorAlpha = (int)(k*255.0f);
		UI()->DoLabelScaled(&Text, Localize("Alpha"), 15.0f, -1);
	}

}
