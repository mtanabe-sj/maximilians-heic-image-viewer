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
#include <ocidl.h>
#include <objsafe.h>


//#define SUPPORT_FREE_THREADING_COM

#ifndef LIB_ADDREF
// no explicit reference counting is performed to keep the app alive.
// if that's needed, re-define LIB_ADDREF and LIB_RELEASE in stdafx.h.
#define LIB_ADDREF
#define LIB_RELEASE
#endif//#ifndef LIB_ADDREF

//#define NO_VTABLE __declspec(novtable)
#define NO_VTABLE


template <class T>
T* ADDREFASSIGN(T *p) {
	if (p)
		dynamic_cast<IUnknown*>(p)->AddRef();
	return p;
}
template <class T>
void FINALRELEASE(T** p) {
	T* p2 = (T*)InterlockedExchangePointer((LPVOID*)p, NULL);
	if (p2)
		dynamic_cast<IUnknown*>(p2)->Release();
}


//NOTE: make sure you include <combaseapi.h> to avoid a compiler error.
template <class T>
class _ComRef : public T
{
private:
	STDMETHOD_(ULONG, AddRef)() = 0;
	STDMETHOD_(ULONG, Release)() = 0;
};


template <class T>
class AutoComRel {
public:
	AutoComRel(T* p = NULL) : _p(ADDREFASSIGN(p)) {}
	~AutoComRel() { release(); }

	void release() { FINALRELEASE<T>(&_p); }
	BOOL attach(T* p) {
		release();
		if (!p) return FALSE;
		_p = p;
		return TRUE;
	}
	BOOL attach(T** pp) {
		if (!attach(*pp)) return FALSE;
		*pp = NULL;
		return TRUE;
	}
	T* detach() {
		T* p = _p;
		_p = NULL;
		return p;
	}
	operator T*() const { return _p; }
	T** operator &() { return &_p; }
	_ComRef<T>* operator->() const throw() {
		return (_ComRef<T>*)_p;
	}
	T* _p;
};


class _FTComLock
{
public:
	_FTComLock() :
#ifdef SUPPORT_FREE_THREADING_COM
		_locks(0), _mutex(NULL),
#endif//#ifdef SUPPORT_FREE_THREADING_COM
		_ref(1)
	{
		LIB_ADDREF;
#ifdef SUPPORT_FREE_THREADING_COM
		_mutex = ::CreateMutex(0, FALSE, 0);
#endif//#ifdef SUPPORT_FREE_THREADING_COM
	}
	virtual ~_FTComLock()
	{
		ASSERT(_ref == 0);
#ifdef SUPPORT_FREE_THREADING_COM
		HANDLE h = InterlockedExchangePointer(&_mutex, NULL);
		if (h)
			::CloseHandle(h);
#endif//#ifdef SUPPORT_FREE_THREADING_COM
		LIB_RELEASE;
	}

	LONG Lock()
	{
#ifdef SUPPORT_FREE_THREADING_COM
		DWORD res = ::WaitForSingleObject(_mutex, INFINITE);
		ASSERT(res == WAIT_OBJECT_0);
		LONG c = InterlockedIncrement(&_locks);
		return c;
#else//#ifdef SUPPORT_FREE_THREADING_COM
		return 1;
#endif//#ifdef SUPPORT_FREE_THREADING_COM
	}
	LONG Unlock()
	{
#ifdef SUPPORT_FREE_THREADING_COM
		LONG c = InterlockedDecrement(&_locks);
		::ReleaseMutex(_mutex);
		return c;
#else//#ifdef SUPPORT_FREE_THREADING_COM
		return 0;
#endif//#ifdef SUPPORT_FREE_THREADING_COM
	}

protected:
	ULONG _ref;
#ifdef SUPPORT_FREE_THREADING_COM
	LONG _locks;
	HANDLE _mutex;
#endif//#ifdef SUPPORT_FREE_THREADING_COM
};

class _AutoComLock
{
public:
	_AutoComLock(_FTComLock *p) : _p(p)
	{
		_p->Lock();
	}
	~_AutoComLock()
	{
		_p->Unlock();
	}
	_FTComLock *_p;
};

