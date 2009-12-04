/******************************************************************************
	WinGHCi, a GUI for GHCi

	WndMain.c: WinGHCi main dialog

	Original code taken from Winhugs (http://haskell.org/hugs)

	With modifications by Pepe Gallardo, 2009-March
*******************************************************************************/

#include "CommonIncludes.h"
#include "Colors.h"
#include "DlgAbout.h"
#include "DlgOpts.h"
#include "DlgTools.h"
#include "General.h"
#include "History.h"
#include "MruFiles.h"
#include "resource.h"
#include "Registry.h"
#include "RtfWindow.h"
#include "Strings.h"
#include <Shlwapi.h>
#include <shlobj.h>
#include "Toolbar.h"
#include "Utf8.h"
#include "WndMain.h"
#include "WinGHCi.h"


VOID EnableButtons(VOID);

TCHAR LastFileLoaded[MAX_PATH] = TEXT("");

// returns NULL if CommandPreffix is not command Command
// returns pointer to next token otherwise
LPTSTR IsCommand(LPTSTR Command, LPTSTR CommandPreffix)
{
	// avoid case of empty string
	if (StringIsEmpty(CommandPreffix))
		return NULL;

	while(*Command && *CommandPreffix && (*Command == *CommandPreffix)) {
		Command++;
		CommandPreffix++;
	}

	if(StringIsEmpty(CommandPreffix))
		return CommandPreffix;
	else if (*CommandPreffix == TEXT(' ')) {
		while(*CommandPreffix == TEXT(' '))
			CommandPreffix++;
		return CommandPreffix;
	} else
		return NULL;
}

VOID PreprocessCommand(LPCTSTR InputCommand, LPTSTR NewCommand)
{
	LPTSTR path, opt, prompt, flag, editor, Command = (LPTSTR) InputCommand;
	TCHAR Buffer[MAX_PATH];
	
	StringCpy(NewCommand,Command);


	// skip initial spaces
	while(*Command == TEXT(' '))
		Command++;

	if(*Command == TEXT(':'))
		Command++;
	else
		// it is not a command
		return;

	
	if (path=IsCommand(TEXT("cd"), Command)) {
		// in case the user typed ":cd <dir>", update WinGHCi current dir
		if(StringIsEmpty(path)) {
			// get user home dir
			SHGetFolderPath(NULL,CSIDL_PROFILE,NULL,SHGFP_TYPE_CURRENT,Buffer);
		    path = Buffer;
		}
		SetCurrentDirectory(path);
		StringCpy(LastFileLoaded,TEXT("")); // no file loaded after changing dir
    
	} else if (IsCommand(TEXT("quit"), Command)) {
		SendMessage(hWndMain,WM_CLOSE,0,0);
	} else if (opt=IsCommand(TEXT("set"),Command)) {

		if(prompt=StringIsPreffix(TEXT("prompt "), opt)) {
			//skip whitespaces
			while(*prompt==TEXT(' ')) prompt++;
			
			// in case the user wants to change the prompt, add markers
			ComboAdd(GHCi_Combo_Prompt,prompt);
			MakeGHCiPromptCommand(prompt, NewCommand, TRUE);
		} else if (editor=StringIsPreffix(TEXT("editor "), opt)) {
			ComboAdd(GHCi_Combo_Editor,editor);
			MakeGHCiExpandedEditorCommand(editor, NewCommand);
		} else if (flag=StringIsPreffix(TEXT("+r"), opt)) {
			GHCi_Flag_RevertCAFs = TRUE;
		} else if (flag=StringIsPreffix(TEXT("+s"), opt)) {
			GHCi_Flag_PrintStats = TRUE;
		} else if (flag=StringIsPreffix(TEXT("+t"), opt)) {
			GHCi_Flag_PrintTypes = TRUE;
		}

	} else if (opt=IsCommand(TEXT("unset"),Command)) {

		if (flag=StringIsPreffix(TEXT("+r"), opt)) {
			GHCi_Flag_RevertCAFs = FALSE;
		} else if (flag=StringIsPreffix(TEXT("+s"), opt)) {
			GHCi_Flag_PrintStats = FALSE;
		} else if (flag=StringIsPreffix(TEXT("+t"), opt)) {
			GHCi_Flag_PrintTypes = FALSE;
		}

	} else if (path=IsCommand(TEXT("load"),Command)) {
		StringCpy(LastFileLoaded,path); // a new file is loaded
	}
	
	
	/* else if (path=IsCommand(TEXT("load"), Command)) {
		LoadFile(path);
		StringCpy(NewCommand,TEXT(""));
	}*/
}


