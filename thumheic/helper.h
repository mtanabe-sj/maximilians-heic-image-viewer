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

#include <stdlib.h>
#include <strsafe.h>
#include <time.h>


#ifdef _DEBUG
#define ASSERT(good) if (!(good)) DebugBreak();
#else
#define ASSERT(good)
#endif

//////////////////////////////////////////////////////
using namespace std;

/* extends std::wstring for extra things we need.
*/
class ustring : public wstring
{
public:
	ustring(const WCHAR *src, size_t pos = 0, size_t len = npos) : wstring(src, pos, len) {}
	ustring(const wstring& src, size_t pos = 0, size_t len = npos) : wstring(src, pos, len) {}
	ustring(const ustring& src) : wstring(src) {}
	ustring(char c) : wstring(1, c) {}
	ustring() {}

	operator const WCHAR*() { return c_str(); }
	const ustring& operator=(const ustring& src)
	{
		if (&src != this)
			wstring::assign(src);
		return *this;
	}
	const ustring& operator=(const WCHAR* src)
	{
		wstring::append(src);
		return *this;
	}
	const ustring& operator+=(const ustring& src)
	{
		wstring::append(src);
		return *this;
	}
	const ustring& operator+=(const WCHAR *src)
	{
		wstring::append(src);
		return *this;
	}
	int vformat(const WCHAR *fmt, va_list &args)
	{
		clear();
		int len = _vscwprintf(fmt, args) + 1;
		resize(len); // len covers a termination char.
		int len2 = vswprintf_s((LPWSTR)data(), len, fmt, args);
		resize(len2); // resize to the char count excluding the termination char.
		return len2;
	}
	const ustring &format(const WCHAR *fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		vformat(fmt, args);
		va_end(args);
		return *this;
	}
};

// possible flags of ustringv::hexdump
#define USTRHEXDUMP_FLAG_NO_OFFSET 1
#define USTRHEXDUMP_FLAG_NO_ASCII 2
//#define USTRHEXDUMP_FLAG_NO_ENDING_LF 4
//#define USTRHEXDUMP_FLAG_ELLIPSIS 8

class ustringv : public ustring
{
public:
	ustringv() {}
	ustringv(LPCWSTR fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		vformat(fmt, args);
		va_end(args);
	}

	LPCWSTR hexdump(LPVOID dataSrc, int dataLen, UINT flags = 0)
	{
		LPBYTE src = (LPBYTE)dataSrc;
		WCHAR buf[80] = { 0 };
		for (int i = 0; i < (dataLen + 15) / 16; i++)
		{
			memset(buf, ' ', sizeof(buf)-sizeof(WCHAR));
			LPWSTR dest = buf;
			if (!(flags & USTRHEXDUMP_FLAG_NO_OFFSET))
				dest += _swprintf(buf, L"%04X: ", i);
			size_t j, j2 = min(16, dataLen - i * 16);
			for (j = 0; j < j2; j++)
			{
				_swprintf(dest, L"%02X ", src[i * 16 + j]);
				dest += 3;
			}
			if (!(flags & USTRHEXDUMP_FLAG_NO_ASCII))
			{
				dest[0] = ' ';
				for (; j < 16; j++)
					dest += 3;
				for (j = 0; j < j2; j++)
				{
					WCHAR c = src[i * 16 + j];
					if (c < ' ' || c > 0x7f)
						c = '.';
					*dest++ = c;
				}
			}
			*dest++ = '\r';
			*dest++ = '\n';
			*dest = '\0';
			append(buf);
		}
		return c_str();
	}
};

//////////////////////////////////////////////////////
#ifdef APP_WRITES_EVENTLOG
#define EVENTLOG_FLAGS_CANLOG 1

class EventLog
{
public:
	EventLog() : _lfi{ 0 }
	{
	}
	~EventLog()
	{
	}

	struct LOGFILEINFO
	{
		DWORD Flags;
		CHAR Path[_MAX_PATH];
	} _lfi;

	void init(LPCSTR filename="logs\\heicviewer\\thumbnailer.log", DWORD overrideFlags=0)
	{
		if (overrideFlags == 0)
			Registry_GetDwordValue(HKEY_LOCAL_MACHINE, LIBREGKEY, L"TroubleshootFlags", (DWORD*)&_lfi.Flags);
		else
			_lfi.Flags = overrideFlags;
		GetTempPathA(sizeof(_lfi.Path), _lfi.Path);
		strcat_s(_lfi.Path, sizeof(_lfi.Path), filename);
	}

	void write(LPCWSTR msg, bool timestamp = true, bool feedline=true)
	{
		if (timestamp)
		{
			time_t now;
			time(&now);
			struct tm t = *localtime(&now);
			ustringv msg2(
#ifdef _WIN64
				L"[%02d:%02d:%02d] ",
#else//#ifdef _WIN64
				L"[%02d:%02d:%02d*] ",
#endif//#ifdef _WIN64
				t.tm_hour, t.tm_min, t.tm_sec);
			msg2.append(msg);
			OutputDebugString(msg2);
			OutputDebugString(L"\r\n");
			_logwrite(msg2, feedline);
		}
		else
		{
			OutputDebugString(msg);
			OutputDebugString(L"\r\n");
			_logwrite(msg, feedline);
		}
	}
	void writef(LPCWSTR fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ustring msg;
		msg.vformat(fmt, args);
		write(msg, true);
		va_end(args);
	}

	void dumpData(LPCSTR dataLabel, LPVOID dataSrc, int dataLen)
	{
		writef(L"%s (%d bytes)", dataLabel, dataLen);
		write(ustringv().hexdump(dataSrc, dataLen), false);
	}

protected:
	void _logwrite(const WCHAR* msg, bool feedline)
	{
		if (!(_lfi.Flags & EVENTLOG_FLAGS_CANLOG))
			return;
		HANDLE hf = CreateFileA(_lfi.Path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
		if (hf == INVALID_HANDLE_VALUE)
		{
			_lfi.Flags &= ~EVENTLOG_FLAGS_CANLOG;
			return;
		}
		ULONG len;
		DWORD ecode = GetLastError();
		if (ecode == ERROR_SUCCESS)
		{
			// the file has just been created. identify it as a unicode text file.
			WCHAR bom = 0xfeff; // byte-order mark for unicode, little endian
			WriteFile(hf, &bom, sizeof(bom), &len, NULL);
		}
		else // we're appending to an existing file. so go to the eof.
			SetFilePointer(hf, 0, NULL, FILE_END);
		// write out the event message.
		WriteFile(hf, msg, lstrlen(msg) * sizeof(WCHAR), &len, NULL);
		// add a linefeed if asked.
		if (feedline)
			WriteFile(hf, L"\r\n", 2 * sizeof(WCHAR), &len, NULL);
		CloseHandle(hf);
	}
};
#endif//#ifdef APP_WRITES_EVENTLOG
