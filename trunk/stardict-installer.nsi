; Installer script for win32 StarDict
; Hu Zheng <huzheng_001@163.com>

;--------------------------------
;Configuration

;General

!define STARDICT_VERSION		"2.4.8"

Name "StarDict ${STARDICT_VERSION}"

OutFile "stardict-${STARDICT_VERSION}.exe"

XPStyle on

; SetCompressor bzip2

ShowInstDetails show
ShowUninstDetails show
SetDateSave on

; $INSTDIR is set in .onInit function..

!include "MUI.nsh"
!include Sections.nsh

;--------------------------------
;Defines

!define MUI_ICON			.\stardict.ico
!define MUI_UNICON			.\stardict-uninst.ico
!define MUI_SPECIALBITMAP		.\src\win32\nsis\stardict-intro.bmp
!define MUI_HEADERBITMAP		.\src\win32\nsis\stardict-header.bmp

!define STARDICT_NSIS_INCLUDE_PATH		".\src\win32\nsis"

!define STARDICT_REG_KEY			"SOFTWARE\stardict"
!define STARDICT_UNINSTALL_KEY		"SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\StarDict"
!define HKLM_APP_PATHS_KEY		"SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\stardict.exe"
!define STARDICT_UNINST_EXE			"stardict-uninst.exe"

!define GTK_VERSION			"2.6.0"
!define GTK_REG_KEY			"SOFTWARE\GTK\2.0"
!define GTK_INSTALL_VERIFIER		"bin\libgtk-win32-2.0-0.dll"



;--------------------------------
;Modern UI Configuration

  !define MUI_CUSTOMPAGECOMMANDS

  !define MUI_WELCOMEPAGE
  !define MUI_LICENSEPAGE
  !define MUI_DIRECTORYPAGE
  !define MUI_FINISHPAGE
  
  !define MUI_ABORTWARNING

  !define MUI_UNINSTALLER
  !define MUI_UNCONFIRMPAGE

;--------------------------------
;Pages
  
  !insertmacro MUI_PAGE_WELCOME
  !insertmacro MUI_PAGE_LICENSE			"./COPYING"
  !insertmacro MUI_PAGE_DIRECTORY
  !insertmacro MUI_PAGE_INSTFILES
  !insertmacro MUI_PAGE_FINISH

  !insertmacro MUI_UNPAGE_CONFIRM
  !insertmacro MUI_UNPAGE_INSTFILES
  !insertmacro MUI_UNPAGE_FINISH

;--------------------------------
;Languages
 
  !insertmacro MUI_LANGUAGE "English"

;--------------------------------
;Language Strings
  LangString GTK_INSTALLER_NEEDED		${LANG_ENGLISH} \
		"The GTK+ runtime environment is either missing or needs to be upgraded.$\r \
		Please install v${GTK_VERSION} or higher of the GTK+ runtime"


  ; StarDict Section Prompts and Texts
  LangString STARDICT_UNINSTALL_DESC		${LANG_ENGLISH} \
		"StarDict (remove only)"
  LangString STARDICT_PROMPT_WIPEOUT		${LANG_ENGLISH} \
		"You're old StarDict directory is about to be deleted. Would you like to continue?$\r$\r \
		StarDict user settings will not be affected."
  LangString STARDICT_PROMPT_DIR_EXISTS		${LANG_ENGLISH} \
		"The installation directory you specified already exists. Any contents $\r \
		it may have will be deleted. Would you like to continue?"

  ; Uninstall Section Prompts
  LangString un.STARDICT_UNINSTALL_ERROR_1         	${LANG_ENGLISH} \
		"The uninstaller could not find registry entries for StarDict.$\r \
		It is likely that another user installed this application."
  LangString un.STARDICT_UNINSTALL_ERROR_2         	${LANG_ENGLISH} \
		"You do not have permission to uninstall this application."

;--------------------------------
;Data
  
  LicenseData "./COPYING"

;--------------------------------
;Uninstall any old version of StarDict

