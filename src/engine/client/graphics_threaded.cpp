	/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */

#include <base/detect.h>
#include <base/math.h>
#include <base/tl/threading.h>

#if defined(CONF_FAMILY_UNIX)
#include <pthread.h>
#endif

#include <base/system.h>
#include <engine/external/pnglite/pnglite.h>

#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/keys.h>
#include <engine/console.h>

#include <math.h> // cosf, sinf

// lua
#include <lua.hpp>
#include "graphics_threaded.h"
#include "luabinding.h"

static CVideoMode g_aFakeModes[] = {
	{320,240,8,8,8}, {400,300,8,8,8}, {640,480,8,8,8},
	{720,400,8,8,8}, {768,576,8,8,8}, {800,600,8,8,8},
	{1024,600,8,8,8}, {1024,768,8,8,8}, {1152,864,8,8,8},
	{1280,768,8,8,8}, {1280,800,8,8,8}, {1280,960,8,8,8},
	{1280,1024,8,8,8}, {1368,768,8,8,8}, {1400,1050,8,8,8},
	{1440,900,8,8,8}, {1440,1050,8,8,8}, {1600,1000,8,8,8},
	{1600,1200,8,8,8}, {1680,1050,8,8,8}, {1792,1344,8,8,8},
	{1800,1440,8,8,8}, {1856,1392,8,8,8}, {1920,1080,8,8,8},
	{1920,1200,8,8,8}, {1920,1440,8,8,8}, {1920,2400,8,8,8},
	{2048,1536,8,8,8},

	{320,240,5,6,5}, {400,300,5,6,5}, {640,480,5,6,5},
	{720,400,5,6,5}, {768,576,5,6,5}, {800,600,5,6,5},
	{1024,600,5,6,5}, {1024,768,5,6,5}, {1152,864,5,6,5},
	{1280,768,5,6,5}, {1280,800,5,6,5}, {1280,960,5,6,5},
	{1280,1024,5,6,5}, {1368,768,5,6,5}, {1400,1050,5,6,5},
	{1440,900,5,6,5}, {1440,1050,5,6,5}, {1600,1000,5,6,5},
	{1600,1200,5,6,5}, {1680,1050,5,6,5}, {1792,1344,5,6,5},
	{1800,1440,5,6,5}, {1856,1392,5,6,5}, {1920,1080,5,6,5},
	{1920,1200,5,6,5}, {1920,1440,5,6,5}, {1920,2400,5,6,5},
	{2048,1536,5,6,5}
};

void CGraphics_Threaded::FlushVertices()
{
	if(m_NumVertices == 0)
		return;

	int NumVerts = m_NumVertices;
	m_NumVertices = 0;

	CCommandBuffer::SCommand_Render Cmd;
	Cmd.m_State = m_State;

	if(m_Drawing == DRAWING_QUADS)
	{
		if(g_Config.m_GfxQuadAsTriangle)
		{
			Cmd.m_PrimType = CCommandBuffer::PRIMTYPE_TRIANGLES;
			Cmd.m_PrimCount = NumVerts/3;
		}
		else
		{
			Cmd.m_PrimType = CCommandBuffer::PRIMTYPE_QUADS;
			Cmd.m_PrimCount = NumVerts/4;
		}
	}
	else if(m_Drawing == DRAWING_LINES)
	{
		Cmd.m_PrimType = CCommandBuffer::PRIMTYPE_LINES;
		Cmd.m_PrimCount = NumVerts/2;
	}
	else
		return;

	Cmd.m_pVertices = (CCommandBuffer::SVertex *)m_pCommandBuffer->AllocData(sizeof(CCommandBuffer::SVertex)*NumVerts);
	if(Cmd.m_pVertices == 0x0)
	{
		// kick command buffer and try again
		KickCommandBuffer();

		Cmd.m_pVertices = (CCommandBuffer::SVertex *)m_pCommandBuffer->AllocData(sizeof(CCommandBuffer::SVertex)*NumVerts);
		if(Cmd.m_pVertices == 0x0)
		{
			dbg_msg("graphics", "failed to allocate data for vertices");
			return;
		}
	}

	// check if we have enough free memory in the commandbuffer
	if(!m_pCommandBuffer->AddCommand(Cmd))
	{
		// kick command buffer and try again
		KickCommandBuffer();

		Cmd.m_pVertices = (CCommandBuffer::SVertex *)m_pCommandBuffer->AllocData(sizeof(CCommandBuffer::SVertex)*NumVerts);
		if(Cmd.m_pVertices == 0x0)
		{
			dbg_msg("graphics", "failed to allocate data for vertices");
			return;
		}

		if(!m_pCommandBuffer->AddCommand(Cmd))
		{
			dbg_msg("graphics", "failed to allocate memory for render command");
			return;
		}
	}

	mem_copy(Cmd.m_pVertices, m_aVertices, sizeof(CCommandBuffer::SVertex)*NumVerts);
}

void CGraphics_Threaded::AddVertices(int Count)
{
	m_NumVertices += Count;
	if((m_NumVertices + Count) >= MAX_VERTICES)
		FlushVertices();
}

void CGraphics_Threaded::Rotate(const CCommandBuffer::SPoint &rCenter, CCommandBuffer::SVertex *pPoints, int NumPoints)
{
	const float c = cosf(m_Rotation);
	const float s = sinf(m_Rotation);

	for(int i = 0; i < NumPoints; i++)
	{
		float x = pPoints[i].m_Pos.x - rCenter.x;
		float y = pPoints[i].m_Pos.y - rCenter.y;
		pPoints[i].m_Pos.x = x * c - y * s + rCenter.x;
		pPoints[i].m_Pos.y = x * s + y * c + rCenter.y;
	}
}

CGraphics_Threaded::CGraphics_Threaded()
{
	m_State.m_ScreenTL.x = 0;
	m_State.m_ScreenTL.y = 0;
	m_State.m_ScreenBR.x = 0;
	m_State.m_ScreenBR.y = 0;
	m_State.m_ClipEnable = false;
	m_State.m_ClipX = 0;
	m_State.m_ClipY = 0;
	m_State.m_ClipW = 0;
	m_State.m_ClipH = 0;
	m_State.m_Texture = -1;
	m_State.m_BlendMode = CCommandBuffer::BLEND_NONE;
	m_State.m_WrapMode = CCommandBuffer::WRAP_REPEAT;

	m_CurrentCommandBuffer = 0;
	m_pCommandBuffer = 0x0;
	m_apCommandBuffers[0] = 0x0;
	m_apCommandBuffers[1] = 0x0;

	m_NumVertices = 0;

	m_ScreenWidth = -1;
	m_ScreenHeight = -1;

	m_Rotation = 0;
	m_Drawing = 0;
	m_DrawingLua = 0;
	m_InvalidTexture = 0;

	m_TextureMemoryUsage = 0;

	m_RenderEnable = true;
	m_DoScreenshot = false;
}

