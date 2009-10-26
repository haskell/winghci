#include "CommonIncludes.h"

VOID ReadMruFilesFromRegistry();
VOID WriteMruFiles2Registry();

VOID AddMruFile(LPCTSTR file);
LPTSTR GetMruFile(INT i);
VOID InitMruFiles();