Section -SecUninstallOldStarDict
  ; Check install rights..
  Call CheckUserInstallRights
  Pop $R0

  StrCmp $R0 "HKLM" stardict_hklm
  StrCmp $R0 "HKCU" stardict_hkcu done

  stardict_hkcu:
      ReadRegStr $R1 HKCU ${STARDICT_REG_KEY} ""
      ReadRegStr $R2 HKCU ${STARDICT_REG_KEY} "Version"
      ReadRegStr $R3 HKCU "${STARDICT_UNINSTALL_KEY}" "UninstallString"
      Goto try_uninstall

  stardict_hklm:
      ReadRegStr $R1 HKLM ${STARDICT_REG_KEY} ""
      ReadRegStr $R2 HKLM ${STARDICT_REG_KEY} "Version"
      ReadRegStr $R3 HKLM "${STARDICT_UNINSTALL_KEY}" "UninstallString"

  ; If previous version exists .. remove
  try_uninstall:
    StrCmp $R1 "" done
      StrCmp $R2 "" uninstall_problem
        ; Check if we have uninstall string..
        IfFileExists $R3 0 uninstall_problem
          ; Have uninstall string.. go ahead and uninstall.
          SetOverwrite on
          ; Need to copy uninstaller outside of the install dir
          ClearErrors
          CopyFiles /SILENT $R3 "$TEMP\${STARDICT_UNINST_EXE}"
          SetOverwrite off
          IfErrors uninstall_problem
            ; Ready to uninstall..
            ClearErrors
	    ExecWait '"$TEMP\${STARDICT_UNINST_EXE}" /S _?=$R1'
	    IfErrors exec_error
              Delete "$TEMP\${STARDICT_UNINST_EXE}"
	      Goto done

	    exec_error:
              Delete "$TEMP\${STARDICT_UNINST_EXE}"
              Goto uninstall_problem

        uninstall_problem:
	  ; In this case just wipe out previous StarDict install dir..
          MessageBox MB_YESNO $(STARDICT_PROMPT_WIPEOUT) IDYES do_wipeout IDNO cancel_install
          cancel_install:
            Quit

          do_wipeout:
            StrCmp $R0 "HKLM" stardict_del_lm_reg stardict_del_cu_reg
            stardict_del_cu_reg:
              DeleteRegKey HKCU ${STARDICT_REG_KEY}
              Goto uninstall_prob_cont
            stardict_del_lm_reg:
              DeleteRegKey HKLM ${STARDICT_REG_KEY}

            uninstall_prob_cont:
	      RMDir /r "$R1"

  done:
SectionEnd

;--------------------------------
;StarDict Install Section

Section SecStarDict
  SectionIn 1 RO

  ; Check install rights..
  Call CheckUserInstallRights
  Pop $R0

  ; Get GTK+ lib dir if we have it..

  StrCmp $R0 "NONE" stardict_none
  StrCmp $R0 "HKLM" stardict_hklm stardict_hkcu

  stardict_hklm:
    ReadRegStr $R1 HKLM ${GTK_REG_KEY} "Path"
    WriteRegStr HKLM "${HKLM_APP_PATHS_KEY}" "Path" "$R1\bin"
    WriteRegStr HKLM ${STARDICT_REG_KEY} "" "$INSTDIR"
    WriteRegStr HKLM ${STARDICT_REG_KEY} "Version" "${STARDICT_VERSION}"
    WriteRegStr HKLM "${STARDICT_UNINSTALL_KEY}" "DisplayName" $(STARDICT_UNINSTALL_DESC)
    WriteRegStr HKLM "${STARDICT_UNINSTALL_KEY}" "UninstallString" "$INSTDIR\${STARDICT_UNINST_EXE}"
    ; Sets scope of the desktop and Start Menu entries for all users.
    SetShellVarContext "all"
    Goto stardict_install_files

  stardict_hkcu:
    ReadRegStr $R1 HKCU ${GTK_REG_KEY} "Path"
    StrCmp $R1 "" 0 stardict_hkcu1
      ReadRegStr $R1 HKLM ${GTK_REG_KEY} "Path"
    stardict_hkcu1:
    WriteRegStr HKCU ${STARDICT_REG_KEY} "" "$INSTDIR"
    WriteRegStr HKCU ${STARDICT_REG_KEY} "Version" "${STARDICT_VERSION}"
    WriteRegStr HKCU "${STARDICT_UNINSTALL_KEY}" "DisplayName" $(STARDICT_UNINSTALL_DESC)
    WriteRegStr HKCU "${STARDICT_UNINSTALL_KEY}" "UninstallString" "$INSTDIR\${STARDICT_UNINST_EXE}"
    Goto stardict_install_files

  stardict_none:
    ReadRegStr $R1 HKLM ${GTK_REG_KEY} "Path"

  stardict_install_files:
    SetOutPath "$INSTDIR"
    ; StarDict files
    SetOverwrite on
    File /r /x StarDict.api .\win32-install-dir\*.*

    Call InstallWordPick

    ; If we don't have install rights and no hklm GTK install.. then Start in lnk property should
    ; remain stardict dir.. otherwise it should be set to the GTK lib dir. (to avoid dll hell)
    StrCmp $R0 "NONE" 0 startin_gtk
      StrCmp $R1 "" startin_stardict
    startin_gtk:
      SetOutPath "$R1\bin"
    startin_stardict:
    CreateDirectory "$SMPROGRAMS\StarDict"
    CreateShortCut "$SMPROGRAMS\StarDict\StarDict.lnk" "$INSTDIR\stardict.exe"
    CreateShortCut "$DESKTOP\StarDict.lnk" "$INSTDIR\stardict.exe"
    SetOutPath "$INSTDIR"

    ; If we don't have install rights.. we're done
    StrCmp $R0 "NONE" done
    CreateShortCut "$SMPROGRAMS\StarDict\Uninstall.lnk" "$INSTDIR\${STARDICT_UNINST_EXE}"
    SetOverwrite off

    ; write out uninstaller
    SetOverwrite on
    WriteUninstaller "$INSTDIR\${STARDICT_UNINST_EXE}"
    SetOverwrite off

  done:
