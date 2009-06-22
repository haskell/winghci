#include "CommonIncludes.h"


VOID AddHistory(LPCTSTR Item);
LPCTSTR GoToHistory(INT NewPos);
LPCTSTR GoToRelativeHistory(INT delta);
VOID InitHistory();
VOID WriteHistory2Registry();
INT ReadHistoryFromRegistry();


INT FindFirstHistory(LPTSTR What);
INT FindNextHistory(VOID);
