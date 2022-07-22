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
#include "stdafx.h"


LONG Registry_GetDwordValue(HKEY hkeyRoot, LPCTSTR pszKey, LPCTSTR pszName, DWORD* pdwValue)
{
	LONG res;
	HKEY hkey;

	res = RegOpenKeyEx(hkeyRoot, pszKey, 0, KEY_QUERY_VALUE, &hkey);
	if (res == ERROR_SUCCESS)
	{
		DWORD dwType = REG_DWORD;
		DWORD dwLength = sizeof(DWORD);
		res = RegQueryValueEx(hkey, pszName, NULL, &dwType, (LPBYTE)pdwValue, &dwLength);
		RegCloseKey(hkey);
	}
	return res;
}

LONG Registry_GetStringValue(HKEY hkeyRoot, LPCTSTR pszKey, LPCTSTR pszName, std::wstring &wstrValue)
{
	LONG res;
	HKEY hkey;
	WCHAR buf[256];

	res = RegOpenKeyEx(hkeyRoot, pszKey, 0, KEY_QUERY_VALUE, &hkey);
	if (res == ERROR_SUCCESS)
	{
		DWORD dwType = REG_SZ;
		DWORD dwLength = sizeof(buf);
		res = RegQueryValueEx(hkey, pszName, NULL, &dwType, (LPBYTE)buf, &dwLength);
		if (res == ERROR_SUCCESS && dwLength >= sizeof(WCHAR))
			wstrValue.assign(buf, (dwLength - sizeof(WCHAR)) / sizeof(WCHAR));
		RegCloseKey(hkey);
	}
	return res;
}

LONG Registry_SetDwordValue(HKEY hkeyRoot, LPCTSTR pszKey, LPCTSTR pszName, DWORD dwValue)
{
	LONG res;
	HKEY hkey;
	DWORD dwDisposition;

	res = RegCreateKeyEx(hkeyRoot, pszKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, &dwDisposition);
	if (res == ERROR_SUCCESS)
	{
		res = RegSetValueEx(hkey, pszName, NULL, REG_DWORD, (LPBYTE)&dwValue, sizeof(DWORD));
		RegCloseKey(hkey);
	}
	return res;
}


HRESULT WriteToFile(LPBYTE srcData, int srcLen, LPCWSTR filename)
{
	WCHAR path[MAX_PATH];
	ExpandEnvironmentStrings(filename, path, ARRAYSIZE(path));
	HANDLE hf = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hf == INVALID_HANDLE_VALUE)
		return HRESULT_FROM_WIN32(GetLastError());
	HRESULT hr = S_OK;
	ULONG writtenLen;
	if (WriteFile(hf, srcData, srcLen, &writtenLen, NULL))
	{
		if (writtenLen != srcLen)
			hr = E_UNEXPECTED;
	}
	else
		hr = HRESULT_FROM_WIN32(GetLastError());
	CloseHandle(hf);
	return hr;
}

LPCWSTR GetFileTitlePtr(LPCWSTR pathInBuf, int bufSize/*=-1*/)
{
	LPCWSTR p = wcsrchr(pathInBuf, '\\');
	if (p)
		p++;
	else
		p = pathInBuf;
	return p;
}

int CopyFileTitle(LPCWSTR filename, LPWSTR buf, int bufSize)
{
	ZeroMemory(buf, bufSize * sizeof(WCHAR));
	int n = 0;
	LPCWSTR p1 = GetFileTitlePtr(filename);
	LPCWSTR p2 = wcsrchr(p1, '.');
	if (p2)
		n = (int)(p2 - p1);
	else
		n = (int)wcslen(p1);
	if (n < bufSize)
		CopyMemory(buf, p1, n * sizeof(WCHAR));
	return n;
}


int PreserveAspectRatio(int srcWidth, int srcHeight, int destWidth, int destHeight, RECT *correctedDestRect)
{
	int x2 = 0;
	int y2 = 0;
	int cx2 = srcWidth;
	int cy2 = srcHeight;
	double aspratio = 1.0;
	if (srcWidth != destWidth || srcHeight != destHeight)
	{
		aspratio = (double)srcHeight / (double)srcWidth;
		if (cx2 > destWidth)
		{
			cx2 = destWidth;
			cy2 = (int)((double)cx2*aspratio);
		}
		if (cy2 > destHeight)
		{
			cy2 = destHeight;
			cx2 = (int)((double)cy2 / aspratio);
		}
	}
	x2 = (destWidth - cx2) / 2;
	y2 = (destHeight - cy2) / 2;
	correctedDestRect->left = x2;
	correctedDestRect->right = x2 + cx2;
	correctedDestRect->top = y2;
	correctedDestRect->bottom = y2 + cy2;
	return MulDiv(100, cx2, srcWidth);
}
