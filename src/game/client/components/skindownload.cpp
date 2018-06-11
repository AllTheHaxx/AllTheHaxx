#include <sstream>
#include <fstream>

#include <engine/graphics.h>
#include <engine/textrender.h>

#include "skins.h"
#include "skindownload.h"


void CSkinDownload::OnInit()
{
	m_pFetcher = Kernel()->RequestInterface<IFetcher>();
	m_pStorage = Kernel()->RequestInterface<IStorageTW>();

	mem_zero(m_apFetchTasks, sizeof(m_apFetchTasks));

	m_DefaultSkin = -1;
	LoadUrls();
}

CSkinDownload::~CSkinDownload()
{
	LOCK_SECTION_MUTEX(m_FetchTasksLock)
	for(int i = 0; i < MAX_FETCHTASKS; i++)
	{
		if(m_apFetchTasks[i])
		{
			delete m_apFetchTasks[i];
			m_apFetchTasks[i] = NULL;
		}
	}
}

void CSkinDownload::OnConsoleInit()
{
	Console()->Register("skinfetcher_request", "s[skinname]", CFGFLAG_CLIENT, ConFetchSkin, this, "Fetch a skin by name from the available databases");
#if defined(CONF_DEBUG)
	Console()->Register("skinfetcher_spam", "i[num]", CFGFLAG_CLIENT, ConDbgSpam, this, "Spam random skin download requests for debugging purposes");
#endif
}

void CSkinDownload::OnRender()
{
	if(!g_Config.m_ClSkinFetcherUi || Client()->State() != IClient::STATE_ONLINE)
		return;

	const int NUM_TASKS = NumTasks();
	const float MAX_HEIGHT = NUM_TASKS == 0 ? 0.001f : 5.0f + 25.0f + 5.0f + 20.0f * (float)NUM_TASKS;

	static float s_SmoothPos = 0.0f;
	smooth_set(&s_SmoothPos, MAX_HEIGHT, 23.0f, Client()->RenderFrameTime());

	if(s_SmoothPos <= 0.01f)
	{
		return;
	}

	CUIRect Screen = *(UI()->Screen()), Button;
	Graphics()->MapScreen(0, 0, Screen.w, Screen.h);
	Screen.HSplitBottom(s_SmoothPos, 0, &Screen);
	//float dings = clamp(s_SmoothPos/(5.0f+25.0f+5.0f+20.0f*(float)m_FetchTasks.size()), 0.75f, 1.0f);
	Screen.VMargin(Screen.w / 3.0f, &Screen);
	//Screen.x = UI()->Screen()->w/2-Screen.w/2;

	Screen.y += 5.0f;
	RenderTools()->DrawUIRect(&Screen, vec4(0, 0, 0, 0.3f), CUI::CORNER_T, 5.0f);

	Screen.VMargin(10.0f, &Screen);
	TextRender()->TextColor(0.8f, 0.8f, 0.8f, 0.8f);
	Screen.HSplitTop(5.0f, 0, &Screen);
	Screen.HSplitTop(15.0f, &Button, &Screen);
	UI()->DoLabelScaled(&Button, Localize("Skin Downloads"), 14.0f, -1);

	for(int i = 0; i < MAX_FETCHTASKS; i++)
	{
		CSkinFetchTask *e = m_apFetchTasks[i];
		if(!e)
			continue;

		int S = e->State();
		if(S == CFetchTask::STATE_ERROR)
			TextRender()->TextColor(1,0,0,1);
		else if(S == CFetchTask::STATE_DONE)
			TextRender()->TextColor(0,1,0,1);
		else if(S == CFetchTask::STATE_QUEUED)
			TextRender()->TextColor(1,1,0,1);
		else if(S == CFetchTask::STATE_RUNNING)
			TextRender()->TextColor(0.3f, 0.3f, 0.6f, 1);
		else if(S == CFetchTask::STATE_ABORTED)
			TextRender()->TextColor(0.7f, 0.2f, 0.7f, 1);

		// make sure that tasks don't stay in the list for ever
		if(S != CFetchTask::STATE_QUEUED && S != CFetchTask::STATE_RUNNING && e->FinishTime() < 0)
			e->Finish();

		Screen.HSplitTop(5.0f, 0, &Screen);
		Screen.HSplitTop(13.5f, &Button, &Screen);
		char aBuf[128];
		if(NumURLs() > 1)
			str_format(aBuf, sizeof(aBuf), "%s (database %i/%i)", e->SkinName(), e->Url()+1, NumURLs());
		else
			str_format(aBuf, sizeof(aBuf), "%s", e->SkinName());

		UI()->DoLabelScaled(&Button, aBuf, 12.0f, -1);

		// draw a progress bar for running tasks
		if(e->State() == CFetchTask::STATE_RUNNING)
		{
			Screen.HSplitTop(3.0f, 0, &Screen);
			Screen.HSplitTop(2.0f, &Button, &Screen);
			RenderTools()->DrawUIRect(&Button, vec4(0.2f, 0.2f, 0.8f, 0.9f), 0, 0);
			Button.w *= (float)e->Progress()/100.0f;
			RenderTools()->DrawUIRect(&Button, vec4(0.4f, 0.4f, 0.9f, 1.0f), 0, 0);
		}

		// delete tasks 2 seconds after they've finished
		if(e->FinishTime() > 0 && time_get() > e->FinishTime() + 2 * time_freq())
		{
			LOCK_SECTION_MUTEX(m_FetchTasksLock)
			delete m_apFetchTasks[i];
			m_apFetchTasks[i] = NULL;
		}
	}

	TextRender()->TextColor(1,1,1,1);

}

