/******************************************************************************
	WinGhci, a GUI for GHCI

	WinGhci.c: Initialization of GHCI interpreter and comunication with it
	through stdin, stdout and stderr
	
	Pepe Gallardo, 2009-March
*******************************************************************************/

#include "CommonIncludes.h"
#include "Colors.h"
#include "DlgOpts.h"
#include "DlgTools.h"
#include "General.h"
#include "History.h"
#include "Registry.h"
#include "RtfWindow.h"
#include "Strings.h"
#include "Utf8.h"
#include "WndMain.h"
#include "WinGhci.h"

#include <locale.h>

#define BUFFER_MAXLEN			128
#define UNICODE_BUFFER_MAXLEN	5*BUFFER_MAXLEN

HANDLE hChildStdinRd, hChildStdinWr, hChildStderrWr, hChildStderrRd, 
	   hChildStdoutRd, hChildStdoutWr;

HANDLE hEventCtrlBreak, hKillGHCI;

BOOL CreateGHCIProcess(VOID); 
VOID ErrorExit(LPTSTR); 


HINSTANCE hThisInstance;

HWND hWndMain, hWndStatus, hWndToolbar, hWndRtf;
HANDLE hStdoutPrinterThread, hStderrPrinterThread, hReadGHCIStdoutThread; 
HANDLE hGhciProcess, hGhciThread;


CRITICAL_SECTION PrinterCritSect;

HANDLE hSigStopPrintGhciOutput;

HANDLE hSigSuspendStdoutPrinterThread, hSigStdoutPrinterThreadSuspended, hSigResumeStdoutPrinterThread;


#define printf(x) ;//MessageBox(hWndMain,x,x,MB_OK);


//#define SwitchThread() SwitchToThread() //	Sleep(0)
#define SwitchThread() Sleep(0)



DWORD WINAPI StdoutPrinterThread( LPVOID lpParam ) ;
BOOL isGhciOutputAvailable(HANDLE hHandle, int nTimes);


// This thread reads output sent by GHCI to stdout, and saves it on a buffer. Other threads
// can read the buffer waiting on hSigGHCIStdoutAvailable and using ReadGHCIStdout

#define READ_GHCI_STDOUT_THREAD_BUFFSIZE 1024

BYTE	ReadGHCIStdoutThreadBuff[READ_GHCI_STDOUT_THREAD_BUFFSIZE];
INT		ReadGHCIStdoutThreadBuffLen;
HANDLE	hSigGHCIStdoutAvailable, hSigReadGHCIStdoutThreadResume;


DWORD WINAPI ReadGHCIStdoutThread( LPVOID lpParam ) 
{
	DWORD BytesRead;

	ReadGHCIStdoutThreadBuffLen = 0;
	hSigGHCIStdoutAvailable = CreateEvent(NULL, FALSE, FALSE, NULL);
	hSigReadGHCIStdoutThreadResume = CreateEvent(NULL, FALSE, FALSE, NULL);

	for(;;) {
		if(ReadGHCIStdoutThreadBuffLen==0) {
			//do {
				ReadFile( hChildStdoutRd, &ReadGHCIStdoutThreadBuff[ReadGHCIStdoutThreadBuffLen]
					    , READ_GHCI_STDOUT_THREAD_BUFFSIZE-ReadGHCIStdoutThreadBuffLen,&BytesRead, NULL);
				ReadGHCIStdoutThreadBuffLen += BytesRead;
			//} while(isGhciOutputAvailable(hChildStdoutRd,1000) && (ReadGHCIStdoutThreadBuffLen<READ_GHCI_STDOUT_THREAD_BUFFSIZE));
			FlushFileBuffers(hChildStdoutRd);           
		}
		
		SignalObjectAndWait(hSigGHCIStdoutAvailable,hSigReadGHCIStdoutThreadResume,INFINITE,FALSE);
	}
	return 0;
}



INT ReadGHCIStdout(BYTE *Bytes, INT BytesMaxLen)
{
	INT Bytes2Copy = min(BytesMaxLen,ReadGHCIStdoutThreadBuffLen);

	memmove(Bytes,ReadGHCIStdoutThreadBuff,Bytes2Copy*sizeof(BYTE));
	ReadGHCIStdoutThreadBuffLen -= Bytes2Copy;
	memmove(ReadGHCIStdoutThreadBuff,&ReadGHCIStdoutThreadBuff[Bytes2Copy],ReadGHCIStdoutThreadBuffLen*sizeof(BYTE));
	SetEvent(hSigReadGHCIStdoutThreadResume);
	return Bytes2Copy;

}

BOOL isGhciOutputAvailable(HANDLE hHandle, INT nTimes);

