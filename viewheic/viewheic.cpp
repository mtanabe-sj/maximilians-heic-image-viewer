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
// viewheic.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "viewheic.h"
#include "app.h"
#include "AboutDlg.h"


// this embeds a manifest for using v6 common controls of win32.
// it is needed for a SysLink control used in the about box.
#pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


#define VIEWHEIC_FLAG_CONVERT_TO_JPEG 0x00000001

#ifdef _DEBUG
#define PHOTOMARGIN_CX 0
#define PHOTOMARGIN_CY 0
#else//#ifdef _DEBUG
#define PHOTOMARGIN_CX 0
#define PHOTOMARGIN_CY 0
#endif//#ifdef _DEBUG


// global variables
HMODULE AppInstanceHandle;
ULONG LibRefCount;

bool parseCommandline(LPCWSTR cmdline, std::vector<std::wstring> &paths, ULONG &outflags);
HRESULT convertHeifToJpeg(LPCWSTR srcfile, LPCWSTR destfile);

//////////////////////////////////////////////////////////////////////////

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPWSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	UNREFERENCED_PARAMETER(hPrevInstance);
	AppInstanceHandle = hInstance;
	LibRefCount = 0;

	ULONG flags = 0;
	std::vector<std::wstring> args;
	if (!parseCommandline(lpCmdLine, args, flags))
		return ERROR_INVALID_PARAMETER;

	if (flags & VIEWHEIC_FLAG_CONVERT_TO_JPEG)
	{
		if (args.size() != 2)
			return ERROR_INVALID_PARAMETER;
		return convertHeifToJpeg(args[0].c_str(), args[1].c_str());
	}

	MainDlg dlg(args[0]);
	dlg.DoModal();
	return ERROR_SUCCESS;
}

bool parseCommandline(LPCWSTR cmdline, std::vector<std::wstring> &args, ULONG &outflags)
{
	strvalenum senum(' ', cmdline);
	LPVOID pos = senum.getHeadPosition();
	while (pos)
	{
		LPCWSTR p = senum.getNext(pos);
		if (*p == '-')
		{
			if (p[1] == 'j')
				outflags |= VIEWHEIC_FLAG_CONVERT_TO_JPEG;
			else
				return false;
		}
		else
			args.push_back(p);
	}
	if (args.size() == 0)
		args.push_back(L"");
	return true;
}

HRESULT convertHeifToJpeg(LPCWSTR srcfile, LPCWSTR destfile)
{
	HRESULT hr = CoInitialize(NULL);
	if (SUCCEEDED(hr))
	{
		HeifImage heif2;
		if (heif2.init(NULL, srcfile, 0))
		{
			HBITMAP hbmp = heif2.decode({ 0 });
			if (hbmp)
			{
				DeleteObject(hbmp);
				hr = heif2.convert(destfile);
			}
			else
				hr = E_FAIL;
		}
		else
			hr = E_UNEXPECTED;
		CoUninitialize();
	}
	return hr;
}


////////////////////////////////////////////////////////////////
#define SCROLLER_MORE_TRANSPARENT 0x7f
#define SCROLLER_LESS_TRANSPARENT 0xaa
#define SCROLLER_MARGIN 2
#define SCROLLER_WIDTH 8
#define SCROLLER_WIDTHF 8.0
#define SCROLLER_STEP_DISTANCE 10

BOOL VIEWINFO::blendRect(HDC hdc, int x, int y, int cx, int cy, BYTE transparency)
{
	if (cx <= 0 || cy <= 0) return FALSE;
	HDC hdc2 = CreateCompatibleDC(hdc);
	if (hdc2) {
		BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER), cx, cy, 1, 32, BI_RGB, (DWORD)(cx*cy * 4) };
		UINT32* bits;
		HBITMAP hbmp = CreateDIBSection(hdc2, &bmi, DIB_RGB_COLORS, (LPVOID*)&bits, NULL, 0);
		if (hbmp) {
			RECT rc = {0,0,cx,cy};
			FillRect(hdc2, &rc, GetStockBrush(GRAY_BRUSH));
			SelectObject(hdc2, hbmp);
			BLENDFUNCTION bf = { AC_SRC_OVER, 0, transparency, 0 };
			AlphaBlend(hdc, x, y, cx, cy, hdc2, 0, 0, cx, cy, bf);
			DeleteObject(hbmp);
		}
		DeleteDC(hdc2);
	}
	return TRUE;
}

void VIEWINFO::drawScrollerH(HDC hdc, BYTE transparency)
{
	float rco, rci, rcw, r, rd;
	rci = (float)szImage.cx;
	rcw = (float)szPage.cx;
	rco = (float)(-ptPage.x);
	rcw -= SCROLLER_WIDTHF;
	r = rco / rci;
	rd = rcw / rci;
	int x = (int)(r*rcw) + SCROLLER_MARGIN;
	int cx = (int)(rd*rcw);
	rectH = { x, szPage.cy - SCROLLER_MARGIN - SCROLLER_WIDTH, cx, SCROLLER_WIDTH };
	blendRect(hdc, rectH.left, rectH.top, rectH.right, rectH.bottom, transparency);
	rectH.right += rectH.left;
	rectH.bottom += rectH.top;
}

