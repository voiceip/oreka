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

#include "xercesc/util/XMLString.hpp"
#include <xercesc/util/PlatformUtils.hpp>
#include <xercesc/dom/DOM.hpp>
#include <xercesc/dom/DOMElement.hpp>
#include <xercesc/dom/DOMImplementation.hpp>
#include <xercesc/dom/DOMImplementationLS.hpp>
#include <xercesc/dom/DOMWriter.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>

#include "DomSerializer.h"

void DomSerializer::ObjectValue(const char* key, Object& value, bool required)
{
	if (m_deSerialize == true)
	{
		GetObject(key, value, required);
	}
	else
	{
		AddObject(key, value);
	}
}

void DomSerializer::ListValue(const char* key, std::list<ObjectRef>& value, Object& model, bool required)
{
	if (m_deSerialize == true)
	{
		GetList(key, value, model, required);
	}
	else
	{
		AddList(key, value);
	}
}

void DomSerializer::GetString(const char* key, CStdString& value, bool required)
{
	// Find the right node
	DOMNode* stringNode = FindElementByName(m_node, CStdString(key));

	if(stringNode)
	{
		// Now, the string associated to element should be the first child (text element)
		DOMNode* textNode = stringNode->getFirstChild();
		if (textNode && textNode->getNodeType() == DOMNode::TEXT_NODE)
		{
			value = XMLStringToLocal(textNode->getNodeValue());
		}
	}
	else if (required)
	{
		throw(CStdString("DomSerializer::GetString: required parameter missing:") + key);
	}
}

void DomSerializer::GetObject(const char* key, Object& value, bool required)
{
	// Find the node corresponding to the object wanting to be populated
	DOMNode* objectNode = FindElementByName(m_node, CStdString(key));

	// Create a new serializer and affect it to this object
	if (objectNode)
	{
		DomSerializer serializer(&value);
		serializer.DeSerialize(objectNode);
	}
	else if (required)
	{
		throw(CStdString("DomSerializer::GetObject: required node missing:") + key);
	}
}

void DomSerializer::GetList(const char* key, std::list<ObjectRef>& value, Object& model, bool required)
{
	// Find the node corresponding to the object list wanting to be populated
	DOMNode* listNode = FindElementByName(m_node, CStdString(key));

	// Create a new serializer and affect it to this object
	if (listNode)
	{
		// Iterate over the nodes #####
		DOMNode* node = listNode->getFirstChild();
		while(node)
		{
			// Create a new object instance
			ObjectRef newObject = model.NewInstance();
			try
			{
				DomSerializer serializer(newObject.get());
				serializer.DeSerialize(node);
				value.push_back(newObject);

			}
			catch (CStdString& e)
			{
				// For now, do not interrupt the deserialization process.
				// in the future, we might let this exception go through if the node has been 
				// recognized to bear the proper tag name 
				;
			}
			node = node->getNextSibling();
		}

	}
	else if (required)
	{
		throw(CStdString("DomSerializer::GetList: required node missing:") + key);
	}
}


void DomSerializer::AddObject(const char* key, Object& value)
{
	// Not yet implemented ####
	;
}

void DomSerializer::AddList(const char* key, std::list<ObjectRef>& value)
{
	// Not yet implemented ####
	;
}


void DomSerializer::AddString(const char* key, CStdString& value)
{
	DOMElement*  newElem = m_document->createElement(XStr(key).unicodeForm());
	m_node->appendChild(newElem);

	DOMText*    newText = m_document->createTextNode(XStr((PCSTR)value).unicodeForm());
	newElem->appendChild(newText);
}

void DomSerializer::Serialize(XERCES_CPP_NAMESPACE::DOMDocument* doc)
{
	if(doc)
	{
		m_document = doc;
		m_node = m_document->getDocumentElement();
		m_deSerialize = false;		// Set Serialize mode
		m_object->Define(this);
	}
	else
	{
		throw(CStdString("DomSerializer::Serialize: input DOM document is NULL"));
	}
}

void DomSerializer::DeSerialize(DOMNode* node)
{
	m_node = node;
	Serializer::DeSerialize();
}

CStdString DomSerializer::XMLStringToLocal(const XMLCh* const toTranscode)
{
	char* szResult = XMLString::transcode(toTranscode);
	CStdString result = szResult;
	XMLString::release(&szResult);
	return result;
}

DOMNode* DomSerializer::FindElementByName(DOMNode *node, CStdString name)
{
	DOMNode *child = node->getFirstChild();
	while(child)
	{
		if (XMLStringToLocal(child->getNodeName()) == name && child->getNodeType() == DOMNode::ELEMENT_NODE)
		{
			return child;
		}
		child = child->getNextSibling();
	}
	return NULL;
}

CStdString DomSerializer::DomNodeToString(DOMNode* node)
{
	CStdString output;

    DOMImplementation *impl          = DOMImplementationRegistry::getDOMImplementation(XStr("LS").unicodeForm());
    DOMWriter         *theSerializer = ((DOMImplementationLS*)impl)->createDOMWriter();
    // set user specified output encoding
    //theSerializer->setEncoding(gOutputEncoding);
	theSerializer->setFeature(XStr("format-pretty-print").unicodeForm(), true); 

    XMLFormatTarget *myFormTarget;
	myFormTarget = new MemBufFormatTarget ();
    theSerializer->writeNode(myFormTarget, *node);

	output = (char *)((MemBufFormatTarget*)myFormTarget)->getRawBuffer();

	// Clean up
    delete theSerializer;
    //
    // Filter, formatTarget and error handler
    // are NOT owned by the serializer.
    delete myFormTarget;

	return output;
}
