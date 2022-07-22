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
#include "HeifImage.h"
#include "CodecImage.h"
#include "StreamOnFile.h"
#include "ExifIfd.h"
#include "app.h"


enum IMAGEFILETYPE { IFT_UNK, IFT_JPEG, IFT_HEIC, IFT_PNG, IFT_BMP, IFT_MAX };



HeifImage::HeifImage() : _ctx(NULL), _hwnd(NULL), _imageHandle(NULL), _imageSize{ 0 }, _imageCount(0), _imagePath{ 0 }, _data{ 0 }, _resX(0), _resY(0), _resUnit(0)
{
}

HeifImage::~HeifImage()
{
	term();
}

void HeifImage::_clear()
{
	if (_data.bits)
		free(_data.bits);
	ZeroMemory(&_data, sizeof(_data));
}

bool HeifImage::init(HWND hwnd, LPCWSTR imagePath, ULONG flags)
{
	_hwnd = hwnd;
	_ctrlFlags = flags;
	WideCharToMultiByte(CP_ACP, 0, imagePath, -1, _imagePath, sizeof(_imagePath), NULL, NULL);

	_ctx = heif_context_alloc();

	struct heif_error err;
	err = heif_context_read_from_file(_ctx, _imagePath, nullptr);
	if (err.code != 0)
	{
		MessageBoxA(_hwnd, err.message, "heif_context_read_from_file", MB_OK);
		return false;
	}
	_imageCount = heif_context_get_number_of_top_level_images(_ctx);
	if (_imageCount == 0)
	{
		MessageBoxA(_hwnd, "File contains no image.", "heif_context_get_number_of_top_level_images", MB_OK);
		return false;
	}
	err = heif_context_get_primary_image_handle(_ctx, &_imageHandle);
	if (err.code)
	{
		MessageBoxA(_hwnd, err.message, "heif_context_get_primary_image_handle", MB_OK);
		return false;
	}
	_imageSize.cx = heif_image_handle_get_width(_imageHandle);
	_imageSize.cy = heif_image_handle_get_height(_imageHandle);

	// collect exif metadata.
	heif_item_id metadata_id;
	int count = heif_image_handle_get_list_of_metadata_block_IDs(_imageHandle, "Exif", &metadata_id, 1);
	for (int i = 0; i < count; i++) {
		size_t datasize = heif_image_handle_get_metadata_size(_imageHandle, metadata_id);
		std::vector<BYTE> nextmeta(datasize);
		heif_error error = heif_image_handle_get_metadata(_imageHandle, metadata_id, nextmeta.data());
		if (error.code != heif_error_Ok)
			continue;
		_parseExif(nextmeta);
	}
	return true;
}

void HeifImage::term()
{
	_clear();

	heif_image_handle_release(_imageHandle);
	_imageHandle = NULL;
	_imageCount = 0;
	_imageSize = { 0 };
	if (_ctx)
	{
		heif_context_free(_ctx);
		_ctx = NULL;
	}
}