BOOL isGHCIStdoutAvailable(VOID)
{
	if (ReadGHCIStdoutThreadBuffLen>0)
		return TRUE;
	else
		return FALSE; //isGhciOutputAvailable(hChildStdoutRd,1);

}

// StderrPrinterThread thread reads output sent by GHCI to stderr, and prints it in RED color on WinGhci window
BOOL isGhciOutputAvailable(HANDLE hHandle, INT nTimes)
{
	TCHAR Buf[BUFFER_MAXLEN];
	DWORD Read, Available;
	INT i;

	for(i=0; i<nTimes; i++) {						
		    PeekNamedPipe(hHandle,Buf,BUFFER_MAXLEN,&Read,&Available,NULL);
			if(Available)
				return Available;
			//SwitchThread();
	}
	return Available;
}

DWORD WINAPI StderrPrinterThread( LPVOID lpParam ) 
{
	BYTE Bytes[BUFFER_MAXLEN]; 
	INT svColor, BytesLen, BytesRead
			   , conversionResult, UnicodeBufferLen, UnicodeCharsConverted;
	TCHAR UnicodeBuffer[UNICODE_BUFFER_MAXLEN];

	BytesLen = 0;
	UnicodeBufferLen = 0;

	for(;;)	{
		// ghci sends a multibyte sequence through stdout and stderr
		ReadFile(hChildStderrRd,&Bytes[BytesLen],BUFFER_MAXLEN-BytesLen,&BytesRead,NULL);
		BytesLen += BytesRead;
		FlushFileBuffers(hChildStderrRd);

		//while(isGHCIStdoutAvailable());

		EnterCriticalSection(&PrinterCritSect);
		svColor = winGhciColor(RED);

		while (isGhciOutputAvailable(hChildStderrRd,1000)) {

			conversionResult = LocalCodePageToUnicode(Bytes, BytesLen, &UnicodeBuffer[UnicodeBufferLen], UNICODE_BUFFER_MAXLEN-UnicodeBufferLen, &UnicodeCharsConverted);
			UnicodeBufferLen += UnicodeCharsConverted;

			if(conversionResult>=0) {
				// save non converted BYTES for next iteration
				BytesLen = BytesLen-conversionResult;
				memmove(Bytes,&Bytes[conversionResult],BytesLen*sizeof(BYTE));
			} else
				BytesLen = 0;

			RtfWindowPutSExt(UnicodeBuffer,UnicodeBufferLen);
			RtfWindowFlushBuffer();
			UnicodeBufferLen = 0;

			ReadFile(hChildStderrRd,&Bytes[BytesLen],BUFFER_MAXLEN-BytesLen,&BytesRead,NULL);
			BytesLen += BytesRead;
			FlushFileBuffers(hChildStderrRd);

		}

		conversionResult = LocalCodePageToUnicode(Bytes, BytesLen, &UnicodeBuffer[UnicodeBufferLen], UNICODE_BUFFER_MAXLEN-UnicodeBufferLen, &UnicodeCharsConverted);
		UnicodeBufferLen += UnicodeCharsConverted;

		if(conversionResult>=0) {
			// save non converted BYTES for next iteration
			BytesLen = BytesLen-conversionResult;
			memmove(Bytes,&Bytes[conversionResult],BytesLen*sizeof(BYTE));
		} else
			BytesLen = 0;

		RtfWindowPutSExt(UnicodeBuffer,UnicodeBufferLen);
		RtfWindowFlushBuffer();
		UnicodeBufferLen = 0;

		winGhciColor(svColor);
		LeaveCriticalSection(&PrinterCritSect);
	}
	return 0;
}



// These functions are used to send data to GHCI through stdin
VOID SendToGHCIStdinExt(LPCTSTR Str, BOOL SendNewline)
{
	#define UTF8_MAXLEN    2048 
	BYTE Utf8Bytes[UTF8_MAXLEN];
	INT Utf8BytesLen;
	DWORD BytesWritten;

    //UnicodeToUtf8(Str, Utf8Bytes, UTF8_MAXLEN, &Utf8BytesLen);
	UnicodeToLocalCodePage(Str, Utf8Bytes, UTF8_MAXLEN, &Utf8BytesLen);

	WriteFile(hChildStdinWr, Utf8Bytes, Utf8BytesLen, &BytesWritten, NULL); 

	if(SendNewline) {
		//UnicodeToUtf8(TEXT("\r\n"), Utf8Bytes, UTF8_MAXLEN, &Utf8BytesLen);
		UnicodeToLocalCodePage(TEXT("\r\n"), Utf8Bytes, UTF8_MAXLEN, &Utf8BytesLen);

		WriteFile(hChildStdinWr, Utf8Bytes, Utf8BytesLen, &BytesWritten, NULL); 
	}

	FlushFileBuffers(hChildStdinWr);
}


