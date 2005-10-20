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

package net.sf.oreka.persistent;

import javax.persistence.Column;
import javax.persistence.Entity;
import javax.persistence.GeneratorType;
import javax.persistence.Id;


@Entity
public class AnnotatedTestClass {

	public enum TestEnum{value1, value2};
	
	long Id;
	TestEnum myEnum;
	
	String firstname;
	String lastname;
	public String getFirstname() {
		return firstname;
	}
	public void setFirstname(String firstname) {
		this.firstname = firstname;
	}
	@Id(generate=GeneratorType.AUTO)
	public long getId() {
		return Id;
	}
	public void setId(long id) {
		Id = id;
	}
	public String getLastname() {
		return lastname;
	}
	public void setLastname(String lastname) {
		this.lastname = lastname;
	}
	
	@Column(columnDefinition="varchar(255)")
	public TestEnum getMyEnum() {
		return myEnum;
	}
	public void setMyEnum(TestEnum myEnum) {
		this.myEnum = myEnum;
	}
}
