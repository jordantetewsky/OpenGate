; OpenGate prototype: keep runtime files together; never modify Resolve or user settings.
#ifndef SourceRoot
  #error SourceRoot is required
#endif
#ifndef BuildVersion
  #error BuildVersion is required
#endif
#ifndef BuildRevision
  #error BuildRevision is required
#endif
[Setup]
AppId=OpenGate-CineWaltz-Prototype
AppName=OpenGate
AppVersion={#BuildVersion}
AppVerName=OpenGate {#BuildVersion} prototype.{#BuildRevision}
AppPublisher=CineWaltz; based on Unbound
DefaultDirName={autopf}\OpenGate
DefaultGroupName=OpenGate
; Resolve discovers native workflow shortcut targets in the shared ProgramData
; directory. Standard installer elevation writes only OpenGate-owned files there.
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
OutputDir={#SourceRoot}\release\packages
OutputBaseFilename=OpenGate-{#BuildVersion}-Prototype.{#BuildRevision}-Windows-x64
Compression=lzma2
SolidCompression=yes
SetupLogging=yes
CloseApplications=yes
RestartApplications=no
UninstallDisplayIcon={app}\Unbound.exe
[Files]
Source: "{#SourceRoot}\release\win\{#BuildVersion}\OpenGate.dvcp"; DestDir: "{commonappdata}\Blackmagic Design\DaVinci Resolve\Support\IOPlugins\OpenGate.dvcp.bundle\Contents\Win64"; Flags: ignoreversion
Source: "{#SourceRoot}\release\win\{#BuildVersion}\Unbound.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\release\win\{#BuildVersion}\Unbound.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\LICENSE"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\infrastructure\windows\prototype-notes.txt"; DestDir: "{app}"; DestName: "README.txt"; Flags: ignoreversion
Source: "{#SourceRoot}\release\win\{#BuildVersion}\release-manifest.json"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\release\win\{#BuildVersion}\workflow-commands\*.py"; DestDir: "{commonappdata}\Blackmagic Design\DaVinci Resolve\Support\Workflow Integration Plugins"; Flags: ignoreversion
Source: "{#SourceRoot}\release\win\{#BuildVersion}\workflow-commands\manifest.json"; DestDir: "{app}"; DestName: "workflow-commands-manifest.json"; Flags: ignoreversion
Source: "{#SourceRoot}\infrastructure\windows\export-support.ps1"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SourceRoot}\infrastructure\windows\export-support.cmd"; DestDir: "{app}"; DestName: "Export OpenGate Support Logs.cmd"; Flags: ignoreversion
[Icons]
Name: "{group}\OpenGate"; Filename: "{app}\Unbound.exe"
Name: "{group}\Export OpenGate Support Logs"; Filename: "{app}\Export OpenGate Support Logs.cmd"
Name: "{group}\Prototype limitations"; Filename: "{app}\README.txt"
Name: "{group}\Uninstall OpenGate"; Filename: "{uninstallexe}"
[Code]
function ResolveStopReason(): String;
var Locator, Services, Processes: Variant;
begin
  Result := '';
  try
    Locator := CreateOleObject('WbemScripting.SWbemLocator');
    Services := Locator.ConnectServer('', 'root\CIMV2');
    Processes := Services.ExecQuery('SELECT ProcessId FROM Win32_Process WHERE Name="Resolve.exe"');
    if Processes.Count > 0 then
      Result := 'Save your work and quit DaVinci Resolve before installing or uninstalling OpenGate.';
  except
    Result := 'Could not check whether Resolve is closed. Close Resolve and retry.';
  end;
end;

function PrepareToInstall(var NeedsRestart: Boolean): String;
begin
  Result := ResolveStopReason();
end;
function InitializeUninstall(): Boolean;
var Reason: String;
begin
  Reason := ResolveStopReason();
  Result := Reason = '';
  if not Result then MsgBox(Reason, mbInformation, MB_OK);
end;
