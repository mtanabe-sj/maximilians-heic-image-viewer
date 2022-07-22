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

/////////////////////////////////////////////////////////
msiName = "template.msi";
MaxDataFileSize = 2000000;

msiOpenDatabaseModeReadOnly = 0;
msiOpenDatabaseModeTransact = 1;
msiViewModifyUpdate = 2;
msiViewModifyDelete = 6;
msiReadStreamAnsi = 2;
msiReadStreamBytes = 1;

var fso = new ActiveXObject("Scripting.FileSystemObject");
var wsh = new ActiveXObject("WScript.Shell");

var basePath = wsh.ExpandEnvironmentStrings("%ProjectSource%") + "\\";
if(basePath == "%ProjectSource%\\") {
  WScript.Echo("Environment variable %ProjectSource% not found");
  WScript.Quit(2);
}
// name of the MSI database.
var homePath = basePath + "installer\\";
var dataPath = homePath + "data\\";
var msiPath = homePath + msiName;

// start an event log in case we need to troubleshoot.
var log = new eventLog;
log.write(WScript.ScriptName + " Started");
log.write("MSI PATH", msiPath);

var installer = new ActiveXObject("WindowsInstaller.Installer");
try {
  var msi = installer.OpenDatabase(msiPath, msiOpenDatabaseModeTransact);
  updateMsiBinaryRow(msi, "bannrbmp", "bannrbmp.bmp");
  updateMsiBinaryRow(msi, "dlgbmp", "dlgbmp.bmp");
  updateMsiIconRow(msi, "product.ico", "product.ico");
  msi.Commit();
} catch(e) {
	var errorcode = e.number<0? (0x100000000+e.number):e.number;
	var msg = "0x"+errorcode.toString(16)+"; "+e.description;
	log.write("MSI OPEN FAILED", msg);
	WScript.Echo("MSI OPEN FAILED\n\n"+msg);
}

log.write(WScript.ScriptName + " Stopped");

/////////////////////////////////////////////////////////
function updateMsiBinaryRow( db, binName, sourceBinFile ) {
  var datafilePath = dataPath + sourceBinFile;
  if(!fso.FileExists(datafilePath)) {
    WScript.Echo("updateMsiBinaryRow failed. It could not find the source data file for the control.\n\nBinary name: " + binName + "\nData file: " + datafilePath);
    WScript.Quit(3);
  }
  var query = "SELECT `Data` FROM `Binary` WHERE `Name`='" + binName + "'";
  log.write("QUERY", query);
  var view = db.OpenView(query);
  view.Execute();
  var rec = view.Fetch();
  log.write("View.Modify", datafilePath);
  rec.SetStream(1, datafilePath);
  view.Modify(msiViewModifyUpdate, rec);
}

function updateMsiIconRow( db, binName, sourceBinFile ) {
  var datafilePath = dataPath + sourceBinFile;
  if(!fso.FileExists(datafilePath)) {
    WScript.Echo("updateMsiIconRow failed. It could not find the source data file for the control.\n\nBinary name: " + binName + "\nData file: " + datafilePath);
    WScript.Quit(3);
  }
  var query = "SELECT `Data` FROM `Icon` WHERE `Name`='" + binName + "'";
  log.write("QUERY", query);
  var view = db.OpenView(query);
  view.Execute();
  var rec = view.Fetch();
  log.write("View.Modify", datafilePath);
  rec.SetStream(1, datafilePath);
  view.Modify(msiViewModifyUpdate, rec);
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

