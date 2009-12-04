/******************************************************************************
	WinGHCi, a GUI for GHCi

	Strings.c: Some String utilities

	Pepe Gallardo, 2009-March
*******************************************************************************/

#include "CommonIncludes.h"
#include "Strings.h"

// all strings are null terminated. Assumes enough space in Result
VOID StringReplaceAllOccurrences(LPTSTR Str, LPTSTR ToReplace, LPTSTR Replacement, LPTSTR Result)
{
	LPTSTR p;
	TCHAR svChar;
	INT ToReplaceLen = StringLen(ToReplace);
	INT ReplacementLen = StringLen(Replacement);

	Result[0] = TEXT('\0');	
	for(;;) {
		p = StringSearchString(Str,ToReplace);
		if (p==NULL) {
			StringCat(Result,Str);
			break;
		}
		svChar = *p;
		*p=TEXT('\0');
		StringCat(Result,Str);
		*p=svChar;
		StringCat(Result,Replacement);
		Str = (p+ToReplaceLen);

	}
	
}

// searchs in a non-null terminated string a null terminated string
LPTSTR MemSearchString (
        LPCTSTR wcs1, // non-null terminated string
		INT numChars, // its length
        LPCTSTR wcs2  // null terminated string to search
        )
{
        TCHAR *cp = (TCHAR *) wcs1;
        TCHAR *s1, *s2;
		INT numCharsAux;

        if ( !*wcs2)
            return (TCHAR *)wcs1;

        while (numChars>0)
        {
                s1 = cp;
				numCharsAux = numChars;
                s2 = (TCHAR *) wcs2;

                while ( (numCharsAux>0) && *s2 && !(*s1-*s2) ) 
                        s1++, numCharsAux--, s2++;

                if (!*s2)
                        return(cp);

                cp++;
				numChars--;
        }

        return(NULL);
}

// replaces all occurrences of a char in a non-null terminated string with another char
VOID MemReplaceChars (
        LPCTSTR wcs1, // non-null terminated string
		INT numChars, // its length
        TCHAR ch1,  // char to look for
		TCHAR ch2   // char to replace with
        )
{
	TCHAR *cp = (TCHAR *) wcs1;
	while(numChars>0) {
		if(*cp == ch1)
			*cp = ch2;
		cp++;
		numChars--;
	}


}


LPTSTR StringIsPreffix(LPCTSTR preffix, LPCTSTR str)
{

	// skip initial white spaces
	LPTSTR pStr = (LPTSTR)str,
		   pPref = (LPTSTR)preffix;

	while(*pStr==TEXT(' ')) pStr++;

	while(*pPref && (*pPref==*pStr)) {
		pPref++;
		pStr++;
	}

	if(!*pPref)
		return pStr;
	else
		return NULL;

}