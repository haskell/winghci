#include <windows.h>


extern TCHAR LastFileLoaded[];

BOOL CreateWinGHCiMainWindow(INT);


VOID FireAsyncCommand(LPCTSTR Command);
VOID FireCommand(LPCTSTR Command);
VOID FireCommandExt(LPCTSTR Command, BOOL WaitForResponse, BOOL startThread, BOOL StopStdoutPrinterThread, BOOL WantPreprocess);

VOID EnableButtons(VOID);
VOID AbortExecution(VOID);

VOID LoadFile(LPTSTR FileName);
VOID LoadFileExt(LPTSTR File, BOOL async);

VOID MainSize(HWND hWnd, INT x, INT y);
