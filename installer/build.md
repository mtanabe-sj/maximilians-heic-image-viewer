# HEICVIEWER Installer

## PURPOSE

Build a self-extracting setup program that bundles both the x64 and x86 msi's and can install `HEICVIEWER` on either x64 or x86 Windows system.


## REQUIREMENTS

- Microsoft Windows 10 (x86 or x64)
- Microsoft Visual Studio 2017
- Windows SDK 10.0.17763.0


## BUILD TOOLS AND FOLDERS

- `CleanAndBuildAll.bat`: a master batch file to build the 64- and 32-bit program and msi files. It runs msbuild on the heicviewer solution to build the viewer and thumbnail executables. It runs build.bat to build the msi. It builds the setup program, and binds the msi to it. It places the completed setup executable in the out folder.
- `build.bat`: a slave batch file to manage and stage the msi build. The process generates a 32-bit or 64-bit fully functional msi based on the template msi. It runs MakeMsi to merge product info with the msi, and import the program and other installation files into it.
- `MakeMsi.js`: a script for laying out the installation files, and populating the MSI tables.
- `template.msi`: an incomplete MSI containing minimum amounts of installation logic and table data. It is used by the build process to generate a fully functional MSI.
- `data`: a subfolder with data files that are imported into the MSI. They are installer image bitmaps, release notes and a software license agreement.
- `work`: a subfolder where the installer build is staged. The folder is recreated at the start of a new build process.
- `out`: this is the output folder that, after a build process, contains the completed MSI. It also contains a log file that the build process generates. If there is a build issue, use it to troubleshoot.

The build process uses the Microsoft tools shown below. Make sure they exist.

```
Tools      Env var   File location
=========  ========  ====================================
msidb      PSDKBIN   %ProgramFiles(x86)%\Windows Kits\10\bin\10.0.17763.0\x86
msiinfo
uuidgen
signtool
---------  --------  ------------------------------------
msbuild    VCDIR     %ProgramFiles(x86)%\Microsoft Visual Studio\2017\Professional\MSBuild\15.0
```

## BUILD PROCESS

Follow these steps to build the setup program:

- Open installer\build.bat in Notepad. Locate the following SET statement shown below. ProjectSource is the base folder of the product's source tree. Correct the assigned value of path if your project configuration differs.
> @set ProjectSource=<directory>\heicviewer

- Update heicviewer\viewheic\appver.h with the latest product info and build number. Skip this step if there is no change.

- Run installer\CleanAndBuildAll.bat. It builds the setup. It is not necessary to use an admin account to run the batch process.

The build process uses the 'installer\work' folder to hold temporary data files. The process outputs the built installer to 'installer\out'.


## SIGNING THE FILES

- For your local build purposes, you can generate test certs yourself using Windows SDK tools. Here is a possible command procedure.

```Batchfile
set PSDKBIN=%ProgramFiles(x86)%\Windows Kits\10\bin\x86"
set CA_CERT=TestCA
set CA_PSWD=hard2crack
set CERT_STORE=TestCerts
set SIGNING_CERT=TestCert
rem Make a root CA.
"%PSDKBIN%\makecert" /$ individual /r /pe /a sha256 /sv %CA_CERT%.pvk /n "CN=%CA_CERT%" %CA_CERT%.cer
rem Install the new CA in the root CA store.
certutil -addstore ROOT %CA_CERT%.cer
rem Use a password of "%CA_PSWD%" to pack the private and public keys in a .pfx file for portability.
"%PSDKBIN%\pvk2pfx" /pvk %CA_CERT%.pvk /pi "%CA_PSWD%" /spc %CA_CERT%.cer /pfx %CA_CERT%.pfx /f
rem IMPORTANT: this part must be run as a local admin.
rem Make a code signing cert named '%SIGNING_CERT%', issued by the CA just created.
"%PSDKBIN%\makecert" -a sha256 -is root -in "%CA_CERT%" -ss %CERT_STORE% -n "CN=%SIGNING_CERT%" -eku 1.3.6.1.5.5.7.3.3
```

- Test-sign an app using the above code signing cert.

```Batchfile
set APPTOSIGN=<path\>testapp.exe
"%PSDKBIN%\signtool" sign /v /s "%CERT_STORE%" /n "%SIGNING_CERT%" /t http://timestamp.comodoca.com/authenticode "%APPTOSIGN%"
```

- Define Cert Variables. Make sure that the two environment variables `CERT_STORE` and `SIGNING_CERT` are defined before running CleanAndBuild.bat or build.bat.


July 2022, mtanabe
