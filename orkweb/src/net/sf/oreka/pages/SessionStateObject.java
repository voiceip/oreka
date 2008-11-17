package net.sf.oreka.pages;

import net.sf.oreka.persistent.OrkUser;

public class SessionStateObject {

	private OrkUser user = null;
	
	public boolean isUserLoggedIn() {
		if(user == null) {
			return false;
		}
		return true;
	}

	public OrkUser getUser() {
		return user;
	}
	

	public void setUser(OrkUser user) {
		this.user = user;
	}
	
	
}
