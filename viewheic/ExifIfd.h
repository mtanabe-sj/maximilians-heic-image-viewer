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
#include <vector>
#include <string>


#pragma pack(1)
struct EXIF_TIFFHEADER
{
	USHORT ByteOrder; // 0x4949=little endian; 0x4D4D=big endian
	USHORT Signature; // always set to 0x002A
	ULONG IFDOffset; // offset to 0th IFD. If the TIFF header is followed immediately by the 0th IFD, it is written as 00000008.
};

// data type values for EXIF_IFD.Type
enum EXIF_IFD_DATATYPE
{
	IFD_UNKNOWN,
	IFD_BYTE,
	IFD_ASCII,
	IFD_USHORT,
	IFD_ULONG,
	IFD_ULONGLONG,
	IFD_CHAR,
	IFD_UNDEFINED,
	IFD_SHORT,
	IFD_LONG,
	IFD_LONGLONG,
	IFD_FLOAT,
	IFD_DOUBLE,
	IFD_MAX
};
struct EXIF_IFD
{
	USHORT Tag;
	USHORT Type; // 1=byte, 2=ascii, 3=short (2 bytes), 4=long (4 bytes), 5=rational (2 longs), 7=undefined, 9=signed long, 10=signed rational (2 slongs).
	ULONG Count; // number of elements of data type Type recorded in either ValueOrOffset or in a memory location pointed to by ValueOrOffset plus the starting address of the TIFFHeader.
	ULONG ValueOrOffset; // This tag records the offset from the start of the TIFF header to the position where the value itself is recorded. In cases where the value fits in 4 bytes, the value itself is recorded.If the value is smaller than 4 bytes, the value is stored in the 4 - byte area starting from the left, i.e., from the lower end of the byte offset area.For example, in big endian format, if the type is SHORT and the value is 1, it is recorded as 00010000.H.
};
#pragma pack()

enum EXIF_IFDCLASS
{
	EXIFIFDCLASS_PRIMARY = 0, // ImageWidth (0x100) .. Copyright (0x8298)
	EXIFIFDCLASS_EXIF, // ExposureTime (0x829A) .. (0xFFFF)
	EXIFIFDCLASS_GPS, // GPSVersionID (0) .. (0xFF)
	EXIFIFDCLASS_THUMBNAIL,
	EXIFIFDCLASS_MAX
};


// exif (exchangeable image file format) metadata parser
// ifd (image file directory)

class ExifIfd;
class ExifIfdAttribute
{
public:
	ExifIfdAttribute(EXIF_IFD* header, ExifIfd* coll);

	int parse();

	ExifIfd *_coll;
	int _offset;
	int _headerLen;
	int _valueOffset; // an offset to where the value data is located; relative to _coll->_base->_basePtr. the value's length is in _value._length.
	EXIF_IFD _ifd;
	std::string _name;
	std::vector<BYTE> _value;

	LPCSTR _IfdTagName();

	enum EXIFTAG
	{
		// primary tags
		ImageWidth = 0x100,
		ImageLength = 0x101,
		BitsPerSample = 0x102,
		Compression = 0x103,
		PhotometricInterpretation = 0x106,
		ImageDescription = 0x10E,
		Make = 0x10F,
		Model = 0x110,
		StripOffsets = 0x111,
		Orientation = 0x112,
		SamplesPerPixel = 0x115,
		RowsPerStrip = 0x116,
		StripByteCounts = 0x117,
		XResolution = 0x11A,
		YResolution = 0x11B,
		PlanarConfiguration = 0x11C,
		ResolutionUnit = 0x128,
		TransferFunction = 0x12D,
		Software = 0x131,
		DateTime = 0x132,
		Artist = 0x13B,
		WhitePoint = 0x13E,
		PrimaryChromaticities = 0x13F,
		JPEGInterchangeFormat = 0x201,
		JPEGInterchangeFormatLength = 0x202,
		YCbCrCoefficients = 0x211,
		YCbCrSubSampling = 0x212,
		YCbCrPositioning = 0x213,
		ReferenceBlackWhite = 0x214,
		Copyright = 0x8298,
		OffsetToExifIFD = 0x8769,
		OffsetToGpsIFD = 0x8825,