void CGraphics_Threaded::ClipEnable(int x, int y, int w, int h)
{
	if(x < 0)
		w += x;
	if(y < 0)
		h += y;

	x = clamp(x, 0, ScreenWidth());
	y = clamp(y, 0, ScreenHeight());
	w = clamp(w, 0, ScreenWidth()-x);
	h = clamp(h, 0, ScreenHeight()-y);

	m_State.m_ClipEnable = true;
	m_State.m_ClipX = x;
	m_State.m_ClipY = ScreenHeight()-(y+h);
	m_State.m_ClipW = w;
	m_State.m_ClipH = h;
}

void CGraphics_Threaded::ClipDisable()
{
	m_State.m_ClipEnable = false;
}

void CGraphics_Threaded::BlendNone()
{
	m_State.m_BlendMode = CCommandBuffer::BLEND_NONE;
}

void CGraphics_Threaded::BlendNormal()
{
	m_State.m_BlendMode = CCommandBuffer::BLEND_ALPHA;
}

void CGraphics_Threaded::BlendAdditive()
{
	m_State.m_BlendMode = CCommandBuffer::BLEND_ADDITIVE;
}

void CGraphics_Threaded::WrapNormal()
{
	m_State.m_WrapMode = CCommandBuffer::WRAP_REPEAT;
}

void CGraphics_Threaded::WrapClamp()
{
	m_State.m_WrapMode = CCommandBuffer::WRAP_CLAMP;
}

int CGraphics_Threaded::MemoryUsage() const
{
	return m_pBackend->MemoryUsage();
}

void CGraphics_Threaded::MapScreen(float TopLeftX, float TopLeftY, float BottomRightX, float BottomRightY)
{
	m_State.m_ScreenTL.x = TopLeftX;
	m_State.m_ScreenTL.y = TopLeftY;
	m_State.m_ScreenBR.x = BottomRightX;
	m_State.m_ScreenBR.y = BottomRightY;
}

void CGraphics_Threaded::GetScreen(float *pTopLeftX, float *pTopLeftY, float *pBottomRightX, float *pBottomRightY) const
{
	if(pTopLeftX) *pTopLeftX = m_State.m_ScreenTL.x;
	if(pTopLeftY) *pTopLeftY = m_State.m_ScreenTL.y;
	if(pBottomRightX) *pBottomRightX = m_State.m_ScreenBR.x;
	if(pBottomRightY) *pBottomRightY = m_State.m_ScreenBR.y;
}

void CGraphics_Threaded::LinesBegin()
{
	dbg_assert(m_Drawing == 0, "called Graphics()->LinesBegin twice");
	m_Drawing = DRAWING_LINES;
	SetColor(1,1,1,1);
}

void CGraphics_Threaded::LinesEnd()
{
	dbg_assert(m_Drawing == DRAWING_LINES, "called Graphics()->LinesEnd without begin");
	FlushVertices();
	m_Drawing = 0;
}

void CGraphics_Threaded::LinesDraw(const CLineItem *pArray, int Num)
{
	dbg_assert(m_Drawing == DRAWING_LINES, "called Graphics()->LinesDraw without begin");

	for(int i = 0; i < Num; ++i)
	{
		m_aVertices[m_NumVertices + 2*i].m_Pos.x = pArray[i].m_X0;
		m_aVertices[m_NumVertices + 2*i].m_Pos.y = pArray[i].m_Y0;
		m_aVertices[m_NumVertices + 2*i].m_Tex = m_aTexture[0];
		m_aVertices[m_NumVertices + 2*i].m_Color = m_aColor[0];

		m_aVertices[m_NumVertices + 2*i + 1].m_Pos.x = pArray[i].m_X1;
		m_aVertices[m_NumVertices + 2*i + 1].m_Pos.y = pArray[i].m_Y1;
		m_aVertices[m_NumVertices + 2*i + 1].m_Tex = m_aTexture[1];
		m_aVertices[m_NumVertices + 2*i + 1].m_Color = m_aColor[1];
	}

	AddVertices(2*Num);
}

void CGraphics_Threaded::LinesBeginLua(lua_State *L)
{
	dbg_assert_lua(m_DrawingLua == 0, "called Graphics()->LinesBegin within begin");
	m_DrawingLua = DRAWING_LINES;
	LinesBegin();
}

void CGraphics_Threaded::LinesEndLua(lua_State *L)
{
	dbg_assert_lua(m_DrawingLua == DRAWING_LINES, "called Graphics()->LinesEnd without begin");
	m_DrawingLua = 0;
	LinesEnd();
}

int CGraphics_Threaded::LinesDrawLua(lua_State *L)
{
	dbg_assert_lua(m_DrawingLua == DRAWING_LINES, "called Graphics()->LinesDraw without begin");

	int n = lua_gettop(L)-1; // REMEMBER THAT THERE IS A 'self' ON THE STACK!
	if(n != 1 && n != 2)
		return luaL_error(L, "Engine.Graphics:LinesDraw expects 1 or 2 arguments, got %d", n);

	argcheck(lua_istable(L, 2), 2, "table");
	int MaxNum = (int)luaL_optinteger(L, 3, (MAX_VERTICES-m_NumVertices)/2);

	size_t len = lua_objlen(L, 2);
	if(len == 0)
		return 0;//luaL_error(L, "the given table doesn't contain any elements!");

	const int NUM = min((int)len, MaxNum);

	LuaRef v = LuaRef::fromStack(L, 2);
	if(!v.isTable()) // this case should never actually happen
		return luaL_error(L, "something bad happened while getting a LuaRef to your table");

	CLineItem aLineItems[MAX_VERTICES];
	for(int i = 1; i <= NUM; i++)
	{
		if(!v[i].isUserdata())
			luaL_error(L, "elements in array for LinesDraw must be LineItem (got %s @ %d)", luaL_typename(L, v[i].type()), i);
		aLineItems[i-1] = v[i].cast<CLineItem>();
	}
	LinesDraw(aLineItems, NUM);
	return 0;
}

int CGraphics_Threaded::UnloadTexture(int Index)
{
	if(Index == m_InvalidTexture)
		return 0;

	if(Index < 0)
		return 0;

	CCommandBuffer::SCommand_Texture_Destroy Cmd;
	Cmd.m_Slot = Index;
	m_pCommandBuffer->AddCommand(Cmd);

	m_aTextureIndices[Index] = m_FirstFreeTexture;
	m_FirstFreeTexture = Index;
	return 0;
}

int CGraphics_Threaded::UnloadTextureLua(int Index, lua_State *L)
{
	if(Index >= 0 && Index != m_InvalidTexture)
		CLuaBinding::GetLuaFile(L)->GetResMan()->DeregisterTexture(Index);
	return UnloadTexture(Index);
}

