/******************************************************************************
	WinGHCi, a GUI for GHCi

	Registry.c: functions to read and write to Windows registry

	Original code taken from Winhugs (http://haskell.org/hugs)

	With minor modifications by Pepe Gallardo, 2009-March
*******************************************************************************/

#include "CommonIncludes.h"
#include "General.h"
#include "Strings.h"
#include "WndMain.h"


LPTSTR WinGHCiRegRoot = TEXT("SOFTWARE\\Haskell\\WinGHCi ") WinGHCi_VERSION_STRING TEXT("\\");

static BOOL createKey(hKey, regPath, phRootKey, samDesired)
HKEY    hKey;
LPTSTR  regPath;
PHKEY   phRootKey; 
REGSAM  samDesired; {
    DWORD  dwDisp;
    return RegCreateKeyEx(hKey, regPath,
			  0, TEXT(""), REG_OPTION_NON_VOLATILE,
			  samDesired, NULL, phRootKey, &dwDisp) 
	   == ERROR_SUCCESS;
}

static BOOL queryValue(hKey, regPath, var, type, buf, bufSize)
HKEY    hKey;
LPTSTR  regPath;
LPTSTR  var;
LPDWORD type;
LPBYTE  buf;
DWORD   bufSize; {
    HKEY hRootKey;

    if (!createKey(hKey, regPath, &hRootKey, KEY_READ)) {
	return FALSE;
    } else {
	LONG res = RegQueryValueEx(hRootKey, var, NULL, type, buf, &bufSize);
	RegCloseKey(hRootKey);
	return (res == ERROR_SUCCESS);
    }
}

/* Specialised version of queryValue(), which doesn't require
 * you to guess the length of a REG_SZ value. Allocates a big
 * enough buffer (using malloc()) to hold the key's value, which
 * is then returned to the callee (along with the resp. to free the
 * buffer.)
 */
static BOOL queryString(hKey, regPath, var, pString)
HKEY    hKey;
LPTSTR  regPath;
LPTSTR  var;
LPTSTR* pString; {
    HKEY  hRootKey;
    LONG  rc;
    DWORD bufSize;
    DWORD valType = REG_SZ;
    BOOL  res = FALSE;

    if (!createKey(hKey, regPath, &hRootKey, KEY_READ)) {
	return FALSE;
    } else {
        /* Determine the length of the entry */
	rc = RegQueryValueEx(hRootKey, var, NULL, &valType, NULL, &bufSize);
	
	if (rc == ERROR_SUCCESS && valType == REG_SZ) {
	  /* Got the length, now allocate the buffer and retrieve the LPTSTR. */

	  bufSize = sizeof(TCHAR) * (bufSize + 1);
	  if ((*pString = (LPTSTR)malloc(bufSize)) != NULL) {
		  
		(*pString)[0] = TEXT('\0');
	    rc = RegQueryValueEx(hRootKey, var, NULL, &valType, (LPBYTE)*pString, &bufSize);
	    res = (rc == ERROR_SUCCESS);
	  }
	}

	RegCloseKey(hRootKey);
	return (res);
    }
}

static BOOL setValue(hKey, regPath, var, type, buf, bufSize)
HKEY   hKey;
LPTSTR regPath;
LPTSTR var;
DWORD  type;
LPBYTE buf;
DWORD  bufSize; {
    HKEY hRootKey;

    if (!createKey(hKey, regPath, &hRootKey, KEY_WRITE)) {
	return FALSE;
    } else {
	LONG res = RegSetValueEx(hRootKey, var, 0, type, buf, bufSize);
	RegCloseKey(hRootKey);
	return (res == ERROR_SUCCESS);
    }
}

LPTSTR readRegString(HKEY key, LPTSTR regPath, LPTSTR var, LPTSTR def) /* read LPTSTR from registry */
{
    LPTSTR StringVal;
    
    if (queryString(key, regPath, var, &StringVal)) {
        /* The callee is responsible for freeing the returned LPTSTR */
		return (LPTSTR)StringVal;
    } else {
        /* Create a *copy* of the default LPTSTR, so that it can be freed
	   without worry. */
      if ((StringVal = malloc(sizeof(TCHAR) * (StringLen(def) + 1))) == NULL) {
		return NULL;
      } else {
		StringCpy(StringVal, def);
		return (LPTSTR)StringVal;
      }
    }
}

BOOL writeRegString(var,val)      /* write LPTSTR to registry */
LPTSTR var;                        
LPTSTR val; {
    LPTSTR realVal = ( (NULL == val) ? TEXT("") : val);

    return setValue(HKEY_CURRENT_USER, WinGHCiRegRoot, var,
		    REG_SZ, (LPBYTE)realVal, sizeof(TCHAR)*lstrlen(realVal)+1);
}


