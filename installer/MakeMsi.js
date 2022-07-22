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
/******************************************************
 USAGE: CD to the installer folder, and run build.bat.
   The batch file internally runs MakeMsi.js.

 MakeMsi writes events to log file, installer\MakeMsi.js.log.
 if you get an error, refer to the log. it might give you a clue...
'******************************************************/

/******************************************************
 COMMAND LINE PARAMETERS

 Arguments(0) - [required] platform type of x86 or x64

******************************************************
 ENVIRONMENT VARIABLES AS ADDITIONAL INPUT PARAMETERS

 %ProjectSource% - pathname of the project home folder.
 %ProjectCompany% - name of a base folder in C:\Program Files.
 %ProjectName% - name of a product folder in %ProjectCompany%.
 %ProductCode% - GUID to set MSI ProductCode property to.
 %ProductName% - product name
 %ProductDescription% - product description

 Installation path: %ProgramFiles%\%ProjectCompany%\%ProjectName%

 if %ProjectCompany% is XyzCo, and %ProjectName% is shellextabc, the installer copies
 installation files (files of FileList below) to the folder,
   C:\Program Files\XyzCo\shellextabc.
 %ProductCode% is unique to a product.
*/

/******************************************************
 CONFIGURATION PARAMETERS

FileList - a list of program file definitions. Each definition consists of these source file attributes.
  <Filename>,<ComponentName>,<x86-source-folder1>,<x64-source-folder1>,<1=self-registering-dll;0=no>
The definitions are separated by simicolons.
All files of FileList are bundled to a single Cabinet file (programs.cab).
All files of FileList belong to the MSI feature, 'programs'. See the definition of programsFeatureName below.
*/
var FileList =
 "viewheic.exe,CoreProgram,release,x64\\release,0;"+
 "thumheic.dll,CoreLibrary,release,x64\\release,1;"+
 "heif.dll,SupportLibrary,lib\\x86,lib\\x64,0;"+
 "libde265.dll,SupportLibrary,lib\\x86,lib\\x64,0;"+
 "libx265.dll,SupportLibrary,lib\\x86,lib\\x64,0;"+
 "heif_lic.txt,SupportLibrary,lib,lib,0;"+
 "lib_stub.txt,SupportLibrary,lib,lib,0;"+
 "thumheic.dll,WOW64CoreLibrary,release,,1;"+
 "heif.dll,WOW64SupportLibrary,lib\\x86,,0;"+
 "libde265.dll,WOW64SupportLibrary,lib\\x86,,0;"+
 "libx265.dll,WOW64SupportLibrary,lib\\x86,,0;"+
 "lib_stub.txt,WOW64SupportLibrary,lib,,0";

/* ComponentList - a list of component definitions. Each definition consists of these fields.
  <ComponentName>,<ComponentID>,<32-bitCondition>,<64-bitCondition>
The definitions are separated by simicolons.
 1) CoreProgram - program files
 2) CoreLibrary - library files
 3) SupportLibrary - DLLs from 3rd parties
 4) WOW64CoreLibrary - library files for WOW64
 5) WOW64SupportLibrary - DLLs from 3rd parties for use in WOW64
*/
var ComponentList =
 "CoreProgram,{317AC085-C3C1-44D8-8BAB-A89E034452D1},Privileged,Privileged;"+
 "CoreLibrary,{317AC086-C3C1-44D8-8BAB-A89E034452D1},Privileged,Privileged;"+
 "SupportLibrary,{9E49AF1A-E36B-4E09-A172-DE6B67A1E260},Privileged,Privileged;"+
 "WOW64CoreLibrary,{317AC086-C3C2-44D8-8BAB-A89E034452D1},Privileged,Privileged;"+
 "WOW64SupportLibrary,{640E5D64-655A-45AD-ADD6-63BC8D3F2E16},Privileged,Privileged";

/******************************************************
 MSI-DEPENDENT DEFINITIONS

 name of a MSI Directory table entry for the installation folder where the program files of FileList will be installed.
*/
var programDirectoryEntryName = "MTPROGDIR";
var programDirectoryEntryNameWow64 = "WOW64MTPROGDIR";
// this header file contains #define's for LIB_VERMAJOR and other pieces of product version info.
// the header file must be located in %ProjectSource%.
var versionHeaderName = "viewheic\\appver.h";
// name of a cab to generate that consists of archived program files of FileList.
var cabName = "programs";
// name of a MSI Feature table entry that is associated with the components of ComponentList.
var programsFeatureName = "programs";
// maximum number of bytes a data file can contain. The data files accessed by this script are license.rtf and readme.rtf.
// the upper limit depends on the ScrollableText control that MSI uses to manage RTF text data.
var MaxDataFileSize = 100000;

