/******************************************************************************
	WinGhci, a GUI for GHCI

	DlgOpts.c: options dialog code

	Original code taken from Winhugs (http://haskell.org/hugs)

	With modifications by Pepe Gallardo, 2009-March
*******************************************************************************/

#include "Combo.h"
#include "CommonIncludes.h"
#include "Colors.h"
#include "General.h"
#include "Registry.h"
#include "RtfWindow.h"
#include "Strings.h"
#include "WndMain.h"
#include "WinGhci.h"


#define MAX_STRING 3*MAX_PATH



BOOL GHCI_Flag_RevertCAFs, GHCI_Flag_PrintStats, GHCI_Flag_PrintTypes;

COMBO GHCI_Combo_Startup,  GHCI_Combo_Editor, GHCI_Combo_Prompt;




BOOL GetDlgItemBool(HWND hDlg, INT CtrlID)
{
    return (IsDlgButtonChecked(hDlg, CtrlID) == BST_CHECKED);
}

VOID SetDlgItemBool(HWND hDlg, INT CtrlID, BOOL Value)
{
    CheckDlgButton(hDlg, CtrlID, Value ? BST_CHECKED : BST_UNCHECKED);
}

typedef struct {
	BOOL	*FlagState;
	LPTSTR	GhciText;
	INT		CtrlID;
} FLAG;


FLAG flags[] = 
	{ {&GHCI_Flag_RevertCAFs, TEXT("r"), IDC_ChkRevertCAFs}
    , {&GHCI_Flag_PrintStats, TEXT("s"), IDC_ChkPrintStats}
	, {&GHCI_Flag_PrintTypes, TEXT("t"), IDC_ChkPrintTypes}
	, {NULL,NULL,0}
    };


#define FLAG_REGISTRY_PREFFIX	TEXT("GHCI_FLAG_")



VOID ReadOptionsFromRegistry(VOID)
{
 FLAG *fs;
 TCHAR Buffer[1024];
 
 for(fs=flags; fs->FlagState != NULL; fs++) {
	 wsprintf(Buffer, TEXT("%s%s"), FLAG_REGISTRY_PREFFIX, fs->GhciText);
	 *(fs->FlagState) = (BOOL) readRegInt(Buffer, FALSE);
 }


 ReadComboFromRegistry(GHCI_Combo_Startup);
 ReadComboFromRegistry(GHCI_Combo_Editor);
 ReadComboFromRegistry(GHCI_Combo_Prompt);

}


VOID WriteOptions2Registry(VOID)
{
 FLAG *fs;
 TCHAR Buffer[1024];
 
 for(fs=flags; fs->FlagState != NULL; fs++) {
	 wsprintf(Buffer, TEXT("%s%s"), FLAG_REGISTRY_PREFFIX, fs->GhciText);
	 writeRegInt(Buffer, *(fs->FlagState));
 }


 WriteCombo2Registry(GHCI_Combo_Startup);
 WriteCombo2Registry(GHCI_Combo_Editor);
 WriteCombo2Registry(GHCI_Combo_Prompt);
}


VOID MakeGhciFlagCommand(FLAG *f, LPTSTR Command)
{
	wsprintf(Command,TEXT(":%s +%s"),*(f->FlagState) ? TEXT("set") : TEXT("unset"), f->GhciText);
}

VOID MakeGhciEditorCommand(LPTSTR Editor, LPTSTR Command)
{
	wsprintf(Command,TEXT(":set editor %s"), Editor);
}

VOID MakeGhciExpandedEditorCommand(LPTSTR Editor, LPTSTR Command)
{
	if(Editor[0]==TEXT('&')) {
		wsprintf(Command,TEXT(":set editor \"%sStartProcess\" %s"), GetWinGhciInstallDir(), &Editor[1]);
	} else 
		wsprintf(Command,TEXT(":set editor \"%s\""), Editor);
}


VOID MakeGhciPromptCommand(LPTSTR Prompt, LPTSTR Command, BOOL AddMarkers) 
{
	#define SETPROMPT TEXT(":set prompt %s%s%s")
	wsprintf(Command,SETPROMPT,AddMarkers ? BEGIN_OF_PROMPT : TEXT(""),Prompt,AddMarkers ? END_OF_PROMPT : TEXT(""));
}



