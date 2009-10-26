/******************************************************************************
	WinGhci, a GUI for GHCI

	RftWindow.c: rich text output control
	
	Original code taken from Winhugs (http://haskell.org/hugs)

	With modifications by Pepe Gallardo, 2009-March
*******************************************************************************/

#include "CommonIncludes.h"
#include "Colors.h"
#include "General.h"
#include "History.h"
#include "Registry.h"
#include "RtfWindow.h"
#include "Strings.h"
#include "Utf8.h"
#include "WndMain.h"
#include "WinGhci.h"



VOID RtfWindowSetCommand(LPCTSTR Command);
VOID RtfWindowGetCommand(LPTSTR Command, INT MaxLen);


BOOL Running = FALSE;

// have a max of 100Kb in the scroll window
#define MAXIMUM_BUFFER   100000

// Buffer the RTF Window Handle
// Only allow one RTF Window at a time

BOOL PuttingChar = FALSE;
DWORD StartOfInput = 0; // points to the start of the current input
DWORD OutputStart;

typedef struct _Format {
    INT ForeColor;
    INT BackColor;
    BOOL Bold;
    BOOL Italic;
    BOOL Underline;
} Format;

BOOL FormatChanged = FALSE;
Format DefFormat = {BLACK, WHITE, FALSE, FALSE, FALSE};
Format BufFormat = {BLACK, WHITE, FALSE, FALSE, FALSE};
Format NowFormat = {BLACK, WHITE, FALSE, FALSE, FALSE};

CRITICAL_SECTION CriticalSect;

// let the rich edit control break words anywhere
INT CALLBACK EditWordBreakProc(LPTSTR lpch,
    INT ichCurrent,
    INT cch,
    INT code
	)
{
	if(code==WB_ISDELIMITER)
		return TRUE;
	else if(code==WB_CLASSIFY)
		return WBF_BREAKAFTER;
	else 
		return ichCurrent;
}

VOID RtfWindowInit(VOID)
{
    CHARFORMAT cf;

    //make it all protected
    SendMessage(hWndRtf, EM_SETEVENTMASK, 0,
	ENM_PROTECTED | ENM_LINK | ENM_KEYEVENTS | ENM_SELCHANGE);
    cf.cbSize = sizeof(cf);
    cf.dwEffects = CFE_PROTECTED;
    cf.dwMask = CFM_PROTECTED;
    SendMessage(hWndRtf, EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &cf);

	SendMessage(hWndRtf, EM_SETTEXTMODE, TM_RICHTEXT|TM_MULTICODEPAGE, (LPARAM) &cf);


    // Allow them 1 million characters
    // the system will sort out overflows later
    SendMessage(hWndRtf, EM_LIMITTEXT, 1000000, 0);

	// Let words break anywhere
	SendMessage(hWndRtf,EM_SETWORDBREAKPROC,0,(LPARAM)EditWordBreakProc);

    // Default formatting information
    BufFormat = DefFormat;
    NowFormat = DefFormat;

    // And syncronisation stuff
    InitializeCriticalSection(&CriticalSect);

    //update the font
    RtfWindowUpdateFont();
}

VOID RtfWindowUpdateFont(VOID)
{
    CHARFORMAT cf;

    RegistryReadFont(&cf);
    SendMessage(hWndRtf, EM_SETCHARFORMAT, SCF_ALL, (LPARAM) &cf);
}

// get number of chars in rtf control
INT RtfWindowTextLength()
{
    GETTEXTLENGTHEX gtl;
    gtl.codepage = CP_UNICODE;
    gtl.flags = GTL_DEFAULT;
    return SendMessage(hWndRtf, EM_GETTEXTLENGTHEX, (WPARAM) &gtl, 0);
}

// return a bit mask of DROPEFFECT_NONE, DROPEFFECT_COPY, DROPEFFECT_MOVE
INT RtfWindowCanCutCopy()
{
    DWORD Start, End;
    SendMessage(hWndRtf, EM_GETSEL, (WPARAM) &Start, (WPARAM) &End);
    if (Start == End)
		return DROPEFFECT_NONE;
    else if (Start >= StartOfInput)
		return DROPEFFECT_COPY | DROPEFFECT_MOVE;
    else
		return DROPEFFECT_COPY;
}