//******************************************************

msiOpenDatabaseModeReadOnly = 0;
msiOpenDatabaseModeTransact = 1;
msiViewModifyUpdate = 2;
msiViewModifyDelete = 6;
msiReadStreamAnsi = 2;

var fso = new ActiveXObject("Scripting.FileSystemObject");
var wsh = new ActiveXObject("WScript.Shell");
var vinfo = new ActiveXObject("MaxsUtilLib.VersionInfo");

var PlatformId = "x86";
if(WScript.arguments.length > 0)
  PlatformId = WScript.arguments(0) // "x86" or "x64"

var platform64 = false;
if (PlatformId == "x64")
  platform64 = true;

// path to the code project folder that contains 'installer' and the file of versionHeaderName.
var basePath = wsh.ExpandEnvironmentStrings("%ProjectSource%") + "\\";
if(basePath == "%ProjectSource%\\") {
  WScript.Echo("Environment variable %ProjectSource% not found");
  WScript.Quit(2);
}
// name of the MSI database.
var ProjectName = wsh.ExpandEnvironmentStrings("%ProjectName%");
if(ProjectName == "%ProjectName%") {
  WScript.Echo("Environment variable %ProjectName% not found");
  WScript.Quit(2);
}
// company name
var ProjectCompany = wsh.ExpandEnvironmentStrings("%ProjectCompany%");
if(ProjectCompany == "") {
  WScript.Echo("Environment variable %ProjectCompany% not found");
  WScript.Quit(2);
}
// product code
var ProductCode = wsh.ExpandEnvironmentStrings("%ProductCode%");
if(ProductCode == "") {
  WScript.Echo("Environment variable %ProductCode% not found");
  WScript.Quit(2);
}
// product name (full name)
var ProductFullName = wsh.ExpandEnvironmentStrings("%ProductName%");
if(ProductFullName == "") {
  WScript.Echo("Environment variable %ProductName% not found");
  WScript.Quit(2);
}
// product description
var ProductDescription = wsh.ExpandEnvironmentStrings("%ProductDescription%");
if(ProductDescription == "") {
  WScript.Echo("Environment variable %ProductDescription% not found");
  WScript.Quit(2);
}

var homePath = basePath + "installer\\";
var workDir = "Work";
var workPath = homePath + workDir + "\\";
var dataPath = homePath + "data\\";
var ddfPath = workPath + cabName + ".ddf";
var msiPath = workPath + ProjectName + ".msi";
var versionHeaderFile = basePath + versionHeaderName;
// the folder that the program files are copied to is named after the project name.
var programFolder;
if(ProjectName.length > 8) {
  // note that the folder's shortname is fixed to heicviewer regardless of the project name.
  programFolder = ProjectName.substr(0,8) + "|" + ProjectName;
} else {
  programFolder = ProjectName;
}
// this tracks the LastSequence value which we will need for the Media table.
var fSequence = 0;

// start an event log in case we need to troubleshoot.
var log = new eventLog;
log.write(WScript.ScriptName + " Started");
log.write("MSI PATH", msiPath);
log.write("ProjectName", ProjectName);
log.write("ProjectCompany", ProjectCompany);
log.write("ProductCode", ProductCode);
log.write("VersionHeader", versionHeaderFile);
log.write("Target Platform", PlatformId);
log.write("Platform64", platform64.toString());

ensureFolder(homePath + workDir);

var componentArray = ComponentList.split(";");
var fileArray = FileList.split(";");

for(var i = 0; i < componentArray.length; i++) {
	log.write("componentArray-"+i, componentArray[i]);
}
for(var i = 0; i < fileArray.length; i++) {
	log.write("fileArray-"+i, fileArray[i]);
}

// generate a timestamp from current date time. That's the modification time for all files we install.
var tNow = new Date;
var dateStr = getDateString(tNow);
var timeStr = getTimeString(tNow);

copyFileAlways(homePath+"template.msi", msiPath);

