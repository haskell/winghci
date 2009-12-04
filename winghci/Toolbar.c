/******************************************************************************
	WinGHCi, a GUI for GHCi

	Toolbar.c: WinGHCi toolbar

	Original code taken from Winhugs (http://haskell.org/hugs)

	With minor modifications by Pepe Gallardo, 2009-March
*******************************************************************************/

#include "CommonIncludes.h"
#include "resource.h"
#include "WinGHCi.h"

INT Buttons[] = {
    // -1 is a separator, 0 is the end
    ID_FILE_LOAD, -1,
    ID_CUT, ID_COPY, ID_PASTE, -1,
    ID_RUN, ID_STOP, ID_COMPILE, ID_GOEDIT, -1,
	ID_SETOPTIONS, ID_TOOLS_CONFIGURE, -1,
    ID_HELPCONTENTS,
    0
};

VOID CreateToolbar(VOID)
{
    INT i;
    INT AnyButtons = 0, RealButtons = 0;
    TBBUTTON* TbButtons;
    HIMAGELIST hImgList;
    HBITMAP hBmp;
	BITMAP bm;

    for (AnyButtons = 0; Buttons[AnyButtons] != 0; AnyButtons++)
	; // no code required

    TbButtons = malloc(sizeof(TBBUTTON) * AnyButtons);
    for (i = 0; i < AnyButtons; i++) {
	if (Buttons[i] == -1) {
	    TbButtons[i].iBitmap = 0;
	    TbButtons[i].fsStyle = BTNS_SEP;
	    TbButtons[i].idCommand = 0;
	} else {
	    TbButtons[i].iBitmap = RealButtons;
	    RealButtons++;
	    TbButtons[i].idCommand = Buttons[i];
	    TbButtons[i].fsStyle = TBSTYLE_BUTTON;
	}
	TbButtons[i].fsState = TBSTATE_ENABLED;
	TbButtons[i].dwData = (DWORD_PTR) NULL;
	TbButtons[i].iString = (INT_PTR) NULL;
    }

    hWndToolbar = CreateWindowEx(
	0,
	TOOLBARCLASSNAME, NULL,
	TBSTYLE_TOOLTIPS | WS_CHILD | WS_VISIBLE | CCS_NODIVIDER | TBSTYLE_FLAT,
	0, 0, 0, 0,
	hWndMain, (HMENU) IDC_Toolbar, hThisInstance, NULL);

	// create the image list
	hBmp = LoadBitmap(hThisInstance, MAKEINTRESOURCE(BMP_NORMALTOOLBAR));
	GetObject(hBmp, sizeof(BITMAP), (LPTSTR)&bm);

	hImgList = ImageList_Create(bm.bmHeight, bm.bmHeight, ILC_COLOR24 | ILC_MASK, RealButtons, RealButtons);
    
    ImageList_AddMasked(hImgList, hBmp, RGB(255,0,255));
    DeleteObject(hBmp);
    SendMessage(hWndToolbar, TB_SETIMAGELIST, 0, (LPARAM) hImgList);

	// create the disabled image list
	hBmp = LoadBitmap(hThisInstance, MAKEINTRESOURCE(BMP_DISABLEDTOOLBAR));
	GetObject(hBmp, sizeof(BITMAP), (LPTSTR)&bm);
	

	hImgList = ImageList_Create(bm.bmHeight, bm.bmHeight, ILC_COLOR8 | ILC_MASK, RealButtons, RealButtons);
    
    ImageList_AddMasked(hImgList, hBmp, RGB(255,0,255));
    DeleteObject(hBmp);
    SendMessage(hWndToolbar, TB_SETDISABLEDIMAGELIST, 0, (LPARAM) hImgList);

    // setup the toolbar properties
 	SendMessage(hWndToolbar, TB_AUTOSIZE, 0, 0); 
    SendMessage(hWndToolbar, TB_ADDBUTTONS, AnyButtons, (LPARAM) TbButtons);
    free(TbButtons);
}
