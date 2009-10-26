#include "CommonIncludes.h"

LPTSTR MemSearchString (LPCTSTR wcs1, INT numChars, LPCTSTR wcs2);
VOID MemReplaceChars (LPCTSTR wcs1, INT numChars, TCHAR ch1, TCHAR ch2);
LPTSTR StringIsPreffix(LPCTSTR preffix, LPCTSTR str);
VOID StringReplaceAllOccurrences(LPTSTR Str, LPTSTR ToReplace, LPTSTR Replacement, LPTSTR Result);

#define StringCat(xs,ys)			_tcscat((xs),(ys))
#define StringChr(xs,x)				_tcschr((xs),(x))
#define StringCmp(xs,ys)			_tcscmp((xs),(ys))
#define StringCmpN(xs,ys,n)			_tcsnccmp((xs),(ys),(n)) 
#define StringCmpIgnoreCase(xs,ys)  _tcsicmp((xs),(ys))
#define StringIsEmpty(xs)			((xs)[0]==TEXT('\0'))
#define StringRChr(xs,x)			_tcsrchr((xs),(x))
#define StringCpy(xs,ys)			_tcscpy((xs),(ys))
#define StringDup(xs)				_tcsdup((xs))
#define StringLen(xs)				_tcsclen((xs))
#define StringSearchString(xs,ys)	_tcsstr((xs),(ys))
#define StringToInt(xs)				_tstoi((xs))
#define StringToUpper(xs)			_tcsupr((xs))
