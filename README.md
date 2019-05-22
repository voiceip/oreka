## Oreka, an opensource VoIP media capture and retrieval platform

Based on [Orecx Oreka](http://www.orecx.com/open-source/), this project tries to provide a complete Call Recording (SIPREC) solution.  

### Components
- **Orkaudio**:  
    The audio capture and storage daemon with pluggable capture modules currently comes with modules for VoIP and sound device recording.
- **Orktrack**:  
    Tracks and publishes all activity from one or more orkaudio services to any mainstream database/storage system.
- **Orkweb**:   
    Web based user interface for retrieval (UnChanged)
    
### Improvements

- Support for G729 Codec  
- CallID Tracking   
- Switch to faster logging (Log4j2)
- Upgrade to latest version of libraries.

### Building

The build tool is separately available at [github:Oreka-build](https://github.com/voiceip/oreka-build) which builds the project on a Ubuntu14.04 Virtual Box. 
You can natively build if you have all dependencies but I develop on a OSx system, so have kept it separate.

### Distribution

Debian installers are planned for release via [Bintray](https://bintray.com/voiceip/deb/oreka)

#### More Information
Read [more](README.txt)
