package net.sf.oreka.srvc;

import net.sf.oreka.bo.UserBo;

public interface UserService {

	public UserBo login(String username, String password);
	public boolean changePassword(int userId, String oldPassword, String newPassword);
	
}
