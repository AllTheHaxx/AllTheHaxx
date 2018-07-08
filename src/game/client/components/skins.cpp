/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/system.h>
#include <base/math.h>

#include <engine/graphics.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

#include "skins.h"

const char* vanillaSkins[] = {"bluekitty.png", "bluestripe.png", "brownbear.png",
	"cammo.png", "cammostripes.png", "coala.png", "default.png", "limekitty.png",
	"pinky.png", "redbopp.png", "redstripe.png", "saddo.png", "toptri.png",
	"twinbop.png", "twintri.png", "warpaint.png", "x_ninja.png"};



int CSkins::CSkin::GetColorTexture() const
{
	if(m_ColorTexture == SKIN_TEXTURE_NOT_LOADED)
	{
		m_pSkins->LoadTexturesThreaded(const_cast<CSkin *>(this)); // const-cheatsy allowed here because this should look like a plain getter to the outside
		return m_pSkins->m_pDefaultSkin->m_ColorTexture;
	}
	else if(m_ColorTexture == SKIN_TEXTURE_LOADING)
		return m_pSkins->GetDefaultSkinColorTexture();
	else if(m_ColorTexture > 0) // loaded
		return m_ColorTexture;

	dbg_assert_critical(false, "shit happened");
}

int CSkins::CSkin::GetOrgTexture() const
{
	if(m_OrgTexture == SKIN_TEXTURE_NOT_LOADED)
	{
		m_pSkins->LoadTexturesThreaded(const_cast<CSkin *>(this));
		return m_pSkins->m_pDefaultSkin->m_OrgTexture;
	}
	else if(m_ColorTexture == SKIN_TEXTURE_LOADING)
		return m_pSkins->GetDefaultSkinOrgTexture();
	else if(m_OrgTexture > 0)// loaded
		return m_OrgTexture;

	dbg_assert_critical(false, "shit happened");
}


void CSkins::LoadTexturesImpl(CSkin *pSkin)
{
	if(g_Config.m_Debug)
		dbg_msg("skins", "loading texture for skin '%s' from '%s'", pSkin->GetName(), pSkin->m_FileInfo.m_aFullPath);

	CImageInfo Info;
	if(!Graphics()->LoadPNG(&Info, pSkin->m_FileInfo.m_aFullPath, pSkin->m_FileInfo.m_DirType))
	{
		// it failed to load, set the textures to default
		pSkin->m_ColorTexture = m_apSkins[0]->m_ColorTexture;//m_DefaultSkinColorTexture;//CSkin::SKIN_TEXTURE_NOT_FOUND;
		pSkin->m_OrgTexture = m_apSkins[0]->m_OrgTexture;//m_DefaultSkinOrgTexture;//CSkin::SKIN_TEXTURE_NOT_FOUND;
		Console()->Printf(IConsole::OUTPUT_LEVEL_ADDINFO, "game", "failed to load skin from %s", pSkin->m_FileInfo.m_aFullPath);
		// rename invalid downloaded skins so we don't try to load them again
		if(str_comp_nocase_num(pSkin->m_FileInfo.m_aFullPath, "downloadedskins", 15) == 0)
		{
			char aBuf[512];
			str_formatb(aBuf, "%s.FAIL", pSkin->m_FileInfo.m_aFullPath);
			Storage()->RenameFile(pSkin->m_FileInfo.m_aFullPath, aBuf, IStorageTW::TYPE_SAVE);
		}
		return;
	}

	pSkin->m_OrgTexture = Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);

	int BodySize = 96; // body size
	if (BodySize > Info.m_Height)
		return;

	unsigned char *d = (unsigned char *)Info.m_pData;
	int Pitch = Info.m_Width*4;

	// dig out blood color
	{
		int aColors[3] = {0};
		for(int y = 0; y < BodySize; y++)
			for(int x = 0; x < BodySize; x++)
			{
				if(d[y*Pitch+x*4+3] > 128)
				{
					aColors[0] += d[y*Pitch+x*4+0];
					aColors[1] += d[y*Pitch+x*4+1];
					aColors[2] += d[y*Pitch+x*4+2];
				}
			}

		pSkin->m_BloodColor = normalize(vec3(aColors[0], aColors[1], aColors[2]));
	}

	// create colorless version
	int Step = Info.m_Format == CImageInfo::FORMAT_RGBA ? 4 : 3;

	// make the texture gray scale
	for(int i = 0; i < Info.m_Width*Info.m_Height; i++)
	{
		int v = (d[i*Step]+d[i*Step+1]+d[i*Step+2])/3;
		d[i*Step] = v;
		d[i*Step+1] = v;
		d[i*Step+2] = v;
	}


	int Freq[256] = {0};
	int OrgWeight = 0;
	int NewWeight = 192;

	// find most common frequence
	for(int y = 0; y < BodySize; y++)
		for(int x = 0; x < BodySize; x++)
		{
			if(d[y*Pitch+x*4+3] > 128)
				Freq[d[y*Pitch+x*4]]++;
		}

	for(int i = 1; i < 256; i++)
	{
		if(Freq[OrgWeight] < Freq[i])
			OrgWeight = i;
	}

	// reorder
	int InvOrgWeight = 255-OrgWeight;
	int InvNewWeight = 255-NewWeight;
	for(int y = 0; y < BodySize; y++)
		for(int x = 0; x < BodySize; x++)
		{
			int v = d[y*Pitch+x*4];
			if(v <= OrgWeight)
				v = (int)(((v/(float)OrgWeight) * NewWeight));
			else
				v = (int)(((v-OrgWeight)/(float)InvOrgWeight)*InvNewWeight + NewWeight);
			d[y*Pitch+x*4] = v;
			d[y*Pitch+x*4+1] = v;
			d[y*Pitch+x*4+2] = v;
		}

	pSkin->m_ColorTexture = Graphics()->LoadTextureRaw(Info.m_Width, Info.m_Height, Info.m_Format, Info.m_pData, Info.m_Format, 0);
	mem_free(Info.m_pData);

	if(g_Config.m_Debug)
		Console()->Printf(IConsole::OUTPUT_LEVEL_ADDINFO, "game", "loaded skin texture for '%s'", pSkin->m_aName);
}


