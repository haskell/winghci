/******************************************************************************
	WinGhci, a GUI for GHCI

	History.c: line input history

	Pepe Gallardo, 2009-March
*******************************************************************************/

#include "CommonIncludes.h"
#include "Registry.h"
#include "Strings.h"

#define MAX_HISTORY 100

LPTSTR History[MAX_HISTORY];
INT nextSlot, // 0 <= nextSlot < MAX_HISTORY
    historySize, // 0 <= cursor <= MAX_HISTORY
    cursor; 

#define lastItemCursor	(historySize-1)
#define AdvanceNextSlot (nextSlot = (nextSlot + 1) % MAX_HISTORY)



LPCTSTR BlankLine = TEXT("");

#define HISTORY_FMT TEXT("History%02d")


INT ReadHistoryFromRegistry(VOID);

VOID InitHistory(VOID)
{
	INT i;
	for(i=0; i<MAX_HISTORY; i++)
		History[i] = NULL;
	historySize = ReadHistoryFromRegistry();
	nextSlot = cursor = lastItemCursor;
	AdvanceNextSlot;

}



// pre:  0 <= Pos < historySize
INT GetSlot(INT Pos) 
{
	INT realPos = (nextSlot-historySize)+Pos;
	if(realPos<0)
		realPos = MAX_HISTORY+realPos;

	return realPos;
}




VOID AddHistory(LPCTSTR Item)
{
	if ( (StringCmp(Item, BlankLine) == 0) // no input
		||
		((historySize>0) && (StringCmp(Item, History[GetSlot(lastItemCursor)]) == 0)) // repeated input
		) {
		;
	} else if (historySize<MAX_HISTORY) {
		historySize++;
		History[nextSlot] = StringDup(Item);
		AdvanceNextSlot;

	} else {
		free(History[nextSlot]);
		History[nextSlot] = StringDup(Item);
		AdvanceNextSlot;		
	}
	cursor = lastItemCursor;	
}



// pre:  0 <= NewPos < historySize
LPCTSTR GoToHistory(INT NewPos)
{

	// no history
	if(historySize<=0)
		return BlankLine;

	cursor = NewPos;
	return History[GetSlot(cursor)];

}

LPCTSTR GoToRelativeHistory(INT delta)
{
	INT Indx;
	
	// no history
	if(historySize<=0)
		return BlankLine;

	if(delta==-1) {
		Indx = cursor; //return last entry
		cursor--;
		if (cursor<0)
			cursor = historySize + cursor;
		
	} else if (delta==1) {
		cursor++;
		if(cursor>=historySize)
			cursor = 0;
		Indx = cursor;
	}

	return History[GetSlot(Indx)];
}



VOID WriteHistory2Registry(VOID)
{
    INT i;
    TCHAR Buf[1024];

    for (i = 0; i<historySize ; i++) {
		wsprintf(Buf,HISTORY_FMT,i);
		writeRegString(Buf, History[GetSlot(i)]);
    }
}

INT ReadHistoryFromRegistry(VOID)
{
    INT i;
    TCHAR Buf[1024];

	for (i = 0; i<MAX_HISTORY ; i++) {
		LPTSTR Res;
		wsprintf(Buf,HISTORY_FMT,i);
		Res = readRegStrDup(Buf, TEXT(""));
		
		if (StringIsEmpty(Res)) {
			free(Res);
			break;
		}
		else
			History[i] = Res;
	}
	return i;
}


#define MAX_FINDWHAT 2048
TCHAR	FindWhat[MAX_FINDWHAT];
INT		FindNextFrom;


INT FindNextHistory(VOID)
{
	INT i;
	TCHAR HistoryUpper[MAX_FINDWHAT];


	// backwards circular search
	for(i=FindNextFrom; i>=0; i--) {
		StringCpy(HistoryUpper,History[GetSlot(i)]);
		StringToUpper(HistoryUpper);
		if(StringSearchString(HistoryUpper,FindWhat)) {
			FindNextFrom = i > 0 ? i-1 : lastItemCursor;
			return i;
		}
	}

	for(i=lastItemCursor; i>FindNextFrom; i--) {
		StringCpy(HistoryUpper,History[GetSlot(i)]);
		StringToUpper(HistoryUpper);
		if(StringSearchString(HistoryUpper,FindWhat)) {
			FindNextFrom = i > 0 ? i-1 : lastItemCursor;
			return i;
		}
	}

	// not found
	return -1;
}

INT FindFirstHistory(LPTSTR What)
{
	FindNextFrom = lastItemCursor;
	StringCpy(FindWhat,	What);
	StringToUpper(FindWhat);  //ignore case when searching
	return FindNextHistory();
}