VOID SendToGHCIStdin(LPCTSTR Str)
{
	SendToGHCIStdinExt(Str, FALSE);
}

VOID SendToGHCIStdinLn(LPCTSTR Str)
{
	SendToGHCIStdinExt(Str, TRUE);
}


VOID PrintGhciOutput(HANDLE hHandle, INT color) 
{
	// ghci sends a multibyte sequence through stdout and stderr
	BYTE Bytes[BUFFER_MAXLEN]; 
	INT svColor, BytesLen
			   , BytesRead
			   , UnicodeCharsConverted
			   , conversionResult, UnicodeBufferLen, UnicodeBufferAuxLen = 0
			   , promptShown;
	BOOL svBold, beginOfPromptFound = FALSE
		       , endOfPromptFound = FALSE
			   , firstCharOfBeginOfPromptFound = FALSE
			   , firstCharOfEndOfPromptFound = FALSE;
    #define UNICODE_BUFFER_MAXLEN  5*BUFFER_MAXLEN
	TCHAR UnicodeBuffer[UNICODE_BUFFER_MAXLEN], *p, 
		  UnicodeBufferAux[UNICODE_BUFFER_MAXLEN];	
	DWORD signal;
	BOOL printing = FALSE;

	HANDLE h[2];

	h[0] = hSigStopPrintGhciOutput;
	h[1] = hSigGHCIStdoutAvailable;


	SwitchThread();
	
	BytesLen = 0;
	UnicodeBufferLen = 0;

	
	while(!beginOfPromptFound) {
		signal = WaitForMultipleObjects(2, h, FALSE, INFINITE);
		switch (signal) {
			  case WAIT_OBJECT_0 + 0:  
				goto end; 
				break;			
			  case WAIT_OBJECT_0 + 1:   
				BytesRead = ReadGHCIStdout(&Bytes[BytesLen], BUFFER_MAXLEN-BytesLen);
				BytesLen += BytesRead;
				break;

		}

		conversionResult = LocalCodePageToUnicode( Bytes, BytesLen, &UnicodeBuffer[UnicodeBufferLen]
		                                         , UNICODE_BUFFER_MAXLEN-UnicodeBufferLen, &UnicodeCharsConverted);
		UnicodeBufferLen += UnicodeCharsConverted;

		if(conversionResult>=0) {
			// save non converted BYTES for next iteration
			BytesLen = BytesLen-conversionResult;
			memmove(Bytes,&Bytes[conversionResult],BytesLen*sizeof(BYTE));
		} else
			BytesLen = 0;

		if(p = MemSearchString(UnicodeBuffer,UnicodeBufferLen,BEGIN_OF_PROMPT)) {
			beginOfPromptFound = TRUE;
			// Move TCHARS after BEGIN_OF_PROMPT marker to UnicodeBufferAux 
			UnicodeBufferAuxLen = UnicodeBufferLen-(BEGIN_OF_PROMPT_LENGTH+p-UnicodeBuffer);
			memcpy(UnicodeBufferAux,p+BEGIN_OF_PROMPT_LENGTH,UnicodeBufferAuxLen*sizeof(TCHAR));
			UnicodeBufferLen = p-UnicodeBuffer;
		} else {
			firstCharOfBeginOfPromptFound = (UnicodeBuffer[UnicodeBufferLen-1] == BEGIN_OF_PROMPT_FIRST_CHAR);
			// save char for next iteration
			if(firstCharOfBeginOfPromptFound)
				UnicodeBufferLen--;
		}

		if(!printing) {
			EnterCriticalSection(&PrinterCritSect);
			printing = TRUE;
		}
			svColor = winGhciColor(color);
			// output converted TCHARS
			RtfWindowPutSExt(UnicodeBuffer,UnicodeBufferLen);
			//RtfWindowFlushBuffer();
			winGhciColor(svColor);
			//SwitchThread();
		if(!isGHCIStdoutAvailable()) {
			printing = FALSE;
			LeaveCriticalSection(&PrinterCritSect);
		}

		// save BEGIN_OF_PROMPT_FIRST_CHAR for next iteration
		if(firstCharOfBeginOfPromptFound) {
			UnicodeBuffer[0] = BEGIN_OF_PROMPT_FIRST_CHAR;
			UnicodeBufferLen = 1;
			firstCharOfBeginOfPromptFound = FALSE;
		}
		else
			UnicodeBufferLen = 0;
	}

	SwitchThread();
	// now, find end of prompt
	#define MAX_SHOW_PROMPT	256
	UnicodeBufferLen = UnicodeBufferAuxLen;
	memcpy(UnicodeBuffer,UnicodeBufferAux,UnicodeBufferLen*sizeof(TCHAR));
	promptShown = 0;
	while(!endOfPromptFound && (promptShown<MAX_SHOW_PROMPT)) {

		if(p = MemSearchString(UnicodeBuffer,UnicodeBufferLen,END_OF_PROMPT)) {
			endOfPromptFound = TRUE;
			// Move TCHARS after BEGIN_OF_PROMPT marker to UnicodeBufferAux 
			UnicodeBufferAuxLen = UnicodeBufferLen-(END_OF_PROMPT_LENGTH+p-UnicodeBuffer);
			memcpy(UnicodeBufferAux,p+END_OF_PROMPT_LENGTH,UnicodeBufferAuxLen*sizeof(TCHAR));
			UnicodeBufferLen = p-UnicodeBuffer;
		} else {
			firstCharOfEndOfPromptFound = (UnicodeBuffer[UnicodeBufferLen-1] == END_OF_PROMPT_FIRST_CHAR);
			// save char for next iteration
			if(firstCharOfEndOfPromptFound)
				UnicodeBufferLen--;
		}


		if(!printing) {
			EnterCriticalSection(&PrinterCritSect);
			printing = TRUE;
		}
			svColor = winGhciColor(PROMPT_COLOR);
			svBold = winGhciBold(TRUE);
			// output converted TCHARS
			RtfWindowPutSExt(UnicodeBuffer,UnicodeBufferLen);
			promptShown += UnicodeBufferLen;
			//RtfWindowFlushBuffer();
			winGhciBold(svBold);
			winGhciColor(svColor);
			//SwitchThread();
		if(!isGHCIStdoutAvailable()) {
			printing = FALSE;
			LeaveCriticalSection(&PrinterCritSect);
		}

		if(endOfPromptFound)
			break;

		// save END_OF_PROMPT_FIRST_CHAR for next iteration
		if(firstCharOfEndOfPromptFound) {
			UnicodeBuffer[0] = END_OF_PROMPT_FIRST_CHAR;
			UnicodeBufferLen = 1;
			firstCharOfEndOfPromptFound = FALSE;
		}
		else
			UnicodeBufferLen = 0;

		signal = WaitForMultipleObjects(2, h, FALSE, INFINITE);
		switch (signal) {
			  case WAIT_OBJECT_0 + 0:  
				goto end; 
				break;			
			  case WAIT_OBJECT_0 + 1:   
				BytesRead = ReadGHCIStdout(&Bytes[BytesLen], BUFFER_MAXLEN-BytesLen);
				BytesLen += BytesRead;
				break;

		}

		conversionResult = LocalCodePageToUnicode( Bytes, BytesLen, &UnicodeBuffer[UnicodeBufferLen]
		                                         , UNICODE_BUFFER_MAXLEN-UnicodeBufferLen, &UnicodeCharsConverted);
		UnicodeBufferLen += UnicodeCharsConverted;

		if(conversionResult>=0) {
			// save non converted BYTES for next iteration
			BytesLen = BytesLen-conversionResult;
			memmove(Bytes,&Bytes[conversionResult],BytesLen*sizeof(BYTE));
		} else
			BytesLen = 0;

	}



	// print chars after end of prompt, if any
	if(!printing) {
		EnterCriticalSection(&PrinterCritSect);
		printing = TRUE;
	}
		svColor = winGhciColor(color);
		// output converted TCHARS
		RtfWindowPutSExt(TEXT(" "),1);	
		RtfWindowPutSExt(UnicodeBufferAux,UnicodeBufferAuxLen);
		winGhciColor(svColor);
		//SwitchThread();
end:		
	RtfWindowFlushBuffer();
	if(printing) {
		printing = FALSE;
		LeaveCriticalSection(&PrinterCritSect);
	}

}




