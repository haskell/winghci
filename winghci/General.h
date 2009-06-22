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

TCHAR *ExpandFileName(LPTSTR str);