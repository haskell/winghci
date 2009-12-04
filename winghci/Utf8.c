/******************************************************************************
	WinGHCi, a GUI for GHCi

	Utf8.c: Simple Unicode Utf8 and multibyte conversion functions

	Pepe Gallardo, 2009-March
*******************************************************************************/

#include "CommonIncludes.h"


// Converts a null terminated unicode string to current system Windows ANSI code page
BOOL UnicodeToLocalCodePage(const WCHAR *WCharsIn, BYTE *BytesOut, INT maxBytes, INT *bytesOut)
{

	(*bytesOut) = WideCharToMultiByte( CP_ACP, 0, WCharsIn, -1,
										BytesOut, maxBytes, NULL, NULL );

	(*bytesOut)--;

	return TRUE;
}

// Converts a null terminated unicode string to utf8
BOOL UnicodeToUtf8(const WCHAR *WCharsIn, BYTE *BytesOut, INT maxBytes, INT *bytesOut)
{
    WCHAR* w;

    (*bytesOut) = 0;

    for ( w = (WCHAR*) WCharsIn; *w; w++ ) {
		if ( *w < 0x0080 ) {
			if ((*bytesOut)<maxBytes)
				BytesOut[(*bytesOut)++] = (BYTE) *w;
			else
				return FALSE;
		} else if ( *w < 0x0800 ) {
			if (((*bytesOut)+1)<maxBytes) {
				BytesOut[(*bytesOut)++] = 0xc0 | ((*w) >> 6 );
				BytesOut[(*bytesOut)++] = 0x80 | ((*w) & 0x3f);
			} else
				return FALSE;
        }
        else {
			if (((*bytesOut)+2)<maxBytes) {
				BytesOut[(*bytesOut)++] = 0xe0 | ((*w) >> 12);
				BytesOut[(*bytesOut)++] = 0x80 | (((*w) >> 6) & 0x3f);
				BytesOut[(*bytesOut)++] = 0x80 | ((*w) & 0x3f);
			} else
				return FALSE;
        }    
	}

    return TRUE;
}


