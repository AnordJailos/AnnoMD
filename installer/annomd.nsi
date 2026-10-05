; AnnoMD installer - normally built via `make` (see Makefile)
Unicode true
!define APPNAME "AnnoMD"
!define EXE     "AnnoMD.exe"
!ifndef VERSION
  !define VERSION "0.0.0"
!endif
!ifndef EXE_PATH
  !define EXE_PATH "AnnoMD.exe"
!endif
!ifndef ICON_PATH
  !define ICON_PATH "app.ico"
!endif
!ifndef OUT_PATH
  !define OUT_PATH "AnnoMD-Setup.exe"
!endif
!define UNKEY   "Software\Microsoft\Windows\CurrentVersion\Uninstall\AnnoMD"

Name "${APPNAME}"
OutFile "${OUT_PATH}"
InstallDir "$PROGRAMFILES64\${APPNAME}"
InstallDirRegKey HKLM "Software\AnnoMD" "InstallDir"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
BrandingText "${APPNAME} ${VERSION}"

VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "${APPNAME}"
VIAddVersionKey "FileDescription" "${APPNAME} installer"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "CompanyName" "AnordJailos"
VIAddVersionKey "LegalCopyright" "MIT License"

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

!define MUI_ICON   "${ICON_PATH}"
!define MUI_UNICON "${ICON_PATH}"
!define MUI_ABORTWARNING
!define MUI_FINISHPAGE_RUN "$INSTDIR\${EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${APPNAME}"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

; Refuse to overwrite / remove files while AnnoMD is open (it holds a named mutex while running)
!macro CheckRunning
  !define _L ${__LINE__}
  retry_${_L}:
  System::Call 'kernel32::OpenMutexW(i 0x00100000, i 0, w "AnnoMD_Running") p .R0'
  ${If} $R0 <> 0
    System::Call 'kernel32::CloseHandle(p $R0)'
    MessageBox MB_RETRYCANCEL|MB_ICONEXCLAMATION "${APPNAME} is currently running.$\r$\n$\r$\nPlease close it, then click Retry." /SD IDCANCEL IDRETRY retry_${_L}
    Abort
  ${EndIf}
  !undef _L
!macroend

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_OK|MB_ICONSTOP "${APPNAME} needs 64-bit Windows."
    Abort
  ${EndIf}
  SetRegView 64
  !insertmacro CheckRunning
FunctionEnd

Function un.onInit
  SetRegView 64
  !insertmacro CheckRunning
FunctionEnd

