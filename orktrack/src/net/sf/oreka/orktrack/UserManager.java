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

package net.sf.oreka.orktrack;

import java.util.HashMap;
import java.util.List;

import net.sf.oreka.persistent.User;

import org.apache.log4j.Logger;
import org.hibernate.Session;

public class UserManager {

	private static Logger log = null;
	private static UserManager userManager = null;
	
	private HashMap<User, Port> userLocations = new HashMap<User, Port>();
	
	private UserManager() {
		log = LogManager.getInstance().getUserLogger();
	}
	
	public static UserManager instance() {
		
		if(userManager == null) {
			userManager = new UserManager();
		}
		return userManager;
	}
	
	public User getByLoginString(String loginString, Session hbnSession) {
		
		User user = null;
		List users = hbnSession.createQuery(
	    "from LoginString as ls join ls.user as usr where ls.loginString=:loginstring")
	    .setString("loginstring", loginString)
	    .list();
		if (users.size() > 0) {
			Object[] row =  (Object[])users.get(0);
			if (row.length > 1) {
				user = (User)row[1];
			}
		}
		return user;
	}
	
	public synchronized void setUserLocation(User user, Port port) {
		userLocations.put(user, port);
	}
	
	public synchronized Port getUserLocation(User user) {
		return userLocations.get(user);
	}
}
