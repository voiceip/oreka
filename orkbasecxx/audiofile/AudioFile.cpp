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

#include "AudioFile.h"
#include "ace/OS_NS_unistd.h"

AudioFile::AudioFile()
{
	m_numOutputChannels = 1;
}

void AudioFile::Open(fileOpenModeEnum mode, bool stereo , int sampleRate)
{
	Open(m_filename, mode, stereo, sampleRate);
}

void AudioFile::MoveOrig()
{
	CStdString newName = m_filename + ".orig";
	if (ACE_OS::rename((PCSTR)m_filename, (PCSTR)newName) == 0)
	{
		m_filename = newName;
	}
	else
	{
		throw(CStdString("AudioFile::MoveOrig: could not rename file:" + m_filename));
	}
}

void AudioFile::Delete()
{
	ACE_OS::unlink((PCSTR)m_filename);
}

void AudioFile::SetFilename(CStdString& filename)
{
	m_filename = filename;
}


int AudioFile::GetSampleRate()
{
	return m_sampleRate;
}