Section "${APPNAME} (required)" SecMain
  SectionIn RO
  SetShellVarContext all
  SetOutPath "$INSTDIR"
  File "${EXE_PATH}"
  WriteUninstaller "$INSTDIR\Uninstall.exe"

  ; Add/Remove Programs entry
  WriteRegStr   HKLM "Software\AnnoMD" "InstallDir" "$INSTDIR"
  WriteRegStr   HKLM "${UNKEY}" "DisplayName"     "${APPNAME}"
  WriteRegStr   HKLM "${UNKEY}" "DisplayVersion"  "${VERSION}"
  WriteRegStr   HKLM "${UNKEY}" "Publisher"       "AnordJailos"
  WriteRegStr   HKLM "${UNKEY}" "URLInfoAbout"    "https://github.com/AnordJailos/AnnoMD"
  WriteRegStr   HKLM "${UNKEY}" "HelpLink"        "https://github.com/AnordJailos/AnnoMD/issues"
  WriteRegStr   HKLM "${UNKEY}" "DisplayIcon"     "$INSTDIR\${EXE}"
  WriteRegStr   HKLM "${UNKEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr   HKLM "${UNKEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
  WriteRegStr   HKLM "${UNKEY}" "QuietUninstallString" '"$INSTDIR\Uninstall.exe" /S'
  WriteRegDWORD HKLM "${UNKEY}" "NoModify" 1
  WriteRegDWORD HKLM "${UNKEY}" "NoRepair" 1
  WriteRegDWORD HKLM "${UNKEY}" "EstimatedSize" 120

  ; make it show up under "Open with" for Markdown files (does not steal the default)
  WriteRegStr HKLM "Software\Classes\AnnoMD.md" "" "Markdown document"
  WriteRegStr HKLM "Software\Classes\AnnoMD.md\DefaultIcon" "" "$INSTDIR\${EXE},0"
  WriteRegStr HKLM "Software\Classes\AnnoMD.md\shell\open\command" "" '"$INSTDIR\${EXE}" "%1"'
  WriteRegStr HKLM "Software\Classes\.md\OpenWithProgids" "AnnoMD.md" ""
  WriteRegStr HKLM "Software\Classes\.markdown\OpenWithProgids" "AnnoMD.md" ""
  WriteRegStr HKLM "Software\Classes\Applications\${EXE}\shell\open\command" "" '"$INSTDIR\${EXE}" "%1"'
SectionEnd

Section "Start Menu shortcut" SecStart
  SetShellVarContext all
  CreateShortCut "$SMPROGRAMS\${APPNAME}.lnk" "$INSTDIR\${EXE}" "" "$INSTDIR\${EXE}" 0
SectionEnd

Section "Desktop shortcut" SecDesk
  SetShellVarContext all
  CreateShortCut "$DESKTOP\${APPNAME}.lnk" "$INSTDIR\${EXE}" "" "$INSTDIR\${EXE}" 0
SectionEnd

Section "Open .md files with ${APPNAME} (default app + right-click entry)" SecAssoc
  ; right-click "Edit with AnnoMD" on .md / .markdown
  WriteRegStr HKLM "Software\Classes\SystemFileAssociations\.md\shell\AnnoMD" "" "Edit with ${APPNAME}"
  WriteRegStr HKLM "Software\Classes\SystemFileAssociations\.md\shell\AnnoMD" "Icon" "$INSTDIR\${EXE}"
  WriteRegStr HKLM "Software\Classes\SystemFileAssociations\.md\shell\AnnoMD\command" "" '"$INSTDIR\${EXE}" "%1"'
  WriteRegStr HKLM "Software\Classes\SystemFileAssociations\.markdown\shell\AnnoMD" "" "Edit with ${APPNAME}"
  WriteRegStr HKLM "Software\Classes\SystemFileAssociations\.markdown\shell\AnnoMD" "Icon" "$INSTDIR\${EXE}"
  WriteRegStr HKLM "Software\Classes\SystemFileAssociations\.markdown\shell\AnnoMD\command" "" '"$INSTDIR\${EXE}" "%1"'
  ; become the default only where nothing else already is
  ReadRegStr $0 HKLM "Software\Classes\.md" ""
  ${If} $0 == ""
    WriteRegStr HKLM "Software\Classes\.md" "" "AnnoMD.md"
  ${EndIf}
  ReadRegStr $0 HKLM "Software\Classes\.markdown" ""
  ${If} $0 == ""
    WriteRegStr HKLM "Software\Classes\.markdown" "" "AnnoMD.md"
  ${EndIf}
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
SectionEnd

Section "Uninstall"
  SetShellVarContext all
  Delete "$SMPROGRAMS\${APPNAME}.lnk"
  Delete "$DESKTOP\${APPNAME}.lnk"
  Delete "$INSTDIR\${EXE}"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir  "$INSTDIR"

  DeleteRegKey HKLM "${UNKEY}"
  DeleteRegKey HKLM "Software\AnnoMD"
  DeleteRegKey HKLM "Software\Classes\AnnoMD.md"
  DeleteRegKey HKLM "Software\Classes\Applications\${EXE}"
  DeleteRegKey HKLM "Software\Classes\SystemFileAssociations\.md\shell\AnnoMD"
  DeleteRegKey HKLM "Software\Classes\SystemFileAssociations\.markdown\shell\AnnoMD"
  DeleteRegValue HKLM "Software\Classes\.md\OpenWithProgids" "AnnoMD.md"
  DeleteRegValue HKLM "Software\Classes\.markdown\OpenWithProgids" "AnnoMD.md"
  ReadRegStr $0 HKLM "Software\Classes\.md" ""
  ${If} $0 == "AnnoMD.md"
    DeleteRegValue HKLM "Software\Classes\.md" ""
  ${EndIf}
  ReadRegStr $0 HKLM "Software\Classes\.markdown" ""
  ${If} $0 == "AnnoMD.md"
    DeleteRegValue HKLM "Software\Classes\.markdown" ""
  ${EndIf}

  ; per-user settings
  SetShellVarContext current
  Delete "$APPDATA\AnnoMD\settings.ini"
  RMDir  "$APPDATA\AnnoMD"
  System::Call 'shell32::SHChangeNotify(i 0x08000000, i 0, p 0, p 0)'
SectionEnd
