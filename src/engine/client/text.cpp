/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>
#include <engine/graphics.h>
#include <engine/textrender.h>

#ifdef CONF_FAMILY_WINDOWS
	#include <windows.h>
#endif

// ft2 texture
#include <ft2build.h>
#include <game/client/components/fontmgr.h>
#include FT_FREETYPE_H

// TODO: Refactor: clean this up
enum
{
	MAX_CHARACTERS = 64,
};


static unsigned int aFontSizes[] = {8,9,10,11,12,13,14,15,16,17,18,19,20,36,64};
#define NUM_FONT_SIZES (sizeof(aFontSizes)/sizeof(int))
#define COLOR_CODE_TAG "$$" /* don't use non-ascii for this! it's giving str_length & co. some serious problems... */
static const int COLOR_CODE_LEN = str_length(COLOR_CODE_TAG);


struct CFontChar
{
	FT_ULong m_ID;

	// these values are scaled to the pFont size
	// width * font_size == real_size
	float m_Width;
	float m_Height;
	float m_OffsetX;
	float m_OffsetY;
	float m_AdvanceX;

	float m_aUvs[4];
	int64 m_TouchTime;
};

struct CFontSizeData
{
	unsigned int m_FontSize;

	int m_aTextures[2];
	int m_TextureWidth;
	int m_TextureHeight;

	unsigned int m_NumXChars;
	unsigned int m_NumYChars;

	unsigned int m_CharMaxWidth;
	unsigned int m_CharMaxHeight;

	CFontChar m_aCharacters[MAX_CHARACTERS*MAX_CHARACTERS];

	int m_CurrentCharacter;
};

class CFont
{
public:
	char m_aFilename[512];
	FT_Face m_FtFace;
	CFontSizeData m_aSizes[NUM_FONT_SIZES];
};


class CTextRender : public IEngineTextRender
{
	IGraphics *m_pGraphics;
	IGraphics *Graphics() { return m_pGraphics; }

	struct CTextRenderSection
	{
		const char *m_pStart;
		float m_ColorR;
		float m_ColorG;
		float m_ColorB;
		bool m_OverrideColor;
		int m_Length;

		CTextRenderSection()
		{
			m_pStart = 0;
			m_Length = 0;
		}

		inline const char *GetEnd() const { return m_pStart + m_Length /*+ m_Skip*/; }

		bool operator>(const CTextRenderSection& other) const { return this->m_pStart > other.GetEnd(); }
	};

	int WordLength(const char *pText)
	{
		int s = 1;
		while(1)
		{
			if(*pText == 0)
				return s-1;
			if(*pText == '\n' || *pText == '\t' || *pText == ' ')
				return s;
			pText++;
			s++;
		}
	}

	float m_TextR;
	float m_TextG;
	float m_TextB;
	float m_TextA;

	float m_TextOutlineR;
	float m_TextOutlineG;
	float m_TextOutlineB;
	float m_TextOutlineA;

	//int m_FontTextureFormat;

	CFont *m_pDefaultFont;

	FT_Library m_FTLibrary;

	int GetFontSizeIndex(unsigned int Pixelsize)
	{
		for(unsigned i = 0; i < NUM_FONT_SIZES; i++)
		{
			if(aFontSizes[i] >= Pixelsize)
				return i;
		}

		return NUM_FONT_SIZES-1;
	}



	void Grow(unsigned char *pIn, unsigned char *pOut, int w, int h)
	{
		for(int y = 0; y < h; y++)
			for(int x = 0; x < w; x++)
			{
				unsigned char c = pIn[y*w+x];

				for(int sy = -1; sy <= 1; sy++)
					for(int sx = -1; sx <= 1; sx++)
					{
						int GetX = x+sx;
						int GetY = y+sy;
						if (GetX >= 0 && GetY >= 0 && GetX < w && GetY < h)
						{
							int Index = GetY*w+GetX;
							if(pIn[Index] > c)
								c = pIn[Index];
						}
					}

				pOut[y*w+x] = c;
			}
	}

	void InitTexture(CFontSizeData *pSizeData, unsigned int CharWidth, unsigned int CharHeight, unsigned int Xchars, unsigned int Ychars)
	{
		static int FontMemoryUsage = 0;
		unsigned int Width = CharWidth*Xchars;
		unsigned int Height = CharHeight*Ychars;
		void *pMem = mem_alloc(Width*Height, 1);
		mem_zero(pMem, Width*Height);

		for(int i = 0; i < 2; i++)
		{
			if(pSizeData->m_aTextures[i] != 0)
			{
				Graphics()->UnloadTexture(pSizeData->m_aTextures[i]);
				FontMemoryUsage -= pSizeData->m_TextureWidth*pSizeData->m_TextureHeight;
				pSizeData->m_aTextures[i] = 0;
			}

			pSizeData->m_aTextures[i] = Graphics()->LoadTextureRaw(Width, Height, CImageInfo::FORMAT_ALPHA, pMem, CImageInfo::FORMAT_ALPHA, IGraphics::TEXLOAD_NOMIPMAPS);
			FontMemoryUsage += Width*Height;
		}

		pSizeData->m_NumXChars = Xchars;
		pSizeData->m_NumYChars = Ychars;
		pSizeData->m_TextureWidth = Width;
		pSizeData->m_TextureHeight = Height;
		pSizeData->m_CurrentCharacter = 0;

		dbg_msg("text", "font data memory usage: %d byte (%.2f KiB)", FontMemoryUsage, (float)FontMemoryUsage/1024.0f);

		mem_free(pMem);
	}

