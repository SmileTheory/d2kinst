;--------------------------------

!include nsDialogs.nsh
!include LogicLib.nsh

; The name of the installer
Name "D2K Installer"

; The file to write
OutFile "D2KInst.exe"

; The default installation directory
InstallDir $PROGRAMFILES\Dune2000

; Registry key to check for directory (so if you install again, it will 
; overwrite the old one automatically)
InstallDirRegKey HKLM "Software\Dune2000" "Install_Dir"

; Request application privileges for Windows Vista
RequestExecutionLevel admin

;--------------------------------

Var Dialog
Var Label
Var D2kCdPathDirRequest
Var D2kCdPathBrowse
Var D2kCdPath
Var ResourceCfgFile

; Pages

Page custom selectD2kCdPage selectD2kCdPageLeave
Page components
Page directory
Page instfiles

UninstPage uninstConfirm
UninstPage instfiles

Function BrowseForD2kCdPath
	nsDialogs::SelectFolderDialog "Select Dune 2000 CD path..." ""
	Pop $D2kCdPath
	${NSD_SetText} $D2kCdPathDirRequest $D2kCdPath
FunctionEnd

Function selectD2kCdPage

  nsDialogs::Create 1018
  Pop $Dialog

	${If} $Dialog == error
		Abort
	${EndIf}

	${NSD_CreateLabel} 0 0 100% 12u "Please enter the path to your Dune 2000 CD here:"
	Pop $Label

	${NSD_CreateDirRequest} 0 13u 70% 13u ""
	Pop $D2kCdPathDirRequest

	${NSD_CreateButton} 75% 13u 25% 13u "Browse..."
	Pop $D2kCdPathBrowse

	${NSD_OnClick} $D2kCdPathBrowse BrowseForD2kCdPath

	nsDialogs::Show

FunctionEnd

Function selectD2kCdPageLeave
	${NSD_GetText} $D2kCdPathDirRequest $D2kCdPath
	IfFileExists $D2kCdPath\SETUP\SETUP.Z +3 0
		MessageBox MB_OK "Dune 2000 CD not found!"
		Abort
FunctionEnd

;--------------------------------

; The stuff to install
Section "Dune 2000 (required)"

  SectionIn RO
  
  ; Set output path to the installation directory.
  SetOutPath $INSTDIR
  
  ; Extract extractor
  File "unZ.exe"
  
  ; Extract files from CD
  ExecWait '"$INSTDIR\unZ.exe" "$D2kCdPath\SETUP\SETUP.Z" "$INSTDIR"'
  
  ; Overwrite RESOURCE.CFG
  FileOpen $ResourceCfgFile $INSTDIR\DUNE\RESOURCE.CFG w
  FileWrite $ResourceCfgFile "data\$\r$\n"
  FileWrite $ResourceCfgFile "$D2kCdPath\movies\$\r$\n"
  FileWrite $ResourceCfgFile "$D2kCdPath\music\$\r$\n"
  FileWrite $ResourceCfgFile "$D2kCdPath\missions\$\r$\n"
  FileWrite $ResourceCfgFile "data\maps\$\r$\n"
  FileClose $ResourceCfgFile
  
  ; Write the installation path into the registry
  WriteRegStr HKLM SOFTWARE\Dune2000 "Install_Dir" "$INSTDIR"
  
  ; Write the uninstall keys for Windows
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Dune2000" "DisplayName" "Dune2000"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Dune2000" "UninstallString" '"$INSTDIR\uninstall.exe"'
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Dune2000" "NoModify" 1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Dune2000" "NoRepair" 1
  WriteUninstaller "uninstall.exe"
  
SectionEnd

;Section "Custom cnc-ddraw.dll"
;   File /oname=DUNE\ddraw.dll ddraw.dll
;SectionEnd

; Optional section (can be disabled by the user)
Section "Start Menu Shortcuts"

  CreateDirectory "$SMPROGRAMS\Dune2000"
  CreateShortCut "$SMPROGRAMS\Dune2000\Uninstall.lnk" "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0
  CreateShortCut "$SMPROGRAMS\Dune2000\Dune2000.lnk" "$INSTDIR\DUNE\Dune2000.exe" "" "$INSTDIR\DUNE\Dune2000.exe" 0
  
SectionEnd

;--------------------------------

; Uninstaller

Section "Uninstall"
  
  ; Remove registry keys
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\Dune2000"
  DeleteRegKey HKLM SOFTWARE\Dune2000

  ; Remove shortcuts, if any
  Delete "$SMPROGRAMS\Dune2000\*.*"

  ; Remove directories used
  RMDir "$SMPROGRAMS\Dune2000"
  RMDir /r "$INSTDIR"

SectionEnd