typedef struct {
	BOOL	WaitForResponse, StopStdoutPrinterThread, WantPreprocess;
	LPTSTR	Command;
} FireCommandThreadArgs;

VOID FireCommandAux(LPCTSTR Command, BOOL WaitForResponse, BOOL startThread, BOOL StopStdoutPrinterThread, BOOL WantPreprocess, FireCommandThreadArgs *pArgs) 
{

	#define MAXLEN    (3*1024)

	TCHAR NewCommand[MAXLEN];

	if(StopStdoutPrinterThread)
		//stop StdoutPrinterThread
		SignalObjectAndWait( hSigSuspendStdoutPrinterThread
			                ,hSigStdoutPrinterThreadSuspended, INFINITE, FALSE);


	RtfWindowSetCommand(Command);
	RtfWindowStartOutput();
	AddHistory(Command);

	if(WantPreprocess) {
		PreprocessCommand(Command, NewCommand);
		SendToGHCiStdinLn(NewCommand);
	}
	else
		SendToGHCiStdinLn(Command);


	if(WaitForResponse) {
		Running = TRUE;
		EnableButtons();
		
		PrintGHCiOutput(hChildStdoutRd, STDOUT_COLOR);
		
		Running = FALSE;
		EnableButtons();

	}

	// resume StdoutPrinterThread
	if(StopStdoutPrinterThread)
		SetEvent(hSigResumeStdoutPrinterThread);

	if(startThread) {
		// free arguments
		free((LPTSTR)Command);
		free(pArgs);
	}
}




DWORD WINAPI FireCommandThread( LPVOID lpParam ) 
{
    FireCommandThreadArgs *pArgs = (FireCommandThreadArgs*) lpParam;

	FireCommandAux(pArgs->Command, pArgs->WaitForResponse, TRUE, pArgs->StopStdoutPrinterThread, pArgs->WantPreprocess, pArgs) ;

	return 0;
}



VOID FireCommandExt(LPCTSTR Command, BOOL WaitForResponse, BOOL startThread, BOOL StopStdoutPrinterThread, BOOL WantPreprocess)
{ 
	if(startThread) {

		DWORD ThreadId;
		FireCommandThreadArgs *pArgs = (FireCommandThreadArgs*) malloc(sizeof(FireCommandThreadArgs));


		pArgs->WaitForResponse = WaitForResponse;
		pArgs->StopStdoutPrinterThread = StopStdoutPrinterThread;
		pArgs->Command = StringDup(Command);
		pArgs->WantPreprocess = WantPreprocess;

		if(!CreateThread(NULL,0,FireCommandThread,pArgs,0,&ThreadId))
			ErrorExit(TEXT("CreateThread FireCommandThread failed\n"));
	} else
			FireCommandAux(Command, WaitForResponse, startThread, StopStdoutPrinterThread, WantPreprocess, NULL);

}

VOID FireCommand(LPCTSTR Command)
{
	FireCommandExt(Command,TRUE,FALSE,TRUE,TRUE);
}

VOID FireAsyncCommand(LPCTSTR Command)
{
	FireCommandExt(Command,TRUE,TRUE,TRUE,TRUE);
}

