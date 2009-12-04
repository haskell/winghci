#include "Combo.h"
#include "CommonIncludes.h"

BOOL ShowOptsDialog(VOID);


extern COMBO GHCi_Combo_Startup;
extern COMBO GHCi_Combo_Editor;
extern COMBO GHCi_Combo_Prompt;


extern BOOL GHCi_Flag_RevertCAFs, GHCi_Flag_PrintStats, GHCi_Flag_PrintTypes;

VOID InitOptions(VOID);
VOID FinalizeOptions(VOID);

VOID ReadOptionsFromRegistry(VOID);
VOID WriteOptions2Registry(VOID);

VOID SendOptions2GHCi(VOID);

VOID MakeGHCiPromptCommand(LPTSTR GHCi_Prompt, LPTSTR Command, BOOL AddMarkers);
VOID MakeGHCiExpandedEditorCommand(LPTSTR Editor, LPTSTR Command);
