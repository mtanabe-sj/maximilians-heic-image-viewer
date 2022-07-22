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
#include "ExifIfd.h"


ExifIfdAttribute::ExifIfdAttribute(EXIF_IFD* header, ExifIfd* coll) : _ifd(*header), _coll(coll), _offset(0), _headerLen(0), _valueOffset(0)
{
	_ifd.Tag = _coll->getUSHORT(&_ifd.Tag);
	_ifd.Type = _coll->getUSHORT(&_ifd.Type);
	_ifd.Count = _coll->getULONG(&_ifd.Count);
	// defer endian translation for ValueOrOffset until parse().
	_offset = (int)((LPBYTE)header - _coll->_basePtr);
}

LPCSTR ExifIfdAttribute::_IfdTagName()
{
	switch (_ifd.Tag)
	{
	// primary tags
	case 0x100: return "ImageWidth";
	case 0x101: return "ImageLength";
	case 0x102: return "BitsPerSample";
	case 0x103: return "Compression";
	case 0x106: return "PhotometricInterpretation";
	case 0x10E: return "ImageDescription";
	case 0x10F: return "Make";
	case 0x110: return "Model";
	case 0x111: return "StripOffsets";
	case 0x112: return "Orientation";
	case 0x115: return "SamplesPerPixel";
	case 0x116: return "RowsPerStrip";
	case 0x117: return "StripByteCounts";
	case 0x11A: return "XResolution";
	case 0x11B: return "YResolution";
	case 0x11C: return "PlanarConfiguration";
	case 0x128: return "ResolutionUnit";
	case 0x12D: return "TransferFunction";
	case 0x131: return "Software";
	case 0x132: return "DateTime";
	case 0x13B: return "Artist";
	case 0x13E: return "WhitePoint";
	case 0x13F: return "PrimaryChromaticities";
	case 0x201: return "JPEGInterchangeFormat";
	case 0x202: return "JPEGInterchangeFormatLength";
	case 0x211: return "YCbCrCoefficients";
	case 0x212: return "YCbCrSubSampling";
	case 0x213: return "YCbCrPositioning";
	case 0x214: return "ReferenceBlackWhite";
	case 0x8298: return "Copyright";
	case 0x8769: return "OffsetToExifIFD";
	case 0x8825: return "OffsetToGpsIFD";
		// EXIF tags
	case 0x829A: return "ExposureTime";
	case 0x829D: return "FNumber";
	case 0x8822: return "ExposureProgram";
	case 0x8824: return "SpectralSensitivity";
	case 0x8827: return "ISOSpeedRatings";
	case 0x8828: return "OECF";
	case 0x9000: return "ExifVersion";
	case 0x9003: return "DateTimeOriginal";
	case 0x9004: return "DateTimeDigitized";
	case 0x9101: return "ComponentsConfiguration";
	case 0x9102: return "CompressedBitsPerPixel";
	case 0x9201: return "ShutterSpeedValue";
	case 0x9202: return "ApertureValue";
	case 0x9203: return "BrightnessValue";
	case 0x9204: return "ExposureBiasValue";
	case 0x9205: return "MaxApertureValue";
	case 0x9206: return "SubjectDistance";
	case 0x9207: return "MeteringMode";
	case 0x9208: return "LightSource";
	case 0x9209: return "Flash";
	case 0x920A: return "FocalLength";
	case 0x9214: return "SubjectArea";
	case 0x927C: return "MakerNote";
	case 0x9286: return "UserComment";
	case 0x9290: return "SubsecTime";
	case 0x9291: return "SubsecTimeOriginal";
	case 0x9292: return "SubsecTimeDigitized";
	case 0xA000: return "FlashpixVersion";
	case 0xA001: return "ColorSpace";
	case 0xA002: return "PixelXDimension";
	case 0xA003: return "PixelYDimension";
	case 0xA004: return "RelatedSoundFile";
	case 0xA20B: return "FlashEnergy";
	case 0xA20C: return "SpatialFrequencyResponse";
	case 0xA20E: return "FocalPlaneXResolution";
	case 0xA20F: return "FocalPlaneYResolution";
	case 0xA210: return "FocalPlaneResolutionUnit";
	case 0xA214: return "SubjectLocation";
	case 0xA215: return "ExposureIndex";
	case 0xA217: return "SensingMethod";
	case 0xA300: return "FileSource";
	case 0xA301: return "SceneType";
	case 0xA302: return "CFAPattern";
	case 0xA401: return "CustomRendered";
	case 0xA402: return "ExposureMode";
	case 0xA403: return "WhiteBalance";
	case 0xA404: return "DigitalZoomRatio";
	case 0xA405: return "FocalLengthIn35mmFilm";
	case 0xA406: return "SceneCaptureType";
	case 0xA407: return "GainControl";
	case 0xA408: return "Contrast";
	case 0xA409: return "Saturation";
	case 0xA40A: return "Sharpness";
	case 0xA40B: return "DeviceSettingDescription";
	case 0xA40C: return "SubjectDistanceRange";
	case 0xA420: return "ImageUniqueID";
		// GPS tags
	case 0x0: return "GPSVersionID";
	case 0x1: return "GPSLatitudeRef";
	case 0x2: return "GPSLatitude";
	case 0x3: return "GPSLongitudeRef";
	case 0x4: return "GPSLongitude";
	case 0x5: return "GPSAltitudeRef";
	case 0x6: return "GPSAltitude";
	case 0x7: return "GPSTimestamp";
	case 0x8: return "GPSSatellites";
	case 0x9: return "GPSStatus";
	case 0xA: return "GPSMeasureMode";
	case 0xB: return "GPSDOP";
	case 0xC: return "GPSSpeedRef";
	case 0xD: return "GPSSpeed";
	case 0xE: return "GPSTrackRef";
	case 0xF: return "GPSTrack";
	case 0x10: return "GPSImgDirectionRef";
	case 0x11: return "GPSImgDirection";
	case 0x12: return "GPSMapDatum";
	case 0x13: return "GPSDestLatitudeRef";
	case 0x14: return "GPSDestLatitude";
	case 0x15: return "GPSDestLongitudeRef";
	case 0x16: return "GPSDestLongitude";
	case 0x17: return "GPSDestBearingRef";
	case 0x18: return "GPSDestBearing";
	case 0x19: return "GPSDestDistanceRef";
	case 0x1A: return "GPSDestDistance";
	case 0x1B: return "GPSProcessingMethod";
	case 0x1C: return "GPSAreaInformation";
	case 0x1D: return "GPSDateStamp";
	case 0x1E: return "GPSDifferential";
	}

	static char tagname[16];
	sprintf_s(tagname, sizeof(tagname), "Tag %d", _ifd.Tag);
	return tagname;
}

