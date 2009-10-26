/******************************************************************************

	StartProcess starts a process with its command line

	Pepe Gallardo, 2009-March

*******************************************************************************/
 
#pragma warning(disable : 4996)

#define STRICT
#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
 
#define StringCat(xs,ys)			_tcscat((xs),(ys))
#define StringCpy(xs,ys)			_tcscpy((xs),(ys))

 
INT wmain(INT argc, TCHAR *argv[], TCHAR *envp[])
{
	 STARTUPINFO         si;
	 PROCESS_INFORMATION pi;
	 BOOL f;
	 TCHAR Buffer[10*MAX_PATH];
	 INT i;


	 // Make sure that we've been passed the right number of arguments
	 if (argc < 2) {
		 wprintf(
			 TEXT("Usage: %s <process> <args>\n"), 
			 argv[0]);
		 return(0);
	 }


	 ZeroMemory( &si, sizeof(STARTUPINFO) );
     ZeroMemory( &pi, sizeof(PROCESS_INFORMATION) );


	StringCpy(Buffer,argv[1]);
	for(i=2;i<argc;i++) {
		StringCat(Buffer, TEXT(" "));
		if(i==2)
			StringCat(Buffer, TEXT("\""));
		StringCat(Buffer,argv[i]);
	}
	StringCat(Buffer, TEXT("\""));

	 // Spawn process 
	 f = CreateProcess(NULL, Buffer, NULL, NULL, TRUE, 
		 0, NULL, NULL, &si, &pi);

	 if (f) {
		 CloseHandle(pi.hThread);
		 CloseHandle(pi.hProcess);
	 }

	 return 0;
 }
 
