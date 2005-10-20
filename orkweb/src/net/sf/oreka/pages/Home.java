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

import org.apache.tapestry.IRequestCycle;
import org.apache.tapestry.html.BasePage;

public abstract class Home extends BasePage {
	
	public abstract String getUsername();
	public abstract void setUsername(String username);

	public abstract String getPassword();
	public abstract void setPassword(String password);
	
	//@Persist
	public abstract String getErrorMessage();
	public abstract void setErrorMessage(String msg);
	
	public Home() {

	}

	public void formSubmit(IRequestCycle cycle) {

			if(		getUsername() != null && getPassword() != null && 
					getUsername().equals("bruno") && 
					getPassword().equals("bru")			)
			{
				cycle.activate("RecSegments");
			}
			else if(ContextListener.debugSwitch == true) {
				cycle.activate("RecSegments");
			}
			else {
				setErrorMessage("Invalid login or password");
			}
	}
}
