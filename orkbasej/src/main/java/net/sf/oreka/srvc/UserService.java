package net.sf.oreka.srvc;

import java.util.List;

import net.sf.oreka.bo.UserBo;
import net.sf.oreka.persistent.OrkUser;

public interface UserService {

	public UserBo login(String username, String password);
	public boolean changePassword(int userId, String oldPassword, String newPassword);
	
	public int getUsers(UserFilter filter, int offset, int number, String orderBy, boolean ascending, List<UserBo> results);

	public void deleteUser(int userId);
	public void disableUser(int userId);
	
	public String getUserLoginStrings(int userId);
	public void setUserLoginStrings(int userId, String loginStringsCsv);
	
	public OrkUser getUserByLoginString(String loginString);
	
	public int getNumNonDisabledUsers();
}