VOID UpdateOptions(HWND hDlg) 
{
  FLAG *fs;
  TCHAR Buffer[3*MAX_PATH];

  if(ComboHasChanged(hDlg,GHCI_Combo_Startup)) {
	  INT resp = MessageBox( hDlg
		                   , TEXT("GHCI startup has changed. The interpreter must be initialized. Do you want to proceed?")
						   , TEXT("WinGhci"), MB_YESNO|MB_ICONQUESTION);

	  if(resp==IDYES) {
		ComboUpdate(hDlg,GHCI_Combo_Startup);
		SetEvent(hKillGHCI);
		//pause StdoutPrinterThread thread
		SignalObjectAndWait(hSigSuspendStdoutPrinterThread
			               ,hSigStdoutPrinterThreadSuspended, INFINITE, FALSE);

		CreateGHCIProcess();
		Sleep(100);
		// resume StdoutPrinterThread
		SetEvent(hSigResumeStdoutPrinterThread);
	  }	
  }



 //update flags
 for(fs = flags; fs->FlagState != NULL; fs++) {
	 BOOL newState = GetDlgItemBool(hDlg, fs->CtrlID);
	 BOOL hasChanged = newState != *(fs->FlagState);
	 *(fs->FlagState) = newState;

	 if(hasChanged) {
		MakeGhciFlagCommand(fs,Buffer);
		FireCommand(Buffer);
	 }
 }

 // update editor	
 if(ComboHasChanged(hDlg,GHCI_Combo_Editor)) {
	 ComboUpdate(hDlg,GHCI_Combo_Editor);
	 MakeGhciEditorCommand(ComboGetValue(GHCI_Combo_Editor), Buffer);
	 FireCommand(Buffer);
 }


 //update prompt
 if(ComboHasChanged(hDlg,GHCI_Combo_Prompt)) {
	 ComboUpdate(hDlg,GHCI_Combo_Prompt);
	 MakeGhciPromptCommand(ComboGetValue(GHCI_Combo_Prompt), Buffer, FALSE);
	 FireCommand(Buffer);
 }


}

VOID SendOptions2GHCI(VOID)
{
	FLAG *fs;
	TCHAR Buffer[1024];

	for(fs = flags; fs->FlagState != NULL; fs++) {
		MakeGhciFlagCommand(fs,Buffer);
		SendToGHCIStdinLn(Buffer);
	}

	MakeGhciExpandedEditorCommand(ComboGetValue(GHCI_Combo_Editor), Buffer);
	SendToGHCIStdinLn(Buffer);
	 
	MakeGhciPromptCommand(ComboGetValue(GHCI_Combo_Prompt), Buffer, TRUE);
	SendToGHCIStdinLn(Buffer);

	PrintGhciOutput(hChildStdoutRd, STDOUT_COLOR);

	// clear screen, so that only one instance of the prompt is shown
	RtfWindowClearLastLine();
	SendToGHCIStdinLn(TEXT(""));
	PrintGhciOutput(hChildStdoutRd, STDOUT_COLOR);

}


INT CALLBACK ListAllFonts(CONST LOGFONT* lpelfe, CONST TEXTMETRIC* lpntme, DWORD FontType, LPARAM lParam)
{
    HWND hLst = (HWND) lParam;
    LPCTSTR FontName = (LPCTSTR) lpelfe->lfFaceName;
    if (SendMessage(hLst, CB_FINDSTRINGEXACT, -1, (LPARAM) FontName) == CB_ERR)
	SendMessage(hLst, CB_ADDSTRING, 0, (LPARAM) FontName);
    return 1;
}


VOID CalculateFont(HWND hDlg, CHARFORMAT* cf)
{
    BOOL ValidSize;
    INT NewSize;
    INT CurSel;
    HWND hFace = GetDlgItem(hDlg, IDC_LstFontFace);

    RegistryReadFont(cf);

    CurSel = (INT) SendMessage(hFace, CB_GETCURSEL, 0, 0);
    if (CurSel == CB_ERR)
	GetWindowText(hFace, cf->szFaceName, 32);
    else
	SendMessage(hFace, CB_GETLBTEXT, CurSel, (LPARAM) cf->szFaceName);

    cf->dwEffects = 0;
    cf->dwEffects |= (GetDlgItemBool(hDlg, IDC_ChkFontBold) ? CFE_BOLD : 0);
    cf->dwEffects |= (GetDlgItemBool(hDlg, IDC_ChkFontItalic) ? CFE_ITALIC : 0);

    // check the size
    NewSize = GetDlgItemInt(hDlg, IDC_TxtFontSize, &ValidSize, FALSE);
    if (ValidSize) cf->yHeight = PointToTwip(NewSize);
}

VOID UpdateFontPreview(HWND hDlg)
{
    CHARFORMAT cf;
    HWND hRTF = GetDlgItem(hDlg, IDC_RtfPreview);

    CalculateFont(hDlg, &cf);
    SendMessage(hRTF, EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &cf);
}

