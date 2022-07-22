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
#include <string>


#ifdef _DEBUG
#define ASSERT(good) if (!(good)) DebugBreak();
#else
#define ASSERT(good)
#endif

#define LIB_ADDREF InterlockedIncrement(&LibRefCount)
#define LIB_RELEASE InterlockedDecrement(&LibRefCount)
#define LIB_LOCK InterlockedIncrement(&LibRefCount)
#define LIB_UNLOCK InterlockedDecrement(&LibRefCount)


#ifdef _DEBUG
#define DBGPUTS( _x_ ) OutputDebugString _x_
#define DBGPRINTF( _x_ ) OutputDebugString(resstring _x_)
#else//#ifdef _DEBUG
#define DBGPUTS( _x_ )
#define DBGPRINTF( _x_ )
#endif//#ifdef _DEBUG


extern LONG Registry_GetDwordValue(HKEY hkeyRoot, LPCTSTR pszKey, LPCTSTR pszName, DWORD* pdwValue);
extern LONG Registry_GetStringValue(HKEY hkeyRoot, LPCTSTR pszKey, LPCTSTR pszName, std::wstring &wstrValue);
extern LONG Registry_SetDwordValue(HKEY hkeyRoot, LPCTSTR pszKey, LPCTSTR pszName, DWORD dwValue);

extern HRESULT WriteToFile(LPBYTE srcData, int srcLen, LPCWSTR filename);
extern LPCWSTR GetFileTitlePtr(LPCWSTR pathInBuf, int bufSize=-1);
extern int CopyFileTitle(LPCWSTR filename, LPWSTR buf, int bufSize);
extern int PreserveAspectRatio(int srcWidth, int srcHeight, int destWidth, int destHeight, RECT *correctedDestRect);


class resstring
{
public:
	resstring(HINSTANCE h, UINT ids) { _s = NULL; load(h, ids); }
	resstring(LPCTSTR fmt, ...)
	{
		_s = NULL;
		va_list	args;
		va_start(args, fmt);
		bool success = vformat(fmt, args);
		va_end(args);
	}
	~resstring() { clear(); }

	LPTSTR _s;
	
	void clear()
	{
		if (_s)
		{
			free(_s);
			_s = NULL;
		}
	}
	int load(HINSTANCE hInst, UINT ids)
	{
		LPTSTR pszText = NULL;
		int i, cc1 = 32, cc2 = 0;
		for (i = 0; i < 8; i++) {
			if (!(pszText = (LPTSTR)malloc(cc1 * sizeof(TCHAR))))
				return -1;
			cc2 = ::LoadString(hInst, ids, pszText, cc1);
			if (cc2 < cc1 - 1) {
				if (cc2 == 0)
					_stprintf_s(pszText, cc1, _T("(STR_%d)"), ids);
				break;
			}
			free(pszText);
			cc1 += 32;
		}
		clear();
		_s = pszText;
		return cc2;
	}
	int vformat(const WCHAR *fmt, va_list &args)
	{
		clear();
		int len1 = _vscwprintf(fmt, args) + 1;
		_s = (LPTSTR)malloc(len1 * sizeof(TCHAR)); // len covers a termination char.
		return vswprintf_s(_s, len1, fmt, args);
	}
	operator LPCTSTR() const { return _s; }
};


class strvalenum
{
public:
	strvalenum(wchar_t sep, LPCWSTR src) : _sep(sep), _p0(_wcsdup(src)) { _decompose(); }
	~strvalenum() { if (_p0) free(_p0); }

	LPCWSTR getAt(LPVOID pos)
	{
		INT_PTR i2 = ((INT_PTR)pos) - 1;
		if (i2 < 0 || i2 >= _count)
			return NULL;
		LPTSTR p = _p0;
		for (INT_PTR i = 0; i < i2; i++)
		{
			p += wcslen(p) + 1;
			while (*p == ' ') { p++; };
		}
		return p;
	}
	LPVOID getHeadPosition()
	{
		if (_count)
			return (LPVOID)1;
		return (LPVOID)0;
	}
	LPCWSTR getNext(LPVOID& pos)
	{
		LPCWSTR p = getAt(pos);
		INT_PTR c = (INT_PTR)pos;
		if (c >= _count)
			c = 0;
		else
			c++;
		pos = (LPVOID)c;
		return p;
	}
	LPVOID getTailPosition()
	{
		if (_count)
			return (LPVOID)(ULONG_PTR)_count;
		return (LPVOID)0;
	}
	LPCWSTR getPrior(LPVOID& pos)
	{
		LPCWSTR p = getAt(pos);
		INT_PTR c = (INT_PTR)pos;
		if (c > 0)
			c--;
		else
			c = 0;
		pos = (LPVOID)c;
		return p;
	}
	int getCount() { return _count; }

protected:
	LPWSTR _p0;
	wchar_t _sep;
	int _count;

	void _decompose()
	{
		int cc = (int)wcslen(_p0);
		_count = 0;
		if (*_p0)
			_count++;
		LPWSTR p = _p0;
		for (int i = 0; i < cc; i++) {
			if (_p0[i] == '"') {
				for (int j = i + 1; j < cc; j++) {
					if (_p0[j] == '"') {
						*p = 0; // in case this is the last entry
						i = j;
						break;
					}
					*p++ = _p0[j];
				}
			}
			else if (_p0[i] == _sep) {
				*p++ = 0;
				_count++;
			}
			else {
				*p++ = _p0[i];
			}
		}
		if (p < _p0 + cc)
			*p = 0; // terminate the last item.
	}
};