SectionEnd ; end of default StarDict section


;--------------------------------
;Uninstaller Section


Section Uninstall
  Call un.CheckUserInstallRights
  Pop $R0
  StrCmp $R0 "NONE" no_rights
  StrCmp $R0 "HKCU" try_hkcu try_hklm

  try_hkcu:
    ReadRegStr $R0 HKCU ${STARDICT_REG_KEY} ""
    StrCmp $R0 $INSTDIR 0 cant_uninstall
      ; HKCU install path matches our INSTDIR.. so uninstall
      DeleteRegKey HKCU ${STARDICT_REG_KEY}
      DeleteRegKey HKCU "${STARDICT_UNINSTALL_KEY}"
      Goto cont_uninstall

  try_hklm:
    ReadRegStr $R0 HKLM ${STARDICT_REG_KEY} ""
    StrCmp $R0 $INSTDIR 0 try_hkcu
      ; HKLM install path matches our INSTDIR.. so uninstall
      DeleteRegKey HKLM ${STARDICT_REG_KEY}
      DeleteRegKey HKLM "${STARDICT_UNINSTALL_KEY}"
      DeleteRegKey HKLM "${HKLM_APP_PATHS_KEY}"
      ; Sets start menu and desktop scope to all users..
      SetShellVarContext "all"

  cont_uninstall:
    RMDir "$INSTDIR\dic"
    RMDir "$INSTDIR\treedict"
    RMDir /r "$INSTDIR\locale"
    RMDir /r "$INSTDIR\sounds"
    RMDir /r "$INSTDIR\help"
    RMDir /r "$INSTDIR\pixmaps"
    Delete "$INSTDIR\stardict.exe"
    Delete "$INSTDIR\TextOutSpy.dll"
    Delete "$INSTDIR\TextOutHook.dll"
    Delete "$INSTDIR\${STARDICT_UNINST_EXE}"
    ;Try to remove StarDict install dir .. if empty
    RMDir "$INSTDIR"

    Call un.DeleteWordPick

    ; Shortcuts..
    RMDir /r "$SMPROGRAMS\StarDict"
    Delete "$DESKTOP\StarDict.lnk"

    Goto done

  cant_uninstall:
    MessageBox MB_OK $(un.STARDICT_UNINSTALL_ERROR_1) IDOK
    Quit

  no_rights:
    MessageBox MB_OK $(un.STARDICT_UNINSTALL_ERROR_2) IDOK
    Quit

  done:
SectionEnd ; end of uninstall section


;--------------------------------
;Functions

Function CheckUserInstallRights
	ClearErrors
	UserInfo::GetName
	IfErrors Win9x
	Pop $0
	UserInfo::GetAccountType
	Pop $1

	StrCmp $1 "Admin" 0 +3
                StrCpy $1 "HKLM"
		Goto done
	StrCmp $1 "Power" 0 +3
                StrCpy $1 "HKLM"
		Goto done
	StrCmp $1 "User" 0 +3
		StrCpy $1 "HKCU"
		Goto done
	StrCmp $1 "Guest" 0 +3
		StrCpy $1 "NONE"
		Goto done
	; Unknown error
	StrCpy $1 "NONE"
        Goto done

	Win9x:
		StrCpy $1 "HKLM"

	done:
        Push $1
FunctionEnd