HBITMAP HeifImage::decode(SIZE viewSize)
{
	if (viewSize.cx == 0 && viewSize.cy == 0)
		viewSize = _imageSize;

	struct heif_decoding_options* decode_options = heif_decoding_options_alloc();
	decode_options->convert_hdr_to_8bit = true;

	int bit_depth = 8;

	struct heif_image* image = NULL;
	struct heif_error err;
	err = heif_decode_image(_imageHandle,
		&image,
		heif_colorspace_RGB,
		heif_chroma_interleaved_RGB, // heif_chroma_interleaved_RGBA
		decode_options);
	if (err.code)
	{
		MessageBoxA(_hwnd, err.message, "heif_decode_image", MB_OK);
		return NULL;
	}
	ASSERT(image);

	SIZE destSize = _imageSize;

	if (_imageSize.cx > viewSize.cx || _imageSize.cy > viewSize.cy)
	{
		// zoom out (compact)
		if (_imageSize.cx > _imageSize.cy)
		{
			destSize.cy = MulDiv(viewSize.cx, _imageSize.cy, _imageSize.cx);
			destSize.cx = viewSize.cx;
		}
		else
		{
			destSize.cx = MulDiv(viewSize.cy, _imageSize.cx, _imageSize.cy);
			destSize.cy = viewSize.cy;
		}
		struct heif_image* scaled_image = NULL;
		err = heif_image_scale_image(image, &scaled_image, destSize.cx, destSize.cy, NULL);
		if (err.code)
		{
			heif_image_release(image);
			MessageBoxA(_hwnd, err.message, "heif_image_scale_image", MB_OK);
			return NULL;
		}
		heif_image_release(image);
		image = scaled_image;
	}
	else if (_imageSize.cx < viewSize.cx && _imageSize.cy < viewSize.cy)
	{
		// zoom in (expand)
		destSize.cy = MulDiv(viewSize.cx, _imageSize.cy, _imageSize.cx);
		destSize.cx = viewSize.cx;

		struct heif_image* scaled_image = NULL;
		err = heif_image_scale_image(image, &scaled_image, destSize.cx, destSize.cy, NULL);
		if (err.code)
		{
			heif_image_release(image);
			MessageBoxA(_hwnd, err.message, "heif_image_scale_image", MB_OK);
			return NULL;
		}
		heif_image_release(image);
		image = scaled_image;
	}

	int srcStride = 0;
	const BYTE* srcData = heif_image_get_plane_readonly(image, heif_channel_interleaved, &srcStride);
	if (!srcData)
	{
		heif_image_release(image);
		MessageBoxA(_hwnd, "Unexpected NULL pointer received", "heif_image_get_plane_readonly", MB_OK);
		return NULL;
	}
	ASSERT(srcStride / destSize.cx == sizeof(RGBTRIPLE));

	HDC hdc0 = GetDC(_hwnd);
	HDC hdc = CreateCompatibleDC(hdc0);
	BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER), destSize.cx, destSize.cy, 1, 32, BI_RGB, destSize.cx * destSize.cy * sizeof(RGBQUAD) };
	LPBYTE bits = NULL;
	HBITMAP hbmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (LPVOID*)&bits, NULL, 0);
	if (hbmp)
	{
		HBITMAP hbm0 = (HBITMAP)SelectObject(hdc, hbmp);
		int destStride = destSize.cx * sizeof(RGBQUAD);
		const BYTE* srcRow = srcData;
		BYTE* destRow = bits + (destSize.cy - 1) * destStride;
		for (int y = 0; y < destSize.cy; y++)
		{
			for (int x = 0; x < destSize.cx; x++)
			{
				RGBTRIPLE sx = *((RGBTRIPLE*)srcRow + x);
				// swap red and blue. they are transposed.
				RGBQUAD qx = { sx.rgbtRed, sx.rgbtGreen, sx.rgbtBlue, 0 };
				*((RGBQUAD*)destRow + x) = qx;
			}
			srcRow += srcStride;
			destRow -= destStride;
		}
		SetDIBits(hdc, hbmp, 0, destSize.cy, bits, &bmi, DIB_RGB_COLORS);
		SelectObject(hdc, hbm0);
	}
	DeleteDC(hdc);
	ReleaseDC(_hwnd, hdc0);

	ASSERT(_data.bits == NULL && _data.length == 0);
	_data.dims = destSize;
	_data.stride = srcStride;
	_data.length = srcStride * destSize.cy;
	_data.bits = (LPBYTE)malloc(_data.length);
	if (_data.bits)
	{
		CopyMemory(_data.bits, srcData, _data.length);
#ifdef _DEBUG
		WCHAR dbgpath[MAX_PATH];
		GetTempPath(ARRAYSIZE(dbgpath), dbgpath);
		wcscat_s(dbgpath, ARRAYSIZE(dbgpath), L"logs\\heicviewer\\HeifImageDebug.bmp");
		_saveBmp(dbgpath);
#endif//_DEBUG
	}

	heif_image_release(image);
	return hbmp;
}

