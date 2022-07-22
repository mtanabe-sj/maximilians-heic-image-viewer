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
#include "IUnknownImpl.h"

#define MAX_SOF_STREAM_BUFF 0x1000

const ULONG SOF_READ = 0x00000001;
const ULONG SOF_WRITE = 0x00000002;
const ULONG SOF_SHARED = 0x00000010;
const ULONG SOF_SHAREDREAD = 0x00000020;
const ULONG SOF_SHAREDWRITE = 0x00000040;
const ULONG SOF_DELETE = 0x10000000;

class StreamOnFile : public IUnknownImpl<IStream, &IID_IStream>
{
public:
	StreamOnFile(HANDLE hf=NULL) { _init(); _hf = hf; }
	virtual ~StreamOnFile() { Close(); }

	operator LPSTREAM() { return this; }

	DWORD _lastError;
	// *** IStream methods ***
	STDMETHOD(Read) (VOID* lpv, ULONG cb, ULONG* lpcbRead) {
		DWORD cbRead = 0;
		if (!::ReadFile(_hf, lpv, cb, &cbRead, NULL)) {
			_lastError = ::GetLastError();
			return E_FAIL;
		}
		if (lpcbRead)
			*lpcbRead = cbRead;
		return S_OK;
	}
	STDMETHOD(Write) (VOID const* lpv, ULONG cb, ULONG* lpcbWritten) {
		if (!(_openFlags & SOF_WRITE))
			return STG_E_ACCESSDENIED;
		if (cb == 0)
			return S_OK; // Nothing to do.
		_dirty = TRUE;
		DWORD cbWritten = 0;
		if (!::WriteFile(_hf, lpv, cb, &cbWritten, NULL)) {
			_lastError = GetLastError();
			return E_FAIL;
		}
		if (cbWritten != cb) {
			_lastError = ERROR_WRITE_FAULT;
			return E_UNEXPECTED;
		}
		if (lpcbWritten)
			*lpcbWritten = cbWritten;
		return S_OK;
	}
	STDMETHOD(Seek) (LARGE_INTEGER libMove, DWORD dwOrigin, ULARGE_INTEGER* lplibNewPosition) {
		if (libMove.HighPart)
			return E_INVALIDARG;
		LARGE_INTEGER cbSeek;
		DWORD dwMove = FILE_BEGIN; // For STREAM_SEEK_SET
		if (dwOrigin == STREAM_SEEK_CUR)
			dwMove = FILE_CURRENT;
		else if (dwOrigin == STREAM_SEEK_END)
			dwMove = FILE_END;
		if (!SetFilePointerEx(_hf, libMove, &cbSeek, dwMove)) {
			_lastError = GetLastError();
			return E_FAIL;
		}
		if (lplibNewPosition)
			lplibNewPosition->QuadPart = cbSeek.QuadPart;
		return S_OK;
	}
	STDMETHOD(SetSize) (ULARGE_INTEGER libNewSize) {
		LARGE_INTEGER liCurPos;
		LARGE_INTEGER liZero = { 0, 0 };
		HRESULT hr = Seek(liZero, STREAM_SEEK_CUR, (ULARGE_INTEGER*)&liCurPos);
		if (hr == S_OK) {
			LARGE_INTEGER libMove; libMove.QuadPart = (LONGLONG)libNewSize.QuadPart;
			hr = Seek(libMove, STREAM_SEEK_SET, NULL);
			if (hr == S_OK) {
				if (!SetEndOfFile(_hf)) {
					_lastError = GetLastError();
					hr = E_FAIL;
				}
			}
			Seek(liCurPos, STREAM_SEEK_SET, NULL);
		}
		return hr;
	}
	STDMETHOD(CopyTo) (IStream* lpstm, ULARGE_INTEGER cbCopy, ULARGE_INTEGER* lpcbRead, ULARGE_INTEGER* lpcbWritten) {
		HRESULT hr;
		if (cbCopy.HighPart) //TODO: remove this restriction
			return E_INVALIDARG;
		/* If our buffer is dirty, then we've been writing into it
		and we need to flush it.
		*/
		if (_dirty) {
			hr = Commit(STGC_DEFAULT);
			if (FAILED(hr))
				return hr;
		}
		// Save the current position. This will be copied
		// to a new stream.
		LARGE_INTEGER liCurPos;
		LARGE_INTEGER liZero = { 0, 0 };
		hr = Seek(liZero, STREAM_SEEK_CUR, (ULARGE_INTEGER*)&liCurPos);
		if (hr != S_OK)
			return hr;
		HGLOBAL hDTA = GlobalAlloc(GHND, MAX_SOF_STREAM_BUFF);
		if (!hDTA) {
			_lastError = GetLastError();
			return E_OUTOFMEMORY;
		}
		LPBYTE pDTA = (LPBYTE)GlobalLock(hDTA);
		ULONG cbLeft = cbCopy.LowPart;
		while (cbLeft) {
			ULONG cb = min(cbLeft, MAX_SOF_STREAM_BUFF);
			ULONG cbRead;
			hr = Read(pDTA, cb, &cbRead);
			if (hr != S_OK)
				break;
			if (lpcbRead)
				lpcbRead->LowPart += cbRead;
			ULONG cbWritten;
			hr = lpstm->Write(pDTA, cbRead, &cbWritten);
			if (hr != S_OK)
				break;
			if (lpcbWritten)
				lpcbWritten->LowPart += cbWritten;
			cbLeft -= cbRead;
			if (cbRead < cb)
				break;
		}
		GlobalUnlock(hDTA);
		GlobalFree(hDTA);
		// Move the seek pointer to the same position.
		Seek(liCurPos, STREAM_SEEK_SET, NULL);
		return hr;
	}
	STDMETHOD(Commit) (DWORD dwCommitFlags) {
		if (!(_openFlags & SOF_WRITE))
			return E_INVALIDARG;
		if (dwCommitFlags != STGC_DEFAULT && dwCommitFlags != STGC_OVERWRITE)
			return E_INVALIDARG;
		_commits++;
		_committedLen.LowPart = GetFileSize(_hf, &_committedLen.HighPart);
		if (!_dirty)
			return S_OK;
		HRESULT hr = Reopen();
		if (hr == S_OK) {
			LARGE_INTEGER liZero = { 0, 0 };
			Seek(liZero, STREAM_SEEK_END, &_committedLen);
			_dirty = FALSE;
		}
		return hr;
	}
	STDMETHOD(Revert) () {
		_dirty = FALSE;
		// Reset the cache size.
		return SetSize(_committedLen);
	}
	STDMETHOD(LockRegion) (ULARGE_INTEGER libOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
		return E_NOTIMPL;
	}
	STDMETHOD(UnlockRegion) (ULARGE_INTEGER ulibOffset, ULARGE_INTEGER cb, DWORD dwLockType) {
		return E_NOTIMPL;
	}
	STDMETHOD(Stat) (STATSTG* lpstatstg, DWORD dwStatFlag) {
		if (!lpstatstg)
			return E_POINTER;
		memset(lpstatstg, 0, sizeof(STATSTG));
		lpstatstg->type = STGTY_STREAM;
		BY_HANDLE_FILE_INFORMATION fi;
		if (!GetFileInformationByHandle(_hf, &fi)) {
			_lastError = GetLastError();
			return E_FAIL;
		}
		lpstatstg->ctime = fi.ftCreationTime;
		lpstatstg->mtime = fi.ftLastWriteTime;
		lpstatstg->atime = fi.ftLastAccessTime;
		lpstatstg->cbSize.LowPart = fi.nFileSizeLow;
		lpstatstg->cbSize.HighPart = fi.nFileSizeHigh;
		if (!(dwStatFlag & STATFLAG_NONAME)) {
			// caller wants the file name too
			LPCTSTR pszTitle = GetFileTitlePtr(_filename);
			UINT cb = lstrlen(pszTitle) + 1;
			lpstatstg->pwcsName = (LPWSTR)CoTaskMemAlloc(cb * sizeof(OLECHAR));
			lstrcpy(lpstatstg->pwcsName, pszTitle);
		}
		return S_OK;
	}
	STDMETHOD(Clone) (IStream** lppstm) {
		*lppstm = NULL;
		ASSERT(!_dirty);
		// Save the current position. This will be copied
		// to a new stream.
		HRESULT hr;
		LARGE_INTEGER liCurPos;
		LARGE_INTEGER liZero = { 0, 0 };
		hr = Seek(liZero, STREAM_SEEK_CUR, (ULARGE_INTEGER*)&liCurPos);
		if (hr != S_OK)
			return hr;
		IStream* pStream = NULL;
		hr = CreateStreamOnHGlobal(NULL, TRUE, &pStream);
		if (hr != S_OK)
			return hr;
		ULARGE_INTEGER cbSize;
		cbSize.LowPart = GetFileSize(_hf, &cbSize.HighPart);
		hr = pStream->SetSize(cbSize);
		if (hr != S_OK) {
			pStream->Release();
			Seek(liCurPos, STREAM_SEEK_SET, NULL);
			return hr;
		}
		ASSERT(cbSize.HighPart == 0);
		HGLOBAL hGlobal = NULL;
		GetHGlobalFromStream(pStream, &hGlobal);
		ASSERT(hGlobal);
		LPBYTE pBuffer = (LPBYTE)GlobalLock(hGlobal);
		hr = Read(pBuffer, cbSize.LowPart, NULL);
		GlobalUnlock(hGlobal);
		if (hr == S_OK)
			hr = pStream->Commit(STGC_DEFAULT);
		// Move the seek pointer to the same position.
		Seek(liCurPos, STREAM_SEEK_SET, NULL);
		*lppstm = pStream;
		return hr;
	}

