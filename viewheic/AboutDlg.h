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
#include <shellapi.h>
#include <commctrl.h>
#include "SimpleDlg.h"


class AboutDlg : public SimpleDlg
{
public:
	AboutDlg(HWND hp_) : SimpleDlg(IDD_ABOUTBOX, hp_) {}

protected:
	virtual BOOL OnNotify(LPNMHDR pNmHdr)
	{
		if (pNmHdr->code = NM_CLICK && pNmHdr->idFrom == IDC_SYSLINK_GMP_LIC)
		{
			openLink((NMLINK*)pNmHdr);
			return TRUE;
		}
		return FALSE;
	}
	// process the hyperlink click.
	// open a GMP license file. it's located in the directory the app module resides.
	// the file is called 'gmplic.txt'. use ShellExecute to open it in Notepad.
	void openLink(NMLINK *pLink)
	{
		if (0 != wcscmp(pLink->item.szID, L"idOpenLink"))
			return;
		WCHAR path[MAX_PATH];
		GetModuleFileName(NULL, path, sizeof(path));
		LPWSTR ftitle = (LPWSTR)GetFileTitlePtr(path, ARRAYSIZE(path));
		lstrcpy(ftitle, LIBHEIFLICENCE_FILENAME);
		ShellExecute(_hdlg, L"open", path, NULL, NULL, SW_SHOW);
	}
};