	int AdjustOutlineThicknessToFontSize(int OutlineThickness, unsigned int FontSize)
	{
		if(FontSize > 36)
			OutlineThickness *= 4;
		else if(FontSize >= 18)
			OutlineThickness *= 2;
		return OutlineThickness;
	}

	void IncreaseTextureSize(CFontSizeData *pSizeData)
	{
		if(pSizeData->m_TextureWidth < pSizeData->m_TextureHeight)
			pSizeData->m_NumXChars <<= 1;
		else
			pSizeData->m_NumYChars <<= 1;
		InitTexture(pSizeData, pSizeData->m_CharMaxWidth, pSizeData->m_CharMaxHeight, pSizeData->m_NumXChars, pSizeData->m_NumYChars);
	}


	// TODO: Refactor: move this into a pFont class
	void InitIndex(CFont *pFont, int Index)
	{
		CFontSizeData *pSizeData = &pFont->m_aSizes[Index];

		pSizeData->m_FontSize = aFontSizes[Index];
		FT_Set_Pixel_Sizes(pFont->m_FtFace, 0, pSizeData->m_FontSize);

		int OutlineThickness = AdjustOutlineThicknessToFontSize(1, pSizeData->m_FontSize);

		{
			unsigned GlyphIndex;
			FT_Pos MaxH = 0;
			FT_Pos MaxW = 0;

			FT_ULong Charcode = FT_Get_First_Char(pFont->m_FtFace, &GlyphIndex);
			while(GlyphIndex != 0)
			{
				// do stuff
				FT_Load_Glyph(pFont->m_FtFace, GlyphIndex, FT_LOAD_DEFAULT);

				if(pFont->m_FtFace->glyph->metrics.width > MaxW) MaxW = pFont->m_FtFace->glyph->metrics.width; // ignore_convention
				if(pFont->m_FtFace->glyph->metrics.height > MaxH) MaxH = pFont->m_FtFace->glyph->metrics.height; // ignore_convention
				Charcode = FT_Get_Next_Char(pFont->m_FtFace, Charcode, &GlyphIndex);
			}

			MaxW = (MaxW>>6)+2+OutlineThickness*2;
			MaxH = (MaxH>>6)+2+OutlineThickness*2;

			for(pSizeData->m_CharMaxWidth = 1; pSizeData->m_CharMaxWidth < (unsigned int)MaxW; pSizeData->m_CharMaxWidth <<= 1);
			for(pSizeData->m_CharMaxHeight = 1; pSizeData->m_CharMaxHeight < (unsigned int)MaxH; pSizeData->m_CharMaxHeight <<= 1);
		}

		//dbg_msg("pFont", "init size %d, texture size %d %d", pFont->sizes[index].font_size, w, h);
		//FT_New_Face(m_FTLibrary, "data/fonts/vera.ttf", 0, &pFont->ft_face);
		InitTexture(pSizeData, pSizeData->m_CharMaxWidth, pSizeData->m_CharMaxHeight, 8, 8);
	}

	CFontSizeData *GetSize(CFont *pFont, unsigned int Pixelsize)
	{
		int Index = GetFontSizeIndex(Pixelsize);
		if(pFont->m_aSizes[Index].m_FontSize != aFontSizes[Index])
			InitIndex(pFont, Index);
		return &pFont->m_aSizes[Index];
	}


	void UploadGlyph(CFontSizeData *pSizeData, int Texnum, int SlotID, FT_ULong Chr, const void *pData)
	{
		int x = (SlotID%pSizeData->m_NumXChars) * (pSizeData->m_TextureWidth/pSizeData->m_NumXChars);
		int y = (SlotID/pSizeData->m_NumXChars) * (pSizeData->m_TextureHeight/pSizeData->m_NumYChars);

		Graphics()->LoadTextureRawSub(pSizeData->m_aTextures[Texnum], x, y,
			pSizeData->m_TextureWidth/pSizeData->m_NumXChars,
			pSizeData->m_TextureHeight/pSizeData->m_NumYChars,
			CImageInfo::FORMAT_ALPHA, pData);
		/*
		glBindTexture(GL_TEXTURE_2D, pSizeData->m_aTextures[Texnum]);
		glTexSubImage2D(GL_TEXTURE_2D, 0, x, y,
			pSizeData->m_TextureWidth/pSizeData->m_NumXChars,
			pSizeData->m_TextureHeight/pSizeData->m_NumYChars,
			m_FontTextureFormat, GL_UNSIGNED_BYTE, pData);*/
	}

