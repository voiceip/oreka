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

#ifndef __OBJECT_H__
#define __OBJECT_H__

#include "dll.h"
#include "OrkBase.h"
#include "StdString.h"
#include "shared_ptr.h"
#include "xercesc/dom/DOMNode.hpp"

class Serializer;
class Object;

using namespace XERCES_CPP_NAMESPACE;

typedef oreka::shared_ptr<Object> ObjectRef;

#define OBJECT_TYPE_TAG "type"

/** An Object is the equivalent of a Java Object
*/
class DLL_IMPORT_EXPORT_ORKBASE Object
{
public:
	virtual void Define(Serializer* s) = 0;
	virtual void Validate() = 0;

	CStdString SerializeSingleLine();
	void DeSerializeSingleLine(CStdString& input);

	void SerializeDom(XERCES_CPP_NAMESPACE::DOMDocument* doc);
	void DeSerializeDom(DOMNode* doc);

	CStdString SerializeUrl();
	void DeSerializeUrl(CStdString& input);

	virtual CStdString GetClassName() = 0;
	virtual ObjectRef NewInstance() = 0;

	virtual ObjectRef Process() = 0;

	oreka::shared_ptr<Serializer> GetSerializer();

private:
	oreka::shared_ptr<Serializer> m_serializer;
};



#endif

