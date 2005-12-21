package net.sf.oreka.bo;

import org.apache.log4j.Logger;

import net.sf.oreka.persistent.User;
import net.sf.oreka.srvc.UserServiceHbn;

public class UserBo {
	
	static Logger logger = Logger.getLogger(UserBo.class);	
	private User user = new User();

	public boolean isAdmin() {
		
		//logger.debug("isAdmin:" + user.getId());
		
		// Should look for admin role
		if(user.getId() == 1) {
			return true;
		}
		return false;
	}
	
	public User getUser() {
		return user;
	}

	public void setUser(User user) {
		this.user = user;
	}
	
}
