#include <map>
#include <algorithm>

#include "system++.h"
#include <base/system.h>
#include <base/math.h>

CTWException::CTWException()
{
	str_copyb(m_aWhat, "unknown error");
#if defined(CONF_DEBUG)
	dbg_msg("exception", "an exception was thrown: %s", m_aWhat);
#endif
}

CTWException::CTWException(const char *msg)
{
	str_formatb(m_aWhat, "%s", msg);
#if defined(CONF_DEBUG)
	dbg_msg("exception", "an exception was thrown: %s", m_aWhat);
#endif
}

CTWException::CTWException(const char *pFilename, int Line, const char *pAssertStr, const char *pMsg)
{
	str_formatb(m_aWhat, "At %s(%d) assert '%s' failed: %s", pFilename, Line, pAssertStr, pMsg);
#if defined(CONF_DEBUG)
	dbg_msg("assert", "%s", m_aWhat);
#endif
}

const char *CTWException::what() const throw ()
{
	return m_aWhat;
}


void dbg_abort()
{
	wait_log_queue();
	throw CTWException("dbg_break");
}

void mem_debug_dump(IOHANDLE file)
{
#if defined(CONF_DEBUG)
	char aBuf[1024];
	MEMHEADER *header = mem_stats()->first;
	if(!file)
		file = io_open("memory.txt", IOFLAG_WRITE);
	if(!file)
		return;

	// gather data
	struct Integer
	{
		int value;
		Integer() : value(0) {}

		int operator+=(int val) { return value += val; }
	};
	std::map<std::string, int> DataMap;
	while(header)
	{
		str_format(aBuf, sizeof(aBuf), "%s(%d)", header->filename, header->line);
		std::string Key(aBuf);
		DataMap[Key] += header->size;

		header = header->next;
	}

	// copy it over into a list-type so we can sort it
	std::vector< std::pair<std::string, int> > DataList;
	for(std::map<std::string, int>::iterator it = DataMap.begin(); it != DataMap.end(); ++it)
	{
		DataList.emplace_back(std::pair<std::string, int>(it->first, it->second));
	}

	// sort our data by 'size'
	std::sort(DataList.begin(), DataList.end(), [](std::pair<std::string, int> a, std::pair<std::string, int> b) -> bool {
		return a.second == b.second
			   ? a.first.compare(b.first) < 0 // sort alphabetically if primary sorting condition is equal
			   : a.second > b.second; // primarily sort by the 'size'
	});

	// write the sorted list out onto the disk
	int TotalBytes = 0;
	int TotalLocations = 0;
	for(std::vector< std::pair<std::string, int> >::iterator it = DataList.begin(); it != DataList.end(); ++it)
	{
		str_formatb(aBuf, "%s: %i (%i KiB / %i MiB)", it->first.c_str(), it->second, it->second/1024, it->second/1024/1024);
		io_write(file, aBuf, (unsigned)str_length(aBuf));
		io_write_newline(file);

		TotalBytes += it->second;
		TotalLocations++;
	}

	// write overview
	io_write_newline(file);
	str_formatb(aBuf, "Got a total of %i bytes (%i KiB / %i MiB) from %i locations", TotalBytes, TotalBytes/1024, TotalBytes/1024/1024, TotalLocations);
	io_write(file, aBuf, (unsigned)str_length(aBuf));
	io_write_newline(file);

	io_close(file);
#endif
}

void StringSplit(const char *pString, const char *pDelim, std::vector<std::string> *pDest)
{
	const char *pFound = pString;
	const char *pLast = pString;
	while((pFound = str_find(pFound, pDelim)))
	{
		pFound++;
		char aPart[512];
		str_copy(aPart, pLast, (int)min<long unsigned int>((long unsigned int)sizeof(aPart), (long unsigned int)(pFound - pLast)));
		pDest->push_back(std::string(aPart));

		if(*(pLast = pFound) == '\0')
			break;
	}

	pDest->push_back(pLast);
}