void CSkinDownload::CSkinFetchTask::ProgressCallback(CFetchTask *pTask, void *pUser)
{
	CSkinDownload *pSelf = (CSkinDownload *)pUser;

	CSkinFetchTask *pTaskHandler = pSelf->FindTask(pTask);
	if(!pTaskHandler)
	{
		dbg_msg("skinfetcher/error", "FATAL: NO HANDLER FOR TASK %p", pTask);
		return;
	}

	pTaskHandler->m_State = pTask->State();
	pTaskHandler->m_Progress = pTask->Progress();
}

void CSkinDownload::CompletionCallback(CFetchTask *pTask, void *pUser)
{
	CSkinDownload *pSelf = (CSkinDownload *)pUser;

	const char *pDest = pTask->Dest();

	CSkinFetchTask *pTaskHandler = pSelf->FindTask(pTask);
	if(!pTaskHandler)
	{
		dbg_msg("SKINFETCHER/ERROR", "Something really bad happened. I have no clue how that comes. I'm sorry.");
		dbg_msg("SKINFETCHER/ERROR", "INFO: pTask@%p={ dest='%s' curr=%.2f, size=%.2f }", pTask, pTask->Dest(), pTask->Current(), pTask->Size());
		return;
	}

	CSkinDownload::CSkinFetchTask::ProgressCallback(pTask, pSelf);

	if(pTask->State() == CFetchTask::STATE_ERROR || pTask->State() == CFetchTask::STATE_ABORTED)
	{
		if(g_Config.m_Debug)
			dbg_msg("skinfetcher/debug", "download failed: '%s'", pDest);

		if(!pSelf->FetchNext(pTaskHandler))
			pSelf->m_pStorage->RemoveBinaryFile(pDest); // delete the empty file dummy
	}
	else if(pTask->State() == CFetchTask::STATE_DONE)
	{
		if(g_Config.m_Debug)
			dbg_msg("skinfetcher/debug", "download finished: '%s'", pDest);
		pSelf->GameClient()->m_pSkins->RefreshSkinList(false);
		pTaskHandler->Finish();
	}
}

