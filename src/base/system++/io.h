#ifndef BASE_SYSTEMPP_IO_H
#define BASE_SYSTEMPP_IO_H

#include <string>
#include <base/system.h>

/**
 * Wrapper for IOHANDLE and all io_ functions
 *
 * Provides multiple functionalities:
 * - Makes sure the file is closed when the handle goes out of scope
 * - Provides wrappers for all io_* functions that are crash save
 */
class IOHANDLE_SMART
{
	const std::string m_Filename;
	IOHANDLE m_FileHandle;
public:
	IOHANDLE_SMART(const char *pFilename, int Flags) : m_Filename(std::string(pFilename))
	{
		m_FileHandle = io_open(pFilename, Flags);
	}

	~IOHANDLE_SMART()
	{
		if(m_FileHandle)
			io_close(m_FileHandle);
	}

#define RETURN_ON_NOT_OPEN(RET) if(!m_FileHandle) return RET;

	/**
	 * Re-Open the file with the given flags
	 * @param Flags one of the IOFLAG_*
	 * @return true on success, false on error
	 * @note before using this function, you must use the Close function
	 */
	bool Open(int Flags)
	{
		if(m_FileHandle)
			return false;
		m_FileHandle = io_open(m_Filename.c_str(), Flags);
		return m_FileHandle != NULL;
	}

	/**
	 * Reads Size bytes from the file and stores it into pBuffer
	 * @param pBuffer a block of pre-allocated memory
	 * @param Size how many bytes to read at most
	 * @return the number of actual bytes read (0 on failure)
	 */
	unsigned Read(void *pBuffer, unsigned Size)
	{
		RETURN_ON_NOT_OPEN(0)
		return io_read(m_FileHandle, pBuffer, Size);
	}

	/**
	 * Reads Size bytes from the file, and stores it in an std::string
	 * @param Size nubmer of characters to read
	 * @return a string containing the text
	 */
	const std::string ReadText(unsigned Size)
	{
		char aHugeBuf[64*1024];
		mem_zerob(aHugeBuf);
		if(Size > sizeof(aHugeBuf))
			Size = sizeof(aHugeBuf);
		Read(aHugeBuf, Size);
		return std::string(aHugeBuf);
	}

	/**
	 * Writes Size bytes from pBuffer to the file, if it is open.
	 * @param pBuffer Data to write
	 * @param Size Number of bytes to write
	 * @return 0 if file is closed, otherwise the number of bytes written
	 */
	unsigned Write(const void *pBuffer, unsigned Size) const
	{
		RETURN_ON_NOT_OPEN(0)
		return io_write(m_FileHandle, pBuffer, Size);
	}

	/**
	 * Writes the text plus an appropriate newline to the file
	 * @param pText text to write
	 * @return number of bytes written, see Write
	 */
	unsigned WriteLine(const char *pText) const
	{
		RETURN_ON_NOT_OPEN(0)
		unsigned Size = 0;
		Size += Write(pText, (unsigned int)str_length(pText));
		Size += WriteNewline();
		return Size;
	}

	/**
	 * Writes a newline to the file, depending on the OS
	 * @return number of bytes written
	 */
	unsigned WriteNewline() const
	{
		RETURN_ON_NOT_OPEN(0)
		return io_write_newline(m_FileHandle);
	}

	/**
	 * Set the file position indicator to the given Offset from Origin
	 * @param Offset The offset from the given
	 * @param Origin IOSEEK_START, IOSEEK_CUR or IOSEEK_END
	 * @return true on success, false on error
	 */
	bool Seek(long Offset, int Origin) const
	{
		RETURN_ON_NOT_OPEN(false)
		return io_seek(m_FileHandle, Offset, Origin) == 0;
	}

	/**
	 * Skip Size bytes in the file
	 * @param Size the number of bytes to skip
	 * @return true on success, false on error
	 */
	bool Skip(long Size) const
	{
		RETURN_ON_NOT_OPEN(false)
		return Seek(Size, IOSEEK_CUR);
	}

	/**
	 * Gets the current offset of the file position indicator from IOSEEK_START
	 * @return -1 on error; otherwise the offset of the file position indicator from IOSEEK_START
	 */
	long int Tell() const
	{
		RETURN_ON_NOT_OPEN(-1)
		return io_tell(m_FileHandle);
	}

	/**
	 * Retrieves the size of the file in bytes
	 * @return -1 on error; otherwise the file's size in bytes
	 */
	long int Length() const
	{
		RETURN_ON_NOT_OPEN(-1)
		long int CurrPos = Tell();
		Seek(0, IOSEEK_END);
		long int Length = Tell();
		Seek(CurrPos, IOSEEK_START);
		return Length;
	}

	/**
	 * Flushes the file @see fflush
	 * @return true on success, false on error
	 */
	bool Flush() const
	{
		RETURN_ON_NOT_OPEN(false)
		return io_flush(m_FileHandle) == 0;
	}

	/**
	 * Closes the file @see fclose
	 * @return true on success, false on error
	 * @note this function only must be invoked if you want to use re-open in a different mode, using Open
	 */
	bool Close() const
	{
		RETURN_ON_NOT_OPEN(false)
		return io_close(m_FileHandle) == 0;
	}

	/**
	 * Check whether the file is open
	 * @return a bool indicating whether the file is open
	 */
	bool IsOpen() const
	{
		return m_FileHandle != NULL;
	}

#undef RETURN_ON_NOT_OPEN

	//operator IOHANDLE() const { return f; }
	IOHANDLE operator=(const IOHANDLE& New)
	{
		if(m_FileHandle)
			io_close(m_FileHandle);
		m_FileHandle = New;
		return m_FileHandle;
	}

	IOHANDLE_SMART& operator=(const IOHANDLE_SMART& other)
	{
		if(m_FileHandle)
			io_close(m_FileHandle);
		m_FileHandle = other.m_FileHandle;
		return *this;
	}
};



#endif