		// EXIF tags
		ExposureTime = 0x829A,
		FNumber = 0x829D,
		ExposureProgram = 0x8822,
		SpectralSensitivity = 0x8824,
		ISOSpeedRatings = 0x8827,
		OECF = 0x8828,
		ExifVersion = 0x9000,
		DateTimeOriginal = 0x9003,
		DateTimeDigitized = 0x9004,
		ComponentsConfiguration = 0x9101,
		CompressedBitsPerPixel = 0x9102,
		ShutterSpeedValue = 0x9201,
		ApertureValue = 0x9202,
		BrightnessValue = 0x9203,
		ExposureBiasValue = 0x9204,
		MaxApertureValue = 0x9205,
		SubjectDistance = 0x9206,
		MeteringMode = 0x9207,
		LightSource = 0x9208,
		Flash = 0x9209,
		FocalLength = 0x920A,
		SubjectArea = 0x9214,
		MakerNote = 0x927C,
		UserComment = 0x9286,
		SubsecTime = 0x9290,
		SubsecTimeOriginal = 0x9291,
		SubsecTimeDigitized = 0x9292,
		FlashpixVersion = 0xA000,
		ColorSpace = 0xA001,
		PixelXDimension = 0xA002,
		PixelYDimension = 0xA003,
		RelatedSoundFile = 0xA004,
		FlashEnergy = 0xA20B,
		SpatialFrequencyResponse = 0xA20C,
		FocalPlaneXResolution = 0xA20E,
		FocalPlaneYResolution = 0xA20F,
		FocalPlaneResolutionUnit = 0xA210,
		SubjectLocation = 0xA214,
		ExposureIndex = 0xA215,
		SensingMethod = 0xA217,
		FileSource = 0xA300,
		SceneType = 0xA301,
		CFAPattern = 0xA302,
		CustomRendered = 0xA401,
		ExposureMode = 0xA402,
		WhiteBalance = 0xA403,
		DigitalZoomRatio = 0xA404,
		FocalLengthIn35mmFilm = 0xA405,
		SceneCaptureType = 0xA406,
		GainControl = 0xA407,
		Contrast = 0xA408,
		Saturation = 0xA409,
		Sharpness = 0xA40A,
		DeviceSettingDescription = 0xA40B,
		SubjectDistanceRange = 0xA40C,
		ImageUniqueID = 0xA420,

		// GPS tags
		GPSVersionID = 0x0,
		GPSLatitudeRef = 0x1,
		GPSLatitude = 0x2,
		GPSLongitudeRef = 0x3,
		GPSLongitude = 0x4,
		GPSAltitudeRef = 0x5,
		GPSAltitude = 0x6,
		GPSTimestamp = 0x7,
		GPSSatellites = 0x8,
		GPSStatus = 0x9,
		GPSMeasureMode = 0xA,
		GPSDOP = 0xB,
		GPSSpeedRef = 0xC,
		GPSSpeed = 0xD,
		GPSTrackRef = 0xE,
		GPSTrack = 0xF,
		GPSImgDirectionRef = 0x10,
		GPSImgDirection = 0x11,
		GPSMapDatum = 0x12,
		GPSDestLatitudeRef = 0x13,
		GPSDestLatitude = 0x14,
		GPSDestLongitudeRef = 0x15,
		GPSDestLongitude = 0x16,
		GPSDestBearingRef = 0x17,
		GPSDestBearing = 0x18,
		GPSDestDistanceRef = 0x19,
		GPSDestDistance = 0x1A,
		GPSProcessingMethod = 0x1B,
		GPSAreaInformation = 0x1C,
		GPSDateStamp = 0x1D,
		GPSDifferential = 0x1E
	};
};


