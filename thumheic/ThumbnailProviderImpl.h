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
#include "HeifReader.h"


/* refer to this ms documentation on IThumbnailProvider.
https://docs.microsoft.com/en-us/windows/win32/api/thumbcache/nn-thumbcache-ithumbnailprovider
*/

class ThumbnailProviderImpl :
	public IInitializeWithStream,
	public IUnknownImpl<IThumbnailProvider, &IID_IThumbnailProvider>
{
public:
	ThumbnailProviderImpl();
	~ThumbnailProviderImpl();

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj)
	{
		if (IsEqualIID(riid, IID_IInitializeWithStream))
		{
			AddRef();
			*ppvObj = (IInitializeWithStream*)this;
			return S_OK;
		}
		return IUnknownImpl<IThumbnailProvider, &IID_IThumbnailProvider>::QueryInterface(riid, ppvObj);
	}
	STDMETHOD_(ULONG, AddRef)() { return IUnknownImpl<IThumbnailProvider, &IID_IThumbnailProvider>::AddRef(); }
	STDMETHOD_(ULONG, Release)() { return IUnknownImpl<IThumbnailProvider, &IID_IThumbnailProvider>::Release(); }
	
	// IInitializeWithStream methods
	STDMETHOD(Initialize)(/* [annotation][in] */_In_  IStream *pstream,/* [annotation][in] */_In_  DWORD grfMode);
	// IThumbnailProvider methods
	STDMETHOD(GetThumbnail)(/* [in] */ UINT cx, /* [out] */ __RPC__deref_out_opt HBITMAP *phbmp, /* [out] */ __RPC__out WTS_ALPHATYPE *pdwAlpha);

protected:
	struct heif_context* _ctx;
	HeifReader *_reader;
	bool _searchThumbnail;

	HBITMAP decodeHeif(struct heif_image_handle* image_handle, SIZE image_size);
};