VOID RtfWindowClearLastLine()
{
    CHARRANGE cr;
    INT Lines = SendMessage(hWndRtf, EM_GETLINECOUNT, 0, 0);
    INT LastLine = SendMessage(hWndRtf, EM_LINEINDEX, Lines, 0);
	INT PrevLine = SendMessage(hWndRtf, EM_LINEINDEX, Lines-1, 0);

    SendMessage(hWndRtf, EM_EXGETSEL, 0, (LPARAM) &cr);
    SendMessage(hWndRtf, EM_SETSEL, PrevLine, LastLine);
    PuttingChar = TRUE;
    SendMessage(hWndRtf, EM_REPLACESEL, FALSE, (LPARAM) TEXT(""));
    PuttingChar = FALSE;

    cr.cpMax -= LastLine;
    cr.cpMin -= LastLine;

    StartOfInput -= LastLine;
    if (cr.cpMin < 0)
        SendMessage(hWndRtf, EM_SETSEL, StartOfInput, StartOfInput);
    else
		SendMessage(hWndRtf, EM_EXSETSEL, 0, (LPARAM) &cr);
}

VOID RtfWindowClear()
{
    CHARRANGE cr;
    INT Lines = SendMessage(hWndRtf, EM_GETLINECOUNT, 0, 0);
    INT ThisLine = SendMessage(hWndRtf, EM_LINEINDEX, Lines-1, 0);

    SendMessage(hWndRtf, EM_EXGETSEL, 0, (LPARAM) &cr);
    SendMessage(hWndRtf, EM_SETSEL, 0, ThisLine);
    PuttingChar = TRUE;
    SendMessage(hWndRtf, EM_REPLACESEL, FALSE, (LPARAM) TEXT(""));
    PuttingChar = FALSE;

    cr.cpMax -= ThisLine;
    cr.cpMin -= ThisLine;

    StartOfInput -= ThisLine;
    if (cr.cpMin < 0)
        SendMessage(hWndRtf, EM_SETSEL, StartOfInput, StartOfInput);
    else
		SendMessage(hWndRtf, EM_EXSETSEL, 0, (LPARAM) &cr);
}

// deletes selected text
VOID RtfWindowDelete()
{
    SendMessage(hWndRtf, EM_REPLACESEL, FALSE, (LPARAM) "");
}

VOID RtfWindowRelativeHistory(INT Delta)
{
    LPCTSTR x = GoToRelativeHistory(Delta);
    if (x == NULL)
		MessageBeep((UINT)-1);
    else
		RtfWindowSetCommand(x);
}

VOID RtfWindowSelectAll()
{
    SendMessage(hWndRtf, EM_SETSEL, 0, -1);
}