void VIEWINFO::drawScrollerV(HDC hdc, BYTE transparency)
{
	float rco, rci, rcw, r, rd;
	rci = (float)szImage.cy;
	rcw = (float)szPage.cy;
	rco = (float)(-ptPage.y);
	rcw -= SCROLLER_WIDTHF;
	r = rco / rci;
	rd = rcw / rci;
	int y = (int)(r*rcw) + SCROLLER_MARGIN;
	int cy = (int)(rd*rcw);
	rectV = { szPage.cx - SCROLLER_MARGIN - SCROLLER_WIDTH, y, SCROLLER_WIDTH, cy };
	blendRect(hdc, rectV.left, rectV.top, rectV.right, rectV.bottom, transparency);
	rectV.right += rectV.left;
	rectV.bottom += rectV.top;
}

BOOL VIEWINFO::drawScrollers(HDC hdc, BYTE transparency)
{
	if (scrollH & VI_SCROLLER_VISIBLE)
		drawScrollerH(hdc, transparency);

	if (scrollV & VI_SCROLLER_VISIBLE)
		drawScrollerV(hdc, transparency);

	return TRUE;
}

void VIEWINFO::onMouseMove(HWND hwnd, UINT state, POINT pt)
{
	if (scrollH & VI_SCROLLER_VISIBLE)
	{
		if (scrollH & VI_SCROLLER_CAPTURED)
		{
			int dx = pt.x - ptCapture.x;
			int cx = MulDiv(szImage.cx, dx, szPage.cx);
			// a negative change in cursor position means a scrollbar moving left. a positive change a scrollbar moving right.
			ptPage.x -= cx;
			int xmin = szPage.cx - szImage.cx;
			if (xmin <= ptPage.x && ptPage.x < 0)
				InvalidateRect(hwnd, NULL, TRUE);
			if (ptPage.x < xmin)
				ptPage.x = xmin;
			else if (ptPage.x > 0)
				ptPage.x = 0;
			ptCapture = pt;
			return;
		}
		if (PtInRect(&rectH, pt))
		{
			// cursor has moved into the scrollbar area. increase opacity. make it less transparent.
			HDC hdc = GetDC(hwnd);
			drawScrollerH(hdc, SCROLLER_LESS_TRANSPARENT);
			ReleaseDC(hwnd, hdc);
			scrollH |= VI_SCROLLER_SHADED;
		}
		else if (scrollH & VI_SCROLLER_SHADED)
		{
			// cursor has moved out. reduce opacity.
			InvalidateRect(hwnd, &rectH, TRUE);
			scrollH &= ~VI_SCROLLER_SHADED;
		}
	}
	if (scrollV & VI_SCROLLER_VISIBLE)
	{
		if (scrollV & VI_SCROLLER_CAPTURED)
		{
			int dy = pt.y - ptCapture.y;
			int cy = MulDiv(szImage.cy, dy, szPage.cy);
			// a negative change in cursor position means a scrollbar moving up. a positive change a scrollbar moving down.
			ptPage.y -= cy;
			int ymin = szPage.cy - szImage.cy;
			if (ymin <= ptPage.y && ptPage.y < 0)
				InvalidateRect(hwnd, NULL, TRUE);
			if (ptPage.y < ymin)
				ptPage.y = ymin;
			else if (ptPage.y > 0)
				ptPage.y = 0;
			ptCapture = pt;
			return;
		}
		if (PtInRect(&rectV, pt))
		{
			HDC hdc = GetDC(hwnd);
			drawScrollerV(hdc, SCROLLER_LESS_TRANSPARENT);
			ReleaseDC(hwnd, hdc);
			scrollV |= VI_SCROLLER_SHADED;
		}
		else if (scrollV & VI_SCROLLER_SHADED)
		{
			InvalidateRect(hwnd, &rectV, TRUE);
			scrollV &= ~VI_SCROLLER_SHADED;
		}
	}
}

bool VIEWINFO::onMouseWheel(short nButtonType, short nWheelDelta, POINT pt)
{
	if (scrollH & VI_SCROLLER_VISIBLE)
	{
		// wheel sideways if the mouse cursor is in a region near the bottom edge of the window (including the horizontal scrollbar) 
		RECT rc = { 0, rectH.top - 10, szPage.cx, szPage.cy };
		if (PtInRect(&rc, pt))
		{
			int x0 = ptPage.x;
			int dx = szImage.cx - szPage.cx;
			if (nWheelDelta > 0)
			{
				// scroll sideways to the left
				do {
					// note x is negative. so stop if it's no longer negative.
					ptPage.x += SCROLLER_STEP_DISTANCE;
					if (ptPage.x > 0)
					{
						ptPage.x = 0;
						break;
					}
				} while ((nWheelDelta -= WHEEL_DELTA) > 0);
			}
			else if (nWheelDelta < 0)
			{
				// scroll to the right
				do {
					ptPage.x -= SCROLLER_STEP_DISTANCE;
					if (ptPage.x < -dx)
					{
						ptPage.x = -dx;
						break;
					}
				} while ((nWheelDelta += WHEEL_DELTA) < 0);
			}
			return (x0 != ptPage.x);
		}
	}
	if (scrollV & VI_SCROLLER_VISIBLE)
	{
		int y0 = ptPage.y;
		int dy = szImage.cy - szPage.cy;
		if (nWheelDelta > 0)
		{
			// wheel is moving forward; scroll up
			do {
				// note y is negative. so stop if it's no longer negative.
				ptPage.y += SCROLLER_STEP_DISTANCE;
				if (ptPage.y > 0)
				{
					ptPage.y = 0;
					break;
				}
			} while ((nWheelDelta -= WHEEL_DELTA) > 0);
		}
		else if (nWheelDelta < 0)
		{
			// wheeling backward; scroll down
			do {
				ptPage.y -= SCROLLER_STEP_DISTANCE;
				if (ptPage.y < -dy)
				{
					ptPage.y = -dy;
					break;
				}
			} while ((nWheelDelta += WHEEL_DELTA) < 0);
		}
		return (y0 != ptPage.y);
	}
	return FALSE;
}

