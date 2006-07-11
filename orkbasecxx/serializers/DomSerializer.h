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

#ifndef __DOMSERIALIZER_H__
#define __DOMSERIALIZER_H__

#include "Serializer.h"
#include "Object.h"

#include "xercesc/dom/DOMNode.hpp"
#include "xercesc/util/XMLString.hpp"

using namespace XERCES_CPP_NAMESPACE;

/** Serializer class for Document Object Model.
	This class allows a nested object to be serialized or deserialized
	to or from a xerces DOM object.
*/
class DLL_IMPORT_EXPORT_ORKBASE DomSerializer : public Serializer
{
public:
	DomSerializer(Object* object) : Serializer(object){};

	void ObjectValue(const char* key, Object& value, bool required = false);
	void ListValue(const char* key, std::list<ObjectRef>& value, Object& model, bool required = false);


	void AddInt(const char* key, int value);
	void AddString(const char* key, CStdString& value);
	void AddObject(const char* key, Object& value);
	void AddList(const char* key, std::list<ObjectRef>& value);
	void Serialize(XERCES_CPP_NAMESPACE::DOMDocument* node);

	void GetString(const char* key, CStdString& value, bool required = false);
	void GetObject(const char* key, Object& value, bool required = false);
	void GetList(const char* key, std::list<ObjectRef>& value, Object& model, bool required = false);
	void DeSerialize(DOMNode* node);

	static CStdString XMLStringToLocal(const XMLCh* const toTranscode);
	static XMLCh* LocalStringToXML(CStdString& toTranscode);

	DOMNode* FindElementByName(DOMNode *node, CStdString name);

	static CStdString DomNodeToString(DOMNode *);
protected:
	DOMNode* m_node;
	XERCES_CPP_NAMESPACE::DOMDocument* m_document;
};

/** Container for xerces unicode string initialized with char* string */
class DLL_IMPORT_EXPORT_ORKBASE XStr
{
public :
    inline XStr(const char* const toTranscode)
    {
        fUnicodeForm = XMLString::transcode(toTranscode);
    }

    inline ~XStr()
    {
        XMLString::release(&fUnicodeForm);
    }

    inline const XMLCh* unicodeForm() const
    {
        return fUnicodeForm;
    }

private :
    XMLCh*   fUnicodeForm;
};


#endif

