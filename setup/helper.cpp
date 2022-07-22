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


int Pathname_AppendFileTitle(LPTSTR pszPath, LPCTSTR pszTitle)
{
	int ccDir = lstrlen(pszPath);
	if (ccDir)
	{
		if (pszPath[ccDir - 1] != '\\')
			lstrcat(pszPath, L"\\");
		lstrcat(pszPath, pszTitle);
	}
	return ccDir; // char length of directory part of the path
}

// kernel32 IsWow64Process()
typedef BOOL(WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);

BOOL IsWow64()
{
	BOOL bIsWow64 = FALSE;
	LPFN_ISWOW64PROCESS fnIsWow64Process;
	HMODULE h;
	h = GetModuleHandle(TEXT("kernel32"));
	fnIsWow64Process = (LPFN_ISWOW64PROCESS)GetProcAddress(h, "IsWow64Process");
	if (NULL != fnIsWow64Process)
	{
		fnIsWow64Process(GetCurrentProcess(), &bIsWow64);
	}
	return bIsWow64;
}

HRESULT createBitmap(HDC hdc, IWICBitmapSource *wbsrc, HBITMAP *phbmp)
{
	if (!wbsrc)
		return E_POINTER;
	UINT cx, cy;
	HRESULT hr = wbsrc->GetSize(&cx, &cy);
	if (hr != S_OK)
		return hr;
	if (cx == 0 || cy == 0)
		return E_UNEXPECTED;

	BITMAPINFO bmi = { 0 };
	bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bmi.bmiHeader.biWidth = cx;
	bmi.bmiHeader.biHeight = -((LONG)cy);
	bmi.bmiHeader.biPlanes = 1;
	bmi.bmiHeader.biBitCount = 32; // 32-bit color depth
	bmi.bmiHeader.biCompression = BI_RGB;
	bmi.bmiHeader.biSizeImage = cx * cy * sizeof(RGBQUAD);

	LPBYTE bits;
	HBITMAP hbmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (LPVOID*)&bits, NULL, 0);
	if (!hbmp)
		return E_FAIL;

	UINT stride = sizeof(RGBQUAD) * cx; // 4 bytes per pixel (since it's 32bpp)
	hr = wbsrc->CopyPixels(NULL, stride, stride*cy, bits);
	if (hr == S_OK)
		*phbmp = hbmp;
	else
		DeleteObject(hbmp);
	return hr;
}