int HeifImage::queryDPI()
{
	SIZE dims = _imageSize;
	int aspratio = 0;
	SIZE dpi = { 0 };
	if (dims.cx > dims.cy)
	{
		// landscape style
		aspratio = MulDiv(100, dims.cx, dims.cy);
		switch (aspratio)
		{
		case 150:
			// 6" x 4" (or 15 x 10 cm)
			dpi.cx = dims.cx / 6;
			dpi.cy = dims.cy / 4;
			ASSERT(dpi.cx == dpi.cy);
			return dpi.cx; // 600 dpi for a 3600 x 2400 pixel landscape
		case 140:
			// 7" x 5" (or 21 x 15 cm)
			dpi.cx = dims.cx / 7;
			dpi.cy = dims.cy / 5;
			ASSERT(dpi.cx == dpi.cy);
			return dpi.cx;
		case 141:
			// 8.5" x 6"
			dpi.cx = lrint((double)dims.cx / 8.5);
			dpi.cy = dims.cy / 6;
			ASSERT(dpi.cx == dpi.cy);
			return dpi.cy;
		case 135:
			// 9.5" x 7"
			dpi.cx = lrint((double)dims.cx / 9.5);
			dpi.cy = dims.cy / 7;
			ASSERT(dpi.cx == dpi.cy);
			return dpi.cy;

		case 133:
			// 24 x 18 cm
			dpi.cx = dims.cx / 24;
			dpi.cy = dims.cy / 18;
			ASSERT(dpi.cx == dpi.cy);
			return MulDiv(254,dpi.cx,100); // 168 dots-per-cm for a 4032 x 3024 pixel landscape
		case 138:
			// 18 x 13 cm
			dpi.cx = dims.cx / 18;
			dpi.cy = dims.cy / 13;
			ASSERT(dpi.cx == dpi.cy);
			return MulDiv(254, dpi.cx, 100);
		}
	}
	if (dims.cy > dims.cx)
	{
		// portrait style. swap cx and cy.
		aspratio = MulDiv(100, dims.cy, dims.cx);
		switch (aspratio)
		{
		case 150:
			// 4" x 6" (or 10 x 15 cm)
			dpi.cx = dims.cx / 4;
			dpi.cy = dims.cy / 6;
			ASSERT(dpi.cx == dpi.cy);
			return dpi.cx; // 600 dpi for a 2400 x 3600 pixel landscape
		case 140:
			// 5" x 7" (or 15 x 21 cm)
			dpi.cx = dims.cx / 5;
			dpi.cy = dims.cy / 7;
			ASSERT(dpi.cx == dpi.cy);
			return dpi.cx;
		case 141:
			// 6" x 8.5"
			dpi.cy = lrint((double)dims.cy / 8.5);
			dpi.cx = dims.cx / 6;
			ASSERT(dpi.cx == dpi.cy);
			return dpi.cx;
		case 135:
			// 7" x 9.5"
			dpi.cy = lrint((double)dims.cy / 9.5);
			dpi.cx = dims.cx / 7;
			ASSERT(dpi.cx == dpi.cy);
			return dpi.cx;

		case 133:
			// 18 x 24 cm
			dpi.cx = dims.cx / 18;
			dpi.cy = dims.cy / 24;
			ASSERT(dpi.cx == dpi.cy);
			return MulDiv(254, dpi.cx, 100); // 168 dots-per-cm for a 3024 x 4032 pixel landscape
		case 138:
			// 13 x 18 cm
			dpi.cx = dims.cx / 13;
			dpi.cy = dims.cy / 18;
			ASSERT(dpi.cx == dpi.cy);
			return MulDiv(254, dpi.cx, 100);
		}
	}
	/*
	int candidates[] = { 96, 150, 300,600,1200,2400,3600 };
	for (int i = 0; i < ARRAYSIZE(candidates); i++)
	{
		float rx = dims.cx / candidates[i];
		float ry = dims.cy / candidates[i];

	}
	*/
	if (_resX)
	{
		if (_resUnit == 2) // dpi?
			return _resX;
		if (_resUnit == 3) // dpc?
			return MulDiv(254, _resX, 100);
	}
	return 300;
}

bool HeifImage::hasFullscaleData()
{
	if (_data.bits)
	{
		if (_data.dims.cx == _imageSize.cx && _data.dims.cy == _imageSize.cy)
			return true;
	}
	return false;
}