void CSkinDownload::RequestSkin(int *pDestID, const char *pName)
{
	if(m_DefaultSkin < 0)
		m_DefaultSkin = GameClient()->m_pSkins->Find("default");
	*pDestID = m_DefaultSkin;

	// search for the wanted skin
	int SkinID = GameClient()->m_pSkins->Find(pName);
	if(SkinID >= 0)
	{
		*pDestID = SkinID;
		return;
	}

	// don't fetch anything if it's disabled or the tasklist is full
	if(!g_Config.m_ClSkinFetcher || g_Config.m_ClVanillaSkinsOnly || !pName || pName[0] == '\0')
		return;

	if(NumTasks() >= MAX_FETCHTASKS/* || NumTasks(true) >= MAX_ACTIVE_TASKS*/)
		return;

	// don't rerun failed tasks
	{
		LOCK_SECTION_MUTEX(m_FailedTasksLock)
		for(int i = 0; i < m_FailedTasks.size(); i++)
			if(str_comp_nocase(m_FailedTasks[i].c_str(), pName) == 0)
				return;
	}

	// protect against malicious skin names
	while(*pName && (*pName == '.' || *pName == '/'))
		pName++;
#if defined(CONF_FAMILY_WINDOWS)
	if(	str_find(pName, "/") || str_find(pName, "*") || str_find(pName, "\"") || str_find(pName, "?") ||
		str_find(pName, "\\") || str_find(pName, ":") || str_find(pName, ">") || str_find(pName, "<") || str_find(pName, "|"))
	{
		dbg_msg("skinfetcher", "couldn't fetch skin '%s': invalid name", pName);
		Fail(pName);
		return;
	}
#endif

	// don't queue tasks multiple times
	{
		LOCK_SECTION_MUTEX(m_FetchTasksLock)
		for(int i = 0; i < MAX_FETCHTASKS; i++)
			if(m_apFetchTasks[i])
				if(str_comp_nocase(m_apFetchTasks[i]->SkinName(), pName) == 0)
					return;
	}

	CSkinFetchTask **ppSlot = FindFreeSlot();
	if(!ppSlot) // this check shouldn't be necessary... but safe is safe :/
	{
		dbg_msg("skinfetcher/debug", "!! WARNING !! > Couldn't find a free slot for new task '%s' (got %i/%i tasks)", pName, NumTasks(), MAX_FETCHTASKS);
		return;
	}

	CSkinFetchTask *pTask = new CSkinFetchTask(pName);
	*ppSlot = pTask;
	FetchSkin(pTask);
}

bool CSkinDownload::FetchNext(CSkinFetchTask *pTaskHandler)
{
	dbg_assert(pTaskHandler != NULL, "CSkinDownload::FetchNext called with pTaskHandler == NULL");

	if(pTaskHandler->Url()+1 >= NumURLs())
	{
		pTaskHandler->Finish();
		Fail(pTaskHandler->SkinName());
		return false;
	}

	pTaskHandler->Next();
	dbg_msg("skinfetcher/debug", "trying next url (%i/%i): '%s' for '%s.png'", pTaskHandler->Url()+1, NumURLs(), GetURL(pTaskHandler->Url()), pTaskHandler->SkinName());

	FetchSkin(pTaskHandler);

	return true;
}

void CSkinDownload::FetchSkin(CSkinFetchTask *pTaskHandler)
{
	dbg_assert(pTaskHandler != NULL, "CSkinDownload::FetchSkin called with pTaskHandler == NULL");

	if(pTaskHandler->Url() >= NumURLs())
		return;

	const char * const pURL = GetURL(pTaskHandler->Url());

	char aDestPath[256];
	str_copy(aDestPath, pURL, sizeof(aDestPath));
	//if(str_length(aDestPath) <= 0)
	//	return;

	if(aDestPath[str_length(aDestPath)-1] != '/')
		str_append(aDestPath, "/", sizeof(aDestPath));

	char aEscapedName[128];
	m_pFetcher->Escape(aEscapedName, sizeof(aEscapedName), pTaskHandler->SkinName());

	char aUrl[256];
	str_format(aUrl, sizeof(aUrl), "%s%s.png", aDestPath, aEscapedName);

	if(g_Config.m_Debug)
		dbg_msg("skinfetcher/debug", "fetching file from '%s' (url=%i '%s')", aUrl, pTaskHandler->Url(), pURL);

	str_format(aDestPath, sizeof(aDestPath), "downloadedskins/%s.png", pTaskHandler->SkinName());

	char aFullPath[512];
	IOHANDLE f = Storage()->OpenFile(aDestPath, IOFLAG_WRITE, IStorageTW::TYPE_SAVE, aFullPath, sizeof(aFullPath));
	if(f)
	{
		io_close(f);
	}
	else
	{
		dbg_msg("skinfetcher/error", "cannot write to '%s'", aFullPath);
		pTaskHandler->Invalidate();
		Fail(pTaskHandler->SkinName());
		return;
	}

	pTaskHandler->m_pCurlTask = m_pFetcher->QueueAdd(false, aUrl, aFullPath, -2, this, &CSkinDownload::CompletionCallback, &CSkinDownload::CSkinFetchTask::ProgressCallback);
}