int ExifIfdAttribute::parse()
{
	LPBYTE p;
	int valLen = _ifd.Count*_coll->_getDataUnitLength((EXIF_IFD_DATATYPE)_ifd.Type);
	if (valLen <= 4)
	{
		p = (LPBYTE)&_ifd.ValueOrOffset;
		_value.resize(valLen);
		memcpy(_value.data(), p, valLen);
	}
	else
	{
		_ifd.ValueOrOffset = _coll->getULONG(&_ifd.ValueOrOffset);
		p = _coll->_basePtr + _ifd.ValueOrOffset;
		_value.resize(valLen);
		memcpy(_value.data(), p, valLen);
		// save the value offset. MetaJpegScan will be use it to create a region in gray that's associated with this ifd attribute entry.
		_valueOffset = (int)_ifd.ValueOrOffset;
	}
	// fix up the data type. some software says user comment is of unknown type. converting it to a blob causes IWICMetadataQueryWriter::SetMetadataByName to raise an E_INVALIDARG. so, force the string type on it.
	if (_ifd.Tag == UserComment && _ifd.Type == IFD_UNKNOWN)
		_ifd.Type = IFD_ASCII;

	_name = _IfdTagName();
	_headerLen = sizeof(EXIF_IFD);
	return _headerLen;
}


///////////////////////////////////

