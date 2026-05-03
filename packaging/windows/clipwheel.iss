; Inno Setup script for ClipWheel
[Setup]
AppName=ClipWheel
AppVersion=2.1.0
DefaultDirName={autopf}\ClipWheel
DefaultGroupName=ClipWheel
OutputBaseFilename=ClipWheel-Setup
Compression=lzma
SolidCompression=yes
SetupIconFile=..\..\assets\icons\windows\clipwheel.ico

[Files]
Source: "..\..\clipwheel.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\clipwheel.ini"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\..\assets\icons\windows\clipwheel.ico"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{group}\ClipWheel"; Filename: "{app}\clipwheel.exe"; IconFilename: "{app}\clipwheel.ico"
Name: "{commondesktop}\ClipWheel"; Filename: "{app}\clipwheel.exe"; IconFilename: "{app}\clipwheel.ico"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create desktop shortcut"; GroupDescription: "Additional icons:"

[Registry]
Root: HKCR; Subkey: ".cwh"; ValueType: string; ValueName: ""; ValueData: "ClipWheel.Snippet"; Flags: uninsdeletevalue
Root: HKCR; Subkey: "ClipWheel.Snippet\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\clipwheel.ico,0"
