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

import junit.framework.Test;
import junit.framework.TestCase;
import junit.framework.TestSuite;
import net.sf.oreka.OrkObject;
import net.sf.oreka.OrkObjectFactory;
import net.sf.oreka.messages.test.TestMessage;
import net.sf.oreka.serializers.ServletRequestSerializer;

public class ServletRequestSerializerTest extends TestCase {
	
	public static void main (String[] args) {
		junit.textui.TestRunner.run (suite());
	}
	protected void setUp() {
		OrkObjectFactory.instance().registerOrkObject(new TestMessage());
	}
	public static Test suite() {
		return new TestSuite(ServletRequestSerializerTest.class);
	}
	
	public void testSerializeBasic() throws Exception
	{
		NullHttpServletRequest req = new NullHttpServletRequest();
		ServletRequestSerializer ser = new ServletRequestSerializer();
		req.setParameter("cmd", "test");
		req.setParameter("intParm", "45");
		OrkObject obj = ser.deSerialize(req);
		assertTrue(((TestMessage)obj).getIntParm() == 45);
	}
}