log.write("Creating DDF", ddfPath);
var ddf = fso.CreateTextFile(ddfPath, true);
ddf.WriteLine(".Set DiskDirectory1="+homePath+workDir);
ddf.WriteLine(".Set CabinetName1="+cabName+".cab");
ddf.WriteLine(".Set UniqueFiles=OFF");
ddf.WriteLine(".Set Cabinet=ON");
ddf.WriteLine(".Set Compress=ON");
ddf.WriteLine(".Set MaxDiskSize=0");
for(var i = 0; i < fileArray.length; i++) {
  var fdesc = fileArray[i].split(",");
  log.write(i.toString()+") "+fdesc[0], fdesc[1]);
  addFileToDDF(ddf, fdesc, platform64, dateStr, timeStr);
}
ddf.Close();

var installer = new ActiveXObject("WindowsInstaller.Installer");
var msi = installer.OpenDatabase(msiPath, msiOpenDatabaseModeTransact);

if(!platform64)
  removeMsiFeatureEntry(msi, "WOW64"+programsFeatureName);

if(platform64) {
  log.write("UPDATING X86 ENTRY", "DIRECTORY");
  updateMsiDirectoryEntry(msi, "ProgramFilesFolder", "DefaultDir", "progra~2|Program Files (x86)");
  updateMsiDirectoryEntry(msi, "BASEINSTALLDIR", "Directory_Parent", "ProgramFiles64Folder");
  log.write("UPDATING X64 ENTRIES", "DIRECTORY");
  updateMsiDirectoryEntry(msi, "WOW64BASEINSTALLDIR", "DefaultDir", ProjectCompany);
  updateMsiDirectoryEntry(msi, "BASEINSTALLDIR", "DefaultDir", ProjectCompany);
  updateMsiDirectoryEntry(msi, programDirectoryEntryNameWow64, "DefaultDir", programFolder);
  updateMsiDirectoryEntry(msi, programDirectoryEntryName, "DefaultDir", programFolder);
} else {
  log.write("REMOVING X64 ENTRY", "DIRECTORY");
  removeMsiDirectoryEntry(msi, "ProgramFiles64Folder");
  removeMsiDirectoryEntry(msi, "WOW64BASEINSTALLDIR");
  removeMsiDirectoryEntry(msi, programDirectoryEntryNameWow64);
  removeMsiRegistryEntry(msi, "ProgramDirPathWow64");
  log.write("UPDATING X86 ENTRIES", "DIRECTORY");
  updateMsiDirectoryEntry(msi, "BASEINSTALLDIR", "DefaultDir", ProjectCompany);
  updateMsiDirectoryEntry(msi, programDirectoryEntryName, "DefaultDir", programFolder);
}

log.write("COLLECTING PRODUCT VERSION INFO", "PROPERTY");
var prodVerMajor = readCDefinedConstValue("APP_VERMAJOR", versionHeaderFile);
var prodVerMinor = readCDefinedConstValue("APP_VERMINOR", versionHeaderFile);
var prodRevision = readCDefinedConstValue("APP_REVISION", versionHeaderFile);
var prodBuild = readCDefinedConstValue("APP_BUILD", versionHeaderFile);
var prodVerNum = prodVerMajor*0x10000+prodVerMinor*0x100+prodRevision;
var prodVerStr = prodVerMajor.toString() + "." + prodVerMinor.toString() + "." + prodRevision.toString() + "." + prodBuild.toString();
var prodBuildDate = prodVerMajor.toString() + "." + prodVerMinor.toString() + "." + prodRevision.toString() + " b" + prodBuild.toString() + " (" + dateStr + ")";

log.write("ADDING PRODUCT PROPERTIES", "PROPERTY");
updateMsiPropertyRow(msi, "ProductName", ProductFullName);
updateMsiPropertyRow(msi, "ProductDescription", ProductDescription);
updateMsiPropertyRow(msi, "ProductVersion", prodVerStr);
updateMsiPropertyIntRow(msi, "ProductVersionNumber", prodVerNum);
updateMsiPropertyIntRow(msi, "ProductBuildNumber", prodBuild);
updateMsiPropertyRow(msi, "ProductBuildDate", prodBuildDate);
updateMsiPropertyRow(msi, "ProductCode", ProductCode);
if(platform64)
  updateMsiPropertyRow(msi, "ProductEdition", "x64");
updateMsiPropertyRow(msi, "Manufacturer", ProjectCompany);
msi.Commit();

log.write("UPDATING LICENSE/README TEXTS", "Control");
updateMsiControlText(msi, "LicenseAgreementDlg", "AgreementText", "license.rtf");
updateMsiControlText(msi, "ReadmeDlg", "ReadmeText", "readme.rtf");
msi.Commit();

