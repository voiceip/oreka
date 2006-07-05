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

#include "serializers/SingleLineSerializer.h"
#include "serializers/DomSerializer.h"
#include "serializers/UrlSerializer.h"
#include "Object.h"


CStdString Object::SerializeSingleLine()
{
	SingleLineSerializer* serializer = new SingleLineSerializer(this);
	m_serializer.reset(serializer);
	return serializer->Serialize();
}

void Object::DeSerializeSingleLine(CStdString& input)
{
	SingleLineSerializer* serializer = new SingleLineSerializer(this);
	m_serializer.reset(serializer);
	serializer->DeSerialize(input);
}

void Object::SerializeDom(XERCES_CPP_NAMESPACE::DOMDocument* doc)
{

	DomSerializer* serializer = new DomSerializer(this);
	m_serializer.reset(serializer);
	serializer->Serialize(doc);
}

void Object::DeSerializeDom(DOMNode* doc)
{
	DomSerializer* serializer = new DomSerializer(this);
	m_serializer.reset(serializer);
	serializer->DeSerialize(doc);
}

CStdString Object::SerializeUrl()
{
	UrlSerializer* serializer = new UrlSerializer(this);
	m_serializer.reset(serializer);
	return serializer->Serialize();
}

void Object::DeSerializeUrl(CStdString& input)
{
	UrlSerializer* serializer = new UrlSerializer(this);
	m_serializer.reset(serializer);
	serializer->DeSerialize(input);
}

SerializerRef Object::GetSerializer()
{
	return m_serializer;
}