BOOL RtfNotify(HWND hDlg, NMHDR* nmhdr)
{
	static BOOL FindFirst = TRUE;
	#define BUFF_LEN 2048
	static TCHAR FindWhat[BUFF_LEN];

	if (nmhdr->code == EN_PROTECTED && !PuttingChar) {
		//block
		ENPROTECTED* enp = (ENPROTECTED*) nmhdr;
		CHARRANGE cr;
		INT TextLen = RtfWindowTextLength();
		BOOL Reset = FALSE, Disallow = FALSE;


		// just let it go ahead anyway
		if (enp->msg == WM_COPY)
			return FALSE;

		// they hit backspace
		if (enp->wParam == VK_BACK) {
			if ((DWORD) enp->chrg.cpMin < StartOfInput ||
				((DWORD) enp->chrg.cpMin == StartOfInput &&
				enp->chrg.cpMin == enp->chrg.cpMax)) {
					Reset = TRUE;
					Disallow = TRUE;
			}
		} else if ((DWORD) enp->chrg.cpMin < StartOfInput) {
			Reset = TRUE;
			Disallow = (enp->wParam == VK_DELETE);
		}

		if (Reset) {
			cr.cpMin = TextLen;
			cr.cpMax = cr.cpMin;
			SendMessage(hWndRtf, EM_EXSETSEL, 0, (LPARAM) &cr);
		}

		// we don't want to paste rich text, as that makes it look weird
		// so send only plain text paste commands
		if ((enp->msg == WM_PASTE) && !Disallow) {
			LPTSTR Buffer = NULL;
			Disallow = TRUE;
#if defined _UNICODE
#define CLIP_FORMAT  CF_UNICODETEXT
#else
#define CLIP_FORMAT  CF_TEXT
#endif
			if (IsClipboardFormatAvailable(CLIP_FORMAT) &&
				OpenClipboard(hWndMain)) {
					HGLOBAL hGlb; 
					LPTSTR str; 

					if ((hGlb = GetClipboardData(CLIP_FORMAT)) != NULL &&
						(str = GlobalLock(hGlb)) != NULL) {
							Buffer = StringDup(str);
							GlobalUnlock(hGlb);
					}
					CloseClipboard();
			}

			if (Buffer != NULL) {
				// strip trailing new line characters
				INT i;
				for (i = StringLen(Buffer)-1;
					i >= 0 && (Buffer[i] == TEXT('\r') || Buffer[i] == TEXT('\n'));
					i--)
					Buffer[i] = 0;

#if defined _UNICODE
				{
					SETTEXTEX stt;

					stt.codepage = CP_UNICODE;
					stt.flags = ST_SELECTION;
					SendMessage(hWndRtf,EM_SETTEXTEX,(WPARAM)&stt,(LPARAM)Buffer);
				}
#else
				SendMessage(hWndRtf, EM_REPLACESEL, FALSE, (LPARAM)Buffer);
#endif


				free(Buffer);
			}
		}

		SetWindowLong(hDlg, DWL_MSGRESULT, (Disallow ? 1 : 0));
		return TRUE;
	} else if (nmhdr->code == EN_LINK) {
		// should really fire on up
		// but that screws up the cursor position

		ENLINK* enl = (ENLINK*) nmhdr;
		if (enl->msg == WM_LBUTTONDOWN) {
			TEXTRANGE tr;
			TCHAR Buffer[1000];
			tr.lpstrText = Buffer;
			tr.chrg.cpMin = enl->chrg.cpMin;
			tr.chrg.cpMax = enl->chrg.cpMax;

			SendMessage(hWndRtf, EM_GETTEXTRANGE, 0, (LPARAM) &tr);
			ExecuteFile(Buffer);

			SetWindowLong(hDlg, DWL_MSGRESULT, 1);
			return TRUE;
		}
	} else if (nmhdr->code == EN_MSGFILTER) {
		MSGFILTER* mf = (MSGFILTER*) nmhdr;
#if 0
		//if (mf->msg == WM_CHAR && Running) {
		if (mf->msg == WM_CHAR && 0) {
			WinGhciReceiveC((TCHAR)mf->wParam == TEXT('\r') ? TEXT('\n') : mf->wParam);
			SetWindowLong(hDlg, DWL_MSGRESULT, 1);
			return TRUE;
		} //else if (Running && mf->msg == WM_KEYDOWN) {
		else if (0) {
			SHORT n = GetKeyState(VK_CONTROL);
			BOOL Control = (n & (1 << 16));
			if(((CHAR)(mf->wParam) ==(CHAR)TEXT('C')) && Control)
				AbortExecution();
			else
				SetWindowLong(hDlg, DWL_MSGRESULT, 1);
			return TRUE;
		} //else if (mf->msg == WM_KEYDOWN && !Running) {
		else 
#endif

		
		if (mf->msg == WM_CHAR) {

			if (mf->wParam == VK_TAB) {

				INT pos;

				if(FindFirst) {
					RtfWindowGetCommand(FindWhat,BUFF_LEN);
					pos = FindFirstHistory(FindWhat);
					FindFirst = FALSE;
				} else
					pos = FindNextHistory();

				if(pos>=0) 
					RtfWindowSetCommand(GoToHistory(pos));					
				else
					MessageBeep((UINT)-1);



				SetWindowLong(hDlg, DWL_MSGRESULT, 1);
				return TRUE;

			} else {
				// any other key resets search
				FindFirst = TRUE;

				if (mf->wParam == VK_ESCAPE) {

					//Clear current command
					RtfWindowSetCommand(TEXT(""));
					// Go to last item in history
					AddHistory(TEXT(""));
					SetWindowLong(hDlg, DWL_MSGRESULT, 1);
					return TRUE;
				} else {

					SetWindowLong(hDlg, DWL_MSGRESULT, 0);
					return TRUE;
				}
			}


		} else if (mf->msg == WM_KEYDOWN) {
			BOOL History = (mf->wParam == VK_UP || mf->wParam == VK_DOWN);
			SHORT n = GetKeyState(VK_CONTROL);
			BOOL Control = (n & (1 << 16));

			if(((CHAR)(mf->wParam) ==(CHAR)TEXT('C')) && Control) {
				if(RtfWindowCanCutCopy() && DROPEFFECT_COPY)
					RtfWindowClipboard(WM_COPY);
				else
					AbortExecution();
			} else if (History && (mf->lParam & (1 << 24))) {
				CHARRANGE cr;
				SendMessage(hWndRtf, EM_EXGETSEL, 0, (LPARAM) &cr);
				if ((DWORD) cr.cpMin >= StartOfInput) {
					RtfWindowRelativeHistory(mf->wParam == VK_UP ? -1 : +1);
					SetWindowLong(hDlg, DWL_MSGRESULT, 1);
					return TRUE;
				}
			} else if (mf->wParam == VK_RETURN) {
				#define BUFF_LEN 2048
				TCHAR Buffer[BUFF_LEN];


				RtfWindowGetCommand(Buffer,BUFF_LEN);

				if(Running) {
					if(!(mf->lParam & (1<<30))) //avoid repetition
						FireCommandExt(Buffer,FALSE,TRUE,FALSE,FALSE);
				}
				else
					//if(!(mf->lParam & (1<<30)))
						FireAsyncCommand(Buffer);


				SetWindowLong(hDlg, DWL_MSGRESULT, 1);
				return TRUE;
			} else if (mf->wParam == VK_HOME) {
				CHARRANGE cr;
				SendMessage(hWndRtf, EM_EXGETSEL, 0, (LPARAM) &cr);
				if ((DWORD) cr.cpMin > StartOfInput) {
					SHORT n = GetKeyState(VK_SHIFT);
					BOOL Shift = (n & (1 << 16));

					SetWindowLong(hDlg, DWL_MSGRESULT, 1);
					cr.cpMin = StartOfInput;
					cr.cpMax = (Shift ? cr.cpMax : StartOfInput);
					SendMessage(hWndRtf, EM_EXSETSEL, 0, (LPARAM) &cr);
					SetWindowLong(hDlg, DWL_MSGRESULT, 1);
					return TRUE;
				}
			}
		}
	} else if (nmhdr->code == EN_SELCHANGE) {
		EnableButtons();
		return TRUE;
	}

	return FALSE;
}