	// 32k of data used for rendering glyphs
	unsigned char ms_aGlyphData[(1024/8) * (1024/8)];
	unsigned char ms_aGlyphDataOutlined[(1024/8) * (1024/8)];

	int GetSlot(CFontSizeData *pSizeData)
	{
		int CharCount = pSizeData->m_NumXChars*pSizeData->m_NumYChars;
		if(pSizeData->m_CurrentCharacter < CharCount)
		{
			int i = pSizeData->m_CurrentCharacter;
			pSizeData->m_CurrentCharacter++;
			return i;
		}

		// kick out the oldest
		// TODO: remove this linear search
		{
			int Oldest = 0;
			for(int i = 1; i < CharCount; i++)
			{
				if(pSizeData->m_aCharacters[i].m_TouchTime < pSizeData->m_aCharacters[Oldest].m_TouchTime)
					Oldest = i;
			}

			if(time_get()-pSizeData->m_aCharacters[Oldest].m_TouchTime < time_freq() &&
				(pSizeData->m_NumXChars < MAX_CHARACTERS || pSizeData->m_NumYChars < MAX_CHARACTERS))
			{
				IncreaseTextureSize(pSizeData);
				return GetSlot(pSizeData);
			}

			return Oldest;
		}
	}

	int RenderGlyph(CFont *pFont, CFontSizeData *pSizeData, FT_ULong Chr)
	{
		FT_Bitmap *pBitmap;
		int SlotID = 0;
		unsigned int SlotW = pSizeData->m_TextureWidth / pSizeData->m_NumXChars;
		unsigned int SlotH = pSizeData->m_TextureHeight / pSizeData->m_NumYChars;
		unsigned int SlotSize = SlotW*SlotH;
		int x = 1;
		int y = 1;
		unsigned int px, py;

		FT_Set_Pixel_Sizes(pFont->m_FtFace, 0, pSizeData->m_FontSize);

		if(FT_Load_Char(pFont->m_FtFace, Chr, FT_LOAD_RENDER|FT_LOAD_NO_BITMAP))
		{
			dbg_msg("pFont", "error loading glyph %lu", Chr);
			return -1;
		}

		pBitmap = &pFont->m_FtFace->glyph->bitmap; // ignore_convention

		// fetch slot
		SlotID = GetSlot(pSizeData);
		if(SlotID < 0)
			return -1;

		// adjust spacing
		int OutlineThickness = AdjustOutlineThicknessToFontSize(1, pSizeData->m_FontSize);
		x += OutlineThickness;
		y += OutlineThickness;

		// prepare glyph data
		mem_zero(ms_aGlyphData, SlotSize);

		if(pBitmap->pixel_mode == FT_PIXEL_MODE_GRAY) // ignore_convention
		{
			for(py = 0; py < (unsigned)pBitmap->rows; py++) // ignore_convention
				for(px = 0; px < (unsigned)pBitmap->width; px++) // ignore_convention
					ms_aGlyphData[(py+y)*SlotW+px+x] = pBitmap->buffer[py*pBitmap->pitch+px]; // ignore_convention
		}
		else if(pBitmap->pixel_mode == FT_PIXEL_MODE_MONO) // ignore_convention
		{
			for(py = 0; py < (unsigned)pBitmap->rows; py++) // ignore_convention
				for(px = 0; px < (unsigned)pBitmap->width; px++) // ignore_convention
				{
					if(pBitmap->buffer[py*pBitmap->pitch+px/8]&(1<<(7-(px%8)))) // ignore_convention
						ms_aGlyphData[(py+y)*SlotW+px+x] = 255;
				}
		}

//		for(py = 0; (int)py < SlotW; py++)
//			for(px = 0; (int)px < SlotH; px++)
//				ms_aGlyphData[py*SlotW+px] = 255;

		// upload the glyph
		UploadGlyph(pSizeData, 0, SlotID, Chr, ms_aGlyphData);

		if(OutlineThickness == 1)
		{
			Grow(ms_aGlyphData, ms_aGlyphDataOutlined, SlotW, SlotH);
			UploadGlyph(pSizeData, 1, SlotID, Chr, ms_aGlyphDataOutlined);
		}
		else
		{
			for(int i = OutlineThickness; i > 0; i-=2)
			{
				Grow(ms_aGlyphData, ms_aGlyphDataOutlined, SlotW, SlotH);
				Grow(ms_aGlyphDataOutlined, ms_aGlyphData, SlotW, SlotH);
			}
			UploadGlyph(pSizeData, 1, SlotID, Chr, ms_aGlyphData);
		}

		// set char info
		{
			CFontChar *pFontchr = &pSizeData->m_aCharacters[SlotID];
			float Scale = 1.0f/pSizeData->m_FontSize;
			float Uscale = 1.0f/pSizeData->m_TextureWidth;
			float Vscale = 1.0f/pSizeData->m_TextureHeight;
			int Height = pBitmap->rows + OutlineThickness*2 + 2; // ignore_convention
			int Width = pBitmap->width + OutlineThickness*2 + 2; // ignore_convention

			pFontchr->m_ID = Chr;
			pFontchr->m_Height = Height * Scale;
			pFontchr->m_Width = Width * Scale;
			pFontchr->m_OffsetX = (pFont->m_FtFace->glyph->bitmap_left-1) * Scale; // ignore_convention
			pFontchr->m_OffsetY = (pSizeData->m_FontSize - pFont->m_FtFace->glyph->bitmap_top) * Scale; // ignore_convention
			pFontchr->m_AdvanceX = (pFont->m_FtFace->glyph->advance.x>>6) * Scale; // ignore_convention

			pFontchr->m_aUvs[0] = (SlotID%pSizeData->m_NumXChars) / (float)(pSizeData->m_NumXChars);
			pFontchr->m_aUvs[1] = (SlotID/pSizeData->m_NumXChars) / (float)(pSizeData->m_NumYChars);
			pFontchr->m_aUvs[2] = pFontchr->m_aUvs[0] + Width*Uscale;
			pFontchr->m_aUvs[3] = pFontchr->m_aUvs[1] + Height*Vscale;
		}

		return SlotID;
	}

