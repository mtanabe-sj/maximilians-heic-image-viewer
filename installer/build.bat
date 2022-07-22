rem  Copyright (c) 2022 Makoto Tanabe <mtanabe.sj@outlook.com>
rem  Licensed under the MIT License.
rem
rem Syntax: build [platform] [options]
rem Platforms: x86 or x64
rem Options: specify INSTALLSRC to enable the bundling of the source code with the installer.
rem 32-bit MSI Output: %ProjectSource%\installer\work\%ProjectName%86.msi
rem 64-bit MSI Output: %ProjectSource%\installer\work\%ProjectName%64.msi

@set ProjectSource=c:\src\projects\cpp\shell\heicviewer
@set ProjectName=heicviewer
@set ProjectCompany=mtanabe
@set ProductCode={84B09E9E-D6AF-480C-A93C-885F4C854008}
@set ProductName=Maximilian's HEIC Image Viewer
@set ProductDescription=This setup program installs shell integration software. It lets you preview, display and print HEIC image files from within the Windows Explorer. You need administrator privileges to install.

@if not defined PSDKBIN set PSDKBIN=%ProgramFiles(x86)%\Microsoft SDKs\Windows\v7.1A\Bin
@if not defined ProgramFiles(x86) goto initializeCompilerPaths86
@if not defined VCDIR set VCDIR=%ProgramFiles(x86)%\Microsoft Visual Studio 9.0\VC
@if not defined VCSDKBIN set VCSDKBIN=%ProgramFiles(x86)%\\Microsoft Visual Studio 9.0\SDK\v3.5\Bin
goto startBuild
:initializeCompilerPaths86
@if not defined VCDIR set VCDIR=%ProgramFiles%\Microsoft Visual Studio 9.0\VC
@if not defined VCSDKBIN set VCSDKBIN=%ProgramFiles%\Microsoft Visual Studio 9.0\SDK\v3.5\Bin

:startBuild
cd %ProjectSource%
rd /s/q %ProjectSource%\installer\work
md %ProjectSource%\installer\out

set MTPLATFORM=x64
if "%1"=="x64" (
"%PSDKBIN%\signtool.exe" sign /v /s "TestCerts" /n "MTanabeDev" /t http://timestamp.comodoca.com/authenticode  .\x64\release\thumheic.dll
@if errorlevel 1 goto exitBatch
"%PSDKBIN%\signtool.exe" sign /v /s "TestCerts" /n "MTanabeDev" /t http://timestamp.comodoca.com/authenticode  .\x64\release\viewheic.exe
@if errorlevel 1 goto exitBatch
goto runMakeMsi
)
if not "%1"=="x86" (
@echo *** BUILD ABORTED. PLATFORM TYPE IS NOT SPECIFIED ***
goto exitBatch
)
"%PSDKBIN%\signtool.exe" sign /v /s "TestCerts" /n "MTanabeDev" /t http://timestamp.comodoca.com/authenticode  .\release\thumheic.dll
@if errorlevel 1 goto exitBatch
"%PSDKBIN%\signtool.exe" sign /v /s "TestCerts" /n "MTanabeDev" /t http://timestamp.comodoca.com/authenticode  .\release\viewheic.exe
@if errorlevel 1 goto exitBatch
set MTPLATFORM=x86

:runMakeMsi
@echo _____________________________________________ MakeMsi
wscript %ProjectSource%\installer\MakeMsi.js %MTPLATFORM%
@echo MakeMsi Result: %errorlevel%
@if errorlevel 1 goto exitBatch

@echo _____________________________________________ AssignPackageCode
cd %ProjectSource%\installer\work
"%PSDKBIN%\uuidgen.exe"  -c > tempuuid.txt
set /p UUIDVar= <tempuuid.txt
set UUIDVarNew= {%UUIDVar%}
@echo PACKAGECODE: %UUIDVarNew%
"%PSDKBIN%\msiinfo" %ProjectName%.msi -v %UUIDVarNew%
del tempuuid.txt
@if errorlevel 1 goto exitBatch

rem orignal MSI should have set Platform to 'Intel'
if %MTPLATFORM%==x64 "%PSDKBIN%\msiinfo.exe" %ProjectName%.msi /p x64;1033

@echo _____________________________________________ GenerateCab
cd %ProjectSource%\installer\work
DEL programs.cab
MAKECAB /F programs.ddf
DEL setup.inf
DEL setup.rpt
DEL programs.ddf

@echo _____________________________________________ EmbedCab
cd %ProjectSource%\installer\work
"%PSDKBIN%\msidb.exe" -d %ProjectName%.msi -a programs.cab
@if errorlevel 1 goto exitBatch
DEL programs.cab

@echo _____________________________________________ SignMsi
"%PSDKBIN%\signtool.exe" sign /v /s "TestCerts" /n "MTanabeDev" /t http://timestamp.comodoca.com/authenticode %ProjectName%.msi
@if errorlevel 1 goto exitBatch

@echo _____________________________________________ MsiSummary

if %MTPLATFORM%==x64 (
del %ProjectName%64.msi
ren %ProjectName%.msi %ProjectName%64.msi
"%PSDKBIN%\msiinfo.exe" %ProjectName%64.msi
copy %ProjectName%64.msi %ProjectSource%\installer\out
) else (
del %ProjectName%86.msi
ren %ProjectName%.msi %ProjectName%86.msi
"%PSDKBIN%\msiinfo.exe" %ProjectName%86.msi
copy %ProjectName%86.msi %ProjectSource%\installer\out
)
del *.msi

:exitBatch
@set buildErrorLevel=%errorlevel%
cd %ProjectSource%\installer
@echo _____________________________________________ Finish (%buildErrorLevel%)
@exit /b %buildErrorLevel%