// this script assumes these MSI tables are clean with not a single row existing in any of them. To make sure, delete them, and recreate.
dropMsiTable(msi, "FeatureComponents");
dropMsiTable(msi, "Component");
dropMsiTable(msi, "File");
dropMsiTable(msi, "Media");
createMsiTable(msi, "FeatureComponents", "`Feature_` CHAR(38) NOT NULL, `Component_` CHAR(72) NOT NULL PRIMARY KEY `Feature_`,`Component_`");
createMsiTable(msi, "Component", "`Component` CHAR(72) NOT NULL, `ComponentId` CHAR(38), `Directory_` CHAR(72) NOT NULL, `Attributes` INTEGER, `Condition` CHAR(255), `KeyPath` CHAR(72) PRIMARY KEY `Component`");
createMsiTable(msi, "File", "`File` CHAR(72) NOT NULL, `Component_` CHAR(72) NOT NULL, `FileName` CHAR(255) NOT NULL LOCALIZABLE, `FileSize` LONG NOT NULL, `Version` CHAR(72), `Language` CHAR(72), `Attributes` INTEGER, `Sequence` INTEGER PRIMARY KEY `File`");
createMsiTable(msi, "Media", "`DiskId` INTEGER NOT NULL, `LastSequence` INTEGER NOT NULL, `DiskPrompt` CHAR(72) LOCALIZABLE, `Cabinet` CHAR(72), `VolumeLabel` CHAR(72) LOCALIZABLE, `Source` CHAR(72) LOCALIZABLE PRIMARY KEY `DiskId`");
msi.Commit();

for(var i = 0; i < componentArray.length; i++) {
	log.write("componentArray-"+i, componentArray[i]);
  var cdesc = componentArray[i].split(",");
  if (!addMsiComponentRow(msi, cdesc, fileArray, programDirectoryEntryName, platform64)) {
    WScript.Echo("Component Table Add failed for "+cdesc[0]);
    WScript.Quit(4);
  }
	if (!addMsiFeatureComponentsRow(msi, cdesc[0], programsFeatureName, platform64)) {
    WScript.Echo("FeatureComponents Table Add failed for "+cdesc[0]);
    WScript.Quit(4);
  }
}

// create a properties row for each file and insert it into the File table
for(var i = 0; i < fileArray.length; i++) {
	log.write("fileArray-"+i, fileArray[i]);
  var fdesc = fileArray[i].split(",");
  if (!addMsiFileRow(msi, fdesc, platform64)) {
    WScript.Echo("File Table Add failed for "+fdesc[0]);
    WScript.Quit(4);
  }
}
addMsiMediaRow(msi, "1", fSequence, "programs.cab");
msi.Commit();

log.write(WScript.ScriptName + " Stopped");


/////////////////////////////////////////////////////////
/* copies a source file to be installed to the msi-build staging area, and adds the file's description (the destination pathname and current date-time) to the ddf stream.

Note that the function reads the global variables, basePath and workPath.
*/
function addFileToDDF(stream, fileDesc, x64, dateStamp, timeStamp) {
  var fLongname = fileDesc[0];
  var fComponent = fileDesc[1];
  var fDestinationName = fLongname;
  if(fComponent.substr(0,5) == "WOW64") {
    if(!x64) {
      log.write("X86 SKIPPING WOW64 FILE " + fLongname, fComponent);
      return false;
    }
    fSourcePath = basePath+fileDesc[2]+"\\"+fLongname;
    fDestinationName = "WOW64"+fLongname;
  } else if(x64) {
    fSourcePath = basePath+fileDesc[3]+"\\"+fLongname;
  } else { // 32-bit (x86)
    fSourcePath = basePath+fileDesc[2]+"\\"+fLongname;
  }
  fDestinationPath = "SrcFiles";
  ensureFolder(workPath+fDestinationPath);
  fDestinationPath = fDestinationPath+"\\"+cabName;
  ensureFolder(workPath+fDestinationPath);
  fDestinationPath = fDestinationPath+"\\"+fComponent;
  ensureFolder(workPath+fDestinationPath);
  fDestinationPath = fDestinationPath+"\\"+ProjectCompany;
  ensureFolder(workPath+fDestinationPath);
  fDestinationPath = fDestinationPath+"\\"+ProjectName;
  ensureFolder(workPath+fDestinationPath);
  fDestinationPath = fDestinationPath+"\\"+fDestinationName;
  stream.WriteLine("\""+fDestinationPath+"\" /date="+dateStamp+" /time="+timeStamp);
  fDestinationPath = workPath+fDestinationPath;
  return copyFileAlways(fSourcePath, fDestinationPath);
}