	const CFontChar *GetChar(CFont *pFont, CFontSizeData *pSizeData, FT_ULong Chr)
	{
		CFontChar *pFontchr = NULL;

		// search for the character
		// TODO: remove this linear search
		for(int i = 0; i < pSizeData->m_CurrentCharacter; i++)
		{
			if(pSizeData->m_aCharacters[i].m_ID == Chr)
			{
				pFontchr = &pSizeData->m_aCharacters[i];
				break;
			}
		}

		// check if we need to render the character
		if(!pFontchr)
		{
			int Index = RenderGlyph(pFont, pSizeData, Chr);
			if(Index >= 0)
				pFontchr = &pSizeData->m_aCharacters[Index];
		}

		// touch the character
		// TODO: don't call time_get here || shouldn't be too bad at all since caching is used...
		if(pFontchr)
			pFontchr->m_TouchTime = time_get();

		return pFontchr;
	}

	// must only be called from the rendering function as the pFont must be set to the correct size
	void RenderSetup(CFont *pFont, FT_UInt size)
	{
		FT_Set_Pixel_Sizes(pFont->m_FtFace, 0, size);
	}

	float Kerning(CFont *pFont, FT_UInt Left, FT_UInt Right)
	{
		FT_Vector Kerning = {0,0};
		FT_Get_Kerning(pFont->m_FtFace, Left, Right, FT_KERNING_DEFAULT, &Kerning);
		return (Kerning.x>>6);
	}

	bool IsValidColorCode(const char *pText)
	{
		return str_comp_num(pText, COLOR_CODE_TAG, COLOR_CODE_LEN) == 0 && (
				(pText[COLOR_CODE_LEN+0] >= '0' && pText[COLOR_CODE_LEN+0] <= '9') ||
				(pText[COLOR_CODE_LEN+0] >= 'A' && pText[COLOR_CODE_LEN+0] <= 'Z') ||
				(pText[COLOR_CODE_LEN+0] >= 'a' && pText[COLOR_CODE_LEN+0] <= 'z') )
				&& (
				(pText[COLOR_CODE_LEN+1] >= '0' && pText[COLOR_CODE_LEN+1] <= '9') ||
				(pText[COLOR_CODE_LEN+1] >= 'A' && pText[COLOR_CODE_LEN+1] <= 'Z') ||
				(pText[COLOR_CODE_LEN+1] >= 'a' && pText[COLOR_CODE_LEN+1] <= 'z') )
				&& (
				(pText[COLOR_CODE_LEN+2] >= '0' && pText[COLOR_CODE_LEN+2] <= '9') ||
				(pText[COLOR_CODE_LEN+2] >= 'A' && pText[COLOR_CODE_LEN+2] <= 'Z') ||
				(pText[COLOR_CODE_LEN+2] >= 'a' && pText[COLOR_CODE_LEN+2] <= 'z') );
	}