bool VIEWINFO::startScroll(HWND hwnd, UINT buttonType, POINT pt)
{
	if (PtInRect(&rectH, pt))
	{
		scrollH |= VI_SCROLLER_CAPTURED;
		ptCapture = pt;
		rectCapture = rectH;
		SetCapture(hwnd);
		return true;
	}
	if (PtInRect(&rectV, pt))
	{
		scrollV |= VI_SCROLLER_CAPTURED;
		ptCapture = pt;
		rectCapture = rectV;
		SetCapture(hwnd);
		return true;
	}
	return false;
}

bool VIEWINFO::stopScroll(HWND hwnd, UINT buttonType, POINT pt)
{
	if (scrollH & VI_SCROLLER_CAPTURED)
	{
		scrollH &= ~VI_SCROLLER_CAPTURED;
		ptCapture = { 0 };
		ReleaseCapture();
		return true;
	}
	if (scrollV & VI_SCROLLER_CAPTURED)
	{
		scrollV &= ~VI_SCROLLER_CAPTURED;
		ptCapture = { 0 };
		ReleaseCapture();
		return true;
	}
	return false;
}

//////////////////////////////////////////////////////////////////////////

BOOL MainDlg::OnInitDialog()
{
	SimpleDlg::OnInitDialog();

	INITCOMMONCONTROLSEX ccx = { sizeof(INITCOMMONCONTROLSEX), ICC_TREEVIEW_CLASSES };
	InitCommonControlsEx(&ccx);

	// set up a custom icon for the app.
	HICON hicon = LoadIcon(AppInstanceHandle, MAKEINTRESOURCE(IDI_APP)); // note we don't save the handle in _hicon.
	SendMessage(_hdlg, WM_SETICON, ICON_BIG, (LPARAM)hicon);
	hicon = LoadIcon(AppInstanceHandle, MAKEINTRESOURCE(IDI_APP_SMALL));
	SendMessage(_hdlg, WM_SETICON, ICON_SMALL, (ULONG_PTR)hicon);
	// add 'about this app' menu command to the system menu.
	HMENU hsysmenu = GetSystemMenu(_hdlg, FALSE);
	if (hsysmenu)
	{
		InsertMenu(hsysmenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);
		InsertMenu(hsysmenu, -1, MF_BYPOSITION | MF_STRING, IDM_ABOUT, (LPCWSTR)resstring(AppInstanceHandle, IDS_ABOUT_APP));
	}

	// load optional settings from the user's registry.
	DWORD val;
	if (ERROR_SUCCESS == Registry_GetDwordValue(HKEY_CURRENT_USER, APPREGKEY, L"ViewFlags", &val))
		_heifFlags = val;
	// create the toolbar as a modeless dialog.
	_tools = new ToolsDlg(_hdlg, _heifFlags & VIEWHEIC_ALWAYS_CENTER_TOOLBAR, _heifFlags & VIEWHEIC_TEXT_LABEL_ON_TOOLBAR);
	_tools->Create();

	// find the margins, the difference between the window frame and the image display area.
	RECT rcFrame, rcClient;
	GetWindowRect(_hdlg, &rcFrame);
	GetClientRect(_hdlg, &rcClient);
	_frameMargins.cx = rcFrame.right - rcFrame.left - rcClient.right;
	_frameMargins.cy = rcFrame.bottom - rcFrame.top - rcClient.bottom;

	// if an image filename has been given, open it and display the image. otherwise, run a file browse dialog and let the user choose an image to show.
	if (_filename.empty())
		PostMessage(_hdlg, WM_COMMAND, MAKEWPARAM(IDM_OPEN, 0), 0);
	else
		PostMessage(_hdlg, WM_COMMAND, MAKEWPARAM(IDM_DECODE, 0), 0);
	return TRUE;
}

void MainDlg::OnDestroy()
{
	if (_tools)
	{
		_tools->Destroy();
		delete _tools;
	}
	if (_heif)
		delete _heif;
	if (_hbmpImage)
		DeleteObject(_hbmpImage);
	SimpleDlg::OnDestroy();
}

void MainDlg::_setCaption(ULONG flags)
{
	LPCWSTR caption = wcsrchr(_filename.c_str(), '\\');
	if (caption)
		caption++;
	else
		caption = _filename.c_str();
	std::wstring c;
	LPCWSTR extension = wcsrchr(caption, '.');
	if (extension)
		c.assign(caption, extension - caption);
	else
		c = caption;
	int deg = 0;
	if (_heif && _heif->_data.rotated > 0)
		deg = (_heif->_data.rotated % 4) * 90;
	if (deg)
	{
		resstring rotationAngle(resstring(AppInstanceHandle, IDS_ROTATION_ANGLE_FMT), deg);
		c += rotationAngle;
	}
	if (_vi.zoomRatio && ((flags & 1) || (_vi.zoomRatio != 100)))
	{
		resstring percentZoom(L" (%d%%)", _vi.zoomRatio);
		c += percentZoom;
	}
	SetWindowText(_hdlg, c.c_str());
}

