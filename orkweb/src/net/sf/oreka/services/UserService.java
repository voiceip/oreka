package net.sf.oreka.services;

import net.sf.oreka.persistent.User;

public interface UserService {

	public User login(String username, String password);
	public boolean changePassword(int userId, String oldPassword, String newPassword);
	
}