/////////////////////////////////////////////////////////
function createMsiTable(db, tableName, columns) {
	var query = "CREATE TABLE `"+tableName+"`("+columns+")";
  log.write("MSI-SQL", query);
	var view = db.OpenView(query);
	view.Execute();
}

function dropMsiTable(db, tableName) {
	var query = "DROP TABLE "+tableName;
  log.write("MSI-SQL", query);
	var view = db.OpenView(query);
	view.Execute();
}

/////////////////////////////////////////////////////////
function addMsiComponentRow(db, compDesc, fileDesc, dirName, x64) {
  var componentName = compDesc[0]; // component name
  var componentId = compDesc[1]; // component id
  var componentDir = dirName;
  var componentAttribs = 0;
  if(x64)
    componentCondition = compDesc[3]; // component condition for x64
  else
    componentCondition = compDesc[2]; // component condition for x86
  var componentKey = "";
  for(var x = 0; x < fileDesc.length; x++) {
    var fileProps = fileDesc[x].split(",");
    log.write("Finding KeyPath for Component", fileProps[1] + " <--> " + componentName);
    if(fileProps[1] == componentName) {
      componentKey = fileProps[0];
      break;
    }
  }
  if(componentName.substr(0,5) == "WOW64") {
    if(!x64) {
      log.write("X86 SKIPPING WOW64 COMPONENT", componentName);
      return true;
    }
    componentDir = "WOW64"+componentDir;
    componentKey = "WOW64"+componentKey;
  } else if(x64)
    componentAttribs = 256;

  var values = "'" + componentName + "'," +
    "'" + componentId + "'," +
    "'" + componentDir + "'," +
    componentAttribs.toString() + "," +
    "'" + componentCondition + "'," +
    "'" + componentKey + "'";
  return addMsiRecord(db, "Component", "`Component`.`Component`,`Component`.`ComponentId`,`Component`.`Directory_`,`Component`.`Attributes`,`Component`.`Condition`,`Component`.`KeyPath`", values);
}

function addMsiFeatureComponentsRow(db, componentName, featureName, x64) {
  if(componentName.substr(0,5) == "WOW64") {
    if(!x64) {
      log.write("X86 SKIPPING WOW64 COMPONENT", componentName);
      return true;
    }
    featureName = "WOW64"+featureName;
  }
  values = "'" + featureName + "'," + "'" + componentName + "'";
  return addMsiRecord(db, "FeatureComponents", "`FeatureComponents`.`Feature_`,`FeatureComponents`.`Component_`", values);
}

/* adds a file descriptor as a new row to the MSI File table.

Return value:
1 - a row was added successfully.
0 - no row was added because the input file does not belong to the platform.
-1 - no row was added because an error was encountered on table access.
*/
function addMsiFileRow(db, fileDesc, x64) {
  var fLongname = fileDesc[0];
  var fComponent = fileDesc[1];
  var fKey = fLongname;
  if(fComponent.substr(0,5) == "WOW64") {
    if(!x64) {
      log.write("X86 SKIPPING WOW64 FILE " + fKey, fComponent);
      return true;
    }
    fKey = "WOW64"+fKey;
  }
  if(fileDesc[4] == '1')
    addMsiSelfregRow(db, fKey, 1024);

  fSequence = fSequence+1;
  var fPathname = workPath+"SrcFiles\\programs\\"+fComponent+"\\"+ProjectCompany+"\\"+ProjectName+"\\"+fKey;
  log.write(fSequence.toString()+") "+fKey+" @ "+fComponent, fPathname);
  var f = fso.GetFile(fPathname);
  var fVersion = "";
  var fLang = "";
  if (isExecutableFile(f)) {
		fVersion = queryFileVersion(f);
    fLang = "0"; // language-neutral
  }
  var fName;
  if(f.ShortName != fLongname)
    fName = f.ShortName + "|" + fLongname;
  else
    fName = fLongname;
  var values = "'" + fKey + "'," +
    "'" + fComponent + "'," +
    "'" + fName + "'," +
    f.Size.toString() + "," +
    "'" + fVersion + "'," +
    "'" + fLang + "'," +
    f.Attributes.toString() + "," +
    fSequence.toString();
  return addMsiRecord(db, "File", "`File`.`File`,`File`.`Component_`,`File`.`FileName`,`File`.`FileSize`,`File`.`Version`,`File`.`Language`,`File`.`Attributes`,`File`.`Sequence`", values);
}

