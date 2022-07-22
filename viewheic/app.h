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


// viewer option flags for ViewFlags in registry key [HKCR\SOFTWARE\mtanabe\heicviewer].
#define VIEWHEIC_SAVE_NO_EXIF 0x00000001
#define VIEWHEIC_ALWAYS_CENTER_TOOLBAR 0x00010000
#define VIEWHEIC_TEXT_LABEL_ON_TOOLBAR 0x00020000

// subkey of the app, applicable to both HKLM and HLCR
#define APPREGKEY L"SOFTWARE\\mtanabe\\heicviewer"

// file containing libheic's license text.
#define LIBHEIFLICENCE_FILENAME L"heif_lic.txt"

