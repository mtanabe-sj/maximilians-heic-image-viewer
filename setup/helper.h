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

#ifdef _DEBUG
#define ASSERT(good) if (!(good)) DebugBreak();
#else
#define ASSERT(good)
#endif


HRESULT createBitmap(HDC hdc, IWICBitmapSource *wbsrc, HBITMAP *phbmp);
int Pathname_AppendFileTitle(LPTSTR pszPath, LPCTSTR pszTitle);
BOOL IsWow64();


//////////////////////////////////////////////////////

#define SIMPLECOLLECTION_GROW_SIZE 8

template <class T>
class objList
{
public:
	objList() : _p(NULL), _n(0), _max(0) {}
	virtual ~objList() { clear(); }

	T* operator[](const int index) const
	{
		ASSERT(0 <= index && index < _n);
		return _p[index];
	}
	size_t size() const
	{
		return _n;
	}

	void clear(bool deleteObjs = true)
	{
		if (!_p)
			return;
		if (deleteObjs)
		{
			for (int i = 0; i < _max; i++)
			{
				T *pi = (T*)InterlockedExchangePointer((LPVOID*)(_p + i), NULL);
				if (pi)
				{
					delete pi;
					_n--;
				}
			}
		}
		ASSERT(_n == 0);
		T **p = (T**)InterlockedExchangePointer((LPVOID*)&_p, NULL);
		free(p);
		_max = 0;
	}

	int add(T *obj)
	{
		int i;
		for (i = 0; i < _max; i++)
		{
			T *pi = (T*)InterlockedCompareExchangePointer((LPVOID*)(_p + i), obj, NULL);
			if (!pi)
			{
				_n++;
				return i;
			}
		}
		T **p2 = (T**)realloc(_p, (_max + SIMPLECOLLECTION_GROW_SIZE) * sizeof(T*));
		if (!p2)
			return -1;
		T **p3 = p2 + _max;
		ZeroMemory(p3, SIMPLECOLLECTION_GROW_SIZE * sizeof(T*));
		*p3 = obj;
		_max += SIMPLECOLLECTION_GROW_SIZE;
		_p = p2;
		i = _n++;
		return i;
	}
	bool remove(T *obj)
	{
		for (int i = 0; i < _max; i++)
		{
			T *pi = InterlockedCompareExchangePointer((LPVOID*)(_p + i), NULL, obj);
			if (pi)
			{
				delete pi;
				_n--;
				ASSERT(_n >= 0);
				_repack(i);
				return true;
			}
		}
		return false;
	}
	bool removeAt(int index)
	{
		if (index < 0 || index >= _max)
			return false;
		T *pi = InterlockedExchangePointer((LPVOID*)(_p + index), NULL);
		if (!pi)
			return false;
		delete pi;
		_n--;
		ASSERT(_n >= 0);
		_repack(index);
		return true;
	}

protected:
	int _max, _n;
	T **_p;

	void _repack(int i0)
	{
		int j = i0;
		for (int i = i0; i < _max; i++)
		{
			if (_p[i])
			{
				if (j < i)
					_p[j] = InterlockedExchangePointer((LPVOID*)(_p + i), NULL);
				j++;
			}
		}
	}
};

class ustring
{
public:
	ustring() { init(); }
	ustring(ustring &src) { init(); assign(src); }
	ustring(LPCWSTR pszText, int ccText = -1) { init(); assign(pszText, ccText); }
	ustring(HINSTANCE hInst, UINT resId) { init(); LoadString(hInst, resId); }
	virtual ~ustring() { clear(); }

	operator LPCWSTR() { return _buffer ? _buffer : L""; }
	const ustring& operator+=(LPCWSTR src)
	{
		append(src);
		return *this;
	}
	const ustring& operator=(const ustring& src)
	{
		assign(src);
		return *this;
	}