HBITMAP HeifImage::zoom(int ratio, SIZE &newSize)
{
	_clear();

	newSize = _imageSize;
	if (ratio != 0 && ratio != 100)
	{
		newSize.cx = MulDiv(ratio, _imageSize.cx, 100);
		newSize.cy = MulDiv(ratio, _imageSize.cy, 100);
	}
	return decode(newSize);
}

void _rotateImage(HeifImage::DecodedImageData &src, RGBQUAD *dest)
{
	int x1, y1, x2, y2, cx2, cy2;

	if (++src.rotated & 1)
	{
		cx2 = src.dims.cy;
		cy2 = src.dims.cx;
	}
	else
	{
		cx2 = src.dims.cx;
		cy2 = src.dims.cy;
	}
	
	for (y1 = 0; y1 < src.dims.cy; y1++)
	{
		for (x1 = 0; x1 < src.dims.cx; x1++)
		{
			switch (src.rotated % 4)
			{
			case 1: // 90 degrees rotated clockwise
				x2 = cx2 - y1 - 1;
				y2 = x1;
				break;
			case 2: // 180 degrees rotated
				x2 = cx2 - x1 - 1;
				y2 = cy2 - y1 - 1;
				break;
			case 3: // 270 degrees rotated
				x2 = y1;
				y2 = cy2 - x1 - 1;
				break;
			default: // 360 degrees rotated, back to original position
				x2 = x1;
				y2 = y1;
				break;
			}
			int i2 = (cy2-y2-1) * cx2 + x2;
			LPBYTE srcRow = src.bits + y1 * src.stride;
			RGBTRIPLE sx = *((RGBTRIPLE*)srcRow + x1);
			RGBQUAD qx = { sx.rgbtRed,sx.rgbtGreen,sx.rgbtBlue, 0 };
			dest[i2] = qx;
		}
	}
}

/* rotate reuses the color data the decode method has generated. Each time rotate is called, it turns the color data clockwise by 90 degrees. So, after being rotated 4 times, the image has made a complete rotation, and is back to the original position. since rotate does not call the expensive heif_decode_image, it is quite fast.
*/
HBITMAP HeifImage::rotate(SIZE &newSize)
{
	if (!_data.bits)
		return false; // make sure decode is called before you use rotate.

	// swap the image width and height if this is an even-numbered call. if this is the first rotation, the original top and bottom become right and left sides.
	SIZE destSize = _data.dims;
	if (!(_data.rotated & 1))
		std::swap(destSize.cx, destSize.cy);
	// create a DIB based on the rotated image resolutions.
	HDC hdc0 = GetDC(_hwnd);
	HDC hdc = CreateCompatibleDC(hdc0);
	BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER), destSize.cx, destSize.cy, 1, 32, BI_RGB, destSize.cx * destSize.cy * sizeof(RGBQUAD) };
	LPBYTE bits = NULL;
	HBITMAP hbmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (LPVOID*)&bits, NULL, 0);
	if (hbmp)
	{
		// turn the image clockwise by 90 degrees. move the rotated color data into bits.
		_rotateImage(_data, (RGBQUAD*)bits);
		// select the new bitmap into the screen context.
		HBITMAP hbm0 = (HBITMAP)SelectObject(hdc, hbmp);
		// show the rotated color data by binding it to the new bitmap handle.
		SetDIBits(hdc, hbmp, 0, destSize.cy, bits, &bmi, DIB_RGB_COLORS);
		SelectObject(hdc, hbm0);
	}
	// done. clean up.
	DeleteDC(hdc);
	ReleaseDC(_hwnd, hdc0);
	// pass the rotated image size to the caller.
	newSize = destSize;
	return hbmp;
}

#define SCANLINELENGTH_24BPP(bmWidth) (4 * ((sizeof(RGBTRIPLE)*bmWidth + 3) / 4))

