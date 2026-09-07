; OBS Drop Uyarısı - Inno Setup kurulum betiği
;
; Derleme:
;   installer\Build-Installer.ps1
; ya da elle:
;   ISCC.exe /DMyAppVersion=1.0.0 installer\obs-drop-alert.iss
;
; Eklenti ProgramData altına kurulur. OBS Windows'ta kullanıcı eklentilerini
; YALNIZCA orada arar (frontend/widgets/OBSBasic.cpp, AddExtraModulePaths ->
; GetProgramDataPath), %APPDATA% altına konan eklenti sessizce yok sayılır.
; Bu yüzden hedef klasör değiştirilirse kullanıcı uyarılıyor.

#ifndef MyAppVersion
  #define MyAppVersion "1.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\release\RelWithDebInfo\obs-drop-alert"
#endif

#define MyAppName "OBS Drop Uyarısı"
#define MyAppModule "obs-drop-alert"
#define MyAppPublisher "yazar"
#define MyAppURL "https://github.com/yazar/obs-drop-alert"

[Setup]
AppId={{7B2F9C14-6E3A-4D51-9A88-1C0E5D3F7A62}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
VersionInfoVersion={#MyAppVersion}
VersionInfoDescription={#MyAppName} kurulumu

DefaultDirName={commonappdata}\obs-studio\plugins\{#MyAppModule}
DisableDirPage=no
DisableProgramGroupPage=yes
DisableWelcomePage=no
UninstallDisplayName={#MyAppName}
UninstallDisplayIcon={app}\bin\64bit\{#MyAppModule}.dll

LicenseFile=..\LICENSE
OutputDir=..\release
OutputBaseFilename={#MyAppModule}-{#MyAppVersion}-windows-x64-installer
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern

; Yönetici hakkı istemiyoruz: ProgramData altına standart kullanıcı da yazabilir.
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible

; OBS açıksa DLL kilitli olur; Inno kullanıcıdan kapatmasını ister.
; Mutex adı: frontend/utility/platform-windows.cpp, CheckIfAlreadyRunning()
AppMutex=OBSStudioCore

[Languages]
Name: "tr"; MessagesFile: "compiler:Languages\Turkish.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
tr.ObsNotFound=OBS Studio bu bilgisayarda bulunamadı.%n%nEklenti yine de kurulabilir, ama çalışması için OBS Studio gerekir.%n%nYine de devam edilsin mi?
en.ObsNotFound=OBS Studio was not found on this computer.%n%nThe plugin can still be installed, but it needs OBS Studio to run.%n%nContinue anyway?

tr.ObsFoundAt=Bulunan OBS Studio klasörü:
en.ObsFoundAt=OBS Studio was found at:

tr.ObsNotFoundMemo=OBS Studio bulunamadı - eklenti yine de kurulacak.
en.ObsNotFoundMemo=OBS Studio not found - the plugin will be installed anyway.

tr.DirWarning=Seçtiğin klasör OBS'in eklenti klasörünün altında değil.%n%nOBS eklentileri yalnızca şurada arar:%n%n%1%n%nBaşka bir yere kurarsan eklenti yüklenmez ve OBS bunu hata olarak da bildirmez.%n%nYine de buraya kurulsun mu?
en.DirWarning=The folder you picked is not inside the OBS plugin folder.%n%nOBS only looks for plugins in:%n%n%1%n%nIf you install anywhere else the plugin will not load, and OBS will not report an error either.%n%nInstall here anyway?

tr.LaunchObs=OBS Studio'yu şimdi başlat
en.LaunchObs=Launch OBS Studio now

tr.WhereToFind=Kurulumdan sonra OBS'i başlat. Ayarlar üst menü çubuğundaki "Drop Uyarısı" menüsünde.
en.WhereToFind=Start OBS after setup. The settings live in the "Drop Alert" menu on the main menu bar.

[Files]
Source: "{#SourceDir}\bin\64bit\{#MyAppModule}.dll"; DestDir: "{app}\bin\64bit"; Flags: ignoreversion
Source: "{#SourceDir}\data\*"; DestDir: "{app}\data"; Flags: ignoreversion recursesubdirs createallsubdirs

[Run]
Filename: "{code:GetObsExe}"; Description: "{cm:LaunchObs}"; Flags: nowait postinstall skipifsilent unchecked; Check: ObsWasFound

[UninstallDelete]
Type: filesandordirs; Name: "{app}\bin"
Type: filesandordirs; Name: "{app}\data"
Type: dirifempty; Name: "{app}"

[Code]
var
  ObsPath: String;

{ OBS kurulum klasörünü kayıt defterinden okur. OBS kurulumu bu anahtarı
  hem 64-bit hem WOW6432Node görünümüne yazar. }
function FindObsPath(): String;
var
  Value: String;
begin
  Result := '';
  if RegQueryStringValue(HKEY_LOCAL_MACHINE, 'SOFTWARE\OBS Studio', '', Value) then
    Result := Value
  else if RegQueryStringValue(HKEY_LOCAL_MACHINE_32, 'SOFTWARE\OBS Studio', '', Value) then
    Result := Value;
end;

function ObsWasFound(): Boolean;
begin
  Result := (ObsPath <> '') and FileExists(ObsPath + '\bin\64bit\obs64.exe');
end;

function GetObsExe(Param: String): String;
begin
  Result := ObsPath + '\bin\64bit\obs64.exe';
end;

function InitializeSetup(): Boolean;
begin
  ObsPath := FindObsPath();
  if ObsPath <> '' then
    Result := True
  else
    Result := MsgBox(CustomMessage('ObsNotFound'), mbConfirmation, MB_YESNO) = IDYES;
end;

{ Hedef klasör OBS'in taradığı yerin dışına alınırsa uyar: yanlış yere kurulan
  eklenti hiçbir hata vermeden çalışmaz, bu da bulunması en zor durum. }
function NextButtonClick(CurPageID: Integer): Boolean;
var
  PluginRoot: String;
  Message: String;
begin
  Result := True;
  if CurPageID = wpSelectDir then
  begin
    PluginRoot := ExpandConstant('{commonappdata}\obs-studio\plugins');
    if Pos(Uppercase(PluginRoot), Uppercase(WizardDirValue)) <> 1 then
    begin
      Message := FmtMessage(CustomMessage('DirWarning'), [PluginRoot]);
      Result := MsgBox(Message, mbConfirmation, MB_YESNO) = IDYES;
    end;
  end;
end;

function UpdateReadyMemo(Space, NewLine, MemoUserInfoInfo, MemoDirInfo, MemoTypeInfo,
  MemoComponentsInfo, MemoGroupInfo, MemoTasksInfo: String): String;
begin
  Result := MemoDirInfo + NewLine + NewLine;
  if ObsWasFound() then
    Result := Result + CustomMessage('ObsFoundAt') + NewLine + Space + ObsPath
  else
    Result := Result + CustomMessage('ObsNotFoundMemo');
  Result := Result + NewLine + NewLine + CustomMessage('WhereToFind');
end;