function addMsiMediaRow(db, mDiskId, mLastSeq, mCabinet) {
  var values = mDiskId + "," + mLastSeq + ",'','#" + mCabinet + "','',''";
  return addMsiRecord(db, "Media", "`Media`.`DiskId`,`Media`.`LastSequence`,`Media`.`DiskPrompt`,`Media`.`Cabinet`,`Media`.`VolumeLabel`,`Media`.`Source`", values);
}

function addMsiSelfregRow( db, fileKey, fileCost ) {
  var columns = "`SelfReg`.`File_`,`SelfReg`.`Cost`";
  var values =  "'" + fileKey + "'," + fileCost.toString();
  var query = "SELECT `Cost` FROM `SelfReg` WHERE `File_`='" + fileKey + "'";
  return addMsiRecordIfNotSelect(db, "SelfReg", columns, values, query);
}

function addMsiRecordIfNotSelect( db, recTable, recColumns, recValues, selectQuery ) {
  log.write("addMsiRecordIfNotSelect", "Started");
  log.write("MSI-SQL", selectQuery);
  var view;
  try {
    view = db.OpenView(selectQuery);
  } catch(e) {
    log.write("addMsiRecordIfNotSelect - OpenView failed", e.number.toString() + "; " + e.description);
    return addMsiRecord(db, recTable, recColumns, recValues);
  }
  try {
    view.Execute();
  } catch(e) {
    log.write("addMsiRecordIfNotSelect - Execute Failed", e.number.toString(16) + "; " + e.description);
    return false;
  }
  try {
    var rec = view.Fetch();
    for(var i = 1; i < rec.fieldCount; i++) {
      log.write("RecordField-" + i, rec.StringData(i));
    }
    log.write("INSERT Skipped", "Record aleady exists");
  } catch(e) {
    view.Close();
    return addMsiRecord(db, recTable, recColumns, recValues);
  }
  return true;
}

function addMsiRecord( db, recTable, recColumns, recValues ) {
  log.write("addMsiRecord", "Started");
  var query = "INSERT INTO `" + recTable + "` (" + recColumns + ") VALUES (" + recValues + ")";
  log.write("MSI-SQL", query);
  try {
    var view = db.OpenView(query);
    view.Execute();
  } catch(e) {
    /* if you got a 80004005 error from WSCRIPT, the file name may have appeared twice check if there is another folder that has a file of same name either this one or that one has to go. */
    log.write("addMsiRecord failed", e.number.toString(16) + "; " + e.description);
    return false;
  }
  log.write("addMsiRecord", "Succeeded");
  return true;
}

/////////////////////////////////////////////////////////
function updateMsiControlText( db, dialogName, controlName, sourceTextFile ) {
  var datafilePath = dataPath + sourceTextFile;
  if(!fso.FileExists(datafilePath)) {
    WScript.Echo("updateMsiControlText failed. It could not find the source data file for the control.\n\nDialog control: " + dialogName + ":" + controlName + "\nData file: " + datafilePath);
    WScript.Quit(3);
  }
  var query = "SELECT `Text` FROM `Control` WHERE `Dialog_`='" + dialogName + "' AND `Control`='" + controlName + "'";
  log.write("MSI-SQL", query);
  var view = db.OpenView(query);
  view.Execute();
  var rec = view.Fetch();
  log.write("View.Modify", datafilePath);
  rec.SetStream(1, datafilePath);
  rec.StringData(1) = rec.ReadStream(1, MaxDataFileSize, msiReadStreamAnsi);
  view.Modify(msiViewModifyUpdate, rec);
}

function updateMsiPropertyRow( db, propName, propValue ) {
  var query = "SELECT `Value` FROM `Property` WHERE `Property`='" + propName + "'";
  log.write("MSI-SQL", query);
  var view = db.OpenView(query);
  view.Execute();
  var rec = view.Fetch();
  log.write("New Value", propValue);
  rec.StringData(1) = propValue;
  view.Modify(msiViewModifyUpdate, rec);
}

function updateMsiPropertyIntRow( db, propName, propValue ) {
  var query = "SELECT `Value` FROM `Property` WHERE `Property`='" + propName + "'";
  log.write("MSI-SQL", query);
  var view = db.OpenView(query);
  view.Execute();
  var rec = view.Fetch();
  log.write("New Value", propValue);
  rec.IntegerData(1) = propValue;
  view.Modify(msiViewModifyUpdate, rec);
}