VOID InitOptionsFont(HWND hDlg)
{
    // load up the list of fonts
    HDC hDC = GetDC(hDlg);
    CHARFORMAT cf;
    LOGFONT lf;

    SendDlgItemMessage(hDlg, IDC_SpnFontSize, UDM_SETRANGE, 0, MAKELONG(72, 6));

    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfFaceName[0] = TEXT('\0');;
    lf.lfPitchAndFamily = 0;
    EnumFontFamiliesEx(hDC, &lf, ListAllFonts, (LPARAM) GetDlgItem(hDlg, IDC_LstFontFace), 0);
    ReleaseDC(hDlg, hDC);

    SetDlgItemText(hDlg, IDC_RtfPreview, TEXT("Text Preview ABC abc 123"));

    // setup the config options
    RegistryReadFont(&cf);
    SetDlgItemText(hDlg, IDC_LstFontFace, cf.szFaceName);
    SetDlgItemBool(hDlg, IDC_ChkFontBold, cf.dwEffects & CFE_BOLD);
    SetDlgItemBool(hDlg, IDC_ChkFontItalic, cf.dwEffects & CFE_ITALIC);
    SetDlgItemInt(hDlg, IDC_TxtFontSize, TwipToPoint(cf.yHeight), FALSE);

	SetDlgItemBool(hDlg, IDC_ChkRevertCAFs, GHCI_Flag_RevertCAFs);
	SetDlgItemBool(hDlg, IDC_ChkPrintStats, GHCI_Flag_PrintStats);
	SetDlgItemBool(hDlg, IDC_ChkPrintTypes, GHCI_Flag_PrintTypes);


    UpdateFontPreview(hDlg);
}




INT_PTR CALLBACK OptsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

BOOL ShowOptsDialog(VOID)
{
    return DialogBox(hThisInstance, MAKEINTRESOURCE(DLG_OPTS), hWndMain, OptsDlgProc);
}



INT_PTR CALLBACK OptsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
		case WM_INITDIALOG:
			CenterDialogInParent(hDlg);
			InitOptionsFont(hDlg);
			ComboInitDialog(hDlg,GHCI_Combo_Editor);
			ComboInitDialog(hDlg,GHCI_Combo_Startup);
			ComboInitDialog(hDlg,GHCI_Combo_Prompt);

			break;

		case WM_COMMAND: {
				INT Code = HIWORD(wParam);
				INT Ctrl = LOWORD(wParam);

				if ((Ctrl == IDC_ChkFontBold && Code == BN_CLICKED) ||
					(Ctrl == IDC_ChkFontItalic && Code == BN_CLICKED) ||
					(Ctrl == IDC_TxtFontSize && Code == EN_CHANGE) ||
					(Ctrl == IDC_LstFontFace && (Code == CBN_EDITCHANGE || Code == CBN_SELCHANGE))
					)
					UpdateFontPreview(hDlg);


			switch (LOWORD(wParam)) {
				case IDOK: {

					CHARFORMAT cf, prevCf;

					RegistryReadFont(&prevCf);
					CalculateFont(hDlg, &cf);

					if( (StringCmp(cf.szFaceName,prevCf.szFaceName)) 
						  || (cf.yHeight != prevCf.yHeight)  
						  || (cf.dwEffects != prevCf.dwEffects)) {

							RegistryWriteFont(&cf);
							RtfWindowUpdateFont();
					}

					UpdateOptions(hDlg) ;

					EndDialog(hDlg, TRUE);
					return (INT_PTR)TRUE;
				}
				case IDCANCEL:
					EndDialog(hDlg, TRUE);
					return (INT_PTR)TRUE;
			}
		}
		break;
	}
	return (INT_PTR)FALSE;
}


VOID InitOptions(VOID)
{
	GHCI_Combo_Startup = NewCombo( TEXT("GHCI_STARTUP_COMBO")
		                         , IDC_GHCI_Combo_Startup
								 , TEXT("ghc --interactive"));
	GHCI_Combo_Editor = NewCombo( TEXT("GHCI_EDITOR_COMBO")
		                        , IDC_GHCI_Combo_Editor
								, TEXT("notepad"));
	GHCI_Combo_Prompt = NewCombo( TEXT("GHCI_PROMPT_COMBO")
		                        , IDC_GHCI_Combo_Prompt
								, TEXT("%s>"));
	ReadOptionsFromRegistry();
}

VOID FinalizeOptions(VOID)
{
	WriteOptions2Registry();

	FreeCombo(GHCI_Combo_Startup);
	FreeCombo(GHCI_Combo_Editor);
	FreeCombo(GHCI_Combo_Prompt);
}
