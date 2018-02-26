package net.sf.oreka.orktrack.messages;

import net.sf.oreka.OrkException;
import net.sf.oreka.messages.SimpleResponseMessage;
import net.sf.oreka.serializers.OrkSerializer;

public class TapeResponse extends SimpleResponseMessage {

	private boolean deleteTape = false;
	
	public void define(OrkSerializer serializer) throws OrkException {

		super.define(serializer);
		deleteTape = serializer.booleanValue("deletetape", deleteTape, false);
	}

	public String getOrkClassName() {

		return "taperesponse";
	}

	public void validate() {
		// TODO Auto-generated method stub
		
	}

	public boolean isDeleteTape() {
		return deleteTape;
	}
	

	public void setDeleteTape(boolean deleteTape) {
		this.deleteTape = deleteTape;
	}
	
	
}
