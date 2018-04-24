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
	std::string m_FilePath;
	IOHANDLE m_FileHandle;
public:
	IOHANDLE_SMART(const char *pFilename, int Flags) : m_FileHandle(NULL)
	{
		Open(pFilename, Flags);
	}

	IOHANDLE_SMART()
	{
		m_FilePath = "";
		m_FileHandle = NULL;
	}

	IOHANDLE_SMART(const IOHANDLE_SMART& other)
	{
		if(IsOpen())
			Close();
		m_FilePath = other.m_FilePath;
		m_FileHandle = other.m_FileHandle;
	}

	// moving (transferring ownership over the file) is still allowed
	IOHANDLE_SMART(IOHANDLE_SMART& source)
	{
		this->seize(&source);
	}

	~IOHANDLE_SMART()
	{
		if(m_FileHandle)
			io_close(m_FileHandle);
		m_FileHandle = NULL;
	}


	/**
	 * Re-Open the file with the given flags
	 * @param Flags one of IOFLAG_*
	 * @return true on success, false on error
	 * @note before using this function, you must use the Close function
	 */
	bool Open(int Flags);

	/**
	 * Open a new file on this handler
	 * @param pFilename path to the file to open
	 * @param Flags one of IOFLAG_*
	 * @return true on success, false on error
	 * @note before using this function, you must use the Close function
	 */
	bool Open(const char *pFilename, int Flags);

	/**
	 * Reads Size bytes from the file and stores it into pBuffer
	 * @param pBuffer a block of pre-allocated memory
	 * @param Size how many bytes to read at most
	 * @return the number of actual bytes read (0 on failure)
	 */
	unsigned Read(void *pBuffer, unsigned Size);

	/**
	 * Reads Size bytes from the file, and stores it in an std::string
	 * @param Size nubmer of characters to read
	 * @return a string containing the text
	 */
	const std::string ReadText(unsigned Size);

	/**
	 * Reads the whole file and returns in as an std::string
	 * @return a string containing the text
	 * @note This leaves the cursor at the end of the file
	 */
	const std::string ReadAllText();

	/**
	 * Reads the whole file into a heap buffer and returns a pointer to it
	 * @return a string containing the text
	 * @note This leaves the cursor at the end of the file
	 * @note You have to free the buffer yourself!
	 */
	char *ReadAllTextRaw(unsigned int *pLen = NULL);

	/**
	 * Reads characters until it encounters a linebreak or EOF and
	 * appends all read characters to the pDest string.
	 * @param pDest string to append the characters to
	 * @return true if there is a next line, false on encountering EOF or read error
	 */
	bool ReadNextLine(std::string *pDest);

	/**
	 * Reads characters until it encounters a linebreak or EOF and
	 * replaces the buffer contents with the read characters.
	 * @param pBuffer buffer to write into
	 * @param BufferSize maximum size of the buffer
	 * @return true if there is a next line or the buffer is full, false on encountering EOF or read error
	 */
	bool ReadNextLine(char *pBuffer, unsigned BufferSize);

	/**
	 * Writes Size bytes from pBuffer to the file, if it is open.
	 * @param pBuffer Data to write
	 * @param Size Number of bytes to write
	 * @return 0 if file is closed, otherwise the number of bytes written
	 */
	unsigned Write(const void *pBuffer, unsigned Size) const;

	/**
	 * Writes the string in the buffer to the file.
	 * @param pStr Data to write
	 * @param Raw whether to write the \0 terminator aswell (only do this for raw files!)
	 * @return 0 if file is closed, otherwise the number of bytes written
	 */
	unsigned WriteString(const char *pStr, bool Raw) const;

	/**
	 * Writes the string to the file.
	 * @param pStr Data to write
	 * @param Raw whether to write the \0 terminator aswell (only do this for raw files!)
	 * @return 0 if file is closed, otherwise the number of bytes written
	 */
	unsigned WriteString(const std::string& Str, bool Raw) const;

	/**
	 * Writes the text plus an appropriate newline to the file
	 * @param pText text to write
	 * @return number of bytes written, see Write
	 */
	unsigned WriteLine(const char *pText) const;

	unsigned WriteLine(const std::string& Str) const;

	/**
	 * Writes a newline to the file, depending on the OS
	 * @return number of bytes written
	 */
	unsigned WriteNewline() const;

	/**
	 * Set the file position indicator to the given Offset from Origin
	 * @param Offset The offset from the given
	 * @param Origin IOSEEK_START, IOSEEK_CUR or IOSEEK_END
	 * @return true on success, false on error
	 */
	bool Seek(long Offset, int Origin) const;

	/**
	 * Skip Size bytes in the file
	 * @param Size the number of bytes to skip
	 * @return true on success, false on error
	 */
	bool Skip(long Size) const;

	/**
	 * Gets the current offset of the file position indicator from IOSEEK_START
	 * @return -1 on error; otherwise the offset of the file position indicator from IOSEEK_START
	 */
	long int Tell() const;

	/**
	 * Retrieves the size of the file in bytes
	 * @return -1 on error; otherwise the file's size in bytes
	 */
	long int Length() const;

	/**
	 * Flushes the file @see fflush
	 * @return true on success, false on error
	 */
	bool Flush() const;

	/**
	 * Closes the file @see fclose
	 * @return true on success, false on error
	 * @note this function only must be invoked if you want to use re-open in a different mode, using Open
	 */
	bool Close();

	/**
	 * Check whether the file is open
	 * @return a bool indicating whether the file is open
	 */
	bool IsOpen() const
	{
		return m_FileHandle != NULL;
	}


	/**
	 * Get the path of the associated file
	 * @return the path to the associated file
	 * @note validity of the returned path depends on the user!
	 */
	const char *GetPath() const
	{
		return m_FilePath.c_str();
	}


	//operator IOHANDLE() const { return f; }
	/*IOHANDLE operator=(const IOHANDLE& New)
	{
		if(m_FileHandle)
			io_close(m_FileHandle);
		m_FileHandle = New;
		return m_FileHandle;
	}*/

	// copying is forbidden
	IOHANDLE_SMART& operator=(const IOHANDLE_SMART& other) = delete;

	// moving (transferring ownership over the file) is still allowed, though
	IOHANDLE_SMART& operator=(IOHANDLE_SMART&& source) noexcept { return this->seize(&source); }

	IOHANDLE_SMART& seize(IOHANDLE_SMART *source)
	{
		if(m_FileHandle)
			io_close(m_FileHandle);
		m_FileHandle = source->m_FileHandle;
		m_FilePath = source->m_FilePath;
		source->m_FileHandle = NULL;
		source->m_FilePath = "";
		return *this;
	}


private:
	friend class CStorage;

	IOHANDLE_SMART(const char *pFilename, IOHANDLE FileHandle) : m_FilePath(std::string(pFilename))
	{
		m_FileHandle = FileHandle;
	}
};



#endif