static int ImageFormatToTexFormat(int Format)
{
	if(Format == CImageInfo::FORMAT_RGB) return CCommandBuffer::TEXFORMAT_RGB;
	if(Format == CImageInfo::FORMAT_RGBA) return CCommandBuffer::TEXFORMAT_RGBA;
	if(Format == CImageInfo::FORMAT_ALPHA) return CCommandBuffer::TEXFORMAT_ALPHA;
	return CCommandBuffer::TEXFORMAT_RGBA;
}

static int ImageFormatToPixelSize(int Format)
{
	switch(Format)
	{
	case CImageInfo::FORMAT_RGB: return 3;
	case CImageInfo::FORMAT_ALPHA: return 1;
	default: return 4;
	}
}


int CGraphics_Threaded::LoadTextureRawSub(int TextureID, int x, int y, int Width, int Height, int Format, const void *pData)
{
	CCommandBuffer::SCommand_Texture_Update Cmd;
	Cmd.m_Slot = TextureID;
	Cmd.m_X = x;
	Cmd.m_Y = y;
	Cmd.m_Width = Width;
	Cmd.m_Height = Height;
	Cmd.m_Format = ImageFormatToTexFormat(Format);

	// calculate memory usage
	int MemSize = Width*Height*ImageFormatToPixelSize(Format);

	// copy texture data
	void *pTmpData = mem_alloc(MemSize, sizeof(void*));
	mem_copy(pTmpData, pData, MemSize);
	Cmd.m_pData = pTmpData;

	//
	m_pCommandBuffer->AddCommand(Cmd);
	return 0;
}

int CGraphics_Threaded::LoadTextureRaw(int Width, int Height, int Format, const void *pData, int StoreFormat, int Flags)
{
	// don't waste memory on texture if we are stress testing
#ifdef CONF_DEBUG
	if(g_Config.m_DbgStress)
		return m_InvalidTexture;
#endif

	// grab texture
	int Tex = m_FirstFreeTexture;
	m_FirstFreeTexture = m_aTextureIndices[Tex];
	m_aTextureIndices[Tex] = -1;

	CCommandBuffer::SCommand_Texture_Create Cmd;
	Cmd.m_Slot = Tex;
	Cmd.m_Width = Width;
	Cmd.m_Height = Height;
	Cmd.m_PixelSize = ImageFormatToPixelSize(Format);
	Cmd.m_Format = ImageFormatToTexFormat(Format);
	Cmd.m_StoreFormat = ImageFormatToTexFormat(StoreFormat);

	// flags
	Cmd.m_Flags = 0;
	if(Flags&IGraphics::TEXLOAD_NOMIPMAPS)
		Cmd.m_Flags |= CCommandBuffer::TEXFLAG_NOMIPMAPS;
	if(Flags&IGraphics::TEXLOAD_FILTER_NEAREST)
		Cmd.m_Flags |= CCommandBuffer::TEXLOAD_FILTER_NEAREST;
	if(g_Config.m_GfxTextureCompression)
		Cmd.m_Flags |= CCommandBuffer::TEXFLAG_COMPRESSED;
	if(g_Config.m_GfxTextureQuality || Flags&TEXLOAD_NORESAMPLE)
		Cmd.m_Flags |= CCommandBuffer::TEXFLAG_QUALITY;

	// copy texture data
	int MemSize = Width*Height*Cmd.m_PixelSize;
	void *pTmpData = mem_alloc(MemSize, sizeof(void*));
	mem_copy(pTmpData, pData, MemSize);
	Cmd.m_pData = pTmpData;

	//
	m_pCommandBuffer->AddCommand(Cmd);

	return Tex;
}

// simple uncompressed RGBA loaders
int CGraphics_Threaded::LoadTexture(const char *pFilename, int StorageType, int StoreFormat, int Flags)
{
	int l = str_length(pFilename);
	int ID;
	CImageInfo Img;

	if(l < 3)
		return -1;
	if(LoadPNG(&Img, pFilename, StorageType))
	{
		if (StoreFormat == CImageInfo::FORMAT_AUTO)
			StoreFormat = Img.m_Format;

		ID = LoadTextureRaw(Img.m_Width, Img.m_Height, Img.m_Format, Img.m_pData, StoreFormat, Flags);
		mem_free(Img.m_pData);
		if(ID != m_InvalidTexture && g_Config.m_Debug)
			dbg_msg("graphics/texture", "loaded %s", pFilename);
		return ID;
	}

	return m_InvalidTexture;
}

int CGraphics_Threaded::LoadTextureLua(const char *pFilename, int StorageType, int StoreFormat, int Flags, lua_State *L)
{
	int TexID = LoadTexture(pFilename, StorageType, StoreFormat, Flags);
	if(TexID >= 0 && TexID != m_InvalidTexture)
		CLuaBinding::GetLuaFile(L)->GetResMan()->RegisterTexture(TexID);
	return TexID;
}

int CGraphics_Threaded::LoadTextureLuaSimple(const char *pFilename, lua_State *L)
{
	int TexID = LoadTexture(pFilename, IStorageTW::TYPE_ALL, CImageInfo::FORMAT_AUTO, TEXLOAD_NORESAMPLE);
	if(TexID >= 0 && TexID != m_InvalidTexture)
		CLuaBinding::GetLuaFile(L)->GetResMan()->RegisterTexture(TexID);
	return TexID;
}

int CGraphics_Threaded::LoadPNG(CImageInfo *pImg, const char *pFilename, int StorageType)
{
	char aCompleteFilename[512];
	unsigned char *pBuffer;
	png_t Png; // ignore_convention

	// open file for reading
	png_init(0,0); // ignore_convention

	IOHANDLE File = m_pStorage->OpenFile(pFilename, IOFLAG_READ, StorageType, aCompleteFilename, sizeof(aCompleteFilename));
	if(File)
		io_close(File);
	else
	{
		dbg_msg("game/png", "failed to open file. filename='%s'", pFilename);
		return 0;
	}

	int Error = png_open_file(&Png, aCompleteFilename); // ignore_convention
	if(Error != PNG_NO_ERROR)
	{
		dbg_msg("game/png", "failed to open file. filename='%s'", aCompleteFilename);
		if(Error != PNG_FILE_ERROR)
			png_close_file(&Png); // ignore_convention
		return 0;
	}

	if(Png.depth != 8 || (Png.color_type != PNG_TRUECOLOR && Png.color_type != PNG_TRUECOLOR_ALPHA)) // ignore_convention
	{
		dbg_msg("game/png", "invalid format. filename='%s'", aCompleteFilename);
		png_close_file(&Png); // ignore_convention
		return 0;
	}

	pBuffer = (unsigned char *)mem_alloc(Png.width * Png.height * Png.bpp, 1); // ignore_convention
	png_get_data(&Png, pBuffer); // ignore_convention
	png_close_file(&Png); // ignore_convention

	pImg->m_Width = Png.width; // ignore_convention
	pImg->m_Height = Png.height; // ignore_convention
	if(Png.color_type == PNG_TRUECOLOR) // ignore_convention
		pImg->m_Format = CImageInfo::FORMAT_RGB;
	else if(Png.color_type == PNG_TRUECOLOR_ALPHA) // ignore_convention
		pImg->m_Format = CImageInfo::FORMAT_RGBA;
	pImg->m_pData = pBuffer;
	return 1;
}

