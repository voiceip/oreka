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



package net.sf.oreka.serializers;


import java.io.ByteArrayOutputStream;

import javax.xml.transform.Result;
import javax.xml.transform.Transformer;
import javax.xml.transform.TransformerException;
import javax.xml.transform.TransformerFactory;
import javax.xml.transform.dom.DOMSource;
import javax.xml.transform.stream.StreamResult;

import net.sf.oreka.OrkException;
import net.sf.oreka.OrkObject;
import net.sf.oreka.OrkObjectFactory;

import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.Text;

public class DomSerializer extends OrkSerializer {

	Node node = null;
	Document document = null;
	
	public OrkObject deSerialize(Node node) throws OrkException
	{
		if (node == null)
		{
			throw (new OrkException("DomSerializer: null node was passed in"));
		}
		this.node = node;
		deserialize = true;		// Set DeSerialize mode
		OrkObject obj = OrkObjectFactory.instance().newOrkObject(node.getNodeName());
		obj.define(this);
		obj.validate();
		return obj;
	}
	
	public void serialize(Document document, Node node, OrkObject object) throws OrkException {
		
		if (node == null) {
			throw ( new OrkException("DomSerializer: passed DOM node is null"));
		}
		if (document == null) {
			throw ( new OrkException("DomSerializer: passed DOM document is null"));
		}
		this.document = document;
		this.node = node;
		
		deserialize = false;		// Set Serialize mode
		object.define(this);
	}
	
	@Override
	public void addClassName(String value) {
		// TODO Auto-generated method stub
		
	}

	@Override
	void addString(String key, String value) {
		
		Element  newElem = document.createElement(key);
		node.appendChild(newElem);

		Text newText = document.createTextNode(value);
		newElem.appendChild(newText);
	}

	@Override
	String getString(String key, String oldValue, boolean required) throws OrkException {
		
		String value = null;
		
		// Find the right node
		Node stringNode = findElementByName(node, key);

		if(stringNode != null)
		{
			// Now, the string associated to element should be the first child (text element)
			Node textNode = stringNode.getFirstChild();
			if (textNode != null && textNode.getNodeType() == Node.TEXT_NODE) {
				value = textNode.getNodeValue().trim();
			}
		}
		if (value == null) {
			if (required) {
				throw(new OrkException("DomSerializer::GetString: required parameter missing:" + key));
			}
			value = oldValue;
		}
		return value;
	}

	@Override
	public OrkObject objectValue(String key, OrkObject value, boolean required) throws OrkException {
		
		if (deserialize) {
			return getObject(key, value, required);
		}
		else {
			addObject(key, value);
			return value;
		}
	}
	
	void addObject(String key, OrkObject value) throws OrkException {
		
		Element newElem = document.createElement(key);
		node.appendChild(newElem);
		DomSerializer ser = new DomSerializer();
		ser.serialize(document, newElem, value);
	}

	OrkObject getObject(String key, OrkObject oldValue, boolean required) throws OrkException {
		
		OrkObject value = null;
		
		// Find the node corresponding to the object wanting to be populated
		Node objectNode = findElementByName(node, key);

		// Create a new serializer and affect it to this object
		if (objectNode != null)
		{
			DomSerializer serializer = new DomSerializer();
			value = serializer.deSerialize(objectNode);
		}
		if (value == null) {
			 if (required) {
				 throw(new OrkException("DomSerializer.getObject: required node missing:" + key));
			 }
			 value = oldValue;
		}
		return value;
	}
	
	public Node findElementByName(Node node, String name)
	{
		Node child = node.getFirstChild();
		while(child != null)
		{
			if (child.getNodeName().equalsIgnoreCase(name) && child.getNodeType() == Node.ELEMENT_NODE)
			{
				return child;
			}
			child = child.getNextSibling();
		}
		return null;
	}

	public static byte[] nodeToByteArray(Node node) throws TransformerException {
	
		TransformerFactory xformerFactory = TransformerFactory.newInstance();
		Transformer xformer = xformerFactory.newTransformer();
		xformer.setOutputProperty("indent", "yes");
		ByteArrayOutputStream byteArray = new ByteArrayOutputStream();
		Result output = new StreamResult(byteArray);
		DOMSource source = new DOMSource(node);
		xformer.transform(source,output);
		return byteArray.toByteArray();
	}
	
}
