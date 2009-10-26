#include "CommonIncludes.h"

#define CP_UNICODE 1200

BOOL UnicodeToUtf8(const WCHAR *WCharsIn, BYTE *BytesOut, INT maxBytes, INT *bytesOut);
BOOL UnicodeToLocalCodePage(const WCHAR *WCharsIn, BYTE *BytesOut, INT maxBytes, INT *bytesOut);
INT LocalCodePageToUnicode(const BYTE *strIn, INT lenIn, WCHAR *strOut, INT maxWChars, INT *lenOut);