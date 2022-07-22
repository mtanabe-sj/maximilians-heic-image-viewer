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
#include "ThumbnailProviderImpl.h"


ThumbnailProviderImpl::ThumbnailProviderImpl() : _ctx(NULL), _reader(NULL), _searchThumbnail(true)
{
	DBGPUTS((L"ThumbnailProviderImpl created"));
}

ThumbnailProviderImpl::~ThumbnailProviderImpl()
{
	if (_reader)
		delete _reader;
	if (_ctx)
		heif_context_free(_ctx);
	DBGPUTS((L"ThumbnailProviderImpl deleted"));
}

STDMETHODIMP ThumbnailProviderImpl::Initialize(/* [annotation][in] */_In_  IStream *pstream,/* [annotation][in] */_In_  DWORD grfMode)
{
	if (!pstream)
		return E_POINTER;
	if (_reader)
		return E_UNEXPECTED;
	HRESULT hr = S_OK;
	_ctx = heif_context_alloc();
	if (_ctx)
		_reader = new HeifReader(pstream);
	else
		hr = E_UNEXPECTED;
	DBGPRINTF((L"ThumbnailProviderImpl::Initialize: HR=%X", hr));
	return hr;
}

STDMETHODIMP ThumbnailProviderImpl::GetThumbnail(/* [in] */ UINT cx, /* [out] */ __RPC__deref_out_opt HBITMAP *phbmp, /* [out] */ __RPC__out WTS_ALPHATYPE *pdwAlpha)
{
	DBGPRINTF((L"ThumbnailProviderImpl::GetThumbnail started: CX=%d", cx));
	SIZE szImage = { (int)cx, (int)cx };

	struct heif_error err;
	err = heif_context_read_from_reader(_ctx, &_reader->_interface, _reader, NULL);
	DBGPRINTF((L"heif_context_read_from_reader: code %d (subcode %d); '%S'", err.code, err.subcode, err.message));
	if (err.code != 0)
	{
		return false;
	}
	int num_images = heif_context_get_number_of_top_level_images(_ctx);
	DBGPRINTF((L"heif_context_get_number_of_top_level_images: num_images=%d", num_images));
	if (num_images == 0)
	{
		return false;
	}
	struct heif_image_handle* image_handle = NULL;
	err = heif_context_get_primary_image_handle(_ctx, &image_handle);
	DBGPRINTF((L"heif_context_get_primary_image_handle: code %d (subcode %d); '%S'", err.code, err.subcode, err.message));
	if (err.code)
	{
		return false;
	}

	bool thumbnailFound = false;
	HBITMAP hbmp = NULL;
	if (_searchThumbnail)
	{
		heif_item_id thumbnail_ID;
		int nThumbnails = heif_image_handle_get_list_of_thumbnail_IDs(image_handle, &thumbnail_ID, 1);
		DBGPRINTF((L"heif_image_handle_get_list_of_thumbnail_IDs: num_thumbnails=%d", nThumbnails));
		if (nThumbnails > 0)
		{
			struct heif_image_handle* thumbnail_handle;
			err = heif_image_handle_get_thumbnail(image_handle, thumbnail_ID, &thumbnail_handle);
			DBGPRINTF((L"heif_image_handle_get_thumbnail: code %d (subcode %d); '%S'", err.code, err.subcode, err.message));
			if (err.code)
			{
				heif_image_handle_release(image_handle);
				return false;
			}
			hbmp = decodeHeif(thumbnail_handle, szImage);
			heif_image_handle_release(thumbnail_handle);
			thumbnailFound = true;
		}
	}
	if (!hbmp)
	{
		hbmp = decodeHeif(image_handle, szImage);
	}
	heif_image_handle_release(image_handle);
	if (hbmp)
	{
		*phbmp = hbmp;
		*pdwAlpha = WTSAT_RGB;
		DBGPRINTF((L"ThumbnailProviderImpl::GetThumbnail stopped: searchThumbnail=%d, thumbnailFound=%d", _searchThumbnail, thumbnailFound));
		return S_OK;
	}
	*phbmp = NULL;
	DBGPRINTF((L"ThumbnailProviderImpl::GetThumbnail failed: searchThumbnail=%d, thumbnailFound=%d", _searchThumbnail, thumbnailFound));
	return E_FAIL;
}

