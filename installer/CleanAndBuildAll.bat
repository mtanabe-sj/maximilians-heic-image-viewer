rem  Copyright (c) 2022 Makoto Tanabe <mtanabe.sj@outlook.com>
rem  Licensed under the MIT License.
rem
rem A signing cert is a prerequisite. %SIGNING_CERT% must exist in %CERT_STORE%. Check it with CertMgr.
if "%SIGNING_CERT%"=="" exit /b 7

set PSDKBIN=%ProgramFiles(x86)%\Windows Kits\10\bin\10.0.17763.0\x86
set VCDIR=%ProgramFiles(x86)%\Microsoft Visual Studio\2017\Professional\MSBuild\15.0
set ProjectSource=c:\src\projects\cpp\shell\heicviewer
rd /s/q %ProjectSource%\release
rd /s/q %ProjectSource%\x64\release
rd /s/q %ProjectSource%\viewheic\release
rd /s/q %ProjectSource%\viewheic\x64
rd /s/q %ProjectSource%\thumheic\release
rd /s/q %ProjectSource%\thumheic\x64
rd /s/q %ProjectSource%\setup\release
rd /s/q %ProjectSource%\setup\x64

"%VCDIR%\bin\amd64\msbuild.exe" %ProjectSource%\heicviewer.sln /property:Configuration=Release /property:Platform=x64
@if errorlevel 1 goto ExitBatch
"%VCDIR%\bin\amd64\msbuild.exe" %ProjectSource%\heicviewer.sln /property:Configuration=Release /property:Platform=x86
@if errorlevel 1 goto ExitBatch

call %ProjectSource%\installer\build.bat x86
@if errorlevel 1 goto ExitBatch
copy %ProjectSource%\installer\MakeMsi.js.log %ProjectSource%\installer\out\MakeMsi86.log
call %ProjectSource%\installer\build.bat x64
@if errorlevel 1 goto ExitBatch
copy %ProjectSource%\installer\MakeMsi.js.log %ProjectSource%\installer\out\MakeMsi64.log

"%VCDIR%\bin\amd64\msbuild.exe" %ProjectSource%\setup.sln /property:Configuration=Release /property:Platform=x86
@if errorlevel 1 goto ExitBatch

"%PSDKBIN%\signtool" sign /v /s "%CERT_STORE%" /n "%SIGNING_CERT%" /t http://timestamp.comodoca.com/authenticode "%ProjectSource%\installer\out\heicviewer_setup.exe"

copy %ProjectSource%\installer\out\heicviewer64.msi c:\src\projects\installers\
copy %ProjectSource%\installer\out\heicviewer86.msi c:\src\projects\installers\
copy %ProjectSource%\installer\out\heicviewer_setup.exe c:\src\projects\installers\

:exitBatch
@echo CleanAndBuildAll stopped
@time /t