void CGraphics_Threaded::KickCommandBuffer()
{
	m_pBackend->RunBuffer(m_pCommandBuffer);

	// swap buffer
	m_CurrentCommandBuffer ^= 1;
	m_pCommandBuffer = m_apCommandBuffers[m_CurrentCommandBuffer];
	m_pCommandBuffer->Reset();
}

void CGraphics_Threaded::ScreenshotDirect()
{
	// add swap command
	CImageInfo Image;
	mem_zero(&Image, sizeof(Image));

	CCommandBuffer::SCommand_Screenshot Cmd;
	Cmd.m_pImage = &Image;
	m_pCommandBuffer->AddCommand(Cmd);

	// kick the buffer and wait for the result
	KickCommandBuffer();
	WaitForIdle();

	if(Image.m_pData)
	{
		// find filename
		char aWholePath[1024];
		png_t Png; // ignore_convention

		IOHANDLE File = m_pStorage->OpenFile(m_aScreenshotName, IOFLAG_WRITE, IStorageTW::TYPE_SAVE, aWholePath, sizeof(aWholePath));
		if(File)
			io_close(File);

		// save png
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "saved screenshot to '%s'", aWholePath);
		m_pConsole->Print(IConsole::OUTPUT_LEVEL_STANDARD, "client", aBuf);
		png_open_file_write(&Png, aWholePath); // ignore_convention
		png_set_data(&Png, Image.m_Width, Image.m_Height, 8, PNG_TRUECOLOR, (unsigned char *)Image.m_pData); // ignore_convention
		png_close_file(&Png); // ignore_convention

		mem_free(Image.m_pData);
	}
}

void CGraphics_Threaded::TextureSet(int TextureID)
{
	dbg_assert(m_Drawing == 0, "called Graphics()->TextureSet within begin");
	m_State.m_Texture = TextureID;
}

void CGraphics_Threaded::TextureSetLua(int TextureID, lua_State *L)
{
	dbg_assert_lua(m_DrawingLua == 0, "called Graphics()->TextureSet within begin");
	TextureSet(TextureID);
}

void CGraphics_Threaded::Clear(float r, float g, float b)
{
	CCommandBuffer::SCommand_Clear Cmd;
	Cmd.m_Color.r = r;
	Cmd.m_Color.g = g;
	Cmd.m_Color.b = b;
	Cmd.m_Color.a = 0;
	m_pCommandBuffer->AddCommand(Cmd);
}

void CGraphics_Threaded::QuadsBegin()
{
	dbg_assert(m_Drawing == 0, "called Graphics()->QuadsBegin twice");
	m_Drawing = DRAWING_QUADS;

	QuadsSetSubset(0,0,1,1);
	QuadsSetRotation(0);
	SetColor(1,1,1,1);
}

void CGraphics_Threaded::QuadsEnd()
{
	dbg_assert(m_Drawing == DRAWING_QUADS, "called Graphics()->QuadsEnd without begin");
	FlushVertices();
	m_Drawing = 0;
}

void CGraphics_Threaded::QuadsSetRotation(float Angle)
{
	dbg_assert(m_Drawing == DRAWING_QUADS, "called Graphics()->QuadsSetRotation without begin");
	m_Rotation = Angle;
}

void CGraphics_Threaded::QuadsBeginLua(lua_State *L)
{
	dbg_assert_lua(m_DrawingLua == 0, "called Graphics()->QuadsBegin twice");
	m_DrawingLua = DRAWING_QUADS;
	QuadsBegin();
}

void CGraphics_Threaded::QuadsEndLua(lua_State *L)
{
	dbg_assert_lua(m_DrawingLua == DRAWING_QUADS, "called Graphics()->QuadsEnd without begin");
	m_DrawingLua = 0;
	QuadsEnd();
}

void CGraphics_Threaded::QuadsSetRotationLua(float Angle, lua_State *L)
{
	dbg_assert_lua(m_DrawingLua == DRAWING_QUADS, "called Graphics()->QuadsSetRotation without begin");
	QuadsSetRotation(Angle);
}

void CGraphics_Threaded::SetColorVertex(const CColorVertex *pArray, int Num)
{
	dbg_assert(m_Drawing != 0, "called Graphics()->SetColorVertex without begin");

	for(int i = 0; i < Num; ++i)
	{
		const IGraphics::CColorVertex &Elem = pArray[i];
		CCommandBuffer::SColor &Color = m_aColor[Elem.m_Index];
		Color.r = Elem.m_R;
		Color.g = Elem.m_G;
		Color.b = Elem.m_B;
		Color.a = Elem.m_A;
	}
}

int CGraphics_Threaded::SetColorVertexLua(lua_State *L)
{
	dbg_assert_lua(m_DrawingLua != 0, "called Graphics()->SetColorVertex without begin");

	int n = lua_gettop(L)-1; // REMEMBER THE 'self'!!
	if(n != 1 && n != 2)
		return luaL_error(L, "Engine.Graphics:SetColorVertex expects 1 or 2 arguments, got %d", n);

	argcheck(lua_istable(L, 2), 2, "table");
	int MaxNum = (int)luaL_optinteger(L, 3, (MAX_VERTICES-m_NumVertices)/(3*2));

	size_t len = lua_objlen(L, 2);
	if(len == 0)
		return 0;//luaL_error(L, "the given table doesn't contain any elements!");

	const int NUM = min((int)len, MaxNum);

	LuaRef v = LuaRef::fromStack(L, 2);
	if(!v.isTable()) // this case should never actually happen
		return luaL_error(L, "something bad happened while getting a LuaRef to your table");

	CColorVertex aColorItems[MAX_VERTICES];
	for(int i = 1; i <= NUM; i++)
	{
		if(!v[i].isUserdata())
			luaL_error(L, "elements in array for SetColorVertex must be ColorVertex (got %s @ %d)", luaL_typename(L, v[i].type()), i);
		aColorItems[i-1] = v[i].cast<CColorVertex>();
	}
	SetColorVertex(aColorItems, NUM);
	return 0;
}