VOID EnableButton(INT id, BOOL Enable)
{
	TBBUTTONINFO tbi;
	tbi.cbSize = sizeof(tbi);
	tbi.dwMask = TBIF_STATE;
	tbi.fsState = (Enable ? TBSTATE_ENABLED : 0);
	SendDlgItemMessage(hWndMain, IDC_Toolbar, TB_SETBUTTONINFO, id, (LPARAM) &tbi);

	EnableMenuItem(GetMenu(hWndMain), id,
		MF_BYCOMMAND | (Enable ? MF_ENABLED : MF_GRAYED));
}

VOID EnableButtons(VOID)
{
	INT CopyState = (Running ? 0 : RtfWindowCanCutCopy());

	EnableButton(ID_FILE_LOAD, !Running);
	EnableButton(ID_FILE_ADD, !Running);

	EnableButton(ID_STOP, Running);
	EnableButton(ID_RUN, !Running);

	EnableButton(ID_CUT, CopyState & DROPEFFECT_MOVE);
	EnableButton(ID_DELETE, CopyState & DROPEFFECT_MOVE);
	EnableButton(ID_COPY, CopyState & DROPEFFECT_COPY);
	EnableButton(ID_PASTE, !Running);
	EnableButton(ID_CLEARSCREEN, !Running);
	EnableButton(ID_SELECTALL, !Running);	
	EnableButton(ID_COMPILE, !Running);
	EnableButton(ID_GOEDIT, !Running);
	EnableButton(ID_CLEARALL, !Running);	
	EnableButton(ID_SETOPTIONS, !Running);
	EnableButton(ID_TOOLS_CONFIGURE, !Running);
	EnableButton(ID_HELPCOMMANDS, !Running);

}

VOID SetStatusBar(LPCTSTR Str)
{
	SendMessage(hWndStatus,SB_SETTEXT,SB_SIMPLEID|SBT_NOBORDERS, (LPARAM)Str);
}



VOID LoadFileFromFileDialog(HWND hWnd)
{
	TCHAR FileName[MAX_PATH];

	if (ShowOpenFileDialog(hWnd, FileName)) {
		LoadFile(FileName);
	}
}

VOID AddFileFromFileDialog(HWND hWnd)
{
	TCHAR FileName[MAX_PATH], RelPath[MAX_PATH], CurrentWorkingDir[MAX_PATH];
	TCHAR Command[2*MAX_PATH];

    GetCurrentDirectory(MAX_PATH,CurrentWorkingDir);

	if (ShowOpenFileDialog(hWnd, FileName)) {
		AddMruFile(FileName);
		

		PathRelativePathTo(RelPath,
                       CurrentWorkingDir,
                       FILE_ATTRIBUTE_DIRECTORY,
                       FileName,
                       FILE_ATTRIBUTE_NORMAL);


		wsprintf(Command, TEXT(":add %s"), ExpandFileName(RelPath));
		FireCommand(Command);
	}

    SetCurrentDirectory(CurrentWorkingDir);
}



DWORD WINAPI AbortExecutionThread(LPVOID lpParam)
{
	INT i=0;
	#define MAX_TRIES 4


	SetEvent(hEventCtrlBreak);
	MessageBox(hWndMain,TEXT("Interrupted"), TEXT("WinGHCi"), MB_ICONSTOP);

	while(Running && (i<MAX_TRIES)) {
		//SetEvent(hSigStopPrintGHCiOutput);
		//if(!CreateThread(NULL,0,SendLn,NULL,0,&ThreadId))
		//	ErrorExit(TEXT("CreateThread SendLn failed\n"));
		SetEvent(hEventCtrlBreak);
		Sleep(100);
		i++;

	}	
	

	if(Running) {
		SetEvent(hSigStopPrintGHCiOutput);		
		SendToGHCiStdinLn(TEXT(" "));	
	}

	return 0;


}