// Respond to a clipboard message
// WM_PASTE, WM_COPY, WM_CUT
VOID RtfWindowClipboard(UINT Msg)
{
    SendMessage(hWndRtf, Msg, 0, 0);
}



// Replaces the current input with Command
// NULL means freeze in the existing command
VOID RtfWindowSetCommand(LPCTSTR Command)
{
#if defined _UNICODE
	CHARRANGE cr;
	SETTEXTEX stt;

	cr.cpMin = StartOfInput;
	cr.cpMax = RtfWindowTextLength();
	if(cr.cpMin==cr.cpMax) 
		cr.cpMax++;
	SendMessage(hWndRtf, EM_EXSETSEL, 0, (LPARAM) &cr);

	stt.codepage = CP_UNICODE;
	stt.flags = ST_SELECTION;
	PuttingChar = TRUE;
	SendMessage(hWndRtf,EM_SETTEXTEX,(WPARAM)&stt,(LPARAM)Command);
#else
    SendMessage(hWndRtf, EM_SETSEL, StartOfInput, RtfWindowTextLength());
    PuttingChar = TRUE;
    SendMessage(hWndRtf, EM_REPLACESEL, FALSE, (LPARAM) Command); 
#endif
   PuttingChar = FALSE;
}

VOID RtfWindowGetCommand(LPTSTR Command, INT MaxLen)
{
    TEXTRANGE tr;
	GETTEXTEX gtt;

    tr.lpstrText = Command;
    tr.chrg.cpMin = StartOfInput;
    tr.chrg.cpMax = RtfWindowTextLength();

    if (tr.chrg.cpMin == tr.chrg.cpMax)
		// no input
		Command[0] = TEXT('\0');
    else
#if defined _UNICODE
	{
		CHARRANGE cr;

		cr.cpMin = StartOfInput;
		cr.cpMax = RtfWindowTextLength();
		SendMessage(hWndRtf,EM_EXSETSEL,0,(LPARAM)&cr);

		gtt.cb = MaxLen;
		gtt.flags = GT_SELECTION;
	    gtt.codepage = CP_UNICODE;
		gtt.lpDefaultChar = NULL;
		gtt.lpUsedDefChar = NULL;
		SendMessage(hWndRtf, EM_GETTEXTEX, (WPARAM)&gtt, (LPARAM) Command);
		
	}
#else
		SendMessage(hWndRtf, EM_GETTEXTRANGE, 0, (LPARAM) &tr);
#endif
}

