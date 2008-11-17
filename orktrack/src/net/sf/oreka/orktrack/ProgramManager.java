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

package net.sf.oreka.orktrack;

import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import net.sf.oreka.Cycle;
import net.sf.oreka.Direction;
import net.sf.oreka.HibernateManager;
import net.sf.oreka.persistent.OrkProgram;
import net.sf.oreka.persistent.OrkSegment;

import org.apache.log4j.Logger;
import org.hibernate.Session;
import org.hibernate.Transaction;

public class ProgramManager {
	
	static Logger logger = Logger.getLogger(ProgramManager.class);
	
	private static ProgramManager programManager = null;
	private ArrayList<OrkProgram> recPrograms = new ArrayList<OrkProgram>();
	
	private ProgramManager() {
	}
	
	public static ProgramManager instance() {
		
		if (programManager == null) {
			programManager = new ProgramManager();
		}
		return programManager;
	}
	
	public boolean load() {
		
		Session hbnSession = null;
		Transaction tx = null;
		boolean success = false;
		
		try {
			hbnSession = OrkTrack.hibernateManager.getSession();
			tx = hbnSession.beginTransaction();
			
			List progs = hbnSession.createQuery(
		    "from OrkProgram as prg where prg.active=:active and prg.discarded=:discarded")
		    .setBoolean("active", true)
		    .setBoolean("discarded", false)
		    .list();
			
			recPrograms = new ArrayList<OrkProgram>(progs);
			tx.commit();
			success = true;
		}
		catch (Exception e) {
			logger.error("Could not load programs"+ e.getClass().getName());
		}
		finally {
			if(hbnSession != null) {hbnSession.close();}
		}
		return success;
	}
	
	public void addProgram(OrkProgram prog) {
		
		recPrograms.add(prog);
	}
	
	public boolean filterSegmentAgainstAllPrograms(OrkSegment seg, Session hbnSession) {
		
		boolean result = false;
		// Iterate over programs
		ArrayList<OrkProgram> progs = recPrograms;
		if(recPrograms.size() == 0) {
			// If there are no programs specified, keep everything
			result = true;
		}
		else {
			for(int i=0; i<progs.size(); i++) {
				if(filterSegmentAgainstProgram(seg, progs.get(i), hbnSession)) {
					result = true;
				}
			}
		}
		return result;
	}
	
	public boolean filterSegmentAgainstProgram(OrkSegment seg, OrkProgram prog, Session hbnSession) {
		
		boolean drop = false;
		boolean result = false;
		String dropReason = "";
		
		if (	prog.getDirection() != Direction.ALL &&
				seg.getDirection() != prog.getDirection()) {
			dropReason = "Dir.";
			drop = true;
		}
		
		if (	!drop && prog.getMaxDuration() > 0 &&
				(seg.getDuration() > (prog.getMaxDuration()*1000))) {
			dropReason = "Max Dur.";
			drop = true;
		}
		
		if (	!drop && prog.getMinDuration() > 0 &&
				(seg.getDuration() < (prog.getMinDuration()*1000))) {
			dropReason = "Min Dur.";
			drop = true;
		}
		
		if (!drop && prog.getRandomPercent() != 0.0) {
			double randomNumber = java.lang.Math.random();
			if (randomNumber > prog.getRandomPercent()) {
				dropReason = "Random percent";
				drop = true;
			}
		}
		
		if (	!drop && prog.getLocalParty().length() > 0 &&
				(!seg.getLocalParty().equals(prog.getLocalParty()))   ) {
			dropReason = "LocalParty";
			drop = true;
		}
		
		if (	!drop && prog.getRemoteParty().length() > 0 &&
				(!seg.getRemoteParty().equals(prog.getRemoteParty()))    ) {
			dropReason = "RemoteParty";
			drop = true;
		}
			
		if (	!drop &&
				(prog.getTargetUser() != null) &&
				(seg.getUser().getId() != prog.getTargetUser().getId())   ) {
			dropReason = "Target User";
			drop = true;
		}
		
		if (	!drop &&
				(prog.getTargetPort() != null) &&
				(seg.getPort().getId() != prog.getTargetPort().getId())   ) {
			dropReason = "Target Port";
			drop = true;
		}
		
		if ( 	!drop &&
				prog.getRecPerCycle() != 0 &&
				prog.getRecordedSoFar() > prog.getRecPerCycle() ) {
			dropReason = "Enough recordings for this Cycle";
			drop = true;
		}

		if ( 	!drop &&
				filterSegmentAgainstProgramSchedule(seg, prog) == false ) {
			dropReason = "Schedule";
			drop = true;
		}
		
		if(drop == false) {
			// All criteria have passed, the segment is accepted by the program
			prog.setRecordedSoFar(prog.getRecordedSoFar() + 1);
			if(hbnSession != null) {
				hbnSession.update(prog);
				seg.getRecPrograms().add(prog);
			}
			result = true;
		}
		else {
			logger.debug("Contact dropped, reason: " + dropReason);
		}
		return result;
	}
	
	public boolean filterSegmentAgainstProgramSchedule(OrkSegment seg, OrkProgram prog) {
		
		if(prog.getCycle() == Cycle.PERMANENT) {
			return true;
		}
		else {
			long startPrgMillis = prog.getStartTime().getTime();
			long stopPrgMillis = prog.getStopTime().getTime();			
			long startSegMillis = seg.getTimestamp().getTime();
			Date startOfDay = new Date(seg.getTimestamp().getTime());
			startOfDay.setHours(0);
			startOfDay.setMinutes(0);
			startOfDay.setSeconds(0);
			long startOfDayMillis = startOfDay.getTime();
			long startSegInDayMillis =  startSegMillis - startOfDayMillis;
			logger.debug("Hour: Seg start:" + startSegInDayMillis + " Prg start:" +  startPrgMillis + " Prg stop:" + stopPrgMillis);
						
			if(prog.getCycle() == Cycle.DAILY) {				
				if(startSegInDayMillis >=  startPrgMillis && startSegInDayMillis <= stopPrgMillis) {
					return true;
				}
			}
			else if(prog.getCycle() == Cycle.WEEKLY) {
				// Determine the day that the call has been made
				int startDay = seg.getTimestamp().getDay();
				startDay--;	// in java.util.Date, {Sunday ... Saturday} = {0 ... 6}. In net.sf.oreka.Day {monday ... sunday} = {0 ... 6}
				if(startDay == -1) {
					startDay = 6;
				}
				logger.debug("Day: Seg start:" + startDay + " Prg start:" +  prog.getStartDay().ordinal() + " Prg stop:" + prog.getStopDay().ordinal());				
				if(startDay >= prog.getStartDay().ordinal() && startDay <= prog.getStopDay().ordinal()) {
					if(startSegInDayMillis >=  startPrgMillis && startSegInDayMillis <= stopPrgMillis) {
						return true;
					}
				}
			}
			else {
				logger.warn("Cycle not yet implemented:" + prog.getCycle().name());
			}
		}
		return false;
	}
}