void MainDlg::_autoZoom(bool syncParams)
{
	SIZE szView;
	int zoom = 0;
	if (_heif->_imageSize.cx > _vi.szScreen.cx || _heif->_imageSize.cy > _vi.szScreen.cy)
	{
		RECT rc;
		int z = PreserveAspectRatio(_heif->_imageSize.cx, _heif->_imageSize.cy, _vi.szScreen.cx, _vi.szScreen.cy, &rc);
		zoom = (z / 5) * 5;
		if (zoom < z)
		{
			double fz = 0.01*(double)zoom;
			SIZE s2 = _heif->_imageSize;
			s2.cx = (int)(fz*(double)s2.cx);
			s2.cy = (int)(fz*(double)s2.cy);
			PreserveAspectRatio(s2.cx, s2.cy, _vi.szScreen.cx, _vi.szScreen.cy, &rc);
		}
		szView.cx = rc.right - rc.left;
		szView.cy = rc.bottom - rc.top;
	}
	else
	{
		szView = _heif->_imageSize;
		zoom = 100; // 100 percent; no zoom
	}
	if (syncParams)
	{
		_vi.zoomAuto = _vi.zoomRatio = zoom;
		_vi.szImage = _vi.szAuto = _vi.szPage = szView;
		_vi.ptPage = { 0 };
	}
	_sizeWindow(szView);
}

void MainDlg::_sizeWindow(SIZE viewSize)
{
	SIZE szDlg = { viewSize.cx + 2 * PHOTOMARGIN_CX + _frameMargins.cx, viewSize.cy + 2 * PHOTOMARGIN_CY + _frameMargins.cy };
	SetWindowPos(_hdlg, NULL, 0, 0, szDlg.cx, szDlg.cy, SWP_NOMOVE | SWP_NOZORDER);
}

BOOL MainDlg::OnSysCommand(WPARAM wp_, LPARAM lp_)
{
	if ((wp_ & 0xF000) == 0)
	{
		// not a system menu command. see if it's one of ours.
		if (wp_ == IDM_ABOUT)
		{
			// let our regular handler respond to it.
			PostMessage(_hdlg, WM_COMMAND, wp_ & 0x00000FFF, 0);
			return TRUE;
		}
	}
	return SimpleDlg::OnSysCommand(wp_, lp_);
}

BOOL MainDlg::OnCommand(WPARAM wp_, LPARAM lp_)
{
	switch ((LOWORD(wp_)))
	{
	case IDM_DECODE:
		if (lp_ == 0)
		{
			_setCaption();

			HeifImage *h = new HeifImage();
			if (h->init(_hdlg, _filename.c_str(), _heifFlags))
			{
				if (_heif)
					delete _heif;
				_heif = h;
				_autoZoom(true);
				PostMessage(_hdlg, WM_COMMAND, MAKEWPARAM(IDM_DECODE, 0), 1);
			}
			else
				delete h;
		}
		else if (lp_ == 1)
		{
			SetCursor(LoadCursor(NULL, IDC_WAIT));
			if (_hbmpImage)
				DeleteObject(_hbmpImage);
			_hbmpImage = _heif->decode(_vi.szImage);
			SetCursor(NULL);
			InvalidateRect(_hdlg, NULL, TRUE);
		}
		return TRUE;
	case IDM_OPEN:
		OnButtonOpen();
		return TRUE;

	case IDC_BUTTON_ROTATE:
		OnButtonRotate();
		return TRUE;
	case IDC_BUTTON_ZOOM:
		OnButtonZoom();
		return TRUE;
	case IDC_BUTTON_SAVEAS:
		OnButtonSaveAs();
		return TRUE;
	case IDC_BUTTON_PRINT:
		OnButtonPrint();
		return TRUE;

	case IDM_ABOUT:
		{
		AboutDlg dlg(_hdlg);
		dlg.DoModal();
		return TRUE;
		}
	}
	return SimpleDlg::OnCommand(wp_, lp_);
}

void MainDlg::OnButtonRotate()
{
	if (!_heif)
		return;
	// put up a wait cursor.
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	// free the old bitmap.
	if (_hbmpImage)
		DeleteObject(_hbmpImage);
	// get the heic image rotated by 45 degrees.
	_hbmpImage = _heif->rotate(_vi.szImage);
	// update the window size so that the rotated image fits.
	// note that szAuto has optimal viewport extents. it's the reference size we go back to after each operation of zoom or rotation, although the cx and cy may have to be swapped. szImage is the actual image extents after the rotation. if the orientations of the two sets of extents differ, swap the extents of szAuto to correct the orientation of the reference size.
	_vi.szPage = _vi.szImage;
	SIZE szRef = _vi.szAuto;
	if (_vi.szAuto.cy > _vi.szAuto.cx && _vi.szImage.cy < _vi.szImage.cx)
		std::swap(szRef.cx, szRef.cy);
	// szPage has the extents of the displayable portion of the image. if the rotated image exceeds the reference extent, limit the displayable to it.
	if (_vi.szPage.cx > szRef.cx)
		_vi.szPage.cx = szRef.cx;
	if (_vi.szPage.cy > szRef.cy)
		_vi.szPage.cy = szRef.cy;
	// ptPage.x is the distance from the left endpoint of the full image width and the starting point of the displayable width. it says how much of the image is pushed to the left of the viewport and so is hidden from view.
	_vi.ptPage.x = (_vi.szPage.cx - _vi.szImage.cx) / 2;
	_vi.ptPage.y = (_vi.szPage.cy - _vi.szImage.cy) / 2;
	// determine if a scrollbar should be shown.
	if (_vi.szPage.cx < _vi.szImage.cx)
		_vi.scrollH |= VI_SCROLLER_VISIBLE;
	else
		_vi.scrollH &= ~VI_SCROLLER_VISIBLE;
	if (_vi.szPage.cy < _vi.szImage.cy)
		_vi.scrollV |= VI_SCROLLER_VISIBLE;
	else
		_vi.scrollV &= ~VI_SCROLLER_VISIBLE;
	// correct the window size.
	_sizeWindow(_vi.szPage);
	// print the new rotation angle next to the file name on the caption bar.
	_setCaption();
	// take out the wait cursor.
	SetCursor(NULL);
	// paint the rotated image in the updated viewport.
	InvalidateRect(_hdlg, NULL, TRUE);
	// don't want to keep the focus with the toolbar button. move it to the image window. this is needed for directing the mouse wheel to the image.
	SetFocus(_hdlg);
}

