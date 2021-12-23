package net.sf.oreka;



public class OrkBase {
	
	private static OrkBase orkBase = null;
	
	private boolean debugSwitch = false;
	
	public boolean isDebugSwitch() {
		return debugSwitch;
	}
	

	public void setDebugSwitch(boolean debugSwitch) {
		this.debugSwitch = debugSwitch;
	}
	

	private OrkBase() {
	}
	
	public static OrkBase instance() {
		if(orkBase == null) {
			orkBase = new OrkBase();
		}
		return orkBase;
	}
	
	
}