function updateMsiDirectoryEntry( db, entryName, columnName, columnValue) {
  var query = "SELECT `" + columnName + "` FROM `Directory` WHERE `Directory`='" + entryName + "'";
  log.write("MSI-SQL", query);
  var view = db.OpenView(query);
  view.Execute();
  var rec = view.Fetch();
  log.write("Old " + columnName, rec.StringData(1));
  log.write("New " + columnName, columnValue);
  rec.StringData(1) = columnValue;
  view.Modify(msiViewModifyUpdate, rec);
}

/////////////////////////////////////////////////////////
function removeMsiFeatureEntry( db, entryName ) {
  var query = "SELECT * FROM `Feature` WHERE `Feature`='" + entryName + "'";
  removeMsiRecord(db, query);
}

function removeMsiDirectoryEntry( db, entryName ) {
  var query = "SELECT * FROM `Directory` WHERE `Directory`='" + entryName + "'";
  removeMsiRecord(db, query);
}

function removeMsiRegistryEntry( db, entryName ) {
  var query = "SELECT * FROM `Registry` WHERE `Registry`='" + entryName + "'";
  removeMsiRecord(db, query);
}

function removeMsiRecord( db, selectQuery ) {
  log.write("MSI-SQL", selectQuery);
  try {
    var view = db.OpenView(selectQuery);
    view.Execute();
  } catch(e) {
		// error code of 0x1A8 means a 'Record Not Found'.
		if (e.number == 0x1a8)
		  return true;
		log.write("SELECT Failed", e.number.toString(16) + "; " + e.description);
		return false;
  }
  try {
    var rec = view.Fetch();
    /*for(i = 1; i < rec.fieldCount; i++) {
      log.write("RecordField-" + i, rec.StringData(i));
    }*/
    view.Modify(msiViewModifyDelete, rec);
  } catch(e) {
		log.write("DELETE Skipped", "Record Not Found");
  }
	return true;
}

/////////////////////////////////////////////////////////
// reads a named value from the #define statement line of a macro in a C header file.
function readCDefinedConstValue(constName, headerFile) {
  //log.write("readCDefinedConstValue", "ConstName=" + constName + "; HeaderFile=" + headerFile);
  var f = fso.OpenTextFile(headerFile, 1);
  while(!f.AtEndOfStream) {
    var nextLine = f.ReadLine();
    var posDefine = nextLine.indexOf("#define");
    if(posDefine >= 0) {
      var posConst = nextLine.indexOf(constName);
      if (posConst >= 0 && posConst > posDefine) {
				//log.write("Found name at "+posConst, nextLine);
        var valStr = nextLine.substr(posConst+constName.length);
        var posTab = valStr.lastIndexOf("\t");
        var posSpc = valStr.lastIndexOf(" ");
        var valData;
        if (posTab > posSpc)
          valData = valStr.substr(posTab+1);
        else if (posSpc >= 0)
          valData = valStr.substr(posSpc+1)
        else
          valData = valStr;
        if(valData.length > 0) {
          f.Close();
          //log.write("readCDefinedConstValue ConstValue", valData);
          return parseInt(valData);
        }
        log.write("ERROR", "Empty data encountered");
        break;
      }
    }
  }
  f.Close();
  log.write("readCDefinedConstValue Failed");
  return 0;
}

/////////////////////////////////////////////////////////
// makes sure that a specified folder exists by creating it if it's not there.
function ensureFolder( folderPath) {
  //log.write("ensureFolder", folderPath);
  if (!fso.FolderExists(folderPath))
    fso.CreateFolder(folderPath);
}

/* makes a file writable by removing the read-only, hidden, and system attributes from a file.

Pertinent file attributes:
Normal: 0
ReadOnly: 1
Hidden: 2
System: 4
Combined attributes: 0x00000007
Attribute mask for the above bits: 0xfffffff8
*/
function ensureWritableFile( filePath) {
  if(!fso.FileExists(filePath))
    return false;
  var f = fso.GetFile(filePath);
  var oldAttribs = f.Attributes;
  f.Attributes &= 0xfffffff8;
  log.write("ensureWritableFile("+f.Name+")", oldAttribs.toString(16)+" --> "+f.Attributes.toString(16));
  return true;
}

// copies a source file to a destination making sure that the file at destination is writable.
function copyFileAlways( sourcePath, destinationPath) {
  ensureWritableFile(destinationPath);
  log.write("copyFileAlways", sourcePath + " -> " + destinationPath);
  if(!fso.FileExists(sourcePath)) {
    WScript.Echo("Source File Not Found\n\n"+sourcePath);
    return false;
  }
  fso.CopyFile(sourcePath, destinationPath, true);
  return ensureWritableFile(destinationPath);
}

