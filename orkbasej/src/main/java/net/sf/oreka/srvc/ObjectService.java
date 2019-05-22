package net.sf.oreka.srvc;

public interface ObjectService {
	
	public void saveObject(Object obj);
	public Object getObjectById(java.lang.Class cl, int id);
	public int getNumObjects(java.lang.Class cl);
}
