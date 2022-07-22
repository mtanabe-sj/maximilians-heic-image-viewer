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
/* the setup project builds only in 32-bit. there is no 64-bit edition. the same 32-bit executable supports running of a 64-bit MSI installation. the point of that is to use a single exe to support both 64- and 32-bit installs. although the setup project generates a single 32-bit exe, the exe itself embeds one 32-bit MSI and one 64-bit MSI as resources. when run, setup extracts one of the MSIs that matches the system it runs on to a temp file, and executes it from there.
*/

#include "resource.h"
#include "helper.h"


#define SETUP_MSI64_NAME L"heicviewer64.msi"
#define SETUP_MSI86_NAME L"heicviewer86.msi"
#define SETUP_APP_NAME L"heicviewer_setup.exe"

#define SETUP_MSI_LOG L"MaxsHeicviewerMsi.log"
#define SETUP_MSI_COMPLETION L"MaxsHeicviewerSetupResult.txt"
#define SETUP_SETUP_LOG		L"MaxsHeicviewerSetup.log"


/* manages all aspects of the setup execution, initialization, MSI preparation, MSI execution, and setup termination. call the methods in this order: init, exportMSI, and runMSI. */
class AppSetup
{
public:
	AppSetup() : _wow64(FALSE) {}
	~AppSetup() { cleanup(); }

	virtual int init(class CommandArgs *cargs);
	int exportMSI();
	int runMSI();

protected:
	BOOL _wow64;
	EventLog _log;
	ustring _quietLevel;
	ustring _msiLogPath;
	ustring _userProps;
	ustring _msiPath;
	ustring _installerPath;

	int exportResoure(LPCWSTR resourceType, UINT datafileId, LPCWSTR path);
	void cleanup();
};

/* the subclass adds support for detecting a HEIF/HEVC codec. the init override internally calls decodeTestImage to determine if WIC comes with a working codec. */
class AppSetup2 : public AppSetup
{
public:
	AppSetup2() {}

	virtual int init(class CommandArgs *cargs);

protected:
	HRESULT decodeTestImage(bool canShow);
	HRESULT openImage(LPCWSTR fileIn, bool canShow);
	HRESULT showImage(HBITMAP hbmp);
};
