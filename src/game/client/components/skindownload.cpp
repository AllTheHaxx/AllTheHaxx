
#include <sstream>
#include <fstream>

#include <engine/graphics.h>
#include <engine/textrender.h>

#include "skins.h"
#include "skindownload.h"

void CSkinDownload::OnConsoleInit()
{
	Console()->Register("fetch_skin", "s[skinname]", CFGFLAG_CLIENT, ConFetchSkin, this, "Fetch a skin by name from the available databases");
}

void CSkinDownload::OnInit()
{
	m_pFetcher = Kernel()->RequestInterface<IFetcher>();
	m_pStorage = Kernel()->RequestInterface<IStorageTW>();

	m_Lock = lock_create();
	lock_unlock(m_Lock);
	m_apFetchTasks.clear();
	m_apFetchTasks.hint_size(5);

	LoadUrls();
}

void CSkinDownload::OnRender()
{
	if(lock_trylock(m_Lock) != 0) // just don't render if it's locked, but don't block!
		return;

	const float MAX_HEIGHT = m_apFetchTasks.empty() ? 0.001f : 5.0f+25.0f+5.0f+20.0f*(float)m_apFetchTasks.size();
	static float s_SmoothPos = 0.0f;
	smooth_set(&s_SmoothPos, MAX_HEIGHT, 23.0f, Client()->RenderFrameTime());

	if(s_SmoothPos <= 0.01f)
	{
		lock_unlock(m_Lock);
		return;
	}

	CUIRect Screen = *(UI()->Screen()), Button;
	Graphics()->MapScreen(0, 0, Screen.w, Screen.h);
	Screen.HSplitBottom(s_SmoothPos, 0, &Screen);
	//float dings = clamp(s_SmoothPos/(5.0f+25.0f+5.0f+20.0f*(float)m_FetchTasks.size()), 0.75f, 1.0f);
	Screen.VMargin(Screen.w/3.0f/**ding*/, &Screen);
	//Screen.x = UI()->Screen()->w/2-Screen.w/2;

	Screen.y += 5.0f;
	RenderTools()->DrawUIRect(&Screen, vec4(0,0,0,0.3f), CUI::CORNER_T, 5.0f);

	Screen.VMargin(10.0f, &Screen);
	TextRender()->TextColor(0.8f, 0.8f, 0.8f, 0.8f);
	Screen.HSplitTop(5.0f, 0, &Screen);
	Screen.HSplitTop(15.0f, &Button, &Screen);
	UI()->DoLabelScaled(&Button, Localize("Skin Downloads"), 14.0f, -1);

	for(int i = 0; i < m_apFetchTasks.size(); i++)
	//for(std::map<CFetchTask*, SkinFetchTask>::iterator it = m_FetchTasks.begin(); it != m_FetchTasks.end(); it++)
	{
		CSkinFetchTask *e = m_apFetchTasks[i];
		if(e->State() == CFetchTask::STATE_ERROR)
			TextRender()->TextColor(1,0,0,1);
		if(e->State() == CFetchTask::STATE_DONE)
			TextRender()->TextColor(0,1,0,1);
		if(e->State() == CFetchTask::STATE_QUEUED)
			TextRender()->TextColor(1,1,0,1);
		if(e->State() == CFetchTask::STATE_RUNNING)
			TextRender()->TextColor(0.3f, 0.3f, 0.6f, 1);
		if(e->State() == CFetchTask::STATE_ABORTED)
			TextRender()->TextColor(0.7f, 0.2f, 0.7f, 1);

		Screen.HSplitTop(5.0f, 0, &Screen);
		Screen.HSplitTop(13.5f, &Button, &Screen);
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "%s (try %i/%i)", e->SkinName(), e->Url()+1, m_SkinDbUrls.size());

		UI()->DoLabelScaled(&Button, aBuf, 12.0f, -1);

		if(e->State() == CFetchTask::STATE_RUNNING)
		{
			Screen.HSplitTop(3.0f, 0, &Screen);
			Screen.HSplitTop(2.0f, &Button, &Screen);
			RenderTools()->DrawUIRect(&Button, vec4(0.2f, 0.2f, 0.8f, 0.9f), 0, 0);
			Button.w *= (float)e->Progress()/100.0f;
			RenderTools()->DrawUIRect(&Button, vec4(0.4f, 0.4f, 0.9f, 1.0f), 0, 0);
		}

		if(e->FinishTime() > 0 && time_get() > e->FinishTime() + 2 * time_freq())
		{
			delete m_apFetchTasks[i];
			m_apFetchTasks.remove_index_fast(i);
		}
	}

	TextRender()->TextColor(1,1,1,1);

	lock_unlock(m_Lock);
}

