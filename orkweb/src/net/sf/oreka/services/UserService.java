package net.sf.oreka.services;

import net.sf.oreka.persistent.OrkUser;

public interface UserService {

	public OrkUser login(String username, String password);
	public boolean changePassword(int userId, String oldPassword, String newPassword);
	
}
