; BATorrent Installer - Inno Setup Script
; Custom dark theme with branding

[Setup]
AppName=BATorrent
AppVersion=2.2.0
AppVerName=BATorrent 2.2.0
AppPublisher=Mateuscruz19
AppPublisherURL=https://github.com/Mateuscruz19/BAT-Torrent
AppSupportURL=https://github.com/Mateuscruz19/BAT-Torrent/issues
AppUpdatesURL=https://github.com/Mateuscruz19/BAT-Torrent/releases
DefaultDirName={autopf}\BATorrent
DefaultGroupName=BATorrent
OutputDir=..\output
OutputBaseFilename=BATorrent-setup-x86_64
Compression=lzma2
SolidCompression=yes
SetupIconFile=..\src\images\logo1.ico
UninstallDisplayIcon={app}\BATorrent.exe
WizardStyle=modern
WizardSizePercent=120,120
WizardImageFile=..\installer\wizard_large.bmp
WizardSmallImageFile=..\installer\wizard_small.bmp
WizardImageStretch=yes
PrivilegesRequired=lowest
ChangesAssociations=yes
CloseApplications=yes
RestartApplications=no
ArchitecturesInstallIn64BitMode=x64compatible
LicenseFile=..\LICENSE
VersionInfoVersion=2.2.0.0
VersionInfoCompany=Mateuscruz19
VersionInfoDescription=BATorrent - A modern BitTorrent client
VersionInfoCopyright=Copyright (c) 2024-2026 Mateus Cruz
VersionInfoProductName=BATorrent
VersionInfoProductVersion=2.2.0

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"

[Files]
Source: "..\release\*"; DestDir: "{app}"; Flags: recursesubdirs

[Icons]
Name: "{group}\BATorrent"; Filename: "{app}\BATorrent.exe"; IconFilename: "{app}\BATorrent.exe"
Name: "{group}\Uninstall BATorrent"; Filename: "{uninstallexe}"
Name: "{autodesktop}\BATorrent"; Filename: "{app}\BATorrent.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create a desktop shortcut"; GroupDescription: "Additional icons:"
Name: "fileassoc"; Description: "Associate .torrent files with BATorrent"; GroupDescription: "File associations:"; Flags: checkedonce
Name: "magnetassoc"; Description: "Associate magnet links with BATorrent"; GroupDescription: "File associations:"; Flags: checkedonce

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

[UninstallDelete]
Type: filesandordirs; Name: "{app}\cache"
Type: filesandordirs; Name: "{app}\logs"

[Code]
const
  // Dark theme colors
  BG_DARK       = $0A0A0E;    // #0E0A0A  (Inno uses BGR)
  BG_SURFACE    = $191519;    // #151919
  BG_PANEL      = $201A1A;    // #1A1A20
  TEXT_COLOR     = $D0C8C8;    // #C8C8D0
  TEXT_MUTED     = $685555;    // #555568
  ACCENT_RED     = $2626DC;    // #DC2626 (BGR)
  ACCENT_DARK    = $1B1B99;    // #991B1B (BGR)
  BORDER_COLOR   = $352A2A;    // #2A2A35

procedure InitializeWizard();
var
  BrandLabel: TNewStaticText;
  VersionLabel: TNewStaticText;