class PropVariant : public PROPVARIANT
{
public:
	PropVariant()
	{
		PropVariantInit(this);
	}
	~PropVariant()
	{
		PropVariantClear(this);
	}

	operator const PROPVARIANT *() { return this; }
	HRESULT assignIfdValue(ExifIfdAttribute &attrib);
};


class EndianMetadata
{
public:
	EndianMetadata(LPBYTE srcPtr, int srcLen, bool bigEndian = false) : _basePtr(srcPtr), _baseLen(srcLen), _big(bigEndian), _headerLen(0) {}

	bool _big;
	int _headerLen;
	int _baseLen;
	const LPBYTE _basePtr;

	USHORT toUSHORT(USHORT *src) const
	{
		_reverseByteOrder(src, sizeof(USHORT));
		return *src;
	}
	ULONG toULONG(ULONG *src) const
	{
		_reverseByteOrder(src, sizeof(ULONG));
		return *src;
	}

	USHORT getUSHORT(LPVOID src) const
	{
		USHORT v = *(USHORT*)src;
		if (_big)
			_reverseByteOrder(&v, sizeof(v));
		return v;
	}
	ULONG getULONG(LPVOID src) const
	{
		ULONG v = *(ULONG*)src;
		if (_big)
			_reverseByteOrder(&v, sizeof(v));
		return v;
	}
	ULONGLONG getULONGLONG(LPVOID src) const
	{
		ULONGLONG v = *(ULONGLONG*)src;
		if (_big)
			_reverseByteOrder(&v, sizeof(v));
		return v;
	}
	float getFLOAT(LPVOID src) const
	{
		float v = *(float*)src;
		if (_big)
			_reverseByteOrder(&v, sizeof(v));
		return v;
	}
	double getDOUBLE(LPVOID src) const
	{
		double v = *(double*)src;
		if (_big)
			_reverseByteOrder(&v, sizeof(v));
		return v;
	}

	static void _reverseByteOrder(LPVOID pv, size_t len)
	{
		LPBYTE p = (LPBYTE)pv;
		unsigned char *p2 = p + len - 1;
		size_t n = len >> 1;
		while (n--)
		{
			std::swap(*p2--, *p++);
		}
	}
};


class ExifIfd : public EndianMetadata, public std::vector<ExifIfdAttribute>
{
public:
	ExifIfd(EXIF_IFDCLASS c, LPBYTE srcPtr, int srcLen, bool bigEndian) : _class(c), EndianMetadata(srcPtr, srcLen, bigEndian), _offset(0), _headerLen(0), _directoryLen(0), _dataLen(0), _totalLen(0) {}

	EXIF_IFDCLASS _class; // directory class
	int _offset; // an offset of the ifd header measured from the app1 tiff header.
	int _headerLen; // a 2-byte directory size specifier.
	int _directoryLen; // data fields represented by instances of ExifIfdAttribute.
	int _dataLen; // size of a space that follows the directory fields and holds the values of attributes specified in the fields.
	int _totalLen; // = _headerLen+_directoryLen+_dataLen

	int parse(int dataOffset);
	const ExifIfdAttribute *find(USHORT tagId) const;
	ULONG queryAttributeValueInt(USHORT tagId) const;
	std::string queryAttributeValueStr(USHORT tagId) const;

	int _getDataUnitLength(EXIF_IFD_DATATYPE dtype) const;
	ULONG _getIntValueOf(const ExifIfdAttribute *attrib) const;
	std::string _getStrValueOf(const ExifIfdAttribute *attrib) const;

	// use this instead of the [] operator if the attribute's _coll may be obsolete and needs to be corrected.
	ExifIfdAttribute &at(size_t pos)
	{
		ExifIfdAttribute &attrib = std::vector<ExifIfdAttribute>::at(pos);
		attrib._coll = this;
		return attrib;
	}
};