VOID AbortExecution(VOID)
{
	DWORD ThreadId;

	if(!CreateThread(NULL,0,AbortExecutionThread,NULL,0,&ThreadId))
			ErrorExit(TEXT("CreateThread AbortExecutionThread failed\n"));

#if 0
	if(Running) {
		SetEvent(hSigStopPrintGHCiOutput);
		//if(!CreateThread(NULL,0,SendLn,NULL,0,&ThreadId))
		//	ErrorExit(TEXT("CreateThread SendLn failed\n"));

	}
#endif 

}




DWORD WINAPI MainCommandThread(LPVOID lpParam)
{
	HWND hWnd = hWndMain;
	INT ID = (INT) lpParam;

	switch (ID) {
		case IDCANCEL: 
			EndDialog(hWnd, 0); 
			break;
		case ID_FILE_LOAD: 
			LoadFileFromFileDialog(hWnd); 
			break;
		case ID_FILE_ADD: 
			AddFileFromFileDialog(hWnd); 
			break;
		case ID_EXIT: 
			SendMessage(hWndMain, WM_CLOSE,0,0); 
			break;
		/* Load one of the last 10 open files */
		case ID_MRU+0: case ID_MRU+1: case ID_MRU+2: case ID_MRU+3:
		case ID_MRU+4: case ID_MRU+5: case ID_MRU+6: case ID_MRU+7:
		case ID_MRU+8: case ID_MRU+9:
			LoadFile(GetMruFile(ID-ID_MRU));
			break;

		case ID_CUT: 
			RtfWindowClipboard(WM_CUT); 
			break;
		case ID_COPY: 
			RtfWindowClipboard(WM_COPY); 
			break;
		case ID_PASTE: 
			RtfWindowClipboard(WM_PASTE); 
			break;
		case ID_CLEARSCREEN: 
			RtfWindowClear(); 
			break;
		case ID_DELETE: 
			RtfWindowDelete(); 
			break;
		case ID_SELECTALL: 
			RtfWindowSelectAll(); 
			break;
		case ID_GOPREVIOUS: 
			RtfWindowRelativeHistory(-1); 
			break;
		case ID_GONEXT: 
			RtfWindowRelativeHistory(+1); 
			break;


		case ID_COMPILE: 
			FireCommand(TEXT(":reload")); 
			break;
		case ID_CLEARALL: 
			FireCommand(TEXT(":load")); 
			break;
		case ID_GOEDIT: 
			FireCommand(TEXT(":edit")); 
			break;

		/* Stop program execution */
		case ID_STOP:
			AbortExecution();
			break;

		case ID_TOOLS_TOOL1:
		case ID_TOOLS_TOOL2:
		case ID_TOOLS_TOOL3:
		case ID_TOOLS_TOOL4:
		case ID_TOOLS_TOOL5:
		case ID_TOOLS_TOOL6:
		case ID_TOOLS_TOOL7:
		case ID_TOOLS_TOOL8:
		case ID_TOOLS_TOOL9:
		case ID_TOOLS_TOOL10:		
			ToolFireCommand(ID-ID_TOOLS_TOOL1);
			break;

		/* Evaluate main expression */
		case ID_RUN:
			{
				#define BUFF_LEN 2048
				TCHAR Buffer[BUFF_LEN];
				RtfWindowGetCommand(Buffer,BUFF_LEN);
				if (StringIsEmpty(Buffer))
					FireCommand(TEXT(":main"));
				else
					FireCommand(Buffer);
			}
			break;

		/* Set interpreter options using dialog box */
		case ID_SETOPTIONS:
			ShowOptsDialog();
			break;

		case ID_TOOLS_CONFIGURE:
			ShowToolsDialog();
			break;

			// HELP MENU
		case ID_HELPCONTENTS:  {
			ExecuteFileDocs(TEXT("index.html"));
			break;
							   }
		case ID_HELPCOMMANDS: 
			FireCommand(TEXT(":?")); 
			break;
		case ID_LIBRARIES: 
			ExecuteFileDocs(TEXT("libraries\\index.html")); 
			break;
		case ID_WWWHASKELL: 
			ExecuteFile(TEXT("http://haskell.org/")); 
			break;
		case ID_WWWGHC: 
			ExecuteFile(TEXT("http://haskell.org/ghc/")); 
			break;
		case ID_ABOUT: 
			ShowAboutDialog(); 
			break;
	}
	return 0;
}