/////////////////////////////////////////////////////////////////////
// BUFFERING AND OUTPUT
/////////////////////////////////////////////////////////////////////

const INT BufSize = 995;
TCHAR Buf[1000];
INT BufPos = 0; // where to write out in the buffer
INT BufLen = 0; // how much of the buffer is useful
INT OutputPos = 0; // how much to delete of the existing thing
BOOL IsTimer = FALSE;

// buffer to hold an escape character
BOOL InEscBuf = FALSE;
#define EscBufSize  100
TCHAR EscBuf[EscBufSize];
INT EscBufPos = 0;

#define TIMER_ID  666

VOID EnsureTimer()
{
    if (!IsTimer) {
		IsTimer = TRUE;
		SetTimer(GetParent(hWndRtf), TIMER_ID, 100, NULL);
    }
}

VOID DestTimer()
{
    KillTimer(GetParent(hWndRtf), TIMER_ID);
    IsTimer = FALSE;
}

VOID FixCharFormat(CHARFORMAT2* cf)
{
	if (cf->crTextColor == BLACK)
		cf->dwEffects |= CFE_AUTOCOLOR;
	if (cf->crBackColor == WHITE)
		cf->dwEffects |= CFE_AUTOBACKCOLOR;
}

VOID WriteBuffer(LPCTSTR s, INT Len)
{
    CHARRANGE cr;
    CHARFORMAT2 cf;		
	SETTEXTEX stt;

	PuttingChar = TRUE;
    StartOfInput = RtfWindowTextLength();


    cr.cpMin = max(OutputStart, StartOfInput + OutputPos);
    cr.cpMax = cr.cpMin + BufLen;
    SendMessage(hWndRtf, EM_EXSETSEL, 0, (LPARAM) &cr);

    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR | CFM_BACKCOLOR | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE;
    cf.dwEffects = 0;
    cf.crTextColor = BufFormat.ForeColor;
    cf.crBackColor = BufFormat.BackColor;
    cf.dwEffects = (BufFormat.Bold ? CFE_BOLD : 0) |
		   (BufFormat.Italic ? CFE_ITALIC : 0) |
		   (BufFormat.Underline ? CFE_UNDERLINE : 0);
	FixCharFormat(&cf);
    SendMessage(hWndRtf, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf);
    // setcharformat seems to screw up the current selection!

    SendMessage(hWndRtf, EM_EXSETSEL, 0, (LPARAM) &cr);
    //PuttingChar = TRUE;
#if defined _UNICODE
	stt.codepage = CP_UNICODE;
	stt.flags = ST_SELECTION;
	// EM_SETTEXT uses '\0' as a terminator. we replace it with ' ' 
	MemReplaceChars (s,Len,TEXT('\0'),TEXT(' '));
	SendMessage(hWndRtf,EM_SETTEXTEX,(WPARAM)&stt,(LPARAM)s);
#else
    SendMessage(hWndRtf, EM_REPLACESEL, FALSE, (LPARAM) s);
#endif
    //PuttingChar = FALSE;

    StartOfInput = RtfWindowTextLength();
	if (StartOfInput > MAXIMUM_BUFFER) {
		LPCTSTR Blank = TEXT("");
		CHARRANGE cr;

		SendMessage(hWndRtf, EM_HIDESELECTION, TRUE, 0);

		cr.cpMin = 0;
		cr.cpMax = (StartOfInput - MAXIMUM_BUFFER) + (MAXIMUM_BUFFER / 8);
		SendMessage(hWndRtf, EM_EXSETSEL, 0, (LPARAM) &cr);

		PuttingChar = TRUE;
		SendMessage(hWndRtf, EM_REPLACESEL, FALSE, (LPARAM) Blank);
		PuttingChar = FALSE;

		cr.cpMin = -1;
		cr.cpMax = -1;
		SendMessage(hWndRtf, EM_EXSETSEL, 0, (LPARAM) &cr);

		SendMessage(hWndRtf, EM_HIDESELECTION, FALSE, 0);

		StartOfInput = RtfWindowTextLength();
	}
	PuttingChar = FALSE;
}

