#include "CommonIncludes.h"



extern HINSTANCE hThisInstance;

extern HWND hWndMain, hWndStatus, hWndToolbar, hWndRtf;
extern HANDLE hStdoutPrinterThread; 


extern BOOL Running;

extern HANDLE hChildStdinRd, hChildStdinWr, hChildStdoutRd, hChildStdoutWr, hChildStderrRd, hChildStderrWr;

extern HANDLE hEventCtrlBreak, hKillGHCI, hSigSuspendStdoutPrinterThread, hSigStdoutPrinterThreadSuspended, hSigStopPrintGhciOutput, hSigResumeStdoutPrinterThread;




BOOL CreateGHCIProcess(VOID);



VOID SendToGHCIStdin(LPCTSTR Str);
VOID SendToGHCIStdinLn(LPCTSTR Str);


VOID FinalizeWinGhci(VOID);

VOID PrintGhciOutput(HANDLE hHandle, INT color);
VOID PrintGhciOutputIfAvailable(HANDLE hHandle, INT color);


// These string are used as markers at the begining and end of the prompt in order to detect end of evaluation.
// Currently, ghci does not use utf8 to print the prompt, so chars in this sequence must be less than \128
// In addtion, these strings must be 2 chars long
#define END_OF_PROMPT				TEXT("\33\10")
#define END_OF_PROMPT_FIRST_CHAR    TEXT('\33')
#define END_OF_PROMPT_LENGTH		2

#define BEGIN_OF_PROMPT				TEXT("\33\7")
#define BEGIN_OF_PROMPT_FIRST_CHAR  TEXT('\33')
#define BEGIN_OF_PROMPT_LENGTH		2
