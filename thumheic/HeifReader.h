/*
  Copyright (c) 2022 Makoto Tanabe <mtanabe.sj@outlook.com>
  Licensed under the MIT License.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/
#pragma once
#include <Windows.h>
#include <libheif/heif.h>


class HeifReader
{
public:
	HeifReader(LPSTREAM pstream) : _stream(pstream)
	{
		ZeroMemory(&_interface, sizeof(_interface));
		ZeroMemory(&_stg, sizeof(_stg));
		_stream->AddRef();
		_stream->Stat(&_stg, STATFLAG_NONAME);
		_interface.reader_api_version = 1;
		_interface.get_position = _get_position;
		_interface.read = _read;
		_interface.seek = _seek;
		_interface.wait_for_file_size = _wait_for_file_size;
	}
	~HeifReader()
	{
		_stream->Release();
	}
	heif_reader _interface;

	int64_t getPosition()
	{
		LARGE_INTEGER mov = { 0 };
		ULARGE_INTEGER pos;
		HRESULT hr = _stream->Seek(mov, STREAM_SEEK_CUR, &pos);
		if (hr == S_OK)
			return pos.QuadPart;
		return 0;
	}
	int read(void* data, size_t size)
	{
		ULONG cbRead = 0;
		HRESULT hr = _stream->Read(data, (ULONG)size, &cbRead);
		if (hr == S_OK)
		{
			if (cbRead == size)
				return 0;
			return ERANGE;
		}
		return EACCES;
	}
	int seek(int64_t position)
	{
		LARGE_INTEGER pos;
		pos.QuadPart = position;
		HRESULT hr = _stream->Seek(pos, STREAM_SEEK_SET, NULL);
		if (hr == S_OK)
			return 0;
		return ERANGE;
	}
	heif_reader_grow_status waitForFileSize(int64_t target_size)
	{
		if (_stg.cbSize.QuadPart != 0)
		{
			if (target_size <= (LONGLONG)_stg.cbSize.QuadPart)
				return heif_reader_grow_status_size_reached;
		}
		return heif_reader_grow_status_size_beyond_eof;
	}

protected:
	LPSTREAM _stream;
	STATSTG _stg;

	// --- version 1 functions ---
	static int64_t _get_position(void* pThis) { return ((HeifReader*)pThis)->getPosition(); }
	// The functions read(), and seek() return 0 on success.
	// Generally, libheif will make sure that we do not read past the file size.
	static int _read(void* data, size_t size, void* pThis) { return ((HeifReader*)pThis)->read(data, size); }
	static int _seek(int64_t position, void* pThis) { return ((HeifReader*)pThis)->seek(position); }
	// When calling this function, libheif wants to make sure that it can read the file
	// up to 'target_size'. This is useful when the file is currently downloaded and may
	// grow with time. You may, for example, extract the image sizes even before the actual
	// compressed image data has been completely downloaded.
	//
	// Even if your input files will not grow, you will have to implement at least
	// detection whether the target_size is above the (fixed) file length
	// (in this case, return 'size_beyond_eof').
	static enum heif_reader_grow_status _wait_for_file_size(int64_t target_size, void* pThis) { return ((HeifReader*)pThis)->waitForFileSize(target_size); }
};