void MainDlg::OnButtonZoom()
{
	if (!_heif)
		return;
	// show an hour glass.
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	// if the image has been rotated, make sure the window or viewport is sized correctly.
	if (_heif->_data.rotated)
		_autoZoom();
	// free the current bitmap.
	if (_hbmpImage)
		DeleteObject(_hbmpImage);
	// find the next zoom ratio.
	if (_vi.zoomRatio < 25)
		_vi.zoomRatio = 25;
	else if (_vi.zoomRatio < 50)
		_vi.zoomRatio = 50;
	else if (_vi.zoomRatio < 75)
		_vi.zoomRatio = 75;
	else if (_vi.zoomRatio < 100)
		_vi.zoomRatio = 100;
	else if (_vi.zoomRatio < 150)
		_vi.zoomRatio = 150;
	else if (_vi.zoomRatio < 200)
		_vi.zoomRatio = 200;
	else // back to the optimized size.
		_vi.zoomRatio = _vi.zoomAuto;
	// zoom the image.
	_hbmpImage = _heif->zoom(_vi.zoomRatio, _vi.szImage);
	// we keep the current window size. we won't change it even though we know the zoomed image must be larger. instead we will show the scrollbars.
	// we're centering the image. calculate how much of it should go hidden past the left edge of the viewport.
	_vi.ptPage.x = (_vi.szPage.cx - _vi.szImage.cx) / 2;
	_vi.ptPage.y = (_vi.szPage.cy - _vi.szImage.cy) / 2;
	// enable the scrollbars.
	if (_vi.zoomRatio > _vi.zoomAuto)
	{
		_vi.scrollH |= VI_SCROLLER_VISIBLE;
		_vi.scrollV |= VI_SCROLLER_VISIBLE;
	}
	else
	{
		_vi.scrollH &= ~VI_SCROLLER_VISIBLE;
		_vi.scrollV &= ~VI_SCROLLER_VISIBLE;
	}
	// print the new percent zoom ratio on the caption bar.
	_setCaption(1);
	// stop the wait cursor.
	SetCursor(NULL);
	// paint the zoomed image.
	InvalidateRect(_hdlg, NULL, TRUE);
	// return the focus to the image window.
	SetFocus(_hdlg);
}

void MainDlg::OnMove(int x, int y)
{
	// report the window move to the toolbar so the latter can relocate itself in sync.
	if (_tools)
		_tools->onParentMove({ x, y });
}

void MainDlg::OnSize(UINT nType, int cx, int cy)
{
	// keep the changed frame size.
	_vi.szPage.cx = cx-2* PHOTOMARGIN_CX;
	_vi.szPage.cy = cy-2* PHOTOMARGIN_CY;
	// report the changed frame to the toolbar.
	if (_tools)
		_tools->onParentSize({ cx, cy });
}

BOOL MainDlg::OnMouseWheel(short nButtonType, short nWheelDelta, POINT pt)
{
	// the cursor position in pt is in screen coordinates. so, translate to dialog's client coords.
	ScreenToClient(_hdlg, &pt);
	if (_vi.onMouseWheel(nButtonType, nWheelDelta, pt))
		InvalidateRect(_hdlg, NULL, TRUE);
	return TRUE;
}

BOOL MainDlg::OnMouseMove(UINT state, POINT pt)
{
	// report the cursor motion to the toolbar which may need to bring the buttons out of the transpant state and make them visible.
	if (_tools)
		_tools->onParentMouseMove(state, pt);
	// report the mouse event to the viewinfo so it can make the scrollbars visible.
	_vi.onMouseMove(_hdlg, state, pt);
	return FALSE;
}

BOOL MainDlg::OnLButtonDown(INT_PTR nButtonType, POINT pt)
{
	// if a sustained click-down occurs on a scrollbar, prepare to scroll the image. then, change the cursor shape to a hand with a pointing fingure.
	if (_vi.startScroll(_hdlg, (UINT)nButtonType, pt))
		SetCursor(LoadCursor(NULL, IDC_HAND));
	return TRUE;
}

BOOL MainDlg::OnLButtonUp(INT_PTR nButtonType, POINT pt)
{
	// the click is over. stop the scrolling. switch back to the standard cursor.
	if (_vi.stopScroll(_hdlg, (UINT)nButtonType, pt))
		SetCursor(NULL);
	return TRUE;
}

