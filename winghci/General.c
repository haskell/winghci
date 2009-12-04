/******************************************************************************
	WinGHCi, a GUI for GHCi

	General.c: Assorted utility functions

	Original code taken from Winhugs (http://haskell.org/hugs)

	With modifications by Pepe Gallardo, 2009-March
*******************************************************************************/

#include "CommonIncludes.h"
#include "General.h"
#include "Registry.h"
#include "Strings.h"
#include "WndMain.h"
#include "WinGHCi.h"



LPTSTR GetGHCinstallDir(VOID)
{
	static TCHAR GHCinstallDir[MAX_PATH] = TEXT("");
	#define GHC_REG TEXT("SOFTWARE\\Haskell\\GHC")

	if(StringIsEmpty(GHCinstallDir)){
		LPTSTR res = readRegString(HKEY_CURRENT_USER, GHC_REG, TEXT("InstallDir"), TEXT(""));
		StringCpy(GHCinstallDir, res);
		free(res);
	}
	return GHCinstallDir;
}

LPTSTR GetWinGHCiInstallDir(VOID)
{
	static TCHAR WinGHCiInstallDir[MAX_PATH] = TEXT("");
	TCHAR currentExe[MAX_PATH], Drive[_MAX_DRIVE], Dir[MAX_PATH];

	if(StringIsEmpty(WinGHCiInstallDir)){
		GetModuleFileName(NULL, currentExe, MAX_PATH);

		_tsplitpath (currentExe, Drive, Dir, NULL, NULL);
		wsprintf(WinGHCiInstallDir,TEXT("%s%s"),Drive,Dir);
	}
	return WinGHCiInstallDir;

	//MessageBox(NULL,path,path,MB_OK);

}

/* builds a short name for a file path of maximum length MAX_SHORTNAME */
#define MAX_SHORTNAME	MAX_PATH

VOID ShortFileName(LPTSTR SrcFileName, LPTSTR DestFileName)
{
    TCHAR dir[_MAX_PATH], shortDir[_MAX_PATH], shortAux[_MAX_PATH];
    TCHAR ext[_MAX_EXT];
    TCHAR drive[_MAX_DRIVE];
    TCHAR fName[_MAX_FNAME];
    TCHAR *ptr;
    BOOL Stop = FALSE;

    /* try to get path */
    _tsplitpath (SrcFileName, drive, dir, fName, ext);

    /* delete last '\\' char */
    ptr = StringRChr(dir,TEXT('\\'));
    if (ptr)
	*ptr = (TCHAR)0;

    wsprintf(shortDir, TEXT("\\%s%s"), fName, ext);

    while (*dir && !Stop) {
	ptr = StringRChr(dir,TEXT('\\'));

	if(StringLen(shortDir)+StringLen(ptr) < MAX_SHORTNAME) {
	    /* shortDir = ptr ++ shortDir */
	    wsprintf(shortAux, TEXT("%s%s"), ptr, shortDir);
	    StringCpy(shortDir, shortAux);

	    /* delete appended string from dir */
	    *ptr = (TCHAR)0;
	} else
	    Stop = TRUE;
    }

    if (*dir)
	wsprintf(DestFileName,TEXT("%s\\...%s"),drive,shortDir);
    else
	wsprintf(DestFileName,TEXT("%s%s"),drive,shortDir);

}

/* Call this function in WM_INITDIALOG to center the dialog in Parent window */
// Taken from the MSDN, Using Dialog Boxes
VOID CenterDialogInParent(HWND hDlg)
{
    RECT rDlg, rMain;
    INT posX, posY;

    GetWindowRect(hDlg, &rDlg);
    GetWindowRect(GetParent(hDlg), &rMain);

    posX = rMain.left+((rMain.right-rMain.left-(rDlg.right - rDlg.left))/2);
    posY = rMain.top+((rMain.bottom-rMain.top-(rDlg.bottom - rDlg.top))/2);

    if (posX < 0) posX = 0;
    if (posY < 0) posY = 0;

    SetWindowPos(hDlg, NULL, posX, posY, 0, 0, SWP_NOSIZE | SWP_NOACTIVATE);
}



/* change working directory to location of file  */
VOID SetWorkingDirToFileLoc(LPCTSTR FileName, BOOL doCD)
{
    TCHAR path[MAX_PATH];
    TCHAR drive[_MAX_DRIVE];
    TCHAR thePath[2*MAX_PATH];
	TCHAR Buffer[2*MAX_PATH];

    /* ignore file name and extension */
    _tsplitpath (FileName, drive, path, NULL, NULL);
	// remove trailing /
	if(StringLen(path)>0) {
		path[StringLen(path)-1] = TEXT('\0');
		wsprintf(thePath,TEXT("%s%s"),drive,path);

	
		/* Set path */
		SetCurrentDirectory(thePath);
		if(doCD) {
			wsprintf(Buffer,TEXT(":cd %s"),thePath);
			FireCommand(Buffer);
		}
	}
	
}

// Works for HTML Documents, http links etc
// if it starts with file:, then fire off the filename

LPTSTR hugsdir=TEXT("c:\\");

