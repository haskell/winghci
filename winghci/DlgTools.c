/******************************************************************************
	WinGHCi, a GUI for GHCi

	DlgTools.c: tools dialog code

	By Pepe Gallardo, 2009-March
*******************************************************************************/

#include "Combo.h"
#include "CommonIncludes.h"
#include "General.h"
#include "Registry.h"
#include "Strings.h"
#include "WinGHCi.h"
#include "WndMain.h"


#define IsDefined(xs) (!StringIsEmpty(xs))

typedef struct {
	LPTSTR ToolName;
	LPTSTR ToolHelp;
	LPTSTR ToolCmd;
} Tool;

#define MAX_TOOLS	10

INT nTools = 0;
Tool Tools[MAX_TOOLS];

#define MAX_STRING	1024

#define TOOLS_NAME_FMT	TEXT("ToolName%02d")
#define TOOLS_HELP_FMT	TEXT("ToolHelp%02d")
#define TOOLS_CMD_FMT	TEXT("ToolCmd%02d")



#define FILENAME_KEY	TEXT("<fileName>")
#define FILEEXT_KEY		TEXT("<fileExt>")



#define DEFAULT_NAME		TEXT("GHC compiler")
#define DEFAULT_HELP		TEXT("Compile current file with GHC")
#define DEFAULT_CMD			(TEXT(":! ghc --make \"") FILENAME_KEY FILEEXT_KEY TEXT("\""))


VOID WriteTools2Registry(VOID);


COMBO Tools_Combo_Name, Tools_Combo_Help, Tools_Combo_Cmd;

LPTSTR ToolGetHelp(INT pos)
{
	if(!IsDefined(Tools[pos].ToolCmd))
		return(TEXT("The command for this entry is undefined"));
	else if (!IsDefined(Tools[pos].ToolHelp))
		return Tools[pos].ToolName;
	else
		return Tools[pos].ToolHelp;
}

VOID ToolFireCommand(INT pos)
{
	TCHAR Buf[4*1024], BufAux[4*1024];

	TCHAR CpLastFileLoaded[MAX_PATH];
    TCHAR fileName[MAX_PATH];
    TCHAR fileExt[MAX_PATH];



	BOOL hasQuotes = LastFileLoaded[0]==TEXT('\"');

	// remove quotes
	StringCpy(CpLastFileLoaded,hasQuotes ? &LastFileLoaded[1] : LastFileLoaded);
	if(hasQuotes)
		CpLastFileLoaded[StringLen(CpLastFileLoaded)-1] = TEXT('\0');


	if(!IsDefined(CpLastFileLoaded) &&
		( StringSearchString(Tools[pos].ToolCmd,FILENAME_KEY) 
		  ||
	      StringSearchString(Tools[pos].ToolCmd,FILEEXT_KEY)
		  )) {
			  MessageBox(hWndMain,TEXT("No file currently loaded"),TEXT("WinGHCi"),MB_OK|MB_ICONHAND);
			  return;
	}



    _tsplitpath (CpLastFileLoaded, NULL, NULL, fileName, fileExt);


	StringReplaceAllOccurrences(Tools[pos].ToolCmd, FILENAME_KEY, fileName, Buf);
	StringReplaceAllOccurrences(Buf, FILEEXT_KEY, fileExt, BufAux);

	//wsprintf(Buf,Tools[pos].ToolCmd,LastFileLoaded);
	if(!Running)
		FireCommand(BufAux);
}


VOID UpdateToolsMenu(VOID)
{
	INT i, j;
	HMENU hMenu;
	TCHAR Buf[128+MAX_STRING];
	INT Enable, menusEnabled = 0;

	for(i=0;GetMenuItemID(GetSubMenu(GetMenu(hWndMain), i),0)!=ID_TOOLS_TOOL1;i++);

	hMenu = GetSubMenu(GetMenu(hWndMain), i);

	for(i=0;i<MAX_TOOLS;i++) {	
		Enable = IsDefined(Tools[i].ToolCmd);
		menusEnabled += Enable;

		j = i+1;
		j = j > 9 ? j -10 : j;
		if(IsDefined(Tools[i].ToolName)) {
			wsprintf(Buf,TEXT("%s\tCtrl+%d"),Tools[i].ToolName, j);
		} else {
			wsprintf(Buf,TEXT("Tool %d\tCtrl+%d"),i+1,j);			
		}
		ModifyMenu(hMenu, ID_TOOLS_TOOL1+i, MF_BYCOMMAND|MF_STRING|(Enable ? 0 : MF_DISABLED), ID_TOOLS_TOOL1+i, Buf);
	}

	if(menusEnabled==0) {
		//if nothing is defined, add GHC as a default tool

		StringCpy(Tools[0].ToolName, DEFAULT_NAME);
		StringCpy(Tools[0].ToolHelp, DEFAULT_HELP);
		StringCpy(Tools[0].ToolCmd,  DEFAULT_CMD);
		UpdateToolsMenu();
		WriteTools2Registry();
	}
}

VOID ReadToolsFromRegistry(VOID) 
{
	INT i;
	TCHAR Buf[MAX_STRING];

	for(i=0;i<MAX_TOOLS;i++) {
		LPTSTR Res;

		wsprintf(Buf,TOOLS_NAME_FMT,i);
		Res = readRegStrDup(Buf, TEXT("\0"));
		StringCpy(Tools[i].ToolName,Res);
		free(Res);


		wsprintf(Buf,TOOLS_HELP_FMT,i);
		Res = readRegStrDup(Buf, TEXT("\0"));
		StringCpy(Tools[i].ToolHelp,Res);
		free(Res);

		wsprintf(Buf,TOOLS_CMD_FMT,i);
		Res = readRegStrDup(Buf, TEXT("\0"));
		StringCpy(Tools[i].ToolCmd,Res);
		free(Res);
	}

 ReadComboFromRegistry(Tools_Combo_Name);
 ReadComboFromRegistry(Tools_Combo_Help);
 ReadComboFromRegistry(Tools_Combo_Cmd);
}


