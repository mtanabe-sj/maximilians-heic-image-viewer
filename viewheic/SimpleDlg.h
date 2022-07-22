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

// make sure AppInstanceHandle is set before Create() or DoModal() is called.
// add the extern declaration below in a common header of the app like stdafx.h.
//extern HMODULE AppInstanceHandle;

// make sure you use either Create() or DoModal().
// Create() is for a modeless dialog. DoModal() for modal.
// Call Destroy() only for a modeless instance (started by Create()).
class SimpleDlg
{
public:
	SimpleDlg(int idd, HWND hwndParent = NULL)
		: _idd(idd), _hwndParent(hwndParent), _hdlg(NULL) {}
	virtual ~SimpleDlg() {}

	BOOL Create()
	{
		if (::CreateDialogParam(
			AppInstanceHandle,
			MAKEINTRESOURCE(_idd),
			_hwndParent,
			(DLGPROC)DlgProc,
			(LPARAM)this))
			return TRUE;
		return FALSE;
	}
	void Destroy()
	{
		if (_hdlg)
			::DestroyWindow(_hdlg);
	}
	INT_PTR DoModal()
	{
		return DialogBoxParam(
			AppInstanceHandle,
			MAKEINTRESOURCE(_idd),
			_hwndParent,
			DlgProc,
			(LPARAM)(LPVOID)this);
	}

protected:
	int _idd;
	HWND _hdlg, _hwndParent;

