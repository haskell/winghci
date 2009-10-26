#include "CommonIncludes.h"

INT TwipToPoint(INT x);
INT PointToTwip(INT x);

VOID ExecuteFile(LPTSTR FileName);
VOID ExecuteFileDocs(LPTSTR FileName);
VOID CenterDialogInParent(HWND hDlg);

BOOL ShowOpenFileDialog(HWND hParent, LPTSTR FileName);
VOID SetWorkingDirToFileLoc(LPCTSTR FileName, BOOL doCD);
VOID ErrorExit(LPTSTR lpszFunction);
LPTSTR GetGhcInstallDir(VOID);
LPTSTR GetWinGhciInstallDir(VOID);
VOID AsShortFileName(LPTSTR FileName, LPTSTR ShortFileName, DWORD ShortFileNameSz);


TCHAR *ExpandFileName(LPTSTR str);