HRESULT HeifImage::_saveBmpTmp(std::wstring &nameOut)
{
	WCHAR srcdir[MAX_PATH];
	GetTempPath(ARRAYSIZE(srcdir), srcdir);
	WCHAR srcname[MAX_PATH];
	GetTempFileName(srcdir, L"HCV", 0, srcname);
	HRESULT hr = _saveBmp(srcname);
	if (hr == S_OK)
		nameOut.assign(srcname);
	return hr;
}

HRESULT HeifImage::_saveBmp(LPCWSTR filename)
{
	if (!_data.bits)
		return E_UNEXPECTED;
	// a bmp requires a stride length of a multiple of 4 bytes.
	// it may not equal the hief stride which is in _data.stride.
	int bmpStride = SCANLINELENGTH_24BPP(_data.dims.cx);

	BITMAPINFO bi = { sizeof(BITMAPINFOHEADER) };
	BITMAPINFOHEADER &bih = bi.bmiHeader;
	bih.biWidth = _data.dims.cx;
	bih.biHeight = _data.dims.cy;
	bih.biBitCount = 24;
	bih.biPlanes = 1;
	bih.biSizeImage = _data.dims.cy * bmpStride;

	BITMAPFILEHEADER bfh = { 0x4d42 };
	bfh.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(LPBITMAPINFOHEADER);
	bfh.bfSize = bfh.bfOffBits + bih.biSizeImage;

	HRESULT hr = S_OK;
	HANDLE hf = CreateFile(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hf == INVALID_HANDLE_VALUE)
		return HRESULT_FROM_WIN32(GetLastError());

	ULONG len;
	if (WriteFile(hf, &bfh, sizeof(bfh), &len, NULL) &&
		WriteFile(hf, &bih, sizeof(bih), &len, NULL))
	{
		LPBYTE bmpScanline = (LPBYTE)malloc(bmpStride);
		LPBYTE p = _data.bits + _data.length - _data.stride;
		for (int y = 0; y < _data.dims.cy; y++)
		{
			ZeroMemory(bmpScanline, bmpStride);
			for (int x = 0; x < _data.dims.cx; x++)
			{
				RGBTRIPLE sx = *((RGBTRIPLE*)p + x);
				RGBTRIPLE sx2 = { sx.rgbtRed,sx.rgbtGreen,sx.rgbtBlue };
				*((RGBTRIPLE*)bmpScanline + x) = sx2;
			}
			WriteFile(hf, bmpScanline, bmpStride, &len, NULL);
			if (len != bmpStride)
			{
				hr = E_FAIL;
				break;
			}
			p -= _data.stride;
		}
		free(bmpScanline);
	}
	else
		hr = HRESULT_FROM_WIN32(GetLastError());
	CloseHandle(hf);

	return hr;
}

// a helper function for converting a bmp image file to a jpeg or png file.
template<class T>
HRESULT _convertBmp(LPCWSTR srcName, LPCWSTR destName, std::vector<ExifIfd> *ifd=NULL)
{
	// MS WIC requires an IStream interface to access the image data. so, use a helper class to wrap the source bitmap file and expose the required interface.
	StreamOnFile *sof1 = new StreamOnFile;
	HRESULT hr = sof1->Open(srcName);
	if (hr == S_OK)
	{
		// load the source bitmap in a decoder.
		CodecBMP bmp;
		hr = bmp.Load(sof1);
		if (hr == S_OK)
		{
			// create an encoder for the destination image format.
			T encoder(ifd); // CodecJPEG or CodecPNG
			// attach the decoded source bitmap.
			encoder._wbsrc = bmp._wbsrc;
			bmp._wbsrc = NULL;
			// need an IStream on the destination image file.
			StreamOnFile *sof2 = new StreamOnFile;
			hr = sof2->Create(destName);
			if (hr == S_OK)
			{
				// encode the image and write it to the destination file.
				hr = encoder.Save(sof2);
				if (hr == S_OK)
					hr = sof2->Commit(0);
			}
			sof2->Release();
		}
	}
	sof1->Release();
	return hr;
}

