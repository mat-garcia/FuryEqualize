; FuryEqualize — Inno Setup 6
; Requer: ISCC (compilador Inno). Build da UI + Core antes de compilar.

#define MyAppName "FuryEqualize"
#define MyAppVersion "0.1.0"
#define MyAppPublisher "FuryEqualize"
#define MyAppURL "https://furyequalize.local"
#define SrcApp "..\app\FuryEqualize.UI\bin\Release\net8.0-windows\FuryEqualize.exe"
#define SrcCore "..\core\build\Release\FuryEqualizeCore.dll"

[Setup]
AppId={{8E2B8F1A-FURY-4A9E-9E2A-FURYEQUALIZE}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}
OutputDir=Output
OutputBaseFilename=FuryEqualize-Setup-{#MyAppVersion}
Compression=lzma
SolidCompression=yes
PrivilegesRequired=admin
ArchitecturesInstallIn64BitMode=x64
UninstallDisplayIcon={app}\FuryEqualize.exe

[Languages]
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Flags: unchecked
Name: "autostart"; Description: "Iniciar com o Windows (System Tray)"; GroupDescription: "Inicialização:"

[Files]
Source: "{#SrcApp}"; DestDir: "{app}"; Flags: ignoreversion
Source: "{#SrcCore}"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
Source: "..\app\FuryEqualize.UI\bin\Release\net8.0-windows\*.dll"; DestDir: "{app}"; Flags: ignoreversion
Source: "..\app\FuryEqualize.UI\bin\Release\net8.0-windows\*.json"; DestDir: "{app}"; Flags: ignoreversion skipifsourcedoesntexist
; VC++ Redist (incluir vcredist_x64.exe ao lado do .iss se distribuir offline)
; Source: "vcredist_x64.exe"; DestDir: "{tmp}"; Flags: deleteafterinstall

[Icons]
Name: "{group}\{#MyAppName}"; Filename: "{app}\FuryEqualize.exe"
Name: "{autodesktop}\{#MyAppName}"; Filename: "{app}\FuryEqualize.exe"; Tasks: desktopicon
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"

[Registry]
Root: HKCU; Subkey: "Software\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "FuryEqualize"; ValueData: """{app}\FuryEqualize.exe"" --minimized"; Tasks: autostart

[Run]
Filename: "{app}\FuryEqualize.exe"; Description: "{cm:LaunchProgram,{#StringChange(MyAppName, '&', '&&')}}"; Flags: nowait postinstall skipifsilent
; Filename: "{tmp}\vcredist_x64.exe"; Parameters: "/install /quiet /norestart"; StatusMsg: "Instalando Visual C++ Redistributable..."; Flags: waituntilterminated

[Code]
function InitializeSetup(): Boolean;
begin
  Result := True;
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  ResultCode: Integer;
begin
  if CurStep = ssPostInstall then
  begin
    // Checagem pós-instalação: avisa sobre Loudness Equalization
    // (a checagem completa de registry é feita no primeiro boot do app via SystemCheckService)
    Exec('powershell.exe',
      '-NoProfile -Command "Write-Host ''Verifique: Painel de Controle > Som > Propriedades > Enhancements > desmarque Loudness Equalization''"',
      '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
  end;
end;

// Instalação do cabo virtual (VB-Cable / Hi-Fi Cable) — placeholder
// Descomente e ajuste se for empacotar o driver:
// [Run]
// Filename: "{app}\tools\VBCABLE_Setup_x64.exe"; Parameters: "-i -h"; Flags: waituntilterminated; StatusMsg: "Instalando VB-Cable..."