void CSkinDownload::CSkinFetchTask::ProgressCallback(CFetchTask *pTask, void *pUser)
{
	CSkinFetchTask *pSelf = (CSkinFetchTask *)pUser;
	//lock_wait(pSelf->m_Lock); // we don't really need a lock here, do we?

	pSelf->m_State = pTask->State();
	pSelf->m_Progress = pTask->Progress();

	//lock_unlock(pSelf->m_Lock);
}

void CSkinDownload::CompletionCallback(CFetchTask *pTask, void *pUser)
{
	CSkinDownload *pSelf = (CSkinDownload *)pUser;

	const char *pDest = pTask->Dest();

	lock_wait(pSelf->m_Lock);
	CSkinFetchTask *pTaskHandler = pSelf->FindTask(pTask);
	lock_unlock(pSelf->m_Lock);
	if(!pTaskHandler)
	{
		dbg_msg("SKINFETCHER/ERROR", "Something really bad happened. I have no clue how that comes. I'm sorry.");
		delete pTask;
		return;
	}

	CSkinDownload::CSkinFetchTask::ProgressCallback(pTask, pTaskHandler);

	if(pTask->State() == CFetchTask::STATE_ERROR)
	{
		if(g_Config.m_Debug)
			pSelf->m_pStorage->RemoveBinaryFile(pDest); // delete the empty file dummy
		dbg_msg("skinfetcher/debug", "download failed: '%s'", pDest);
		pSelf->FetchNext(pTaskHandler);
	}

	if(pTask->State() == CFetchTask::STATE_DONE)
	{
		if(g_Config.m_Debug)
			dbg_msg("skinfetcher/debug", "download finished: '%s'", pDest);
		pSelf->GameClient()->m_pSkins->RefreshSkinList(false);
		pTaskHandler->Finish();
	}
}

void CSkinDownload::RequestSkin(int *pDestID, const char *pName)
{
	int DefaultSkin = GameClient()->m_pSkins->Find("default");

	if(!g_Config.m_ClSkinFetcher)
	{
		*pDestID = DefaultSkin;
		return;
	}

	int SkinID = GameClient()->m_pSkins->Find(pName);
	if(SkinID >= 0)
	{
		*pDestID = SkinID;
		return;
	}
	else
		*pDestID = DefaultSkin;

	if(m_apFetchTasks.size() >= MAX_FETCHTASKS)
		return;

	if(lock_trylock(m_Lock) != 0) // try again when lock is available
		return;

	// don't rerun failed tasks
	for(int i = 0; i < m_FailedTasks.size(); i++)
		if(str_comp_nocase(m_FailedTasks[i].c_str(), pName) == 0)
		{
			lock_unlock(m_Lock);
			return;
		}

	// don't queue tasks multiple times
	for(int i = 0; i < m_apFetchTasks.size(); i++)
		if(str_comp_nocase(m_apFetchTasks[i]->SkinName(), pName) == 0)
		{
			lock_unlock(m_Lock);
			return;
		}

	int i = m_apFetchTasks.add(new CSkinFetchTask(pName));
	lock_unlock(m_Lock);
	FetchSkin(m_apFetchTasks[i]);
}

void CSkinDownload::FetchNext(CSkinFetchTask *pTaskHandler) // doesn't lock!
{
	dbg_assert(pTaskHandler != NULL, "CSkinDownload::FetchNext called with pTaskHandler == NULL");

	if(pTaskHandler->Url()+1 >= m_SkinDbUrls.size())
	{
		pTaskHandler->Finish();
		Fail(pTaskHandler->SkinName());
		return;
	}

	dbg_msg("skinfetcher/debug", "trying next url (%i/%i): '%s'", pTaskHandler->Url()+2, m_SkinDbUrls.size(), m_SkinDbUrls[pTaskHandler->Url()+1].aUrl);

	pTaskHandler->Next();
	FetchSkin(pTaskHandler);
}

