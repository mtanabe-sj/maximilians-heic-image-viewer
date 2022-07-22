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

// registry helpers, defined in helper.cpp
extern LONG Registry_GetStringValue(HKEY hkeyRoot, LPCTSTR pszKey, LPCTSTR pszName, std::wstring &wstrValue);
extern LONG Registry_GetDwordValue(HKEY hkeyRoot, LPCTSTR pszKey, LPCTSTR pszName, DWORD* pdwValue);
extern LONG Registry_SetDwordValue(HKEY hkeyRoot, LPCTSTR pszKey, LPCTSTR pszName, DWORD dwValue);
extern LONG Registry_DeleteValue(HKEY hkeyRoot, LPCTSTR pszBaseKey, LPCTSTR pszValueName);
extern LONG Registry_EnumPropEntries(HKEY hkeyRoot, LPCTSTR pszKey, std::vector<std::pair<std::wstring, DWORD>> &outList, int maxNames = 0x100);
extern LONG Registry_CreateComClsidKey(LPCTSTR pszClsid, LPCTSTR pszHandlerName);
extern LONG Registry_CreateNameValue(HKEY hkeyRoot, LPCTSTR pszKey, LPCTSTR pszName, LPCTSTR pszValue);
extern LONG Registry_DeleteSubkey(HKEY hkeyRoot, LPCTSTR pszBaseKey, LPCTSTR pszSubKey);
extern LONG Registry_DeleteValue(HKEY hkeyRoot, LPCTSTR pszBaseKey, LPCTSTR pszValueName);

// btimap helpers, defined in helper.cpp
extern HRESULT SaveBmp(HBITMAP hbm, LPCWSTR filename);

///////////////////////////////////////////////////////////////
#define LIBREGKEY L"SOFTWARE\\mtanabe\\heicviewer\\thumbnailer"