VOID MainCommand(HWND hWnd, INT ID)
{
	DWORD ThreadId;

	if(!CreateThread(NULL,0,MainCommandThread,(LPVOID) ID,0,&ThreadId))
			ErrorExit(TEXT("CreateThread MainCommandThread failed\n"));
}

#if 0
extern DWORD StartOfInput;
VOID LoadFile(LPTSTR File)
{
	TCHAR Command[2*MAX_PATH], fName[MAX_PATH], fExt[MAX_PATH], Buffer[MAX_PATH];

	//Move the current directory
	SetWorkingDirToFileLoc(File);

	

	// get file name and extension
	_tsplitpath (File, NULL, NULL, fName, fExt);
	wsprintf(Buffer,TEXT("%s%s"),fName,fExt);

	wsprintf(Command, TEXT(":load %s"), ExpandFileName(Buffer));
	AddMruFile(File);


	RtfWindowSetCommand(TEXT(":load "));
	StartOfInput += 6;	
	WinGHCiHyperlink(ExpandFileName(Buffer));
	StartOfInput += StringLen(ExpandFileName(Buffer));
	
	RtfWindowStartOutput();
	AddHistory(Command);
	SendToGHCiStdinLn(Command);

	Running = TRUE;
	EnableButtons();
	
	PrintGHCiOutputIfAvailable(hChildStderrRd, STDERR_COLOR);
	PrintGHCiOutput(hChildStdoutRd, STDOUT_COLOR);
	
	Running = FALSE;
	EnableButtons();
}
#else
VOID LoadFileExt(LPTSTR File, BOOL async)
{
	TCHAR Command[2*MAX_PATH], fName[MAX_PATH], fExt[MAX_PATH], Buffer[MAX_PATH];

	//Move the current directory
	SetWorkingDirToFileLoc(File,TRUE);
	
	AddMruFile(File);

	// get file name and extension
	_tsplitpath (File, NULL, NULL, fName, fExt);
	wsprintf(Buffer,TEXT("%s%s"),fName,fExt);

	wsprintf(Command, TEXT(":load %s"), ExpandFileName(Buffer));
	if(async)
		FireAsyncCommand(Command); //may be called from DragAndDrop
	else
		FireCommand(Command); 

}

VOID LoadFile(LPTSTR File)
{
	LoadFileExt(File,FALSE);
}
#endif

VOID MainDropFiles(HWND hWnd, HDROP hDrop)
{
	TCHAR File[MAX_PATH];

	DragQueryFile(hDrop, 0, File, MAX_PATH);
	DragFinish(hDrop);

	LoadFileExt(File,TRUE);

}



VOID MainMenuSelect(HWND hWnd, INT ID, INT Flags)
{
	TCHAR Buffer[1024];

	if (Flags & MF_POPUP || Flags == 0xFFFF)
		ID = 0;

	if ((ID >= ID_TOOLS_TOOL1) && (ID <= ID_TOOLS_TOOL10))
		StringCpy(Buffer,ToolGetHelp(ID-ID_TOOLS_TOOL1));

	else if (ID == 0 || !LoadString(hThisInstance, ID, Buffer, sizeof(Buffer)))
		Buffer[0] = 0;

	SetStatusBar(Buffer);
}

