#pragma codepage 65001
; KFTCOneCAP Inno Setup Script
; Requires Inno Setup 6.x (https://jrsoftware.org/isinfo.php)

#define AppName "KFTCOneCAP"
#define AppVersion "3.0.1.300"
#define AppPublisher "KFTC"
#define AppExeName "KFTCOneCAP.exe"
#define AppInstallDir "{pf32}\KFTCOneCAP"

[Setup]
AppId={{A1B2C3D4-E5F6-7890-ABCD-EF1234567890}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher={#AppPublisher}
AppPublisherURL=https://www.kftc.or.kr
DefaultDirName={#AppInstallDir}
DefaultGroupName={#AppName}
DisableProgramGroupPage=yes
OutputDir=D:\OneCap\RELEASE
OutputBaseFilename=KFTCOneCAP_Setup_{#AppVersion}
SetupIconFile=D:\OneCap\RELEASE\KFTCOneCAP\setupico.ico
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x86 x64

[Languages]
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"

[Dirs]
Name: "{app}"; Permissions: users-modify

[Files]
Source: "D:\OneCap\RELEASE\KFTCOneCAP\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
; 시작 메뉴 - 프로그램 그룹
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppExeName}"; IconFilename: "{app}\KFTCOneCAP.ico"
; 바탕화면
Name: "{commondesktop}\{#AppName}"; Filename: "{app}\{#AppExeName}"; IconFilename: "{app}\KFTCOneCAP.ico"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "바탕화면에 아이콘 만들기"; GroupDescription: "추가 작업:"

[InstallDelete]
Type: files; Name: "{commonstartup}\{#AppName}.lnk"
Type: files; Name: "{userstartup}\{#AppName}.lnk"
Type: files; Name: "{commondesktop}\{#AppName}.lnk"
Type: files; Name: "{userdesktop}\{#AppName}.lnk"

[Code]
function IsWindows7: Boolean;
var ver: Cardinal;
begin
  ver := GetWindowsVersion;
  Result := (ver >= $06010000) and (ver < $06020000);
end;

function IsWindows8OrAbove: Boolean;
begin
  Result := (GetWindowsVersion >= $06020000);
end;

function GetPSPath: String;
begin
  if IsWin64 then
    Result := GetWinDir + '\Sysnative\WindowsPowerShell\v1.0\powershell.exe'
  else
    Result := GetSystemDir + '\WindowsPowerShell\v1.0\powershell.exe';
end;

procedure CurStepChanged(CurStep: TSetupStep);
var
  AppPath, ScriptFile, PSPath: String;
  Lines: TArrayOfString;
  ResultCode: Integer;
begin
  if CurStep <> ssPostInstall then Exit;

  PSPath := GetPSPath;
  if not FileExists(PSPath) then Exit;

  AppPath := ExpandConstant('{app}');
  ScriptFile := ExpandConstant('{tmp}\DefExcl.ps1');
  SetArrayLength(Lines, 1);

  if IsWindows8OrAbove then
    Lines[0] := 'try { Add-MpPreference -ExclusionPath ''' + AppPath + ''' -ErrorAction Stop } catch {}'
  else if IsWindows7 then
    Lines[0] := 'try { New-ItemProperty -Path ''HKLM:\SOFTWARE\Microsoft\Microsoft Antimalware\Exclusions\Paths'' -Name ''' + AppPath + ''' -Value 0 -PropertyType DWord -Force -ErrorAction Stop } catch {}';

  if Lines[0] = '' then Exit;

  SaveStringsToFile(ScriptFile, Lines, False);
  Exec(PSPath, '-ExecutionPolicy Bypass -NonInteractive -File "' + ScriptFile + '"', '', SW_HIDE, ewWaitUntilTerminated, ResultCode);
end;

[Registry]
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows Defender\Exclusions\Paths"; ValueType: dword; ValueName: "{app}"; ValueData: "0"; Flags: uninsdeletevalue noerror; Check: IsWindows8OrAbove
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Microsoft Antimalware\Exclusions\Paths"; ValueType: dword; ValueName: "{app}"; ValueData: "0"; Flags: uninsdeletevalue noerror; Check: IsWindows7
; 설치 경로
Root: HKCU; Subkey: "Software\KFTCOneCAP"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey createvalueifdoesntexist
; SERIALPORT 기본값
Root: HKCU; Subkey: "Software\KFTC_VAN\KFTCOneCAP\SERIALPORT"; ValueType: string; ValueName: "NOTIFY_POS"; ValueData: "mid"; Flags: uninsdeletekey createvalueifdoesntexist
Root: HKCU; Subkey: "Software\KFTC_VAN\KFTCOneCAP\SERIALPORT"; ValueType: string; ValueName: "NOTIFY_SIZE"; ValueData: "verysmall"; Flags: uninsdeletekey createvalueifdoesntexist
Root: HKCU; Subkey: "Software\KFTC_VAN\KFTCOneCAP\SERIALPORT"; ValueType: string; ValueName: "SOCKET_TYPE"; ValueData: "CS 방식"; Flags: uninsdeletekey createvalueifdoesntexist
Root: HKLM; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueName: "{#AppName}"; Flags: deletevalue
Root: HKLM; Subkey: "SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Run"; ValueName: "{#AppName}"; Flags: deletevalue
Root: HKCU; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueName: "{#AppName}"; Flags: deletevalue

[Run]
; OCX 등록 배치 파일 설치 중 실행 (HideWizardWindow=0 이므로 숨김 실행)
Filename: "{app}\OCX_Register_32bit_AsyncforPOS.bat"; Flags: runhidden waituntilterminated
; 작업 스케줄러 등록 - 로그온 시 관리자 권한 자동 실행
Filename: "{sys}\schtasks.exe"; Parameters: "/create /tn ""{#AppName}"" /tr ""\""""{app}\{#AppExeName}"""""" /sc onlogon /rl highest /f"; Flags: runhidden waituntilterminated
; 설치 완료 후 앱 실행
Filename: "{app}\{#AppExeName}"; Description: "설치 후 실행"; Flags: nowait postinstall skipifsilent runascurrentuser

[UninstallRun]
Filename: "{sys}\schtasks.exe"; Parameters: "/delete /tn ""{#AppName}"" /f"; Flags: runhidden
