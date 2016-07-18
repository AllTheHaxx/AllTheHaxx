
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
	m_FetchTasks.clear();
	m_FetchTasks.hint_size(5);

	LoadUrls();
}

void CSkinDownload::OnRender()
{
	if(lock_trylock(m_Lock) != 0) // just don't render if it's locked, but don't block!
		return;

	const float MAX_HEIGHT = m_FetchTasks.empty() ? 0.001f : 5.0f+25.0f+5.0f+20.0f*(float)m_FetchTasks.size();
	static float s_SmoothPos = 0.0f;
	smooth_set(&s_SmoothPos, MAX_HEIGHT, (0.005f/Client()->RenderFrameTime())*23.0f);

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

	for(int i = 0; i < m_FetchTasks.size(); i++)
	//for(std::map<CFetchTask*, SkinFetchTask>::iterator it = m_FetchTasks.begin(); it != m_FetchTasks.end(); it++)
	{
		SkinFetchTask *e = &m_FetchTasks[i];
		if(e->State == CFetchTask::STATE_ERROR)
			TextRender()->TextColor(1,0,0,1);
		if(e->State == CFetchTask::STATE_DONE)
			TextRender()->TextColor(0,1,0,1);
		if(e->State == CFetchTask::STATE_QUEUED)
			TextRender()->TextColor(1,1,0,1);

		Screen.HSplitTop(5.0f, 0, &Screen);
		Screen.HSplitTop(13.5f, &Button, &Screen);
		char aBuf[128];
		if(e->url > 0)
			str_format(aBuf, sizeof(aBuf), "%s (try %i/%i)", e->SkinName.c_str(), e->url+1, m_SkinDbUrls.size());
		else
			str_format(aBuf, sizeof(aBuf), "%s", e->SkinName.c_str());

		UI()->DoLabelScaled(&Button, aBuf, 12.0f, -1);

		if(e->State == CFetchTask::STATE_RUNNING)
		{
			Screen.HSplitTop(3.0f, 0, &Screen);
			Screen.HSplitTop(2.0f, &Button, &Screen);
			RenderTools()->DrawUIRect(&Button, vec4(0.2f, 0.2f, 0.8f, 0.9f), 0, 0);
			Button.w *= (float)e->Progress/100.0f;
			RenderTools()->DrawUIRect(&Button, vec4(0.4f, 0.4f, 0.9f, 1.0f), 0, 0);
		}

		if(e->FinishTime > 0 && time_get() > e->FinishTime + 2 * time_freq())
			m_FetchTasks.remove_index_fast(i);
	}

	TextRender()->TextColor(1,1,1,1);

	lock_unlock(m_Lock);
}

void CSkinDownload::ProgressCallback(CFetchTask *pTask, void *pUser)
{
	CSkinDownload *pSelf = (CSkinDownload *)pUser;
	//lock_wait(pSelf->m_Lock); // we don't really need a lock here, do we?

	pSelf->FindTask(pTask)->State = pTask->State();
	pSelf->FindTask(pTask)->Progress = pTask->Progress();

	//lock_unlock(pSelf->m_Lock);
}

