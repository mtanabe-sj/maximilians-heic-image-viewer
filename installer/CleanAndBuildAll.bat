rem  Copyright (c) 2022 Makoto Tanabe <mtanabe.sj@outlook.com>
rem  Licensed under the MIT License.
rem
set VCDIR=C:\Program Files (x86)\Microsoft Visual Studio\2017\Professional\MSBuild\15.0
set BASESRCDIR=\src\projects\cpp\shell\heicviewer
rd /s/q %BASESRCDIR%\release
rd /s/q %BASESRCDIR%\x64\release
rd /s/q %BASESRCDIR%\viewheic\release
rd /s/q %BASESRCDIR%\viewheic\x64
rd /s/q %BASESRCDIR%\thumheic\release
rd /s/q %BASESRCDIR%\thumheic\x64
rd /s/q %BASESRCDIR%\setup\release
rd /s/q %BASESRCDIR%\setup\x64

"%VCDIR%\bin\amd64\msbuild.exe" %BASESRCDIR%\heicviewer.sln /property:Configuration=Release /property:Platform=x64
@if errorlevel 1 goto ExitBatch
"%VCDIR%\bin\amd64\msbuild.exe" %BASESRCDIR%\heicviewer.sln /property:Configuration=Release /property:Platform=x86
@if errorlevel 1 goto ExitBatch

call %BASESRCDIR%\installer\build.bat x86
@if errorlevel 1 goto ExitBatch
copy %BASESRCDIR%\installer\MakeMsi.js.log %BASESRCDIR%\installer\out\MakeMsi86.log
call %BASESRCDIR%\installer\build.bat x64
@if errorlevel 1 goto ExitBatch
copy %BASESRCDIR%\installer\MakeMsi.js.log %BASESRCDIR%\installer\out\MakeMsi64.log

"%VCDIR%\bin\amd64\msbuild.exe" %BASESRCDIR%\setup.sln /property:Configuration=Release /property:Platform=x86
@if errorlevel 1 goto ExitBatch

"%PSDKBIN%\signtool" sign /v /s "TestCerts" /n "MTanabeDev" /t http://timestamp.comodoca.com/authenticode "%BASESRCDIR%\installer\out\heicviewer_setup.exe"

copy %BASESRCDIR%\installer\out\heicviewer64.msi c:\src\projects\installers\
copy %BASESRCDIR%\installer\out\heicviewer86.msi c:\src\projects\installers\
copy %BASESRCDIR%\installer\out\heicviewer_setup.exe c:\src\projects\installers\

:exitBatch
@echo CleanAndBuildAll stopped
@time /t
