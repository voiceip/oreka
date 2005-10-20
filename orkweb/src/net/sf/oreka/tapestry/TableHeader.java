/*
 * Oreka -- A media capture and retrieval platform
 * 
 * Copyright (C) 2005, orecx LLC
 *
 * http://www.orecx.com
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License.
 * Please refer to http://www.gnu.org/copyleft/gpl.html
 *
 */

/**
 * 
 */
package net.sf.oreka.tapestry;

import java.util.*;
import org.apache.commons.lang.StringUtils;
import org.apache.log4j.*;
import org.apache.tapestry.*;


public abstract class TableHeader extends BaseComponent {

	static Logger logger = Logger.getLogger(TableHeader.class);
	
	public abstract TableState getTableState();
	public abstract void setTableState(TableState state);
	
	//private String columns;
	public abstract String getColumnsCSV();
	public abstract void setColumnsCSV(String columns);

	
	public List getColumns() {
		String[] columnsArray = StringUtils.split(getColumnsCSV(),", ");
		//logger.log(Level.INFO, "ColumnsCSV: "+getColumnsCSV());
		
		List columns = new ArrayList();
		for (int i=0; i<columnsArray.length; i++) {
			TableColumn column = new TableColumn();
			column.setSortable(true);
			String columnKey = columnsArray[i];
			if (columnsArray[i].charAt(0) == '!')	{
				column.setSortable(false);
				columnKey = columnsArray[i].substring(1);
			}
			column.setKey(columnKey);
			String LocalizedMessage = getPage().getMessages().getMessage(columnKey);
			column.setMessage(LocalizedMessage);
			columns.add(column);
		}
		return columns;
	}
	
	public abstract void setColumn(TableColumn col);
	public abstract TableColumn getColumn();

	/**
	 * 
	 */
	public TableHeader() {
	}

    public void columnClickAction(IRequestCycle cycle) {
      
        Object[] parameters = cycle.getServiceParameters();
		String column = (String)parameters[0];
		if (!column.equals(getTableState().getOrderBy())) {
			getTableState().setOrderBy(column);
			getTableState().setAscending(true);
		}
		else {
			// Toggle direction
			getTableState().setAscending(!getTableState().isAscending());
		}
		setTableState(getTableState());
    }
}