HRESULT PropVariant::assignIfdValue(ExifIfdAttribute &attrib)
{
	EXIF_IFD ifd_ = attrib._ifd;
	std::vector<BYTE> &val_ = attrib._value;
	EndianMetadata *emd_ = attrib._coll;

	HRESULT hr = S_OK;
	switch (ifd_.Type)
	{
	case IFD_UNDEFINED: // considered a blob
		/*
		this->vt = VT_BLOB;
		this->blob.cbSize = val_.size();
		this->blob.pBlobData = (LPBYTE)CoTaskMemAlloc(val_.size());
		if (!this->blob.pBlobData)
			return E_OUTOFMEMORY;
		memcpy(this->blob.pBlobData, val_.data(), val_.size());
		break;
		*/
		hr = InitPropVariantFromBuffer(val_.data(), (UINT)val_.size(), this);
		break;
	case IFD_ASCII:
		this->vt = VT_LPSTR;
		this->pszVal = (LPSTR)CoTaskMemAlloc(val_.size());
		if (!this->pszVal)
			return E_OUTOFMEMORY;
		memcpy(this->pszVal, val_.data(), val_.size());
		break;
	case IFD_BYTE: // VT_UI1 | VT_VECTOR
		if (ifd_.Count == 1)
		{
			this->vt = VT_UI1;
			this->bVal = val_[0];
		}
		else
		{
			this->vt |= VT_VECTOR | VT_UI1;
			this->caub.cElems = ifd_.Count;
			this->caub.pElems = (UCHAR*)CoTaskMemAlloc(val_.size());
			memcpy(this->caub.pElems, val_.data(), ifd_.Count);
		}
		break;
	case IFD_CHAR:
		if (ifd_.Count == 1)
		{
			this->vt = VT_I1;
			this->cVal = val_[0];
		}
		else
		{
			this->vt |= VT_VECTOR | VT_I1;
			this->cac.cElems = ifd_.Count;
			this->cac.pElems = (CHAR*)CoTaskMemAlloc(val_.size());
			memcpy(this->cac.pElems, val_.data(), ifd_.Count);
		}
		break;
	case IFD_USHORT:
		if (ifd_.Count == 1)
		{
			hr = InitPropVariantFromUInt16(emd_->getUSHORT(val_.data()), this);
		}
		else
		{
			std::vector<USHORT> v;
			USHORT *d = (USHORT*)val_.data();
			for (UINT i = 0; i < ifd_.Count; i++)
				v.push_back(emd_->getUSHORT(d++));
			hr = InitPropVariantFromUInt16Vector(v.data(), ifd_.Count, this);
		}
		break;
	case IFD_SHORT:
		if (ifd_.Count == 1)
		{
			hr = InitPropVariantFromInt16(emd_->getUSHORT(val_.data()), this);
		}
		else
		{
			std::vector<SHORT> v;
			SHORT *d = (SHORT*)val_.data();
			for (UINT i = 0; i < ifd_.Count; i++)
				v.push_back((SHORT)emd_->getUSHORT(d++));
			hr = InitPropVariantFromInt16Vector(v.data(), ifd_.Count, this);
		}
		break;
	case IFD_ULONG:
		if (ifd_.Count == 1)
		{
			hr = InitPropVariantFromUInt32(emd_->getULONG(val_.data()), this);
		}
		else
		{
			std::vector<ULONG> v;
			ULONG *d = (ULONG*)val_.data();
			for (UINT i = 0; i < ifd_.Count; i++)
				v.push_back(emd_->getULONG(d++));
			hr = InitPropVariantFromUInt32Vector(v.data(), ifd_.Count, this);
		}
		break;
	case IFD_LONG:
		if (ifd_.Count == 1)
		{
			hr = InitPropVariantFromInt32(emd_->getULONG(val_.data()), this);
		}
		else
		{
			std::vector<LONG> v;
			LONG *d = (LONG*)val_.data();
			for (UINT i = 0; i < ifd_.Count; i++)
				v.push_back(emd_->getULONG(d++));
			hr = InitPropVariantFromInt32Vector(v.data(), ifd_.Count, this);
		}
		break;
	case IFD_ULONGLONG:
		if (ifd_.Count == 1)
		{
			hr = InitPropVariantFromUInt64(emd_->getULONGLONG(val_.data()), this);
		}
		else
		{
			std::vector<ULONGLONG> v;
			ULONGLONG *d = (ULONGLONG*)val_.data();
			for (UINT i = 0; i < ifd_.Count; i++)
				v.push_back(emd_->getULONGLONG(d++));
			hr = InitPropVariantFromUInt64Vector(v.data(), ifd_.Count, this);
			/* // alternatively, we could use this.
			this->vt |= VT_VECTOR|VT_UI8;
			this->cauh.cElems = ifd_.Count;
			this->cauh.pElems = (ULARGE_INTEGER*)CoTaskMemAlloc(ifd_.Count * sizeof(ULONGLONG));
			ULONGLONG *d = (ULONGLONG*)val_.data();
			for (UINT i = 0; i < ifd_.Count; i++)
				this->cauh.pElems[i].QuadPart = emd_->getULONGLONG(d++);
			*/
		}
		break;
	case IFD_LONGLONG:
		if (ifd_.Count == 1)
		{
			hr = InitPropVariantFromInt64(emd_->getULONGLONG(val_.data()), this);
		}
		else
		{
			std::vector<LONGLONG> v;
			LONGLONG *d = (LONGLONG*)val_.data();
			for (UINT i = 0; i < ifd_.Count; i++)
				v.push_back(emd_->getULONGLONG(d++));
			hr = InitPropVariantFromInt64Vector(v.data(), ifd_.Count, this);
		}
		break;
	case IFD_FLOAT:
		if (ifd_.Count == 1)
		{
			this->vt = VT_R4;
			this->fltVal = emd_->getFLOAT(val_.data());
		}
		else
		{
			// unfortunately, there is no InitPropVariantFromFloatVector...
			this->vt |= VT_VECTOR | VT_R4;
			this->caflt.cElems = ifd_.Count;
			this->caflt.pElems = (FLOAT*)CoTaskMemAlloc(ifd_.Count * sizeof(FLOAT));
			FLOAT *d = (FLOAT*)val_.data();
			for (UINT i = 0; i < ifd_.Count; i++)
				this->caflt.pElems[i] = emd_->getFLOAT(d++);
		}
		break;
	case IFD_DOUBLE:
		if (ifd_.Count == 1)
		{
			hr = InitPropVariantFromDouble(emd_->getDOUBLE(val_.data()), this);
		}
		else
		{
			std::vector<DOUBLE> v;
			DOUBLE *d = (DOUBLE*)val_.data();
			for (UINT i = 0; i < ifd_.Count; i++)
				v.push_back(emd_->getDOUBLE(d++));
			hr = InitPropVariantFromDoubleVector(v.data(), ifd_.Count, this);
		}
		break;
	default:
		ASSERT(FALSE);
		hr = E_INVALIDARG;
	}
	return hr;
}