BOOL MainDlg::OnPaint()
{
	// to render the heic image, we bitblt the decoded bitmap on to the screen device context.
	PAINTSTRUCT	ps;
	BeginPaint(_hdlg, (LPPAINTSTRUCT)&ps);

	// pt is the origin, the upper-left corner of the image.
	POINT pt = _vi.ptPage;
#if PHOTOMARGIN_CX > 0
	// paint the margin strips in white that surround the image.
	//TODO: if the image has been zoomed in, some or all of the margin strips may fall outside the viewport. they should not be painted at all.
	RECT rc = { pt.x, pt.y, pt.x + 2 * PHOTOMARGIN_CX + _vi.szImage.cx, pt.y + PHOTOMARGIN_CY };
	FillRect(ps.hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
	rc = { pt.x, pt.y, pt.x + PHOTOMARGIN_CX, pt.y + 2 * PHOTOMARGIN_CY + _vi.szImage.cy };
	FillRect(ps.hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
	rc = { pt.x + PHOTOMARGIN_CX+_vi.szImage.cx, pt.y, pt.x + 2 * PHOTOMARGIN_CX + _vi.szImage.cx, pt.y + 2 * PHOTOMARGIN_CY + _vi.szImage.cy };
	FillRect(ps.hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
	rc = { pt.x, pt.y + PHOTOMARGIN_CY + _vi.szImage.cy, pt.x + 2 * PHOTOMARGIN_CX + _vi.szImage.cx, pt.y + 2 * PHOTOMARGIN_CY + _vi.szImage.cy };
	FillRect(ps.hdc, &rc, (HBRUSH)GetStockObject(WHITE_BRUSH));
#endif//#if PHOTOMARGIN_CX > 0
	if (_hbmpImage)
	{
		BITMAP bm;
		GetObject(_hbmpImage, sizeof(bm), &bm);
#if PHOTOMARGIN_CX > 0
		pt.x += PHOTOMARGIN_CX;
		pt.y += PHOTOMARGIN_CY;
#endif//PHOTOMARGIN_CX
		HDC hdc2 = CreateCompatibleDC(ps.hdc);
		HBITMAP hbmp0 = (HBITMAP)SelectObject(hdc2, _hbmpImage);
		BitBlt(ps.hdc, pt.x, pt.y, bm.bmWidth, bm.bmHeight, hdc2, 0, 0, SRCCOPY);
		SelectObject(hdc2, hbmp0);
		DeleteDC(hdc2);

		// overlay the scrollbars if they should be visible.
		_vi.drawScrollers(ps.hdc, SCROLLER_MORE_TRANSPARENT);
	}

	EndPaint(_hdlg, (LPPAINTSTRUCT)&ps);
	return TRUE;
}

BOOL MainDlg::OnEraseBkgnd(HDC hdc)
{
	// return a true to report to the system that we've handled the background erase. it's to prevent the system from doing an actual erase. it would cause a bad screen flicker.
	return TRUE;
}

void MainDlg::OnButtonOpen()
{
	resstring filter(AppInstanceHandle, IDS_OPEN_FILTER);
	WCHAR path[_MAX_PATH] = { 0 };
	OPENFILENAME ofn = { sizeof(OPENFILENAME), 0 };
	ofn.hwndOwner = _hdlg;
	ofn.Flags = OFN_FILEMUSTEXIST;
	ofn.lpstrFilter = filter;
	ofn.lpstrDefExt = L".heic";
	ofn.lpstrFile = path;
	ofn.nMaxFile = ARRAYSIZE(path);
	if (!GetOpenFileName(&ofn))
	{
		DWORD cderr = CommDlgExtendedError(); // CDERR_DIALOGFAILURE, etc.
		if (cderr != 0) // 0 if user has canceled.
			MessageBox(_hdlg, resstring(resstring(AppInstanceHandle, IDS_CDERR), cderr), resstring(AppInstanceHandle, IDS_APP_TITLE), MB_OK | MB_ICONEXCLAMATION);
		// user canceled. we're shutting down.
		PostMessage(_hdlg, WM_CLOSE, 0, 0);
		return;
	}
	// we have a hiec image. save its filename. then, issue a command to decode and display it.
	_filename = path;
	PostMessage(_hdlg, WM_COMMAND, MAKEWPARAM(IDM_DECODE, 0), 0);
}

int MainDlg::_queryFileTitle(LPWSTR buf, int bufSize)
{
	if (_filename.empty())
		return -1;
	return CopyFileTitle(_filename.c_str(), buf, bufSize);
}

void MainDlg::OnButtonSaveAs()
{
	WCHAR path[_MAX_PATH] = { 0 };
	_queryFileTitle(path, ARRAYSIZE(path));

	resstring filter(AppInstanceHandle, IDS_SAVEAS_FILTER);
	OPENFILENAME ofn = { sizeof(OPENFILENAME), 0 };
	ofn.hwndOwner = _hdlg;
	ofn.Flags = OFN_PATHMUSTEXIST;
	ofn.lpstrFilter = filter;
	ofn.lpstrDefExt = L".jpg";
	ofn.lpstrFileTitle;
	ofn.lpstrFile = path;
	ofn.nMaxFile = ARRAYSIZE(path);
	if (!GetSaveFileName(&ofn))
	{
		DWORD cderr = CommDlgExtendedError(); // CDERR_DIALOGFAILURE, etc.
		if (cderr != 0) // 0 if user has canceled.
			MessageBox(_hdlg, resstring(resstring(AppInstanceHandle, IDS_CDERR), cderr), resstring(AppInstanceHandle, IDS_APP_TITLE), MB_OK | MB_ICONEXCLAMATION);
		return;
	}
	// we have a destination filename. find the destination image encoding type based on the file extension name. then, we will convert the heic to it and save the converted image in the destination file.
	SetCursor(LoadCursor(NULL, IDC_WAIT));
	// if the current hief object holds a full-scale bitmap, we will use it and make the conversion in one quick step.
	if (_heif && _heif->hasFullscaleData())
	{
		// we have a full-scale image. just need to run the conversion.
		_heif->convert(path);
	}
	else
	{
		// the current hief object is not at full scale. we need to create a temp heif and decode the image at full scale from scratch.
		HeifImage heif2;
		if (heif2.init(_hdlg, _filename.c_str(), _heifFlags))
		{
			HBITMAP hbmp = heif2.decode({ 0 });
			if (hbmp)
			{
				//TODO: we don't need a win32 HBITMAP generated by the decoder in the first place.
				DeleteObject(hbmp);
				// we have a decoded bitmap. encode it in the destination format and save it it the file at path.
				heif2.convert(path);
			}
		}
	}
	SetCursor(NULL);
}


/* _setupDevMode
prepares a DEVMODE structure for a custom printer configuration.
currently, only paper orientation is supported.
*/
HGLOBAL _setupDevMode(short paperOrientation)
{
	HGLOBAL hg = GlobalAlloc(GMEM_MOVEABLE| GMEM_ZEROINIT, sizeof(DEVMODE));
	DEVMODE* pdm = (DEVMODE*)GlobalLock(hg);
	pdm->dmSize = sizeof(DEVMODE);
	pdm->dmFields |= DM_ORIENTATION;
	pdm->dmOrientation = paperOrientation; // e.g., DMORIENT_LANDSCAPE
	GlobalUnlock(hg);
	return hg;
}

/* _queryPrintDimensions
calculates paper margins and printed extents of an input bitmap image that are appropriate for a printer device context.
*/
int _queryPrintDimensions(PRINTDLGEX *pdx, HeifImage *heif, POINT &printMargin, SIZE &printExtents)
{
	// printer dpi (typically 600)
	SIZE devDpi = { GetDeviceCaps(pdx->hDC, LOGPIXELSX), GetDeviceCaps(pdx->hDC, LOGPIXELSY) };
	// width and height resolutions in pixels
	SIZE devRes = { GetDeviceCaps(pdx->hDC, HORZRES), GetDeviceCaps(pdx->hDC, VERTRES) };
	// dimensions of the paper, translated to 1/10000"
	SIZE paperExts = { MulDiv(10000,devRes.cx,devDpi.cx),MulDiv(10000,devRes.cy,devDpi.cy) };

#ifdef _DEBUG
	short orientation_ = 0;
	short paperSize_ = 0;
	short paperWidth_ = 0;
	short paperLength_ = 0;
	short scale_ = 0;
	if (pdx->hDevMode)
	{
		DEVMODE *pdm = (DEVMODE*)GlobalLock(pdx->hDevMode);
		if (pdm->dmFields & DM_YRESOLUTION)
		{
			devDpi.cx = pdm->dmPrintQuality; // x resolution
			devDpi.cy = pdm->dmYResolution; // y resolution
		}
		if (pdm->dmFields & DM_ORIENTATION)
			orientation_ = pdm->dmOrientation; // DMORIENT_PORTRAIT (1), DMORIENT_LANDSCAPE (2)
		if (pdm->dmFields & DM_PAPERSIZE)
			paperSize_ = pdm->dmPaperSize; // DMPAPER_LETTER (1), DMPAPER_A4 (9)
		if (pdm->dmFields & DM_PAPERWIDTH)
			paperWidth_ = pdm->dmPaperWidth; // width of the paper in 1/10 mm
		if (pdm->dmFields & DM_PAPERLENGTH)
			paperLength_ = pdm->dmPaperLength; // length of the paper in 1/10 mm
		if (pdm->dmFields & DM_SCALE)
			scale_ = pdm->dmScale; // scaling factor in percents
		GlobalUnlock(pdx->hDevMode);
	}
#endif//#ifdef _DEBUG

	int imageRes = heif->queryDPI();
	SIZE desiredExts;
	desiredExts.cx = MulDiv(10000, heif->_imageSize.cx, imageRes);
	desiredExts.cy = MulDiv(10000, heif->_imageSize.cy, imageRes);
	SIZE actualExts = desiredExts;
	if (desiredExts.cx > paperExts.cx || desiredExts.cy > paperExts.cy)
	{
		// the image width or height or both exceeds the corresponding paper dimension.
		// auto-scale the printed image. we are ignoring scale_.
		double rx = (double)desiredExts.cx / (double)paperExts.cx;
		double ry = (double)desiredExts.cy / (double)paperExts.cy;

		if (rx > 1.0 && ry > 1.0)
		{
			if (rx > ry)
			{
				ASSERT(FALSE);
				actualExts.cx = paperExts.cx;
				actualExts.cy = MulDiv(paperExts.cx, desiredExts.cy, desiredExts.cx);
			}
			else if (rx < ry)
			{
				ASSERT(FALSE);
				actualExts.cy = paperExts.cy;
				actualExts.cx = MulDiv(paperExts.cy, desiredExts.cx, desiredExts.cy);
			}
			else
			{
				actualExts.cx = paperExts.cx;
				actualExts.cy = paperExts.cy;
			}
		}
		else if (rx > 1.0)
		{
			actualExts.cx = paperExts.cx;
			actualExts.cy = MulDiv(paperExts.cx, desiredExts.cy, desiredExts.cx);
		}
		else if (ry > 1.0)
		{
			actualExts.cy = paperExts.cy;
			actualExts.cx = MulDiv(paperExts.cy, desiredExts.cx, desiredExts.cy);
		}
	}
	// determine the margins. center the image. take the difference between the paper width and image width. halve it to get the amount of margin that works for both left and right.
	POINT marginInchPt = { (paperExts.cx - actualExts.cx)/2, (paperExts.cy - actualExts.cy)/2 };
	// convert the width and height in inches to numbers of device pixels.
	printExtents.cx = MulDiv(devRes.cx, actualExts.cx, paperExts.cx);
	printExtents.cy = MulDiv(devRes.cy, actualExts.cy, paperExts.cy);
	printMargin.x = MulDiv(devRes.cx, marginInchPt.x, paperExts.cx);
	printMargin.y = MulDiv(devRes.cy, marginInchPt.y, paperExts.cy);
	return imageRes;
}

DWORD _printImage(PRINTDLGEX *pdx, HBITMAP _hbmpImage, HeifImage *heif)
{
	HDC hdc = pdx->hDC;
	DWORD ret = ERROR_SUCCESS;
	int cap = GetDeviceCaps(hdc, RC_BITBLT); //RC_STRETCHBLT

	POINT printMargin;
	SIZE printExtents;
	int imageRes = _queryPrintDimensions(pdx, heif, printMargin, printExtents);

	// map logical (screen) coordinates in pels to printer (device) coordinates in 1/100mm.
	SetMapMode(hdc, MM_ISOTROPIC);
	// logical origin and extents in ppi
	SetWindowOrgEx(hdc, 0, 0, NULL);
	SetWindowExtEx(hdc, heif->_imageSize.cx, heif->_imageSize.cy, NULL);
	// device origin and extents in device dpi
	SetViewportOrgEx(hdc, printMargin.x, printMargin.y, NULL);
	SetViewportExtEx(hdc, printExtents.cx, printExtents.cy, NULL);

	HDC hdc2 = CreateCompatibleDC(hdc);
	HBITMAP hbmp0 = (HBITMAP)SelectObject(hdc2, _hbmpImage);
	if (!BitBlt(hdc, 0, 0, heif->_imageSize.cx, heif->_imageSize.cy, hdc2, 0, 0, SRCCOPY))
		ret = GetLastError();
	SelectObject(hdc2, hbmp0);
	DeleteDC(hdc2);
	return ret;
}


#define HEICPRINT_MAX_RANGES 1

void MainDlg::OnButtonPrint()
{
	HRESULT hr;
	PRINTDLGEX *pdx = (PRINTDLGEX*)GlobalAlloc(LPTR, sizeof(PRINTDLGEX));
	pdx->lStructSize = sizeof(PRINTDLGEX);
	pdx->hwndOwner = _hdlg;
	// we make one customization. it's for the paper orientation. if the image is wide, ask for landscape mode.
	if (_heif->_imageSize.cx > _heif->_imageSize.cy)
		pdx->hDevMode = _setupDevMode(DMORIENT_LANDSCAPE);
	pdx->hDC = NULL;
	pdx->Flags = PD_RETURNDC | PD_COLLATE | PD_CURRENTPAGE | PD_NOSELECTION;
	pdx->nMaxPageRanges = HEICPRINT_MAX_RANGES;
	pdx->lpPageRanges = (LPPRINTPAGERANGE)GlobalAlloc(LPTR, HEICPRINT_MAX_RANGES * sizeof(PRINTPAGERANGE));
	pdx->nMinPage = 1;
	pdx->nMaxPage = 1;
	pdx->nCopies = 1;
	pdx->nStartPage = START_PAGE_GENERAL;

	//  run the printer selection dialog.
	hr = PrintDlgEx(pdx);
	if ((hr == S_OK) && pdx->dwResultAction == PD_RESULT_PRINT)
	{
		SetCursor(LoadCursor(NULL, IDC_WAIT));
		// we need to obtain an HBITMAP of the full-scale image. if the current one, _heif, is not that, we will create a temp heif and run a full-scale decode.
		bool delHeif = false;
		HeifImage *heif = _heif;
		HBITMAP hbmp = _hbmpImage;
		if (!_heif->hasFullscaleData())
		{
			heif = new HeifImage;
			if (heif->init(_hdlg, _filename.c_str(), _heifFlags))
			{
				// decode the image at a full scale
				hbmp = heif->decode({ 0 });
			}
			delHeif = true;
		}
		if (hbmp)
		{
			// we now have the decoded bitmap. let's start the print job.
			DOCINFO docInfo = { sizeof(DOCINFO) };
			int docId = ::StartDoc(pdx->hDC, &docInfo);
			if (docId > 0)
			{
				if (::StartPage(pdx->hDC))
				{
					// print the image to the destination medium.
					_printImage(pdx, hbmp, heif);
					::EndPage(pdx->hDC);
				}
				::EndDoc(pdx->hDC);
			}
		}
		// if we used a temp heif decoder, delete it.
		if (delHeif)
		{
			if (hbmp)
				DeleteObject(hbmp);
			delete heif;
		}
		SetCursor(NULL);
	}
	// clean up. we're done.
	if (pdx->hDC != NULL)
		DeleteDC(pdx->hDC);
	if (pdx->lpPageRanges)
		GlobalFree((HGLOBAL)pdx->lpPageRanges);
	if (pdx->hDevMode)
		GlobalFree(pdx->hDevMode);
	if (pdx->hDevNames)
		GlobalFree(pdx->hDevNames);
	GlobalFree((HGLOBAL)pdx);
}