Function un.CheckUserInstallRights
	ClearErrors
	UserInfo::GetName
	IfErrors Win9x
	Pop $0
	UserInfo::GetAccountType
	Pop $1
	StrCmp $1 "Admin" 0 +3
                StrCpy $1 "HKLM"
		Goto done
	StrCmp $1 "Power" 0 +3
                StrCpy $1 "HKLM"
		Goto done
	StrCmp $1 "User" 0 +3
		StrCpy $1 "HKCU"
		Goto done
	StrCmp $1 "Guest" 0 +3
		StrCpy $1 "NONE"
		Goto done
	; Unknown error
	StrCpy $1 "NONE"
        Goto done

	Win9x:
		StrCpy $1 "HKLM"

	done:
        Push $1
FunctionEnd

;
; Usage:
;   Push $0 ; Path string
;   Call VerifyDir
;   Pop $0 ; 0 - Bad path  1 - Good path
;
Function VerifyDir
  Pop $0
  Loop:
    IfFileExists $0 dir_exists
    StrCpy $1 $0 ; save last
    Push $0
    Call GetParent
    Pop $0
    StrLen $2 $0
    ; IfFileExists "C:" on xp returns true and on win2k returns false
    ; So we're done in such a case..
    StrCmp $2 "2" loop_done
    Goto Loop

  loop_done:
    StrCpy $1 "$0\StarDicTFooB"
    ; Check if we can create dir on this drive..
    ClearErrors
    CreateDirectory $1
    IfErrors DirBad DirGood

  dir_exists:
    ClearErrors
    FileOpen $1 "$0\stardictfoo.bar" w
    IfErrors PathBad PathGood

    DirGood:
      RMDir $1
      Goto PathGood1

    DirBad:
      RMDir $1
      Goto PathBad1

    PathBad:
      FileClose $1
      Delete "$0\stardictfoo.bar"
      PathBad1:
      StrCpy $0 "0"
      Push $0
      Return

    PathGood:
      FileClose $1
      Delete "$0\stardictfoo.bar"
      PathGood1:
      StrCpy $0 "1"
      Push $0
FunctionEnd

Function .onVerifyInstDir
  Push $INSTDIR
  Call VerifyDir
  Pop $0
  StrCmp $0 "0" 0 dir_good
    Abort
  dir_good:
FunctionEnd

; GetParent
; input, top of stack  (e.g. C:\Program Files\Poop)
; output, top of stack (replaces, with e.g. C:\Program Files)
; modifies no other variables.
;
; Usage:
;   Push "C:\Program Files\Directory\Whatever"
;   Call GetParent
;   Pop $R0
;   ; at this point $R0 will equal "C:\Program Files\Directory"
Function GetParent
   Exch $0 ; old $0 is on top of stack
   Push $1
   Push $2
   StrCpy $1 -1
   loop:
     StrCpy $2 $0 1 $1
     StrCmp $2 "" exit
     StrCmp $2 "\" exit
     IntOp $1 $1 - 1
   Goto loop
   exit:
     StrCpy $0 $0 $1
     Pop $2
     Pop $1
     Exch $0 ; put $0 on top of stack, restore $0 to original value
FunctionEnd


; CheckGtkVersion
; inputs: Push 2 GTK+ version strings to check. The major value needs to
; be equal and the minor value needs to be greater or equal.
;
; Usage:
;   Push "2.1.0"  ; Refrence version
;   Push "2.2.1"  ; Version to check
;   Call CheckGtkVersion
;   Pop $R0
;   $R0 will now equal "1", because 2.2 is greater than 2.1
;
Function CheckGtkVersion
  ; Version we want to check
  Pop $6 
  ; Reference version
  Pop $8 

  ; Check that the string to check is at least 5 chars long (i.e. x.x.x)
  StrLen $7 $6
  IntCmp $7 5 0 bad_version

  ; Major version check
  StrCpy $7 $6 1
  StrCpy $9 $8 1
  IntCmp $7 $9 check_minor bad_version bad_version

  check_minor:
    StrCpy $7 $6 1 2
    StrCpy $9 $8 1 2
    IntCmp $7 $9 good_version bad_version good_version

  bad_version:
    StrCpy $6 "0"
    Push $6
    Goto done

  good_version:
    StrCpy $6 "1"
    Push $6
  done:
FunctionEnd