	bool ProcessStringPart(const CTextCursor *pCursor, const char *pStr, const char *pHighlight, CTextRenderSection *pOut, bool NoColorCodes)
	{
		if(!pStr || *pStr == '\0')
			return false;

		const char *pNextInterestingSectionStart = NULL;

		// check for url
		if(pCursor->m_Flags&TEXTFLAG_RENDER)
		{
			const char *pUrlStart = NULL;
			const char *pUrlStartHttp = str_find_nocase(pStr, "http://");
			const char *pUrlStartHttps = str_find_nocase(pStr, "https://");
			if(pUrlStartHttp == NULL) // if A is null, ..
				pUrlStart = pUrlStartHttps; // ..take B
			else if(pUrlStartHttps == NULL) // if A is present but B is null, ..
				pUrlStart = pUrlStartHttp; // ..take A
			else // if both are present, ..
				pUrlStart = min(pUrlStartHttp, pUrlStartHttps); // ..take the left one

			// check if we are right on the url
			if(pUrlStart > pStr)
				pNextInterestingSectionStart = pUrlStart;
			else if(pUrlStart == pStr) // only render it from its start, thus when we are right on it
			{
				// find the end of the url
				const char *pUrlEnd = NULL;
				const char *pUrlEndSpace = str_find_nocase(pUrlStart, " "); // pUrlEnd points to the first character *not* in the url
				const char *pUrlEndColor = str_find_nocase(pUrlStart, "$$"); // pUrlEnd points to the first character *not* in the url
				if(pUrlEndSpace == NULL)
					pUrlEnd = pUrlEndColor;
				else if(pUrlEndColor == NULL)
					pUrlEnd = pUrlEndSpace;
				else
					pUrlEnd = min(pUrlEndSpace, pUrlEndColor);
				if(!pUrlEnd) pUrlEnd = pUrlStart + str_length(pUrlStart);
				int UrlLen = (int)(pUrlEnd-pUrlStart);
				pOut->m_pStart = pUrlStart;
				pOut->m_Length = UrlLen;
				pOut->m_OverrideColor = true;

				HandleURL(pCursor, pOut);

				return true;
			}
		}

		// check for highlight
		if((pCursor->m_Flags&TEXTFLAG_RENDER) && pHighlight && pHighlight[0] != '\0')
		{
			const char *pHightlightFound = str_find_nocase(pStr, pHighlight);
			if(!pNextInterestingSectionStart && pHightlightFound > pStr)
				pNextInterestingSectionStart = pHightlightFound;
			else if(pHightlightFound == pStr) // only render it from its start, thus when we are right on it
			{
				pOut->m_pStart = pHightlightFound;
				pOut->m_ColorR = 0.4f;
				pOut->m_ColorG = 0.4f;
				pOut->m_ColorB = 1.0f;
				pOut->m_Length = str_length(pHighlight);
				pOut->m_OverrideColor = true;

				return true;
			}
		}

		// find color code, e.g. $$AJ9hey there
		if(!NoColorCodes)
		{
			// check how far we are allowed to render
			const char *pNextCandidate = str_find(pStr+/*COLOR_CODE_LEN+3*/1, COLOR_CODE_TAG);
			if(pNextCandidate && (!pNextInterestingSectionStart || pNextCandidate < pNextInterestingSectionStart) && IsValidColorCode(pNextCandidate))
				pNextInterestingSectionStart = pNextCandidate; // do not overwrite a predicted highlight

			if(IsValidColorCode(pStr))
			{
				char aHackyBuf[2] = { pStr[COLOR_CODE_LEN+0], '\0'};
				float Red = str_toint_base(aHackyBuf, 36) / 35.0f;

				aHackyBuf[0] = pStr[COLOR_CODE_LEN+1];
				float Green = str_toint_base(aHackyBuf, 36) / 35.0f;

				aHackyBuf[0] = pStr[COLOR_CODE_LEN+2];
				float Blue = str_toint_base(aHackyBuf, 36) / 35.0f;

				const char *pStrBegin = pStr+COLOR_CODE_LEN+3;

				// color code at string end? Don't render it.
				if(*pStrBegin == '\0')
					return false; // no need to process the string once more

				int MaxLen = pNextInterestingSectionStart ? (int)(pNextInterestingSectionStart-pStrBegin) : str_length(pStrBegin);
				if(MaxLen < 0)
				{
					/* in this case the next interesting section (most likely highlight) begins within our color tag,
					 * thus the color tag should be ignored.
					 * instead, just hand over to the metioned section.
					 */
					return ProcessStringPart(pCursor, pNextInterestingSectionStart, pHighlight, pOut, NoColorCodes);
				}

				pOut->m_Length = MaxLen;
				pOut->m_pStart = pStrBegin;
				pOut->m_ColorR = Red;
				pOut->m_ColorG = Green;
				pOut->m_ColorB = Blue;
				pOut->m_OverrideColor = true;

				return true;
			}
		}

		pOut->m_OverrideColor = false;
		pOut->m_pStart = pStr;
		pOut->m_Length = pNextInterestingSectionStart ? (int)(pNextInterestingSectionStart-pStr) : str_length(pStr); // don't render into the next section

		return true;
	}