	virtual BOOL OnInitDialog() { return TRUE; }
	virtual void OnDestroy() { _hdlg = NULL; }
	virtual BOOL OnOK() { EndDialog(_hdlg, IDOK); return TRUE; }
	virtual BOOL OnCancel() { EndDialog(_hdlg, IDCANCEL); return TRUE; }
	virtual BOOL OnSysCommand(WPARAM wp_, LPARAM lp_) { return FALSE; }
	virtual BOOL OnCommand(WPARAM wp_, LPARAM lp_)
	{
		if (LOWORD(wp_) == IDOK)
			return OnOK();
		else if (LOWORD(wp_) == IDCANCEL)
			return OnCancel();
		return FALSE;
	}
	virtual BOOL OnNotify(LPNMHDR pNmHdr) { return FALSE; }
	virtual void OnContextMenu(HWND hwnd, int x, int y) {}
	virtual void OnTimer(WPARAM nIDEvent) {}
	virtual void OnSize(UINT nType, int cx, int cy) {}
	virtual void OnMove(int x, int y) {}
	virtual BOOL OnDrawItem(UINT idCtrl, LPDRAWITEMSTRUCT pDIS) { return FALSE; }
	virtual BOOL OnLButtonDown(INT_PTR nButtonType, POINT pt) { return FALSE; }
	virtual BOOL OnLButtonUp(INT_PTR nButtonType, POINT pt) { return FALSE; }
	virtual BOOL OnMouseMove(UINT state, POINT pt) { return FALSE; }
	virtual BOOL OnMouseWheel(short nButtonType, short nWheelDelta, POINT pt) { return FALSE; }
	virtual BOOL OnSetCursor(HWND hwnd, UINT nHitTest, UINT message) { return FALSE; }
	virtual BOOL OnQueryDragIcon(HICON *phicon) { return FALSE; }
	virtual BOOL OnPaint() { return FALSE; }
	virtual BOOL OnEraseBkgnd(HDC hdc) { return FALSE; }
	/*
	virtual BOOL OnHScroll(int scrollAction, int scrollPos, LRESULT *pResult) { return FALSE; }
	virtual BOOL OnVScroll(int scrollAction, int scrollPos, LRESULT *pResult) { return FALSE; }
	*/

private:
	static INT_PTR CALLBACK DlgProc(HWND h_, UINT m_, WPARAM wp_, LPARAM lp_)
	{
		SimpleDlg* pThis = (SimpleDlg*)GetWindowLongPtr(h_, DWLP_USER);
		if (m_ == WM_INITDIALOG)
		{
			pThis = (SimpleDlg*)lp_;
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
		else if (m_ == WM_TIMER)
		{
			if (pThis) {
				pThis->OnTimer(wp_);
				return TRUE;
			}
		}
		else if (m_ == WM_SIZE)
		{
			pThis->OnSize((UINT)(ULONG_PTR)wp_, GET_X_LPARAM(lp_), GET_Y_LPARAM(lp_));
			return TRUE;
		}
		else if (m_ == WM_MOVE)
		{
			pThis->OnMove(GET_X_LPARAM(lp_), GET_Y_LPARAM(lp_));
			return TRUE;
		}
		else if (m_ == WM_SYSCOMMAND)
		{
			if (pThis->OnSysCommand(wp_, lp_))
			{
				::SetWindowLongPtr(h_, DWLP_MSGRESULT, TRUE);
				return TRUE;
			}
		}
		else if (m_ == WM_COMMAND)
		{
			if (pThis && pThis->OnCommand(wp_, lp_))
			{
				::SetWindowLongPtr(h_, DWLP_MSGRESULT, TRUE);
				return TRUE;
			}
		}
		else if (m_ == WM_NOTIFY)
		{
			if (pThis && pThis->OnNotify((LPNMHDR)lp_))
			{
				::SetWindowLongPtr(h_, DWLP_MSGRESULT, TRUE);
				return TRUE;
			}
		}
		else if (m_ == WM_CONTEXTMENU)
		{
			pThis->OnContextMenu((HWND)wp_, GET_X_LPARAM(lp_), GET_Y_LPARAM(lp_));
			return TRUE;
		}
		else if (m_ == WM_DRAWITEM)
		{
			if (pThis->OnDrawItem((UINT)wp_, (LPDRAWITEMSTRUCT)lp_))
				return TRUE;
		}
		else if (m_ == WM_LBUTTONDOWN)
		{
			if (pThis->OnLButtonDown(wp_, { GET_X_LPARAM(lp_), GET_Y_LPARAM(lp_)
		}))
			{
				::SetWindowLongPtr(h_, DWLP_MSGRESULT, 0);
				return TRUE;
			}
		}
		else if (m_ == WM_LBUTTONUP)
		{
			if (pThis->OnLButtonUp(wp_, { GET_X_LPARAM(lp_), GET_Y_LPARAM(lp_) }))
			{
				::SetWindowLongPtr(h_, DWLP_MSGRESULT, 0);
				return TRUE;
			}
		}
		else if (m_ == WM_MOUSEMOVE)
		{
			if (pThis->OnMouseMove((UINT)wp_, { GET_X_LPARAM(lp_), GET_Y_LPARAM(lp_)
		}))
			{
				::SetWindowLongPtr(h_, DWLP_MSGRESULT, 0);
				return TRUE;
			}
		}
		else if (m_ == WM_MOUSEWHEEL)
		{
			if (pThis->OnMouseWheel(GET_KEYSTATE_WPARAM(wp_), GET_WHEEL_DELTA_WPARAM(wp_), { GET_X_LPARAM(lp_), GET_Y_LPARAM(lp_) }))
			{
				::SetWindowLongPtr(h_, DWLP_MSGRESULT, 0);
				return TRUE;
			}
		}
		else if (m_ == WM_SETCURSOR)
		{
			//MS documentation: If the parent window returns TRUE, further processing is halted. 
			if (pThis->OnSetCursor((HWND)wp_, LOWORD(lp_), HIWORD(lp_)))
			{
				::SetWindowLongPtr(h_, DWLP_MSGRESULT, TRUE);
				return TRUE;
			}
		}
		else if (m_ == WM_QUERYDRAGICON)
		{
			HICON hicon;
			if (pThis->OnQueryDragIcon(&hicon))
			{
				::SetWindowLongPtr(h_, DWLP_MSGRESULT, (LRESULT)(LPVOID)hicon);
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
		/*else if (m_ == WM_HSCROLL)
		{
			LRESULT lres = 0;
			if (pThis->OnHScroll(LOWORD(wp_), HIWORD(wp_), &lres))
			{
				::SetWindowLongPtr(h_, DWLP_MSGRESULT, lres);
				return TRUE;
			}
		}
		else if (m_ == WM_VSCROLL)
		{
			LRESULT lres = 0;
			if (pThis->OnVScroll(LOWORD(wp_), HIWORD(wp_), &lres))
			{
				::SetWindowLongPtr(h_, DWLP_MSGRESULT, lres);
				return TRUE;
			}
		}*/
		return FALSE;
	}
};

