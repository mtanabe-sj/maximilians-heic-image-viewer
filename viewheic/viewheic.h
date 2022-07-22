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
#include "SimpleDlg.h"
#include "resource.h"
#include "HeifImage.h"
#include "ToolsDlg.h"


// flags for VIEWINFO.scrollH and scrollV
#define VI_SCROLLER_VISIBLE 0x0001
#define VI_SCROLLER_SHADED 0x0002
#define VI_SCROLLER_CAPTURED 0x0004

struct VIEWINFO
{
	POINT ptCapture;
	POINT ptPage;
	SIZE szPage;
	SIZE szImage;
	SIZE szAuto;
	SIZE szScreen;
	int zoomRatio;
	int zoomAuto;
	USHORT scrollH;
	USHORT scrollV;
	RECT rectH;
	RECT rectV;
	RECT rectCapture;

	VIEWINFO() : ptCapture{ 0 }, ptPage{ 0 }, szPage{ 0 }, szImage{ 0 }, szAuto{ 0 }, szScreen{ 0 }, rectH{ 0 }, rectV{ 0 }, zoomRatio(0), zoomAuto(0), scrollH(0), scrollV(0)
	{
		szScreen.cx = GetSystemMetrics(SM_CXFULLSCREEN);
		szScreen.cy = GetSystemMetrics(SM_CYFULLSCREEN);
	}

	void onMouseMove(HWND hwnd, UINT state, POINT pt);
	bool onMouseWheel(short nButtonType, short nWheelDelta, POINT pt);
	bool startScroll(HWND hwnd, UINT buttonType, POINT pt);
	bool stopScroll(HWND hwnd, UINT buttonType, POINT pt);

	void drawScrollerH(HDC hdc, BYTE transparency);
	void drawScrollerV(HDC hdc, BYTE transparency);
	BOOL drawScrollers(HDC hdc, BYTE transparency);
	BOOL blendRect(HDC hdc, int x, int y, int cx, int cy, BYTE transparency);
};


class MainDlg : public SimpleDlg
{
public:
	MainDlg(const std::wstring &path) : SimpleDlg(IDD_MAIN), _filename(path), _tools(NULL), _heif(NULL), _hbmpImage(NULL), _heifFlags(0) {}

protected:
	VIEWINFO _vi;
	std::wstring _filename;
	HBITMAP _hbmpImage;
	HeifImage *_heif;
	ULONG _heifFlags;
	ToolsDlg *_tools;
	SIZE _frameMargins;

	virtual BOOL OnInitDialog();
	virtual void OnDestroy();
	virtual void OnMove(int x, int y);
	virtual void OnSize(UINT nType, int cx, int cy);
	virtual BOOL OnLButtonDown(INT_PTR nButtonType, POINT pt);
	virtual BOOL OnLButtonUp(INT_PTR nButtonType, POINT pt);
	virtual BOOL OnMouseMove(UINT state, POINT pt);
	virtual BOOL OnMouseWheel(short nButtonType, short nWheelDelta, POINT pt);
	virtual BOOL OnSysCommand(WPARAM wp_, LPARAM lp_);
	virtual BOOL OnCommand(WPARAM wp_, LPARAM lp_);
	virtual BOOL OnPaint();
	virtual BOOL OnEraseBkgnd(HDC hdc);

	void OnButtonOpen();
	void OnButtonSaveAs();
	void OnButtonPrint();
	void OnButtonRotate();
	void OnButtonZoom();

	void _sizeWindow(SIZE imageSize);
	void _autoZoom(bool syncParams=false);
	void _setCaption(ULONG flags=0);
	int _queryFileTitle(LPWSTR buf, int bufSize);
};

