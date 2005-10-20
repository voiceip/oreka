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

#include "Utils.h"
#include "TestMsg.h"

TestMsg::TestMsg()
{
	// Here is where default values are set
	m_timestamp = 0;
}

void TestMsg::Define(Serializer* s)
{
	CStdString testMessageName("test");
	s->StringValue("test", testMessageName, true);
	s->StringValue(STAGE_PARAM, m_stage, true);
	s->StringValue(CAPTURE_PORT_PARAM, m_capturePort, true);
	s->IntValue(TIMESTAMP_PARAM, (int&)m_timestamp, true);
	s->StringValue(FILENAME_PARAM, m_fileName, true);

	s->StringValue(LOCALPARTY_PARAM, m_localParty);
	s->StringValue(LOCALENTRYPOINT_PARAM, m_localEntryPoint);
	s->StringValue(REMOTEPARTY_PARAM, m_remoteParty);
	s->StringValue(DIRECTION_PARAM, m_direction);
	s->CsvValue("csv", m_csv);
	s->DateValue("date", m_time);
}

void TestMsg::Validate()
{
}

CStdString TestMsg::GetClassName()
{
	return CStdString("test");
}

ObjectRef TestMsg::NewInstance()
{
	return ObjectRef(new TestMsg);
}