void CSkinDownload::FetchSkin(CSkinFetchTask *pTaskHandler) // doesn't lock!
{
	dbg_assert(pTaskHandler != NULL, "CSkinDownload::FetchSkin called with pTaskHandler == NULL");

	if(pTaskHandler->Url() >= m_SkinDbUrls.size())
		return;

	char aBuf[256];
	char aDestPath[256] = {0};
	char aFullPath[512] = {0};

	str_copy(aDestPath, m_SkinDbUrls[pTaskHandler->Url()].aUrl, sizeof(aDestPath));
	if(aDestPath[str_length(aDestPath)-1] != '/')
		str_append(aDestPath, "/", sizeof(aDestPath));
	char aEscapedName[128] = {0};
	m_pFetcher->Escape(aEscapedName, sizeof(aEscapedName), pTaskHandler->SkinName());
	str_format(aBuf, sizeof(aBuf), "%s%s.png", aDestPath, aEscapedName);

	if(g_Config.m_Debug)
		dbg_msg("skinfetcher/debug", "fetching file from '%s'", aBuf);

	str_format(aDestPath, sizeof(aDestPath), "downloadedskins/%s.png", pTaskHandler->SkinName());
	IOHANDLE f = Storage()->OpenFile(aDestPath, IOFLAG_WRITE, IStorageTW::TYPE_SAVE, aFullPath, sizeof(aFullPath));
	if(f)
	{
		io_close(f);
		//Storage()->RemoveBinaryFile(aFullPath);
	}
	else
	{
		dbg_msg("skinfetcher/error", "cannot write to '%s'", aFullPath);
		return;
	}

	m_pFetcher->QueueAdd(pTaskHandler->Task(), aBuf, aFullPath, -2, this, &CSkinDownload::CompletionCallback, &CSkinDownload::CSkinFetchTask::ProgressCallback);
}

void CSkinDownload::LoadUrls()
{
	m_SkinDbUrls.clear();

	int prior = 0;
	std::string line;
	std::ifstream file(g_Config.m_ClSkinDbFile);
	if(file.is_open())
	{
		while(std::getline(file, line))
		{
			if(line == "" || line.c_str()[0] == '#' || str_length(line.c_str()) > MAX_URL_LEN)
				continue;

			SkinDbUrl e;
			e.prior = prior++;
			str_copy(e.aUrl, line.c_str(), sizeof(e.aUrl));
			m_SkinDbUrls.add_unsorted(e);
		}
		file.close();

		m_SkinDbUrls.sort_range();
		dbg_msg("skinfetcher", "loaded %i url%s from file '%s'", prior, prior > 1 ? "s" : "", g_Config.m_ClSkinDbFile);
	}
	else
		dbg_msg("skinfetcher/error", "failed to open url file '%s', using ddnet's database only", g_Config.m_ClSkinDbFile);

	if(m_SkinDbUrls.size() == 0)
	{
		SkinDbUrl e;
		e.prior = 0;
		str_copy(e.aUrl, "https://ddnet.tw/skins/skin/", sizeof(e.aUrl));
		m_SkinDbUrls.add(e);
	}
}

void CSkinDownload::ConFetchSkin(IConsole::IResult *pResult, void *pUserData)
{
	CSkinDownload *pSelf = (CSkinDownload *)pUserData;
	if(!g_Config.m_ClSkinFetcher)
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "skinfetcher", "You must turn on cl_skin_fetcher to use this command");
	else
	{
		lock_wait(pSelf->m_Lock);
		int i = pSelf->m_apFetchTasks.add(new CSkinFetchTask(pResult->GetString(0)));
		lock_unlock(pSelf->m_Lock);
		pSelf->FetchSkin(pSelf->m_apFetchTasks[i]);
	}
}
