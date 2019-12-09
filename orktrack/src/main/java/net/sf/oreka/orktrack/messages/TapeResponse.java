package net.sf.oreka.orktrack.messages;

import lombok.Getter;
import lombok.Setter;
import net.sf.oreka.OrkException;
import net.sf.oreka.messages.SimpleResponseMessage;
import net.sf.oreka.serializers.OrkSerializer;

@Getter
@Setter
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


}