/////////////////////////////////////////////////////////
// searches for an extension part in filename. fso.GetExtensionName() may not work for us since it requires that the file exists.
function getFileExtension(filename) {
	var pos = filename.lastIndexOf(".");
	if (pos==-1)
	  return "";
	// return up to 4 characters including a leading period. the size restriction is important for generating a DOS-style shortname for the File table.
	return filename.substr(pos, 4);
}

// tests a file's type. returns true if the file is an excutable (exe or dll).
function isExecutableFile(f) {
	var ext = getFileExtension(f.Name).toLowerCase();
	if (ext == ".exe")
	  return true;
	if (ext == ".dll")
	  return true;
  if (ext == ".sys")
	  return true;
	return false;
}

/////////////////////////////////////////////////////////
// generates a double-digit numeric string. If the input is 2022, the function returns '22'.
function getDoubleDigitString(numVal) {
  var s = "";
  if(numVal > 99)
	  numVal = numVal % 100;
  if(numVal < 10)
	  s = "0";
  s = s+numVal.toString();
  return s
}

// returns a DDF time stamp: e.g., '/date=02/15/07 /time=01:00:00p'
function getDdfTimeStamp(dateTimeVal) {
  var curAmOrPm = "a";
  var hour2 = dateTimeVal.getHours();
  if(hour2 > 12) {
    hour2 -= 12;
    curAmOrPm = "p";
  }
  if(hour2 == 0)
	  hour2 = 12;
  var curYear = getDoubleDigitString(dateTimeVal.getYear());
  var curMonth = getDoubleDigitString(dateTimeVal.getMonth()+1);
  var curDay = getDoubleDigitString(dateTimeVal.getDate());
  var curHour = getDoubleDigitString(hour2);
  var curMinute = getDoubleDigitString(dateTimeVal.getMinutes());
  var curSecond = "00";
  return " /date="+curMonth+"/"+curDay+"/"+curYear+" /time="+curHour+":"+curMinute+":"+curSecond+curAmOrPm;
}

// formats time components as hh:mm:00# where # is set to 'a' for am or 'p' for pm. The seconds digits are always reset to 0.
function getTimeString(t) {
  var hours = t.getHours();
  var minutes = t.getMinutes();
  if(hours == 0)
    return "12:" + getDoubleDigitString(minutes) + ":00a";
  else if(hours < 12)
    return getDoubleDigitString(hours) + ":" + hours.toString() + ":00a";
  else if(hours == 12)
    return "12:" + getDoubleDigitString(minutes) + ":00p";
  else
    return getDoubleDigitString(hours-12) + ":" + getDoubleDigitString(minutes) + ":00p";
}

// formats date components as MM/DD/YY.
function getDateString(t) {
  var year = getDoubleDigitString(t.getYear());
  var month = getDoubleDigitString(t.getMonth()+1);
  var day = getDoubleDigitString(t.getDate());
  return month+"/"+day+"/"+year;
}

// reads the version number of an exe or dll using automation object MaxsUtilLib.VersionInfo.
function queryFileVersion(f) {
	log.write("queryFileVersion", f.Name);
  vinfo.File = f.Path;
  try {
    var vs = vinfo.VersionString;
    return vs;
  } catch(e) {
		var errorcode = e.number<0? (0x100000000+e.number):e.number;
		var msg = "0x"+errorcode.toString(16)+"; "+e.description;
		log.write("queryFileVersion Failed", msg);
	}
  return "";
}

/////////////////////////////////////////////////////////
/* This is an event log object. It creates a file with the same name as that of this script with extension .log in the directory the script resides in.
To write an event, use a call statement of form eventLog.write(event). The time of the call and the event text are written to the log.
To write an event with associated data, call eventLog.write(event, data). Event is written to the log first. Data is written next, actually preceded by a separator colon. This time, no timestamp is written.
*/
function eventLog() {
	this._path = WScript.ScriptFullName + ".log";
  var f = fso.CreateTextFile(this._path, true, true);
  f.Close();
	this.write = function(eventLabel, eventData) {
    var f = fso.OpenTextFile(this._path, 8, false, -1);
    if (typeof eventData == "string" || typeof eventData == "number") {
      f.Write(eventLabel+": ");
      f.WriteLine(eventData);
    } else {
			var now = new Date;
			var time = "["+now.getHours()+":"+now.getMinutes()+":"+now.getSeconds()+"] ";
      f.WriteLine(time+eventLabel);
	  }
    f.Close();
	};
}