void CGraphics_Threaded::SetColor(float r, float g, float b, float a)
{
	dbg_assert(m_Drawing != 0, "called Graphics()->SetColor without begin");
	CColorVertex Array[4] = {
		CColorVertex(0, r, g, b, a),
		CColorVertex(1, r, g, b, a),
		CColorVertex(2, r, g, b, a),
		CColorVertex(3, r, g, b, a)};
	SetColorVertex(Array, 4);
}

void CGraphics_Threaded::SetColorLua(float r, float g, float b, float a, lua_State *L)
{
	dbg_assert_lua(m_DrawingLua != 0, "called Graphics()->SetColor without begin");
	SetColor(r, g, b, a);
}

void CGraphics_Threaded::QuadsSetSubset(float TlU, float TlV, float BrU, float BrV)
{
	dbg_assert(m_Drawing == DRAWING_QUADS, "called Graphics()->QuadsSetSubset without begin");

	m_aTexture[0].u = TlU;	m_aTexture[1].u = BrU;
	m_aTexture[0].v = TlV;	m_aTexture[1].v = TlV;

	m_aTexture[3].u = TlU;	m_aTexture[2].u = BrU;
	m_aTexture[3].v = BrV;	m_aTexture[2].v = BrV;
}

void CGraphics_Threaded::QuadsSetSubsetFree(
	float x0, float y0, float x1, float y1,
	float x2, float y2, float x3, float y3)
{
	m_aTexture[0].u = x0; m_aTexture[0].v = y0;
	m_aTexture[1].u = x1; m_aTexture[1].v = y1;
	m_aTexture[2].u = x2; m_aTexture[2].v = y2;
	m_aTexture[3].u = x3; m_aTexture[3].v = y3;
}

void CGraphics_Threaded::QuadsDraw(CQuadItem *pArray, int Num)
{
	for(int i = 0; i < Num; ++i)
	{
		pArray[i].m_X -= pArray[i].m_Width/2;
		pArray[i].m_Y -= pArray[i].m_Height/2;
	}

	QuadsDrawTL(pArray, Num);
}

int CGraphics_Threaded::QuadsDrawLua(lua_State *L)
{
	dbg_assert_lua(m_DrawingLua == DRAWING_QUADS, "called Graphics()->QuadsDraw without begin");

	int n = lua_gettop(L)-1; // REMEMBER THE 'self'!!
	if(n != 1 && n != 2)
		return luaL_error(L, "Engine.Graphics:QuadsDraw expects 1 or 2 arguments, got %d", n);

	argcheck(lua_istable(L, 2), 2, "table");
	int MaxNum = (int)luaL_optinteger(L, 3, (MAX_VERTICES-m_NumVertices)/(3*2));

	size_t len = lua_objlen(L, 2);
	if(len == 0)
		return 0;//luaL_error(L, "the given table doesn't contain any elements!");

	const int NUM = min((int)len, MaxNum);

	LuaRef v = LuaRef::fromStack(L, 2);
	if(!v.isTable()) // this case should never actually happen
		return luaL_error(L, "something bad happened while getting a LuaRef to your table");

	CQuadItem aQuadItems[MAX_VERTICES];
	for(int i = 1; i <= NUM; i++)
	{
		if(!v[i].isUserdata())
			luaL_error(L, "elements in array for QuadsDraw must be QuadItem (got %s @ %d)", luaL_typename(L, v[i].type()), i);
		aQuadItems[i-1] = v[i].cast<CQuadItem>();
	}
	QuadsDraw(aQuadItems, NUM);
	return 0;
}

int CGraphics_Threaded::QuadsDrawTLLua(lua_State *L)
{
	dbg_assert_lua(m_DrawingLua == DRAWING_QUADS, "called Graphics()->QuadsDrawTL without begin");

	int n = lua_gettop(L)-1; // REMEMBER THE 'self'!!
	if(n != 1 && n != 2)
		return luaL_error(L, "Engine.Graphics:QuadsDrawTL expects 1 or 2 arguments, got %d", n);

	argcheck(lua_istable(L, 2), 2, "table");
	int MaxNum = (int)luaL_optinteger(L, 3, (MAX_VERTICES-m_NumVertices)/(3*2));

	size_t len = lua_objlen(L, 2);
	if(len == 0)
		return 0;//luaL_error(L, "the given table doesn't contain any elements!");

	const int NUM = min((int)len, MaxNum);

	LuaRef v = LuaRef::fromStack(L, 2);
	if(!v.isTable()) // this case should never actually happen
		return luaL_error(L, "something bad happened while getting a LuaRef to your table");

	CQuadItem aQuadItems[MAX_VERTICES];
	for(int i = 1; i <= NUM; i++)
	{
		if(!v[i].isUserdata())
			luaL_error(L, "elements in array for QuadsDrawTL must be QuadItem (got %s @ %d)", luaL_typename(L, v[i].type()), i);
		aQuadItems[i-1] = v[i].cast<CQuadItem>();
	}
	QuadsDrawTL(aQuadItems, NUM);
	return 0;
}

int CGraphics_Threaded::QuadsDrawFreeformLua(lua_State *L)
{
	dbg_assert_lua(m_DrawingLua == DRAWING_QUADS, "called Graphics()->QuadsDrawFreeform without begin");

	int n = lua_gettop(L)-1; // REMEMBER THE 'self'!!
	if(n != 1 && n != 2)
		return luaL_error(L, "Engine.Graphics:QuadsDrawFreeform expects 1 or 2 arguments, got %d", n);

	argcheck(lua_istable(L, 2), 2, "table");
	int MaxNum = (int)luaL_optinteger(L, 3, (MAX_VERTICES-m_NumVertices)/(3*2));

	size_t len = lua_objlen(L, 2);
	if(len == 0)
		return 0;//luaL_error(L, "the given table doesn't contain any elements!");

	const int NUM = min((int)len, MaxNum);

	LuaRef v = LuaRef::fromStack(L, 2);
	if(!v.isTable()) // this case should never actually happen
		return luaL_error(L, "something bad happened while getting a LuaRef to your table");

	CFreeformItem aQuadItems[MAX_VERTICES];
	for(int i = 1; i <= NUM; i++)
	{
		if(!v[i].isUserdata())
			luaL_error(L, "elements in array for QuadsDrawFreeform must be FreeformItem (got %s @ %d)", luaL_typename(L, v[i].type()), i);
		aQuadItems[i-1] = v[i].cast<CFreeformItem>();
	}
	QuadsDrawFreeform(aQuadItems, NUM);
	return 0;
}

