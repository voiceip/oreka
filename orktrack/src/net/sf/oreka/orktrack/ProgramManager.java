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
import java.util.List;

import net.sf.oreka.Direction;
import net.sf.oreka.HibernateManager;
import net.sf.oreka.persistent.RecProgram;
import net.sf.oreka.persistent.RecSegment;

import org.apache.log4j.Logger;
import org.hibernate.HibernateException;
import org.hibernate.Session;
import org.hibernate.Transaction;

public class ProgramManager {

	private Logger log = null;
	
	private static ProgramManager programManager = null;
	private ArrayList<RecProgram> recPrograms = new ArrayList<RecProgram>();
	
	private ProgramManager() {
		log = LogManager.getInstance().getPortLogger();;
	}
	
	public static ProgramManager instance() {
		
		if (programManager == null) {
			programManager = new ProgramManager();
		}
		return programManager;
	}
	
	public void load() {
		
		Session hbnSession = null;
		Transaction tx = null;
		
		try {
			hbnSession = HibernateManager.getSession();
			tx = hbnSession.beginTransaction();
			
			List progs = hbnSession.createQuery(
		    "from RecProgram as prg where prg.active=:active and prg.discarded=:discarded")
		    .setBoolean("active", true)
		    .setBoolean("discarded", false)
		    .list();
			
			recPrograms = new ArrayList<RecProgram>(progs);
			tx.commit();
		}
		catch (Exception e) {
			log.error("Could not load programs", e);
		}
		finally {
			hbnSession.close();
		}
	}
	
	public void addProgram(RecProgram prog) {
		
		recPrograms.add(prog);
	}
	
	public boolean filterSegmentAgainstAllPrograms(RecSegment seg, Session hbnSession) {
		
		boolean result = false;
		// Iterate over programs
		ArrayList<RecProgram> progs = recPrograms;
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
	
	public boolean filterSegmentAgainstProgram(RecSegment seg, RecProgram prog, Session hbnSession) {
		
		boolean drop = false;
		boolean result = false;
		
		if (	prog.getDirection() != Direction.ALL &&
				seg.getDirection() != prog.getDirection()) {
			drop = true;
		}
		
		if (	!drop && prog.getMaxDuration() > 0 &&
				(seg.getDuration() > (prog.getMaxDuration()*1000))) {
			drop = true;
		}
		
		if (	!drop && prog.getMinDuration() > 0 &&
				(seg.getDuration() < (prog.getMinDuration()*1000))) {
			drop = true;
		}
		
		if (!drop && prog.getRandomPercent() != 0.0) {
			double randomNumber = java.lang.Math.random();
			if (randomNumber > prog.getRandomPercent()) {
				drop = true;
			}
		}
		
		if (	!drop && prog.getLocalParty().length() > 0 &&
				(!seg.getLocalParty().equals(prog.getLocalParty()))   ) {
			drop = true;
		}
		
		if (	!drop && prog.getRemoteParty().length() > 0 &&
				(!seg.getRemoteParty().equals(prog.getRemoteParty()))    ) {
			drop = true;
		}
			
		if (	!drop &&
				(prog.getTargetUser() != null) &&
				(seg.getUser().getId() != prog.getTargetUser().getId())   ) {
			drop = true;
		}
		
		if (	!drop &&
				(prog.getTargetPort() != null) &&
				(seg.getPort().getId() != prog.getTargetPort().getId())   ) {
			drop = true;
		}
		
		if ( 	!drop &&
				prog.getRecordedSoFar() < prog.getRecPerCycle() )
			drop = true;
		
		if(drop == false) {
			// All criteria have passed, the segment is accepted by the program
			prog.setRecordedSoFar(prog.getRecordedSoFar() + 1);
			hbnSession.update(prog);
			seg.getRecPrograms().add(prog);
			result = true;
		}
		return result;
	}
	
	public void filterTime() {
		;
	}
}