BOOL writeRegInt(var,val)         /* write LPTSTR to registry */
LPTSTR var;                        
INT    val; {
    return setValue(HKEY_CURRENT_USER, WinGHCiRegRoot, var, 
		    REG_DWORD, (LPBYTE)&val, sizeof(val));
}

INT readRegInt(var, def)          /* read INT from registry */
LPTSTR var;
INT    def; {
    DWORD buf;
    DWORD type;

    if (queryValue(HKEY_CURRENT_USER, WinGHCiRegRoot, var, &type, 
		   (LPBYTE)&buf, sizeof(buf))
	&& type == REG_DWORD) {
	return (INT)buf;
    } else if (queryValue(HKEY_LOCAL_MACHINE, WinGHCiRegRoot, var, &type, 
			  (LPBYTE)&buf, sizeof(buf))
	       && type == REG_DWORD) {
	return (INT)buf;
    } else {
	return def;
    }
}

LPTSTR readRegStrDup(LPTSTR Key, LPTSTR Default)
{
    return readRegString(HKEY_CURRENT_USER, WinGHCiRegRoot, Key, Default);
}

VOID readRegStr(LPTSTR Key, LPTSTR Default, LPTSTR Buffer)
{
    LPTSTR res = readRegStrDup(Key, Default);
    StringCpy(Buffer, res);
    free(res);
}

#define FONT_NAME	TEXT("FontName")
#define FONT_SIZE	TEXT("FontSize")
#define FONT_WEIGHT TEXT("FontWeight")
#define FONT_ITALIC TEXT("FontItalic")

VOID RegistryReadFont(CHARFORMAT* cf)
{
    cf->cbSize = sizeof(CHARFORMAT);
    cf->dwMask = CFM_BOLD | CFM_FACE | CFM_ITALIC | CFM_SIZE;
    cf->dwEffects = 0;

    readRegStr(FONT_NAME, TEXT("Consolas"), cf->szFaceName);
    cf->yHeight = readRegInt(FONT_SIZE, PointToTwip(11));
    if (readRegInt(FONT_WEIGHT, 0)) cf->dwEffects |= CFE_BOLD;
    if (readRegInt(FONT_ITALIC, 0)) cf->dwEffects |= CFE_ITALIC;
}

VOID RegistryWriteFont(CHARFORMAT* cf)
{
    writeRegString(FONT_NAME, cf->szFaceName);
    writeRegInt(FONT_SIZE, cf->yHeight);
    writeRegInt(FONT_WEIGHT, (cf->dwEffects & CFE_BOLD ? 1 : 0));
    writeRegInt(FONT_ITALIC, (cf->dwEffects & CFE_ITALIC ? 1 : 0));
}

#define WINDOW_MAXIMIZED TEXT("WindowMaximized")
#define WINDOW_LEFT		 TEXT("WindowLeft")
#define WINDOW_TOP		 TEXT("WindowTop")
#define WINDOW_WIDTH	 TEXT("WindowWidth")
#define WINDOW_HEIGHT	 TEXT("WindowHeight")


VOID RegistryReadWindowPos(HWND hWnd)
{
    INT x, y, cx, cy;
    INT Maximized = readRegInt(WINDOW_MAXIMIZED, 1);

	
    if (Maximized) {
	ShowWindow(hWnd, SW_MAXIMIZE);
	return;
    }

	x = readRegInt(WINDOW_LEFT, -1);
    y = readRegInt(WINDOW_TOP, -1);
    cx = readRegInt(WINDOW_WIDTH, -1);
    cy = readRegInt(WINDOW_HEIGHT, -1);

    if (x == -1) return;

	MainSize(hWnd,cx,cy);
    MoveWindow( hWnd, x, y, cx, cy, TRUE);
	
}

VOID RegistryWriteWindowPos(HWND hWnd)
{
    RECT rc;
    INT Maximized;

    // The user has closed while the app is minimized
    // The current values are wrong, who knows what the correct
    // ones are, so just be safe and store nothing
    if (IsIconic(hWnd))
	return;

    Maximized = (IsZoomed(hWnd) ? 1 : 0);
    writeRegInt(WINDOW_MAXIMIZED, Maximized);

    if (Maximized)
	return;

    GetWindowRect(hWnd, &rc);
    writeRegInt(WINDOW_LEFT, rc.left);
    writeRegInt(WINDOW_TOP, rc.top);
    writeRegInt(WINDOW_WIDTH, rc.right - rc.left);
    writeRegInt(WINDOW_HEIGHT, rc.bottom - rc.top);
}

