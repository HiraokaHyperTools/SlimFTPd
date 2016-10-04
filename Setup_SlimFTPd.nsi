; example2.nsi
;
; This script is based on example1.nsi, but it remember the directory, 
; has uninstall support and (optionally) installs start menu shortcuts.
;
; It will install example2.nsi into a directory that the user selects,

;--------------------------------

!define APP "SlimFTPd"

; The name of the installer
Name "${APP}"

; The file to write
OutFile "Setup_${APP}.exe"

; The default installation directory
InstallDir "$PROGRAMFILES\${APP}"

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\${APP}" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------

; Pages

Page directory
Page components
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

;--------------------------------

InstType "C:\scan"
InstType "DD"

;--------------------------------

Section "${APP} サービス停止と削除"
  SectionIn 1 2
  
  ExecWait "net.exe stop SlimFTPd" $0
  DetailPrint "結果: $0"

  ExecWait "sc.exe delete SlimFTPd" $0
  DetailPrint "結果: $0"
SectionEnd

; The stuff to install
Section ""
  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Put file there
  File "src\Release\SlimFTPd.exe"
  File "src\ServiceTool.exe"
  File "src\slimftpd.conf"

  ; Write the installation path into the registry
  WriteRegStr HKLM "SOFTWARE\${APP}" "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP}" "DisplayName" "${APP}"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP}" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP}" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP}" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

Section "slimftpd.conf - C:\scan"
  SectionIn 1
  CreateDirectory "C:\scan"
  File "src\scanner\slimftpd.conf"
SectionEnd

Section "slimftpd.conf - C:\Program Files (x86)\デジタルドルフィンズV3\登録フォルダ"
  SectionIn 2
  CreateDirectory "C:\Program Files (x86)\デジタルドルフィンズV3\登録フォルダ"
  File "src\DD\slimftpd.conf"
SectionEnd

Section "${APP} サービス登録と実行"
  SectionIn 1 2

  ExecWait 'sc.exe create SlimFTPd start= auto binPath= "\"$INSTDIR\${APP}.exe\" -service"' $0
  DetailPrint "結果: $0"
  ExecWait "net.exe start SlimFTPd" $0
  DetailPrint "結果: $0"
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ExecWait "net.exe stop SlimFTPd" $0
  DetailPrint "結果: $0"

  ExecWait "sc.exe delete SlimFTPd" $0
  DetailPrint "結果: $0"

  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${APP}"
  DeleteRegKey HKLM "SOFTWARE\${APP}"

  ; Remove files and uninstaller
  Delete "$INSTDIR\SlimFTPd.exe"
  Delete "$INSTDIR\ServiceTool.exe"
  Delete "$INSTDIR\uninstall.exe"

  ; Remove directories used
  RMDir "$INSTDIR"

SectionEnd
