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
#ifdef WIN32
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
//#include "winsock2.h"
#endif
#include "AudioFile.h"
#include "Utils.h"

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
	if(std::rename((PCSTR)m_filename.c_str(), (PCSTR)newName.c_str()) == 0)
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
	std::remove((PCSTR)m_filename.c_str());
}

void AudioFile::SetFilename(CStdString& filename)
{
	m_filename = filename;
}


int AudioFile::GetSampleRate()
{
	return m_sampleRate;
}

