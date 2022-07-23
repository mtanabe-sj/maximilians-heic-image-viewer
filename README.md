# Maximilian's HEIC Image Viewer

Maximilian's HEIC image viewer is a Windows Explorer add-on written in C++. It lets Windows users preview, display and print .heic image files. The viewer uses the GNU `libheic` library to achieve image decompression.

On Windows 10 and earlier, users may not be able to preview image files with extension .heic due to a missing codec in `WIC`. Users could purchase the codec from Windows Store for a small license fee. Or, they could install this freeware.

What is `HEIC`? It stands for High Efficiency Image Container. It's the MIME type name of images encoded in the HEVC coding format and packaged in the HEIF file format. `HEVC` stands for High Efficiency Video Coing. `HEIF` stands for High Efficiency Image File. Photographs taken with Apple's iPhones save as HEIC files. Nokia, a major HEIF promotor, publishes reference materials [here](http://nokiatech.github.io/heif/technical.html). HEIF seeks to achieve a high degree of data packing without losing fidelity. The idea seems rather contradictory, but there you have it. We are not concerned with how HEIF works. We are however interested in how to decode it, and get a bitmap in a familiar data format, like the RBG-based Win32 DIB. Set in a DIB form, the image can be processed using a well-known method of bitmap manipulation. So, we use `libheif` to decode `HEIC`, and use `GDI` and `WIC` to manipulate, and convert, and render `HEIC`.


## Features

The product consists of three Visual Studio projects plus a tool set for building the msi.

1) `viewheic`, a .heic file type handler,
2) `thumheic`, a .heic thumbnail image provider, and
3) `setup`, a self-extracting msi-based installer.

Visual Studio project `viewheic` builds the image viewer. The viewer application, basically a Win32 dialog handler, implements a Windows file type handler for image type HEIC, and is registered with the system as such. The system runs `viewheic` whenever a .heic image file is opened by a user in Windows Explorer. The viewer allows users to zoom, rotate, print and export HEIC images.

The thumbnail provider is a Shell Extension handler implementing the `IThumbnailProvider` interface of the Shell. It is an in-proc COM server registered with the Shell subsystem. So, the library's DllRegisterServer performs both standard COM and Shell-specific registrations. If the source image comes with a thumbnail image itself, the provider tries to use that. Otherwise, it scales the source image to fit the requested size.

Both `viewheic` and `thumheic` depend on [libheif](https://github.com/strukturag/libheif) for image decompression.

Setup runs a test on the WIC of the system for availability of a HEIF codec. It stops the installation if the test result is positive, meaning there is no need for an external codec. Setup comes with both a x64 and x86 msi installers. It starts an appropriate msi appropriate for the system it is run.

The msi tool set consists of a template msi, a build staging batch process, and a script for generating msi tables and packaging the product files in the msi. Refer to [build.md](https://github.com/mtanabe/maximilians-heic-image-viewer/blob/main/installer/build.md) for info on how to build the product.



## Getting Started

Prerequisites:

* Windows 7, 8, or 10.
* Visual Studio 2017 or 2019. The VS solution and project files were generated using VS 2017. They can be imported by VS 2019.
* Windows SDK 10.0.17763.0. More recent SDKs should work, too, although no verification has been made.

The installer is available from [here](https://github.com/mtanabe-sj/maximilians-heic-image-viewer/blob/main/installer/out/heicviewer_setup.exe). It installs `libheic` (version 1.12.0) and associated dll dependecies as well. Installation requires admin priviledges as it performs a per-machine install.

To build the installer, install [`Maximilian's Automation Utility`](https://github.com/mtanabe-sj/maximilians-automation-utility/tree/main/installer/out). The build process requires Utility's `MaxsUtilLib.VersionInfo` automation object. 


## Introduction to the Design and Implementation

### VIEWHEIC Viewer Application

`VIEWHEIC` uses a class named HeifImage to wrap `libheic`. If you are interested in how libheic is used, take a look at the init(), decode() and term() methods.

```C++
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

protected:
	void _clear();
};
```

The image is decoded through a string of calls made by HeifImage to LIBHEIF. The init() method performs these operations.

1)	Create a heif_context by calling heif_context_alloc. The context is kept alive until the HeifImage instance is destroyed.
2)	Load the image into the heif context by calling heif_context_read_from_file on the image path passed from the image window.
3)	Obtain a primary image handle from heif_context_get_primary_image_handle.
4)	Query the image dimensions by calling heif_image_handle_get_width and heif_image_handle_get_height.

The decode() method does the most processor intensive work of HEVC decoding. Actually, LIBHEIC uses the decoder library, LIBDE256, to achieve HEVC decoding.

1)	Decode the image by calling heif_decode_image on the image handle. RGB colorspace is selected. This call may not return immediately if the image resolution is high.
2)	Scale the image by calling heif_image_scale_image if the image is too large to fit the screen.
3)	Generate a GDI DI bitmap image by obtaining the RGB color data from heif_image_get_plane_readonly, and copying the source color data to a destination RGB DIB from Win32 CreateDIBSection.
4)	Return the HBITMAP handle from step 7 to the image window which in turn renders the bitmap image using the handle.

The HeifImage destructor calls term() internally to free used resources.

1)	Delete the image handle by calling heif_image_handle_release on the handle.
2)	Free the operating context by calling heif_context_free.


See MainDlg::OnCommand() on how the viewer responds to the zoom, rotate, print and save-as commands. The method relies on HeifImage to access the source image, and uses Win32 GDI to achieve image scaling and rotation. The save-as feature uses WIC's codecs to convert HEIC to BMP, JPEG or PNG. It supports Exif. If the source .heic has Exif metadata, the destination in JPEG retains the Exif.