void CSkinDownload::CompletionCallback(CFetchTask *pTask, void *pUser)
{
	CSkinDownload *pSelf = (CSkinDownload *)pUser;
	lock_wait(pSelf->m_Lock);

	const char *dest = pTask->Dest();
	SkinFetchTask *pTaskHandler = pSelf->FindTask(pTask);
	pTaskHandler->State = pTask->State();

	if(pTask->State() == CFetchTask::STATE_ERROR)
	{
		pSelf->m_pStorage->RemoveBinaryFile(dest); // delete the empty file dummy
		dbg_msg("skinfetcher/debug", "download failed: '%s'", dest);
		if(pTaskHandler->url+1 < pSelf->m_SkinDbUrls.size())
		{
			dbg_msg("skinfetcher/debug", "trying next url (%i/%i): '%s'", pTaskHandler->url+2, pSelf->m_SkinDbUrls.size(), pSelf->m_SkinDbUrls[pTaskHandler->url+1].url.c_str());
			lock_unlock(pSelf->m_Lock); // temporarily unlock to allow recursion
			pSelf->FetchSkin(pTaskHandler->SkinName.c_str(), pTaskHandler->pDestID, pTaskHandler->url+1);
			lock_wait(pSelf->m_Lock); // get the lock back to go on
		}
		else
		{
			if(pTaskHandler->pDestID)
				*(pTaskHandler->pDestID) = pSelf->GameClient()->m_pSkins->Find("default");
			pSelf->m_FailedTasks.add(pTaskHandler->SkinName);
		}
	}

	if(pTask->State() == CFetchTask::STATE_DONE)
	{
		if(g_Config.m_Debug)
			dbg_msg("skinfetcher/debug", "download finished: '%s'", dest);
		pSelf->GameClient()->m_pSkins->RefreshSkinList(false);
		if(pTaskHandler->pDestID)
			*(pTaskHandler->pDestID) = pSelf->GameClient()->m_pSkins->Find(pTaskHandler->SkinName.c_str());
	}

	pTaskHandler->FinishTime = time_get();

	lock_unlock(pSelf->m_Lock);
	delete pTask;
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

	if(lock_trylock(m_Lock) != 0) // try again when lock is available
		return;

	// don't rerun failed tasks
	for(int i = 0; i < m_FailedTasks.size(); i++)
		if(m_FailedTasks[i] == std::string(pName))
		{
			lock_unlock(m_Lock);
			return;
		}

	// don't queue tasks multiple times
	for(int i = 0; i < m_FetchTasks.size(); i++)
		if(str_comp(m_FetchTasks[i].SkinName.c_str(), pName) == 0)
		{
			lock_unlock(m_Lock);
			return;
		}

	lock_unlock(m_Lock);
	FetchSkin(pName, pDestID);
}

void CSkinDownload::FetchSkin(const char *pName, int *pDestID, int url)
{
	if(url >= m_SkinDbUrls.size())
		return;

	if(m_FetchTasks.size() > MAX_FETCHTASKS)
		return;

	char aBuf[256];
	char aDestPath[256] = {0};
	char aFullPath[512] = {0};

	str_copy(aDestPath, m_SkinDbUrls[url].url.c_str(), sizeof(aDestPath));
	if(aDestPath[str_length(aDestPath)-1] != '/')
		str_append(aDestPath, "/", sizeof(aDestPath));
	char aEscapedName[128] = {0};
	m_pFetcher->Escape(aEscapedName, sizeof(aEscapedName), pName);
	str_format(aBuf, sizeof(aBuf), "%s%s.png", aDestPath, aEscapedName);

	if(g_Config.m_Debug)
		dbg_msg("skinfetcher/debug", "fetching file from '%s'", aBuf);

	str_format(aDestPath, sizeof(aDestPath), "downloadedskins/%s.png", pName);
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

	CFetchTask *pTask = new CFetchTask(false);
	SkinFetchTask Task;
	Task.SkinName = std::string(pName);
	Task.url = url;
	Task.Progress = 0;
	Task.FinishTime = -1;
	Task.pDestID = pDestID;
	Task.pCurlTask = pTask;
	lock_wait(m_Lock);
	m_FetchTasks.add(Task);
	lock_unlock(m_Lock);
	m_pFetcher->QueueAdd(pTask, aBuf, aFullPath, -2, this, &CSkinDownload::CompletionCallback, &CSkinDownload::ProgressCallback);
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
			if(line == "" || line.c_str()[0] == '#')
				continue;

			//line = line.replace(line.begin(), line.end(), "\n", "\0");
			SkinDbUrl e;
			e.prior = prior++;
			e.url = line;
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
		e.url = std::string("https://ddnet.tw/skins/skin/");
		m_SkinDbUrls.add(e);
	}
}

void CSkinDownload::ConFetchSkin(IConsole::IResult *pResult, void *pUserData)
{
	CSkinDownload *pSelf = (CSkinDownload *)pUserData;
	if(!g_Config.m_ClSkinFetcher)
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "skinfetcher", "You must turn on cl_skin_fetcher to use this command");
	else
		pSelf->FetchSkin(pResult->GetString(0));
}
