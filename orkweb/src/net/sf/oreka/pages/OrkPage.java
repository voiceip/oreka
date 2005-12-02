package net.sf.oreka.pages;

import org.apache.tapestry.IAsset;
import org.apache.tapestry.annotations.Asset;
import org.apache.tapestry.annotations.InjectState;
import org.apache.tapestry.html.BasePage;

public abstract class OrkPage extends BasePage
{
	 @InjectState("session-state-object")
	  public abstract SessionStateObject getSessionStateObject();
	 
	  @Asset("css/orekastyle.css")
	  public abstract IAsset getGlobalStylesheet();
}
