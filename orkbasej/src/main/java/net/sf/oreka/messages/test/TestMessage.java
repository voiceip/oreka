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

package net.sf.oreka.messages.test;

import net.sf.oreka.*;
import net.sf.oreka.messages.AsyncMessage;
import net.sf.oreka.messages.SyncMessage;
import net.sf.oreka.serializers.*;

public class TestMessage extends SyncMessage 
{
	static final String orkObjectClass = "test";
	
	public enum TestEnum {value1, value2, value3};
	
	String stringParm = "";
	int intParm = 0;
	double doubleParm = 0.0;
	boolean boolParm = false;
	TestEnum enumParm = TestEnum.value1;
	
	public String getOrkClassName()
	{
		return orkObjectClass;
	}
	
	public void define(OrkSerializer serializer) throws OrkException
	{
		//serializer.stringValue("command", orkObjectClass, true);
		intParm = serializer.intValue("intParm", intParm, true);
		doubleParm = serializer.doubleValue("doubleParm", doubleParm, false);
		stringParm = serializer.stringValue("stringParm", stringParm, false);
		boolParm = serializer.booleanValue("boolParm", boolParm, false);
		enumParm = (TestEnum)serializer.enumValue("enumParm", enumParm, false);
	}
	
	public void validate() 
	{
		;
	}

	public AsyncMessage process()
	{
		return null;
	}

	public int getIntParm() {
		return intParm;
	}

	public void setIntParm(int intParm) {
		this.intParm = intParm;
	}

	public String getStringParm() {
		return stringParm;
	}

	public void setStringParm(String stringParm) {
		this.stringParm = stringParm;
	}

	public boolean isBoolParm() {
		return boolParm;
	}

	public void setBoolParm(boolean boolParm) {
		this.boolParm = boolParm;
	}

	public double getDoubleParm() {
		return doubleParm;
	}

	public void setDoubleParm(double doubleParm) {
		this.doubleParm = doubleParm;
	}

	public TestEnum getEnumParm() {
		return enumParm;
	}

	public void setEnumParm(TestEnum enumParm) {
		this.enumParm = enumParm;
	}
}
