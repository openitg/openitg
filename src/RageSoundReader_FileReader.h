/* SoundReader_FileReader - base class for SoundReaders that read from files. */

#ifndef RAGE_SOUND_READER_FILE_READER_H
#define RAGE_SOUND_READER_FILE_READER_H

#include "RageSoundReader.h"

class SoundReader_FileReader: public SoundReader
{
public:
	/*
	 * Return OPEN_OK if the file is open and ready to go.  Return OPEN_UNKNOWN_FILE_FORMAT
	 * if the file appears to be of a different type.  Return OPEN_FATAL_ERROR if
	 * the file appears to be the correct type, but there was an error initializing
	 * the file.
	 *
	 * If the file can not be opened at all, or contains no data, return OPEN_MATCH_BUT_FAIL.
	 */
	enum OpenResult
	{
		OPEN_OK,
		OPEN_UNKNOWN_FILE_FORMAT=1,
		OPEN_FATAL_ERROR=2,
	};
	virtual OpenResult Open(CString filename) = 0;
	virtual bool IsStreamingFromDisk() const { return true; }

	static SoundReader *OpenFile( CString filename, CString &error );

private:
	static SoundReader_FileReader *TryOpenFile( CString filename, CString &error, CString format, bool &bKeepTrying );
};

#endif

/*
 * Copyright (c) 2003-2004 Glenn Maynard
 * All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, and/or sell copies of the Software, and to permit persons to
 * whom the Software is furnished to do so, provided that the above
 * copyright notice(s) and this permission notice appear in all copies of
 * the Software and that both the above copyright notice(s) and this
 * permission notice appear in supporting documentation.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF
 * THIRD PARTY RIGHTS. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR HOLDERS
 * INCLUDED IN THIS NOTICE BE LIABLE FOR ANY CLAIM, OR ANY SPECIAL INDIRECT
 * OR CONSEQUENTIAL DAMAGES, OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
 * OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