	void HandleURL(const CTextCursor *pCursor, CTextRenderSection *pOut)
	{
		float tw = TextWidth(pCursor->m_pFont, pCursor->m_FontSize, pOut->m_pStart, pOut->m_Length);
		IInput *pInput = Kernel()->RequestInterface<IInput>();

		float x1 = pCursor->m_X;
		float y1 = pCursor->m_Y;
		float x2 = x1+tw;
		float y2 = y1+pCursor->m_FontSize;

		int mx, my;
		pInput->CurrentMousePos(&mx, &my);
		if(!pInput->InputGrabbed()) // if using the native mouse
		{
			my -= 5.0f; // magic correction

			// re-translate the coordinates to the mapped screen
			float MappedW, MappedH;
			Graphics()->GetScreen(NULL, NULL, &MappedW, &MappedH);
			float TrueW, TrueH;
			TrueW = Graphics()->ScreenWidth();
			TrueH = Graphics()->ScreenHeight();
			mx *= MappedW / TrueW;
			my *= MappedH / TrueH;
		}

		bool DoubleClicked = pInput->MouseDoubleClickCurrent() != 0;

		if(mx >= x1 && mx <= x2 && my >= y1 && my <= y2)
		{
			if(pInput->NativeMousePressed(1))
			{
				// mouse clicked on the link
				pOut->m_ColorR = 0.0f;
				pOut->m_ColorG = 1.0f;
				pOut->m_ColorB = 0.39f;
			}
			else
			{
				// mouse pointing at the link
				pOut->m_ColorR = 0.8f;
				pOut->m_ColorG = 0.0f;
				pOut->m_ColorB = 0.39f;
			}

			if(DoubleClicked)
			{
				pInput->MouseDoubleClickCurrentReset();
				open_default_browser(pOut->m_pStart);
			}

		}
		else
		{
			// mouse leaving the link alone
			pOut->m_ColorR = 0.15f;
			pOut->m_ColorG = 0.48f;
			pOut->m_ColorB = 0.87f;
		}
	}


public:
	CTextRender()
	{
		m_pGraphics = 0;

		m_TextR = 1.0f;
		m_TextG = 1.0f;
		m_TextB = 1.0f;
		m_TextA = 1.0f;
		m_TextOutlineR = 0.0f;
		m_TextOutlineG = 0.0f;
		m_TextOutlineB = 0.0f;
		m_TextOutlineA = 0.3f;

		m_pDefaultFont = 0;

		// GL_LUMINANCE can be good for debugging
		//m_FontTextureFormat = GL_ALPHA;
	}

	virtual void Init()
	{
		m_pGraphics = Kernel()->RequestInterface<IGraphics>();
		FT_Init_FreeType(&m_FTLibrary);
	}


	virtual CFont *LoadFont(const char *pFilename)
	{
		CFont *pFont = (CFont *)mem_alloc(sizeof(CFont), 1);

		mem_zero(pFont, sizeof(*pFont));
		str_copy(pFont->m_aFilename, pFilename, sizeof(pFont->m_aFilename));

		if(FT_New_Face(m_FTLibrary, pFont->m_aFilename, 0, &pFont->m_FtFace))
		{
			dbg_msg("engine/font", "failed to load '%s'", pFont->m_aFilename);
			mem_free(pFont);
			return NULL;
		}

		for(unsigned i = 0; i < NUM_FONT_SIZES; i++)
			pFont->m_aSizes[i].m_FontSize = 0;

		dbg_msg("textrender", "loaded pFont from '%s'", pFilename);
		return pFont;
	};

	virtual void DestroyFont(CFont *pFont)
	{
		mem_free(pFont);
	}

	virtual void SetDefaultFont(CFont *pFont)
	{
		dbg_msg("textrender", "default pFont set %p", pFont);
		m_pDefaultFont = pFont;
	}


	virtual void SetCursor(CTextCursor *pCursor, float x, float y, float FontSize, int Flags, CFont *pFont = 0)
	{
		mem_zero(pCursor, sizeof(*pCursor));
		pCursor->m_FontSize = FontSize;
		pCursor->m_StartX = x;
		pCursor->m_StartY = y;
		pCursor->m_X = x;
		pCursor->m_Y = y;
		pCursor->m_LineCount = 1;
		pCursor->m_LineWidth = -1;
		pCursor->m_CharCount = 0;
		pCursor->m_Flags = Flags;
		pCursor->m_pFont = pFont;
	}


	virtual void Text(CFont *pFontSetV, float x, float y, float Size, const char *pText, float MaxWidth)
	{
		CTextCursor Cursor;
		SetCursor(&Cursor, x, y, Size, TEXTFLAG_RENDER, pFontSetV);
		Cursor.m_LineWidth = MaxWidth;
		TextEx(&Cursor, pText, -1);
	}

	virtual float TextWidth(CFont *pFontSetV, float Size, const char *pText, int Length = -1, float LineWidth = -1)
	{
		CTextCursor Cursor;
		SetCursor(&Cursor, 0, 0, Size, 0, pFontSetV);
		if(LineWidth > 0)
			Cursor.m_LineWidth = LineWidth;
		return TextEx(&Cursor, pText, Length);
	}

	virtual float TextWidthParse(CFont *pFontSetV, float Size, const char *pText, float LineWidth, const char *pHightlight)
	{
		CTextCursor Cursor;
		SetCursor(&Cursor, 0, 0, Size, 0, pFontSetV);
		if(LineWidth > 0)
			Cursor.m_LineWidth = LineWidth;
		TextExParse(&Cursor, pText, false, pHightlight);
		return Cursor.m_X;
	}

