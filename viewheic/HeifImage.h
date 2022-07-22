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
#include "ExifIfd.h"


class HeifImage
{
public:
	HeifImage();
	~HeifImage();

	bool init(HWND hwnd, LPCWSTR imagePath, ULONG flags);
	HBITMAP decode(SIZE viewSize);
	void term();

	// operations
	HBITMAP rotate(SIZE &newSize);
	HBITMAP zoom(int ratio, SIZE &newSize);
	HRESULT convert(LPCWSTR filename);
	bool hasFullscaleData();
	int queryDPI();

	HWND _hwnd;
	char _imagePath[_MAX_PATH];
	struct heif_context* _ctx;
	struct heif_image_handle* _imageHandle;
	SIZE _imageSize;
	int _imageCount;

	int _orientation;
	int _resX, _resY; // x- and y-resolutions (e.g., 72 dpi)
	int _resUnit; // valid values: 2=inches, 3=centimeters
	std::string _make, _model, _date, _artist;

	std::vector<ExifIfd> _ifd;

	struct DecodedImageData
	{
		LPBYTE bits;
		ULONG length;
		int stride;
		SIZE dims;
		int rotated;
	} _data;

protected:
	ULONG _ctrlFlags;

	HRESULT _saveBmp(LPCWSTR filename);
	HRESULT _saveBmpTmp(std::wstring &nameOut);
	void _clear();
	bool _parseExif(std::vector<BYTE> &metadata);
};