BOOL readNextUtf8Sequence(const BYTE *val, INT valLen, INT *i, unsigned *unicodeChar)
{
	unsigned short trigger;

	*unicodeChar = 0;
	trigger = val[*i];

	if (trigger >= 0x80)
	{
		int remain = valLen - (*i);
		if ((trigger & 0xE0) == 0xC0)
		{         // 110x xxxx
			if (   (remain > 1)
				&& (val[(*i)+1] & 0xC0) == 0x80)
			{
				(*unicodeChar) = ((val[*i] & 0x1F) << 6)
				    	       | (val[(*i)+1] & 0x3F);
				(*i) += 2;
			}
			else if (remain>1) {
				// this sequence is non valid !!
				(*i) += 1;
				(*unicodeChar) = TEXT('?');
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}

		else if ((trigger & 0xF0) == 0xE0)
		{  // 1110 xxxx
			if (   (remain > 2)
				&& ((val[(*i)+1] & 0xC0) == 0x80)
				&& ((val[(*i)+2] & 0xC0) == 0x80))
			{
				(*unicodeChar) = ((val[*i] & 0x0F) << 12)
					           | ((val[(*i)+1] & 0x3F) << 6)
					           | (val[(*i)+2] & 0x3F);
				(*i) += 3;
			}
			else if  (remain > 2) {
				// this sequence is non valid !!
				(*i) += 1;
				(*unicodeChar) = TEXT('?');
				return TRUE;
			}
		    else  
			{
				return FALSE;
			}
		}


		else {
			// this sequence is non valid !!
			(*i) += 1;
			(*unicodeChar) = TEXT('?');
			return TRUE;
		}
			
	}

	else
	{
		(*i) += 1;
		(*unicodeChar) = trigger;
	}
	return TRUE;
}



// returns -1, if out of space
//         -2, the whole string was converted
//          i, if part of the string was converted (up to character i)

INT LocalCodePageToUnicode(const BYTE *strIn, INT lenIn, WCHAR *strOut, INT maxWChars, INT *lenOut)
{
	INT res = MultiByteToWideChar(CP_ACP, 0, strIn, lenIn, strOut, maxWChars);
	if(res) {
		*lenOut = res;
		return -2;
	} else {
		DWORD err = GetLastError();
		*lenOut = 0;
		if(err==ERROR_INSUFFICIENT_BUFFER)
			return -1;
		else
			return 0;
	}

}

// This one uses utf8

// returns -1, if out of space
//         -2, the whole string was converted
//          i, if part of the string was converted (up to character i)

INT _Utf8ToUnicode(const BYTE *strIn, INT lenIn, WCHAR *strOut, INT maxWChars, INT *lenOut)
{

	INT i = 0;
	unsigned unicodeChar;

	*lenOut = 0;

	while (i < lenIn)
	{
		BOOL ok = readNextUtf8Sequence(strIn, lenIn, &i, &unicodeChar); 

		if (!ok)
			// Incomplete string, only a prefix was converted
			return i;

		if ( (unicodeChar < 0x10000) && ((*lenOut)<maxWChars))
			strOut[(*lenOut)++] = unicodeChar;
		else if ( (unicodeChar < 0x110000) && ( ((*lenOut)+1)<maxWChars))
		{

			unicodeChar -= 0x10000;
			strOut[(*lenOut)++] = ((unicodeChar >> 10) + 0xD800);
			strOut[(*lenOut)++] = ((unicodeChar & 0x3FF) + 0xDC00);
		}

		else
			// out of space
			return -1;
	}
	
	// correct conversion
	return -2;
}


#if 0
char* UnicodeToUtf8_2( const WCHAR* wstr, INT *lenOut )
{
    WCHAR* w;
    
    INT len = 0;
	unsigned char* szOut;
	INT i;

	// calculate length of output string
    for ( w = (WCHAR*) wstr; *w; w++ ) {

        if ( *w < 0x0080 ) len++;
        else if ( *w < 0x0800 ) len += 2;
        else len += 3;
    }
	*lenOut = len;

    szOut = (unsigned char*) malloc(len+1);

    if (szOut == NULL)
        return NULL;

    i = 0;
    for ( w = (WCHAR*) wstr; *w; w++ ) {
        if ( *w < 0x0080 )
            szOut[i++] = (unsigned char) *w;
        else if ( *w < 0x0800 ) {
            szOut[i++] = 0xc0 | (( *w ) >> 6 );
            szOut[i++] = 0x80 | (( *w ) & 0x3f );
        }
        else {
            szOut[i++] = 0xe0 | (( *w ) >> 12 );
            szOut[i++] = 0x80 | (( ( *w ) >> 6 ) & 0x3f );
            szOut[i++] = 0x80 | (( *w ) & 0x3f );
        }    }

    szOut[ i ] = '\0';

    return (char*) szOut;
}

BOOL readNextUtf8Sequence_2(unsigned char *val, INT valLen, INT *i, unsigned *unicodeChar)
{
	unsigned short trigger;

	*unicodeChar = 0;
	trigger = val[*i];

	if (trigger >= 0x80)
	{
		int remain = valLen - (*i);
		if ((trigger & 0xE0) == 0xC0)
		{         // 110x xxxx
			if (   (remain > 1)
				&& (val[(*i)+1] & 0xC0) == 0x80)
			{
				(*unicodeChar) = ((val[*i] & 0x1F) << 6)
				    	       | (val[(*i)+1] & 0x3F);
				(*i) += 2;
			}
			else
			{
				return FALSE;
			}
		}

		else if ((trigger & 0xF0) == 0xE0)
		{  // 1110 xxxx
			if (   (remain > 2)
				&& ((val[(*i)+1] & 0xC0) == 0x80)
				&& ((val[(*i)+2] & 0xC0) == 0x80))
			{
				(*unicodeChar) = ((val[*i] & 0x0F) << 12)
					           | ((val[(*i)+1] & 0x3F) << 6)
					           | (val[(*i)+2] & 0x3F);
				(*i) += 3;
			}
			else
			{
				return FALSE;
			}
		}

		else if ((trigger & 0xF8) == 0xF0)
		{   // 1111 0xxx
			if (   (remain > 3)
				&& ((val[(*i)+1] & 0xC0) == 0x80)
				&& ((val[(*i)+2] & 0xC0) == 0x80)
				&& ((val[(*i)+3] & 0xC0) == 0x80))
			{
				(*unicodeChar) = ((val[*i]   & 0x07) << 18)
					    | ((val[(*i)+1] & 0x3F) << 12)
					    | ((val[(*i)+2] & 0x3F) << 6)
					    | (val[(*i)+3] & 0x3F);
				(*i) += 4;
			}
			else
			{
				return FALSE;
			}
		}

		else if ((trigger & 0xFC) == 0xF8)
		{   // 1111 10xx
			if (   (remain > 4)
				&& ((val[(*i)+1] & 0xC0) == 0x80)
				&& ((val[(*i)+2] & 0xC0) == 0x80)
				&& ((val[(*i)+3] & 0xC0) == 0x80)
				&& ((val[(*i)+4] & 0xC0) == 0x80))
			{
				(*unicodeChar) = ((val[*i]   & 0x03) << 24)
					    | ((val[(*i)+1] & 0x3F) << 18)
					    | ((val[(*i)+2] & 0x3F) << 12)
					    | ((val[(*i)+3] & 0x3F) << 6)
					    | (val[(*i)+4] & 0x3F);
				(*i) += 5;
			}
			else
			{
				return FALSE;
			}
		}

		else if ((trigger & 0xFE) == 0xFC)
		{   // 1111 110x
			if (   (remain > 5)
				&& ((val[(*i)+1] & 0xC0) == 0x80)
				&& ((val[(*i)+2] & 0xC0) == 0x80)
				&& ((val[(*i)+3] & 0xC0) == 0x80)
				&& ((val[(*i)+4] & 0xC0) == 0x80)
				&& ((val[(*i)+5] & 0xC0) == 0x80))
			{
				(*unicodeChar) = ((val[*i]   & 0x01) << 30)
					    | ((val[(*i)+1] & 0x3F) << 24)
					    | ((val[(*i)+2] & 0x3F) << 18)
					    | ((val[(*i)+3] & 0x3F) << 12)
					    | ((val[(*i)+4] & 0x3F) <<  6)
					    | (val[(*i)+5] & 0x3F);
				(*i) += 6;
			}
			else
			{
				return FALSE;
			}
		}
		else {
			// this sequence is non valid !!
			(*i) += 1;
			(*unicodeChar) = TEXT('X');
			return TRUE;
		}
			
	}

	else
	{
		(*i) += 1;
		(*unicodeChar) = trigger;
	}
	return TRUE;
}




#endif