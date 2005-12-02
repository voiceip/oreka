package net.sf.oreka.pages;

import net.sf.oreka.services.UserServiceHbn;

import org.apache.tapestry.IRequestCycle;
import org.apache.tapestry.annotations.Persist;

public abstract class AccountPage extends ProtectedPage {

	static UserServiceHbn srv = new UserServiceHbn();
	
	public abstract String getNewPassword1();
	public abstract void setNewPassword1(String password);

	public abstract String getNewPassword2();
	public abstract void setNewPassword2(String password);	
	
	public abstract String getPassword();
	public abstract void setPassword(String password);
	
	@Persist
	public abstract String getErrorMessage();
	public abstract void setErrorMessage(String msg);
	
	public void formSubmit(IRequestCycle cycle) {

		setErrorMessage("Password Incorrect, please try again");
		
		if(getNewPassword1().equals(getNewPassword2())) {
			if (srv.changePassword(getSessionStateObject().getUser().getId(), getPassword(), getNewPassword1())) {
				setErrorMessage(null);
				cycle.activate("RecSegments");
			}
		}
	}
}