INT MainNotify(HWND hWnd, LPNMHDR nmhdr)
{
	if (nmhdr->code == TBN_GETINFOTIP && nmhdr->idFrom == IDC_Toolbar) {
		LPNMTBGETINFOTIP tt = (LPNMTBGETINFOTIP) nmhdr;
		LoadString(hThisInstance, tt->iItem, tt->pszText, tt->cchTextMax);
	}
	else if (nmhdr->idFrom == IDC_Rtf)
		return RtfNotify(hWnd, nmhdr);

	return FALSE;
}

VOID MainSize(HWND hWnd, INT x, INT y)
{
	RECT rc;

	INT HeightStatus, HeightToolbar;

	GetClientRect(hWndStatus, &rc);
	HeightStatus = rc.bottom;

	GetClientRect(hWndToolbar, &rc);
	HeightToolbar = rc.bottom;

	MoveWindow(hWndRtf, 0, HeightToolbar, x, y-(HeightToolbar+HeightStatus), TRUE);

	MoveWindow(hWndStatus, 0, y - HeightStatus, x, HeightStatus, TRUE);
}



VOID ShowContextMenu(INT x, INT y)
{
	HMENU hEdit = GetSubMenu(GetMenu(hWndMain), 1);

	if (x == 0xffff && y == 0xffff)
	{
		RECT rc;
		GetWindowRect(GetDlgItem(hWndMain, IDC_Rtf), &rc);
		x = rc.left+2;
		y = rc.top+2;
	}

	TrackPopupMenu(hEdit, 0, x, y, 0, hWndMain, NULL);
	CreatePopupMenu();
}


VOID MainCreate(HWND hWnd)
{


}

LRESULT CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {

		case WM_NOTIFY:		
			return MainNotify(hWnd, (LPNMHDR) lParam);
			break;

		case WM_CREATE:
			MainCreate(hWnd);
			break;

		case WM_DROPFILES:
			MainDropFiles(hWnd, (HDROP) wParam);
			break;

		case WM_COMMAND: 
			MainCommand(hWnd, LOWORD(wParam));		
			break;

		case WM_SIZE:
			MainSize(hWnd, LOWORD(lParam), HIWORD(lParam));
			break;

		case WM_MENUSELECT:
			MainMenuSelect(hWnd, LOWORD(wParam), HIWORD(wParam));
			break;

		case WM_TIMER:
			RtfWindowTimer();
			break;

		case WM_SETFOCUS:
			SetFocus(hWndRtf);
			break;

		case WM_CONTEXTMENU: {
				HWND hParam = (HWND) wParam;
				HWND hRtfChild = GetDlgItem(hWnd, IDC_Rtf);
				if (hParam == hWnd || hParam == hRtfChild)
					ShowContextMenu(LOWORD(lParam), HIWORD(lParam));
			}
			break;

		case WM_HELP:		
			MainCommand(hWnd, ID_HELPCONTENTS);
			break;

		case WM_CLOSE:
			FinalizeWinGHCi();

			if (Running)
				AbortExecution();

			FireAsyncCommand(TEXT(":quit"));

			// should not be necessary
			SetEvent(hKillGHCi);

			PostQuitMessage(0);
			break;
		default: return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}




#if 0
typedef struct {
	HWND hWnd;
	UINT uMsg;
	WPARAM wParam;
	LPARAM lParam;
	MSGFILTER msg;
} MainWndProcParams;


DWORD WINAPI MainWndProcThread(LPVOID lpParam)
{
	HWND hWnd;
	UINT uMsg;
	WPARAM wParam;
	LPARAM lParam;
	MSGFILTER msg;
	MainWndProcParams *ptr = (MainWndProcParams *) lpParam;

	hWnd = ptr->hWnd;
	uMsg = ptr->uMsg;
	wParam = ptr->wParam;
	lParam = ptr->lParam; 
	msg = ptr->msg;

	free(ptr);

	MainWndProcAux(hWnd, uMsg, wParam, lParam, msg);

	return 0;


}

