package net.sf.oreka.pages;

import org.apache.tapestry.PageRedirectException;
import org.apache.tapestry.event.PageEvent;
import org.apache.tapestry.event.PageValidateListener;


public abstract class ProtectedPage extends OrkPage implements PageValidateListener
{
	public void pageValidate(PageEvent event)
	{
		if(getSessionStateObject().isUserLoggedIn()) {
			return;	// Ok, all fine
		}
		// Need to authenticate user
		HomePage home = (HomePage) getRequestCycle().getPage("Home");	
		throw new PageRedirectException(home);
		
		// #### See if we can set a callback here
	}
}