void CGraphics_Threaded::QuadsDrawTL(const CQuadItem *pArray, int Num)
{
	CCommandBuffer::SPoint Center;
	Center.z = 0;

	dbg_assert(m_Drawing == DRAWING_QUADS, "called Graphics()->QuadsDrawTL without begin");

	if(g_Config.m_GfxQuadAsTriangle)
	{
		for(int i = 0; i < Num; ++i)
		{
			const IGraphics::CQuadItem &Source = pArray[i];
			int BaseIndex = m_NumVertices + 6*i;

			// first triangle
			m_aVertices[BaseIndex].m_Pos.x = Source.m_X;
			m_aVertices[BaseIndex].m_Pos.y = Source.m_Y;
			m_aVertices[BaseIndex].m_Tex = m_aTexture[0];
			m_aVertices[BaseIndex].m_Color = m_aColor[0];

			m_aVertices[BaseIndex + 1].m_Pos.x = Source.m_X + Source.m_Width;
			m_aVertices[BaseIndex + 1].m_Pos.y = Source.m_Y;
			m_aVertices[BaseIndex + 1].m_Tex = m_aTexture[1];
			m_aVertices[BaseIndex + 1].m_Color = m_aColor[1];

			m_aVertices[BaseIndex + 2].m_Pos.x = Source.m_X + Source.m_Width;
			m_aVertices[BaseIndex + 2].m_Pos.y = Source.m_Y + Source.m_Height;
			m_aVertices[BaseIndex + 2].m_Tex = m_aTexture[2];
			m_aVertices[BaseIndex + 2].m_Color = m_aColor[2];

			// second triangle
			m_aVertices[BaseIndex + 3].m_Pos.x = Source.m_X;
			m_aVertices[BaseIndex + 3].m_Pos.y = Source.m_Y;
			m_aVertices[BaseIndex + 3].m_Tex = m_aTexture[0];
			m_aVertices[BaseIndex + 3].m_Color = m_aColor[0];

			m_aVertices[BaseIndex + 4].m_Pos.x = Source.m_X + Source.m_Width;
			m_aVertices[BaseIndex + 4].m_Pos.y = Source.m_Y + Source.m_Height;
			m_aVertices[BaseIndex + 4].m_Tex = m_aTexture[2];
			m_aVertices[BaseIndex + 4].m_Color = m_aColor[2];

			m_aVertices[BaseIndex + 5].m_Pos.x = Source.m_X;
			m_aVertices[BaseIndex + 5].m_Pos.y = Source.m_Y + Source.m_Height;
			m_aVertices[BaseIndex + 5].m_Tex = m_aTexture[3];
			m_aVertices[BaseIndex + 5].m_Color = m_aColor[3];

			if(m_Rotation != 0)
			{
				Center.x = Source.m_X + Source.m_Width/2;
				Center.y = Source.m_Y + Source.m_Height/2;

				Rotate(Center, &m_aVertices[BaseIndex], 6);
			}
		}

		AddVertices(3*2*Num);
	}
	else
	{
		for(int i = 0; i < Num; ++i)
		{
			const IGraphics::CQuadItem &Source = pArray[i];
			int BaseIndex = m_NumVertices + 4*i;

			m_aVertices[BaseIndex].m_Pos.x = Source.m_X;
			m_aVertices[BaseIndex].m_Pos.y = Source.m_Y;
			m_aVertices[BaseIndex].m_Tex = m_aTexture[0];
			m_aVertices[BaseIndex].m_Color = m_aColor[0];

			m_aVertices[BaseIndex + 1].m_Pos.x = Source.m_X + Source.m_Width;
			m_aVertices[BaseIndex + 1].m_Pos.y = Source.m_Y;
			m_aVertices[BaseIndex + 1].m_Tex = m_aTexture[1];
			m_aVertices[BaseIndex + 1].m_Color = m_aColor[1];

			m_aVertices[BaseIndex + 2].m_Pos.x = Source.m_X + Source.m_Width;
			m_aVertices[BaseIndex + 2].m_Pos.y = Source.m_Y + Source.m_Height;
			m_aVertices[BaseIndex + 2].m_Tex = m_aTexture[2];
			m_aVertices[BaseIndex + 2].m_Color = m_aColor[2];

			m_aVertices[BaseIndex + 3].m_Pos.x = Source.m_X;
			m_aVertices[BaseIndex + 3].m_Pos.y = Source.m_Y + Source.m_Height;
			m_aVertices[BaseIndex + 3].m_Tex = m_aTexture[3];
			m_aVertices[BaseIndex + 3].m_Color = m_aColor[3];

			if(m_Rotation != 0)
			{
				Center.x = Source.m_X + Source.m_Width/2;
				Center.y = Source.m_Y + Source.m_Height/2;

				Rotate(Center, &m_aVertices[m_NumVertices + 4*i], 4);
			}
		}

		AddVertices(4*Num);
	}
}

void CGraphics_Threaded::QuadsDrawFreeform(const CFreeformItem *pArray, int Num)
{
	dbg_assert(m_Drawing == DRAWING_QUADS, "called Graphics()->QuadsDrawFreeform without begin");

	if(g_Config.m_GfxQuadAsTriangle)
	{
		for(int i = 0; i < Num; ++i)
		{
			const IGraphics::CFreeformItem &Source = pArray[i];
			int BaseIndex = m_NumVertices + 6*i;

			m_aVertices[BaseIndex].m_Pos.x = Source.m_X0;
			m_aVertices[BaseIndex].m_Pos.y = Source.m_Y0;
			m_aVertices[BaseIndex].m_Tex = m_aTexture[0];
			m_aVertices[BaseIndex].m_Color = m_aColor[0];

			m_aVertices[BaseIndex + 1].m_Pos.x = Source.m_X1;
			m_aVertices[BaseIndex + 1].m_Pos.y = Source.m_Y1;
			m_aVertices[BaseIndex + 1].m_Tex = m_aTexture[1];
			m_aVertices[BaseIndex + 1].m_Color = m_aColor[1];

			m_aVertices[BaseIndex + 2].m_Pos.x = Source.m_X3;
			m_aVertices[BaseIndex + 2].m_Pos.y = Source.m_Y3;
			m_aVertices[BaseIndex + 2].m_Tex = m_aTexture[3];
			m_aVertices[BaseIndex + 2].m_Color = m_aColor[3];

			m_aVertices[BaseIndex + 3].m_Pos.x = Source.m_X0;
			m_aVertices[BaseIndex + 3].m_Pos.y = Source.m_Y0;
			m_aVertices[BaseIndex + 3].m_Tex = m_aTexture[0];
			m_aVertices[BaseIndex + 3].m_Color = m_aColor[0];

			m_aVertices[BaseIndex + 4].m_Pos.x = Source.m_X3;
			m_aVertices[BaseIndex + 4].m_Pos.y = Source.m_Y3;
			m_aVertices[BaseIndex + 4].m_Tex = m_aTexture[3];
			m_aVertices[BaseIndex + 4].m_Color = m_aColor[3];

			m_aVertices[BaseIndex + 5].m_Pos.x = Source.m_X2;
			m_aVertices[BaseIndex + 5].m_Pos.y = Source.m_Y2;
			m_aVertices[BaseIndex + 5].m_Tex = m_aTexture[2];
			m_aVertices[BaseIndex + 5].m_Color = m_aColor[2];
		}

		AddVertices(3*2*Num);
	}
	else
	{
		for(int i = 0; i < Num; ++i)
		{
			const IGraphics::CFreeformItem &Source = pArray[i];
			int BaseIndex = m_NumVertices + 4*i;

			m_aVertices[BaseIndex].m_Pos.x = Source.m_X0;
			m_aVertices[BaseIndex].m_Pos.y = Source.m_Y0;
			m_aVertices[BaseIndex].m_Tex = m_aTexture[0];
			m_aVertices[BaseIndex].m_Color = m_aColor[0];

			m_aVertices[BaseIndex + 1].m_Pos.x = Source.m_X1;
			m_aVertices[BaseIndex + 1].m_Pos.y = Source.m_Y1;
			m_aVertices[BaseIndex + 1].m_Tex = m_aTexture[1];
			m_aVertices[BaseIndex + 1].m_Color = m_aColor[1];

			m_aVertices[BaseIndex + 2].m_Pos.x = Source.m_X3;
			m_aVertices[BaseIndex + 2].m_Pos.y = Source.m_Y3;
			m_aVertices[BaseIndex + 2].m_Tex = m_aTexture[3];
			m_aVertices[BaseIndex + 2].m_Color = m_aColor[3];

			m_aVertices[BaseIndex + 3].m_Pos.x = Source.m_X2;
			m_aVertices[BaseIndex + 3].m_Pos.y = Source.m_Y2;
			m_aVertices[BaseIndex + 3].m_Tex = m_aTexture[2];
			m_aVertices[BaseIndex + 3].m_Color = m_aColor[2];
		}

		AddVertices(4*Num);
	}
}

