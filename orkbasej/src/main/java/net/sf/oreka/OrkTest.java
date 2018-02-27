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

package net.sf.oreka;

import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import net.sf.oreka.messages.test.TestMessage;
import net.sf.oreka.messages.test.TestNestedMessage;
import net.sf.oreka.messages.test.TestSubMessage;
import net.sf.oreka.serializers.DomSerializer;

import org.w3c.dom.Document;
import org.w3c.dom.Element;

public class OrkTest {

	enum MyEnum {one, two, three};
	enum MyEnum2 {four, five, six};
	
	

	
	public static void main(String args[]) throws Exception
	{
		/* Enums
		MyEnum toto = MyEnum.three;
		MyEnum2 titi = MyEnum2.six;
		
		Enum grrrr = toto;
		System.out.printf("%s", grrrr.name());
		grrrr = titi;
		System.out.printf("%s", grrrr.name());
		
		Class aClass = titi.getClass();
		grrrr = Enum.valueOf(aClass, "five");

		System.out.printf("%s", grrrr.name());
		*/
		/* Reflection
		Class myClass = Class.forName("java.lang.String");
		Object myObj = myClass.newInstance();
		String boum = (String)myObj;
		*/
		/*
		TestMessage msg = new TestMessage();
		SingleLineSerializer ser = new SingleLineSerializer();
		String serialized = ser.serialize(msg);
		System.out.println(serialized);
		*/
		/*
		Class toto = null;
		toto = TestMessage.class;
		
		System.out.println(toto.toString());
		
		java.util.ArrayList al = new java.util.ArrayList();
		al.add(toto);
		
		java.util.HashMap<Class, Class> hm = new java.util.HashMap();
		hm.put(toto, toto);
		*/
		
//		TestMessage toto = new TestMessage();
//		
//		OrkObjectFactory.instance().RegisterOrkObject(toto);
//		OrkObject obj = OrkObjectFactory.instance().newOrkObject("test");
//		
//		TestMessage msg = (TestMessage)obj;
//		msg.setIntParm(5);
//		msg.setStringParm("henri");
//		
//		SingleLineSerializer ser = new SingleLineSerializer();
//		ser.setClassKey("message");
//		String serialized = ser.serialize(msg);
//		System.out.println(serialized);
//		
//		OrkObject obj2 = ser.deSerialize(serialized);
//		String str = ser.serialize(obj2);
//		System.out.println(str);
		
		OrkObjectFactory.instance().registerOrkObject(new TestMessage());
//		InputStream is = ClassLoader.getSystemClassLoader().getResourceAsStream ("net/sf/oreka/TestMessage.xml");
//		
//		DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
//		DocumentBuilder builder = factory.newDocumentBuilder();
//		//Document doc = builder.parse("c:/toto.xml");
//		Document doc = builder.parse(is);
//		
//		DomSerializer ser = new DomSerializer();
//		OrkObject obj = ser.deSerialize(doc.getFirstChild());
		
		TestMessage msg = new TestMessage();
		DomSerializer ser = new DomSerializer();
		DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
		DocumentBuilder builder = factory.newDocumentBuilder();
		Document doc = builder.newDocument();
		
		Element el = doc.createElement("nob");
		doc.appendChild(el);
		
//		System.out.println(DomSerializer.NodeToString(doc));
		
		ser.serialize(doc, el, msg);
		System.out.println(DomSerializer.nodeToByteArray(doc).toString());
		
		TestNestedMessage tnm = new TestNestedMessage();
		TestSubMessage tsm = tnm.getSubMsg();
		
		
	}
}