VOID ExecuteFile(LPTSTR FileName)
{
    if (StringCmpN(FileName, TEXT("file:"), 5) == 0) {
	//find the line number
	TCHAR Buffer[MAX_PATH];
	LPTSTR s = StringRChr(FileName, TEXT(':'));
	INT LineNo;
	if (s != NULL) {
	    INT i;
	    s++;
	    for (i = 0; s[i] != 0; i++) {
		if (!isdigit(s[i])) {
		    s = NULL;
		    break;
		}
	    }
	}

	if (s == NULL)
	    LineNo = 0; // the null line
	else {
	    s[-1] = 0;
	    LineNo = StringToInt(s);
	}

	FileName += 5;			/* skip over "file:" */
	if (StringCmpN(TEXT("{Hugs}"), FileName, 6) == 0) {
	    StringCpy(Buffer, hugsdir);
	    StringCat(Buffer, &FileName[6]);
	} else if (FileName[0] == TEXT('.') && FileName[1] == TEXT('\\')) {
	    GetCurrentDirectory(MAX_PATH, Buffer);
	    StringCat(Buffer, &FileName[1]);
	} else
	    StringCpy(Buffer, FileName);

	//pepe startEdit(LineNo, Buffer);
    } else {
	INT Res = (INT) ShellExecute(hWndMain, NULL, FileName, NULL, NULL, SW_SHOWNORMAL);
	if (Res <= 32) {
	    TCHAR Buffer[MAX_PATH*2];
	    StringCpy(Buffer, TEXT("Failed to launch file:\n"));
	    StringCat(Buffer, FileName);
	    MessageBox(hWndMain, Buffer, TEXT("Hugs98"), MB_ICONWARNING);
	}
    }
}

// same as ExecuteFile, but relative to the Doc's directory
VOID ExecuteFileDocs(LPTSTR FileName)
{

	TCHAR Buffer[MAX_PATH];

	wsprintf(Buffer,TEXT("%s\\doc\\%s"),GetGHCinstallDir(),FileName);
	ExecuteFile(Buffer); 
	
}

#if 0
/* expands characters like \ to \\ in a file name */
LPTSTR ExpandFileName(LPTSTR what)
{
    static TCHAR Expanded[2048];


	return what;

    if (*what == '\"') {
	StringCpy(Expanded, what);
    } else {
	LPTSTR where, t, unlex;

	StringCpy(Expanded,"\"");

	for(where = &Expanded[1],t=what; *t; ++t) {
	    unlex = unlexChar(*t,'"');
	    wsprintf(where, "%s", unlexChar(*t,'"'));
	    where += StringLen(unlex);
	}
	wsprintf(where, "\"%c", TEXT('\0'));
    }
    return Expanded;

}
#endif

BOOL ShowOpenFileDialog(HWND hParent, LPTSTR FileName)
{
    OPENFILENAME ofn;
	TCHAR CurrentWorkingDir[MAX_PATH];

    FileName[0] = 0;
    memset(&ofn, 0, sizeof(ofn));
    ofn.lStructSize = sizeof(OPENFILENAME);
    ofn.hInstance = hThisInstance;
    ofn.hwndOwner = hParent; 
    ofn.lpstrFilter = TEXT("Haskell Files (*.hs;*.lhs)\0*.hs;*.lhs\0All Files (*.*)\0*.*\0");
    ofn.nFilterIndex = 1;
    ofn.lpstrFile= FileName;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrFileTitle = NULL;
    ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_EXPLORER;
    ofn.lpfnHook = NULL;

	GetCurrentDirectory(MAX_PATH,CurrentWorkingDir);
    ofn.lpstrInitialDir = CurrentWorkingDir;

	return GetOpenFileName(&ofn);;
}

INT TwipToPoint(INT x){return x / 20;}
INT PointToTwip(INT x){return x * 20;}

VOID ErrorExit(LPTSTR lpszFunction) 
{ 
	// Retrieve the system error message for the last-error code

	LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	DWORD dw = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	// Display the error message and exit the process

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
		(lstrlen((LPCTSTR)lpMsgBuf)+lstrlen((LPCTSTR)lpszFunction)+40)*sizeof(TCHAR)); 
	wsprintf((LPTSTR)lpDisplayBuf,
		TEXT("%s failed with error %d: %s"), 
		lpszFunction, dw, lpMsgBuf); 
	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);
	ExitProcess(dw); 
}




//todo treat more special chars apart form \

TCHAR ExpandFileNameBuff[MAX_PATH];

TCHAR *ExpandFileName(LPTSTR str)
{
	TCHAR BuffAux[MAX_PATH];

	LPTSTR ptr=ExpandFileNameBuff, pos;
	wsprintf(ExpandFileNameBuff,TEXT("\"%s\""),str);	
	for(;;) {
		pos = StringChr(ptr,TEXT('\\'));
		if (!pos)
			break;

		StringCpy(BuffAux,pos);
		pos++;
		*pos=TEXT('\0');
		StringCat(ExpandFileNameBuff,BuffAux);
		pos++;
		ptr=pos;
	}

	return ExpandFileNameBuff;
}


VOID AsShortFileName(LPTSTR FileName, LPTSTR ShortFileName, DWORD ShortFileNameSz)
{
	if(!GetShortPathName(FileName,ShortFileName,ShortFileNameSz))
		StringCpy(ShortFileName,FileName);

}