void CSkinDownload::LoadUrls()
{
	m_aSkinDbUrls.clear();

	const char * const pSkinDbFile = STORAGE_EDTC_DIR "/skindbs.cfg";

	std::string line;
	std::ifstream file(pSkinDbFile);
	if(file.is_open())
	{
		while(std::getline(file, line))
		{
			const char *pLine = str_skip_whitespaces_const(line.c_str());

			if(pLine[0] == '\0' || pLine[0] == '#')
				continue;

			m_aSkinDbUrls.add(std::string(pLine));
		}
		file.close();

		const int Num = NumURLs();
		dbg_msg("skinfetcher", "loaded %i url%s from file '%s'", Num, Num > 1 ? "s" : "", pSkinDbFile);
		if(g_Config.m_Debug)
			for(int i = 0; i < Num; i++)
				dbg_msg("skinfecher/debug", "  > %i:'%s'", i, GetURL(i));
	}
	else
		dbg_msg("skinfetcher/error", "failed to open url file '%s', using ddnet's database only", pSkinDbFile);

	if(NumURLs() == 0)
	{
		m_aSkinDbUrls.add(std::string("https://ddnet.tw/skins/skin/"));
	}
}

void CSkinDownload::ConFetchSkin(IConsole::IResult *pResult, void *pUserData)
{
	CSkinDownload *pSelf = (CSkinDownload *)pUserData;
	if(!g_Config.m_ClSkinFetcher)
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "skinfetcher", "You must turn on cl_skin_fetcher to use this command");
	else
	{
		const char *pSkinName = pResult->GetString(0);
		int SkinID;
		pSelf->RequestSkin(&SkinID, pSkinName);
		if(SkinID != pSelf->m_DefaultSkin && SkinID >= 0)
			pSelf->Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "skinfetcher", "already got the skin '%s'", pSkinName);
		else
			pSelf->Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "skinfetcher", "trying to fetch '%s'", pSkinName);
	}
}

void CSkinDownload::ConDbgSpam(IConsole::IResult *pResult, void *pUserData)
{
	CSkinDownload *pSelf = (CSkinDownload *)pUserData;
	if(!g_Config.m_ClSkinFetcher)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "skinfetcher", "You must turn on cl_skin_fetcher to use this command");
		return;
	}

	pSelf->Console()->Printf(IConsole::OUTPUT_LEVEL_STANDARD, "skinfetcher", "Creating %i skin fetching requests... (not all may get registered)", pResult->GetInteger(0));
	for(int i = 0; i < pResult->GetInteger(0); i++)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "%i%02x%f-%i", i, i, (float)i/123.0f, rand());
		int Dest;
		pSelf->RequestSkin(&Dest, aBuf);
	}
}

void CSkinDownload::Fail(const char *pSkinName)
{
	LOCK_SECTION_MUTEX(m_FailedTasksLock)
	m_FailedTasks.add(std::string(pSkinName));
}

int CSkinDownload::NumTasks()
{
	LOCK_SECTION_MUTEX(m_FetchTasksLock)
	int ret = 0;
	for(int i = 0; i < MAX_FETCHTASKS; i++)
		if(m_apFetchTasks[i] != NULL)
			ret++;
	return ret;
}

CSkinDownload::CSkinFetchTask *CSkinDownload::FindTask(const CFetchTask *pTask)
{
	LOCK_SECTION_MUTEX(m_FetchTasksLock)
	for(int i = 0; i < MAX_FETCHTASKS; i++)
		if(m_apFetchTasks[i] != NULL)
			if(m_apFetchTasks[i]->Task() == pTask)
				return m_apFetchTasks[i];
	return NULL;
}

CSkinDownload::CSkinFetchTask **CSkinDownload::FindFreeSlot()
{
	LOCK_SECTION_MUTEX(m_FetchTasksLock)
	for(int i = 0; i < MAX_FETCHTASKS; i++)
		if(m_apFetchTasks[i] == NULL)
			return &(m_apFetchTasks[i]);
	return NULL;
}