VOID FlushBuffer(BOOL Force)
{
	if (!Force) {
		BOOL Entered = TryEnterCriticalSection(&CriticalSect);
		if(!Entered)
			return;
	} else
		EnterCriticalSection(&CriticalSect);

	if (BufLen != 0) {
		Buf[BufLen] = 0;
		Buf[BufLen+1] = 0;
		WriteBuffer(Buf, BufLen);
		OutputPos = BufPos - BufLen;
		BufPos = 0;
		BufLen = 0;
	}

	LeaveCriticalSection(&CriticalSect);
}

VOID RtfWindowFlushBuffer(VOID)
{
    FlushBuffer(TRUE);
}

BOOL ParseEscapeCode(Format* f)
{
    INT AnsiColor[8] = {BLACK, RED, GREEN, YELLOW, BLUE,
			MAGENTA, CYAN, WHITE};
    LPTSTR s;
    INT i;

    EscBuf[EscBufPos] = 0;
    if (EscBuf[0] != TEXT('['))
	return FALSE;

    s = &EscBuf[1];
    for (i = 1; i <= EscBufPos; i++) {
	if (EscBuf[i] == TEXT(';'))
	    EscBuf[i] = 0;

	if (EscBuf[i] == 0) {
	    INT Val = StringToInt(s);
	    s = &EscBuf[i+1];

	    if (Val == 0)
		*f = DefFormat;
	    else if (Val == 1)
		f->Bold = TRUE;
	    else if (Val == 4)
		f->Underline = TRUE;
	    else if (Val >= 30 && Val <= 37)
		f->ForeColor = AnsiColor[Val - 30];
	    else if (Val >= 40 && Val <= 47)
		f->BackColor = AnsiColor[Val - 40];
	}
    }
    return TRUE;
}




VOID UpdateFormat(VOID)
{
	if (FormatChanged) {
		if (NowFormat.BackColor != BufFormat.BackColor ||
			NowFormat.ForeColor != BufFormat.ForeColor ||
			NowFormat.Bold      != BufFormat.Bold      ||
			NowFormat.Underline != BufFormat.Underline ||
			NowFormat.Italic    != BufFormat.Italic    )
		{
			FlushBuffer(TRUE);
			BufFormat = NowFormat;
		}
		FormatChanged = FALSE;
	}
}


// need to copy from s to Buf
VOID AddToBufferExt(LPCTSTR s, INT numChars)
{
	UpdateFormat();

	if (InEscBuf) {
		for (; numChars>0; s++, numChars--) {
			if (*s == TEXT('m')) {
				Format f = NowFormat;
				if (ParseEscapeCode(&f)) {
					FormatChanged = TRUE;
					NowFormat = f;
				}
				InEscBuf = FALSE;
				AddToBufferExt(s+1,numChars-1);
				return;
			} else if ((*s >= TEXT('0') && *s <= TEXT('9')) ||
				(*s == TEXT(';')) || (*s == TEXT('['))) {
					EscBuf[EscBufPos++] = *s;
					EscBufPos = min(EscBufPos, EscBufSize);
			} else {
				InEscBuf = FALSE;
				AddToBufferExt(EscBuf,StringLen(EscBuf));
				break;
			}
		}
	}

	for (; numChars>0; s++, numChars--) {
		if (*s == TEXT('\b')) {
			if (BufPos == 0) {
				OutputPos--;
			} else
				BufPos--;
		} else if (*s == TEXT('\07')) {
			Beep( 750, 150 );
		} else if (*s == TEXT('\033')) {
			InEscBuf = TRUE;
			EscBufPos = 0;
			AddToBufferExt(s+1,numChars-1);
			return;
		} else {
			if (BufLen >= BufSize)
				FlushBuffer(TRUE);
			Buf[BufPos++] = *s;
			BufLen = max(BufLen, BufPos);
		}
	}

	EnsureTimer();
}

VOID RtfWindowTimer(VOID)
{
	// if you are doing useful work, why die?
    if (BufLen == 0)
		DestTimer();
    FlushBuffer(FALSE);
}

VOID RtfWindowPutSExt(LPCTSTR s, INT numChars)
{
    AddToBufferExt(s, numChars);
}

VOID RtfWindowPutS(LPCTSTR s)
{
    RtfWindowPutSExt(s,StringLen(s));
}



