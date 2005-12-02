package net.sf.oreka.pages;

import net.sf.oreka.persistent.User;

public class SessionStateObject {

	private User user = null;
	
	public boolean isUserLoggedIn() {
		if(user == null) {
			return false;
		}
		return true;
	}

	public User getUser() {
		return user;
	}
	

	public void setUser(User user) {
		this.user = user;
	}
	
	
}
