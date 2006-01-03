package net.sf.oreka.bo;

import java.io.Serializable;
import java.util.Iterator;

import net.sf.oreka.persistent.LoginString;
import net.sf.oreka.persistent.User;

import org.apache.log4j.Logger;

public class UserBo implements Serializable {
	
	static Logger logger = Logger.getLogger(UserBo.class);	
	private User user = new User();
	
	private boolean selected = false;

	public boolean isSelected() {
		return selected;
	}
	

	public void setSelected(boolean selected) {
		this.selected = selected;
	}
	

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
	
//	public String getLoginStringsCsv() {
//	
//		String loginStringsCsv = "";
//		Iterator it = user.getLoginStrings().iterator();
//		if(it.hasNext()) {
//			loginStringsCsv = ((LoginString)it.next()).getLoginString();
//		}
//		while(it.hasNext()) {
//			loginStringsCsv += ", " + ((LoginString)it.next()).getLoginString();
//		}
//		return loginStringsCsv;
//	}
	
	public void setLoginStringsCsv() {
		
		
	}
	
}