IMAGEFILETYPE _getFileType(LPCWSTR filename)
{
	LPCWSTR extension = wcsrchr(filename, '.');
	if (extension)
	{
		if (0 == _wcsicmp(extension, L".jpg") || 0 == _wcsicmp(extension, L".jpeg"))
			return IFT_JPEG;
		if (0 == _wcsicmp(extension, L".heic"))
			return IFT_HEIC;
		if (0 == _wcsicmp(extension, L".png"))
			return IFT_PNG;
		if (0 == _wcsicmp(extension, L".bmp"))
			return IFT_BMP;
	}
	return IFT_UNK;
}

// the method converts the heic image to bmp, png, or jpeg.
HRESULT HeifImage::convert(LPCWSTR filename)
{
	auto typeId = _getFileType(filename);
	// bmp needs no preprocessing. just write the _data to the destination file.
	if (typeId == IFT_BMP)
		return _saveBmp(filename);
	HRESULT hr = E_NOTIMPL;
	// jpeg and png uses ms codec support to encode _data.
	if (typeId == IFT_JPEG || typeId == IFT_PNG)
	{
		// first, save the source bitmap image in a temp file.
		std::wstring srcname;
		hr = _saveBmpTmp(srcname);
		if (FAILED(hr))
			return hr;
		// jpeg supports exif metadata. so, if metadata is available, copy it to the destination.
		if (typeId == IFT_JPEG)
			hr = _convertBmp<CodecJPEG>(srcname.c_str(), filename, (_ctrlFlags & VIEWHEIC_SAVE_NO_EXIF)? NULL : &_ifd);
		else if (typeId == IFT_PNG)
			hr = _convertBmp<CodecPNG>(srcname.c_str(), filename);
		DeleteFile(srcname.c_str());
	}
	return hr;
}


struct APP1_EXIF {
	BYTE Signature[4]; // 'Exif'
	BYTE Reserved[2];
	EXIF_TIFFHEADER TIFFHeader;
	EXIF_IFD IFD0;
};

bool HeifImage::_parseExif(std::vector<BYTE> &metadata)
{
	// skip the first 4 bytes to get to the exif/tiff header.
	LPBYTE p0 = metadata.data()+4;
	int len0 = (int)metadata.size()-4;

	// make sure the header is valid.
	APP1_EXIF *exif = (APP1_EXIF*)p0;
	if (0 != memcmp(exif->Signature, "Exif", 4))
		return false;
	if (exif->TIFFHeader.ByteOrder != 0x4949 && exif->TIFFHeader.ByteOrder != 0x4d4d)
		return false;
	// parse the primary image file directory (IFD0).
	ExifIfd ifd0(EXIFIFDCLASS_PRIMARY, (LPBYTE)&exif->TIFFHeader, len0 - FIELD_OFFSET(APP1_EXIF, TIFFHeader), exif->TIFFHeader.ByteOrder == 0x4D4D);
	int len = ifd0.parse(ifd0.getULONG(&exif->TIFFHeader.IFDOffset));
	if (len == 0)
		return false;
	_ifd.push_back(ifd0);
	// the offset to another IFD (called IFD1) immediately follows IFD0. it contains a thumbnail image.
	ULONG offsetIfd1 = ifd0.getULONG(ifd0._basePtr+ifd0._offset+len);
	// IFD0 includes an offset to a sub-IFD called 'Exif IFD'. it holds image configuration info.
	const ExifIfdAttribute *offsetToIfd = ifd0.find(ExifIfdAttribute::OffsetToExifIFD);
	if (offsetToIfd)
	{
		ULONG offsetExifIfd = ifd0.getULONG((LPVOID)offsetToIfd->_value.data());
		ExifIfd ifdExif(EXIFIFDCLASS_EXIF, ifd0._basePtr, ifd0._baseLen, ifd0._big);
		len = ifdExif.parse(offsetExifIfd);
		_ifd.push_back(ifdExif);
	}
	// IFD0 may also include an offset to a GPS IFD.
	offsetToIfd = ifd0.find(ExifIfdAttribute::OffsetToGpsIFD);
	if (offsetToIfd)
	{
		ULONG offsetGpsIfd = ifd0.getULONG((LPVOID)offsetToIfd->_value.data());
		ExifIfd ifdGps(EXIFIFDCLASS_GPS, ifd0._basePtr, ifd0._baseLen, ifd0._big);
		len = ifdGps.parse(offsetGpsIfd);
		_ifd.push_back(ifdGps);
	}
	// parse the thumbnail IFD if it exists.
	if (offsetIfd1)
	{
		ExifIfd ifd1(EXIFIFDCLASS_THUMBNAIL, ifd0._basePtr, ifd0._baseLen, ifd0._big);
		len = ifd1.parse(offsetIfd1);
		_ifd.push_back(ifd1);
#ifdef _DEBUG
		// this ifd is followed by thumbnail image data. it's a jpeg too.
		ULONG _thumbnailOffset = offsetIfd1 + ifd1._totalLen;
		int _thumbnailLen = ifd0._baseLen - (int)_thumbnailOffset;
		if (_thumbnailLen > 0)
		{
			WriteToFile(ifd1._basePtr + _thumbnailOffset, _thumbnailLen, L"%TMP%\\logs\\heifthumb.jpg");
		}
#endif//#ifdef _DEBUG
	}

	// keep more important exif attributes so that the caller can gain access without bothering to enumerate the ifds.
	_orientation = ifd0.queryAttributeValueInt(ExifIfdAttribute::Orientation);
	_resX = ifd0.queryAttributeValueInt(ExifIfdAttribute::XResolution);
	_resY = ifd0.queryAttributeValueInt(ExifIfdAttribute::YResolution);
	_resUnit = ifd0.queryAttributeValueInt(ExifIfdAttribute::ResolutionUnit);
	_make = ifd0.queryAttributeValueStr(ExifIfdAttribute::Make);
	_model = ifd0.queryAttributeValueStr(ExifIfdAttribute::Model);
	_date = ifd0.queryAttributeValueStr(ExifIfdAttribute::DateTime);
	_artist = ifd0.queryAttributeValueStr(ExifIfdAttribute::Artist);
	return true;
}

