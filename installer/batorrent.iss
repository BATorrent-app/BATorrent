; BATorrent Installer - Inno Setup Script
; Custom dark theme with branding

; Version is injected by CI via ISCC /DMyAppVersion=x.y.z; fallback for local builds.
#ifndef MyAppVersion
  #define MyAppVersion "4.0.0"
#endif

[Setup]
AppName=BATorrent
AppVersion={#MyAppVersion}
AppVerName=BATorrent {#MyAppVersion}
AppPublisher=Mateuscruz19
AppPublisherURL=https://github.com/Mateuscruz19/BATorrent
AppSupportURL=https://github.com/Mateuscruz19/BATorrent/issues
AppUpdatesURL=https://github.com/Mateuscruz19/BATorrent/releases
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
; The native "Select Setup Language" dialog can't be dark-themed (it shows
; before the wizard form exists), so it clashed with our dark wizard. Skip it
; and auto-detect from the system locale; users can still switch in-app.
ShowLanguageDialog=no
PrivilegesRequired=lowest
ChangesAssociations=yes
CloseApplications=force
CloseApplicationsFilter=BATorrent.exe
RestartApplications=yes
ArchitecturesInstallIn64BitMode=x64compatible
; No license page: its RichEdit memo + accept/decline radios are Windows control
; types that ignore Font.Color (unlike the static labels everywhere else), so on
; our dark wizard the EULA text rendered black-on-dark with no reliable fix. MIT
; doesn't require click-through acceptance — the license still ships as the
; LICENSE file in the install dir and shows in the app's About dialog.
VersionInfoVersion={#MyAppVersion}.0
VersionInfoCompany=Mateuscruz19
VersionInfoDescription=BATorrent - A modern BitTorrent client
VersionInfoCopyright=Copyright (c) 2024-2026 Mateus Cruz
VersionInfoProductName=BATorrent
VersionInfoProductVersion={#MyAppVersion}

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "portuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"

[Files]
Source: "..\release\*"; DestDir: "{app}"; Flags: recursesubdirs
Source: "..\LICENSE"; DestDir: "{app}"; Flags: ignoreversion

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
// Kill any running BATorrent instance before installing. The Restart
// Manager (CloseApplications=force) handles most cases, but it can't
// always reach tray-only processes that have no visible window. This
// taskkill fallback ensures the exe is never locked during file copy.
function PrepareToInstall(var NeedsRestart: Boolean): String;
var
  ResultCode: Integer;
begin
  Exec('taskkill.exe', '/f /im BATorrent.exe', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  // Small delay so Windows releases file handles after process death.
  Sleep(500);
  Result := '';
end;

const
  // Dark theme colors (Inno uses BGR — read the trailing comment for the
  // actual RGB hex code).
  BG_DARK       = $0A0A0E;    // #0E0A0A
  BG_SURFACE    = $191519;    // #151919
  BG_PANEL      = $201A1A;    // #1A1A20
  TEXT_COLOR    = $F0F0F0;    // #F0F0F0  bright off-white
  TEXT_MUTED    = $B0B0B8;    // #B8B0B0  light gray, readable on dark
  ACCENT_RED    = $2626DC;    // #DC2626
  ACCENT_DARK   = $1B1B99;    // #991B1B
  BORDER_COLOR  = $352A2A;    // #2A2A35

procedure StyleControlText(C: TControl);
var
  i: Integer;
  Container: TWinControl;
begin
  // Backgrounds: paint every container/input dark so no page (welcome, finish,
  // license, etc.) is left with the default white Windows background.
  if C is TNewNotebookPage then TNewNotebookPage(C).Color := BG_DARK
  else if C is TPanel then TPanel(C).Color := BG_DARK
  else if C is TRichEditViewer then TRichEditViewer(C).Color := BG_PANEL
  else if C is TNewMemo then TNewMemo(C).Color := BG_PANEL
  else if C is TNewCheckListBox then begin
    // The finished-page "Launch BATorrent" checkbox + the tasks list are
    // TNewCheckListBox — their item text stayed black before.
    TNewCheckListBox(C).Color := BG_DARK;
    TNewCheckListBox(C).Font.Color := TEXT_COLOR;
  end
  else if C is TBevel then TBevel(C).Visible := False;

  // Foreground: readable text on every text-bearing control. Buttons are left
  // native so they keep the standard Windows look.
  if C is TNewStaticText then TNewStaticText(C).Font.Color := TEXT_COLOR
  else if C is TLabel then TLabel(C).Font.Color := TEXT_COLOR
  else if C is TNewCheckBox then TNewCheckBox(C).Font.Color := TEXT_COLOR
  else if C is TNewRadioButton then TNewRadioButton(C).Font.Color := TEXT_COLOR;

  // Recurse into containers so we catch controls nested in panels/pages.
  if C is TWinControl then begin
    Container := TWinControl(C);
    for i := 0 to Container.ControlCount - 1 do
      StyleControlText(Container.Controls[i]);
  end;
end;

procedure InitializeWizard();
var
  BrandLabel: TNewStaticText;
  VersionLabel: TNewStaticText;
begin
  // -- Main form dark background. The welcome/finished pages live on the outer
  //    notebook; we color the PAGES (TNewNotebookPage) via the recursive walk
  //    rather than the notebook itself (TNewNotebook has no Color property). --
  WizardForm.Color := BG_DARK;
  WizardForm.InnerPage.Color := BG_DARK;
  WizardForm.MainPanel.Color := BG_SURFACE;

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

  // -- Task list --
  WizardForm.TasksList.Color := BG_PANEL;
  WizardForm.TasksList.Font.Color := TEXT_COLOR;

  // -- Ready memo --
  WizardForm.ReadyMemo.Color := BG_PANEL;
  WizardForm.ReadyMemo.Font.Color := TEXT_COLOR;

  // -- Page-level header labels (rendered on every page) --
  WizardForm.PageNameLabel.Font.Color := TEXT_COLOR;
  WizardForm.PageDescriptionLabel.Font.Color := TEXT_MUTED;

  // -- Various labels --
  WizardForm.SelectDirLabel.Font.Color := TEXT_MUTED;
  WizardForm.DiskSpaceLabel.Font.Color := TEXT_MUTED;
  WizardForm.SelectTasksLabel.Font.Color := TEXT_MUTED;
  WizardForm.SelectStartMenuFolderLabel.Font.Color := TEXT_MUTED;
  WizardForm.ReadyLabel.Font.Color := TEXT_MUTED;
  WizardForm.StatusLabel.Font.Color := TEXT_MUTED;
  WizardForm.FilenameLabel.Font.Color := TEXT_MUTED;

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
  VersionLabel.Caption := 'v{#MyAppVersion} - A modern BitTorrent client';
  VersionLabel.Font.Color := TEXT_MUTED;
  VersionLabel.Font.Size := 7;
  VersionLabel.Left := BrandLabel.Left + BrandLabel.Width + ScaleX(8);
  VersionLabel.Top := BrandLabel.Top + ScaleY(2);
  VersionLabel.AutoSize := True;
end;

procedure CurPageChanged(CurPageID: Integer);
begin
  // Walk the ENTIRE form each time: task checkboxes and the welcome/finish
  // pages live on different containers and some are created only when their
  // page first shows. Re-theming the whole WizardForm here guarantees no page
  // keeps a white background or dark text.
  StyleControlText(WizardForm);
end;

// Custom colors for uninstaller too
procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    // Clean up any remaining app data
  end;
end;