### THUMHEIC Thumbnail Provider

The thumbnail provider is more interesting in terms of component architecture, as it must deal with Windows Explorer as well as `libheic`. The components of the provider are shown below. The arrows indicate main flows of instructions and image data. The provider and the bound libheif and other libraries reside in a DLLHOST process, a child process created by Explorer on selection of a .heic image file by a user. Our provider and Explorer use Win32 GDI to manage the decoded DIB bitmap and other graphical resources. Explorer passes an IStream on the .heic file to the thumbnail server via the IInitializeWithStream interface. The thumbnail provider uses the HeifReader class to read the .heic image data through the IStream interface. LIBHEIF decodes the image data from the heif_reader interface published by HeifReader of the thumbnail provider. The decoded data is passed to the thumbnail provider which in turn generates a DI bitmap based on the color data by calling GDI CreateDIBSection. A handle to the generated bitmap is passed back to Explorer which caches the bitmap and completes thumbnail display for the .heic file.

![alt Thumbnail provider components](https://github.com/mtanabe-sj/maximilians-heic-image-viewer/blob/main/doc/thumbnail%20provider%20design.png)

The provider implements IThumbnailProvider in the class,

```C++
class ThumbnailProviderImpl :
	public IInitializeWithStream,
	public IUnknownImpl<IThumbnailProvider, &IID_IThumbnailProvider>
{
public:
	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObj);
	STDMETHOD_(ULONG, AddRef)() { return IUnknownImpl<IThumbnailProvider, &IID_IThumbnailProvider>::AddRef(); }
	STDMETHOD_(ULONG, Release)() { return IUnknownImpl<IThumbnailProvider, &IID_IThumbnailProvider>::Release(); }
	
	// IInitializeWithStream methods
	STDMETHOD(Initialize)(/* [annotation][in] */_In_  IStream *pstream,/* [annotation][in] */_In_  DWORD grfMode);
	// IThumbnailProvider methods
	STDMETHOD(GetThumbnail)(/* [in] */ UINT cx, /* [out] */ __RPC__deref_out_opt HBITMAP *phbmp, /* [out] */ __RPC__out WTS_ALPHATYPE *pdwAlpha);
};
```

The class implements a COM object, servicing two interfaces, IInitializeWithStream, and IThumbnailProvider. So, the QI method checks for those interface requests. The AddRef and Release methods are the two other standard IUnknown methods meant for reference counting. The Initialize method belongs to the IInitializeWithStream interface and receives an IStream pointer to an image file for which a bitmap handle is being requested. This is the first method the shell invokes as soon as a user selects an .heic image file. We cache the stream pointer for later use. The second method the class implements is GetBitmap, the only method of IThumbnailProvider. The shell calls this method to obtain a bitmap handle to a thumbnail image representing the image from the data stream. Internally, GetThumbnail must perform these tasks.
* Set up an operating context for LIBHEIF passing an interface to the image data strem from the shell. The interface is heif_reader, a method table structure pre-defined by LIBHEIF API.
* Open the image. If it contains a thumbnail image, obtain a handle to it.
* Decode the HEIF image, scaling it to the requested size.
* Convert it to color data of the 32-bpp DIB_RGB_COLORS format of Win32 GDI.
* Return the HBITMAP handle of the generated RGB bitmap.

The HEVC image decoding is similar to what HeifImage implements. It's essentially a delegation to `libheic`. As the diagram indicates, what's unique to the thumbnail provider is the need to translate IStream requests from Windows Explorer to calls to the `heif_reader` interface of `libheic`.

```C++
class HeifReader
{
public:
	HeifReader(LPSTREAM pstream) : _stream(pstream)
	{
    ...
		_interface.reader_api_version = 1;
		_interface.get_position = _get_position;
		_interface.read = _read;
		_interface.seek = _seek;
		_interface.wait_for_file_size = _wait_for_file_size;
	}
	~HeifReader()
	{
		_stream->Release();
	}
	heif_reader _interface;

	int64_t getPosition();
	int read(void* data, size_t size);
	int seek(int64_t position);
	heif_reader_grow_status waitForFileSize(int64_t target_size);

protected:
	LPSTREAM _stream;
	STATSTG _stg;

	// --- version 1 functions ---
	static int64_t _get_position(void* pThis) { return ((HeifReader*)pThis)->getPosition(); }
	// The functions read(), and seek() return 0 on success.
	// Generally, libheif will make sure that we do not read past the file size.
	static int _read(void* data, size_t size, void* pThis) { return ((HeifReader*)pThis)->read(data, size); }
	static int _seek(int64_t position, void* pThis) { return ((HeifReader*)pThis)->seek(position); }
	// When calling this function, libheif wants to make sure that it can read the file
	// up to 'target_size'. This is useful when the file is currently downloaded and may
	// grow with time. You may, for example, extract the image sizes even before the actual
	// compressed image data has been completely downloaded.
	//
	// Even if your input files will not grow, you will have to implement at least
	// detection whether the target_size is above the (fixed) file length
	// (in this case, return 'size_beyond_eof').
	static enum heif_reader_grow_status _wait_for_file_size(int64_t target_size, void* pThis) { return ((HeifReader*)pThis)->waitForFileSize(target_size); };
```


## Contributing

Bug reports and enhancement requests are welcome.



## License

Copyright (c) mtanabe, All rights reserved.

Maximilian's HEIC Image Viewer is distributed under the terms of the MIT License.
The libheic and associated libraries are distributed under the terms of the GNU license.
