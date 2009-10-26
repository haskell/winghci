#include "Combo.h"
#include "CommonIncludes.h"

BOOL ShowOptsDialog(VOID);


extern COMBO GHCI_Combo_Startup;
extern COMBO GHCI_Combo_Editor;
extern COMBO GHCI_Combo_Prompt;


extern BOOL GHCI_Flag_RevertCAFs, GHCI_Flag_PrintStats, GHCI_Flag_PrintTypes;

VOID InitOptions(VOID);
VOID FinalizeOptions(VOID);

VOID ReadOptionsFromRegistry(VOID);
VOID WriteOptions2Registry(VOID);

VOID SendOptions2GHCI(VOID);

VOID MakeGhciPromptCommand(LPTSTR GHCI_Prompt, LPTSTR Command, BOOL AddMarkers);
VOID MakeGhciExpandedEditorCommand(LPTSTR Editor, LPTSTR Command);
