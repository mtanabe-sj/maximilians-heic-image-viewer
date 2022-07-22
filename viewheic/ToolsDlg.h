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
#include "SimpleDlg.h"
#include "Resource.h"
#include "app.h"


/* // a big objection to the icon use is that the images on the buttons shrink, making them rather ugly. it makes it difficult to understand what the images are supposed to represent.
// if enabling the icon use, make sure the IDC_BUTTON_ZOOM and other control definitions in IDD_TOOLS switch from BS_BITMAP to BS_ICON for the image type identification.
#define TOOLSDLG_USES_ICON_BUTTON_IMAGES
*/

class ToolsDlg : public SimpleDlg
{
public:
	ToolsDlg(HWND parent, bool centerIt, bool textLabel) : SimpleDlg(IDD_TOOLS, parent), _centerIt(centerIt), _textLabel(textLabel), _opacity(0), _pos{ 0 }, _posParent{ 0 }, _szParent{ 0 }, _szDlg{ 0 }, _szMaxTrack{ 0 }, _viewMargin{ 0 }, _hwndTip(NULL) {}
	RECT _rcDlg;
	SIZE _szDlg, _szParent, _szMaxTrack;
	POINT _pos, _posParent, _viewMargin;
	int _opacity;
	bool _centerIt, _textLabel;

	// cursor is moving in the parent window. reduce the opacity to half.
	void onParentMouseMove(UINT state, POINT pt)
	{
		if (!PtInRect(&_rcDlg, pt))
			setOpacity(1);
	}
	// parent window has changed its size. reposition so that we stay in the center of the parent window.
	void onParentSize(SIZE sz)
	{
		// the toolbar extents are 0, if OnInitDialog() has not completed yet. if so, quit.
		if (_szDlg.cx == 0 && _szDlg.cy == 0)
			return; // not ready.
		// note that the window move event handler, OnMove(), will respond only after the parent extents turn non-zero. so, after this assignment, OnMove() will respond.
		_szParent = sz;
		// the toolbar should not be auto-centered if the user has moved it.
		if (!_centerIt && _pos.x > 0 && _pos.y > 0)
		{
			if (_pos.x < _szMaxTrack.cx && _pos.y < _szMaxTrack.cy)
				return;
			// the toolbar is gone outside the max-track region. bring it back.
		}
		_pos.x = _posParent.x + (_szParent.cx - _szDlg.cx) / 2;
		_pos.y = _posParent.y + (_szParent.cy - _szDlg.cy) / 2;
		DBGPRINTF((L"onParentMove(%d, %d): {%d, %d}\n", sz.cx, sz.cy, _pos.x, _pos.y));
		SetWindowPos(_hdlg, NULL, _pos.x, _pos.y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}
	// parent window has moved. reposition so that we say in the center of the parent window.
	void onParentMove(POINT pt0)
	{
		// the toolbar extents are 0, if OnInitDialog() has not completed yet. if so, quit.
		if (_szDlg.cx == 0 && _szDlg.cy == 0)
			return;
		if (_pos.x == 0 && _pos.y == 0)
			return;
		if (_centerIt)
		{
			_posParent = pt0;
			int x = _posParent.x + (_szParent.cx - _szDlg.cx) / 2;
			int y = _posParent.y + (_szParent.cy - _szDlg.cy) / 2;
			SetWindowPos(_hdlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
			return;
		}
		SIZE delta = {pt0.x-_posParent.x,pt0.y - _posParent.y};
		int x = _pos.x + delta.cx;
		int y = _pos.y + delta.cy;
		_posParent = pt0;
		DBGPRINTF((L"onParentMove(%d, %d): {%d, %d}\n", pt0.x, pt0.y, x, y));
		/*char msg[80];
		sprintf_s(msg, sizeof(msg), "onParentMove(%d, %d): {%d, %d}\n", x0, y0, x, y);
		OutputDebugStringA(msg);*/
		SetWindowPos(_hdlg, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);
	}

protected:
	HWND _hwndTip;

	virtual BOOL OnInitDialog()
	{
		SimpleDlg::OnInitDialog();
		// check the button labelling option.
		if (!_textLabel)
		{
			DBGPUTS((L"ToolsDlg::OnInitDialog"));
			// set a bitmap image on each of the tool buttons.
#ifdef TOOLSDLG_USES_ICON_BUTTON_IMAGES
			SendDlgItemMessage(_hdlg, IDC_BUTTON_ZOOM, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)LoadIcon(AppInstanceHandle, MAKEINTRESOURCE(IDI_BUTTON_ZOOM)));
			SendDlgItemMessage(_hdlg, IDC_BUTTON_ROTATE, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)LoadIcon(AppInstanceHandle, MAKEINTRESOURCE(IDI_BUTTON_ROTATE)));
			SendDlgItemMessage(_hdlg, IDC_BUTTON_PRINT, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)LoadIcon(AppInstanceHandle, MAKEINTRESOURCE(IDI_BUTTON_PRINT)));
			SendDlgItemMessage(_hdlg, IDC_BUTTON_SAVEAS, BM_SETIMAGE, (WPARAM)IMAGE_ICON, (LPARAM)LoadIcon(AppInstanceHandle, MAKEINTRESOURCE(IDI_BUTTON_SAVEAS)));
#else//#ifdef TOOLSDLG_USES_ICON_BUTTON_IMAGES
			SendDlgItemMessage(_hdlg, IDC_BUTTON_ZOOM, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)LoadBitmap(AppInstanceHandle, MAKEINTRESOURCE(IDB_BUTTON_ZOOM)));
			SendDlgItemMessage(_hdlg, IDC_BUTTON_ROTATE, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)LoadBitmap(AppInstanceHandle, MAKEINTRESOURCE(IDB_BUTTON_ROTATE)));
			SendDlgItemMessage(_hdlg, IDC_BUTTON_PRINT, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)LoadBitmap(AppInstanceHandle, MAKEINTRESOURCE(IDB_BUTTON_PRINT)));
			SendDlgItemMessage(_hdlg, IDC_BUTTON_SAVEAS, BM_SETIMAGE, (WPARAM)IMAGE_BITMAP, (LPARAM)LoadBitmap(AppInstanceHandle, MAKEINTRESOURCE(IDB_BUTTON_SAVEAS)));