VOID WriteTools2Registry(VOID)
{
	INT i;
	TCHAR Buf[MAX_STRING];

	for(i=0;i<MAX_TOOLS;i++) {
		
		wsprintf(Buf,TOOLS_NAME_FMT,i);
		writeRegString(Buf, Tools[i].ToolName);
		
		wsprintf(Buf,TOOLS_HELP_FMT,i);
		writeRegString(Buf, Tools[i].ToolHelp);

		wsprintf(Buf,TOOLS_CMD_FMT,i);
		writeRegString(Buf, Tools[i].ToolCmd);
	}

 WriteCombo2Registry(Tools_Combo_Name);
 WriteCombo2Registry(Tools_Combo_Help);
 WriteCombo2Registry(Tools_Combo_Cmd);

}






VOID SelectTool(HWND hDlg, INT *currPos)
{
	HWND hLst = GetDlgItem(hDlg, IDC_LstTools);
	INT pos = SendMessage(hLst,CB_GETCURSEL,0,0);

	ComboGetDlgText(hDlg, Tools_Combo_Name, Tools[*currPos].ToolName, MAX_STRING);
	ComboGetDlgText(hDlg, Tools_Combo_Help, Tools[*currPos].ToolHelp, MAX_STRING);
	ComboGetDlgText(hDlg, Tools_Combo_Cmd, Tools[*currPos].ToolCmd, MAX_STRING);

	ComboUpdate(hDlg, Tools_Combo_Name);
	ComboUpdate(hDlg, Tools_Combo_Help);
	ComboUpdate(hDlg, Tools_Combo_Cmd);

	ComboSetDlgText(hDlg,Tools_Combo_Name,Tools[pos].ToolName);
	ComboSetDlgText(hDlg,Tools_Combo_Help,Tools[pos].ToolHelp);
	ComboSetDlgText(hDlg,Tools_Combo_Cmd,Tools[pos].ToolCmd);

	*currPos = pos;

}

VOID InitDlgTools(HWND hDlg)
{
	INT i;
	HWND hLst = GetDlgItem(hDlg, IDC_LstTools);
	TCHAR Buffer[1024];

	for(i=0;i<MAX_TOOLS;i++) {
		wsprintf(Buffer,TEXT("%d"),i+1);
		SendMessage(hLst, CB_ADDSTRING, 0, (LPARAM) Buffer);
	} 


	ComboInitDialog(hDlg,Tools_Combo_Name);
	ComboInitDialog(hDlg,Tools_Combo_Help);
	ComboInitDialog(hDlg,Tools_Combo_Cmd);

	SendMessage(hLst,CB_SETCURSEL,0,0);

	ComboSetDlgText(hDlg,Tools_Combo_Name,Tools[0].ToolName);
	ComboSetDlgText(hDlg,Tools_Combo_Help,Tools[0].ToolHelp);
	ComboSetDlgText(hDlg,Tools_Combo_Cmd,Tools[0].ToolCmd);
}


INT_PTR CALLBACK ToolsDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{

	static INT currPos;

	switch (msg) {
		case WM_INITDIALOG:
			CenterDialogInParent(hDlg);
			InitDlgTools(hDlg);
			currPos = 0;
			break;

		case WM_COMMAND: {
				INT Code = HIWORD(wParam);
				INT Ctrl = LOWORD(wParam);

				if ((Ctrl == IDC_LstTools) && (Code == CBN_EDITCHANGE || Code == CBN_SELCHANGE)) {
					SelectTool(hDlg,&currPos);
					break;
				}


			switch (LOWORD(wParam)) {
				case IDOK: {

					// get data in dialog
					SelectTool(hDlg,&currPos);

					WriteTools2Registry();
					UpdateToolsMenu();
					EndDialog(hDlg, TRUE);
					return (INT_PTR)TRUE;
				}
				case IDCANCEL:
					ReadToolsFromRegistry();
					EndDialog(hDlg, TRUE);
					return (INT_PTR)TRUE;
			}
		}
		break;
	}
	return (INT_PTR)FALSE;
}


BOOL ShowToolsDialog(VOID)
{
    return DialogBox(hThisInstance, MAKEINTRESOURCE(DLG_TOOLS), hWndMain, ToolsDlgProc);
}


VOID InitTools(VOID)
{
	INT i;

	Tools_Combo_Name = NewCombo( TEXT("Tools_name_combo")
		                         , IDC_Tools_Combo_Name
								 , DEFAULT_NAME);
	Tools_Combo_Help = NewCombo( TEXT("Tools_help_combo")
		                         , IDC_Tools_Combo_Help
								 , DEFAULT_HELP);
	Tools_Combo_Cmd = NewCombo( TEXT("Tools_cmd_combo")
		                         , IDC_Tools_Combo_Cmd
								 , DEFAULT_CMD);


	for(i=0;i<MAX_TOOLS;i++) {
		Tools[i].ToolName = malloc(MAX_STRING*sizeof(TCHAR));
		Tools[i].ToolHelp = malloc(MAX_STRING*sizeof(TCHAR));
		Tools[i].ToolCmd  = malloc(MAX_STRING*sizeof(TCHAR));
	}
	ReadToolsFromRegistry();
	UpdateToolsMenu();

}


VOID FinalizeTools(VOID)
{
	WriteTools2Registry();

	FreeCombo(Tools_Combo_Name);
	FreeCombo(Tools_Combo_Help);
	FreeCombo(Tools_Combo_Cmd);
}
