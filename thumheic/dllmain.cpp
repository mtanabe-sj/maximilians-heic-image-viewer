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
// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include <initguid.h>
#include "ThumbnailProviderImpl.h"


// thumbnail provider
#define WSZ_CLSID_MyShellExtension L"{068DA725-811E-4091-8E57-36295570C565}"
DEFINE_GUID(CLSID_MyShellExtension, 0x068DA725, 0x811E, 0x4091, 0x8E, 0x57, 0x36, 0x29, 0x55, 0x70, 0xC5, 0x65);
#define WSZ_NAME_MyShellExtension L"Maximilian's HEIC Thumbnail Provider"


#ifdef APP_WRITES_EVENTLOG
EventLog evlog;
#endif//#ifdef APP_WRITES_EVENTLOG

ULONG LibRefCount = 0;
HMODULE LibInstanceHandle = NULL;


BOOL APIENTRY DllMain(HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved)
{
	switch (ul_reason_for_call)
	{
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hModule);
		LibInstanceHandle = hModule;
#ifdef APP_WRITES_EVENTLOG
		evlog.init();
#endif//#ifdef APP_WRITES_EVENTLOG
		break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	}
	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// Special entry points required for inproc servers

STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv)
{
#ifdef _DEBUGx
	MessageBox(NULL, L"DllGetClassObject", L"THUMHEIC", MB_SYSTEMMODAL|MB_OK);
#endif//#ifdef _DEBUG
	*ppv = NULL;
	IClassFactory *pClassFactory;
	// propertysheet handlers
	if (IsEqualCLSID(rclsid, CLSID_MyShellExtension))
		pClassFactory = new IClassFactoryNoAggrImpl<ThumbnailProviderImpl>;
	else
		return CLASS_E_CLASSNOTAVAILABLE;
	if (pClassFactory == NULL)
		return E_OUTOFMEMORY;
	HRESULT hr = pClassFactory->QueryInterface(riid, ppv);
	pClassFactory->Release();
	return hr;
}

STDAPI DllCanUnloadNow(void)
{
	return (LibRefCount == 0) ? S_OK : S_FALSE;
}

DWORD DllGetVersion(void)
{
	return MAKELONG(1, 0);
}

STDAPI DllRegisterServer(void)
{
#ifdef APP_WRITES_EVENTLOG
	evlog.init("HeicviewerSetup.log", EVENTLOG_FLAGS_CANLOG);
#endif//#ifdef APP_WRITES_EVENTLOG
	long res;
	wstring curVal;
	res = Registry_GetStringValue(HKEY_CLASSES_ROOT,
		L".heic\\ShellEx\\{E357FCCD-A995-4576-B01F-234630154E96}",
		NULL,
		curVal);
	DBGPRINTF((L"DRS1> Registry_GetStringValue(HKCR\\.heic\\ShellEx\\{E357FCCD}): RES=%d; (Default)='%s'", res, curVal.c_str()));
	if (res == ERROR_SUCCESS)
	{
		if (_wcsicmp(curVal.c_str(), WSZ_CLSID_MyShellExtension) != 0)
		{
			// current value is typically "{C7657C4A-9F68-40fa-A4DF-96BC08EB3551}" which is the clsid of microsoft's 'Photo Thumbnail Provider'.
			res = Registry_CreateNameValue(HKEY_LOCAL_MACHINE,
				LIBREGKEY,
				L"PriorShellEx",
				curVal.c_str());
			DBGPRINTF((L"DRS1b> Registry_CreateNameValue(HKLM\\%s, PriorShellEx=%s): RES=%d", LIBREGKEY, curVal.c_str(), res));
			if (res != ERROR_SUCCESS)
				return HRESULT_FROM_WIN32(res);
		}
	}

	res = Registry_CreateComClsidKey(WSZ_CLSID_MyShellExtension, WSZ_NAME_MyShellExtension);
	DBGPRINTF((L"DRS2> Registry_CreateComClsidKey(%s, '%s'): RES=%d", WSZ_CLSID_MyShellExtension, WSZ_NAME_MyShellExtension, res));
	if (res != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(res);

	res = Registry_CreateNameValue(HKEY_CLASSES_ROOT,
		L".heic\\ShellEx\\{E357FCCD-A995-4576-B01F-234630154E96}",
		NULL,
		WSZ_CLSID_MyShellExtension);
	DBGPRINTF((L"DRS3> Registry_CreateNameValue(HKCR\\.heic\\ShellEx\\{E357FCCD}, %s): RES=%d", WSZ_CLSID_MyShellExtension, res));
	if (res != ERROR_SUCCESS)
		return HRESULT_FROM_WIN32(res);

	res = Registry_CreateNameValue(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved",
		WSZ_CLSID_MyShellExtension,
		WSZ_NAME_MyShellExtension);
	DBGPRINTF((L"DRS4> Registry_CreateNameValue(HKLM\\..\\Approved\\%s, '%s'): RES=%d", WSZ_CLSID_MyShellExtension, WSZ_NAME_MyShellExtension, res));

	SHChangeNotify(SHCNE_ASSOCCHANGED, SHCNF_IDLIST, NULL, NULL);
	return S_OK;
}

STDAPI DllUnregisterServer(void)
{
#ifdef APP_WRITES_EVENTLOG
	evlog.init("HeicviewerSetup.log", EVENTLOG_FLAGS_CANLOG);
#endif//#ifdef APP_WRITES_EVENTLOG
	long res;
	wstring priorVal;
	res = Registry_GetStringValue(HKEY_LOCAL_MACHINE,
		LIBREGKEY,
		L"PriorShellEx",
		priorVal);
	DBGPRINTF((L"DUS1> Registry_GetStringValue(HKLM\\%s): RES=%d; PriorShellEx='%s'", LIBREGKEY, res, priorVal.c_str()));
	if (res == ERROR_SUCCESS && priorVal.length() > 0 && _wcsicmp(priorVal.c_str(), WSZ_CLSID_MyShellExtension) != 0)
	{
		res = Registry_CreateNameValue(HKEY_CLASSES_ROOT,
			L".heic\\ShellEx\\{E357FCCD-A995-4576-B01F-234630154E96}",
			NULL,
			priorVal.c_str());
		DBGPRINTF((L"DUS1a> Registry_CreateNameValue(HKCR\\.heic\\ShellEx\\{E357FCCD}, %s): RES=%d", priorVal.c_str(), res));
	}
	else
	{
		res = Registry_DeleteSubkey(HKEY_CLASSES_ROOT,
			L".heic\\ShellEx",
			L"{E357FCCD-A995-4576-B01F-234630154E96}");
		DBGPRINTF((L"DUS1b> Registry_DeleteSubkey(HKCR\\.heic\\ShellEx, {E357FCCD}): RES=%d", res));
	}

	res = Registry_DeleteValue(HKEY_LOCAL_MACHINE,
		L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Shell Extensions\\Approved",
		WSZ_CLSID_MyShellExtension);
	DBGPRINTF((L"DUS2> Registry_DeleteValue(HKLM\\Approved, %s): RES=%d", WSZ_CLSID_MyShellExtension, res));

	res = Registry_DeleteSubkey(HKEY_CLASSES_ROOT,
		L"CLSID",
		WSZ_CLSID_MyShellExtension);
	DBGPRINTF((L"DUS3> Registry_DeleteSubkey(HKCR\\CLSID, %s): RES=%d", WSZ_CLSID_MyShellExtension, res));

	return S_OK;
}