void CGraphics_Threaded::QuadsText(float x, float y, float Size, const char *pText)
{
	float StartX = x;

	while(*pText)
	{
		char c = *pText;
		pText++;

		if(c == '\n')
		{
			x = StartX;
			y += Size;
		}
		else
		{
			QuadsSetSubset(
				(c%16)/16.0f,
				(c/16)/16.0f,
				(c%16)/16.0f+1.0f/16.0f,
				(c/16)/16.0f+1.0f/16.0f);

			CQuadItem QuadItem(x, y, Size, Size);
			QuadsDrawTL(&QuadItem, 1);
			x += Size/2;
		}
	}
}

bool CGraphics_Threaded::LuaCheckDrawingState(lua_State *L, const char *pFuncName, bool NoThrow)
{
	//dbg_assert(m_DrawingLua == m_Drawing, "Graphics()->LuaCheckDrawingState called with rendering pipeline in unsynced state");
	if(m_Drawing == m_DrawingLua && m_DrawingLua != 0)
	{
		int PrevState = m_DrawingLua;

		// clean up the rendering pipeline for em
		switch(m_Drawing)
		{
			case DRAWING_QUADS: QuadsEndLua(L); break;
			case DRAWING_LINES: LinesEndLua(L); break;
		}

		if(!NoThrow)
		{
			// raise a lua error that results in panic, which then throws our exception
			luaL_error(L, "callback for %s left the rendering pipeline in dirty state %d (%s)", pFuncName, PrevState,
					   PrevState == DRAWING_QUADS ? "DRAWING_QUADS" :
					   PrevState == DRAWING_LINES ? "DRAWING_LINES" :
					   "UNKNOWN"
			);
		}
	}
	return false;
}

int CGraphics_Threaded::IssueInit()
{
	int Flags = 0;

	if(g_Config.m_GfxWindowMode == IGraphics::WINDOWMODE_BORDERLESS) Flags |= IGraphicsBackend::INITFLAG_BORDERLESS;
	else if(g_Config.m_GfxWindowMode == IGraphics::WINDOWMODE_FULLSCREEN) Flags |= IGraphicsBackend::INITFLAG_FULLSCREEN;
	if(g_Config.m_GfxVsync) Flags |= IGraphicsBackend::INITFLAG_VSYNC;
	if(g_Config.m_GfxResizable) Flags |= IGraphicsBackend::INITFLAG_RESIZABLE;
	if(g_Config.m_GfxHighdpi) Flags |= IGraphicsBackend::INITFLAG_HIGHDPI;

	return m_pBackend->Init(g_Config.m_GfxSimpleWindowTitle ? "Teeworlds AllTheHaxx" : "Teeworlds AllTheHaxx - The client with the keck!",
							&g_Config.m_GfxScreen, &g_Config.m_GfxScreenWidth, &g_Config.m_GfxScreenHeight, g_Config.m_GfxFsaaSamples, Flags, &m_DesktopScreenWidth, &m_DesktopScreenHeight);
}

int CGraphics_Threaded::InitWindow()
{
	if(IssueInit() == 0)
		return 0;

	// try disabling fsaa
	while(g_Config.m_GfxFsaaSamples)
	{
		g_Config.m_GfxFsaaSamples--;

		if(g_Config.m_GfxFsaaSamples)
			dbg_msg("gfx", "lowering FSAA to %d and trying again", g_Config.m_GfxFsaaSamples);
		else
			dbg_msg("gfx", "disabling FSAA and trying again");

		if(IssueInit() == 0)
			return 0;
	}

	// try lowering the resolution
	if(g_Config.m_GfxScreenWidth != 640 || g_Config.m_GfxScreenHeight != 480)
	{
		dbg_msg("gfx", "setting resolution to 640x480 and trying again");
		g_Config.m_GfxScreenWidth = 640;
		g_Config.m_GfxScreenHeight = 480;

		if(IssueInit() == 0)
			return 0;
	}

	dbg_msg("gfx", "out of ideas. failed to init graphics");

	return -1;
}

