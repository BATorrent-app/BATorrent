[Setup]
AppName=BATorrent
AppVersion=1.0
AppPublisher=Mateuscruz19
AppPublisherURL=https://github.com/Mateuscruz19/BAT-Torrent
DefaultDirName={autopf}\BATorrent
DefaultGroupName=BATorrent
OutputDir=..\output
OutputBaseFilename=BATorrent-setup-x86_64
Compression=lzma2
SolidCompression=yes
SetupIconFile=..\src\images\logo1.ico
UninstallDisplayIcon={app}\BATorrent.exe
WizardStyle=modern
PrivilegesRequired=lowest
ChangesAssociations=yes

[Files]
Source: "..\release\*"; DestDir: "{app}"; Flags: recursesubdirs

[Icons]
Name: "{group}\BATorrent"; Filename: "{app}\BATorrent.exe"
Name: "{group}\Uninstall BATorrent"; Filename: "{uninstallexe}"
Name: "{autodesktop}\BATorrent"; Filename: "{app}\BATorrent.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"
Name: "fileassoc"; Description: "Associate .torrent files with BATorrent"; GroupDescription: "File associations:"
Name: "magnetassoc"; Description: "Associate magnet links with BATorrent"; GroupDescription: "File associations:"

[Registry]
; .torrent file association
Root: HKCU; Subkey: "Software\Classes\.torrent"; ValueType: string; ValueName: ""; ValueData: "BATorrent.Torrent"; Flags: uninsdeletevalue; Tasks: fileassoc
Root: HKCU; Subkey: "Software\Classes\BATorrent.Torrent"; ValueType: string; ValueName: ""; ValueData: "Torrent File"; Flags: uninsdeletekey; Tasks: fileassoc
Root: HKCU; Subkey: "Software\Classes\BATorrent.Torrent\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\BATorrent.exe,0"; Tasks: fileassoc
Root: HKCU; Subkey: "Software\Classes\BATorrent.Torrent\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\BATorrent.exe"" ""%1"""; Tasks: fileassoc

; magnet link association
Root: HKCU; Subkey: "Software\Classes\magnet"; ValueType: string; ValueName: ""; ValueData: "URL:Magnet Protocol"; Flags: uninsdeletekey; Tasks: magnetassoc
Root: HKCU; Subkey: "Software\Classes\magnet"; ValueType: string; ValueName: "URL Protocol"; ValueData: ""; Tasks: magnetassoc
Root: HKCU; Subkey: "Software\Classes\magnet\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\BATorrent.exe,0"; Tasks: magnetassoc
Root: HKCU; Subkey: "Software\Classes\magnet\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\BATorrent.exe"" ""%1"""; Tasks: magnetassoc

[Run]
Filename: "{app}\BATorrent.exe"; Description: "Launch BATorrent"; Flags: nowait postinstall skipifsilent
