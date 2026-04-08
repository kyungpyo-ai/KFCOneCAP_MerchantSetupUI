#pragma codepage 65001
; KFTCOneCAP Inno Setup Script
; Requires Inno Setup 6.x (https://jrsoftware.org/isinfo.php)

#define AppName "KFTCOneCAP"
#define AppVersion "3.0.1.300"
#define AppPublisher "KFTC"
#define AppExeName "MerchantSetup.exe"
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
SetupIconFile=D:\OneCap\RELEASE\KFTCOneCAP\KFTCOneCAP.ico
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
; 시작 프로그램 폴더 (자동 실행) - 기존 InstallFactory {StartupMenu} 방식과 동일
Name: "{commonstartup}\{#AppName}"; Filename: "{app}\{#AppExeName}"; IconFilename: "{app}\KFTCOneCAP.ico"

[Tasks]
Name: "desktopicon"; Description: "바탕화면에 아이콘 만들기"; GroupDescription: "추가 작업:"; Flags: unchecked

[Registry]
; 설치 경로
Root: HKCU; Subkey: "Software\KFTCOneCAP"; ValueType: string; ValueName: "InstallPath"; ValueData: "{app}"; Flags: uninsdeletekey createvalueifdoesntexist
; SERIALPORT 기본값
Root: HKCU; Subkey: "Software\KFTC_VAN\KFTCOneCAP\SERIALPORT"; ValueType: string; ValueName: "NOTIFY_POS"; ValueData: "mid"; Flags: uninsdeletekey createvalueifdoesntexist
Root: HKCU; Subkey: "Software\KFTC_VAN\KFTCOneCAP\SERIALPORT"; ValueType: string; ValueName: "NOTIFY_SIZE"; ValueData: "verysmall"; Flags: uninsdeletekey createvalueifdoesntexist
Root: HKCU; Subkey: "Software\KFTC_VAN\KFTCOneCAP\SERIALPORT"; ValueType: string; ValueName: "SOCKET_TYPE"; ValueData: "CS 방식"; Flags: uninsdeletekey createvalueifdoesntexist

[Run]
; OCX 등록 배치 파일 설치 중 실행 (HideWizardWindow=0 이므로 숨김 실행)
Filename: "{app}\OCX_Register_32bit_AsyncforPOS_Script.bat"; Flags: runhidden waituntilterminated
; 설치 완료 후 앱 실행
Filename: "{app}\{#AppExeName}"; Description: "설치 후 실행"; Flags: nowait postinstall skipifsilent
