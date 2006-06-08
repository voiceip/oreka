=================================================
Welcome to Oreka, an open media capture and retrieval platform

Copyright (C) 2005, orecx LLC	http://www.orecx.com

This program is free software, distributed under the terms of
the GNU General Public License.

The platform currently comprises three services:

* orkaudio:	the audio capture and storage daemon with pluggable capture modules
		currently comes with modules for VoIP and sound device recording.

* orktrack: logs all activity from one or more orkaudio services to any mainstream database.

* orkweb:	Web based user interface for retrieval. 

To get started:

* This is the source distribution package. I you want to get going fast, get Windows or debian Linux binary packages from the project webpage.
* Refer to BUILD_C++.txt for building orkaudio and associated capture modules
* Refer to BUILD_JAVA.txt for building orktrack and orkweb

This package is organized as follows:

* orkbasecxx is the base C++ library
* orkaudio is the audio capture and storage service with pluggable modules
* orkbasej is the base java library
* orkweb is a j2ee tapestry based front-end for media retrieval 