int CGraphics_Threaded::Init()
{
	// fetch pointers
	m_pStorage = Kernel()->RequestInterface<IStorageTW>();
	m_pConsole = Kernel()->RequestInterface<IConsole>();

	// Set all z to -5.0f
	for(int i = 0; i < MAX_VERTICES; i++)
		m_aVertices[i].m_Pos.z = -5.0f;

	// init textures
	m_FirstFreeTexture = 0;
	for(int i = 0; i < MAX_TEXTURES-1; i++)
		m_aTextureIndices[i] = i+1;
	m_aTextureIndices[MAX_TEXTURES-1] = -1;

	m_pBackend = CreateGraphicsBackend();
	if(InitWindow() != 0)
		return -1;

	// fetch final resolution
	m_ScreenWidth = g_Config.m_GfxScreenWidth;
	m_ScreenHeight = g_Config.m_GfxScreenHeight;

	// create command buffers
	for(int i = 0; i < NUM_CMDBUFFERS; i++)
		m_apCommandBuffers[i] = new CCommandBuffer(256*1024, 2*1024*1024);
	m_pCommandBuffer = m_apCommandBuffers[0];

	// create null texture, will get id=0
	static const unsigned char aNullTextureData[] = {
		0xff,0x00,0x00,0xff, 0xff,0x00,0x00,0xff, 0x00,0xff,0x00,0xff, 0x00,0xff,0x00,0xff,
		0xff,0x00,0x00,0xff, 0xff,0x00,0x00,0xff, 0x00,0xff,0x00,0xff, 0x00,0xff,0x00,0xff,
		0x00,0x00,0xff,0xff, 0x00,0x00,0xff,0xff, 0xff,0xff,0x00,0xff, 0xff,0xff,0x00,0xff,
		0x00,0x00,0xff,0xff, 0x00,0x00,0xff,0xff, 0xff,0xff,0x00,0xff, 0xff,0xff,0x00,0xff,
	};

	m_InvalidTexture = LoadTextureRaw(4,4,CImageInfo::FORMAT_RGBA,aNullTextureData,CImageInfo::FORMAT_RGBA,TEXLOAD_NORESAMPLE);
	return 0;
}

void CGraphics_Threaded::Shutdown()
{
	// shutdown the backend
	m_pBackend->Shutdown();
	delete m_pBackend;
	m_pBackend = 0x0;

	// delete the command buffers
	for(int i = 0; i < NUM_CMDBUFFERS; i++)
		delete m_apCommandBuffers[i];
}

int CGraphics_Threaded::GetNumScreens() const
{
	return m_pBackend->GetNumScreens();
}

void CGraphics_Threaded::Minimize()
{
	m_pBackend->Minimize();
}

void CGraphics_Threaded::Maximize()
{
	// TODO: SDL
	m_pBackend->Maximize();
}

bool CGraphics_Threaded::SetFullscreen(bool State)
{
	return m_pBackend->Fullscreen(State);
}

void CGraphics_Threaded::SetWindowBordered(bool State)
{
	m_pBackend->SetWindowBordered(State);
}

bool CGraphics_Threaded::SetWindowScreen(int Index)
{
	return m_pBackend->SetWindowScreen(Index);
}

void CGraphics_Threaded::Resize(int w, int h)
{
	if(m_ScreenWidth == w && m_ScreenHeight == h)
		return;

	if(h > 4*w/5)
		h = 4*w/5;
	if(w > 21*h/9)
		w = 21*h/9;

	m_ScreenWidth = w;
	m_ScreenHeight = h;

	CCommandBuffer::SCommand_Resize Cmd;
	Cmd.m_Width = w;
	Cmd.m_Height = h;
	m_pCommandBuffer->AddCommand(Cmd);

	// kick the command buffer
	KickCommandBuffer();
	WaitForIdle();
}

int CGraphics_Threaded::GetWindowScreen()
{
	return m_pBackend->GetWindowScreen();
}

int CGraphics_Threaded::WindowActive()
{
	return m_pBackend->WindowActive();
}

int CGraphics_Threaded::WindowOpen()
{
	return m_pBackend->WindowOpen();
}

void CGraphics_Threaded::SetWindowGrab(bool Grab)
{
	return m_pBackend->SetWindowGrab(Grab);
}

void CGraphics_Threaded::NotifyWindow()
{
	return m_pBackend->NotifyWindow();
}

void CGraphics_Threaded::HideWindow()
{
	m_pBackend->HideWindow();
}

void CGraphics_Threaded::UnhideWindow()
{
	m_pBackend->UnhideWindow();
}

void CGraphics_Threaded::TakeScreenshot(const char *pFilename)
{
	// TODO: screenshot support
	char aDate[20];
	str_timestamp(aDate, sizeof(aDate));
	str_format(m_aScreenshotName, sizeof(m_aScreenshotName), "screenshots/%s_%s.png", pFilename?pFilename:"screenshot", aDate);
	m_DoScreenshot = true;
}

void CGraphics_Threaded::TakeCustomScreenshot(const char *pFilename)
{
	str_copy(m_aScreenshotName, pFilename, sizeof(m_aScreenshotName));
	m_DoScreenshot = true;
}

void CGraphics_Threaded::Swap()
{
	// TODO: screenshot support
	if(m_DoScreenshot)
	{
		if(WindowActive())
			ScreenshotDirect();
		m_DoScreenshot = false;
	}

	// add swap command
	CCommandBuffer::SCommand_Swap Cmd;
	Cmd.m_Finish = g_Config.m_GfxFinish;
	m_pCommandBuffer->AddCommand(Cmd);

	// kick the command buffer
	KickCommandBuffer();
}

bool CGraphics_Threaded::SetVSync(bool State)
{
	// add vsnc command
	bool RetOk = 0;
	CCommandBuffer::SCommand_VSync Cmd;
	Cmd.m_VSync = State ? 1 : 0;
	Cmd.m_pRetOk = &RetOk;
	m_pCommandBuffer->AddCommand(Cmd);

	// kick the command buffer
	KickCommandBuffer();
	WaitForIdle();
	return RetOk;
}

// syncronization
void CGraphics_Threaded::InsertSignal(semaphore *pSemaphore)
{
	CCommandBuffer::SCommand_Signal Cmd;
	Cmd.m_pSemaphore = pSemaphore;
	m_pCommandBuffer->AddCommand(Cmd);
}

bool CGraphics_Threaded::IsIdle()
{
	return m_pBackend->IsIdle();
}

void CGraphics_Threaded::WaitForIdle()
{
	m_pBackend->WaitForIdle();
}

int CGraphics_Threaded::GetVideoModes(CVideoMode *pModes, int MaxModes, int Screen)
{
	if(g_Config.m_GfxDisplayAllModes)
	{
		int Count = sizeof(g_aFakeModes)/sizeof(CVideoMode);
		mem_copy(pModes, g_aFakeModes, sizeof(g_aFakeModes));
		if(MaxModes < Count)
			Count = MaxModes;
		return Count;
	}

	// add videomodes command
	CImageInfo Image;
	mem_zero(&Image, sizeof(Image));

	int NumModes = 0;
	CCommandBuffer::SCommand_VideoModes Cmd;
	Cmd.m_pModes = pModes;
	Cmd.m_MaxModes = MaxModes;
	Cmd.m_pNumModes = &NumModes;
	Cmd.m_Screen = Screen;
	m_pCommandBuffer->AddCommand(Cmd);

	// kick the buffer and wait for the result and return it
	KickCommandBuffer();
	WaitForIdle();
	return NumModes;
}

extern IEngineGraphics *CreateEngineGraphicsThreaded() { return new CGraphics_Threaded(); }