#endif//#ifdef TOOLSDLG_USES_ICON_BUTTON_IMAGES
			
			// create a tooltip holding window.
			_hwndTip = CreateWindowEx(WS_EX_TOPMOST,
				TOOLTIPS_CLASS,
				NULL,
				WS_VISIBLE | TTS_NOPREFIX | WS_POPUP | TTS_BALLOON | TTS_ALWAYSTIP,
				CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, CW_USEDEFAULT,
				_hdlg, NULL,
				AppInstanceHandle, NULL);
			SetWindowPos(_hwndTip,
				HWND_TOPMOST,
				0, 0, 0, 0,
				SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
			// add a tooltip for each of the tool buttons.
			setTooltip(IDC_BUTTON_ZOOM, (LPCWSTR)resstring(AppInstanceHandle, IDS_BUTTON_ZOOM));
			setTooltip(IDC_BUTTON_ROTATE, (LPCWSTR)resstring(AppInstanceHandle, IDS_BUTTON_ROTATE));
			setTooltip(IDC_BUTTON_PRINT, (LPCWSTR)resstring(AppInstanceHandle, IDS_BUTTON_PRINT));
			setTooltip(IDC_BUTTON_SAVEAS, (LPCWSTR)resstring(AppInstanceHandle, IDS_BUTTON_SAVEAS));
		}
		/* don't assign anything to _szParent. it needs to stay zero. onParentSize() will make the assignment.
		GetWindowRect(_hwndParent, &_rcDlg);
		_szParent = { _rcDlg.right - _rcDlg.left,_rcDlg.bottom - _rcDlg.top };
		*/
		// ptLT points to the left-top corner of the toolbar's client area in screen coordinates.
		POINT ptLT = { 0 };
		ClientToScreen(_hdlg, &ptLT);
		// rcDlg is the toolbar's window frame in screen coordinates.
		GetWindowRect(_hdlg, &_rcDlg);
		// _posParent points to the left-top corner of the window frame.
		_posParent = {_rcDlg.left, _rcDlg.top};
		// _szDlg has the toolbar's horizontal and vertical extents.
		_szDlg = { _rcDlg.right - _rcDlg.left,_rcDlg.bottom - _rcDlg.top };
		// if the user moves the toolbar past the extent of _szMaxTrack, we will force it back to the middle of the image.
		_szMaxTrack.cx = GetSystemMetrics(SM_CXMAXTRACK) - _szDlg.cx;
		_szMaxTrack.cy = GetSystemMetrics(SM_CYMAXTRACK) - _szDlg.cy;
		// the difference between the toolbar's window frame and client area is _viewMargin. it will be used to translate the screen coordinates OnMove() receives to client coordinates. 
		_viewMargin.x = ptLT.x - _posParent.x;
		_viewMargin.y = ptLT.y - _posParent.y;
		// don't return a TRUE. we don't want to move the focus to the toolbar (or one of its buttons).
		return FALSE;
	}
	virtual void OnDestroy()
	{
		if (_hwndTip)
			DestroyWindow(_hwndTip);
		SimpleDlg::OnDestroy();
	}
	// route all button clicks to the parent. we don't do the button operations. parent does.
	virtual BOOL OnCommand(WPARAM wp_, LPARAM lp_)
	{
		switch (LOWORD(wp_))
		{
		case IDC_BUTTON_ZOOM:
		case IDC_BUTTON_ROTATE:
		case IDC_BUTTON_SAVEAS:
		case IDC_BUTTON_PRINT:
			PostMessage(_hwndParent, WM_COMMAND, wp_, lp_);
			return TRUE;
		}
		return SimpleDlg::OnCommand(wp_, lp_);
	}
	// update position info.
	virtual void OnMove(int x, int y)
	{
		// not ready if parent's extents are 0, meaning if onParentSize() has not been invoked.
		if (_szParent.cx == 0 && _szParent.cy == 0)
			return;
		// update the left top corner position. it will be used to keep the toolbar in the same location.
		_pos = { x- _viewMargin.x,y- _viewMargin.y };
	}
	// cursor has moved into the toolbar, show the buttons at full opacity.
	virtual BOOL OnMouseMove(UINT state, POINT pt)
	{
		if (_opacity == 1)
			KillTimer(_hdlg, 1);
		setOpacity(2);
		return FALSE;
	}
	// a standby period has passed. turn off the button display. make them all transparent.
	virtual void OnTimer(WPARAM nIDEvent)
	{
		KillTimer(_hdlg, nIDEvent);
		setOpacity(0);
	}

	// a helper method for setting the opacity level of the toolbar. start a timer to hide the toolbar after 2 seconds of inactivity.
	void setOpacity(int level)
	{
		if (level == _opacity)
			return;
		BYTE op = 0;
		if (level)
			op = level > 1 ? 255 : 126;
		SetLayeredWindowAttributes(_hdlg, 0, op, LWA_ALPHA);
		if (level == 1)
			SetTimer(_hdlg, 1, 2000, NULL);
		_opacity = level;
	}
	// a helper method for assigning tip text to a button.
	LRESULT setTooltip(UINT uidCtrl, LPCTSTR pszTip)
	{
		// initialize members of the toolinfo structure.
		HWND hwndCtrl = GetDlgItem(_hdlg, uidCtrl);
		TOOLINFO ti = { sizeof(TOOLINFO) };
		ti.uFlags = TTF_IDISHWND | TTF_SUBCLASS;
		ti.hwnd = _hdlg;
		ti.uId = (UINT_PTR)hwndCtrl;
		if (!pszTip)
		{
			return SendMessage(_hwndTip, TTM_DELTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
		}
		// assign the button's rectangle and tip text.
		GetClientRect(hwndCtrl, &ti.rect);
		ti.lpszText = (LPTSTR)pszTip;
		// invoke ADDTOOL for the button.
		return SendMessage(_hwndTip, TTM_ADDTOOL, 0, (LPARAM)(LPTOOLINFO)&ti);
	}
};