INT_PTR CALLBACK MainWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	MainWndProcParams *ptr = malloc(sizeof(MainWndProcParams));
	DWORD ThreadId;

	ptr->hWnd = hWnd;
	ptr->uMsg = uMsg;
	ptr->wParam = wParam;
	ptr->lParam = lParam;
	if(uMsg== WM_NOTIFY)
		ptr->msg = * ((MSGFILTER*) lParam);

	if(!CreateThread(NULL,0,MainWndProcThread,ptr,0,&ThreadId))
			ErrorExit(TEXT("CreateThread MainWndProcThread failed\n"));
	return FALSE;
}
#endif

TCHAR WinGHCiWindowClass[] = TEXT("WinGHCiWindowClass");			
TCHAR WinGHCiWindowTitle[] = TEXT("WinGHCi");

ATOM RegisterWinGHCiWindowClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style			= CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc	= MainWndProc;
	wcex.cbClsExtra		= 0;
	wcex.cbWndExtra		= 0;
	wcex.hInstance		= hInstance;
	wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(ID_WinGHCi_ICON));
	wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground	= (HBRUSH)(COLOR_WINDOW+0);
	wcex.lpszMenuName	= MAKEINTRESOURCE(IDR_WinGHCi_MENU);
	wcex.lpszClassName	= WinGHCiWindowClass;
	wcex.hIconSm		= LoadIcon(wcex.hInstance, MAKEINTRESOURCE(ID_GHCi_ICON));

	return RegisterClassEx(&wcex);
}



BOOL CreateWinGHCiMainWindow(INT nCmdShow)
{
	//hWndMain = CreateDialog(hThisInstance, MAKEINTRESOURCE(DLG_MAIN), NULL, &MainWndProc);

	
	if (!RegisterWinGHCiWindowClass(hThisInstance) )
	{	
		ErrorExit(TEXT("Could not register WinGHCiWindowClass"));
		return FALSE;
	}

	hWndMain = CreateWindowEx(WS_EX_ACCEPTFILES,WinGHCiWindowClass, WinGHCiWindowTitle, WS_OVERLAPPEDWINDOW,
      CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, NULL, NULL, hThisInstance, NULL);



	if (!hWndMain)
	{	
		ErrorExit(TEXT("CreateWinGHCiMainWindow"));
		return FALSE;
	}

    ShowWindow(hWndMain, nCmdShow);
    UpdateWindow(hWndMain);

	// Setup the icons
	SendMessage(hWndMain, WM_SETICON, ICON_SMALL, (LPARAM) LoadIcon(hThisInstance, MAKEINTRESOURCE(ID_GHCi_ICON)));	
	SendMessage(hWndMain, WM_SETICON, ICON_BIG,	(LPARAM) LoadIcon(hThisInstance, MAKEINTRESOURCE(ID_WinGHCi_ICON)));

	// Create status line
	hWndStatus = CreateWindowEx(
		0,                       
		STATUSCLASSNAME,        
		(LPCTSTR) TEXT(""),          
		WS_CHILD | SBARS_SIZEGRIP | WS_VISIBLE,               
		0, 0, 0, 0,              
		hWndMain,              
		(HMENU) IDC_Statusbar,      
		hThisInstance,                   
		NULL);                   


	// create rich edit control
	hWndRtf = CreateWindowEx(
		 WS_EX_CLIENTEDGE,
		 RICHEDIT_CLASS,        
		(LPCTSTR) TEXT(""),          
		WS_CHILD  | WS_VISIBLE | WS_VSCROLL | ES_AUTOVSCROLL | ES_MULTILINE | ES_DISABLENOSCROLL | ES_WANTRETURN,               
		0, 0, 0, 0,              
		hWndMain,              
		(HMENU) IDC_Rtf,      
		hThisInstance,                   
		NULL);   

	RtfWindowInit();
	SetFocus(hWndRtf);

	CreateToolbar();
	InitMruFiles();
	InitHistory();

	RegistryReadWindowPos(hWndMain);


	return TRUE;
} 

