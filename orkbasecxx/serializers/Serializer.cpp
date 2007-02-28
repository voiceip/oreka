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

#define _WINSOCKAPI_		// prevents the inclusion of winsock.h

#include "ace/OS_NS_time.h"
#include "Object.h"
#include "Serializer.h"
#include "Utils.h"

using namespace XERCES_CPP_NAMESPACE;

Serializer::Serializer(Object* object)
{
	m_object = object;
}

void Serializer::SetObject(Object* object)
{
	m_object = object;
}

void Serializer::DeSerialize()
{
	m_deSerialize = true;		// Set DeSerialize mode
	m_object->Define(this);
	m_object->Validate();
}


void Serializer::IntValue(const char* key, int& value, bool required)
{
	if (m_deSerialize == true)
	{
		GetInt(key, value, required);
	}
	else
	{
		AddInt(key, value);
	}
}

void Serializer::DoubleValue(const char* key, double& value, bool required)
{
	if (m_deSerialize == true)
	{
		GetDouble(key, value, required);
	}
	else
	{
		AddDouble(key, value);
	}
}

void Serializer::BoolValue(const char* key, bool& value, bool required)
{
	if (m_deSerialize == true)
	{
		GetBool(key, value, required);
	}
	else
	{
		AddBool(key, value);
	}
}

void Serializer::StringValue(const char* key, CStdString& value, bool required)
{
	if (m_deSerialize == true)
	{
		GetString(key, value, required);
	}
	else
	{
		AddString(key, value);
	}
}

void Serializer::EnumValue(const char* key, int& value, StringToEnumFunction toEnum, EnumToStringFunction toString, bool required)
{
	if (m_deSerialize == true)
	{
		GetEnum(key, value, toEnum, required);
	}
	else
	{
		AddEnum(key, value, toString);
	}
}

void Serializer::CsvValue(const char* key, std::list<CStdString>& value, bool required)
{
	if (m_deSerialize == true)
	{
		GetCsv(key, value, required);
	}
	else
	{
		AddCsv(key, value);
	}
}

void Serializer::DateValue(const char* key, time_t& value, bool required)
{
	if (m_deSerialize == true)
	{
		GetDate(key, value, required);
	}
	else
	{
		AddDate(key, value);
	}
}


//=====================================
void Serializer::AddInt(const char* key, int value)
{
	CStdString valueString = IntToString(value);
	AddString(key, valueString);
}

void Serializer::AddDouble(const char* key, double value)
{
	CStdString valueString = DoubleToString(value);
	AddString(key, valueString);
}

void Serializer::AddBool(const char* key, bool value)
{
	if(value)
	{
		CStdString trueString("true");
		AddString(key, trueString);
	}
	else
	{
		CStdString falseString("false");
		AddString(key, falseString);
	}
}

void Serializer::AddEnum(const char* key, int value, EnumToStringFunction function)
{
	if(!function)
	{
		throw(CStdString("Serializer: wrong enumerated type conversion function for parameter:") + key);
	}
	else
	{
		CStdString valueString = function(value);
		AddString(key, valueString);
	}
}

void Serializer::AddCsv(const char* key,  std::list<CStdString>& value)
{
	CStdString csvString;
	bool first = true;
	for(std::list<CStdString>::iterator it = value.begin(); it!=value.end(); it++)
	{
		if(!first)
		{
			csvString += ",";
		}
		first = false;
		csvString += *it;
	}
	AddString(key, csvString);
}

void Serializer::AddDate(const char* key, time_t value)
{
	struct tm date;
	ACE_OS::localtime_r(&value ,&date);
	int month = date.tm_mon + 1;				// january=0, decembre=11
	int year = date.tm_year + 1900;
	CStdString dateString;
	dateString.Format("%.4d-%.2d-%.2d_%.2d-%.2d-%.2d", year, month, date.tm_mday, date.tm_hour, date.tm_min, date.tm_sec);
	AddString(key, dateString);
}


//====================================================================
void Serializer::GetInt(const char* key, int&value, bool required)
{
	CStdString stringValue;
	GetString(key, stringValue, required);
	if(!stringValue.IsEmpty())
	{
		value = StringToInt(stringValue);
	}
}

void Serializer::GetDouble(const char* key, double&value, bool required)
{
	CStdString stringValue;
	GetString(key, stringValue, required);
	if(!stringValue.IsEmpty())
	{
		value = StringToDouble(stringValue);
	}
}

void Serializer::GetBool(const char* key, bool&value, bool required)
{
	CStdString stringValue;
	GetString(key, stringValue, required);
	stringValue.ToLower();
	if(	stringValue == "true" || stringValue == "yes" || stringValue == "1" )
	{
		value = true;
	}
	else if( stringValue == "false" || stringValue == "no" || stringValue == "0" )
	{
		value = false;
	}
	else if( stringValue.IsEmpty() )
	{
		; // do not touch default value
	}
	else
	{
		throw(CStdString("Serializer: Invalid boolean value:") + stringValue + " for parameter:" + key);
	}
}

void Serializer::GetEnum(const char* key, int& value, StringToEnumFunction function, bool required)
{
	if(!function)
	{
		throw(CStdString("Serializer: missing enumerated type conversion function for parameter:") + key);
	}
	else
	{
		CStdString enumStringValue;
		GetString(key, enumStringValue, required);
		if (!enumStringValue.IsEmpty())
		{
			value = function(enumStringValue);
		}
	}

}

void Serializer::GetCsv(const char* key,  std::list<CStdString>& value, bool required)
{
	CStdString stringValue;
	stringValue.Trim();
	CStdString element;
	bool first = true;
	GetString(key, stringValue, required);
	for(int i=0; i<stringValue.length(); i++)
	{
		TCHAR c = stringValue[i];
		if(c == ',')
		{
			if(first)
			{
				first = false;	// only erase default value if something found
				value.clear();
			}
			element.Trim();
			value.push_back(element);
			element.Empty();
		}
		else
		{
			element += c;
		}
	}
	if (!element.IsEmpty())
	{
		if(first)
		{
			first = false;	// only erase default value if something found
			value.clear();
		}
		element.Trim();
		value.push_back(element);
	}
}

void Serializer::GetDate(const char* key, time_t& value, bool required)
{
	throw (CStdString("DeSerializer: GetDate: not implemented yet"));

	CStdString stringValue;
	GetString(key, stringValue, required);
	if(!stringValue.IsEmpty())
	{
		//value = ;
	}
}

//==========================================================

void KeyValueSerializer::GetString(const char* key, CStdString& value, bool required)
{
	std::map<CStdString, CStdString>::iterator it = m_map.find(CStdString(key));
	if (it != m_map.end())
	{
		value = it->second;
	}
	else if (required == true)
	{
		throw CStdString(CStdString("Serializer::GetString: required parameter missing:") + key);
	}
}

void KeyValueSerializer::ObjectValue(const char* key, Object& value, bool required)
{
	throw CStdString(CStdString("KeyValueSerializer::ObjectValue: Nested objects not allowed for key-value serializers"));
}

void KeyValueSerializer::ListValue(const char* key, std::list<ObjectRef>& value, Object& model, bool required )
{
	throw CStdString(CStdString("KeyValueSerializer::ListValue: Nested objects not allowed for key-value serializers"));
}

