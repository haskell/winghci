#include "CommonIncludes.h"

LPTSTR readRegString(HKEY key, LPTSTR regPath, LPTSTR var, LPTSTR def);
VOID readRegStr(LPTSTR Key, LPTSTR Default, LPTSTR Buffer);
BOOL writeRegString(LPTSTR var, LPTSTR val) ;
BOOL writeRegInt(LPTSTR var,INT val);
INT readRegInt(LPTSTR var,INT def);

LPTSTR readRegStrDup(LPTSTR Key, LPTSTR Default);

VOID RegistryReadFont(CHARFORMAT* cf);
VOID RegistryWriteFont(CHARFORMAT* cf);

VOID RegistryReadWindowPos(HWND hWnd);
VOID RegistryWriteWindowPos(HWND hWnd);

