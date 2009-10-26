#include "CommonIncludes.h"


typedef void *COMBO;

COMBO NewCombo(LPTSTR Name, INT CtrlID, LPTSTR DefaultEntry);
VOID FreeCombo(COMBO ptr);
VOID ReadComboFromRegistry(COMBO ptr);
VOID WriteCombo2Registry(COMBO ptr);
VOID ComboAdd(COMBO ptr, LPTSTR Item);
VOID ComboInitDialog(HWND hDlg,COMBO ptr);
VOID ComboGetDlgText(HWND hDlg, COMBO ptr, LPTSTR str, INT strMaxLen);
VOID ComboSetDlgText(HWND hDlg, COMBO ptr, LPTSTR str);
LPTSTR ComboGetValue(COMBO ptr);
VOID ComboUpdate(HWND hDlg, COMBO ptr);
BOOL ComboHasChanged(HWND hDlg, COMBO ptr);