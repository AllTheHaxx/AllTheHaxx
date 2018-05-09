#include <base/system++/system++.h>

#include "io.h"

#if defined(CONF_DEBUG)
#define RETURN_ON_NOT_OPEN(RET) dbg_assert(IsOpen(), "operating on a closed file");
#else
#define RETURN_ON_NOT_OPEN(RET) if(!m_FileHandle) return RET;
#endif


bool IOHANDLE_SMART::Open(int Flags)
{
	if(m_FileHandle)
		return false;
	m_FileHandle = io_open(m_FilePath.c_str(), Flags);
	return m_FileHandle != NULL;
}

bool IOHANDLE_SMART::Open(const char *pFilename, int Flags)
{
	if(m_FileHandle)
		return false;
	m_FilePath = std::string(pFilename);
	m_FileHandle = io_open(pFilename, Flags);
	return m_FileHandle != NULL;
}

unsigned IOHANDLE_SMART::Read(void *pBuffer, unsigned Size)
{
	RETURN_ON_NOT_OPEN(0)
	return io_read(m_FileHandle, pBuffer, Size);
}

const std::string IOHANDLE_SMART::ReadText(unsigned Size)
{
	char aHugeBuf[64*1024];
	mem_zerob(aHugeBuf);
	if(Size > sizeof(aHugeBuf))
		Size = sizeof(aHugeBuf);
	Read(aHugeBuf, Size);
	return std::string(aHugeBuf);
}

const std::string IOHANDLE_SMART::ReadAllText()
{
	char *pBuf = ReadAllTextRaw();
	std::string AllText(pBuf);
	mem_free(pBuf);
	return AllText;
}

char *IOHANDLE_SMART::ReadAllTextRaw(unsigned int *pLen)
{
	RETURN_ON_NOT_OPEN(NULL);
	long TextLen = Length();
	if(TextLen <= 0)
		return NULL;

	if(pLen)
		*pLen = (unsigned int)TextLen;

	char *pBuf = mem_allocb(char, TextLen + 1);
	Seek(0, IOSEEK_START);
	Read(pBuf, (unsigned int)TextLen);
	pBuf[TextLen] = '\0'; // ensure valid string termination; even text files don't necessarily need to have that

	return pBuf;
}

bool IOHANDLE_SMART::ReadNextLine(std::string *pDest)
{
	RETURN_ON_NOT_OPEN(false)
	while(true)
	{
		char c[2] = {0,0};
		if(io_read(m_FileHandle, c, 1U) != 1 || c[0] == '\0')
			return false;

		if(c[0] == '\n' || c[0] == '\r')
			return true;
		else
			pDest->append(c);
	}
	// unreachable //
}

bool IOHANDLE_SMART::ReadNextLine(char *pBuffer, unsigned BufferSize)
{
	RETURN_ON_NOT_OPEN(false)
	mem_zero(pBuffer, BufferSize);
	for(unsigned i = 0; i < BufferSize-1; i++)
	{
		char c = '\0';
		if(io_read(m_FileHandle, &c, 1U) != 1 || c == '\0')
			return false;

		if(c == '\n' || c == '\r')
			return true;
		else
			pBuffer[i] = c;
	}
	return true;
}

unsigned IOHANDLE_SMART::Write(const void *pBuffer, unsigned Size) const
{
	RETURN_ON_NOT_OPEN(0)
	return io_write(m_FileHandle, pBuffer, Size);
}

unsigned IOHANDLE_SMART::WriteString(const char *pStr, bool Raw) const
{
	RETURN_ON_NOT_OPEN(0)
	return io_write(m_FileHandle, pStr, (unsigned int)str_length(pStr)+Raw);
}

unsigned IOHANDLE_SMART::WriteString(const std::string& Str, bool Raw) const
{
	RETURN_ON_NOT_OPEN(0)
	return io_write(m_FileHandle, Str.c_str(), (unsigned int)Str.length()+Raw);
}

unsigned IOHANDLE_SMART::WriteLine(const char *pText) const
{
	RETURN_ON_NOT_OPEN(0)
	unsigned Size = 0;
	Size += WriteString(pText, false);
	Size += WriteNewline();
	return Size;
}

unsigned IOHANDLE_SMART::WriteLine(const std::string& Str) const
{
	RETURN_ON_NOT_OPEN(0)
	unsigned Size = 0;
	Size += WriteString(Str, false);
	Size += WriteNewline();
	return Size;
}

unsigned IOHANDLE_SMART::WriteNewline() const
{
	RETURN_ON_NOT_OPEN(0)
	return io_write_newline(m_FileHandle);
}

bool IOHANDLE_SMART::Seek(long Offset, int Origin) const
{
	RETURN_ON_NOT_OPEN(false)
	return io_seek(m_FileHandle, Offset, Origin) == 0;
}

bool IOHANDLE_SMART::Skip(long Size) const
{
	RETURN_ON_NOT_OPEN(false)
	return Seek(Size, IOSEEK_CUR);
}

long int IOHANDLE_SMART::Tell() const
{
	RETURN_ON_NOT_OPEN(-1)
	return io_tell(m_FileHandle);
}

long int IOHANDLE_SMART::Length() const
{
	RETURN_ON_NOT_OPEN(-1)
	long int CurrPos = Tell();
	Seek(0, IOSEEK_END);
	long int Length = Tell();
	Seek(CurrPos, IOSEEK_START);
	return Length;
}

bool IOHANDLE_SMART::Flush() const
{
	RETURN_ON_NOT_OPEN(false)
	return io_flush(m_FileHandle) == 0;
}

bool IOHANDLE_SMART::Close()
{
	RETURN_ON_NOT_OPEN(false)
	int result = io_close(m_FileHandle);
	if(result)
	{
		char aError[512];
		net_err_str(aError, sizeof(aError), net_errno());
		dbg_msg("IOHANDLE_SMART/DEBUG", "::Close() file '%s', handle %p, ERROR: '%s'", GetPath(), m_FileHandle, aError);
	}
	m_FileHandle = NULL;
	return result == 0;
}

#undef RETURN_ON_NOT_OPEN