///////////////////////////////////


int ExifIfd::parse(int dataOffset)
{
	LPBYTE src = _basePtr + dataOffset;
	int len, usedLen = sizeof(USHORT);
	USHORT i, tagCount = getUSHORT(src);
	LPBYTE p = src + sizeof(tagCount);
	int endingOffset = 0, nextEnding;
	for (i = 0; i < tagCount; i++)
	{
		ExifIfdAttribute next((EXIF_IFD*)p, this);
		// note that len does not include the data bytes.
		len = next.parse();
		push_back(next);
		usedLen += len;
		p += len;
		nextEnding = next._valueOffset + (int)next._value.size();
		if (nextEnding > endingOffset)
			endingOffset = nextEnding;
	}
	_offset = dataOffset;
	_headerLen = sizeof(tagCount);
	_directoryLen = usedLen - _headerLen;
	if (endingOffset)
		_totalLen = endingOffset - dataOffset;
	else
		_totalLen = usedLen;
	_dataLen = _totalLen - _headerLen - _directoryLen;
	return usedLen; // return the directory length.
}

const ExifIfdAttribute *ExifIfd::find(USHORT tagId) const
{
	for (int i = 0; i < (int)size(); i++)
	{
		if ((*this)[i]._ifd.Tag == tagId)
			return &(*this)[i];
	}
	return NULL;
}

