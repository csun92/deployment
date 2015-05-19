; Script generated by the HM NIS Edit Script Wizard.

; HM NIS Edit Wizard helper defines
!define PRODUCT_NAME "Medipy"
!define PRODUCT_VERSION "1.0"
!define PRODUCT_PUBLISHER "Université de Strasbourg"
!define PRODUCT_WEB_SITE "http://medipy.u-strasbg.fr/doc/medipy/"
!define PRODUCT_DIR_REGKEY "Software\Microsoft\Windows\CurrentVersion\App Paths\itkTestDriver.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
!define PRODUCT_UNINST_ROOT_KEY "HKLM"

!include "LogicLib.nsh"
!include "EnvVarUpdate.nsh"
!include "x64.nsh"
; MUI 1.67 compatible ------
!include "MUI.nsh"

; MUI Settings
!define MUI_ABORTWARNING
!define MUI_ICON "${NSISDIR}\Contrib\Graphics\Icons\modern-install.ico"
!define MUI_UNICON "${NSISDIR}\Contrib\Graphics\Icons\modern-uninstall.ico"

; Welcome page
!insertmacro MUI_PAGE_WELCOME
; License page
!insertmacro MUI_PAGE_LICENSE "medipy\LICENSE.txt"
; Components page
!insertmacro MUI_PAGE_COMPONENTS
; Directory page
!insertmacro MUI_PAGE_DIRECTORY
; Instfiles page
!insertmacro MUI_PAGE_INSTFILES
; Finish page
!insertmacro MUI_PAGE_FINISH

; Uninstaller pages
!insertmacro MUI_UNPAGE_INSTFILES

; Language files
!insertmacro MUI_LANGUAGE "English"

; MUI end ------

Var PYTHONPATH
Function getPythonPath
  ReadRegStr $PYTHONPATH HKLM Software\Python\PythonCore\2.7\InstallPath ""
FunctionEnd

Name "${PRODUCT_NAME} ${PRODUCT_VERSION}"
OutFile "medipy_setup.exe"
InstallDir "$PROGRAMFILES64\Medipy"
InstallDirRegKey HKLM "${PRODUCT_DIR_REGKEY}" ""
ShowInstDetails show
ShowUnInstDetails show

Function .onInit
  SetRegView 64
  call getPythonPath
  ${If} $PYTHONPATH == ""
    MessageBox MB_OK "Could not install Medipy. No supported python is installed(python 2.7)"
    Abort
  ${EndIf}
FunctionEnd

Function setSystemPath
  ${EnvVarUpdate} $0 "PATH" "A" "HKLM"  "$PROGRAMFILES64\ITK\bin"
  ${EnvVarUpdate} $0 "PATH" "A" "HKLM"  "$PROGRAMFILES64\VTK\bin"
  ${EnvVarUpdate} $0 "PATH" "A" "HKLM"  "$PROGRAMFILES64\DCMTK\bin"
  ${EnvVarUpdate} $0 "PATH" "A" "HKLM"  "$PROGRAMFILES64\GDCM\bin"
  ${EnvVarUpdate} $0 "MEDIPY_HOME" "P" "HKLM"  "$INSTDIR\"
  ${EnvVarUpdate} $0 "MEDIPY_PLUGINS_PATH" "P" "HKLM"  "$INSTDIR\plugins\"
  ${EnvVarUpdate} $0 "PYTHONPATH" "A" "HKLM"  "$INSTDIR\lib\;$INSTDIR\lib\Release"
FunctionEnd

Function .onInstSuccess
  call setSystemPath
FunctionEnd

Section "Medipy 1.0" SEC01
  SetOutPath "$INSTDIR"
  SetOverwrite try
  File /r "medipy\"
  ${DisableX64FSRedirection}
  SetOutPath "$WINDIR\System32\"
  Setoverwrite off
  File "vcomp90.dll"
  ${EnableX64FSRedirection}
SectionEnd

Section "ITK 4.6.1" SEC02
  SetOutPath "$PROGRAMFILES64\ITK\"
  File /r "ITK\"
  SetOutPath "$PYTHONPATH\Lib\site-packages\"
  File "ITK\WrapITK.pth"
SectionEnd

Section "VTK 5.8" SEC03
  SetOutPath "$PROGRAMFILES64\VTK\"
  File /r "VTK\"
  SetOutPath "$PYTHONPATH\Lib\site-packages\"
  File /r "VTK\lib\site-packages\"
SectionEnd

Section "DCMTK 3.6.0" SEC04
  SetOutPath "$PROGRAMFILES64\DCMTK\"
  File /r "DCMTK\"
SectionEnd

Section "GDCM 2.2" SEC05
  SetOutPath "$PROGRAMFILES64\GDCM\"
  File /r "GDCM\"
SectionEnd

Section -Post
  WriteUninstaller "$INSTDIR\uninst.exe"
  WriteRegStr HKLM "${PRODUCT_DIR_REGKEY}" "" "$PROGRAMFILES\ITK\bin\itkTestDriver.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayName" "$(^Name)"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "UninstallString" "$INSTDIR\uninst.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$PROGRAMFILES\ITK\bin\itkTestDriver.exe"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "URLInfoAbout" "${PRODUCT_WEB_SITE}"
  WriteRegStr ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
SectionEnd

; Section descriptions
!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC01} "Install Medipy"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC02} "Install ITK support"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC03} "Install VTK support"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC04} "Install DCMTK support"
  !insertmacro MUI_DESCRIPTION_TEXT ${SEC05} "Install GDCM support"
!insertmacro MUI_FUNCTION_DESCRIPTION_END


Function un.onUninstSuccess
  HideWindow
  MessageBox MB_ICONINFORMATION|MB_OK "$(^Name) was successfully removed from your computer."
FunctionEnd

Function un.onInit
  MessageBox MB_ICONQUESTION|MB_YESNO|MB_DEFBUTTON2 "Are you sure you want to completely remove $(^Name) and all of its components?" IDYES +2
  Abort
FunctionEnd

Section Uninstall
  RMDir /r "$PROGRAMFILES64\ITK\"
  RMDir /r "$PROGRAMFILES64\VTK\"
  RMDir /r "$PROGRAMFILES64\DCMTK\"
  RMDir /r "$PROGRAMFILES64\GDCM\"
  RMDir /r "$INSTDIR"
  
  ${un.EnvVarUpdate} $0 "PATH" "R" "HKLM"  "$PROGRAMFILES64\ITK\bin"
  ${un.EnvVarUpdate} $0 "PATH" "R" "HKLM"  "$PROGRAMFILES64\VTK\bin"
  ${un.EnvVarUpdate} $0 "PATH" "R" "HKLM"  "$PROGRAMFILES64\DCMTK\bin"
  ${un.EnvVarUpdate} $0 "PATH" "R" "HKLM"  "$PROGRAMFILES64\GDCM\bin"
  ${un.EnvVarUpdate} $0 "MEDIPY_HOME" "R" "HKLM"  "$INSTDIR\"
  ${un.EnvVarUpdate} $0 "MEDIPY_PLUGINS_PATH" "R" "HKLM"  "$INSTDIR\plugins\"
  ${un.EnvVarUpdate} $0 "PYTHONPATH" "R" "HKLM"  "$INSTDIR\lib\;$INSTDIR\lib\Release"

  DeleteRegKey ${PRODUCT_UNINST_ROOT_KEY} "${PRODUCT_UNINST_KEY}"
  DeleteRegKey HKLM "${PRODUCT_DIR_REGKEY}"
  SetAutoClose true
SectionEnd