	// *** StreamOnFileSimple methods ***
	virtual HRESULT Create(LPCTSTR pszFileName, ULONG ulFlags = SOF_READ | SOF_WRITE) {
		DWORD dwAccess;
		DWORD dwShare;
		_openFlags = ulFlags;
		if (S_OK != InitInternal(pszFileName, dwAccess, dwShare))
			return E_OUTOFMEMORY;
	_retryCreate:
		ASSERT(_hf == NULL || _hf == INVALID_HANDLE_VALUE);
		_hf = CreateFile(
			_filename,
			dwAccess,
			dwShare,
			NULL,
			CREATE_ALWAYS,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (_hf != INVALID_HANDLE_VALUE)
			return S_OK;
		_lastError = GetLastError();
		if (_lastError == ERROR_ACCESS_DENIED) {
			DWORD dwVal = GetFileAttributes(_filename);
			if (dwVal & FILE_ATTRIBUTE_READONLY) {
				if (SetFileAttributes(_filename, FILE_ATTRIBUTE_NORMAL)) {
					if (DeleteFile(_filename))
						goto _retryCreate;
				}
			}
		}
		return E_FAIL;
	}
	virtual HRESULT Open(LPCTSTR pszFileName, ULONG ulFlags = SOF_READ) {
		DWORD dwAccess;
		DWORD dwShare;
		_openFlags = ulFlags;
		if (S_OK != InitInternal(pszFileName, dwAccess, dwShare))
			return E_OUTOFMEMORY;
	_retryCreate:
		ASSERT(_hf == NULL || _hf == INVALID_HANDLE_VALUE);
		_hf = CreateFile(
			_filename,
			dwAccess,
			dwShare,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (_hf != INVALID_HANDLE_VALUE)
			return S_OK;
		_lastError = GetLastError();
		if (_lastError == ERROR_ACCESS_DENIED) {
			DWORD dwVal = GetFileAttributes(_filename);
			if (dwVal & (FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_HIDDEN)) {
				if (SetFileAttributes(_filename, FILE_ATTRIBUTE_NORMAL))
					goto _retryCreate;
			}
		}
		return E_FAIL;
	}
	virtual HRESULT Reopen() {
		ASSERT(_hf);
		CloseHandle(_hf);
		ASSERT(_filename);
		DWORD dwAccess;
		DWORD dwShare;
		GetCreateFileFlags(dwAccess, dwShare);
		_hf = CreateFile(
			_filename,
			dwAccess,
			dwShare,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL);
		if (_hf == INVALID_HANDLE_VALUE) {
			_lastError = GetLastError();
			return E_FAIL;
		}
		return S_OK;
	}
	virtual void Close() {
		if (_hf) {
			CloseHandle(_hf);
			_hf = NULL;
		}
		if (_filename) {
			if (_openFlags & SOF_DELETE) {
				DeleteFile(_filename);
			}
			free(_filename);
			_filename = NULL;
		}
	}
	LPCTSTR GetPathName() { return _filename; }


protected:
	LPTSTR _filename; // file name
	HANDLE _hf; // file handle
	ULARGE_INTEGER _committedLen; // committed size
	int _commits; // number of times the commit has been called since last write
	ULONG _openFlags; // open flags
	BOOL _dirty; // TRUE if the content has been modified

	void _init()
	{
		_hf = NULL;
		_committedLen.QuadPart = 0;
		_openFlags = 0;
		_commits = 0;
		_dirty = FALSE;
		_filename = NULL;
		_lastError = 0;
	}
	HRESULT InitInternal(LPCTSTR pszFileName, DWORD& dwAccess, DWORD& dwShare) {
		if (_filename) {
			free(_filename);
			_filename = NULL;
		}
		_filename = _tcsdup(pszFileName);
		if (!_filename)
			return E_OUTOFMEMORY;
		_committedLen.QuadPart = 0;
		_commits = 0;
		GetCreateFileFlags(dwAccess, dwShare);
		return S_OK;
	}
	void GetCreateFileFlags(DWORD& dwAccess, DWORD& dwShare) {
		dwAccess = 0;
		dwShare = 0;
		if (_openFlags & SOF_READ)
			dwAccess |= GENERIC_READ;
		if (_openFlags & SOF_WRITE)
			dwAccess |= GENERIC_WRITE;
		if (_openFlags & SOF_SHARED) {
			dwShare |= FILE_SHARE_READ;
			if (_openFlags & SOF_WRITE)
				dwShare |= FILE_SHARE_WRITE;
		}
		if (_openFlags & SOF_SHAREDREAD)
			dwShare |= FILE_SHARE_READ;
		if (_openFlags & SOF_SHAREDWRITE)
			dwShare |= FILE_SHARE_WRITE;
	}
};

