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

#ifndef __URLSERIALIZER_H__
#define __URLSERIALIZER_H__

#include "messages/Message.h"
#include "serializers/Serializer.h"

/** Serializer that generates and parses URLs from and to objects.
    key-value pairs are separated by ampersands
    keys and values are separated by equal signs
    example: message=doit&what=run
*/
class DLL_IMPORT_EXPORT_ORKBASE UrlSerializer : public KeyValueSerializer
{
public:
	UrlSerializer(Object* object) : KeyValueSerializer(object){};

	void AddString(const char* key, CStdString& value);
	CStdString Serialize();

	void DeSerialize(CStdString& input);

	void EscapeUrl(CStdString& in, CStdString& out);
	void UnEscapeUrl(CStdString& in, CStdString& out);

	static CStdString FindClass(CStdString& input);
private:
	CStdString m_output;
};

#endif