DWORD WINAPI StdoutPrinterThread( LPVOID lpParam ) 
{
	// ghci sends a multibyte sequence through stdout and stderr
	char Bytes[BUFFER_MAXLEN]; 
	INT svColor, svBold, BytesLen
		       , BytesRead, conversionResult, UnicodeBufferLen, UnicodeCharsConverted;
	TCHAR UnicodeBuffer[UNICODE_BUFFER_MAXLEN];
	LPTSTR p, p2;
	
	HANDLE h[2];    
	DWORD signal;

	h[1] = hSigGHCIStdoutAvailable;
	h[0] = hSigSuspendStdoutPrinterThread;

	BytesLen = 0;
	UnicodeBufferLen = 0;

	for(;;) {

		signal = WaitForMultipleObjects(2, h, FALSE, INFINITE);
		switch (signal) {
			  case WAIT_OBJECT_0 + 1:            	
				  BytesRead = ReadGHCIStdout(&Bytes[BytesLen], BUFFER_MAXLEN-BytesLen);
				  BytesLen += BytesRead;
				  break;

			  case WAIT_OBJECT_0 + 0:  // kill this thread
				  SetEvent(hSigStdoutPrinterThreadSuspended);
				  WaitForSingleObject(hSigResumeStdoutPrinterThread,INFINITE);
				  //ExitThread(0);
				  break;
		}


		conversionResult = LocalCodePageToUnicode(Bytes, BytesLen, &UnicodeBuffer[UnicodeBufferLen], UNICODE_BUFFER_MAXLEN-UnicodeBufferLen, &UnicodeCharsConverted);
		UnicodeBufferLen += UnicodeCharsConverted;

		if(conversionResult>=0) {
			// save non converted BYTES for next iteration
			BytesLen = BytesLen-conversionResult;
			memmove(Bytes,&Bytes[conversionResult],BytesLen*sizeof(BYTE));
		} else
			BytesLen = 0;

		while(UnicodeBufferLen>0) {
			EnterCriticalSection(&PrinterCritSect);

			if(p=MemSearchString(UnicodeBuffer,UnicodeBufferLen,BEGIN_OF_PROMPT)) {
				INT rem = UnicodeBufferLen-(p-UnicodeBuffer)-BEGIN_OF_PROMPT_LENGTH;


				// print text before prompt
				RtfWindowPutSExt(UnicodeBuffer,p-UnicodeBuffer);

				svColor = winGhciColor(PROMPT_COLOR);
				svBold = winGhciBold(TRUE);	

				p += BEGIN_OF_PROMPT_LENGTH;
				if(p2=MemSearchString(p,rem,END_OF_PROMPT)) {
					// print prompt
					RtfWindowPutSExt(p,p2-p);
					winGhciColor(svColor);
					winGhciBold(svBold);
					RtfWindowPutS(TEXT(" "));

					p2 += END_OF_PROMPT_LENGTH;
					rem = rem -(p2-p);

					// save remaining text for next iteration
					memcpy(UnicodeBuffer,p2,rem*sizeof(TCHAR));
					UnicodeBufferLen = rem;

				} else {
					// no end of prompt found. Print all in PROMPT_COLOR and bold
					RtfWindowPutSExt(p,rem);
					winGhciColor(svColor);
					winGhciBold(svBold);
					UnicodeBufferLen = 0;
				}

				
				RtfWindowFlushBuffer();

			} else {

				svColor = winGhciColor(BLACK);
				RtfWindowPutSExt(UnicodeBuffer,UnicodeBufferLen);
				UnicodeBufferLen = 0;
				RtfWindowFlushBuffer();
				winGhciColor(svColor);
			}

			LeaveCriticalSection(&PrinterCritSect);
		}
	}


	return 0;
}