	virtual void clear()
	{
		LPVOID p = InterlockedExchangePointer((LPVOID*)&_buffer, NULL);
		if (p)
		{
			LocalFree((HLOCAL)p);
			_maxLength = _length = 0;
		}
	}
	bool assign(const ustring &src)
	{
		return assign(src._buffer, src._length);
	}
	bool assign(LPCWSTR pszText, int ccText = -1)
	{
		clear();
		if (!pszText)
			return true; // Null input string is legal.
		if (ccText == -1)
			ccText = (int)lstrlen(pszText);
		if (ccText < 0) return false;
		if (!alloc(ccText))
			return false;
		lstrcpy(_buffer, pszText);
		_length = ccText;
		return true;
	}
	bool append(const ustring &src)
	{
		return append(src._buffer, src._length);
	}
	bool append(LPCWSTR pszText, int ccText = -1)
	{
		if (!pszText)
			return true; // Null input string is legal.
		ULONG ccExtra = 0;
		if (ccText == -1) {
			ccText = (int)lstrlen(pszText);
			ccText++;
		}
		else
			ccExtra = 1;
		ULONG cc1 = _length;
		ULONG cc2 = cc1 + ccText + ccExtra;
		LPWSTR p1 = (LPWSTR)InterlockedExchangePointer((LPVOID*)&_buffer, NULL);
		if (!alloc(cc2))
			return FALSE;
		if (p1) {
			CopyMemory(_buffer, p1, cc1 * sizeof(WCHAR));
			LocalFree((HLOCAL)p1);
		}
		CopyMemory(_buffer + cc1, pszText, ccText * sizeof(WCHAR));
		_length = cc2 - 1;
		return true;
	}
	LPWSTR alloc(int cc)
	{
		clear();
		if (cc >= 0)
		{
			_buffer = (LPWSTR)LocalAlloc(LPTR, (++cc) * sizeof(WCHAR));
			if (_buffer)
			{
				ZeroMemory(_buffer, cc * sizeof(WCHAR));
				_length = 0;
				_maxLength = cc * sizeof(WCHAR);
			}
		}
		return _buffer;
	}
	bool empty() { return (_buffer && *_buffer) ? false : true; }
#define USTR_VFORMAT_GROWTH_SIZE 64
#define USTR_VFORMAT_MAX_ALLOC 0x1000
	bool vformat(LPCWSTR fmt, va_list& args)
	{
		HRESULT hr;
		int len = lstrlen(fmt) + USTR_VFORMAT_GROWTH_SIZE;
		while (len <= USTR_VFORMAT_MAX_ALLOC && alloc(len))
		{
			size_t remainingLen = 0;
			hr = StringCbVPrintfExW(_buffer, _maxLength, NULL, &remainingLen, STRSAFE_FILL_ON_FAILURE, fmt, args);
			if (SUCCEEDED(hr))
			{
				size_t spentLen = _maxLength - remainingLen;
				_length = spentLen / sizeof(WCHAR);
				return true;
			}
			if (hr != STRSAFE_E_INSUFFICIENT_BUFFER)
				break;
			len += USTR_VFORMAT_GROWTH_SIZE;
		}
		clear();
		return false;
	}
	bool format(LPCWSTR fmt, ...)
	{
		va_list	args;
		va_start(args, fmt);
		bool success = vformat(fmt, args);
		va_end(args);
		return success;
	}
	bool LoadString(HINSTANCE hInst, ULONG ids) {
		int i;
		int cc1 = 32;
		for (i = 0; i < 16; i++) {
			if (!alloc(cc1))
				return FALSE;
			_length = ::LoadString(hInst, ids, _buffer, cc1);
			if ((int)_length < cc1 - 1)
				return true;
			clear();
			cc1 += 32;
		}
		return false;
	}

	LPWSTR _buffer;
	ULONG _maxLength; // number of allocated bytes in the buffer
	ULONG _length; // number of wide characters used in the buffer (_length*sizeof(WCHAR) <= _maxLength)

protected:
	void init()
	{
		_buffer = NULL; _maxLength = _length = 0;
	}
};


//////////////////////////////////////////////////////
class EventLog
{
public:
	EventLog() : _logLevel(0) {}

	BOOL _logLevel;
	ustring _path;

	void write(LPCWSTR msg, bool timestamp = true, bool feedline=true)
	{
		if (timestamp)
		{
			SYSTEMTIME t;
			GetLocalTime(&t);
			ustring msg2;
			msg2.format(L"[%02d:%02d:%02d] ", t.wHour, t.wMinute, t.wSecond);
			msg2.append(msg);
			OutputDebugString(msg2);
			_logwrite(msg2, feedline);
		}
		else
		{
			OutputDebugString(msg);
			_logwrite(msg, feedline);
		}
	}
	void writef(LPCWSTR fmt, ...)
	{
		va_list args;
		va_start(args, fmt);
		ustring msg;
		msg.vformat(fmt, args);
		write(msg, false);
		va_end(args);
	}

protected:
	// note that logs we write are in unicode.
	void _logwrite(const WCHAR* msg, bool feedline)
	{
		if (!_logLevel)
			return;
		HANDLE hf = CreateFile(_path, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
		if (hf == INVALID_HANDLE_VALUE)
		{
			_logLevel = 0;
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