	virtual int TextLineCount(CFont *pFontSetV, float Size, const char *pText, float LineWidth)
	{
		CTextCursor Cursor;
		SetCursor(&Cursor, 0, 0, Size, 0, pFontSetV);
		Cursor.m_LineWidth = LineWidth;
		TextEx(&Cursor, pText, -1);
		return Cursor.m_LineCount;
	}

	virtual void TextColor(float r, float g, float b, float a)
	{
		m_TextR = r;
		m_TextG = g;
		m_TextB = b;
		m_TextA = a;
	}

	virtual void GetTextColor(float *r, float *g, float *b, float *a)
	{
		if(r) *r = m_TextR;
		if(g) *g = m_TextG;
		if(b) *b = m_TextB;
		if(a) *a = m_TextA;
	}

	virtual void TextOutlineColor(float r, float g, float b, float a)
	{
		m_TextOutlineR = r;
		m_TextOutlineG = g;
		m_TextOutlineB = b;
		m_TextOutlineA = a;
	}

	virtual float TextExParse(CTextCursor *pCursor, const char *pText, bool IgnoreColorCodes = false, const char *pHighlight = 0)
	{
		CTextRenderSection Section;
		const vec4 PrevColor(m_TextR, m_TextG, m_TextB, m_TextA);
		float ret = 0.0f;
		while(ProcessStringPart(pCursor, pText, pHighlight, &Section, IgnoreColorCodes))
		{
			if(Section.m_OverrideColor)
				TextColor(Section.m_ColorR, Section.m_ColorG, Section.m_ColorB, PrevColor.a);
			else
				TextColor(PrevColor.r, PrevColor.g, PrevColor.b, PrevColor.a);
			ret += TextEx(pCursor, Section.m_pStart, Section.m_Length);

			pText = Section.GetEnd();
		}
		TextColor(PrevColor.r, PrevColor.g, PrevColor.b, PrevColor.a);

		return ret;
	}

	virtual float TextEx(CTextCursor *pCursor, const char *pText, int Length)
	{
		if(!pText)
			return 0.0f;

		if(Length == 0)
			return 0.0f;

		if(str_length(pText) == 0)
			return 0.0f;

		CFont *pFont = NULL;
		if(pCursor->m_pFont)
		{
			pFont = pCursor->m_pFont;
			if(!pFont)
				pFont = pCursor->m_pFont;
		}
		CFontSizeData *pSizeData = NULL;

		//dbg_msg("textrender", "rendering text '%s'", text);

		float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
		float FakeToScreenX, FakeToScreenY;
		int ActualX, ActualY;

		FT_UInt ActualSize;
		int i;
		int GotNewLine = 0;
		float DrawX = 0.0f, DrawY = 0.0f;
		int LineCount = 0;
		float CursorX, CursorY;

		float Size = pCursor->m_FontSize;

		// to correct coords, convert to screen coords, round, and convert back
		Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);

		FakeToScreenX = (((float)Graphics()->ScreenWidth())/(ScreenX1-ScreenX0));
		FakeToScreenY = (((float)Graphics()->ScreenHeight())/(ScreenY1-ScreenY0));
		ActualX = (int)(pCursor->m_X * FakeToScreenX);
		ActualY = (int)(pCursor->m_Y * FakeToScreenY);

		CursorX = ((float)ActualX) / FakeToScreenX;
		CursorY = ((float)ActualY) / FakeToScreenY;

		// same with size
		ActualSize = (unsigned int)round_to_int(Size * FakeToScreenY);
		Size = ActualSize / FakeToScreenY;

		// fetch pFont data
		if(!pFont)
			pFont = m_pDefaultFont;

		if(!pFont)
			return 0.0f;

		pSizeData = GetSize(pFont, ActualSize);
		RenderSetup(pFont, ActualSize);

		float Scale = 1.0f/(float)pSizeData->m_FontSize;

		// set length
		if(Length < 0)
			Length = str_length(pText);

		float MaxLineWidth = 0.0f;