HBITMAP ThumbnailProviderImpl::decodeHeif(struct heif_image_handle* image_handle, SIZE image_size)
{
	DBGPRINTF((L"decodeHeif started: cx=%d, cy=%d", image_size.cx, image_size.cy));
	struct heif_decoding_options* decode_options = heif_decoding_options_alloc();
	decode_options->convert_hdr_to_8bit = true;

	int bit_depth = 8;

	struct heif_image* image = NULL;
	struct heif_error err;
	// use heif_chroma_interleaved_RGBA if alpha channel is enabled for transparency
	err = heif_decode_image(image_handle, &image, heif_colorspace_RGB, heif_chroma_interleaved_RGB, decode_options);
	DBGPRINTF((L"heif_decode_image: code %d (subcode %d); '%S'", err.code, err.subcode, err.message));
	if (err.code)
	{
		return NULL;
	}
	ASSERT(image);
	int input_width = heif_image_handle_get_width(image_handle);
	int input_height = heif_image_handle_get_height(image_handle);

	int thumbnail_width = input_width;
	int thumbnail_height = input_height;
	if (input_width > image_size.cx || input_height > image_size.cy)
	{
		if (input_width > input_height)
		{
			thumbnail_height = input_height * image_size.cx / input_width;
			thumbnail_width = image_size.cx;
		}
		else if (input_height > 0)
		{
			thumbnail_width = input_width * image_size.cy / input_height;
			thumbnail_height = image_size.cy;
		}
		else
		{
			thumbnail_width = thumbnail_height = 0;
		}
		struct heif_image* scaled_image = NULL;
		err = heif_image_scale_image(image, &scaled_image, thumbnail_width, thumbnail_height, NULL);
		DBGPRINTF((L"heif_image_scale_image: code %d (subcode %d); '%S'", err.code, err.subcode, err.message));
		if (err.code)
		{
			heif_image_release(image);
			return NULL;
		}
		heif_image_release(image);
		image = scaled_image;
	}

	int src_stride = 0;
	const BYTE* src_rgb = heif_image_get_plane_readonly(image, heif_channel_interleaved, &src_stride);
	if (!src_rgb)
	{
		heif_image_release(image);
		DBGPUTS((L"heif_image_get_plane_readonly: nullptr"));
		return NULL;
	}
	ASSERT(src_stride / thumbnail_width == sizeof(RGBTRIPLE));

	HDC hdc0 = CreateDC(L"DISPLAY", NULL, NULL, NULL);
	HDC hdc = CreateCompatibleDC(hdc0);
	BITMAPINFO bmi = { sizeof(BITMAPINFOHEADER), thumbnail_width, thumbnail_height, 1, 32, BI_RGB, thumbnail_width * thumbnail_height * sizeof(RGBQUAD) };
	LPBYTE bits = NULL;
	HBITMAP hbmp = CreateDIBSection(hdc, &bmi, DIB_RGB_COLORS, (LPVOID*)&bits, NULL, 0);
	if (hbmp)
	{
		HBITMAP hbm0 = (HBITMAP)SelectObject(hdc, hbmp);
		int dest_stride = thumbnail_width * sizeof(RGBQUAD);
		const BYTE* src_row = src_rgb;
		BYTE* dest_row = bits + (thumbnail_height - 1) * dest_stride;
		for (int y = 0; y < thumbnail_height; ++y)
		{
			for (int x = 0; x < thumbnail_width; ++x)
			{
				RGBTRIPLE sx = *((RGBTRIPLE*)src_row + x);
				// swap red and blue. they are transposed.
				RGBQUAD rgb = { sx.rgbtRed, sx.rgbtGreen, sx.rgbtBlue, 0 };
				*((RGBQUAD*)dest_row + x) = rgb;
			}
			src_row += src_stride;
			dest_row -= dest_stride;
		}
		SetDIBits(hdc, hbmp, 0, thumbnail_height, bits, &bmi, DIB_RGB_COLORS);
		SelectObject(hdc, hbm0);
	}
	DeleteDC(hdc);
	DeleteDC(hdc0);

	heif_image_release(image);
	DBGPRINTF((L"decodeHeif stopped: hbmp=%p", hbmp));
	return hbmp;
}
