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
VOID RtfWindowStartOutput();
VOID RtfWindowStartInput();

INT winGhciColor(INT Color);
BOOL winGhciBold(BOOL Bold);

VOID RtfWindowUpdateFont(VOID);
VOID RtfWindowSetCommand(LPCTSTR Command);
VOID RtfWindowGetCommand(LPTSTR Command, INT MaxLen);

VOID RtfWindowTimer(VOID);



VOID RtfWindowFlushBuffer(VOID);