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

import junit.framework.*;
import net.sf.oreka.*;
import net.sf.oreka.messages.*;
import net.sf.oreka.messages.test.TestMessage;
import net.sf.oreka.serializers.SingleLineSerializer;

public class SingleLineSerializerTest extends TestCase  
{
	public static void main (String[] args) {
		junit.textui.TestRunner.run (suite());
	}
	protected void setUp() {
		OrkObjectFactory.instance().registerOrkObject(new TestMessage());
	}
	public static Test suite() {
		return new TestSuite(SingleLineSerializerTest.class);
	}
	
	public void testSerializeBasic() throws Exception
	{
		String output = null;
		
		TestMessage msg = new TestMessage();
		SingleLineSerializer ser = new SingleLineSerializer();
		
		msg.setIntParm(6);
		output = ser.serialize(msg);
		System.out.println("\noutput:" + output);
		assertTrue(output.toLowerCase().indexOf("intparm=6") != -1);
		
		msg.setIntParm(-5);
		output = ser.serialize(msg);
		System.out.println("\noutput:" + output);
		assertTrue(output.toLowerCase().indexOf("intparm=-5") != -1);
		
		msg.setDoubleParm(7.7);
		output = ser.serialize(msg);
		System.out.println("\noutput:" + output);
		assertTrue(output.toLowerCase().indexOf("doubleparm=7.7") != -1);
		
		msg.setBoolParm(true);
		output = ser.serialize(msg);
		System.out.println("\noutput:" + output);
		assertTrue(output.toLowerCase().indexOf("boolparm=true") != -1);
		
		msg.setStringParm("hello");
		output = ser.serialize(msg);
		System.out.println("\noutput:" + output);
		assertTrue(output.toLowerCase().indexOf("stringparm=hello") != -1);
	}

	public void testDeSerializeBasic() throws Exception
	{
		String input;
		SingleLineSerializer ser = new SingleLineSerializer();
		TestMessage msg;
		
		input = "class=test intparm=3";
		msg = (TestMessage)ser.deSerialize(input);
		assertTrue(msg.getIntParm() == 3);
		
		input = "class=test doubleparm=-14.6";
		msg = (TestMessage)ser.deSerialize(input);
		assertTrue(msg.getDoubleParm() == -14.6);
		
		input = "class=test boolparm=yes";
		msg = (TestMessage)ser.deSerialize(input);
		assertTrue(msg.isBoolParm());
	}
	
	public void testDeSerializeBoolean() throws Exception
	{
		String input;
		SingleLineSerializer ser = new SingleLineSerializer();
		TestMessage msg;
		
		input = "class=test intparm=78 boolparm=no";
		msg = (TestMessage)ser.deSerialize(input);
		assertFalse(msg.isBoolParm());	
		
		input = "class=test intparm=78 boolparm=0";
		msg = (TestMessage)ser.deSerialize(input);
		assertFalse(msg.isBoolParm());
		
		input = "class=test intparm=78 boolparm=false";
		msg = (TestMessage)ser.deSerialize(input);
		assertFalse(msg.isBoolParm());
		
		input = "class=test intparm=78 boolparm=true";
		msg = (TestMessage)ser.deSerialize(input);
		assertTrue(msg.isBoolParm());
		
		input = "class=test intparm=78 boolparm=yes";
		msg = (TestMessage)ser.deSerialize(input);
		assertTrue(msg.isBoolParm());
		
		input = "class=test intparm=78 boolparm=1";
		msg = (TestMessage)ser.deSerialize(input);
		assertTrue(msg.isBoolParm());
		
		input = "class=test intparm=78 boolparm=asdf";
		boolean threw = false;
		try {
			msg = (TestMessage)ser.deSerialize(input);
		}
		catch (OrkException e) {
			threw = true;
		}
		assertTrue(threw);
	}
	
	public void testDeSerializeRequired() throws Exception
	{
		String input;
		SingleLineSerializer ser = new SingleLineSerializer();
		TestMessage msg;
		
		input = "class=test";
		boolean threw = false;
		try {
			msg = (TestMessage)ser.deSerialize(input);
		}
		catch (OrkException e) {
			threw = true;
		}
		assertTrue(threw);
	}
	
	public void testEnum() throws Exception
	{
		// deserialize
		String input;
		SingleLineSerializer ser = new SingleLineSerializer();
		TestMessage msg;
		
		input = "class=test intparm=78 enumparm=value2";
		msg = (TestMessage)ser.deSerialize(input);
		assertTrue(msg.getEnumParm() == TestMessage.TestEnum.value2);	
		
		input = "class=test intparm=78 enumparm=garbage";
		boolean threw = false;
		try {
			msg = (TestMessage)ser.deSerialize(input);
		}
		catch (OrkException e) {
			threw = true;
		}
		assertTrue(threw);
		
		// serialize
		String output = null;
		msg.setEnumParm(TestMessage.TestEnum.value3);
		output = ser.serialize(msg);
		assertTrue(output.toLowerCase().indexOf("enumparm=value3") != -1);
	}

}