begin
  // -- Main form dark background --
  WizardForm.Color := BG_DARK;

  // -- Inner page panel --
  WizardForm.InnerPage.Color := BG_DARK;
  WizardForm.OuterPage.Color := BG_DARK;

  // -- Main panel (left side on Welcome/Finish) --
  WizardForm.MainPanel.Color := BG_SURFACE;

  // -- Page name and description labels --
  WizardForm.PageNameLabel.Font.Color := TEXT_COLOR;
  WizardForm.PageNameLabel.Font.Size := 12;
  WizardForm.PageNameLabel.Font.Style := [fsBold];
  WizardForm.PageDescriptionLabel.Font.Color := TEXT_MUTED;

  // -- Welcome & Finish labels --
  WizardForm.WelcomeLabel1.Font.Color := TEXT_COLOR;
  WizardForm.WelcomeLabel1.Font.Size := 16;
  WizardForm.WelcomeLabel1.Font.Style := [fsBold];
  WizardForm.WelcomeLabel2.Font.Color := TEXT_MUTED;
  WizardForm.WelcomeLabel2.Font.Size := 10;
  WizardForm.FinishedHeadingLabel.Font.Color := TEXT_COLOR;
  WizardForm.FinishedHeadingLabel.Font.Size := 16;
  WizardForm.FinishedHeadingLabel.Font.Style := [fsBold];
  WizardForm.FinishedLabel.Font.Color := TEXT_MUTED;

  // -- Directory and Group page --
  WizardForm.DirEdit.Color := BG_PANEL;
  WizardForm.DirEdit.Font.Color := TEXT_COLOR;
  WizardForm.GroupEdit.Color := BG_PANEL;
  WizardForm.GroupEdit.Font.Color := TEXT_COLOR;

  // -- Task list and component list --
  WizardForm.TasksList.Color := BG_PANEL;
  WizardForm.TasksList.Font.Color := TEXT_COLOR;
  WizardForm.ComponentsList.Color := BG_PANEL;
  WizardForm.ComponentsList.Font.Color := TEXT_COLOR;

  // -- Ready memo --
  WizardForm.ReadyMemo.Color := BG_PANEL;
  WizardForm.ReadyMemo.Font.Color := TEXT_COLOR;

  // -- License page --
  WizardForm.LicenseMemo.Color := BG_PANEL;
  WizardForm.LicenseMemo.Font.Color := TEXT_COLOR;
  WizardForm.LicenseAcceptedRadio.Font.Color := TEXT_COLOR;
  WizardForm.LicenseNotAcceptedRadio.Font.Color := TEXT_COLOR;

  // -- Various labels --
  WizardForm.SelectDirLabel.Font.Color := TEXT_MUTED;
  WizardForm.SelectDirBrowseLabel.Font.Color := TEXT_MUTED;
  WizardForm.DiskSpaceLabel.Font.Color := TEXT_MUTED;
  WizardForm.SelectGroupLabel.Font.Color := TEXT_MUTED;
  WizardForm.SelectTasksLabel.Font.Color := TEXT_MUTED;
  WizardForm.ReadyLabel.Font.Color := TEXT_MUTED;

  // -- Progress bar page --
  WizardForm.StatusLabel.Font.Color := TEXT_MUTED;
  WizardForm.FilenameLabel.Font.Color := TEXT_MUTED;

  // -- Bottom panel (buttons area) --
  WizardForm.Bevel1.Visible := False;

  // -- Add branding label at the bottom --
  BrandLabel := TNewStaticText.Create(WizardForm);
  BrandLabel.Parent := WizardForm;
  BrandLabel.Caption := 'BATorrent';
  BrandLabel.Font.Color := ACCENT_RED;
  BrandLabel.Font.Size := 8;
  BrandLabel.Font.Style := [fsBold];
  BrandLabel.Left := ScaleX(16);
  BrandLabel.Top := WizardForm.ClientHeight - ScaleY(32);
  BrandLabel.AutoSize := True;

  VersionLabel := TNewStaticText.Create(WizardForm);
  VersionLabel.Parent := WizardForm;
  VersionLabel.Caption := 'v2.2.0 - A modern BitTorrent client';
  VersionLabel.Font.Color := TEXT_MUTED;
  VersionLabel.Font.Size := 7;
  VersionLabel.Left := BrandLabel.Left + BrandLabel.Width + ScaleX(8);
  VersionLabel.Top := BrandLabel.Top + ScaleY(2);
  VersionLabel.AutoSize := True;
end;

// Custom colors for uninstaller too
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    // Clean up any remaining app data
  end;
end;