;
; Usage:
; Call DoWeNeedGtk
; First Pop:
;   0 - We have the correct version
;       Second Pop: Key where Version was found
;   1 - We have an old version that needs to be upgraded
;       Second Pop: HKLM or HKCU depending on where GTK was found.
;   2 - We don't have Gtk+ at all
;       Second Pop: "NONE, HKLM or HKCU" depending on our rights..
;
Function DoWeNeedGtk
  ; Logic should be:
  ; - Check what user rights we have (HKLM or HKCU)
  ;   - If HKLM rights..
  ;     - Only check HKLM key for GTK+
  ;       - If installed to HKLM, check it and return.
  ;   - If HKCU rights..
  ;     - First check HKCU key for GTK+
  ;       - if good or bad exists stop and ret.
  ;     - If no hkcu gtk+ install, check HKLM
  ;       - If HKLM ver exists but old, return as if no ver exits.
  ;   - If no rights
  ;     - Check HKLM

  Call CheckUserInstallRights
  Pop $3
  StrCmp $3 "HKLM" check_hklm
  StrCmp $3 "HKCU" check_hkcu check_hklm
    check_hkcu:
      ReadRegStr $0 HKCU ${GTK_REG_KEY} "Version"
      StrCpy $5 "HKCU"
      StrCmp $0 "" check_hklm have_gtk

    check_hklm:
      ReadRegStr $0 HKLM ${GTK_REG_KEY} "Version"
      StrCpy $5 "HKLM"
      StrCmp $0 "" no_gtk have_gtk


  have_gtk:
    ; GTK+ is already installed.. check version.
    StrCpy $1 ${GTK_VERSION} ; Minimum GTK+ version needed
    Push $1
    Push $0
    Call CheckGtkVersion
    Pop $2
    StrCmp $2 "1" good_version bad_version
    bad_version:
      ; Bad version. If hklm ver and we have hkcu or no rights.. return no gtk
      StrCmp $3 "NONE" no_gtk  ; if no rights.. can't upgrade
      StrCmp $3 "HKCU" 0 upgrade_gtk ; if HKLM can upgrade..
        StrCmp $5 "HKLM" no_gtk upgrade_gtk ; have hkcu rights.. if found hklm ver can't upgrade..
  
      upgrade_gtk:
        StrCpy $2 "1"
        Push $5
        Push $2
        Goto done

  good_version:
    StrCmp $5 "HKLM" have_hklm_gtk have_hkcu_gtk
      have_hkcu_gtk:
        ; Have HKCU version
        ReadRegStr $4 HKCU ${GTK_REG_KEY} "Path"
        Goto good_version_cont

      have_hklm_gtk:
        ReadRegStr $4 HKLM ${GTK_REG_KEY} "Path"
        Goto good_version_cont

    good_version_cont:
      StrCpy $2 "0"
      Push $4  ; The path to existing GTK+
      Push $2
      Goto done

  no_gtk:
    StrCpy $2 "2"
    Push $3 ; our rights
    Push $2
    Goto done

  done:
FunctionEnd

Function .onInit
  ; If this installer dosn't have GTK, check whether we need it.
    Call DoWeNeedGtk
    Pop $0
    Pop $1

    StrCmp $0 "0" have_gtk need_gtk
    need_gtk:
      MessageBox MB_OK $(GTK_INSTALLER_NEEDED) IDOK
      Quit
    have_gtk:

  Call CheckUserInstallRights
  Pop $0

  StrCmp $0 "HKLM" 0 user_dir
    StrCpy $INSTDIR "$PROGRAMFILES\StarDict"
    Goto instdir_done
  user_dir:
    StrCpy $2 "$SMPROGRAMS"
    Push $2
    Call GetParent
    Call GetParent
    Pop $2
    StrCpy $INSTDIR "$2\StarDict"

  instdir_done:

FunctionEnd

Function InstallWordPick
  ReadRegStr $R8 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\AcroRd32.exe" "Path"
  StrCmp $R8 "" next1 copyfile1
  copyfile1:
    SetOutPath "$R8\plug_ins"
    File .\win32-install-dir\StarDict.api
  next1:

  ReadRegStr $R9 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\Acrobat.exe" "Path"
  StrCmp $R9 "" next2 copyfile2
  copyfile2:
    SetOutPath "$R9\plug_ins"
    File .\win32-install-dir\StarDict.api
  next2:

  SetOutPath "$INSTDIR"
FunctionEnd

Function un.DeleteWordPick
  ReadRegStr $R8 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\AcroRd32.exe" "Path"
  StrCmp $R8 "" next1 deletefile1
  deletefile1:
    Delete "$R8\plug_ins\StarDict.api"
  next1:

  ReadRegStr $R9 HKLM "SOFTWARE\Microsoft\Windows\CurrentVersion\App Paths\Acrobat.exe" "Path"
  StrCmp $R9 "" next2 deletefile2
  deletefile2:
    Delete "$R9\plug_ins\StarDict.api"
  next2:

FunctionEnd
