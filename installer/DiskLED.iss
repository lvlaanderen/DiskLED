; Inno Setup script for DiskLED. Build with installer\Build-Release.ps1, or directly:
;   ISCC.exe /DVersion=3.1 /DArch=x64 installer\DiskLED.iss
; The program is taken from dist\DiskLED-<Version>-<Arch>\DiskLED.exe (made by Build-Release.ps1).

#ifndef Version
  #define Version "3.1"
#endif
#ifndef Arch
  #define Arch "x64"
#endif

#define BuildDir "..\dist\DiskLED-" + Version + "-" + Arch

[Setup]
; Never change the AppId: it is how upgrades and the uninstaller find the installation
AppId={{F4EBA757-A853-4814-A09B-A5F96E75CDAD}
AppName=DiskLED
AppVersion={#Version}
AppVerName=DiskLED {#Version}
AppPublisher=Lourens Vlaanderen (fork of DiskLED by Helge Klein)
AppPublisherURL=https://github.com/lvlaanderen/DiskLED
AppSupportURL=https://github.com/lvlaanderen/DiskLED/issues
AppUpdatesURL=https://github.com/lvlaanderen/DiskLED/releases
VersionInfoVersion={#Version}.0.0

; All users (Program Files) or just the current user: the user is asked. The settings are in %AppData% either way.
DefaultDirName={autopf}\DiskLED
DisableProgramGroupPage=yes
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog

; Setup asks to close DiskLED if it is running
AppMutex=DiskLED.SingleInstance

#if Arch == "x64"
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
#endif

LicenseFile=..\LICENSE
UninstallDisplayIcon={app}\DiskLED.exe
OutputDir=..\dist
OutputBaseFilename=DiskLED-{#Version}-{#Arch}-Setup
Compression=lzma2
SolidCompression=yes
WizardStyle=modern

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "autostart"; Description: "Start DiskLED when I log on to Windows"; GroupDescription: "Options:"

[Files]
Source: "{#BuildDir}\DiskLED.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\LICENSE"; DestName: "LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\Additional Files\Release Notes.txt"; DestDir: "{app}"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\DiskLED"; Filename: "{app}\DiskLED.exe"

[Registry]
; The same entry that the Start with Windows option in the settings makes
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "DiskLED"; ValueData: """{app}\DiskLED.exe"""; Flags: uninsdeletevalue; Tasks: autostart

[Run]
Filename: "{app}\DiskLED.exe"; Description: "Start DiskLED"; Flags: nowait postinstall skipifsilent

[Code]
const
  RunKey = 'Software\Microsoft\Windows\CurrentVersion\Run';

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
var
  Command: String;
begin
  if CurUninstallStep = usUninstall then
  begin
    // The Start with Windows option in the settings of the program makes the same entry. Setup did not make it then, so remove it here.
    if RegQueryStringValue(HKCU, RunKey, 'DiskLED', Command) and
       (CompareText(Command, '"' + ExpandConstant('{app}\DiskLED.exe') + '"') = 0) then
      RegDeleteValue(HKCU, RunKey, 'DiskLED');
  end;

  if CurUninstallStep = usPostUninstall then
  begin
    if not UninstallSilent then
      if MsgBox('Do you also want to remove your DiskLED settings?' + #13#10#13#10 + ExpandConstant('{userappdata}\DiskLED'),
                mbConfirmation, MB_YESNO or MB_DEFBUTTON2) = IDYES then
        DelTree(ExpandConstant('{userappdata}\DiskLED'), True, True, True);
  end;
end;
