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

package net.sf.oreka.serializers.test;

import java.io.InputStream;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;
import net.sf.oreka.OrkObject;
import net.sf.oreka.OrkObjectFactory;
import net.sf.oreka.messages.test.TestNestedMessage;
import net.sf.oreka.messages.test.TestSubMessage;
import net.sf.oreka.serializers.DomSerializer;

import org.w3c.dom.Document;
import org.w3c.dom.Element;



public class DomSerializerTest extends TestCase {
	
	DocumentBuilder builder = null;
	
	public static void main (String[] args) {
		junit.textui.TestRunner.run (suite());
	}
	protected void setUp() throws Exception {
		OrkObjectFactory.instance().registerOrkObject(new TestNestedMessage());
		OrkObjectFactory.instance().registerOrkObject(new TestSubMessage());		
		DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
		builder = factory.newDocumentBuilder();
	}
	public static Test suite() {
		return new TestSuite(DomSerializerTest.class);
	}
	
	public void testDeSerializeBasic() throws Exception
	{
		InputStream is = ClassLoader.getSystemClassLoader().getResourceAsStream ("net/sf/oreka/serializers/test/TestNestedMessage.xml");
		Document doc = builder.parse(is);
		DomSerializer ser = new DomSerializer();
		OrkObject obj = ser.deSerialize(doc.getFirstChild());
		assertTrue(((TestNestedMessage)obj).getIntParm() == 546);
		
		TestNestedMessage tnm = (TestNestedMessage)obj;
		TestSubMessage tsm = tnm.getSubMsg();
		assertTrue(tsm.getStringSubParm().equals("hello"));		
	}
	
	public void testSerializeBasic() throws Exception
	{
		// This does a round trip serialization-deserialization in order to test serialization
		TestNestedMessage tnm = new TestNestedMessage();
		tnm.getSubMsg().setStringSubParm("le maure");
		tnm.setDoubleParm(0.666);
		Document doc = builder.newDocument();
		Element  docNode = doc.createElement(tnm.getOrkClassName());
		doc.appendChild(docNode);
		
		DomSerializer ser = new DomSerializer();
		ser.serialize(doc, docNode, tnm);
		System.out.println(DomSerializer.nodeToByteArray(doc).toString());
		OrkObject obj = ser.deSerialize(docNode);	
		assertTrue(((TestNestedMessage)obj).getDoubleParm() == 0.666);
		assertTrue(((TestNestedMessage)obj).getSubMsg().getStringSubParm() == "le maure");
	}
}
