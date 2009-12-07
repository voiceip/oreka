/*
 * Oreka -- A media capture and retrieval platform
 * 
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License.
 * Please refer to http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "ace/OS_NS_ctype.h"
#include "Utils.h"
#include "UrlSerializer.h"


static char url_ok_chars[] = "-.0123456789@ABCDEFGHIJKLMNOPQRSTUVWXYZ_abcdefghijklmnopqrstuvwxyz";
static char hex_digits[17] = "0123456789ABCDEF";


void UrlSerializer::AddString(const char* key, CStdString& value)
{
	CStdString pair;
	CStdString escapedValue;
	EscapeUrl(value, escapedValue);
	pair.Format("%s=%s&", key, (PCSTR)escapedValue);
	m_output += pair;
}

CStdString UrlSerializer::Serialize()
{
	m_deSerialize = false;		// Set Serialize mode
	m_object->Define(this);
	return m_output;
}

typedef enum {UrlStartState, UrlKeyState, UrlValueState, UrlErrorState} UrlState;

void UrlSerializer::DeSerialize(CStdString& input)
{
	// read string and extract values into map
	UrlState state = UrlStartState;
	CStdString key;
	CStdString value;
	CStdString errorDescription;

	input.Trim();

	for(unsigned int i=0; i<input.length() && state!= UrlErrorState; i++)
	{
		TCHAR character = input[i];

		switch(state)
		{
		case UrlStartState:
			if(character == '&')
			{
				;	// ignore ampersands
			}
			else if ( ACE_OS::ace_isalnum(character) )
			{
				state = UrlKeyState;
				key = character;
			}
			else
			{
				state = UrlErrorState;
				errorDescription = "Cannot find key start, keys must be alphanum";
			}
			break;
		case UrlKeyState:
			if( ACE_OS::ace_isalnum(character) )
			{
				key += character;
			}
			else if (character == '=')
			{
				state = UrlValueState;
				value.Empty();
			}
			else
			{
				state = UrlErrorState;
				errorDescription = "Invalid key character, keys must be alphanum";
			}
			break;
		case UrlValueState:
			if( character == '=')
			{
				state = UrlErrorState;
				errorDescription = "Value followed by = sign, value should always be followed by space sign";				
			}
			else if (character == '&')
			{
				state = UrlStartState;
			}
			else
			{
				value += character;
			}
			break;
		default:
			state = UrlErrorState;
			errorDescription = "Non-existing state";
		}	// switch(state)

		if ( (state == UrlStartState) || (i == (input.length()-1)) )
		{
			if (!key.IsEmpty())
			{
				// Url unescape
				CStdString unescapedValue;
				UnEscapeUrl(value, unescapedValue);

				// Add pair to key-value map
				m_map.insert(std::make_pair(key, unescapedValue));
				key.Empty();
				value.Empty();
			}
		}
	}	// for(int i=0; i<input.length() && state!= UrlErrorState; i++)

	Serializer::DeSerialize();
}

void UrlSerializer::EscapeUrl(CStdString& in, CStdString& out)
{
	// Translates all the characters that are not in url_ok_chars string into %xx sequences
	// %xx specifies the character ascii code in hexadecimal
	out = "";
	for (unsigned int i = 0 ; i<in.size() ; i++)
	{
		if (in.GetAt(i) == ' ')
			out += '+';
		else if (strchr(url_ok_chars, in.GetAt(i)))
			out += in.GetAt(i);
		else
		{
			out += '%';
			out += hex_digits[((unsigned char) in.GetAt(i)) >> 4];
			out += hex_digits[in.GetAt(i) & 0x0F];
		}
	}
}

void UrlSerializer::UnEscapeUrl(CStdString& in, CStdString& out)
{
	// Translates all %xx escaped sequences to corresponding ascii characters
	out = "";
	char pchHex[3];
	for (unsigned int i = 0; i<in.size(); i++)
	{
		switch (in.GetAt(i))
		{
			case '+':
				out += ' ';
				break;
			case '%':
				if (in.GetAt(++i) == '%')
				{
					out += '%';
					break;
				}
				pchHex[0] = in.GetAt(i);
				pchHex[1] = in.GetAt(++i);
				pchHex[2] = 0;
				out += (char)strtol(pchHex, NULL, 16);
				break;
			default:
				out += in.GetAt(i);
		}
	}
}

CStdString UrlSerializer::FindClass(CStdString& input)
{
	CStdString result;
	int equalsPostion = input.Find('=');
	if (equalsPostion != -1)
	{
		int ampersandPostion = input.Find('&');
		if (ampersandPostion > equalsPostion)
		{
			// there is at least one parameter
			result = input.Mid(equalsPostion+1, ampersandPostion-equalsPostion-1);
		}
		else
		{
			// there is no parameter
			result = input.Mid(equalsPostion+1, input.GetLength() - equalsPostion - 1);
		}
	}
	result.ToLower();
	return result;
}