int ExifIfd::_getDataUnitLength(EXIF_IFD_DATATYPE dtype) const
{
	switch (dtype)
	{
	case IFD_BYTE:
	case IFD_ASCII:
	case IFD_CHAR:
	case IFD_UNDEFINED:
		return 1;
	case IFD_USHORT:
	case IFD_SHORT:
		return 2;
	case IFD_ULONG:
	case IFD_LONG:
	case IFD_FLOAT:
		return 4;
	case IFD_ULONGLONG:
	case IFD_LONGLONG:
	case IFD_DOUBLE:
		return 8;
	}
	return 0;
}

ULONG ExifIfd::_getIntValueOf(const ExifIfdAttribute *attrib) const
{
	ULONG ret = 0;
	switch (attrib->_ifd.Type)
	{
	case IFD_BYTE:
	case IFD_ASCII:
	case IFD_CHAR:
	case IFD_UNDEFINED:
		ret = std::stoi(_getStrValueOf(attrib));
		break;
	case IFD_FLOAT:
		ret = (ULONG)(LONG)getFLOAT((LPVOID)attrib->_value.data());
		break;
	case IFD_DOUBLE:
		ret = (ULONG)(LONG)getDOUBLE((LPVOID)attrib->_value.data());
		break;
	case IFD_USHORT:
	case IFD_SHORT:
		ret = MAKELONG(getUSHORT((LPVOID)attrib->_value.data()), 0);
		break;
	case IFD_ULONG:
	case IFD_LONG:
		ret = getULONG((LPVOID)attrib->_value.data());
		break;
	case IFD_ULONGLONG:
	case IFD_LONGLONG:
		ret = (ULONG)getULONGLONG((LPVOID)attrib->_value.data());
		break;
	}
	return ret;
}

std::string ExifIfd::_getStrValueOf(const ExifIfdAttribute *attrib) const
{
	std::string ret;
	switch (attrib->_ifd.Type)
	{
	case IFD_BYTE:
	case IFD_ASCII:
	case IFD_CHAR:
	case IFD_UNDEFINED:
		ret.assign(attrib->_value.begin(), attrib->_value.end());
		return ret;
	}

	char buf[128] = { 0 };
	switch (attrib->_ifd.Type)
	{
	case IFD_USHORT:
		sprintf_s(buf, sizeof(buf), "%u", getUSHORT((LPVOID)attrib->_value.data()));
		break;
	case IFD_SHORT:
		sprintf_s(buf, sizeof(buf), "%d", (short)getUSHORT((LPVOID)attrib->_value.data()));
		break;
	case IFD_FLOAT:
		sprintf_s(buf, sizeof(buf), "%g", (double)(float)(short)getUSHORT((LPVOID)attrib->_value.data()));
		break;
	case IFD_DOUBLE:
		sprintf_s(buf, sizeof(buf), "%g", (double)(long)getULONG((LPVOID)attrib->_value.data()));
		break;
	case IFD_ULONG:
		sprintf_s(buf, sizeof(buf), "%lu", getULONG((LPVOID)attrib->_value.data()));
		break;
	case IFD_LONG:
		sprintf_s(buf, sizeof(buf), "%ld", (long)getULONG((LPVOID)attrib->_value.data()));
		break;
	case IFD_ULONGLONG:
		sprintf_s(buf, sizeof(buf), "%I64u", getULONGLONG((LPVOID)attrib->_value.data()));
		break;
	case IFD_LONGLONG:
		sprintf_s(buf, sizeof(buf), "%I64d", (LONGLONG)getULONGLONG((LPVOID)attrib->_value.data()));
		break;
	}
	return buf;
}

ULONG ExifIfd::queryAttributeValueInt(USHORT tagId) const
{
	const ExifIfdAttribute *attrib = find(tagId);
	if (attrib)
		return _getIntValueOf(attrib);
	return 0;
}

std::string ExifIfd::queryAttributeValueStr(USHORT tagId) const
{
	const ExifIfdAttribute *attrib = find(tagId);
	if (attrib)
		return _getStrValueOf(attrib);
	return "";
}
