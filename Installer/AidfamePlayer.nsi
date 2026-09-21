Unicode true
!include "MUI2.nsh"
!include "x64.nsh"
Name "Aidfame Player 1.0.0 Internal"
!define MUI_ICON "..\assets\icons\AidfamePlayer.ico"
!define MUI_UNICON "..\assets\icons\AidfamePlayer.ico"
OutFile "${OUTPUT}"
InstallDir "$PROGRAMFILES64\Aidfame Player"
!ifdef TEST_INSTALL
RequestExecutionLevel user
!define REGROOT HKCU
!else
RequestExecutionLevel admin
!define REGROOT HKLM
!endif
!ifdef TEST_INSTALL
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\AidfamePlayerTestHarness"
!else
!define UNINSTALL_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\AidfamePlayer"
!endif
SetCompressor /SOLID lzma
VIProductVersion "1.0.0.0"
VIAddVersionKey "ProductName" "Aidfame Player"
VIAddVersionKey "FileDescription" "Aidfame Player internal offline installer"
VIAddVersionKey "FileVersion" "1.0.0.0"
VIAddVersionKey "LegalCopyright" "Aidfame Player contributors"
!define MUI_ABORTWARNING
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "Internal-Notice.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "SimpChinese"

Function .onInit
  ${IfNot} ${RunningX64}
    MessageBox MB_ICONSTOP "Windows x64 is required."
    Abort
  ${EndIf}
  SetRegView 64
!ifndef TEST_INSTALL
  SetShellVarContext all
!endif
FunctionEnd

!macro RequireUnlockedPlayer
  ${If} ${FileExists} "$INSTDIR\AidfamePlayer.exe"
    ClearErrors
    FileOpen $0 "$INSTDIR\AidfamePlayer.exe" a
    ${If} ${Errors}
      MessageBox MB_OK|MB_ICONSTOP "Aidfame Player is running or its files are not writable. Close the player and try again." /SD IDOK
      SetErrorLevel 2
      Abort
    ${EndIf}
    FileClose $0
  ${EndIf}
!macroend

Section "Aidfame Player" SEC_APP
  !insertmacro RequireUnlockedPlayer
  SetOutPath "$INSTDIR"
  !include "${MANIFEST_INSTALL}"
  WriteUninstaller "$INSTDIR\Uninstall.exe"
  WriteRegStr ${REGROOT} "${UNINSTALL_KEY}" "DisplayName" "Aidfame Player 1.0.0 Internal"
  WriteRegStr ${REGROOT} "${UNINSTALL_KEY}" "DisplayVersion" "1.0.0"
  WriteRegStr ${REGROOT} "${UNINSTALL_KEY}" "InstallLocation" "$INSTDIR"
  WriteRegStr ${REGROOT} "${UNINSTALL_KEY}" "UninstallString" '$\"$INSTDIR\Uninstall.exe$\"'
  WriteRegDWORD ${REGROOT} "${UNINSTALL_KEY}" "NoModify" 1
  WriteRegDWORD ${REGROOT} "${UNINSTALL_KEY}" "NoRepair" 1
!ifndef TEST_INSTALL
  CreateDirectory "$SMPROGRAMS\Aidfame Player"
  CreateShortcut "$SMPROGRAMS\Aidfame Player\Aidfame Player.lnk" "$INSTDIR\AidfamePlayer.exe"
  CreateShortcut "$SMPROGRAMS\Aidfame Player\Uninstall.lnk" "$INSTDIR\Uninstall.exe"
  CreateShortcut "$DESKTOP\Aidfame Player.lnk" "$INSTDIR\AidfamePlayer.exe"
!endif
SectionEnd

Function un.onInit
  SetRegView 64
!ifndef TEST_INSTALL
  SetShellVarContext all
!endif
FunctionEnd

!macro RemoveOwnedAssociation EXT
  ReadRegStr $0 HKCU "Software\Classes\AidfamePlayer.${EXT}\shell\open\command" ""
  ${If} $0 == '$\"$INSTDIR\AidfamePlayer.exe$\" $\"%1$\"'
    DeleteRegKey HKCU "Software\Classes\AidfamePlayer.${EXT}"
    DeleteRegValue HKCU "Software\Classes\.${EXT}\OpenWithProgids" "AidfamePlayer.${EXT}"
  ${EndIf}
!macroend

Section "Uninstall"
  !insertmacro RequireUnlockedPlayer
!ifndef TEST_INSTALL
  !insertmacro RemoveOwnedAssociation "mp4"
  !insertmacro RemoveOwnedAssociation "mkv"
  !insertmacro RemoveOwnedAssociation "mov"
  !insertmacro RemoveOwnedAssociation "avi"
  ReadRegStr $0 HKCU "Software\Aidfame\AidfamePlayer\Capabilities" "ApplicationIcon"
  ${If} $0 == '$\"$INSTDIR\AidfamePlayer.exe$\",0'
    DeleteRegKey HKCU "Software\Aidfame\AidfamePlayer\Capabilities"
    ReadRegStr $0 HKCU "Software\RegisteredApplications" "Aidfame Player"
    ${If} $0 == "Software\Aidfame\AidfamePlayer\Capabilities"
      DeleteRegValue HKCU "Software\RegisteredApplications" "Aidfame Player"
    ${EndIf}
  ${EndIf}
!endif
  !include "${MANIFEST_UNINSTALL}"
  Delete "$INSTDIR\Uninstall.exe"
  RMDir "$INSTDIR"
  DeleteRegKey ${REGROOT} "${UNINSTALL_KEY}"
!ifndef TEST_INSTALL
  Delete "$DESKTOP\Aidfame Player.lnk"
  Delete "$SMPROGRAMS\Aidfame Player\Aidfame Player.lnk"
  Delete "$SMPROGRAMS\Aidfame Player\Uninstall.lnk"
  RMDir "$SMPROGRAMS\Aidfame Player"
!endif
SectionEnd