VOID RtfEchoCommand(LPCTSTR s)
{
    RtfWindowPutS(s);
    RtfWindowPutS(TEXT("\n"));
}

VOID RtfWindowStartOutput()
{
    RtfWindowPutS(TEXT("\n"));
    RtfWindowFlushBuffer();
    BufFormat = DefFormat;
    NowFormat = DefFormat;
    OutputStart = RtfWindowTextLength();
}

VOID RtfWindowStartInput()
{
    CHARRANGE cr;
    CHARFORMAT cf;
    cf.cbSize = sizeof(cf);
    cf.dwMask = CFM_COLOR;
    cf.dwEffects = 0;
    cf.crTextColor = BLACK;

    cr.cpMin = StartOfInput;
    cr.cpMax = -1;
    SendMessage(hWndRtf, EM_EXSETSEL, 0, (LPARAM) &cr);

    SendMessage(hWndRtf, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf);

    cr.cpMax = cr.cpMin;
    SendMessage(hWndRtf, EM_EXSETSEL, 0, (LPARAM) &cr);
}

INT winGhciColor(INT Color)
{
    INT PrevColor = NowFormat.ForeColor;
    FormatChanged = TRUE;
    //NowFormat = DefFormat;
    NowFormat.ForeColor = Color;
    // InEscBuf = FALSE;
	UpdateFormat();

    return PrevColor;
}

BOOL winGhciBold(BOOL Bold)
{
    BOOL PrevBold = NowFormat.Bold;
    FormatChanged = TRUE;
    //NowFormat = DefFormat;
    NowFormat.Bold = Bold;

    // InEscBuf = FALSE;
	UpdateFormat();

    return PrevBold;
}

/////////////////////////////////////////////////////////////////////
// IO REDIRECTORS
/////////////////////////////////////////////////////////////////////

VOID WinGhciHyperlink(LPCTSTR msg)
{
    CHARFORMAT2 cf2;
    FlushBuffer(TRUE);

    cf2.cbSize = sizeof(cf2);
    cf2.dwMask = CFM_LINK;
    cf2.dwEffects = CFE_LINK;

    SendMessage(hWndRtf, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);
    SendMessage(hWndRtf, EM_REPLACESEL, FALSE, (LPARAM) msg);
    StartOfInput += StringLen(msg);
    cf2.dwEffects = 0;
    SendMessage(hWndRtf, EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM) &cf2);
}


#if 0
// need to copy from s to Buf
VOID AddToBuffer(LPCTSTR s)
{
	if (FormatChanged) {
		if (NowFormat.BackColor != BufFormat.BackColor ||
			NowFormat.ForeColor != BufFormat.ForeColor ||
			NowFormat.Bold      != BufFormat.Bold      ||
			NowFormat.Underline != BufFormat.Underline ||
			NowFormat.Italic    != BufFormat.Italic    )
		{
			FlushBuffer(TRUE);
			BufFormat = NowFormat;
		}
		FormatChanged = FALSE;
	}

	if (InEscBuf) {
		for (; *s != 0; s++) {
			if (*s == TEXT('m')) {
				Format f = NowFormat;
				if (ParseEscapeCode(&f)) {
					FormatChanged = TRUE;
					NowFormat = f;
				}
				InEscBuf = FALSE;
				AddToBuffer(s+1);
				return;
			} else if ((*s >= TEXT('0') && *s <= TEXT('9')) ||
				(*s == TEXT(';')) || (*s == TEXT('['))) {
					EscBuf[EscBufPos++] = *s;
					EscBufPos = min(EscBufPos, EscBufSize);
			} else {
				InEscBuf = FALSE;
				AddToBuffer(EscBuf);
				break;
			}
		}
	}

	for (; *s != TEXT('\0'); s++) {
		if (*s == TEXT('\b')) {
			if (BufPos == 0) {
				OutputPos--;
			} else
				BufPos--;
		} else if (*s == TEXT('\07')) {
			Beep( 750, 150 );
		} else if (*s == TEXT('\033')) {
			InEscBuf = TRUE;
			EscBufPos = 0;
			AddToBuffer(s+1);
			return;
		} else {
			if (BufLen >= BufSize)
				FlushBuffer(TRUE);
			Buf[BufPos++] = *s;
			BufLen = max(BufLen, BufPos);
		}
	}

	EnsureTimer();
}
#endif