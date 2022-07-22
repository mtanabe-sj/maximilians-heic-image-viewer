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
// thumheic.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"


//////////////////////////////////////////////////////////////
LONG Registry_CreateComClsidKey(LPCTSTR pszClsid, LPCTSTR pszHandlerName)
{
	TCHAR regKey[256];
	TCHAR valBuf[256];
	LONG res;
	swprintf_s(regKey, ARRAYSIZE(regKey), L"CLSID\\%s", pszClsid);
	res = Registry_CreateNameValue(HKEY_CLASSES_ROOT, regKey, NULL, pszHandlerName);
	if (res == ERROR_SUCCESS)
	{
		swprintf_s(regKey, ARRAYSIZE(regKey), L"CLSID\\%s\\InProcServer32", pszClsid);
		GetModuleFileName(LibInstanceHandle, valBuf, ARRAYSIZE(valBuf));
		res = Registry_CreateNameValue(HKEY_CLASSES_ROOT, regKey, NULL, valBuf);
		if (res == ERROR_SUCCESS)
		{
			res = Registry_CreateNameValue(HKEY_CLASSES_ROOT, regKey, L"ThreadingModel", L"Apartment");
			if (res == ERROR_SUCCESS)
			{
				swprintf_s(regKey, ARRAYSIZE(regKey), L"CLSID\\%s\\DefaultIcon", pszClsid);
				wcscat_s(valBuf, ARRAYSIZE(valBuf), L",0");
				res = Registry_CreateNameValue(HKEY_CLASSES_ROOT, regKey, NULL, valBuf);
			}
		}
	}
	return res;
}

LONG Registry_CreateNameValue(HKEY hkeyRoot, LPCTSTR pszKey, LPCTSTR pszName, LPCTSTR pszValue)
{
	LONG res;
	HKEY hkey;
	DWORD dwDisposition;

	res = RegCreateKeyEx(hkeyRoot, pszKey, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hkey, &dwDisposition);
	if (res == ERROR_SUCCESS)
	{
		res = RegSetValueEx(hkey, pszName, NULL, REG_SZ, (LPBYTE)pszValue, ((DWORD)wcslen(pszValue) + 1) * sizeof(TCHAR));
		RegCloseKey(hkey);
	}
	return res;
}

LONG Registry_DeleteSubkey(HKEY hkeyRoot, LPCTSTR pszBaseKey, LPCTSTR pszSubKey)
{
	LONG res;
	HKEY hkey;

	res = RegOpenKeyEx(hkeyRoot, pszBaseKey, 0, KEY_ALL_ACCESS, &hkey);
	if (res == ERROR_SUCCESS)
	{
		res = SHDeleteKey(hkey, pszSubKey);
		RegCloseKey(hkey);
	}
	return res;
}

LONG Registry_DeleteValue(HKEY hkeyRoot, LPCTSTR pszBaseKey, LPCTSTR pszValueName)
{
	LONG res;
	HKEY hkey;

	res = RegOpenKeyEx(hkeyRoot, pszBaseKey, 0, KEY_WRITE, &hkey);
	if (res == ERROR_SUCCESS)
	{
		res = RegDeleteValue(hkey, pszValueName);
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

LONG Registry_EnumPropEntries(HKEY hkeyRoot, LPCTSTR pszKey, std::vector<std::pair<std::wstring, DWORD>> &outList, int maxNames/*=0x100*/)
{
	LONG res, status;
	HKEY hkey;
	WCHAR wszName[256];

	res = RegOpenKeyEx(hkeyRoot, pszKey, 0, KEY_QUERY_VALUE, &hkey);
	if (res == ERROR_SUCCESS)
	{
		for (int i = 0; i < maxNames; i++)
		{
			std::pair<std::wstring, std::wstring> nv;
			DWORD ccName = ARRAYSIZE(wszName);
			DWORD dwValue;
			DWORD dwType = 0;
			DWORD cbValue = sizeof(dwValue);
			status = RegEnumValue(hkey, i, wszName, &ccName, NULL, &dwType, (LPBYTE)&dwValue, &cbValue);
			if (status != ERROR_SUCCESS) // E.g., status == ERROR_NO_MORE_ITEMS
				break;
			if (dwType != REG_DWORD)
				continue;
			outList.emplace_back(wszName, dwValue);
		}
		RegCloseKey(hkey);
	}
	return res;
}

/////////////////////////////////////////////////////////////////////
/* writes a bitmap image to a .bmp file.

make sure the input bitmap is a 32-bit DIB. use Win32 CreateDIBSection() to create it.
the code assumes no scanline padding is required and no color palette is needed, meaning that the color depth must be 32 bit.
the input filename can contain environment variables, e.g., %TMP%.
*/
HRESULT SaveBmp(HBITMAP hbm, LPCWSTR filename)
{
	BITMAP bm = { 0 };
	GetObject(hbm, sizeof(bm), &bm);

	if (bm.bmBits == NULL)
		return E_UNEXPECTED;

	BITMAPFILEHEADER bfh = { 0x4d42 };
	bfh.bfSize = sizeof(BITMAPFILEHEADER) + sizeof(LPBITMAPINFOHEADER) + bm.bmHeight * bm.bmWidthBytes;
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(LPBITMAPINFOHEADER);

	BITMAPINFO bi = { sizeof(BITMAPINFOHEADER) };
	BITMAPINFOHEADER &bih = bi.bmiHeader;
	bih.biWidth = bm.bmWidth;
	bih.biHeight = bm.bmHeight;
	bih.biBitCount = bm.bmBitsPixel;
	bih.biPlanes = 1;
	bih.biSizeImage = bm.bmHeight * bm.bmWidthBytes;

	HRESULT hr = S_OK;
	WCHAR buf[MAX_PATH];
	ExpandEnvironmentStrings(filename, buf, ARRAYSIZE(buf));
	HANDLE hf = CreateFile(buf, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hf == INVALID_HANDLE_VALUE)
		return HRESULT_FROM_WIN32(GetLastError());

	ULONG len;
	if (WriteFile(hf, &bfh, sizeof(bfh), &len, NULL) &&
		WriteFile(hf, &bih, sizeof(bih), &len, NULL))
	{
		LPBYTE p = (LPBYTE)bm.bmBits;
		for (int i = bm.bmHeight - 1; i >= 0; i--)
		{
			len = 0;
			WriteFile(hf, p, bm.bmWidthBytes, &len, NULL);
			if (len != bm.bmWidthBytes)
			{
				hr = E_FAIL;
				break;
			}
			p += len;
		}
	}
	else
		hr = HRESULT_FROM_WIN32(GetLastError());
	CloseHandle(hf);
	return hr;
}