		// if we don't want to render, we can just skip the first outline pass
		i = 1;
		if(pCursor->m_Flags&TEXTFLAG_RENDER)
			i = 0;
		for(;i < 2; i++)
		{
			const char *pCurrent = (char *)pText;
			const char *pEnd = pCurrent+Length;
			DrawX = CursorX;
			DrawY = CursorY;
			LineCount = pCursor->m_LineCount;

			if(pCursor->m_Flags&TEXTFLAG_RENDER)
			{
				// TODO: Make this better
				if (i == 0)
					Graphics()->TextureSet(pSizeData->m_aTextures[1]);
				else
					Graphics()->TextureSet(pSizeData->m_aTextures[0]);

				Graphics()->QuadsBegin();
				if (i == 0)
					Graphics()->SetColor(m_TextOutlineR, m_TextOutlineG, m_TextOutlineB, m_TextOutlineA*m_TextA);
				else
					Graphics()->SetColor(m_TextR, m_TextG, m_TextB, m_TextA);
			}

			int debug = 1; // TODO: XXX: come up with a better solution against softlocking!
			while(pCurrent < pEnd && (pCursor->m_MaxLines < 1 || LineCount <= pCursor->m_MaxLines))
			{
				if(pCursor->m_MaxLines > 0 && LineCount > pCursor->m_MaxLines)
					break;

				int NewLine = 0;
				const char *pBatchEnd = pEnd;
				if(pCursor->m_LineWidth > 0 && !(pCursor->m_Flags&TEXTFLAG_STOP_AT_END))
				{
					int Wlen = min(WordLength((char *)pCurrent), (int)(pEnd-pCurrent));
					CTextCursor Compare = *pCursor;
					Compare.m_X = DrawX;
					Compare.m_Y = DrawY;
					Compare.m_Flags &= ~TEXTFLAG_RENDER;
					Compare.m_LineWidth = -1;
					TextEx(&Compare, pCurrent, Wlen);

					if(Compare.m_X-DrawX > pCursor->m_LineWidth)
					{
						// word can't be fitted in one line, cut it
						CTextCursor Cutter = *pCursor;
						Cutter.m_CharCount = 0;
						Cutter.m_X = DrawX;
						Cutter.m_Y = DrawY;
						Cutter.m_Flags &= ~TEXTFLAG_RENDER;
						Cutter.m_Flags |= TEXTFLAG_STOP_AT_END;

						TextEx(&Cutter, pCurrent, Wlen);
						Wlen = Cutter.m_CharCount;
						NewLine = 1;

						if(Wlen <= 3) // if we can't place 3 chars of the word on this line, take the next
							Wlen = 0;
					}
					else if(Compare.m_X-pCursor->m_StartX > pCursor->m_LineWidth)
					{
						NewLine = 1;
						Wlen = 0;
					}

					pBatchEnd = pCurrent + Wlen;
				}

				const char *pTmp = pCurrent;
				FT_UInt NextCharacter = (FT_UInt)str_utf8_decode(&pTmp);
				while(pCurrent < pBatchEnd)
				{
					debug++;

					FT_UInt Character = NextCharacter;
					pCurrent = pTmp;
					NextCharacter = (FT_UInt)str_utf8_decode(&pTmp);

					if(Character == '\n')
					{
						DrawX = pCursor->m_StartX;
						DrawY += Size;
						DrawX = (int)(DrawX * FakeToScreenX) / FakeToScreenX; // realign
						DrawY = (int)(DrawY * FakeToScreenY) / FakeToScreenY;
						MaxLineWidth = max(MaxLineWidth, DrawX);
						++LineCount;
						if(pCursor->m_MaxLines > 0 && LineCount > pCursor->m_MaxLines)
							break;
						continue;
					}

					const CFontChar *pChr = GetChar(pFont, pSizeData, Character);
					if(pChr)
					{
						float Advance = pChr->m_AdvanceX + Kerning(pFont, Character, NextCharacter)*Scale;
						if(pCursor->m_Flags&TEXTFLAG_STOP_AT_END && DrawX+Advance*Size-pCursor->m_StartX > pCursor->m_LineWidth)
						{
							// we hit the end of the line, no more to render or count
							pCurrent = pEnd;
							break;
						}

						if(pCursor->m_Flags&TEXTFLAG_RENDER)
						{
							Graphics()->QuadsSetSubset(pChr->m_aUvs[0], pChr->m_aUvs[1], pChr->m_aUvs[2], pChr->m_aUvs[3]);
							IGraphics::CQuadItem QuadItem(DrawX+pChr->m_OffsetX*Size, DrawY+pChr->m_OffsetY*Size, pChr->m_Width*Size, pChr->m_Height*Size);
							Graphics()->QuadsDrawTL(&QuadItem, 1);
						}

						DrawX += Advance*Size;
						pCursor->m_CharCount++;
						MaxLineWidth = max(MaxLineWidth, DrawX);
					}
				}

				if (pBatchEnd == pCurrent && debug == 1)
					break;

				if(NewLine)
				{
					DrawX = pCursor->m_StartX;
					DrawY += Size;
					GotNewLine = 1;
					DrawX = (int)(DrawX * FakeToScreenX) / FakeToScreenX; // realign
					DrawY = (int)(DrawY * FakeToScreenY) / FakeToScreenY;
					++LineCount;
					MaxLineWidth = max(MaxLineWidth, DrawX);
				}

				debug++;
			}

			if(pCursor->m_Flags&TEXTFLAG_RENDER)
				Graphics()->QuadsEnd();
		}

		pCursor->m_X = DrawX;
		pCursor->m_LineCount = LineCount;
		if(GotNewLine)
			pCursor->m_Y = DrawY;

		return MaxLineWidth;
	}

};

#undef COLOR_CODE_TAG


IEngineTextRender *CreateEngineTextRender() { return new CTextRender; }