int CSkins::SkinScan(const char *pName, int IsDir, int DirType, void *pUser)
{
	CALLSTACK_ADD();

	int l = str_length(pName);
	if(l < 4 || IsDir || str_comp(pName+l-4, ".png") != 0)
		return 0;

	CSkin *pSkin = new CSkin();
	pSkin->m_IsVanilla = false;
	for(unsigned int i = 0; i < sizeof(vanillaSkins) / sizeof(vanillaSkins[0]); i++)
	{
		if(str_comp(pName, vanillaSkins[i]) == 0)
		{
			pSkin->m_IsVanilla = true;
			break;
		}
	}

	if(g_Config.m_ClVanillaSkinsOnly && !pSkin->m_IsVanilla)
		return 0;

	IStorageTW::CLoadHelper<CSkins> *pLoadHelper = (IStorageTW::CLoadHelper<CSkins> *)pUser;
	CSkins *pSelf = pLoadHelper->pSelf;

	pSkin->m_pSkins = pSelf;

	// Don't add duplicate skins (one from user's config directory, other from
	// client itself)
	for(int i = 0; i < pSelf->Num(); i++)
	{
		const char *pExName = pSelf->Get(i)->m_aName;
		if(str_comp_num(pExName, pName, l-4) == 0 && str_length(pExName) == l-4)
			return 0;
	}

	pSkin->m_FileInfo.m_DirType = DirType;
	str_formatb(pSkin->m_FileInfo.m_aFullPath, "%s/%s", pLoadHelper->pFullDir, pName);

	if(g_Config.m_ClThreadskinloading)
	{
		// textures are being loaded on-demand; later when skin is needed
		pSkin->m_OrgTexture = CSkin::SKIN_TEXTURE_NOT_LOADED;
		pSkin->m_ColorTexture = CSkin::SKIN_TEXTURE_NOT_LOADED;
	}
	else
	{
		// textures are loaded right away at client start
		pSelf->LoadTexturesImpl(pSkin);
	}

	// set skin data
	str_copy(pSkin->m_aName, pName, min((int)sizeof(pSkin->m_aName),l-3));

	// always load textures for default skin as replacement for skin textures that are currently being loaded
	if(str_comp_nocase(pSkin->m_aName, "default") == 0)
	{
		pSelf->m_pDefaultSkin = pSkin;
		pSelf->LoadTexturesImpl(pSkin);
//		pSelf->m_DefaultSkinColorTexture = pSkin->m_ColorTexture;
//		pSelf->m_DefaultSkinOrgTexture = pSkin->m_OrgTexture;
	}

	LOCK_SECTION_MUTEX(pSelf->m_SkinsLock);
	pSelf->m_apSkins.add(pSkin);

	if(g_Config.m_Debug)
		pSelf->Console()->Printf(IConsole::OUTPUT_LEVEL_ADDINFO, "game", "added skin '%s'", pSkin->m_aName);

	return 0;
}


