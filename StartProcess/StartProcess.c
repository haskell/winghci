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
	 TCHAR AppName[2*MAX_PATH], CmdLine[10*MAX_PATH];
	 LPTSTR lpFilePart; 
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


    // search application name in path
    SearchPath(NULL,argv[1],TEXT(".exe"),2*MAX_PATH,AppName,&lpFilePart);


	//StringCpy(AppName,argv[1]); // application name

	StringCpy(CmdLine,argv[1]); // repeat application full name
    StringCat(CmdLine, TEXT(" "));
    if(argc>2)
		StringCat(CmdLine, TEXT("\"")); // long file names are sorrounded by " ... "
    //StringCat(CmdLine,argv[2]);
	for(i=2;i<argc;i++) {
		//StringCat(CmdLine, TEXT(" "));
		//if(i==2)
		//	StringCat(CmdLine, TEXT("\""));
		StringCat(CmdLine,argv[i]);
		if(i<argc-1)
			StringCat(CmdLine, TEXT(" "));

	}
	if(argc>2)
		StringCat(CmdLine, TEXT("\""));

	//StringCat(CmdLine, TEXT("\""));

	 // Spawn process 
	//MessageBox(NULL,AppName,TEXT("pp"),MB_OK);
	//MessageBox(NULL,CmdLine,TEXT("pp"),MB_OK);
	 f = CreateProcess(AppName, CmdLine, NULL, NULL, TRUE, 
		 0, NULL, NULL, &si, &pi);

	 if (f) {
		 WaitForInputIdle(pi.hProcess, INFINITE);
 
		 CloseHandle(pi.hThread);
		 CloseHandle(pi.hProcess);
	 }

	 return 0;
 }
 
