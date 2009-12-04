#include "CommonIncludes.h"



extern HINSTANCE hThisInstance;

extern HWND hWndMain, hWndStatus, hWndToolbar, hWndRtf;
extern HANDLE hStdoutPrinterThread; 


extern BOOL Running;

extern HANDLE hChildStdinRd, hChildStdinWr, hChildStdoutRd, hChildStdoutWr, hChildStderrRd, hChildStderrWr;

extern HANDLE hEventCtrlBreak, hKillGHCi, hSigSuspendStdoutPrinterThread, hSigStdoutPrinterThreadSuspended, hSigStopPrintGHCiOutput, hSigResumeStdoutPrinterThread;




BOOL CreateGHCiProcess(LPTSTR);



VOID SendToGHCiStdin(LPCTSTR Str);
VOID SendToGHCiStdinLn(LPCTSTR Str);


VOID FinalizeWinGHCi(VOID);

VOID PrintGHCiOutput(HANDLE hHandle, INT color);
VOID PrintGHCiOutputIfAvailable(HANDLE hHandle, INT color);


// These string are used as markers at the begining and end of the prompt in order to detect end of evaluation.
// Currently, GHCi does not use utf8 to print the prompt, so chars in this sequence must be less than \128
// In addtion, these strings must be 2 chars long
#define END_OF_PROMPT				TEXT("\33\10")
#define END_OF_PROMPT_FIRST_CHAR    TEXT('\33')
#define END_OF_PROMPT_LENGTH		2

#define BEGIN_OF_PROMPT				TEXT("\33\7")
#define BEGIN_OF_PROMPT_FIRST_CHAR  TEXT('\33')
#define BEGIN_OF_PROMPT_LENGTH		2