void CSkins::OnInit()
{
	CALLSTACK_ADD();

	// load skins
	RefreshSkinList();
}

void CSkins::RefreshSkinList(bool clear)
{
	CALLSTACK_ADD();

	if(clear)
		Clear();

	IStorageTW::CLoadHelper<CSkins> *pLoadHelper = new IStorageTW::CLoadHelper<CSkins>;
	pLoadHelper->pSelf = this;

	pLoadHelper->pFullDir = "skins";
	Storage()->ListDirectory(IStorageTW::TYPE_ALL, "skins", SkinScan, pLoadHelper);

	if(!g_Config.m_ClVanillaSkinsOnly)
	{
		pLoadHelper->pFullDir = "downloadedskins";
		Storage()->ListDirectory(IStorageTW::TYPE_SAVE, "downloadedskins", SkinScan, pLoadHelper);
	}

	LOCK_SECTION_MUTEX(m_SkinsLock);
	if(m_apSkins.empty())
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "gameclient", "failed to load skins. folder='skins/'");
		CSkin *pDummySkin = new CSkin();
		pDummySkin->m_OrgTexture = CSkin::SKIN_TEXTURE_NOT_LOADED;
		pDummySkin->m_ColorTexture = CSkin::SKIN_TEXTURE_NOT_LOADED;
		str_copy(pDummySkin->m_aName, "dummy", sizeof(pDummySkin->m_aName));
		pDummySkin->m_BloodColor = vec3(1.0f, 1.0f, 1.0f);

		m_apSkins.add(pDummySkin);
	}

	if(!m_pDefaultSkin)
		m_pDefaultSkin = m_apSkins[0];

	delete pLoadHelper;
}

int CSkins::Find(const char *pName)
{
	CALLSTACK_ADD();

	LOCK_SECTION_MUTEX(m_SkinsLock);
	const int Num = m_apSkins.size();
	for(int i = 0; i < Num; i++)
	{
		if(str_comp(m_apSkins[i]->m_aName, pName) == 0)
			return i;
	}
	return -1;
}

void CSkins::Clear()
{
	CALLSTACK_ADD();

	if(m_ThreadRunning)
	{
		dbg_msg("skins/debug", "waiting for loader thread to finish before clearing...");
		thread_wait(m_pThread);
		m_pThread = NULL;
	}

	LOCK_SECTION_MUTEX(m_SkinsLock);

	for(unsigned int i = 0; i < (unsigned)m_apSkins.size(); i++)
	{
		Graphics()->UnloadTexture(m_apSkins[i]->m_OrgTexture);
		Graphics()->UnloadTexture(m_apSkins[i]->m_ColorTexture);
		delete m_apSkins[i];
		m_apSkins[i] = NULL;
	}
	m_apSkins.clear();

	m_pDefaultSkin = NULL;
//	m_DefaultSkinColorTexture = -1;
//	m_DefaultSkinOrgTexture = -1;
}

vec3 CSkins::GetColorV3(int v)
{
	return HslToRgb(vec3(((v>>16)&0xff)/255.0f, ((v>>8)&0xff)/255.0f, 0.5f+(v&0xff)/255.0f*0.5f));
}

vec4 CSkins::GetColorV4(int v)
{
	vec3 r = GetColorV3(v);
	return vec4(r.r, r.g, r.b, 1.0f);
}

void CSkins::LoadTexturesThreaded(CSkins::CSkin *pSkin)
{
	if(m_ThreadRunning)
		return;

	LOCK_SECTION_MUTEX(m_SkinsLock);

	pSkin->m_ColorTexture = CSkin::SKIN_TEXTURE_LOADING;
	pSkin->m_OrgTexture = CSkin::SKIN_TEXTURE_LOADING;

	m_ThreadRunning = true;
	m_pThread = thread_init_named(CSkins::LoadTexturesThreadProxy, pSkin, "skin_loader");
}

void CSkins::LoadTexturesThreadProxy(void *pUser)
{
	CSkin *pSkin = (CSkin*)pUser;
	CSkins *pSelf = pSkin->m_pSkins;
	pSelf->LoadTexturesImpl(pSkin);
	pSelf->m_ThreadRunning = false;
}
