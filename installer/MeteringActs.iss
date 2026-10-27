#define MyAppName "MeteringActs"
#define MyAppVersion "1.0.0"
#define MyAppPublisher "МРЭТ"
#define MyAppExeName "MeteringActs.exe"

[Setup]
AppId={{F923BA89-671A-4DB9-A83A-62D17D87F185}

AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}

DefaultDirName={autopf}\{#MyAppName}
DefaultGroupName={#MyAppName}

OutputDir=output
OutputBaseFilename=MeteringActs-{#MyAppVersion}-Setup

SetupIconFile=..\resources\windows\meteringacts.ico
WizardImageFile=images\installer_wizard.bmp
WizardSmallImageFile=images\installer_wizard_small.bmp
UninstallDisplayIcon={app}\{#MyAppExeName}

Compression=lzma2
SolidCompression=yes

PrivilegesRequired=admin
WizardStyle=modern

ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible



[Files]
Source: "..\release\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\MeteringActs"; Filename: "{app}\{#MyAppExeName}"

Name: "{autodesktop}\MeteringActs"; Filename: "{app}\{#MyAppExeName}"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Создать ярлык на рабочем столе"; GroupDescription: "Дополнительные ярлыки:"; Flags: unchecked

[Run]
Filename: "{app}\{#MyAppExeName}"; Description: "Запустить MeteringActs"; Flags: nowait postinstall skipifsilent