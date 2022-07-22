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


extern HINSTANCE _hInstance;

#define IMAGE_MARGIN_CX 5
#define IMAGE_MARGIN_CY 5

class TestDlg
{
public:
	TestDlg(HBITMAP hbmp) : _hbmp(hbmp), _hdlg(NULL) {}
	virtual ~TestDlg() {}

	INT_PTR DoModal()
	{
		return DialogBoxParam(
			_hInstance,
			MAKEINTRESOURCE(IDD_TEST),
			NULL,
			DlgProc,
			(LPARAM)(LPVOID)this);
	}

protected:
	HBITMAP _hbmp;
	BITMAP _bm;
	SIZE _marginSz;
	HWND _hdlg;

	virtual BOOL OnInitDialog()
	{
		RECT rc;
		GetWindowRect(_hdlg, &rc);
		SIZE wndSz = { rc.right - rc.left, rc.bottom - rc.top };
		GetClientRect(_hdlg, &rc);
		_marginSz = { wndSz.cx - rc.right+2*IMAGE_MARGIN_CX, wndSz.cy-rc.bottom+ 2*IMAGE_MARGIN_CY };
		GetObject(_hbmp, sizeof(_bm), &_bm);
		SetWindowPos(_hdlg, NULL, 0, 0, _bm.bmWidth + _marginSz.cx, _bm.bmHeight + _marginSz.cy, SWP_NOZORDER | SWP_NOMOVE);
		return TRUE;
	}
	virtual void OnDestroy() { _hdlg = NULL; }
	virtual BOOL OnCancel() { EndDialog(_hdlg, IDCANCEL); return TRUE; }
	virtual BOOL OnCommand(WPARAM wp_, LPARAM lp_)
	{
		if (LOWORD(wp_) == IDCANCEL)
			return OnCancel();
		return FALSE;
	}
	virtual BOOL OnPaint()
	{
		if (!_hbmp)
			return FALSE;

		ustring msg(_hInstance, IDS_TESTDLG_MESSAGE);
		RECT rc;
		GetClientRect(_hdlg, &rc);
		rc.left += IMAGE_MARGIN_CX;
		rc.right -= IMAGE_MARGIN_CX;
		rc.top = rc.bottom/2;

		PAINTSTRUCT	ps;
		BeginPaint(_hdlg, (LPPAINTSTRUCT)&ps);

		HDC hdc2 = CreateCompatibleDC(ps.hdc);
		HBITMAP hbmp0 = (HBITMAP)SelectObject(hdc2, _hbmp);
		BitBlt(ps.hdc, IMAGE_MARGIN_CX, IMAGE_MARGIN_CY, _bm.bmWidth, _bm.bmHeight, hdc2, 0, 0, SRCCOPY);
		SelectObject(hdc2, hbmp0);
		DeleteDC(hdc2);

		SetBkMode(ps.hdc, TRANSPARENT);
		DrawText(ps.hdc, msg, msg._length, &rc, DT_CENTER);

		EndPaint(_hdlg, (LPPAINTSTRUCT)&ps);
		return TRUE;
	}
	virtual BOOL OnEraseBkgnd(HDC hdc) { return TRUE; }

private:
	static INT_PTR CALLBACK DlgProc(HWND h_, UINT m_, WPARAM wp_, LPARAM lp_)
	{
		TestDlg* pThis = (TestDlg*)GetWindowLongPtr(h_, DWLP_USER);
		if (m_ == WM_INITDIALOG)
		{
			pThis = (TestDlg*)lp_;
			pThis->_hdlg = h_;
			::SetWindowLongPtr(h_, DWLP_USER, (ULONG_PTR)(LPVOID)pThis);
			return pThis->OnInitDialog();
		}
		else if (m_ == WM_DESTROY)
		{
			if (pThis)
				pThis->OnDestroy();
			SetWindowLongPtr(h_, DWLP_USER, NULL);
			return TRUE;
		}
		else if (m_ == WM_COMMAND)
		{
			if (pThis && pThis->OnCommand(wp_, lp_))
			{
				::SetWindowLongPtr(h_, DWLP_MSGRESULT, TRUE);
				return TRUE;
			}
		}
		else if (m_ == WM_PAINT)
		{
			if (pThis->OnPaint())
			{
				::SetWindowLongPtr(h_, DWLP_MSGRESULT, 0);
				return TRUE;
			}
		}
		else if (m_ == WM_ERASEBKGND)
		{
			if (pThis->OnEraseBkgnd((HDC)wp_))
			{
				//MS documentation: An application should return nonzero if it erases the background; otherwise, it should return zero.
				::SetWindowLongPtr(h_, DWLP_MSGRESULT, TRUE);
				return TRUE;
			}
		}
		return FALSE;
	}
};