///////////////////////////////////////////////////////
// refer to the WIC metadata documentation at
// https://docs.microsoft.com/en-us/windows/win32/wic/-wic-metadata-portal

HRESULT CodecJPEG::_initFrame(IWICBitmapFrameEncode *frame)
{
	HRESULT hr = CodecImage::_initFrame(frame);
	if (hr == S_OK && _metadata)
	{
		// if exif metadata is available in the heic, copy it to the destination jpeg.
		// first, get a wic metadata writer.
		IWICMetadataQueryWriter *qwriter = NULL;
		hr = frame->GetMetadataQueryWriter(&qwriter);
		if (hr == S_OK)
		{
			std::vector<ExifIfd> *v = (std::vector<ExifIfd> *)_metadata;
			WCHAR path[80];
			// make sure the format strings are ordered according to IFDCLASS.
			LPCWSTR pathfmt[] =
			{
				L"/app1/ifd/{ushort=%d}",
				L"/app1/ifd/exif/{ushort=%d}",
				L"/app1/ifd/gps/{ushort=%d}",
				L"/app1/ifd/thumb/{ushort=%d}"
			};
			// copy all attributes of each of the image file directories to the jpeg as records of an APP1 resource.
			for (size_t i = 0; i < v->size(); i++)
			{
				ExifIfd &ifd = v->at(i);
				for (size_t j = 0; j < ifd.size(); j++)
				{
					ExifIfdAttribute &attrib = ifd.at(j); // don't use ifd[j]. attrib's _coll needs to be corrected.
					// build an app1 path for the next ifd attribute.
					swprintf_s(path, ARRAYSIZE(path), pathfmt[ifd._class], attrib._ifd.Tag);
					// get the attribute's value as a PropVariant.
					PropVariant pv;
					hr = pv.assignIfdValue(attrib);
					if (FAILED(hr))
						break;
					// write out the attribute in the jpeg.
					hr = qwriter->SetMetadataByName(path, pv);
					if (FAILED(hr))
					{
						if (hr == E_INVALIDARG)
						{
							hr = S_OK;
							continue; // ignore it. we have to skip this attribute though...
						}
						break;
					}
				}
			}
			qwriter->Release();
		}
	}
	return hr;
}

