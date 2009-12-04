/******************************************************************************
	WinGHCi, a GUI for GHCi

	Combo.c: Combo boxes that can be saved to the registry

	Pepe Gallardo, 2009-March
*******************************************************************************/

#include "Combo.h"
#include "CommonIncludes.h"
#include "Registry.h"
#include "Strings.h"

#define MAX_HISTORY		25
#define MAX_BUF_LEN		5*MAX_PATH


typedef struct {
	LPTSTR Name;
	INT CtrlID;
	INT HistoryLen;
	LPTSTR History[MAX_HISTORY];
	LPTSTR DefaulEntry;
} Combo;


COMBO NewCombo(LPTSTR Name, INT CtrlID, LPTSTR DefaultEntry)
{
	Combo *c = malloc(sizeof(Combo));

	c->Name = StringDup(Name);
	c->CtrlID = CtrlID;
	c->HistoryLen = 0;
	c->DefaulEntry = StringDup(DefaultEntry);

	return (COMBO) c;
}

VOID FreeCombo(COMBO ptr)
{
	INT i;
	Combo *c = (Combo*) ptr;

	free(c->Name);
	free(c->DefaulEntry);

	for(i=0;i<c->HistoryLen; i++) {
		free(c->History[i]);
	}
	free(c);

}

VOID ReadComboFromRegistry(COMBO ptr)
{
	INT i;
	TCHAR Buf[1024];
	LPTSTR Res;
	Combo *c = (Combo*) ptr;

	for(i=0;i<MAX_HISTORY; i++) 
		c->History[i] = NULL;

	for(i=0;i<MAX_HISTORY; i++) {
		wsprintf(Buf,TEXT("%s%d"),c->Name,i);
		Res = readRegStrDup(Buf, TEXT(""));
		
		if (StringIsEmpty(Res)) {
			free(Res);
			break;
		}
		else
			c->History[i] = Res;	
	}
	c->HistoryLen = i;

	if(c->HistoryLen==0) {
		c->History[0] = StringDup(c->DefaulEntry);
		(c->HistoryLen)++;
	}

}



VOID WriteCombo2Registry(COMBO ptr)
{
	INT i;
	Combo *c = (Combo*) ptr;
	TCHAR Buf[1024];

	for(i=0;i<c->HistoryLen;i++) {
		wsprintf(Buf,TEXT("%s%d"),c->Name,i);
		writeRegString(Buf, c->History[i]);
	}

}


VOID ComboAdd(COMBO ptr, LPTSTR Item)
{
	INT i;
	Combo *c = (Combo*) ptr;

	if(StringIsEmpty(Item))
		return;

	for(i=0; i<c->HistoryLen; i++)
		if(StringCmp(c->History[i],Item)==0) {
			LPTSTR aux = c->History[c->HistoryLen-1];
			c->History[c->HistoryLen-1] = c->History[i];
			c->History[i] = aux;
			return;
	}


	if(c->HistoryLen<MAX_HISTORY) {
		c->History[c->HistoryLen] = StringDup(Item);
		(c->HistoryLen)++;
	} else {
		free(c->History[0]);
		for(i=0;i<c->HistoryLen-1;i++)
			c->History[i] = c->History[i+1];
		c->History[c->HistoryLen-1] = StringDup(Item);
	}

}

VOID ComboInitDialog(HWND hDlg, COMBO ptr)
{
	INT i;
	Combo *c = (Combo*) ptr;

	HWND hLst = GetDlgItem(hDlg, c->CtrlID);

	SetDlgItemText(hDlg, c->CtrlID, c->History[c->HistoryLen-1]);
	
	for(i=c->HistoryLen-1; i>=0; i--) {
		SendMessage(hLst, CB_ADDSTRING, 0, (LPARAM) c->History[i]);
	}
	
}

VOID ComboGetDlgText(HWND hDlg, COMBO ptr, LPTSTR str, INT strMaxLen)
{
	Combo *c = (Combo*) ptr;

	GetDlgItemText(hDlg, c->CtrlID, str, strMaxLen);
}

VOID ComboSetDlgText(HWND hDlg, COMBO ptr, LPTSTR str)
{
	Combo *c = (Combo*) ptr;

	SetDlgItemText(hDlg, c->CtrlID, str);
}


LPTSTR ComboGetValue(COMBO ptr)
{
	Combo *c = (Combo*) ptr;
	
	return c->History[c->HistoryLen-1];

}



VOID ComboUpdate(HWND hDlg, COMBO ptr)
{
	Combo *c = (Combo*) ptr;
	TCHAR Buf[MAX_BUF_LEN];

	ComboGetDlgText(hDlg, ptr, Buf, MAX_BUF_LEN);
	ComboAdd(ptr,Buf);

}

BOOL ComboHasChanged(HWND hDlg, COMBO ptr)
{
	Combo *c = (Combo*) ptr;
	TCHAR Buf[MAX_BUF_LEN];

	ComboGetDlgText(hDlg, ptr, Buf, MAX_BUF_LEN);
	return (StringCmp(Buf,ComboGetValue(ptr))!=0);

}