BOOL CreateGHCIProcess(VOID) 
{ 
	TCHAR szCmdline[2*MAX_PATH];
	PROCESS_INFORMATION piProcInfo; 
	STARTUPINFO siStartInfo;
	BOOL bFuncRetn = FALSE; 


	// Set up members of the PROCESS_INFORMATION structure. 
	ZeroMemory( &piProcInfo, sizeof(PROCESS_INFORMATION) );

	// Set up members of the STARTUPINFO structure. 
	ZeroMemory( &siStartInfo, sizeof(STARTUPINFO) );
	siStartInfo.cb = sizeof(STARTUPINFO); 
	siStartInfo.hStdError = hChildStderrWr;
	siStartInfo.hStdOutput = hChildStdoutWr;
	siStartInfo.hStdInput = hChildStdinRd;
	siStartInfo.dwFlags |= (STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW);
	siStartInfo.wShowWindow = SW_HIDE;


	// Create the child process. 
	wsprintf( szCmdline,TEXT("%s%s %d %d %s"), GetWinGhciInstallDir(), TEXT("StartGHCI.exe")
		    , hEventCtrlBreak, hKillGHCI, ComboGetValue(GHCI_Combo_Startup));



	bFuncRetn = CreateProcess(NULL, 
		szCmdline,     // command line 
		NULL,          // process security attributes 
		NULL,          // primary thread security attributes 
		TRUE,          // handles are inherited 
		CREATE_NEW_PROCESS_GROUP,             // creation flags 
		NULL,          // use parent's environment 
		NULL,          // use parent's current directory 
		&siStartInfo,  // STARTUPINFO pointer 
		&piProcInfo);  // receives PROCESS_INFORMATION 

	if (bFuncRetn == 0) 
		ErrorExit(TEXT("CreateProcess failed\n"));
	else 
	{
		//hGhciProcess = piProcInfo.hProcess;
		//hGhciThread = piProcInfo.hThread;
		
		CloseHandle(piProcInfo.hProcess);
		CloseHandle(piProcInfo.hThread);
	
	}	

	//Sleep(500);
	SendOptions2GHCI();


	return bFuncRetn;
}

