#include "CommonIncludes.h"

BOOL ShowToolsDialog(VOID);
VOID ReadToolsFromRegistry(VOID);
VOID WriteTools2Registry(VOID);
LPTSTR ToolGetHelp(INT pos);
VOID ToolFireCommand(INT pos);
VOID InitTools(VOID);
VOID FinalizeTools(VOID);