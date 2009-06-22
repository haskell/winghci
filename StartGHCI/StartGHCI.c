/******************************************************************************

	StartGHCI starts GHCI process and sends ctrl-c signals to it

	Pepe Gallardo, 2009-March

*******************************************************************************/
 
#pragma warning(disable : 4996)

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
 
#define StringCat(xs,ys)			_tcscat((xs),(ys))
#define StringCpy(xs,ys)			_tcscpy((xs),(ys))
#define StringToInt(xs)				_tstoi((xs))
 
 BOOL CtrlHandler( DWORD fdwCtrlType ) 
{ 
  switch( fdwCtrlType ) 
  { 
    // StartGHCI process ignores CTRL-BREAK signal  
    case CTRL_BREAK_EVENT: 
      return TRUE; 
    default: 
      return FALSE; 
  } 
} 
 
 
INT wmain(INT argc, TCHAR *argv[], TCHAR *envp[])
{

	 STARTUPINFO         si;
	 PROCESS_INFORMATION pi;
	 DWORD ExitCode = 0;
	 BOOL f;
	 TCHAR Buffer[1024];
	 INT i;

	 HANDLE h[3];    

	 // Make sure that we've been passed the right number of arguments
	 if (argc < 4) {
		 wprintf(
			 TEXT("Usage: %s (InheritableEventHandle) (InheritableEventHandle) (process)\n"), 
			 argv[0]);
		 return(0);
	 }

	 h[0] = (HANDLE) StringToInt(argv[1]);  // The inherited event handle
	 h[1] = (HANDLE) StringToInt(argv[2]);

	 StringCpy(Buffer,argv[3]);
	 for(i=4;i<argc;i++) {
		StringCat(Buffer, TEXT(" "));
		StringCat(Buffer,argv[i]);
	 }


	 // set system code page
	 SetConsoleOutputCP(GetACP());

	 ZeroMemory( &si, sizeof(STARTUPINFO) );
     ZeroMemory( &pi, sizeof(PROCESS_INFORMATION) );

	 // Spawn ghci process in the same process group
	 f = CreateProcess(NULL, Buffer, NULL, NULL, TRUE, 
		 0, NULL, NULL, &si, &pi);

	 SetConsoleCtrlHandler( (PHANDLER_ROUTINE) CtrlHandler, TRUE );

	 if (f) {
		 DWORD signal;
		 CloseHandle(pi.hThread);
		 h[2] = pi.hProcess;

		 // Wait for the spawned-process to die or for the event
		 // indicating that ghci should be ctrl-c'ed.
		 do {
			 signal = WaitForMultipleObjects(3, h, FALSE, INFINITE);
			 switch (signal) {
			  case WAIT_OBJECT_0 + 0:            	
				  GenerateConsoleCtrlEvent(CTRL_BREAK_EVENT, 0);  
				  ResetEvent(h[0]);
				  break;

			  case WAIT_OBJECT_0 + 1:  // Request to kill ghci process
				  TerminateProcess(pi.hProcess,0);
				  break;

			  case WAIT_OBJECT_0 + 2:  // GHCI app terminated normally
				  GetExitCodeProcess(pi.hProcess, &ExitCode);
				  break;
			 }
		 } while(signal==WAIT_OBJECT_0 + 0);
		 CloseHandle(pi.hProcess);
	 }
	 CloseHandle(h[0]);   // Close the inherited event handle
	 return(ExitCode);
}
 