#define REG_DIRECTORY TEXT("WorkingDir")

VOID InitializeWinGhci(VOID)
{
	TCHAR Buffer[MAX_PATH];

	// Get from registry ghc installation dir
	GetGhcInstallDir();

	// Get from registry working directory, and set it
	readRegStr(REG_DIRECTORY, TEXT(""), Buffer);
	SetCurrentDirectory(Buffer);

	// Get GHCI options
	InitOptions();
	
	InitTools();
}


VOID FinalizeWinGhci(VOID) 
{
	TCHAR Buffer[MAX_PATH];

	RegistryWriteWindowPos(hWndMain);
	WriteHistory2Registry();

	GetCurrentDirectory(MAX_PATH,Buffer);
	writeRegString(REG_DIRECTORY, Buffer);

	FinalizeOptions();

	FinalizeTools();

}


INT APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpszCmdLine, INT nCmdShow)
{ 
	SECURITY_ATTRIBUTES saAttr; 
	BOOL fSuccess; 
	MSG msg;
    LPTSTR *Arglist;
    INT nArgs;
	TCHAR File2Load[MAX_PATH] = TEXT("");
	DWORD dwPrinterThreadId;
	HACCEL hAccelTable;


	setlocale( LC_ALL, ".ACP" );


	hThisInstance = hInstance;   


	LoadLibrary(TEXT("RICHED20.DLL"));
	InitCommonControls();
	


	hAccelTable = LoadAccelerators(hThisInstance, MAKEINTRESOURCE(WINGHCIACCELERATORS));
	if(!CreateMainDialog())
		ErrorExit(TEXT("CreateMainDialog failed\n"));

	InitializeWinGhci();



    Arglist = CommandLineToArgvW(GetCommandLineW(), &nArgs);
    if(!Arglist )  {
      ErrorExit(TEXT("CommandLineToArgvW failed\n"));
    }

	if(nArgs==2) {
		StringCpy(File2Load,Arglist[1]);
		SetWorkingDirToFileLoc(File2Load,FALSE);
	}

   LocalFree(Arglist);



	// Set the bInheritHandle flag so pipe handles are inherited. 

	saAttr.nLength = sizeof(SECURITY_ATTRIBUTES); 
	saAttr.bInheritHandle = TRUE; 
	saAttr.lpSecurityDescriptor = NULL; 


	// Create a pipe for the child process's STDOUT. 
	if (! CreatePipe(&hChildStdoutRd, &hChildStdoutWr, &saAttr, 0)) 
		ErrorExit(TEXT("Stdout pipe creation failed\n")); 

	if (! CreatePipe(&hChildStderrRd, &hChildStderrWr, &saAttr, 0)) 
		ErrorExit(TEXT("Stderr pipe creation failed\n")); 


	// Ensure the read handles are  not inherited.
	SetHandleInformation(hChildStdoutRd, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(hChildStderrRd, HANDLE_FLAG_INHERIT, 0);

	// Create a pipe for the child process's STDIN. 
	if (! CreatePipe(&hChildStdinRd, &hChildStdinWr, &saAttr, 0)) 
		ErrorExit(TEXT("Stdin pipe creation failed\n")); 

	// Ensure the write handle to the pipe for STDIN is not inherited. 
	SetHandleInformation( hChildStdinWr, HANDLE_FLAG_INHERIT, 0);


	// Create ctrl-break event
    hEventCtrlBreak = CreateEvent(NULL, FALSE, FALSE, NULL);
	SetHandleInformation(hEventCtrlBreak,HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);

	// Create Kill GHCI event
    hKillGHCI = CreateEvent(NULL, FALSE, FALSE, NULL);
	SetHandleInformation(hKillGHCI,HANDLE_FLAG_INHERIT, HANDLE_FLAG_INHERIT);


	// Create other events
	InitializeCriticalSection(&PrinterCritSect);

	hSigSuspendStdoutPrinterThread = CreateEvent(NULL, FALSE, FALSE, NULL);

	hSigStdoutPrinterThreadSuspended = CreateEvent(NULL, FALSE, FALSE, NULL);

	hSigResumeStdoutPrinterThread = CreateEvent(NULL, FALSE, FALSE, NULL);

	hSigStopPrintGhciOutput = CreateEvent(NULL, FALSE, FALSE, NULL);

	//ResetEvent(hEventCtrlBreak); 


	hStderrPrinterThread = CreateThread(NULL,0,StderrPrinterThread,NULL,0,&dwPrinterThreadId);
	if (hStderrPrinterThread == NULL) {
			ErrorExit(TEXT("Create StderrPrinterThread failed\n"));
	}
	

	hReadGHCIStdoutThread = CreateThread(NULL,0,ReadGHCIStdoutThread,NULL,0,&dwPrinterThreadId);
	if (hReadGHCIStdoutThread == NULL) 	{
			ErrorExit(TEXT("Create ReadGHCIStdoutThread failed\n"));
	}


	// Now create the child process. 
	fSuccess = CreateGHCIProcess();
	if (! fSuccess) 
		ErrorExit(TEXT("CreateGHCIProcess failed with"));     
	

	// after initialization, start StdoutPrinterThread
	hStdoutPrinterThread = CreateThread(NULL,0,StdoutPrinterThread,NULL,0,&dwPrinterThreadId);
	if (hStdoutPrinterThread == NULL) 	{
			ErrorExit(TEXT("Create StdoutPrinterThread failed\n"));
	}

	// if command parameters specified a file to load, do it
	if(StringCmp(File2Load,TEXT("")))
		LoadFileExt(File2Load,TRUE);

	// Main message loop
	while (GetMessage(&msg, NULL, 0, 0)) {
		if (!TranslateAccelerator(hWndMain, hAccelTable, &msg))	{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}		
	}

	CloseHandle(hEventCtrlBreak);
	CloseHandle(hSigSuspendStdoutPrinterThread);
	CloseHandle(hSigStdoutPrinterThreadSuspended);
	CloseHandle(hSigResumeStdoutPrinterThread);
	CloseHandle(hSigStopPrintGhciOutput);
	DeleteCriticalSection(&PrinterCritSect);

	return (INT) msg.wParam;
} 


#if 0
VOID PrintGhciOutput2(HANDLE hHandle, INT color) 
{
	// ghci sends a multibyte sequence through stdout and stderr
	BYTE Bytes[BUFFER_MAXLEN]; 
	INT svColor, BytesLen = 0
			   , BytesRead
			   , UnicodeCharsConverted
			   , conversionResult, UnicodeBufferLen, PromptLen = 0;
	BOOL svBold, beginOfPromptFound = FALSE
		       , endOfPromptFound = FALSE
			   , firstCharOfBeginOfPromptFound = FALSE;
    #define UNICODE_BUFFER_MAXLEN  5*BUFFER_MAXLEN
	TCHAR UnicodeBuffer[UNICODE_BUFFER_MAXLEN], *p, 
		  Prompt[UNICODE_BUFFER_MAXLEN], AuxPrompt[UNICODE_BUFFER_MAXLEN];	
	DWORD signal;
	BOOL printing = FALSE;

	HANDLE h[2];

	h[1] = hSigGHCIStdoutAvailable;
	h[0] = hSigStopPrintGhciOutput;



	SwitchThread();
	
	BytesLen = 0;
	UnicodeBufferLen = 0;

	while(!beginOfPromptFound) {
		signal = WaitForMultipleObjects(2, h, FALSE, INFINITE);
		switch (signal) {
			case WAIT_OBJECT_0 + 1:   
				BytesRead = ReadGHCIStdout(&Bytes[BytesLen], BUFFER_MAXLEN-BytesLen);
				BytesLen += BytesRead;
				break;

			  case WAIT_OBJECT_0 + 0:  
				goto end; 
				break;
		}

		conversionResult = LocalCodePageToUnicode(Bytes, BytesLen, &UnicodeBuffer[UnicodeBufferLen], UNICODE_BUFFER_MAXLEN-UnicodeBufferLen, &UnicodeCharsConverted);
		UnicodeBufferLen += UnicodeCharsConverted;

		if(conversionResult>=0) {
			// save non converted BYTES for next iteration
			BytesLen = BytesLen-conversionResult;
			memmove(Bytes,&Bytes[conversionResult],BytesLen*sizeof(BYTE));
		} else
			BytesLen = 0;


		if(p = MemSearchString(UnicodeBuffer,UnicodeBufferLen,BEGIN_OF_PROMPT)) {
			beginOfPromptFound = TRUE;

			// Move TCHARS after BEGIN_OF_PROMPT marker to Prompt 
			PromptLen = UnicodeBufferLen-(BEGIN_OF_PROMPT_LENGTH+p-UnicodeBuffer);
			memcpy(Prompt,p+END_OF_PROMPT_LENGTH,PromptLen*sizeof(TCHAR));
			UnicodeBufferLen = p-UnicodeBuffer;
		} else {
			firstCharOfBeginOfPromptFound = (UnicodeBuffer[UnicodeBufferLen-1] == BEGIN_OF_PROMPT_FIRST_CHAR);
			// save char for next iteration
			if(firstCharOfBeginOfPromptFound)
				UnicodeBufferLen--;
		}

		if(!printing) {
			EnterCriticalSection(&PrinterCritSect);
			printing = TRUE;
		}
		svColor = winGhciColor(color);
		// output converted TCHARS
		RtfWindowPutSExt(UnicodeBuffer,UnicodeBufferLen);
		RtfWindowFlushBuffer();
		winGhciColor(svColor);
		//SwitchThread();
		if(!isGHCIStdoutAvailable()) {
			printing = FALSE;
			LeaveCriticalSection(&PrinterCritSect);
		}


		// save BEGIN_OF_PROMPT_FIRST_CHAR for next iteration
		if(firstCharOfBeginOfPromptFound) {
			UnicodeBuffer[0] = BEGIN_OF_PROMPT_FIRST_CHAR;
			UnicodeBufferLen = 1;
		}
		else
			UnicodeBufferLen = 0;

		firstCharOfBeginOfPromptFound = FALSE;


	}


	// is the whole prompt in Prompt?
	if(p = MemSearchString(Prompt,PromptLen,END_OF_PROMPT)) {

		INT newPromptLen = p - Prompt;

		endOfPromptFound = TRUE;
		// move remaining chars after END_OF_PROMPT to UnicodeBuffer
		UnicodeBufferLen = PromptLen - (END_OF_PROMPT_LENGTH+newPromptLen);
		memmove(UnicodeBuffer,p+END_OF_PROMPT_LENGTH,UnicodeBufferLen*sizeof(TCHAR));

		PromptLen = newPromptLen;
	}

	// Read chars till end of prompt
	while(!endOfPromptFound) {

		signal = WaitForMultipleObjects(2, h, FALSE, INFINITE);
		switch (signal) {
			case WAIT_OBJECT_0 + 1:            	
				BytesRead = ReadGHCIStdout(&Bytes[BytesLen], BUFFER_MAXLEN-BytesLen);
				BytesLen += BytesRead;
				break;

			  case WAIT_OBJECT_0 + 0:  
				goto end; 
				break;
		}


		conversionResult = LocalCodePageToUnicode(Bytes, BytesLen, &Prompt[PromptLen], UNICODE_BUFFER_MAXLEN-PromptLen, &UnicodeCharsConverted);
		PromptLen += UnicodeCharsConverted;

		if(conversionResult>=0) {
				// save non converted BYTES in Bytes2 for next iteration
			    BytesLen = BytesLen-conversionResult;
				memmove(Bytes,&Bytes[conversionResult],BytesLen*sizeof(BYTE));
		} else
				BytesLen = 0;



		// concat partial Prompt and TCHARS readed into AuxPrompt
		memcpy(AuxPrompt,Prompt,PromptLen*sizeof(TCHAR));
		memcpy(&AuxPrompt[PromptLen], UnicodeBuffer, UnicodeBufferLen*sizeof(TCHAR));

		if(p = MemSearchString(AuxPrompt,PromptLen+UnicodeBufferLen,END_OF_PROMPT)) {
		   endOfPromptFound = TRUE;

		   UnicodeBufferLen = PromptLen+UnicodeBufferLen-(p+END_OF_PROMPT_LENGTH-AuxPrompt);
		   if(UnicodeBufferLen>0) {
			   //save remaining unicode chars for next iteration			   
			   memcpy(UnicodeBuffer,p+END_OF_PROMPT_LENGTH,UnicodeBufferLen*sizeof(TCHAR));
		   }

		   PromptLen = p-AuxPrompt;
		   memcpy(Prompt,AuxPrompt,PromptLen*sizeof(TCHAR));	

		}

	}

	SwitchThread();
	if(!printing) {
			EnterCriticalSection(&PrinterCritSect);
			printing = TRUE;
	}
	
	// print the prompt
	svColor = winGhciColor(PROMPT_COLOR);
	svBold = winGhciBold(TRUE);
	RtfWindowPutSExt(Prompt,PromptLen);
	winGhciBold(svBold);
	winGhciColor(svColor);
	RtfWindowPutSExt(TEXT(" "),1);	
	RtfWindowFlushBuffer();
	

end:
	if(printing) {
		printing = FALSE;
		LeaveCriticalSection(&PrinterCritSect);
	}
	return;
}











#endif