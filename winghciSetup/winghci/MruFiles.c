/******************************************************************************
	WinGhci, a GUI for GHCI

	MruFiles.c: menu list of recently loaded files

    Original code taken from Winhugs (http://haskell.org/hugs)

	With minor modifications by Pepe Gallardo, 2009-March
*******************************************************************************/

#include "CommonIncludes.h"
#include "resource.h"
#include "Registry.h"
#include "Strings.h"
#include "WinGhci.h"

#define MAX_MRU  9

// run over by one, so registry code can see the end
// always MruBuffer[MAX_MRU] == NULL
LPTSTR MruBuffer[MAX_MRU+1] = {0};
INT MenusShown = 1; // the default one


VOID ReadMruFilesFromRegistry();
VOID WriteMruFiles2Registry();


LPTSTR GetMruFile(INT i)
{
    return MruBuffer[i];
}

VOID ShowMruMenu()
{

    INT n, i;
    HMENU hMenu;

	for(i=0; GetMenuItemID(GetSubMenu(GetMenu(hWndMain), i),0)!=ID_FILE_LOAD; i++);
	hMenu = GetSubMenu(GetMenu(hWndMain), i);
 
    //first count the MRU list
    for (n = 0; MruBuffer[n] != NULL; n++)
	; // no code required

    //add enough entries
    for (i = MenusShown; i < n; i++)
		AppendMenu(hMenu, MF_STRING, ID_MRU+i, MruBuffer[i]);
    MenusShown = (n == 0 ? 1 : n);

    //then change them
    for (i = 0; i < n; i++)
		ModifyMenu(hMenu, ID_MRU+i, MF_BYCOMMAND, ID_MRU+i, MruBuffer[i]);
}

VOID InitMruFiles()
{
	int i;
	for(i=0;i<MAX_MRU;i++)
		MruBuffer[i] = NULL;
    ReadMruFilesFromRegistry();
    ShowMruMenu();
}


VOID AddMruFile(LPCTSTR file)
{
    // if its already in the list move it to the top
    // if its not, add it at the top
    INT i;
    BOOL Found;

    // remove from the list if its already there
    Found = FALSE;
    for (i = 0; MruBuffer[i] != NULL; i++) {
	Found = Found || (StringCmpIgnoreCase(MruBuffer[i], file) == 0);
	if (Found)
	    MruBuffer[i] = MruBuffer[i+1]; //rely on trailing NULL
    }

    // if the last entry would die, kill it now
    if (MruBuffer[MAX_MRU-1] != NULL)
	free(MruBuffer[MAX_MRU-1]);

    // shift everything along by one
    for (i = MAX_MRU-1; i > 0; i--)
	MruBuffer[i] = MruBuffer[i-1];

    // and put the new file at the top
    MruBuffer[0] = StringDup(file);

    WriteMruFiles2Registry(MruBuffer);
    ShowMruMenu();
}

#define MRU_STR		TEXT("Mru1")

VOID ReadMruFilesFromRegistry()
{
    INT i;
    TCHAR Buf[20] = MRU_STR;

    for (i = 0; ; i++) {
	LPTSTR Res;
	Buf[3] = i + TEXT('0');
	Res = readRegStrDup(Buf, TEXT(""));

	MruBuffer[i] = Res;
	if (StringIsEmpty(Res)) {
	    MruBuffer[i] = NULL;
	    free(Res);
	    break;
	}
    }
}

VOID WriteMruFiles2Registry()
{
    INT i;
    TCHAR Buf[20] = MRU_STR;

    for (i = 0; MruBuffer[i] != NULL; i++) {
		Buf[3] = i + TEXT('0');
		writeRegString(Buf, MruBuffer[i]);
    }
}
