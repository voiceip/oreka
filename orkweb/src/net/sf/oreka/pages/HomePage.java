/*
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

package net.sf.oreka.pages;

import net.sf.oreka.orkweb.ContextListener;
import net.sf.oreka.persistent.OrkUser;
import net.sf.oreka.services.UserServiceHbn;

import org.apache.tapestry.IRequestCycle;
import org.apache.tapestry.PageRedirectException;
import org.apache.tapestry.annotations.Persist;
import org.apache.tapestry.event.PageBeginRenderListener;
import org.apache.tapestry.event.PageEvent;

public abstract class HomePage extends OrkPage implements PageBeginRenderListener {
	
	static UserServiceHbn srv = new UserServiceHbn();
	
	public abstract String getUsername();
	public abstract void setUsername(String username);

	public abstract String getPassword();
	public abstract void setPassword(String password);
	
	@Persist
	public abstract String getErrorMessage();
	public abstract void setErrorMessage(String msg);
	
	public HomePage() {

	}

	public void formSubmit(IRequestCycle cycle) {

		if (ContextListener.debugSwitch == true && getUsername() == null) {
			// In debug mode, make user = admin by default
			setUsername("admin");
		}
		
		OrkUser user = srv.login(getUsername(), getPassword());
		if (user != null) {
			getSessionStateObject().setUser(user);
			cycle.activate("RecSegments");
		}
		else {
			setErrorMessage("Invalid login or password");
		}
	}
	
	public void pageBeginRender(PageEvent event) {
		
		if(getSessionStateObject().getUser() != null) {
			RecSegmentsPage pg = (RecSegmentsPage)getRequestCycle().getPage("RecSegments");	
			throw new PageRedirectException(pg);	
		}
	}
}
