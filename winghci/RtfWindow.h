#include "CommonIncludes.h"

VOID RtfWindowInit(VOID);

VOID RtfWindowTextColor(INT Color);


VOID RtfWindowPutS(LPCTSTR s);
VOID RtfWindowPutSExt(LPCTSTR s, INT numChars);
BOOL RtfNotify(HWND hDlg, NMHDR* nmhdr);
INT RtfWindowCanCutCopy();
VOID RtfWindowClipboard(UINT Msg);

VOID RtfWindowClear();
VOID RtfWindowClearLastLine();
VOID RtfWindowDelete();
VOID RtfWindowRelativeHistory(INT Delta);
VOID RtfWindowSelectAll();
VOID RtfWindowStartNextOutput();
VOID RtfWindowStartNextInput();
VOID RtfWindowGotoEnd();

INT WinGHCiColor(INT Color);
BOOL WinGHCiBold(BOOL Bold);

VOID RtfWindowUpdateFont(VOID);
VOID RtfWindowSetCommand(LPCTSTR Command);
VOID RtfWindowGetCommand(LPTSTR Command, INT MaxLen);




VOID RtfWindowForceFlushBuffer(VOID);