template <class T, const IID* piid>
class IUnknownImpl : public _FTComLock, public T
{
public:
	IUnknownImpl() {}

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv)
	{
		AddRef();
		if (IsEqualIID(riid, IID_IUnknown))
		{
			*ppv = (LPUNKNOWN)(T*)this;
			return S_OK;
		}
		if (IsEqualIID(riid, *piid))
		{
			*ppv = dynamic_cast<T*>(this);
			return S_OK;
		}
		Release();
		return E_NOINTERFACE;
	}
	STDMETHOD_(ULONG, AddRef)()
	{
		Lock();
		ULONG c = InterlockedIncrement(&_ref);
		Unlock();
		return c;
	}
	STDMETHOD_(ULONG, Release)()
	{
		Lock();
		ULONG c = InterlockedDecrement(&_ref);
		Unlock();
		if (c == 0)
			delete this;
		return c;
	}
};

template <class T, const IID* piid, class T2, const IID* piid2>
class IUnknownImpl2 : public IUnknownImpl<T, piid>
{
public:
	IUnknownImpl2(T2 *t2_) : _t2(t2_) {}

	STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv)
	{
		if (S_OK == IUnknownImpl<T, piid>::QueryInterface(riid, ppv))
			return S_OK;
		if (IsEqualIID(riid, *piid2))
			return _t2._p->QueryInterface(riid, ppv);
		return E_NOINTERFACE;
	}

protected:
	AutoComRel<T2> _t2;
};


////////////////////////////////////////////////////////////////////////
// IClassFactory implementations

class NO_VTABLE IClassFactoryImpl : public _FTComLock, public IClassFactory
{
public:
	IClassFactoryImpl()
	{
		LIB_ADDREF;
		_ref = 1;
	}
	virtual ~IClassFactoryImpl()
	{
		LIB_RELEASE;
	}

	// IUnknown methods
	STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppv)
	{
		HRESULT hr = NOERROR;
		Lock();
		if (_ref == 0)
		{
			Unlock();
			return E_NOINTERFACE; // called from destructor?
		}
		if (GetInterface(riid, ppv))
		{
			_ref++;
			Unlock();
		}
		else
		{
			Unlock();
			//DBGPRINTF(( " riid=%X-%X-%X\n", riid.Data1, (int)riid.Data2, (int)riid.Data3 ));
			*ppv = NULL;
			hr = E_NOINTERFACE;
		}
		return hr;
	}
	STDMETHOD_(ULONG, AddRef)()
	{
		Lock();
		ULONG c = InterlockedIncrement(&_ref);
		Unlock();
		return c;
	}
	STDMETHOD_(ULONG, Release)()
	{
		Lock();
		ULONG c = InterlockedDecrement(&_ref);
		Unlock();
		if (c)
			return c;
		delete this;
		return 0;
	}
	// IClassFactory methods
	STDMETHOD(CreateInstance)(LPUNKNOWN punk, REFIID riid, void** ppv)
	{
		return E_NOTIMPL;
	}
	STDMETHOD(LockServer)(BOOL fLock)
	{
		if (fLock) LIB_LOCK;
		else LIB_UNLOCK;
		return S_OK;
	}

protected:
	virtual BOOL GetInterface(REFIID riid, LPVOID* ppv)
	{
		if (IsEqualIID(riid, IID_IUnknown))
		{
			*ppv = (LPUNKNOWN)this;
			return TRUE;
		}
		else if (IsEqualIID(riid, IID_IClassFactory))
		{
			*ppv = (LPCLASSFACTORY)this;
			return TRUE;
		}
		return FALSE;
	}
};

template <class T>
class NO_VTABLE IClassFactoryNoAggrImpl : public IClassFactoryImpl
{
public:
	IClassFactoryNoAggrImpl() {}

	// IClassFactory methods
	STDMETHOD(CreateInstance)(LPUNKNOWN punk, REFIID riid, void** ppv)
	{
		if (NULL == ppv) return E_POINTER;
		*ppv = NULL;
		if (NULL != punk)
			return CLASS_E_NOAGGREGATION;
		return CreateInstanceInternal(riid, ppv);
	}

protected:
	virtual HRESULT CreateInstanceInternal(REFIID riid, void** ppv)
	{
		T* pObj = new T;
		HRESULT hr = E_OUTOFMEMORY;
		if (pObj)
		{
			hr = pObj->QueryInterface(riid, ppv);
			pObj->Release();
		}
		return hr;
	}
};

